BMC shall support CPU debug via JTAG or any in-system methodologies.

## Processor-Specific Debug Technologies

### x86 (Intel/AMD)
- **JTAG**: BMC provides JTAG access to the CPU for low-level debug via
  OpenBMC's jtag-utils or vendor-specific tools.
- **Crashdump**: BMC collects CPU crashdump data on fatal errors (CATERR/IERR)
  and exposes it via Redfish LogServices.

### NVIDIA Grace (ARM)
- **CXL Debug**: BMC supports CXL debug interfaces for Grace CPU diagnostics.
- **CPER Events**: BMC decodes CPER (Common Platform Error Record) events
  forwarded from the HMC and logs them as structured BMC events.
- **Crash Dump**: BMC collects GPU/CPU crash dumps via the HMC Redfish
  interface and exposes them via Redfish LogServices.
