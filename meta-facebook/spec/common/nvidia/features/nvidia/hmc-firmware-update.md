BMC shall support firmware updates for HMC-managed components by proxying
update requests to the HMC's Redfish UpdateService.

The redfish-client daemon receives firmware update requests via the BMC's
Redfish UpdateService and forwards them to the HMC for execution. Update
progress and status are relayed back through the BMC's TaskService.

Supported update targets include GPU firmware, CPLD firmware, ERoT firmware,
FPGA firmware, and InfoROM images.
