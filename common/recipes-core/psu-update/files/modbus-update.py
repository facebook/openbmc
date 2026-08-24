#!/usr/bin/env python3

import argparse
import re
import sys

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
from modbus_monitor import RackmonMonitor
from modbus_update_helper import auto_int
from pyrmd import RackmonInterface as rmd
from rpu_update_coolermaster import AALCV2_COMPONENTS


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
    parser.add_argument(
        "-n",
        "--name",
        type=str,
        required=False,
        default=None,
        help="Device Name",
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
        "-a",
        "--address",
        type=auto_int,
        help="Rackmon Unique Device Address (also forces rackmon)",
        default=None,
        required=False,
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


# From an interface perspective, any device with position (shelf#) >= 100
# is considered a legacy ORV3 device.
def is_rackmon_position(device_position):
    return device_position >= 100


def get_rackmon_device_uaddr(device_type, device_position, device_number):
    if device_type == "RPU":
        if device_number is not None:
            raise ValueError("RPU does not have a device number")
        upper = device_position + 1 - 100  # convert 100-102 to 1-3
        lower = 0xC
        return upper << 8 | lower
    if device_type == "RPU2":
        if device_number is not None:
            raise ValueError("RPU does not have a device number")
        upper = device_position + 1 - 200  # convert 100-102 to 1-3
        lower = 0xD
        return upper << 8 | lower
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


def get_rackmon_device(
    name, device_type, device_position, device_number, uaddr, force_direct=False
):
    if uaddr is None:
        uaddr = get_rackmon_device_uaddr(device_type, device_position, device_number)
    config = get_rackmon_device_config(uaddr)
    if not force_direct:
        return ModbusRackmon(uaddr)
    if ModbusDirect is None:
        raise ValueError("minimalmodbus not installed")
    baud = config["baudrate"]
    addr = config["devAddress"]
    parity = config["parity"]
    devpath = rmd.get_interface(uaddr)
    return ModbusDirect(addr, baud, parity, devpath, RackmonMonitor())


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


def get_device(name, uaddr, force_direct=False):
    device_type, device_position, device_number = decode_name(name)
    if uaddr or is_rackmon_position(device_position):
        if device_type in ["PSU", "BBU"]:
            device_type = "ORV3_" + device_type
        elif device_type in ["RPU"]:
            device_type = "RPU2" if device_position >= 200 else "RPU"
        else:
            raise ValueError(f"Unknown device type: {device_type}")
        dev = get_rackmon_device(
            name, device_type, device_position, device_number, uaddr, force_direct
        )
    else:
        dev = get_phosphor_modbus_device(name)
    return device_type, dev


def main():
    args = parse_args()

    device_type, dev = get_device(args.name, args.address, args.force_direct)
    with dev.suppress_monitoring():
        device_vendor = manufacturers.get_manufacturer(device_type, dev)
        update = get_updater(device_type, device_vendor, args.component)

        if args.dry_run:
            print(f"Dry run, not updating {args.name}:")
            print(f"  Device Type: {device_type}")
            print(f"  Vendor: {device_vendor}")
            print(f"  Backend: {type(dev).__module__}")
            print(f"  File: {args.file}")
            print(f"  Updater: {getattr(update, 'description', update.__name__)}")
            return
        update(dev, args.file)


if __name__ == "__main__":
    main()
