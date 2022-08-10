All of the sensors found in the [sensor table](#sensors) shall have their
values and thresholds reported on both DBus and Redfish.  The sensors shall
have their values updated at least once every 5s, unless otherwise specified.

Historical telemetry of the sensor values shall be maintained to report the
minimum, maximum, and average value over the preceding 10 second, 1 minute,
10 minute, and 1 hour intervals.  A record of this telemetry shall be captured
in a `xyz.openbmc_project.Dump.Entry` when catastrophic BMC or managed-host
events occur.
