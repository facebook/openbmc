All devices listed should have firmware versions exposed on both DBus, via
`xyz.openbmc_project.Software.Version` interfaces, and Redfish at the
`/redfish/v1/UpdateService/FirmwareInventory` end-point.  If the device
supports a firmware redundancy model, both firmwares should be reported.
