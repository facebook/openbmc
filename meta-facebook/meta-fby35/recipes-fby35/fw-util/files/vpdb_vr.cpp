#include <cstdio>
#include <syslog.h>
#include <openbmc/vr.h>
#include <facebook/fby35_common.h>
#include <facebook/bic.h>
#include "vpdb_vr.h"
#include <openbmc/kv.h>

extern "C" {
extern void plat_vr_preinit(uint8_t slot, const char *name);
}

using namespace std;

const string vpdb_vr_name = "VPDB_VR";

int VpdbVrComponent::update_internal(const string& image, bool force) {
  int ret = 0;

  if (fw_comp != FW_VPDB_VR) {
    return FW_STATUS_NOT_SUPPORTED;
  }

  plat_vr_preinit(slot_id, fru().c_str());
  if (vr_probe() < 0) {
    cout << "VR probe failed!" << endl;
    return FW_STATUS_FAILURE;
  }

  syslog(LOG_CRIT, "Updating %s on %s. File: %s", get_component_name(fw_comp),
         fru().c_str(), image.c_str());

  ret = vr_fw_update(vpdb_vr_name.c_str(), (char *)image.c_str(), force);
  vr_remove();
  syslog(LOG_CRIT, "Updated %s on %s. File: %s. Result: %s", get_component_name(fw_comp),
         fru().c_str(), image.c_str(), (ret) ? "Fail" : "Success");
  if (ret) {
    cout << "ERROR: VR Firmware update failed!" << endl;
    return FW_STATUS_FAILURE;
  }

  return FW_STATUS_SUCCESS;
}

int VpdbVrComponent::update(const string image) {
  return update_internal(image, false);
}

int VpdbVrComponent::fupdate(const string image) {
  return update_internal(image, true);
}

int VpdbVrComponent::get_ver_str(const string& name, string& s) {
  int ret = 0;
  char ver[MAX_VER_STR_LEN] = {0};

  plat_vr_preinit(slot_id, fru().c_str());
  if (vr_probe() < 0) {
    cout << "VR probe failed!" << endl;
    return -1;
  }

  ret = vr_fw_version(-1, name.c_str(), ver);
  vr_remove();
  if (ret) {
    return -1;
  }
  s = string(ver);

  return 0;
}

int VpdbVrComponent::print_version() {
  string ver("");
  char ver_key[MAX_KEY_LEN] = {0};
  char value[MAX_VALUE_LEN] = {0};

  try {
    if (get_ver_str(vpdb_vr_name, ver) < 0) {
      throw "Error in getting the version of " + vpdb_vr_name;
    }
    cout << vpdb_vr_name << " Version: " << ver << endl;
    snprintf(ver_key, sizeof(ver_key), "%s_vr_%02xh_new_crc", fru().c_str(), VPDB_VR_ADDR);
    //The key will be set, if VR firmware updating is success.
    if (kv_get(ver_key, value, NULL, KV_FPERSIST) == 0) {
      cout << vpdb_vr_name << " Version after activation: " << value << endl;
    } else { // no update before
      cout << vpdb_vr_name << " Version after activation: " << ver << endl;
    }
  } catch (string& err) {
    cout << vpdb_vr_name << " Version : NA (" << err.c_str() << ")" << endl;
  }

  return FW_STATUS_SUCCESS;
}

int VpdbVrComponent::get_version(json& j) {
  string ver("");

  try {
    if (get_ver_str(vpdb_vr_name, ver) < 0) {
      throw "Error in getting the version of " + vpdb_vr_name;
    }
    //For IFX and RNS, the str is $vendor $ver, Remaining Writes: $times
    //For TI, the str is $vendor_token1 $vendor_token2 $ver
    string tmp_str("");
    size_t start = 0;
    auto end = ver.find(',');
    start = ver.find(' ');

    tmp_str = ver.substr(0, start);
    transform(tmp_str.begin(), tmp_str.end(), tmp_str.begin(), ::tolower);
    j["VERSION"][vpdb_vr_name]["vendor"] = tmp_str;

    start++;
    tmp_str = ver.substr(start, end - start);
    transform(tmp_str.begin(), tmp_str.end(), tmp_str.begin(), ::tolower);
    j["VERSION"][vpdb_vr_name]["version"] = tmp_str;

    start = ver.rfind(' ');
    end = ver.size();
    tmp_str = ver.substr(start, end - start);
    transform(tmp_str.begin(), tmp_str.end(), tmp_str.begin(), ::tolower);
    j["VERSION"][vpdb_vr_name]["rmng_w"] = tmp_str;
  } catch (string& err) {
    if (err.find("empty") != string::npos) {
      j["VERSION"] = "not_present";
    } else {
      j["VERSION"] = "error_returned";
    }
  }

  return FW_STATUS_SUCCESS;
}
