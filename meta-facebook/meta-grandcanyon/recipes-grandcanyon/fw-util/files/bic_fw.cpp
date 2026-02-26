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
    BicFwComponent(const string& fru, const string& comp, uint8_t _fw_comp)
      : Component(fru, comp), fw_comp(_fw_comp), server(FRU_SERVER, fru) {}
    int update(const string& image);
    int fupdate(const string& image);
    int print_version();
    int get_version(json& j) override;
};

class BicFwRecoveryComponent : public Component {
  uint8_t fw_comp = 0;

  int update_internal(const std::string& image, bool force) {
    int ret = bic_update_fw(FRU_SERVER, fw_comp,
                            const_cast<char*>(image.c_str()),
                            force ? FORCE_UPDATE_SET : FORCE_UPDATE_UNSET);

    if (ret != BIC_FW_UPDATE_SUCCESS) {
      cerr << this->alias_component() << ": recovery failed (ret: " << ret << ")" << endl;
      return FW_STATUS_FAILURE;
    }
    return FW_STATUS_SUCCESS;
  }

 public:
  BicFwRecoveryComponent(const std::string& fru, const std::string& comp, uint8_t _fw_comp)
    : Component(fru, comp), fw_comp(_fw_comp) {}

  int update(const std::string& image) override { return update_internal(image, false); }
  int fupdate(const std::string& image) override { return update_internal(image, true); }

  int print_version() override { return FW_STATUS_SUCCESS; }
  int get_version(json& j) override {
    j["VERSION"] = "not_supported";
    return FW_STATUS_SUCCESS;
  }
};

class BicFwBlComponent : public Component {
  uint8_t fw_comp = 0;
  Server server;

  private:
    int get_ver_str(string& ver);
    int update_internal(string image, bool force);
  public:
    BicFwBlComponent(const string& fru, const string& comp, uint8_t _fw_comp)
      : Component(fru, comp), fw_comp(_fw_comp), server(FRU_SERVER, fru) {}
    int update(const string& image);
    int fupdate(const string& image);
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

int BicFwComponent::update(const string& image) {
  return update_internal(image, false);
}

int BicFwComponent::fupdate(const string& image) {
  return update_internal(image, true);
}

#ifdef CONFIG_GRANDCANYON2
int BicFwComponent::get_ver_str(string& s) {
  int ret = 0;
  uint8_t rbuf[32] = {0};

  ret = bic_get_fw_ver(FRU_SERVER, fw_comp, rbuf);
  if (!ret) {
    stringstream ver;
    size_t len = strlen((char *)rbuf);
    if (len >= 4) {         // new version format
      ver << "obgc2-" << string((char *)(rbuf + 4)) << "-v" << hex << setfill('0')
          << setw(2) << (int)rbuf[0] << setw(2) << (int)rbuf[1] << "."
          << setw(2) << (int)rbuf[2] << "." << setw(2) << (int)rbuf[3];
    } else if (len == 2) {  // old version format
      ver << "v" << hex << (int)rbuf[0] << "." << setfill('0') << setw(2) << (int)rbuf[1];
    } else {
      ver << "Format not supported";
    }
    s = ver.str();
  }

  return ret;
}  
#else
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

#endif


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

int BicFwBlComponent::update(const string& image) {
  return update_internal(image, false);
}

int BicFwBlComponent::fupdate(const string& image) {
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

#ifdef CONFIG_GRANDCANYON2
  // There is no BIC bootloader in Grand Canyon 2.0(AST1030)
BicFwRecoveryComponent bic_rcvy1("server", "bic_rcvy", FW_BIC_RECOVERY);
#else
BicFwBlComponent bicbl_fw1("server", "bicbl", FW_BIC_BOOTLOADER);
#endif

