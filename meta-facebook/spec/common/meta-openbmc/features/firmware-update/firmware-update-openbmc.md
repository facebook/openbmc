The OpenBMC BMC and ROM images must updated fully while the BMC is running
and without reboot.  The BMC firmware should be activated using the reboot
command.

The BMC shall upgrade the BMC and ROM images from within the BMC.  The BMC
shall ensure the ROM image is locked starting in the PVT timeframe.  The BMC
firmware shall be updatable using the fw-util command-line utility and via the
Redfish /redfish/v1/UpdateService interface.
