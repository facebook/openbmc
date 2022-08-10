There shall be a method to reset the BMC into factory condition and erase
all persisted settings.  On DBus, the `xyz.openbmc_project.Common.FactoryReset`
interface should be implemented at `/xyz/openbmc_project/software` to facilitate
this.  On Redfish this should be handled by the
`/redfish/v1/Managers/bmc/Actions/Manager.ResetToDefaults` end-point.
