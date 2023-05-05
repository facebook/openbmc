#include <cstdio>
#include <cstring>
#include <fstream>
#include <unistd.h>
#include <chrono>
#include <thread>
#include <openbmc/pal.h>
#include <openbmc/obmc-i2c.h>
#include <openbmc/vr.h>
#include <libpldm/base.h>
#include <libpldm-oem/pldm.h>
#include <syslog.h>
#include "usbdbg.h"
#include "nic_ext.h"
#include "bios.h"
#include "vr_fw.h"
#include "swb_common.hpp"
#include <openbmc/kv.h>

using namespace std;

class fw_common_config {
  public:
    fw_common_config() {
      static NicExtComponent nic0("nic0", "nic0", "nic0_fw_ver", FRU_NIC0, 0);
      if (!pal_is_artemis()) {
        static UsbDbgComponent usbdbg("ocpdbg", "mcu", "F0T", 14, 0x60, false);
        static UsbDbgBlComponent usbdbgbl("ocpdbg", "mcubl", 14, 0x60, 0x02);  // target ID of bootloader = 0x02
      }
    }
};

fw_common_config _fw_common_config;
