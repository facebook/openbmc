#include "fw-util.h"
#include <cstdio>
#include <cstring>
#include <stdlib.h>
#include "bic_cpld.h"
#include <openbmc/pal.h>
#ifdef BIC_SUPPORT
#include <facebook/bic.h>

using namespace std;

int CpldComponent::print_version() {
  try {
    server.ready();
    uint8_t ver[32] = {0};
    if (bic_get_fw_ver(slot_id, FW_CPLD, ver)) {
      printf("CPLD Version: NA\n");
    }
    else {
      printf("CPLD Version: 0x%02x%02x%02x%02x\n", ver[0], ver[1], ver[2], ver[3]);
    }
  } catch(string err) {
    printf("CPLD Version: NA (%s)\n", err.c_str());
  }
  return 0;
}
int CpldComponent::update(string image) {
  int ret = 0;
  try {
    server.ready();
    ret = bic_update_fw(slot_id, UPDATE_CPLD, (char *)image.c_str());
  } catch(string err) {
    ret = FW_STATUS_NOT_SUPPORTED;
  }
  return ret;
}
#endif
