#include "bios.h"
#include "usbdbg.h"

BiosComponent bios("mb", "bios", "\"pnor\"", "F0G_");

UsbDbgComponent usbdbg("ocpdbg", "mcu", 0, 0x60);
UsbDbgBlComponent usbdbgbl("ocpdbg", "mcubl", 0, 0x60, 0x02);  // target ID of bootloader = 0x02
