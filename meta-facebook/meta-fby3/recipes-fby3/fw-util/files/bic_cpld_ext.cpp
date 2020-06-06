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

int CpldExtComponent::get_ver_str(string& s) {
  char ver[32] = {0};
  uint8_t rbuf[4] = {0};
  int ret = 0;
  ret = bic_get_fw_ver(slot_id, fw_comp, rbuf);
  snprintf(ver, sizeof(ver), "%02X%02X%02X%02X", rbuf[3], rbuf[2], rbuf[1], rbuf[0]);
  s = string(ver);
  return ret;
}

int CpldExtComponent::print_version() {
  string ver("");
  string board_name = name;
  transform(board_name.begin(), board_name.end(), board_name.begin(), ::toupper);
  try {
    server.ready();
    expansion.ready();
    // Print Bridge-IC Version
    if ( get_ver_str(ver) < 0 ) {
      throw "Error in getting the version of " + board_name;
    }
    cout << board_name << " CPLD Version: " << ver << endl;
  } catch(string& err) {
    printf("%s CPLD Version: NA (%s)\n", board_name.c_str(), err.c_str());
  }
  return FW_STATUS_SUCCESS;
}

void CpldExtComponent::get_version(json& j) {
  string ver("");
  try {
    server.ready();
    expansion.ready();
    if ( get_ver_str(ver) < 0 ) {
      throw "Error in getting the version of " + name;
    }

    j["VERSION"] = ver;
  } catch(string& err) {
    if ( err.find("empty") != string::npos ) j["VERSION"] = "not_present";
    else j["VERSION"] = "error_returned";
  }
}
#endif

