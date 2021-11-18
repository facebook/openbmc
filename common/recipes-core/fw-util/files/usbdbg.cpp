#include <cstdio>
#include <cstring>
#include <openbmc/pal.h>
#include <openbmc/mcu.h>
#include <openbmc/misc-utils.h>
#include "usbdbg.h"


using namespace std;

int UsbDbgComponent::print_version() {
  uint8_t ver[8];

  try {
    if (!pal_is_mcu_ready(bus_id) || retry_cond(!mcu_get_fw_ver(bus_id, slv_addr, MCU_FW_RUNTIME, ver), 2, 300)) {
      printf("MCU Version: NA\n");
    }
    else {
      printf("MCU Version: v%x.%02x\n", ver[0], ver[1]);
    }
  } catch(string err) {
    printf("MCU Version: NA (%s)\n", err.c_str());
  }
  return 0;
}

int UsbDbgBlComponent::print_version() {
  uint8_t ver[8];

  try {
    if (!pal_is_mcu_ready(bus_id) || retry_cond(!mcu_get_fw_ver(bus_id, slv_addr, MCU_FW_BOOTLOADER, ver), 2, 300)) {
      printf("MCU Bootloader Version: NA\n");
    }
    else {
      printf("MCU Bootloader Version: v%x.%02x\n", ver[0], ver[1]);
    }
  } catch(string err) {
    printf("MCU Bootloader Version: NA (%s)\n", err.c_str());
  }
  return 0;
}

int UsbDbgBlComponent::update(string image) {
  int ret;

  ret = dbg_update_bootloader(bus_id, slv_addr, target_id, (const char *)image.c_str());
  if (ret != 0) {
    return FW_STATUS_FAILURE;
  }

  return FW_STATUS_SUCCESS;
}
