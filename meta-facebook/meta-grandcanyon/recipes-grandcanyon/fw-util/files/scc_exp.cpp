#include "scc_exp.h"
#include <facebook/exp.h>

int ExpanderComponent :: print_version()
{
  uint8_t ver[FW_VERSION_BYTES] = {0};
  int ret = FW_STATUS_SUCCESS;

  // Read Firmware Versions of Expander via IPMB
  ret = expander_get_fw_ver(ver, FW_VERSION_BYTES);
  if (ret == FW_STATUS_SUCCESS) {
    printf("Expander Version: 0x%02X%02X%02X%02X\n", ver[0], ver[1], ver[2], ver[3]);
  } else {
    printf("Error in getting the version of expander\n");
  }

  return ret;
}

int ExpanderComponent :: get_version(json& j) {
  uint8_t ver[FW_VERSION_BYTES] = {0};
  int ret = expander_get_fw_ver(ver, FW_VERSION_BYTES);

  if (ret == FW_STATUS_SUCCESS) {
    char ver_str[32] = {0};
    snprintf(ver_str, sizeof(ver_str), "0x%02X%02X%02X%02X", ver[0], ver[1], ver[2], ver[3]);
    j["VERSION"] = string(ver_str);
  } else {
    j["VERSION"] = "error_returned";
  }
  return FW_STATUS_SUCCESS;
}
