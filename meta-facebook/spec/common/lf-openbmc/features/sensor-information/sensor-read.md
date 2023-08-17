All of the sensors found in the [sensor table](#sensors) shall have their values
and thresholds reported on both DBus and Redfish. The sensors shall have their
values update at least once every 5s, unless otherwise specified. Any sensor
violating a threshold shall report an error event.
