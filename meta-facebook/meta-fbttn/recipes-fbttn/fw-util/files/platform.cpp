#include "fw-util.h"
#include "usbdbg.h"
#include <openbmc/pal.h>

// Set aliases for BMC components to usbdbg components
UsbDbgComponent usbdbg("bmc", "usbdbgfw", (uint8_t)0xb, (uint8_t)0x60, (uint8_t)0x4E);
UsbDbgBlComponent usbdbgbl("bmc", "usbdbgbl", (uint8_t)0xb, (uint8_t)0x60, (uint8_t)0x4E);
