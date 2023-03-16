The BMC shall have a method to update BIC FW using the UART interface between
the BMC and BIC.  This method is to allow remote recovery of the BIC FW in
the event of a bad firmware image landing or issues with the BIC which
affect the ability of the BMC to communicate through the I3C interface to
the BIC.

The fw-util command-line utility or the Redfish /redfish/v1/UpdateService
interface shall be used to update the BIC firmware through this recovery
mode.  The BIC firmware update shall not require a slot or sled reboot to
enable the updated BIC firmware image to be deployed.
