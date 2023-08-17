The BMC must obtain the MAC address of each managed host from the NIC and
determine the associated hostname for that MAC address.

At a DBus level, each managed host should have a corresponding instance of
`xyz.openbmc_project.Network.SystemConfiguration` to hold the `HostName` of the
managed host. This data must be present in the Redfish model for the
`ComputerSystem` instance corresponding to the host.
