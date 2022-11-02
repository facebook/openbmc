#include <cstdio>
#include <cstring>
#include <iomanip>
#include <syslog.h>
#include <openbmc/pal.h>
#include <openbmc/mcu.h>
#include <openbmc/misc-utils.h>
#include <sstream>
#include "usbdbg.h"


using namespace std;

int UsbDbgComponent::get_version(json& j) {
  uint8_t ver[8];

  j["PRETTY_COMPONENT"] = "MCU";

  try {
    if (!pal_is_mcu_ready(bus_id) || retry_cond(!mcu_get_fw_ver(bus_id, slv_addr, MCU_FW_RUNTIME, ver), 2, 300)) {
      j["VERSION"] = "NA";
    }
    else {
      stringstream ss;
      ss << 'v' << std::hex << +ver[0]
        << std::setw(2) << std::setfill('0') << +ver[1];
      j["VERSION"] = ss.str();
    }
  } catch(string err) {
    j["VERSION"] = string("NA (") + err + ")";
  }
  return 0;
}

int UsbDbgBlComponent::get_version(json& j) {
  uint8_t ver[8];

  j["PRETTY_COMPONENT"] = "MCU Bootloader";
  try {
    if (!pal_is_mcu_ready(bus_id) || retry_cond(!mcu_get_fw_ver(bus_id, slv_addr, MCU_FW_BOOTLOADER, ver), 2, 300)) {
      j["VERSION"] = "NA";
    }
    else {
      stringstream ss;
      ss << 'v' << std::hex << +ver[0]
        << std::setw(2) << std::setfill('0') << +ver[1];
      j["VERSION"] = ss.str();
    }
  } catch(string err) {
    j["VERSION"] = string("NA (") + err + ")";
  }
  return 0;
}

int UsbDbgBlComponent::update(string image) {
  int ret;
  string comp = this->component();

  syslog(LOG_CRIT, "Component %s upgrade initiated", comp.c_str());
  ret = dbg_update_bootloader(bus_id, slv_addr, target_id, (const char *)image.c_str());
  if (ret != 0) {
    return FW_STATUS_FAILURE;
  }

  syslog(LOG_CRIT, "Component %s upgrade completed", comp.c_str());
  return FW_STATUS_SUCCESS;
}
