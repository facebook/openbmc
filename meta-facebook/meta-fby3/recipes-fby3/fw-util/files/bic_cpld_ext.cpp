#include "fw-util.h"
#include <cstdio>
#include <cstring>
#include "server.h"
#include <openbmc/pal.h>
#include "bic_cpld_ext.h"
#ifdef BIC_SUPPORT
#include <facebook/bic.h>

using namespace std;

int CpldExtComponent::update(string image) {
  int ret = 0;
  try {
    server.ready();
    expansion.ready();
    ret = bic_update_fw(slot_id, fw_comp, (char *)image.c_str(), FORCE_UPDATE_UNSET);
  } catch (string err) {
    return FW_STATUS_NOT_SUPPORTED;
  }
  return ret;
}

int CpldExtComponent::fupdate(string image) {
  int ret = 0;
  try {
    server.ready();
    expansion.ready();
    ret = bic_update_fw(slot_id, fw_comp, (char *)image.c_str(), FORCE_UPDATE_SET);
  } catch (string err) {
    return FW_STATUS_NOT_SUPPORTED;
  }
  return ret;
}

int CpldExtComponent::print_version() {
  int ret = 0;
  uint8_t ver[32] = {0};
  string board_name = name;
  transform(board_name.begin(), board_name.end(), board_name.begin(), ::toupper);
  try {
    server.ready();
    expansion.ready();
    // Print Bridge-IC Version
    ret = bic_get_fw_ver(slot_id, fw_comp, ver);
    if ( ret < 0 ) {
      throw "Error in getting the version of " + board_name;
    } else {
      printf("%s CPLD Version: %02X%02X%02X%02X\n", board_name.c_str(), ver[3], ver[2], ver[1], ver[0]);
    }
  } catch(string err) {
    printf("%s CPLD Version: NA (%s)\n", board_name.c_str(), err.c_str());
  }
  return FW_STATUS_SUCCESS;
}
#endif

