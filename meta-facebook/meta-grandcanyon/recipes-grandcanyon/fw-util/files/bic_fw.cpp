#include <cstdio>
#include <cstring>
#include <fw-util.h>
#include <facebook/bic.h>
#include <facebook/bic_fwupdate.h>
#include "server.h"

using namespace std;

enum {
  BIC_FW_UPDATE_SUCCESS                =  0,
  BIC_FW_UPDATE_FAILED                 = -1,
  BIC_FW_UPDATE_NOT_SUPP_IN_CURR_STATE = -2,
};

class BicFwComponent : public Component {
  uint8_t fw_comp = 0;
  Server server;

  private:
    int get_ver_str(string& ver);
    int update_internal(string image, bool force);
  public:
    BicFwComponent(string fru, string comp, uint8_t _fw_comp)
      : Component(fru, comp), fw_comp(_fw_comp), server(FRU_SERVER, fru) {}
    int update(string image);
    int fupdate(string image);
    int print_version();
    int get_version(json& j) override;
};

class BicFwBlComponent : public Component {
  uint8_t fw_comp = 0;
  Server server;

  private:
    int get_ver_str(string& ver);
    int update_internal(string image, bool force);
  public:
    BicFwBlComponent(string fru, string comp, uint8_t _fw_comp)
      : Component(fru, comp), fw_comp(_fw_comp), server(FRU_SERVER, fru) {}
    int update(string image);
    int fupdate(string image);
    int print_version();
    int get_version(json& j) override;
};

int BicFwComponent::update_internal(string image, bool force) {
  int ret = 0;

  try {
    if (force == false) {
      server.ready();
    }
    ret = bic_update_fw(FRU_SERVER, fw_comp, (char *)image.c_str(), (force) ? FORCE_UPDATE_SET : FORCE_UPDATE_UNSET);

    if (ret != BIC_FW_UPDATE_SUCCESS) {
      switch(ret) {
      case BIC_FW_UPDATE_FAILED:
        cerr << this->alias_component() << ": update process failed" << endl;
        break;
      case BIC_FW_UPDATE_NOT_SUPP_IN_CURR_STATE:
        cerr << this->alias_component() << ": firmware update not supported in current state." << endl;
        break;
      default:
        cerr << this->alias_component() << ": unknown error (ret: " << ret << ")" << endl;
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

int BicFwComponent::update(string image) {
  return update_internal(image, false);
}

int BicFwComponent::fupdate(string image) {
  return update_internal(image, true);
}

int BicFwComponent::get_ver_str(string& s) {
  uint8_t ver[MAX_BIC_VER_STR_LEN] = {0};
  char ver_str[MAX_BIC_VER_STR_LEN] = {0};
  int ret = 0;

  // Get Bridge-IC Version
  ret = bic_get_fw_ver(FRU_SERVER, fw_comp, ver);
  snprintf(ver_str, sizeof(ver_str), "v%x.%02x", ver[0], ver[1]);
  s = string(ver_str);

  return ret;
}

int BicFwComponent::print_version() {
  string ver("");

  try {
    server.ready();
    // Print Bridge-IC Version
    if ( get_ver_str(ver) < 0 ) {
      throw string("Error in getting the version of server BIC");
    }
    cout << "Bridge-IC Version: " << ver << endl;
  } catch (string err) {
    printf("Bridge-IC Version: NA (%s)\n", err.c_str());
  }

  return FW_STATUS_SUCCESS;
}

int BicFwComponent::get_version(json& j) {
  string ver("");

  try {
    server.ready();
    if ( get_ver_str(ver) < 0 ) {
      throw "Error in getting the version of server BIC";
    } else {
      j["VERSION"] = ver;
    }
  } catch(string err) {
    if ( err.find("empty") != string::npos ) {
      j["VERSION"] = "not_present";
    } else {
      j["VERSION"] = "error_returned";
    }
  }
  return FW_STATUS_SUCCESS;
}

int BicFwBlComponent::update_internal(string image, bool force) {
  int ret = 0;

  try {
    if (force == false) {
      server.ready();
    }
    ret = bic_update_fw(FRU_SERVER, fw_comp, (char *)image.c_str(), (force) ? FORCE_UPDATE_SET : FORCE_UPDATE_UNSET);

    if (ret != BIC_FW_UPDATE_SUCCESS) {
      switch(ret) {
      case BIC_FW_UPDATE_FAILED:
        cerr << this->alias_component() << ": update process failed" << endl;
        break;
      case BIC_FW_UPDATE_NOT_SUPP_IN_CURR_STATE:
        cerr << this->alias_component() << ": firmware update not supported in current state." << endl;
        break;
      default:
        cerr << this->alias_component() << ": unknown error (ret: " << ret << ")" << endl;
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

int BicFwBlComponent::update(string image) {
  return update_internal(image, false);
}

int BicFwBlComponent::fupdate(string image) {
  return update_internal(image, true);
}

int BicFwBlComponent::get_ver_str(string& s) {
  uint8_t ver[MAX_BIC_VER_STR_LEN] = {0};
  char ver_str[MAX_BIC_VER_STR_LEN] = {0};
  int ret = 0;

  // Get Bridge-IC Version
  ret = bic_get_fw_ver(FRU_SERVER, fw_comp, ver);
  snprintf(ver_str, sizeof(ver_str), "v%x.%02x", ver[0], ver[1]);
  s = string(ver_str);

  return ret;
}

int BicFwBlComponent::print_version() {
  string ver("");

  try {
    server.ready();

    // Print Bridge-IC Bootloader Version
    if (get_ver_str(ver) < 0) {
      throw string("Error in getting the version of server BIC Bootloader");
    }
    cout << "Bridge-IC Bootloader Version: " << ver << endl;
  } catch (string err) {
    printf("Bridge-IC Bootloader Version: NA (%s)\n", err.c_str());
  }

  return FW_STATUS_SUCCESS;
}

int BicFwBlComponent::get_version(json& j) {
  string ver("");

  try {
    server.ready();
    if (get_ver_str(ver) < 0) {
      throw "Error in getting the version of server BIC Bootloader";
    }
    j["VERSION"] = ver;
  } catch(string err) {
    if ( err.find("empty") != string::npos ) {
      j["VERSION"] = "not_present";
    } else {
      j["VERSION"] = "error_returned";
    }
  }
  return FW_STATUS_SUCCESS;
}

BicFwComponent bic_fw1("server", "bic", FW_BIC);
BicFwBlComponent bicbl_fw1("server", "bicbl", FW_BIC_BOOTLOADER);
