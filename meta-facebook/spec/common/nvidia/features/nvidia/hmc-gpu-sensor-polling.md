BMC shall poll GPU and HMC board sensors via the HMC Redfish interface and
expose them on the BMC dbus as standard sensor objects.

The redfish-client daemon queries the HMC's Redfish Chassis sensor endpoints
and maps each sensor to a BMC dbus sensor object under the configured
association path. Sensor values, thresholds, and functional status are
synchronized from the HMC at the configured polling interval.

Supported sensor types include GPU temperature, GPU power, GPU memory
temperature, NVSwitch temperature, HMC board temperatures (inlet, PCB,
HSC, FPGA, PCIe retimer/switch), and HMC power consumption (HSC, total).
