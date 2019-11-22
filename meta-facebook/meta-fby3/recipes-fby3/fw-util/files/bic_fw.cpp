#include "fw-util.h"
#include <cstdio>
#include <cstring>
#include "server.h"
#include <openbmc/pal.h>
#include "bic_fw.h"
#ifdef BIC_SUPPORT
#include <facebook/bic.h>

using namespace std;

int BicFwComponent::update(string image) {
  int ret;
  try {
    //TODO: the function is not ready, we skip it now.
    //server.ready()
    ret = bic_update_fw(slot_id, UPDATE_BIC, intf, (char *)image.c_str(), 0);
  } catch (string err) {
    return FW_STATUS_NOT_SUPPORTED;
  }
  return ret;
}

int BicFwComponent::fupdate(string image) {
  return FW_STATUS_NOT_SUPPORTED;
}

int BicFwComponent::print_version() {
  uint8_t ver[32];
  try {
    //TODO The function is not ready, we skip it now.
    //server.ready();

    // Print Bridge-IC Version
    if (bic_get_fw_ver(slot_id, FW_BIC, ver)) {
      printf("Bridge-IC Version: NA\n");
    }
    else {
      printf("Bridge-IC Version: v%x.%02x\n", ver[0], ver[1]);
    }
  } catch(string err) {
    printf("Bridge-IC Version: NA (%s)\n", err.c_str());
  }
  return 0;
}

int BicFwBlComponent::update(string image) {
  int ret;
  try {
    server.ready();
    ret = bic_update_fw(slot_id, UPDATE_BIC_BOOTLOADER, intf, (char *)image.c_str(), 0);
  } catch(string err) {
    return FW_STATUS_NOT_SUPPORTED;
  }
  return ret;
}

int BicFwBlComponent::print_version() {
  uint8_t ver[32];
  uint8_t comp;
  try {
    server.ready();
    comp = FW_BIC_BOOTLOADER;

    // Print Bridge-IC Bootloader Version
    if (bic_get_fw_ver(slot_id, comp, ver)) {
      printf("Bridge-IC Bootloader Version: NA\n");
    }
    else {
      printf("Bridge-IC Bootloader Version: v%x.%02x\n", ver[0], ver[1]);
    }
  } catch(string err) {
    printf("Bridge-IC Bootloader Version: NA (%s)\n", err.c_str());
  }
  return 0;
}
#endif

