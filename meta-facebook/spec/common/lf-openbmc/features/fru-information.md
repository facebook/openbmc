All serialized FRUs that the BMC has access to should be reported on both DBus
and Redfish. Under DBus there must be a `xyz.openbmc_project.Inventory.Item` for
each, as well as appropriate sub-types based on the FRU type.

If the FRU contains an EEPROM in a standardized format, such as IPMI, all fields
in the FRU EEPROM must be reported in corresponding
`xyz.openbmc_project.Inventory` interfaces (and similarly reflected in Redfish
properties).

Each FRU EEPROM attached to the BMC should have a debug method to update the
EEPROM. In most cases this will be performed by a `dd` call to the sysfs entry
for the EEPROM.
