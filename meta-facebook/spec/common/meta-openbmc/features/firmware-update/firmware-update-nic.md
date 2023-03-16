The BMC shall have a method to upgrade the NIC firmware.  The BMC shall
implement the PLDM over NC-SI or PLDM over MCTP/I3C support to update
the NIC firmware.  The Redfish /redfish/v1/UpdateService interface shall
also be used to drive the underlying PLDM interface for NIC firmware updates.
The NIC firmware shall be activated without user intervention if the NIC
firmware update allows for this or via the power-util sled-cycle command or
equivalent Redfish interface action.
