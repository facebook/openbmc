#include "fw-util.h"
#include <cstdio>
#include <cstring>
#include "server.h"
#include <openbmc/pal.h>
#include "bic_fw.h"
#ifdef BIC_SUPPORT
#include <facebook/bic.h>

using namespace std;

/*
 * The functions are used for the BIC of the server board
*/
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
    if (bic_get_fw_ver(slot_id, FW_BIC, ver, intf)) {
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
  try {
    //TODO The function is not ready, we skip it now.
    //server.ready();

    // Print Bridge-IC Bootloader Version
    if (bic_get_fw_ver(slot_id, FW_BIC_BOOTLOADER, ver, intf)) {
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

/*
 * The functions are used for the 2nd BIC of the expansion board
*/

int BicFwExtComponent::update(string image) {
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

int BicFwExtComponent::fupdate(string image) {
  return FW_STATUS_NOT_SUPPORTED;
}

int BicFwExtComponent::print_version() {
  uint8_t ver[32];
  string board_name = board();
  try {
    //TODO The function is not ready, we skip it now.
    //server.ready();
    //make the strig to uppercase.
    transform(board_name.begin(), board_name.end(), board_name.begin(), ::toupper);

    // Print Bridge-IC Version
    if (bic_get_fw_ver(slot_id, FW_BIC, ver, intf)) {
      printf("%s Bridge-IC Version: NA\n", board_name.c_str());
    }
    else {
      printf("%s Bridge-IC Version: v%x.%02x\n", board_name.c_str(), ver[0], ver[1]);
    }
  } catch(string err) {
    printf("%s Bridge-IC Version: NA (%s)\n", board_name.c_str(), err.c_str());
  }
  return 0;
}

int BicFwBlExtComponent::update(string image) {
  int ret;
  try {
    server.ready();
    ret = bic_update_fw(slot_id, UPDATE_BIC_BOOTLOADER, intf, (char *)image.c_str(), 0);
  } catch(string err) {
    return FW_STATUS_NOT_SUPPORTED;
  }
  return ret;
}

int BicFwBlExtComponent::print_version() {
  uint8_t ver[32];
  string board_name = board();
  try {
    //TODO The function is not ready, we skip it now.
    //server.ready();
    //make the strig to uppercase.
    transform(board_name.begin(), board_name.end(), board_name.begin(), ::toupper);

    // Print Bridge-IC Bootloader Version
    if (bic_get_fw_ver(slot_id, FW_BIC_BOOTLOADER, ver, intf)) {
      printf("%s Bridge-IC Bootloader Version: NA\n", board_name.c_str());
    }
    else {
      printf("%s Bridge-IC Bootloader Version: v%x.%02x\n", board_name.c_str(), ver[0], ver[1]);
    }
  } catch(string err) {
    printf("%s Bridge-IC Bootloader Version: NA (%s)\n", board_name.c_str(), err.c_str());
  }
  return 0;
}

#endif

