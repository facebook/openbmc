#!/usr/bin/env python3
"""
Discover the modbus transport of an inventory item from D-Bus.

Given an inventory name (eg. "BBU_1_1"):

  1. Read the Associations property of
     xyz.openbmc_project.Association.Definitions on
     /xyz/openbmc_project/inventory/system/chassis/<name>
     of xyz.openbmc_project.ModbusRTU, and follow the "contained_by"
     association to the board which holds the device.
  2. The entity-manager configuration of the device lives at
     <board path>/<name>. Read Address, SerialPort and Type from the
     xyz.openbmc_project.Configuration.* interface(s) there.
  3. The line settings (baud rate and parity) are not on D-Bus. They
     come from the phosphor-modbus device profile of the Type, which is
     PROFILE_DIR/<Type>.json.

Every device may be enumerated at once with get_all_device_configs(),
which walks the objects entity-manager exports instead of starting from
an inventory name.

This module also provides PhosphorModbusExclusion, which stops
phosphor-modbus from polling a given serial port. It is the python
equivalent of PhosphorModbusExclusion in
common/recipes-core/rackmon2/rackmon/ModbusUtil.cpp.

D-Bus and systemd are reached through systemd_util.py, which wraps
busctl rather than a python D-Bus binding so this does not add a new
runtime dependency to the image.
"""

import argparse
import json
import os
import sys
import time
from typing import Any, Dict, List, NamedTuple, Optional, Tuple

from systemd_util import (
    ConfigError,
    get_all_properties,
    get_interfaces,
    get_managed_objects,
    get_property,
    get_subtree_paths,
    is_unit_running,
    set_property,
)

__all__ = [
    "ConfigError",
    "DeviceConfig",
    "MODBUS_UNIT",
    "PhosphorModbusExclusion",
    "get_all_device_configs",
    "get_device_config",
    "is_unit_running",
]

PROFILE_DIR = "/usr/share/phosphor-modbus/profiles"

# Parity as spelled in a device profile, as pyserial (and so
# minimalmodbus) spells it.
PARITY_CHARS = {"None": "N", "Even": "E", "Odd": "O"}

INVENTORY_SERVICE = "xyz.openbmc_project.ModbusRTU"
INVENTORY_CHASSIS_PATH = "/xyz/openbmc_project/inventory/system/chassis"
ENTITY_MANAGER_SERVICE = "xyz.openbmc_project.EntityManager"
ENTITY_MANAGER_INVENTORY_PATH = "/xyz/openbmc_project/inventory"

ASSOCIATIONS_IFACE = "xyz.openbmc_project.Association.Definitions"
CONFIGURATION_IFACE_PREFIX = "xyz.openbmc_project.Configuration."
CONTAINED_BY = "contained_by"

MODBUS_SERVICE = INVENTORY_SERVICE
MODBUS_UNIT = MODBUS_SERVICE + ".service"
PORT_NAMESPACE = "/xyz/openbmc_project/inventory/system/connector"
PORT_IFACE = "xyz.openbmc_project.Object.Enable"
PORT_ENABLED = "Enabled"

# How long to wait for the service to report back a value we wrote.
PORT_SETTLE_TIMEOUT_SECS = 5.0
PORT_SETTLE_POLL_SECS = 0.1


class DeviceConfig(NamedTuple):
    """Modbus transport of a single inventory item"""

    name: str
    address: int
    serial_port: str
    types: List[str]
    board_path: str
    config_path: str
    # From the device profile, None if it could not be read.
    baudrate: Optional[int] = None
    parity: Optional[str] = None

    @property
    def device_path(self) -> str:
        """SerialPort as a path under /dev"""
        if self.serial_port.startswith("/"):
            return self.serial_port
        return "/dev/" + self.serial_port

    @property
    def parity_char(self) -> Optional[str]:
        """Parity the way pyserial spells it (eg. "E"), if known"""
        if self.parity is None:
            return None
        return PARITY_CHARS.get(self.parity)

    def __str__(self) -> str:
        return "%s: addr 0x%02X on %s @ %s %s (%s)" % (
            self.name,
            self.address,
            self.device_path,
            self.baudrate if self.baudrate is not None else "?",
            self.parity if self.parity is not None else "?",
            ", ".join(self.types),
        )


_profiles = {}  # type: Dict[str, Optional[Dict[str, Any]]]


def get_device_profile(device_type: str) -> Optional[Dict[str, Any]]:
    """
    Read the phosphor-modbus profile of a configuration Type, or None if
    it is not installed or cannot be parsed. Results are cached, since
    every device of a type shares one profile.
    """
    if device_type in _profiles:
        return _profiles[device_type]

    path = os.path.join(PROFILE_DIR, device_type + ".json")
    profile = None  # type: Optional[Dict[str, Any]]
    try:
        with open(path) as f:
            profile = json.load(f)
    except OSError as e:
        print("WARNING: Could not read %s: %s" % (path, e), file=sys.stderr)
    except ValueError as e:
        print("WARNING: Could not parse %s: %s" % (path, e), file=sys.stderr)
    if profile is not None and not isinstance(profile, dict):
        print("WARNING: %s is not a JSON object" % path, file=sys.stderr)
        profile = None

    _profiles[device_type] = profile
    return profile


def get_line_settings(types: List[str]) -> Tuple[Optional[int], Optional[str]]:
    """
    Get (baudrate, parity) from the device profiles of a configuration's
    types. Either is None if no profile provides it; types whose profiles
    disagree are reported and the first setting is kept.
    """
    baudrate = None  # type: Optional[int]
    parity = None  # type: Optional[str]
    for device_type in types:
        profile = get_device_profile(device_type)
        if not profile:
            continue
        this_baudrate = profile.get("BaudRate")
        this_parity = profile.get("Parity")
        if this_baudrate is not None:
            if baudrate is None:
                baudrate = int(this_baudrate)
            elif baudrate != int(this_baudrate):
                print(
                    "WARNING: BaudRate of %s is %s, not %s as an earlier profile of %s"
                    % (device_type, this_baudrate, baudrate, ", ".join(types)),
                    file=sys.stderr,
                )
        if this_parity is not None:
            if parity is None:
                parity = str(this_parity)
            elif parity != str(this_parity):
                print(
                    "WARNING: Parity of %s is %s, not %s as an earlier profile of %s"
                    % (device_type, this_parity, parity, ", ".join(types)),
                    file=sys.stderr,
                )
    return (baudrate, parity)


def get_containing_board(name: str) -> str:
    """
    Follow the "contained_by" association of an inventory item to the
    board which holds it.
    """
    path = INVENTORY_CHASSIS_PATH + "/" + name
    associations = get_property(
        INVENTORY_SERVICE, path, ASSOCIATIONS_IFACE, "Associations"
    )
    for assoc in associations:
        # Each association is a (forward, reverse, endpoint) struct.
        if len(assoc) == 3 and assoc[0] == CONTAINED_BY:
            return assoc[2]
    raise ConfigError("%s has no '%s' association" % (path, CONTAINED_BY))


def _device_config_from_interfaces(
    config_path: str, interfaces: Dict[str, Dict[str, Any]]
) -> DeviceConfig:
    """
    Build a DeviceConfig from the interfaces of an entity-manager
    configuration object at <board path>/<name>.

    A device may be described by more than one configuration interface
    (one per part number it can be probed as). They must all agree on
    how to reach the device.
    """
    board_path, _, name = config_path.rpartition("/")

    types = []
    configs = {}
    for iface, props in sorted(interfaces.items()):
        if not iface.startswith(CONFIGURATION_IFACE_PREFIX):
            continue
        if "Address" not in props or "SerialPort" not in props:
            continue
        types.append(props.get("Type", iface[len(CONFIGURATION_IFACE_PREFIX) :]))
        configs.setdefault(
            (int(props["Address"]), str(props["SerialPort"])), []
        ).append(iface)

    if not configs:
        raise ConfigError(
            "No %s* interface with Address and SerialPort on %s"
            % (CONFIGURATION_IFACE_PREFIX, config_path)
        )
    if len(configs) > 1:
        raise ConfigError(
            "%s has conflicting configurations: %s"
            % (
                config_path,
                "; ".join(
                    "addr 0x%02X on %s from %s" % (addr, port, ", ".join(ifaces))
                    for (addr, port), ifaces in configs.items()
                ),
            )
        )

    (address, serial_port) = next(iter(configs))
    (baudrate, parity) = get_line_settings(types)
    return DeviceConfig(
        name=name,
        address=address,
        serial_port=serial_port,
        types=types,
        board_path=board_path,
        config_path=config_path,
        baudrate=baudrate,
        parity=parity,
    )


def get_device_config(name: str) -> DeviceConfig:
    """Get the modbus transport of the named inventory item"""
    if not name or "/" in name:
        raise ConfigError("Invalid inventory name: %r" % name)

    config_path = get_containing_board(name) + "/" + name
    interfaces = {
        iface: get_all_properties(ENTITY_MANAGER_SERVICE, config_path, iface)
        for iface in get_interfaces(ENTITY_MANAGER_SERVICE, config_path)
        if iface.startswith(CONFIGURATION_IFACE_PREFIX)
    }
    return _device_config_from_interfaces(config_path, interfaces)


def get_all_device_configs() -> Dict[str, DeviceConfig]:
    """
    Get the modbus transport of every configured device, keyed by
    inventory name.

    Unlike get_device_config() this does not need an inventory name, and
    takes a single D-Bus call: it enumerates the objects entity-manager
    exports and keeps those carrying a configuration interface with both
    Address and SerialPort. Objects which cannot be interpreted are
    skipped with a warning rather than failing the whole enumeration.
    """
    objects = get_managed_objects(ENTITY_MANAGER_SERVICE, ENTITY_MANAGER_INVENTORY_PATH)

    configs = {}  # type: Dict[str, DeviceConfig]
    for path, interfaces in sorted(objects.items()):
        if not any(
            iface.startswith(CONFIGURATION_IFACE_PREFIX)
            and "Address" in props
            and "SerialPort" in props
            for iface, props in interfaces.items()
        ):
            continue
        try:
            config = _device_config_from_interfaces(path, interfaces)
        except (ConfigError, TypeError, ValueError) as e:
            print("WARNING: Skipping %s: %s" % (path, e), file=sys.stderr)
            continue
        if config.name in configs:
            print(
                "WARNING: Ignoring %s, %s is already configured by %s"
                % (path, config.name, configs[config.name].config_path),
                file=sys.stderr,
            )
            continue
        configs[config.name] = config
    return configs


class PhosphorModbusExclusion:
    """
    Stop phosphor-modbus from polling a serial port.

    stop() clears Enabled on every port connector object of
    xyz.openbmc_project.ModbusRTU which refers to the given device port,
    start() re-enables exactly those it disabled. Both are no-ops if the
    service is not active, or if it does not own the port.
    """

    def __init__(self, device_port: str):
        # D-Bus object paths cannot have '-'.
        self.tty_name = os.path.basename(device_port).replace("-", "_")
        self.device_port = device_port
        self.changed_paths = []  # type: List[str]
        self._stopped = False

    def get_port_paths(self) -> List[str]:
        """Port connector objects which refer to our tty"""
        # The connectors sit under the service's inventory ObjectManager
        # rather than under PORT_NAMESPACE, so ask the mapper instead of
        # enumerating an ObjectManager here.
        paths = get_subtree_paths(PORT_NAMESPACE, [PORT_IFACE])
        return sorted(p for p in paths if os.path.basename(p) == self.tty_name)

    def _read_property(self, path: str) -> bool:
        return bool(get_property(MODBUS_SERVICE, path, PORT_IFACE, PORT_ENABLED))

    def _change_property(self, path: str, enable: bool) -> None:
        set_property(
            MODBUS_SERVICE,
            path,
            PORT_IFACE,
            PORT_ENABLED,
            "b",
            "true" if enable else "false",
        )
        # The write only queues the change; the port is not ours until
        # the service reports the new value back.
        deadline = time.monotonic() + PORT_SETTLE_TIMEOUT_SECS
        while self._read_property(path) != enable:
            if time.monotonic() >= deadline:
                raise ConfigError(
                    "%s on %s did not become %s within %gs"
                    % (PORT_ENABLED, path, enable, PORT_SETTLE_TIMEOUT_SECS)
                )
            time.sleep(PORT_SETTLE_POLL_SECS)

    def stop(self) -> bool:
        """
        Disable monitoring of the port. Returns True if monitoring was
        stopped and start() has something to undo.
        """
        if self._stopped:
            return True
        if not is_unit_running(MODBUS_UNIT):
            return False
        try:
            paths = self.get_port_paths()
        except ConfigError as e:
            print("WARNING: %s" % e, file=sys.stderr)
            return False
        for path in paths:
            # Record the port before writing it: a write which is
            # accepted but does not settle in time may still land, so it
            # has to be undone either way.
            self.changed_paths.append(path)
            try:
                self._change_property(path, False)
            except ConfigError as e:
                print(
                    "WARNING: Could not disable %s on %s: %s" % (PORT_ENABLED, path, e),
                    file=sys.stderr,
                )
                # Leave the service the way we found it.
                self.start()
                return False
        self._stopped = bool(self.changed_paths)
        return self._stopped

    def start(self) -> None:
        """Re-enable monitoring on the ports stop() disabled"""
        for path in self.changed_paths:
            try:
                self._change_property(path, True)
            except ConfigError as e:
                print(
                    "WARNING: Could not enable %s on %s: %s" % (PORT_ENABLED, path, e),
                    file=sys.stderr,
                )
        self.changed_paths = []
        self._stopped = False

    def enable_monitoring(self) -> List[str]:
        """
        Enable monitoring on every port object of the device port,
        whether or not stop() disabled it. Returns the paths changed.
        Used to recover by hand after an update was killed.
        """
        paths = self.get_port_paths()
        self.changed_paths = list(paths)
        self.start()
        return paths

    def get_monitoring(self) -> Dict[str, bool]:
        """Enabled of every port connector object of the device port"""
        return {path: self._read_property(path) for path in self.get_port_paths()}


def monitoring_main(args) -> int:
    """--stop-monitoring/--start-monitoring/--show-monitoring"""
    port = args.stop_monitoring or args.start_monitoring or args.show_monitoring
    exclusion = PhosphorModbusExclusion(port)
    print(
        "%s: %s"
        % (
            MODBUS_UNIT,
            "active" if is_unit_running(MODBUS_UNIT) else "not active",
        )
    )
    try:
        if args.stop_monitoring:
            if exclusion.stop():
                print("Disabled monitoring on: %s" % ", ".join(exclusion.changed_paths))
            else:
                print("Did not disable monitoring on any port of %s" % port)
        elif args.start_monitoring:
            paths = exclusion.enable_monitoring()
            if paths:
                print("Enabled monitoring on: %s" % ", ".join(paths))
            else:
                print("No port objects for %s" % port)

        states = exclusion.get_monitoring()
    except ConfigError as e:
        print("ERROR: %s" % e, file=sys.stderr)
        return 1

    if not states:
        print("No port object of %s matches %s" % (MODBUS_SERVICE, port))
        return 1
    for path, enabled in states.items():
        print("%s: %s = %s" % (path, PORT_ENABLED, enabled))
    return 0


def main():
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[1])
    parser.add_argument(
        "name",
        nargs="?",
        help="Inventory name (eg. BBU_1_1)",
    )
    parser.add_argument(
        "--all", action="store_true", help="Print every configured device instead"
    )
    parser.add_argument(
        "--json", action="store_true", help="Print the configuration as JSON"
    )
    monitoring = parser.add_mutually_exclusive_group()
    monitoring.add_argument(
        "--stop-monitoring",
        metavar="PORT",
        help="Stop phosphor-modbus monitoring the port (eg. /dev/ttyRS485-1)",
    )
    monitoring.add_argument(
        "--start-monitoring",
        metavar="PORT",
        help="Resume phosphor-modbus monitoring the port",
    )
    monitoring.add_argument(
        "--show-monitoring",
        metavar="PORT",
        help="Show whether phosphor-modbus is monitoring the port",
    )
    args = parser.parse_args()

    if args.stop_monitoring or args.start_monitoring or args.show_monitoring:
        return monitoring_main(args)

    # Enumerating every device is a different request from asking about
    # one, so make the caller say which they meant rather than treating a
    # forgotten name as "show me everything".
    if bool(args.name) == args.all:
        parser.error("give an inventory name or --all, not both or neither")

    try:
        if args.name:
            configs = [get_device_config(args.name)]
        else:
            configs = list(get_all_device_configs().values())
            if not configs:
                print("No configured modbus device", file=sys.stderr)
                return 1
    except ConfigError as e:
        print("ERROR: %s" % e, file=sys.stderr)
        return 1

    if args.json:
        out = []
        for config in configs:
            entry = config._asdict()
            entry["device_path"] = config.device_path
            entry["parity_char"] = config.parity_char
            out.append(entry)
        print(json.dumps(out[0] if args.name else out, indent=2))
    else:
        for i, config in enumerate(configs):
            if i:
                print()
            print("Name:        %s" % config.name)
            print("Address:     0x%02X (%d)" % (config.address, config.address))
            print("Serial Port: %s" % config.device_path)
            print(
                "Baud Rate:   %s"
                % (config.baudrate if config.baudrate is not None else "unknown")
            )
            print(
                "Parity:      %s"
                % (
                    "%s (%s)" % (config.parity, config.parity_char)
                    if config.parity is not None
                    else "unknown"
                )
            )
            print("Type:        %s" % ", ".join(config.types))
            print("Board:       %s" % config.board_path)
            print("Config:      %s" % config.config_path)
    return 0


if __name__ == "__main__":
    sys.exit(main())
