/*
 *
 * Copyright 2020-present Facebook. All Rights Reserved.
 *
 * GrandCanyon 2.0 Project
 *
 */
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
    printf("Starting BIC FW update, fw_comp: 0x%x\n", fw_comp);
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

BicFwComponent bic_fw1("server", "bic", FW_BIC);