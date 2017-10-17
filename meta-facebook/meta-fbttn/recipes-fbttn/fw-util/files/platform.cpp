#include "fw-util.h"
#include "usbdbg.h"
#include "bic_fw.h"
#include "bic_bios.h"
#include "bic_me.h"
#include "bic_cpld.h"
#include <openbmc/pal.h>

// Set aliases for BMC components to usbdbg components
UsbDbgComponent usbdbg("bmc", "usbdbgfw", (uint8_t)0xb, (uint8_t)0x60, (uint8_t)0x4E);
UsbDbgBlComponent usbdbgbl("bmc", "usbdbgbl", (uint8_t)0xb, (uint8_t)0x60, (uint8_t)0x4E);
// Register BIC FW and BL components
BicFwComponent bicfw1("server", "bic", 1);
BicFwBlComponent bicfwbl1("server", "bicbl", 1);
// Register the BIOC component
BiosComponent bios1("server", "bios", 1);
// Register the ME component
MeComponent me1("server", "me", 1);
// Register CPLD component
CpldComponent cpld1("server", "cpld", 1);
