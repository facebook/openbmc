#include "usbdbg.h"

UsbDbgComponent usbdbg("ocpdbg", "mcu", "F0CE", 8, 0x60, false);
UsbDbgBlComponent usbdbgbl("ocpdbg", "mcubl", 8, 0x60, 0x02);  // target ID of bootloader = 0x02