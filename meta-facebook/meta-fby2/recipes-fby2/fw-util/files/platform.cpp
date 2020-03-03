#include "bic_fw.h"
#include "usbdbg.h"
#include "nic.h"
#include "tpm.h"

// Shared NIC
NicComponent nic("nic", "nic");

// Register BIC FW and BL components
BicFwComponent bicfw1("slot1", "bic", 1);
BicFwComponent bicfw2("slot2", "bic", 2);
BicFwComponent bicfw3("slot3", "bic", 3);
BicFwComponent bicfw4("slot4", "bic", 4);
BicFwBlComponent bicfwbl1("slot1", "bicbl", 1);
BicFwBlComponent bicfwbl2("slot2", "bicbl", 2);
BicFwBlComponent bicfwbl3("slot3", "bicbl", 3);
BicFwBlComponent bicfwbl4("slot4", "bicbl", 4);

// Register USB Debug Card components
UsbDbgComponent usbdbg("ocpdbg", "mcu", "FBY2", 13, 0x60, false);
UsbDbgBlComponent usbdbgbl("ocpdbg", "mcubl", 13, 0x60, 0x02);  // target ID of bootloader = 0x02

// Register BMC TPM
TpmComponent tpm("bmc", "tpm");
