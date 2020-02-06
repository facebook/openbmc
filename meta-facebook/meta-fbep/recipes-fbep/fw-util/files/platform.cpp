#include "usbdbg.h"

AliasComponent bmc("mb", "bmc", "bmc", "bmc");
AliasComponent rom("mb", "rom", "bmc", "rom");

UsbDbgComponent usbdbg("ocpdbg", "mcu", "JG7", 13, 0x60, false);
UsbDbgBlComponent usbdbgbl("ocpdbg", "mcubl", 13, 0x60, 0x02);  // target ID of bootloader = 0x02
