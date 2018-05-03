#include "fw-util.h"
#include "usbdbg.h"
#include <openbmc/pal.h>

// Set aliases for BMC components to MB components
AliasComponent bmc("mb", "bmc", "bmc", "bmc");
AliasComponent rom("mb", "rom", "bmc", "rom");
AliasComponent mb_fscd("mb", "fscd", "bmc", "fscd");
UsbDbgComponent usbdbg("mb", "usbdbgfw", (uint8_t)0x9, (uint8_t)0x60, (uint8_t)0x4E);
UsbDbgBlComponent usbdbgbl("mb", "usbdbgbl", (uint8_t)0x9, (uint8_t)0x60, (uint8_t)0x4E);
