#include "fw-util.h"
#include "usbdbg.h"
#include "tpm.h"
#include "nic.h"
#include "bic_fw.h"
#include "bic_bios.h"
#include "bic_me.h"
#include "bic_cpld.h"
#include <openbmc/pal.h>

// Set aliases for BMC components to usbdbg components
UsbDbgComponent usbdbg("bmc", "usbdbgfw", "FBTTN", 11, 0x60, false);
UsbDbgBlComponent usbdbgbl("bmc", "usbdbgbl", 11, 0x60, 0x02);
// Register BIC FW and BL components
BicFwComponent bicfw1("server", "bic", 1);
BicFwBlComponent bicfwbl1("server", "bicbl", 1);
// Register the BIOC component
BiosComponent bios1("server", "bios", 1);
// Register the ME component
MeComponent me1("server", "me", 1);
// Register CPLD component
CpldComponent cpld1("server", "cpld", 1);
// Register NIC
NicComponent nic("nic", "nic");
// Register BMC TPM
TpmComponent tpm("bmc", "tpm");
