#include "usbdbg.h"
#include "tpm2.h"

UsbDbgComponent usbdbg("ocpdbg", "mcu", "JG7", 13, 0x60, false);
UsbDbgBlComponent usbdbgbl("ocpdbg", "mcubl", 13, 0x60, 0x02);  // target ID of bootloader = 0x02

Tpm2Component tpm2("bmc", "tpm");
