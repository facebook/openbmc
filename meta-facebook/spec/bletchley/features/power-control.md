#### Managed Hosts

We should be able to provide power control of the managed hosts by both Redfish
and internal DBus interfaces. The managed hosts shall support the following
power control operations:

- on - Power ON the host (AC relay on + power button on)
- off - Power OFF the host (power button off + AC relay off)
- cycle - Power cycle the host (on + off)
- reboot - (power button off, power button on)

There should also be a method to put the managed host into recovery mode (DFU)
as part of a power on operation.

In Redfish these power control operations on the managed hosts should be found
under the `/redfish/v1/Systems/hostN` and
`/redfish/v1/Systems/hostN/Actions/ComputerSystem.Reset` end-points.

#### Chassis

We should be able to perform a full Chassis (sled) power cycle via both Redfish
and internal DBus interfaces.

In Redfish the Chassis power cycle should be found under the
`/redfish/v1/Chassis/chassis0/Actions/Chassis.Reset` end-point.
