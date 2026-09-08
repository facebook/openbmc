import json
import os
import re

# How a device reports who built it is described by the modbus-device-util
# config, so this module and the shell tooling in that package agree on the
# answer. Resolution order matches those scripts: an override dropped on the
# persistent partition wins over the config shipped with the image.
CONFIG_PATH_ENV = "MODBUS_DEVICE_UTIL_CONFIG"
OVERRIDE_CONFIG_PATH = (
    "/run/mnt-persist/var-data/lib/modbus-device-util/override-config.json"
)
DEFAULT_CONFIG_PATH = "/var/lib/modbus-device-util/default-config.json"


def get_config_path():
    path = os.environ.get(CONFIG_PATH_ENV)
    if path:
        return path
    if os.path.exists(OVERRIDE_CONFIG_PATH):
        return OVERRIDE_CONFIG_PATH
    return DEFAULT_CONFIG_PATH


def get_determinator(config_file=None):
    """
    Load the config and key it by the device type modbus-update.py works in.

    The config is keyed by the name the shell tooling passes around (psu,
    rpu2, hpr_pmm_bbu, ...), each entry naming the device type it describes.
    Entries without one are not devices this module can identify.
    """
    with open(config_file or get_config_path()) as f:
        config = json.load(f)
    return {
        entry["deviceType"]: entry for entry in config.values() if "deviceType" in entry
    }


def normalize(value):
    """
    Strip the padding a device fills a fixed width string register with.

    A backend reading the wire directly returns the whole register range,
    NUL or space padded to its full width, where rackmon hands back the
    string its register map already trimmed. Only the tail is stripped:
    the regex matchers in the config count characters from the start, so
    any leading padding has to stay.
    """
    return value.rstrip("\x00\t\r\n ")


def get_manufacturer(dev_type, dev, config_file=None):
    path = config_file or get_config_path()
    configs = get_determinator(path)
    # An override on the persistent partition outlives the image it was
    # dropped on, so it can predate the device type we were asked about.
    # The config shipped with the image always knows what the image knows.
    if dev_type not in configs and path != DEFAULT_CONFIG_PATH:
        if os.path.exists(DEFAULT_CONFIG_PATH):
            print(f"{dev_type} is not in {path}, reverting to the default config")
            configs = get_determinator(DEFAULT_CONFIG_PATH)
    config = configs[dev_type]
    value = normalize(
        dev.read_str(
            config["manufacturerDiscriminatorRegister"],
            config["manufacturerDiscriminatorLength"],
        )
    )
    for name, matcher in config["manufacturers"].items():
        regex = matcher.get("registerRegex")
        if regex is not None:
            if re.search(regex, value):
                return name
            continue
        for pattern in matcher.get("registerValues", []):
            if pattern == value:
                return name
    return None
