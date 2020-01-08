#include "bios.h"
#include "usbdbg.h"
#include "nic.h"

// NIC0
NicComponent nic("nic", "nic");

AliasComponent bmc("mb", "bmc", "bmc", "bmc");
AliasComponent rom("mb", "rom", "bmc", "rom");
BiosComponent bios("mb", "bios", "\"pnor\"", "F0C_");

UsbDbgComponent usbdbg("ocpdbg", "mcu", 0, 0x60);
UsbDbgBlComponent usbdbgbl("ocpdbg", "mcubl", 0, 0x60, 0x02);  // target ID of bootloader = 0x02
