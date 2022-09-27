#include <cstdio>
#include <cstring>
#include <syslog.h>
#include <openbmc/mcu.h>
#include "mcu_fw.h"

using namespace std;

int McuFwComponent::update(string image)
{
  int ret;
  string comp = this->component();

  syslog(LOG_CRIT, "Component %s upgrade initiated", comp.c_str());
  ret = mcu_update_firmware(bus_id, slv_addr, (const char *)image.c_str(), (const char *)pld_name.c_str(), type, false);
  if (ret != 0) {
    return FW_STATUS_FAILURE;
  }

  syslog(LOG_CRIT, "Component %s upgrade completed", comp.c_str());
  return FW_STATUS_SUCCESS;
}

int McuFwBlComponent::update(string image)
{
  int ret;
  string comp = this->component();

  syslog(LOG_CRIT, "Component %s upgrade initiated", comp.c_str());
  ret = mcu_update_bootloader(bus_id, slv_addr, target_id, (const char *)image.c_str());
  if (ret != 0) {
    return FW_STATUS_FAILURE;
  }

  syslog(LOG_CRIT, "Component %s upgrade completed", comp.c_str());
  return FW_STATUS_SUCCESS;
}
