The Bletchley managed hosts do not support IPMI, so we do not get a regular
update of 'uptime' from the host.  Instead the Bletchley BMC should approximate
this value for each managed host based on the power state.

This information should be held in the `xyz.openbmc_project.State.PowerOnHours`
interface on DBus for each managed host.
