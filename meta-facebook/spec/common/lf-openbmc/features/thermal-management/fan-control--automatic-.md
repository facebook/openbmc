The fan control algorithms should, by default, be controlled with a JSON-defined
algorithm acceptable by the system's thermal characterization team. The JSON
file should be versioned and the version should be present in the Redfish
FirmwareInventory.

The fan control configuration should be able to be updated for test and debug
purposes by placing an alternative configuration in a persistent, read-write
file-system.
