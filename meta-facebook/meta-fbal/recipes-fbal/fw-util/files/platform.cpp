#include <openbmc/pal.h>
#include "bios.h"
#include "usbdbg.h"
#include "nic_ext.h"

AliasComponent bmc("mb", "bmc", "bmc", "bmc");
AliasComponent rom("mb", "rom", "bmc", "rom");
BiosComponent bios("mb", "bios", "\"pnor\"", "F0C_");

NicExtComponent nic0("nic", "nic0", "nic0_fw_ver", FRU_NIC0, 0);  // fru_name, component, kv, fru_id, eth_index
NicExtComponent nic1("nic", "nic1", "nic1_fw_ver", FRU_NIC1, 1);

UsbDbgComponent usbdbg("ocpdbg", "mcu", "F0C", 0, 0x60, false);
UsbDbgBlComponent usbdbgbl("ocpdbg", "mcubl", 0, 0x60, 0x02);  // target ID of bootloader = 0x02
