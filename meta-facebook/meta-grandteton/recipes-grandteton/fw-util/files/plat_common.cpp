#include <cstdio>
#include <cstring>
#include <fstream>
#include <unistd.h>
#include <chrono>
#include <thread>
#include <openbmc/pal.h>
#include <openbmc/obmc-i2c.h>
#include <syslog.h>
#include "usbdbg.h"
#include "nic_ext.h"
#include "bios.h"

// fru_name, component, kv, fru_id, eth_index, ch_id
NicExtComponent nic0("nic0", "nic0", "nic0_fw_ver", FRU_NIC0, 0);

UsbDbgComponent usbdbg("ocpdbg", "mcu", "F0T", 14, 0x60, false);
UsbDbgBlComponent usbdbgbl("ocpdbg", "mcubl", 14, 0x60, 0x02);  // target ID of bootloader = 0x02

