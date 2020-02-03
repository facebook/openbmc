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
  int ret = 0;

  try {
    server.ready();
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
  uint8_t ver[32]= {0};
  int ret = 0;
  string board_name = board();

  transform(board_name.begin(), board_name.end(), board_name.begin(), ::toupper);
  try {
    server.ready();
    ret = bic_show_fw_ver(slot_id, FW_BIC, ver, 0, 0, intf);
    // Print Bridge-IC Version
    if ( ret < 0 ) {
      throw "Error in getting the version of " + board_name;
    } else {
      printf("Bridge-IC Version: v%x.%02x\n", ver[0], ver[1]);
    }
  } catch(string err) {
    printf("Bridge-IC Version: NA (%s)\n", err.c_str());
  }
  return ret;
}

int BicFwBlComponent::update(string image) {
  int ret = 0;

  try {
    server.ready();
    ret = bic_update_fw(slot_id, UPDATE_BIC_BOOTLOADER, intf, (char *)image.c_str(), 0);
  } catch(string err) {
    return FW_STATUS_NOT_SUPPORTED;
  }
  return ret;
}

int BicFwBlComponent::print_version() {
  uint8_t ver[32] = {0};
  int ret = 0;
  string board_name = board();

  transform(board_name.begin(), board_name.end(), board_name.begin(), ::toupper);
  try {
    server.ready();
    ret = bic_show_fw_ver(slot_id, FW_BIC_BOOTLOADER, ver, 0, 0, intf);
    // Print Bridge-IC Bootloader Version
    if ( ret < 0 ) {
      throw "Error in getting the version of " + board_name;
    } else {
      printf("Bridge-IC Bootloader Version: v%x.%02x\n", ver[0], ver[1]);
    }
  } catch(string err) {
    printf("Bridge-IC Bootloader Version: NA (%s)\n", err.c_str());
  }
  return ret;
}

/*
 * The functions are used for the 2nd BIC of the expansion board
*/

int BicFwExtComponent::update(string image) {
  int ret = 0;
  try {
    server.ready();
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
  uint8_t ver[32] = {0};
  int ret = 0;
  string board_name = board();

  //make the strig to uppercase.
  transform(board_name.begin(), board_name.end(), board_name.begin(), ::toupper);
  try {
    server.ready();
    if ( intf == BB_BIC_INTF ) {
      ; // do nothing
    } else {
      ret = bic_is_m2_exp_prsnt(slot_id);
      if ( ret < 0 ) {
        throw "Error in getting the present status of " + board_name;
      } else if ( intf == FEXP_BIC_INTF && (ret == PRESENT_1OU || ret == (PRESENT_1OU + PRESENT_2OU)) ) {
        ; // Correct status, do nothing
      } else if ( intf == REXP_BIC_INTF && (ret == PRESENT_2OU || ret == (PRESENT_1OU + PRESENT_2OU)) ) {
        ; // Correct status, do nothing
      } else {
        throw board_name + " Board is empty";
      }
    }
    ret = bic_show_fw_ver(slot_id, FW_BIC, ver, 0, 0, intf); 
    // Print Bridge-IC Version
    if ( ret < 0 ) {
      throw "Error in getting the version of " + board_name;
    } else {
      printf("%s Bridge-IC Version: v%x.%02x\n", board_name.c_str(), ver[0], ver[1]);
    }
  } catch(string err) {
    printf("%s Bridge-IC Version: NA (%s)\n", board_name.c_str(), err.c_str());
  }
  return ret;
}

int BicFwBlExtComponent::update(string image) {
  int ret = 0;

  try {
    server.ready();
    ret = bic_update_fw(slot_id, UPDATE_BIC_BOOTLOADER, intf, (char *)image.c_str(), 0);
  } catch(string err) {
    return FW_STATUS_NOT_SUPPORTED;
  }
  return ret;
}

int BicFwBlExtComponent::print_version() {
  uint8_t ver[32] = {0};
  int ret = 0;
  string board_name = board();

  //make the strig to uppercase.
  transform(board_name.begin(), board_name.end(), board_name.begin(), ::toupper);
  try {
    server.ready();
    if ( intf == BB_BIC_INTF ) {
      ; // do nothing
    } else {
      ret = bic_is_m2_exp_prsnt(slot_id);
      if ( ret < 0 ) {
        throw "Error in getting the present status of " + board_name;
      } else if ( intf == FEXP_BIC_INTF && (ret == PRESENT_1OU || ret == (PRESENT_1OU + PRESENT_2OU)) ) {
        ; // Correct status, do nothing
      } else if ( intf == REXP_BIC_INTF && (ret == PRESENT_2OU || ret == (PRESENT_1OU + PRESENT_2OU)) ) {
        ; // Correct status, do nothing
      } else {
        throw board_name + " Board is empty";
      }
    }
    ret = bic_show_fw_ver(slot_id, FW_BIC_BOOTLOADER, ver, 0, 0, intf);
    // Print Bridge-IC Bootloader Version
    if ( ret < 0 ) {
      throw "Error in getting the version of " + board_name;
    } else {
      printf("%s Bridge-IC Bootloader Version: v%x.%02x\n", board_name.c_str(), ver[0], ver[1]);
    }
  } catch(string err) {
    printf("%s Bridge-IC Bootloader Version: NA (%s)\n", board_name.c_str(), err.c_str());
  }
  return 0;
}

#endif

