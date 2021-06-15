#include "fw-util.h"
#include <cstdio>
#include <cstring>
#include "server.h"
#include <openbmc/pal.h>
#include "bic_fw_ext.h"
#ifdef BIC_SUPPORT
#include <facebook/bic.h>

using namespace std;

int BicFwExtComponent::update_internal(string image, bool force) {
  int ret = 0;
  try {
    server.ready();
    expansion.ready();
    ret = bic_update_fw(slot_id, fw_comp, (char *)image.c_str(), (force)?FORCE_UPDATE_SET:FORCE_UPDATE_UNSET);

    if (ret != BIC_FW_UPDATE_SUCCESS) {
      switch(ret) {
      case BIC_FW_UPDATE_FAILED:
        cerr << this->alias_component() << ": update process failed" << endl;
        break;
      case BIC_FW_UPDATE_NOT_SUPP_IN_CURR_STATE:
        cerr << this->alias_component() << ": firmware update not supported in current state." << endl;
        break;
      default:
        cerr << this->alias_component() << ": unknow error (ret: " << ret << ")" << endl;
        break;
      }
      return FW_STATUS_FAILURE;
    }
  } catch (string err) {
    printf("%s\n", err.c_str());
    return FW_STATUS_NOT_SUPPORTED;
  }
  return ret;
}

int BicFwExtComponent::update(string image) {
  return update_internal(image, false);
}

int BicFwExtComponent::fupdate(string image) {
  return update_internal(image, true);
}

int BicFwExtComponent::get_ver_str(string& s) {
  uint8_t ver[32] = {0};
  char ver_str[32] = {0};
  int ret = 0;
  // Get Bridge-IC Version
  ret = bic_get_fw_ver(slot_id, fw_comp, ver);
  snprintf(ver_str, sizeof(ver_str), "v%x.%02x", ver[0], ver[1]);
  s = string(ver_str);
  return ret;
}

int BicFwExtComponent::print_version() {
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
    cout << board_name << " Bridge-IC Version: " << ver << endl;
  } catch(string err) {
    printf("%s Bridge-IC Version: NA (%s)\n", board_name.c_str(), err.c_str());
  }
  return FW_STATUS_SUCCESS;
}

void BicFwExtComponent::get_version(json& j) {
  string ver("");
  try {
    server.ready();
    expansion.ready();
    if ( get_ver_str(ver) < 0 ) {
      throw "Error in getting the version of " + component();
    } else {
      j["VERSION"] = ver;
    }
  } catch(string err) {
    if ( err.find("empty") != string::npos ) j["VERSION"] = "not_present";
    else j["VERSION"] = "error_returned";
  }
}

int BicFwExtBlComponent::update_internal(string image, bool force) {
  int ret = 0;
  try {
    server.ready();
    expansion.ready();
    ret = bic_update_fw(slot_id, fw_comp, (char *)image.c_str(), (force)?FORCE_UPDATE_SET:FORCE_UPDATE_UNSET);

    if (ret != BIC_FW_UPDATE_SUCCESS) {
      switch(ret) {
      case BIC_FW_UPDATE_FAILED:
        cerr << this->alias_component() << ": update process failed" << endl;
        break;
      case BIC_FW_UPDATE_NOT_SUPP_IN_CURR_STATE:
        cerr << this->alias_component() << ": firmware update not supported in current state." << endl;
        break;
      default:
        cerr << this->alias_component() << ": unknow error (ret: " << ret << ")" << endl;
        break;
      }
      return FW_STATUS_FAILURE;
    }
  } catch(string err) {
    printf("%s\n", err.c_str());
    return FW_STATUS_NOT_SUPPORTED;
  }
  return ret;
}

int BicFwExtBlComponent::update(string image) {
  return update_internal(image, false);
}

int BicFwExtBlComponent::get_ver_str(string& s) {
  uint8_t ver[32] = {0};
  char ver_str[32] = {0};
  int ret = 0;
  // Get Bridge-IC Version
  ret = bic_get_fw_ver(slot_id, fw_comp, ver);
  snprintf(ver_str, sizeof(ver_str), "v%x.%02x", ver[0], ver[1]);
  s = string(ver_str);
  return ret;
}

int BicFwExtBlComponent::print_version() {
  string ver("");
  string board_name = name;
  transform(board_name.begin(), board_name.end(), board_name.begin(), ::toupper);
  try {
    server.ready();
    expansion.ready();
    // Print Bridge-IC Bootloader Version
    if ( get_ver_str(ver) < 0 ) {
      throw "Error in getting the version of " + board_name;
    }
    cout << board_name << " Bridge-IC Bootloader Version: " << ver << endl;
  } catch(string err) {
    printf("%s Bridge-IC Bootloader Version: NA (%s)\n", board_name.c_str(), err.c_str());
  }
  return FW_STATUS_SUCCESS;
}

void BicFwExtBlComponent::get_version(json& j) {
  string ver("");
  try {
    server.ready();
    expansion.ready();
    if ( get_ver_str(ver) < 0 ) {
      throw "Error in getting the version of " + component();
    }
    j["VERSION"] = ver;
  } catch(string err) {
    if ( err.find("empty") != string::npos ) j["VERSION"] = "not_present";
    else j["VERSION"] = "error_returned";
  }
}
#endif

