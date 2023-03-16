The BMC shall have a method to upgrade all voltage regulator firmware.  The
voltage regulator update should not be dependent on any other firmware updates
such as BMC FW.  Thus, updating the voltage regulator firmware should always
allow the slot to continue functioning as expected until a later activation
event even if the BMC FW or firmware running on other devices use older
versions of their respective firmware.  Because of the limited number of
updates supported by many voltage regulators, it is required the number of
updates be tracked to ensure when the voltage regulator nears the limit of
updates supported by the voltage regulator that appropriate log entries are
generated to indicate the limit is near.  When the limit is reached, the
voltage regulator firmware update shall fail and log this failure
appropriately. The fw-util command-line utility or the Redfish
/redfish/v1/UpdateService interface shall be used to update the voltage
regulator firmware.  The voltage regulator update shall be activated using
the power-util commands and shall not require reboot of other devices such as
a slot BIC for this process to complete.
