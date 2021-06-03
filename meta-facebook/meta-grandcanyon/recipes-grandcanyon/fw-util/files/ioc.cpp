extern "C" {
#include <openbmc/ipc.h>
}
#include <string>
#include <cstdio>
#include <cstring>
#include <algorithm>
#include "ioc.h"

int IOCComponent :: print_version()
{
  uint8_t request[IOC_FW_VER_SIZE] = {0}, ver[IOC_FW_VER_SIZE] = {0};
  uint8_t i2c_bus = 0;
  size_t req_len = IOC_FW_VER_SIZE, resp_len = IOC_FW_VER_SIZE;
  char ioc_socket[MAX_SOCK_PATH_SIZE] = {0};
  int ret = FW_STATUS_SUCCESS;
  string fru_name = fru();

  memset(ioc_socket, 0 , sizeof(ioc_socket));
  memset(request, 0 , sizeof(request));
  memset(ver, 0 , sizeof(ver));

  if (fru_name.compare("scc") == 0) {
    snprintf(ioc_socket, sizeof(ioc_socket), SOCK_PATH_IOC, FRU_SCC);
    i2c_bus = I2C_T5IOC_BUS;
  } else if (fru_name.compare("iocm") == 0) {
    snprintf(ioc_socket, sizeof(ioc_socket), SOCK_PATH_IOC, FRU_E1S_IOCM);
    i2c_bus = I2C_T5E1S0_T7IOC_BUS;
  }

  transform(fru_name.begin(), fru_name.end(), fru_name.begin(), ::toupper);

  if (pal_is_ioc_ready(i2c_bus) == false) {
    printf("Error in getting the version of %s IOC\n", fru_name.c_str());
    return FW_STATUS_FAILURE;
  }

  // Read Firmware Versions of IOC from iocd.
  ret = ipc_send_req(ioc_socket, request, req_len, ver, &resp_len, TIMEOUT_IOC + 1);

  if ((ret == FW_STATUS_SUCCESS) && (resp_len == IOC_FW_VER_SIZE)) {
    printf("%s IOC Version: %d.%d%d%d.%02d-0000\n", fru_name.c_str(), ver[7], ver[6], ver[5], ver[4], ver[0]);
  } else {
    printf("Error in getting the version of %s IOC\n", fru_name.c_str());
  }

  return ret;
}

