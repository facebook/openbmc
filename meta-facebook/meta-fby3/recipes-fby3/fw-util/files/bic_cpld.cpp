#include <iostream>
#include <sstream>
#include <string>
#include <cstdio>
#include <cstring>
#include <unistd.h>
#include <fcntl.h>
#include <algorithm>
#include <sys/mman.h>
#include <syslog.h>
#include "bic_cpld.h"
#ifdef BIC_SUPPORT
#include <facebook/bic.h>
using namespace std;

int CpldComponent::print_version()
{
  uint8_t ver[4] = {0};
  string fru_name = fru();
  try {
    //TODO The function is not ready, we skip it now.
    //server.ready();

    // Print CPLD Version
    transform(fru_name.begin(), fru_name.end(), fru_name.begin(), ::toupper);
    if (bic_get_cpld_ver(slot_id, FW_CPLD, ver, bus, addr, intf)) {
      printf("%s CPLD Version: NA\n", fru_name.c_str());
    }
    else {
      printf("%s CPLD Version: %02X%02X%02X%02X\n", fru_name.c_str(), ver[3], ver[2], ver[1], ver[0]);
    }
  } catch(string err) {
    printf("%s CPLD Version: NA (%s)\n", fru_name.c_str(), err.c_str());
  }
  return 0;

}

int CpldComponent::update(string image)
{
  int ret;
  try {
    //TODO: the function is not ready, we skip it now.
    //server.ready()
    ret = bic_update_fw(slot_id, UPDATE_CPLD, intf, (char *)image.c_str(), 0);
  } catch (string err) {
    return FW_STATUS_NOT_SUPPORTED;
  }
  return ret;
}
#endif
