All devices listed should have current firmware versions displayed and images be
updatable on both DBus and Redfish. The DBus implementation should conform to
the documented OpenBMC [software design][pdi-software]. The Redfish end-point
`/redfish/v1/FirmwareInventory` should be used to display the current versions
and `/redfish/v1/UpdateService` should be used to perform updates.

As an additional P0 item, all updatable devices must have a debug command
documented to update the device firmware if the native DBus/Redfish support is
not available.

All firmware update procedures should use some digital signature, checksum,
and/or read verification method to protect data in transit all the way to the
end-device.

[pdi-software]:
  https://github.com/openbmc/phosphor-dbus-interfaces/blob/master/yaml/xyz/openbmc_project/Software/README.md
