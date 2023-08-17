Every FRU accessible by the BMC (or a satellite management controller such as a
BIC) must have an EEPROM in a standardized format, such as IPMI. The EEPROMs
should be parsed and have corresponding `entity-manager` configurations to
create the corresponding `xyz.openbmc_project.Inventory` interfaces.
