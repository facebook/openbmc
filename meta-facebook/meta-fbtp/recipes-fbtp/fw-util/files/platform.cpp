#include "fw-util.h"
#include "bios.h"
#include "usbdbg.h"
#include "nic.h"
#include <openbmc/pal.h>

// Set aliases for BMC components to MB components
AliasComponent bmc("mb", "bmc", "bmc", "bmc");
AliasComponent rom("mb", "rom", "bmc", "rom");
AliasComponent mb_fscd("mb", "fscd", "bmc", "fscd");
UsbDbgComponent usbdbg("mb", "usbdbgfw", 9, 0x60);
UsbDbgBlComponent usbdbgbl("mb", "usbdbgbl", 9, 0x60, 0x02);
BiosComponent bios("mb", "bios", "\"bios0\"", "");
NicComponent nic("nic", "nic");
