#include <cstdio>
#include <cstring>
#include <openbmc/pal.h>
#include <openbmc/mcu.h>
#include "usbdbg.h"

using namespace std;

int UsbDbgComponent::print_version() {
  uint8_t ver[8];

  try {
    if (!pal_is_mcu_ready(bus_id) || mcu_get_fw_ver(bus_id, slv_addr, MCU_FW_RUNTIME, ver)) {
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
    if (!pal_is_mcu_ready(bus_id) || mcu_get_fw_ver(bus_id, slv_addr, MCU_FW_BOOTLOADER, ver)) {
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
