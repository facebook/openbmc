import re

device_manufactor_determinator = {
    "RPU": {
        "register": 6604,
        "length": 8,
        "manufacturers": {"delta": ["", "RDF040DSS5193E0"], "quanta": ["L05T"]},
    },
    "RPU2": {"register": 192, "length": 20, "manufacturers": {"coolermaster": []}},
    "ORV3_PSU": {
        "register": 24,
        "length": 16,
        "isRegex": True,
        "manufacturers": {
            "artesyn": [r"^.{8}AE"],
            "delta": [r"^.{8}DE"],
        },
    },
    "ORV3_BBU": {
        "register": 0,
        "length": 8,
        "manufacturers": {"delta": ["Delta"], "panasonic": ["Panasonic", "ARTESYN"]},
    },
    "PSU_PMM": {
        # PMM_MFR_Name
        "register": 8,
        "length": 8,
        "manufacturers": {"delta": ["Delta"], "artesyn": ["ARTESYN"]},
    },
    "BBU_PMM": {
        # PMM_MFR_Name
        "register": 8,
        "length": 8,
        "manufacturers": {"delta": ["Delta"], "panasonic": ["Panasonic"]},
    },
    "CBU_PMM": {
        # PMM_MFR_Name
        "register": 8,
        "length": 8,
        "manufacturers": {"delta": ["Delta"]},
    },
    "PSU": {
        # PSU_MFR_Serial
        "register": 24,
        "length": 16,
        "isRegex": True,
        "manufacturers": {
            "artesyn": [r"^.{8}AE"],
            "delta": [r"^.{8}DE"],
        },
    },
    "BBU": {
        # Manufacture_Name
        "register": 0,
        "length": 8,
        "manufacturers": {"delta": ["Delta"], "panasonic": ["Panasonic"]},
    },
    "CBU": {
        # Manufacture_Name
        "register": 0,
        "length": 8,
        "manufacturers": {"delta": ["Delta"]},
    },
}


def normalize(value):
    """
    Strip the padding a device fills a fixed width string register with.

    A backend reading the wire directly returns the whole register range,
    NUL or space padded to its full width, where rackmon hands back the
    string its register map already trimmed. Only the tail is stripped:
    the regex matchers above count characters from the start, so any
    leading padding has to stay.
    """
    return value.rstrip("\x00\t\r\n ")


def get_manufacturer(dev_type, dev):
    config = device_manufactor_determinator[dev_type]
    value = normalize(dev.read_str(config["register"], config["length"]))
    isRegex = config.get("isRegex", False)
    for name, maps in config["manufacturers"].items():
        # Having no matcher means this is the only manufacturer supported.
        if len(maps) == 0:
            return name
        for pattern in maps:
            if isRegex:
                if re.search(pattern, value):
                    return name
            else:
                if pattern == value:
                    return name
    return None
