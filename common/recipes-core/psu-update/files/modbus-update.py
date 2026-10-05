#!/usr/bin/env python3

import argparse
import fcntl
import os
import re
import sys
from contextlib import contextmanager

import manufacturers
import orv3_device_update_mailbox
import phosphor_modbus
import psu_update_aei
import psu_update_delta_orv3
import rpu_update_coolermaster
import rpu_update_delta_hex
import rpu_update_delta_plc

try:
    # If minimalmodbus is installed, enable ModbusDirect
    import minimalmodbus
    from modbus_impl_minimalmodbus import Modbus as ModbusDirect
except ImportError:
    ModbusDirect = None
from modbus_impl_pyrmd import Modbus as ModbusRackmon
from modbus_monitor import get_rackmon_interface, RackmonMonitor
from modbus_update_helper import auto_int
from pyrmd import RackmonInterface as rmd
from rpu_update_coolermaster import AALCV2_COMPONENTS

# Tools which drive the modbus ports serialize against each other with
# flock on this file. rackmond creates it at startup and points the
# legacy /tmp/modbus_dynamo_solitonbeam.lock at it.
MODBUS_LOCK = "/run/lock/modbus.lock"


def inherited_lock_fd(path):
    """
    A descriptor for the lock file this process was started with.

    Callers used to have to wrap this script in
    `flock /tmp/modbus_dynamo_solitonbeam.lock ...`, and some still do.
    flock(1) holds the lock on a descriptor its command inherits, and
    the /tmp path is a link to the same file, so opening the file afresh
    and locking it would wait on our own parent forever. Locking the
    inherited descriptor instead succeeds, as it is the one holding it.
    """
    try:
        lock = os.stat(path)
    except FileNotFoundError:
        return None
    for name in os.listdir("/proc/self/fd"):
        try:
            st = os.fstat(int(name))
        except OSError:
            # The descriptor listdir() read the directory through
            continue
        if (st.st_dev, st.st_ino) == (lock.st_dev, lock.st_ino):
            return int(name)
    return None


@contextmanager
def modbus_lock(path):
    """Hold the modbus lock, waiting for whoever has it to let it go"""
    fd = inherited_lock_fd(path)
    inherited = fd is not None
    if not inherited:
        fd = os.open(path, os.O_RDONLY | os.O_CREAT, 0o644)
    try:
        try:
            fcntl.flock(fd, fcntl.LOCK_EX | fcntl.LOCK_NB)
        except BlockingIOError:
            print(f"Waiting for {path}", flush=True)
            fcntl.flock(fd, fcntl.LOCK_EX)
        yield
    finally:
        # An inherited lock is our parent's to release, closing our own
        # descriptor releases ours.
        if not inherited:
            os.close(fd)


def _mailbox(variant):
    def update(dev, path):
        orv3_device_update_mailbox.main(dev, path, variant)

    update.description = f"orv3_device_update_mailbox.main(variant={variant})"
    return update


def _aei(variant):
    def update(dev, path):
        psu_update_aei.main(dev, path, variant)

    update.description = f"psu_update_aei.main(variant={variant})"
    return update


def _delta_orv3(dev, path):
    psu_update_delta_orv3.main(dev, path)


_delta_orv3.description = "psu_update_delta_orv3.main"


_PMM_VENDORS = {
    "delta": _mailbox("hpr_pmm_delta"),
    "artesyn": _mailbox("hpr_pmm_aei"),
    "panasonic": _mailbox("hpr_pmm_panasonic"),
}

# device type -> (name used in error messages, {vendor: update function})
UPDATERS = {
    "ORV3_PSU": ("PSU", {"artesyn": _aei("orv3"), "delta": _delta_orv3}),
    "ORV3_BBU": (
        "BBU",
        {"panasonic": _mailbox("panasonic"), "delta": _mailbox("delta")},
    ),
    "PSU_PMM": ("PMM", _PMM_VENDORS),
    "BBU_PMM": ("PMM", _PMM_VENDORS),
    "CBU_PMM": ("PMM", _PMM_VENDORS),
    "PSU": ("PSU", {"delta": _delta_orv3, "artesyn": _aei("hpr")}),
    "BBU": (
        "BBU",
        {"delta": _mailbox("delta"), "panasonic": _mailbox("hpr_panasonic")},
    ),
    "CBU": ("CBU", {"delta": _mailbox("delta_cbu")}),
}


def rpu_updater(vendor, component):
    component = "PLC" if component is None else component
    if component == "PLC":

        def update(dev, path):
            rpu_update_delta_plc.main(dev, path, vendor == "delta")

        update.description = f"rpu_update_delta_plc.main(is_delta={vendor == 'delta'})"
        return update
    if component == "HEX":

        def update(dev, path):
            rpu_update_delta_hex.main(dev, path)

        update.description = "rpu_update_delta_hex.main"
        return update
    print(f"Unsupported RPU component: {component}")
    sys.exit(1)


def rpu2_updater(component):
    # component is optional here, the updater derives it from the image name
    # when it is not given.
    if component is not None and component not in AALCV2_COMPONENTS:
        print(f"Unsupported RPU2 component: {component}")
        sys.exit(1)

    def update(dev, path):
        rpu_update_coolermaster.main(dev, path, component)

    update.description = f"rpu_update_coolermaster.main(component={component})"
    return update


def get_updater(device_type, device_vendor, component):
    if device_type == "RPU":
        return rpu_updater(device_vendor, component)
    if device_type == "RPU2":
        return rpu2_updater(component)
    if device_type not in UPDATERS:
        print(f"Unsupported device type: {device_type}")
        sys.exit(1)
    name, vendors = UPDATERS[device_type]
    if device_vendor not in vendors:
        print(f"Unsupported {name} Vendor: {device_vendor}")
        sys.exit(1)
    return vendors[device_vendor]


def parse_args():
    parser = argparse.ArgumentParser()
    parser.add_argument("file", help="firmware file")
    device = parser.add_mutually_exclusive_group(required=True)
    device.add_argument(
        "-n",
        "--name",
        type=str,
        default=None,
        help="Device Name",
    )
    device.add_argument(
        "-a",
        "--address",
        type=auto_int,
        help=(
            "Rackmon Unique Device Address. Forces rackmon, which is asked "
            "what the device at that address is"
        ),
        default=None,
    )
    parser.add_argument(
        "-c",
        "--component",
        type=str,
        required=False,
        default=None,
        help=(
            "Component to update (Most devices dont have sub components "
            "but things like RPU does)"
        ),
    )
    parser.add_argument(
        "--dry-run",
        action="store_true",
        help=(
            "Resolve the device, detect its vendor and pick the updater, "
            "but stop before running the update"
        ),
    )
    parser.add_argument(
        "--force-direct",
        action="store_true",
        help=(
            "Forces the upgrade to use the direct modbus implementation, "
            "even if its managed by rackmon"
        ),
    )

    return parser.parse_args()


# Rackmon names a device after the register map it matched during the
# scan. Mapped onto the device types this script works in, that is what
# lets an address on its own identify a device.
RACKMON_DEVICE_TYPES = {
    "ORV3_PSU": "ORV3_PSU",
    "ORV3_BBU": "ORV3_BBU",
    "ORV3_RPU": "RPU",
    "ORV3_RPU2": "RPU2",
    "ORV3_HPR_PSU": "PSU",
    "ORV3_HPR_BBU": "BBU",
    "ORV3_HPR_CBU": "CBU",
    "ORV3_HPR_PMM_PSU": "PSU_PMM",
    "ORV3_HPR_PMM_BBU": "BBU_PMM",
    "ORV3_HPR_PMM_CBU": "CBU_PMM",
}


# From an interface perspective, any device with position (shelf#) >= 100
# is considered a legacy ORV3 device.
def is_rackmon_position(device_position):
    return device_position >= 100


def get_rackmon_orv3_device_uaddr(device_type, device_position, device_number):
    if device_type in ["ORV3_PSU", "ORV3_BBU"]:
        dtype = 3 if device_type == "ORV3_PSU" else 1
        dnum = device_number - 1
        r2 = 1 if device_type == "ORV3_PSU" else 0
        rack_num = device_position - 100
        upper = rack_num + 1
        return (upper << 8) | (dtype << 6) | (r2 << 5) | (rack_num << 3) | dnum
    raise ValueError(f"Unknown device type: {device_type}")


def get_rackmon_device_config(uaddr):
    dlist = rmd.list()
    for dev in dlist:
        if dev["uniqueDevAddress"] == uaddr:
            return dev
    raise ValueError(f"Unknown address: {uaddr}")


def get_rackmon_rpu_device_uaddr(device_position):
    """
    The address of the RPU a shelf names.

    Both generations live on shelves 100-102 and neither is derivable
    from the name, so the address comes off rackmon's device list.
    The same pod cannot contain both generations, so we can use the
    device type to disambiguate.

    AALCv1 puts one RPU on each rack, and a unique device address is
    port << 8 | address with the port being the rack, so the shelf picks
    between them.

    An AALCv2 cooling pod is a single modbus device -- the master
    controller -- fronting every rack assembly in the pod. oobit reports
    its components across shelves 100-102 so they do not collide, but
    they are all that one device, so any of those shelves resolves to it.
    Which assembly to write is --component's business, not the shelf's.
    """
    rack = device_position + 1 - 100  # convert 100-102 to 1-3
    for dev in rmd.list():
        device_type = RACKMON_DEVICE_TYPES.get(dev["deviceType"])
        if device_type == "RPU" and dev["uniqueDevAddress"] >> 8 == rack:
            return dev["uniqueDevAddress"]
        if device_type == "RPU2":
            return dev["uniqueDevAddress"]
    raise ValueError(f"Rackmon has no RPU at position {device_position}")


def make_rackmon_device(uaddr, config, force_direct=False):
    if not force_direct:
        return ModbusRackmon(uaddr)
    if ModbusDirect is None:
        raise ValueError("minimalmodbus not installed")
    baud = config["baudrate"]
    addr = config["devAddress"]
    parity = config["parity"]
    devpath = get_rackmon_interface(uaddr)
    return ModbusDirect(addr, baud, parity, devpath, RackmonMonitor())


def get_rackmon_device_by_addr(uaddr, force_direct=False):
    """
    Resolve a device from its rackmon address alone.

    Rackmon already scanned the bus and matched the device against a
    register map, so the device type is something it knows: the register
    map name is authoritative and no name has to be given.
    """
    config = get_rackmon_device_config(uaddr)
    rackmon_type = config["deviceType"]
    if rackmon_type not in RACKMON_DEVICE_TYPES:
        raise ValueError(f"Unsupported rackmon device type: {rackmon_type}")
    return (
        RACKMON_DEVICE_TYPES[rackmon_type],
        make_rackmon_device(uaddr, config, force_direct),
    )


def get_pmodbus_config(name):
    cfgs = phosphor_modbus.get_all_device_configs()
    return cfgs[name]


def get_phosphor_modbus_device(name):
    cfg = get_pmodbus_config(name)
    baud = cfg.baudrate
    addr = cfg.address
    devpath = cfg.device_path
    parity = cfg.parity_char
    if ModbusDirect is None:
        raise ValueError("minimalmodbus not installed")
    return ModbusDirect(addr, baud, parity, devpath)


def decode_name(name):
    parts = re.match(r"^([a-zA-Z_]+)_([0-9]+)_?([0-9]+)?", name)
    if parts is None:
        raise ValueError(f"Invalid device name: {name}")
    device_type = parts.group(1).upper()
    device_position = int(parts.group(2))
    device_number = None if parts.group(3) is None else int(parts.group(3))
    return device_type, device_position, device_number


def name_to_rackmon_uaddr(device_type, device_position, device_number):
    """
    Where a name sits on the bus.

    The rackmon naming scheme describes ORv3 parts, so PSU and BBU mean
    their ORv3 variants here and their address is derived from the name.
    An RPU's is not, see get_rackmon_rpu_device_uaddr(). Either way,
    what the device turns out to be is rackmon's answer, not this one.
    """
    if device_type in ["PSU", "BBU"]:
        return get_rackmon_orv3_device_uaddr(
            "ORV3_" + device_type, device_position, device_number
        )
    if device_type == "RPU":
        if device_number is not None:
            raise ValueError("RPU does not have a device number")
        return get_rackmon_rpu_device_uaddr(device_position)
    raise ValueError(f"Unknown device type: {device_type}")


def get_device(name, uaddr, force_direct=False):
    # Without a name there is nothing to derive an address from, so the
    # address we were given has to be a rackmon one and rackmon is asked
    # what sort of device sits behind it. The two are mutually exclusive,
    # see parse_args().
    if name is None:
        return get_rackmon_device_by_addr(uaddr, force_direct)
    device_type, device_position, device_number = decode_name(name)
    if not is_rackmon_position(device_position):
        return device_type, get_phosphor_modbus_device(name)
    uaddr = name_to_rackmon_uaddr(device_type, device_position, device_number)
    # Rackmon probed the device and matched it against a register map,
    # which is better evidence of what it is than the name we were
    # handed. It is also the only way to tell an HPR part from the ORv3
    # part of the same name.
    return get_rackmon_device_by_addr(uaddr, force_direct)


def main():
    args = parse_args()

    with modbus_lock(MODBUS_LOCK):
        run(args)


def run(args):
    device_type, dev = get_device(args.name, args.address, args.force_direct)
    with dev.suppress_monitoring():
        device_vendor = manufacturers.get_manufacturer(device_type, dev)
        update = get_updater(device_type, device_vendor, args.component)

        if args.name is not None:
            print(f"Updating Name: {args.name} Component: {args.component}")
        else:
            print(f"Updating Address: {hex(args.address)} Component: {args.component}")
        print(f"  File: {args.file}")
        print(f"  Device Type: {device_type}")
        print(f"  Device: {dev}")
        print(f"  Detected vendor: {device_vendor}")
        print(f"  Backend: {type(dev).__module__}")
        print(f"  Updater: {getattr(update, 'description', update.__name__)}")
        if args.dry_run:
            target = args.name if args.name is not None else hex(args.address)
            print(f"  Dry run, not updating {target}")
            return
        update(dev, args.file)


if __name__ == "__main__":
    main()
