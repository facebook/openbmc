#include <openbmc/pal.h>
#include "bios.h"
#include "usbdbg.h"
#include "nic_ext.h"

BiosComponent bios("mb", "bios", "\"pnor\"", "F0G_");

NicExtComponent nic0("nic", "nic0", "nic0_fw_ver", FRU_NIC0, 0);  // fru_name, component, kv, fru_id, eth_index
NicExtComponent nic1("nic", "nic1", "nic1_fw_ver", FRU_NIC1, 1);

UsbDbgComponent usbdbg("ocpdbg", "mcu", 0, 0x60);
UsbDbgBlComponent usbdbgbl("ocpdbg", "mcubl", 0, 0x60, 0x02);  // target ID of bootloader = 0x02
