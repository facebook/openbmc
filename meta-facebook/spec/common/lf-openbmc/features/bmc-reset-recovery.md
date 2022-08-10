We should be able to reboot the BMC over SSH, any debug console, and Redfish.
The BMC reboot should be handled with `reboot` on a terminal or by Redfish with
the `/redfish/v1/Managers/bmc/Actions/Manager.Reset` end-point.
