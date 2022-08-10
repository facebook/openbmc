All devices listed should have firmware versions updatable on both DBus and
Redfish.  The DBus implementation should conform to the documented OpenBMC
[software design][pdi-software].  The Redfish end-point
`/redfish/v1/UpdateService` should be used to perform updates.

[pdi-software]: https://github.com/openbmc/phosphor-dbus-interfaces/blob/master/yaml/xyz/openbmc_project/Software/README.md
