Each managed host shall be able to configure the power policy between:

* Last Power State 
* Always On (default)
* Always Off

This should be available via the cfg-util command line utility and via
Redfish using the `PowerRestorePolicy` property on the `/redfish/v1/Systems/{}`
end-point.
