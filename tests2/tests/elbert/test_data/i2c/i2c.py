plat_i2c_tree = {
    "0-0050": {"name": "24c512", "driver": "at24"},
    "15-004a": {"name": "lm73", "driver": "lm73"},
    "4-004d": {"name": "max6581", "driver": "max6697"},
    "4-0050": {"name": "24c512", "driver": "at24"},
    "3-0040": {"name": "pmbus", "driver": "pmbus"},
    "3-0041": {"name": "pmbus", "driver": "pmbus"},
    "3-004e": {"name": "ucd90160", "driver": "ucd9000"},
    "3-0060": {"name": "raa228228", "driver": "isl68137"},
    "3-0062": {"name": "isl68226", "driver": "isl68137"},
    "4-0023": {"name": "smbcpld", "driver": "smbcpld"},
    "4-0051": {"name": "24c512", "driver": "at24"},
    "6-0060": {"name": "fancpld", "driver": "fancpld"},
    "7-0050": {"name": "24c512", "driver": "at24"},
    "9-0011": {"name": "ucd90320", "driver": "ucd9000"},
    "10-0030": {"name": "cpupwr", "driver": ""},
    "10-0037": {"name": "mempwr", "driver": ""},
    "11-0040": {"name": "pmbus", "driver": "pmbus"},
    "11-004c": {"name": "max6658", "driver": "lm90"},
    "12-0043": {"name": "scmcpld", "driver": "scmcpld"},
    "12-0050": {"name": "24c512", "driver": "at24"},
}
pim16q = [
    # Address, name, driver
    ("0016", "pmbus", "pmbus"),
    ("0018", "pmbus", "pmbus"),
    ("004a", "lm73", "lm73"),
    ("004e", "ucd9090", "ucd9000"),
    ("0050", "24c512", "at24"),
]
pim8ddm = [
    # Address, name, driver
    ("0016", "pmbus", "pmbus"),
    ("0018", "pmbus", "pmbus"),
    ("004a", "lm73", "lm73"),
    ("004e", "ucd9090", "ucd9000"),
    ("0050", "24c512", "at24"),
    ("0054", "isl68224", "isl68137"),
]

psu_devices = [
    # Address, name, driver
    ("0058", "pmbus", "pmbus")
]
