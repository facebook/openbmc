All VRs in the system shall be supported by update.

For VRs which limit the number of write operations that can be performed to an
individual device, the current and max counts should be available on DBus as an
`xyz.openbmc_project.Metric.WriteCount` (TODO: this interface doesn't currently
exist and needs to be defined and accepted upstream.)
