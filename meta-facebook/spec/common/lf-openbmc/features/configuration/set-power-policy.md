Each managed host shall be able to configure the power policy between:

- Last Power State (default)
- Always On
- Always Off

On DBus this shall be implemented with the
`xyz.openbmc_project.Control.Power.RestorePolicy` interface on a
`/xyz/openbmc_project/control/host{}/power_restore_policy` object for each host.
On Redfish this should be reflected by the `PowerRestorePolicy` property on the
`/redfish/v1/Systems/{}` end-point.
