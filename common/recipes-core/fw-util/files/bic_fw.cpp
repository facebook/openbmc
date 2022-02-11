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
    server.ready();
    ret = bic_update_fw(slot_id, UPDATE_BIC, (char *)image.c_str());
  } catch (string err) {
    return FW_STATUS_NOT_SUPPORTED;
  }
  return ret;
}

int BicFwComponent::fupdate(string image) {
  int ret;
  try {
    server.ready();
    ret = pal_force_update_bic_fw(slot_id, UPDATE_BIC, (char *)image.c_str());
  } catch (string err) {
    return FW_STATUS_NOT_SUPPORTED;
  }
  return ret;
}

int BicFwComponent::print_version() {
  uint8_t ver[32];
  try {
    server.ready();
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
    ret = bic_update_fw(slot_id, UPDATE_BIC_BOOTLOADER, (char *)image.c_str());
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
#ifdef CONFIG_FBY2_ND
  int ret;
  uint8_t server_type = 0xFF;

  ret = bic_get_server_type(slot_id, &server_type);
  if (ret) {
    throw "BIC get server type failed";
  }

  switch (server_type) {
    case SERVER_TYPE_ND:
      comp = FW_BOOTLOADER;
      break;
    case SERVER_TYPE_TL:
    default:
      comp = FW_BIC_BOOTLOADER;
      break;
  }
#else
  comp = FW_BIC_BOOTLOADER;
#endif

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

