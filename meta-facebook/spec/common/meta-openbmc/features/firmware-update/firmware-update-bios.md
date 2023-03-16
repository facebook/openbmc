The BMC shall have a method to upgrade system BIOS.  The fw-util command-line
utility or the Redfish /redfish/v1/UpdateService interface shall be used to
update the system BIOS.  The system BIOS update shall be activated using
the power-util commands or via activation via the Redfish interface.  
Updating the system BIOS shall not require reboot of other devices such as
a slot BIC for this process to complete.
