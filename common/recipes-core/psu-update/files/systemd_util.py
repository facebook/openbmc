"""
Talk to systemd and to D-Bus objects over busctl.

Two layers, both used by phosphor_modbus.py:

  1. A thin wrapper over busctl: get_property(), set_property(),
     get_all_properties(), get_managed_objects(), get_subtree_paths()
     and get_interfaces(), all of which raise ConfigError when the call
     fails or its result cannot be understood.
  2. is_unit_running(), which asks the systemd manager whether a unit is
     active.

busctl is used instead of a python D-Bus binding so this does not add a
new runtime dependency to the image.
"""

import json
import subprocess
from typing import Any, Dict, List
from xml.etree import ElementTree

BUSCTL = "busctl"
BUSCTL_TIMEOUT = 10.0

PROPERTIES_IFACE = "org.freedesktop.DBus.Properties"
INTROSPECTABLE_IFACE = "org.freedesktop.DBus.Introspectable"
OBJECT_MANAGER_IFACE = "org.freedesktop.DBus.ObjectManager"

OBJECT_MAPPER_SERVICE = "xyz.openbmc_project.ObjectMapper"
OBJECT_MAPPER_PATH = "/xyz/openbmc_project/object_mapper"
OBJECT_MAPPER_IFACE = "xyz.openbmc_project.ObjectMapper"

SYSTEMD_SERVICE = "org.freedesktop.systemd1"
SYSTEMD_PATH = "/org/freedesktop/systemd1"
SYSTEMD_MANAGER_IFACE = "org.freedesktop.systemd1.Manager"
SYSTEMD_UNIT_IFACE = "org.freedesktop.systemd1.Unit"


class ConfigError(Exception):
    """Raised when something cannot be read from or written to D-Bus"""


def _unwrap(value: Any) -> Any:
    """
    Strip the {"type": ..., "data": ...} wrappers busctl puts around
    messages and variants, recursively.
    """
    if isinstance(value, dict):
        if "data" in value and set(value) <= {"type", "data"}:
            return _unwrap(value["data"])
        return {k: _unwrap(v) for k, v in value.items()}
    elif isinstance(value, list):
        return [_unwrap(v) for v in value]
    return value


def _busctl_call(
    service, path, interface, method, signature=None, *signatureArgs
) -> List[Any]:
    """Call a method and return its (unwrapped) return values"""
    cmd = [BUSCTL, "--json=short", "call", service, path, interface, method]
    if signature:
        cmd += [signature] + [str(a) for a in signatureArgs]
    try:
        proc = subprocess.run(
            cmd,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            timeout=BUSCTL_TIMEOUT,
        )
    except subprocess.TimeoutExpired as e:
        raise ConfigError("%s.%s on %s timed out" % (interface, method, path)) from e
    if proc.returncode != 0:
        raise ConfigError(
            "%s.%s on %s failed: %s"
            % (interface, method, path, proc.stderr.decode(errors="replace").strip())
        )
    out = proc.stdout.decode(errors="replace").strip()
    if not out:
        return []
    try:
        return _unwrap(json.loads(out))
    except ValueError as e:
        raise ConfigError("Could not parse busctl output for %s: %s" % (path, e)) from e


def get_property(service, path, interface, prop) -> Any:
    """Read a single property"""
    ret = _busctl_call(service, path, PROPERTIES_IFACE, "Get", "ss", interface, prop)
    if not ret:
        raise ConfigError("%s.%s on %s returned nothing" % (interface, prop, path))
    return ret[0]


def get_all_properties(service, path, interface) -> Dict[str, Any]:
    """Read every property of an interface"""
    ret = _busctl_call(service, path, PROPERTIES_IFACE, "GetAll", "s", interface)
    if not ret or not isinstance(ret[0], dict):
        raise ConfigError("No properties of %s on %s" % (interface, path))
    return ret[0]


def set_property(service, path, interface, prop, signature, value) -> None:
    """Write a single property"""
    _busctl_call(
        service,
        path,
        PROPERTIES_IFACE,
        "Set",
        "ssv",
        interface,
        prop,
        signature,
        value,
    )


def get_managed_objects(service, path) -> Dict[str, Dict[str, Any]]:
    """Enumerate the objects exported below path by its ObjectManager"""
    ret = _busctl_call(service, path, OBJECT_MANAGER_IFACE, "GetManagedObjects")
    if not ret or not isinstance(ret[0], dict):
        raise ConfigError("Could not enumerate objects under %s" % path)
    return ret[0]


def get_subtree_paths(subtree, interfaces, depth=0) -> List[str]:
    """
    Ask the mapper for the object paths below subtree which implement
    every one of interfaces. depth 0 is the whole subtree.

    Unlike get_managed_objects() this does not need to know which
    service exports the objects, nor where its ObjectManager sits.
    """
    ret = _busctl_call(
        OBJECT_MAPPER_SERVICE,
        OBJECT_MAPPER_PATH,
        OBJECT_MAPPER_IFACE,
        "GetSubTreePaths",
        "sias",
        subtree,
        depth,
        len(interfaces),
        *interfaces,
    )
    if not ret or not isinstance(ret[0], list):
        raise ConfigError("Could not enumerate objects under %s" % subtree)
    return ret[0]


def get_interfaces(service, path) -> List[str]:
    """List the interfaces implemented by an object"""
    ret = _busctl_call(service, path, INTROSPECTABLE_IFACE, "Introspect")
    if not ret:
        raise ConfigError("Could not introspect %s" % path)
    try:
        node = ElementTree.fromstring(ret[0])
    except ElementTree.ParseError as e:
        raise ConfigError("Bad introspection XML for %s: %s" % (path, e)) from e
    return [
        iface.attrib["name"]
        for iface in node.findall("interface")
        if "name" in iface.attrib
    ]


def is_unit_running(unit: str) -> bool:
    """True if the systemd unit is active"""
    try:
        ret = _busctl_call(
            SYSTEMD_SERVICE,
            SYSTEMD_PATH,
            SYSTEMD_MANAGER_IFACE,
            "GetUnit",
            "s",
            unit,
        )
        if not ret:
            return False
        unit_path = ret[0]
        active_state = get_property(
            SYSTEMD_SERVICE, unit_path, SYSTEMD_UNIT_IFACE, "ActiveState"
        )
        return active_state == "active"
    except ConfigError:
        # not loaded or running.
        return False
