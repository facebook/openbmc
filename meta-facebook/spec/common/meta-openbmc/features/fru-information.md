All serialized FRUs that the BMC has access to should be reported using
the fruid-util command and via the Redfish interface.

If the FRU contains an EEPROM in a standardized format, such as IPMI, all
fields in the FRU EEPROM must be reported in the corresponding Redfish
interface and via the fruid-util command

Each FRU EEPROM attached to the BMC should have a debug method to update
the EEPROM.  In most cases this will be performed by a `dd` call to the
sysfs entry for the EEPROM.

