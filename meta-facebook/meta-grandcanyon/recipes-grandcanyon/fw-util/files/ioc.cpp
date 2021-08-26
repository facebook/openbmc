extern "C" {
#include <openbmc/ipc.h>
}
#include <string>
#include <cstdio>
#include <cstring>
#include <algorithm>
#include "ioc.h"

int IOCComponent :: get_ver_str(string& s) {
  int ret = 0;
  uint8_t i2c_bus = 0;
  uint8_t request[IOC_FW_VER_SIZE] = {0}, resp[IOC_FW_VER_SIZE] = {0};
  size_t req_len = IOC_FW_VER_SIZE, resp_len = IOC_FW_VER_SIZE;
  char ioc_ver_key[MAX_KEY_LEN] = {0}, ver[MAX_VALUE_LEN] = {0};
  char ioc_socket[MAX_SOCK_PATH_SIZE] = {0};
  string fru_name = fru();

  if (fru_name.compare("scc") == 0) {
    snprintf(ioc_socket, sizeof(ioc_socket), SOCK_PATH_IOC, FRU_SCC);
    i2c_bus = I2C_T5IOC_BUS;
  } else if (fru_name.compare("iocm") == 0) {
    snprintf(ioc_socket, sizeof(ioc_socket), SOCK_PATH_IOC, FRU_E1S_IOCM);
    i2c_bus = I2C_T5E1S0_T7IOC_BUS;
  }

  if (pal_is_ioc_ready(i2c_bus) == false) {
    return FW_STATUS_NOT_SUPPORTED;
  }

  snprintf(ioc_ver_key, sizeof(ioc_ver_key), "ioc_%d_fw_ver", i2c_bus);

  // Check with IOCD that the Firmware Versions of IOC could be read or not.
  ret = ipc_send_req(ioc_socket, request, req_len, resp, &resp_len, TIMEOUT_IOC + 1);

  if ((ret == FW_STATUS_SUCCESS) && (resp[0] == IOC_FW_VER_SIZE)) {
    if (pal_get_cached_value(ioc_ver_key, ver) == 0) {
      s = string(ver);
    } else {
      ret = FW_STATUS_FAILURE;
    }
  } else {
    ret = FW_STATUS_FAILURE;
  }

  return ret;
}

int IOCComponent :: print_version() {
  string ver("");
  string fru_name = fru();
  int ret = 0;

  transform(fru_name.begin(), fru_name.end(), fru_name.begin(), ::toupper);

  ret = get_ver_str(ver);
  if (ret == FW_STATUS_SUCCESS) {
    cout << fru_name << " IOC Version: " << ver << endl;
  } else if (ret == FW_STATUS_NOT_SUPPORTED){
    cout << "Error in getting the version of " + fru_name + " IOC"<< endl;
    cout << fru_name << " IOC firmware is not ready"<< endl;
  } else {
    cout << "Error in getting the version of " + fru_name + " IOC"<< endl;
    cout << "Please check /var/log/messages"<< endl;
  }

  return 0;
}

