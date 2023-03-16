The BMC shall have a method to update expansion card BIC firmware using
the UART interface between the BMC and BIC.  This method is to allow remote
recovery of the expansion BIC firmware in the event of a bad firmware image
landing or issues with the BIC which affect the ability of the BMC to
communicate through the I3C interface to the BIC.

The fw-util command-line utility or the Redfish /redfish/v1/UpdateService
interface shall be used to update the expansion BIC firmware through this
recovery mode.  The expansion BIC firmware update shall not require a slot,
sled or expansion card reboot to enable the updated expansion BIC firmware
image to be deployed.
