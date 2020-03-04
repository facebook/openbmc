plat_i2c_tree = {
    #  "0-0020": {"name": "tpm_i2c_infineon", "driver": "tpm_i2c_infineon"}, # ELBERTTODO TPM I2C
    # ELBERTTODO SCD, FAN, LINECARD, PSU SMBus, Chassis, BMC EEPROM
    #  "6-0050": {"name": "24c512", "driver": "at24"},
    "10-0030": {"name": "cpupwr", "driver": ""},
    "11-0037": {"name": "mempwr", "driver": ""},
    "11-0040": {"name": "pmbus", "driver": "pmbus"},
    "11-004c": {"name": "max6658", "driver": "lm90"},
    "12-0043": {"name": "supcpld", "driver": "supcpld"},
    "12-0050": {"name": "24c512", "driver": "at24"},
}
