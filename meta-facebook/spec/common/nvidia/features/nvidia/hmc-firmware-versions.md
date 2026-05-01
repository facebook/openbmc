BMC shall track and expose firmware versions for all HMC-managed components
via the Redfish FirmwareInventory endpoint.

The redfish-client daemon queries the HMC's Redfish UpdateService
FirmwareInventory and maps each component's firmware version to a BMC dbus
software object. This enables centralized firmware inventory visibility
from the BMC even for components managed by the HMC.

Components tracked include:
- GPU firmware (per-GPU, e.g., GPU_SXM_1 through GPU_SXM_8)
- CPLD firmware (per baseboard CPLD)
- ERoT firmware (per GPU ERoT)
- FPGA firmware
- InfoROM (per GPU)
- PCIe switch configuration
