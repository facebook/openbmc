#include <cstdio>
#include <syslog.h>
#include <openbmc/vr.h>
#include <facebook/fby35_common.h>
#include <facebook/bic.h>
#include "bic_vr.h"

extern "C" {
extern void plat_vr_preinit(uint8_t slot, const char *name);
}

using namespace std;

static map<uint8_t, map<uint8_t, string>> list = {
  {FW_VR_VCCIN,     {{VCCIN_ADDR, "VCCIN/VCCFA_EHV_FIVRA"}}},
  {FW_VR_VCCD,      {{VCCD_ADDR, "VCCD"}}},
  {FW_VR_VCCINFAON, {{VCCINFAON_ADDR, "VCCINFAON/VCCFA_EHV"}}}
};

bool VrComponent::vr_printed = false;

int VrComponent::update_internal(const string& image, bool force) {
  int ret = 0;

  if (fw_comp == FW_VR) {
    return FW_STATUS_NOT_SUPPORTED;
  }

  try {
    server.ready();
  } catch (string& err) {
    cerr << err << endl;
    return FW_STATUS_NOT_SUPPORTED;
  }

  plat_vr_preinit(slot_id, fru().c_str());
  if (vr_probe() < 0) {
    cout << "VR probe failed!" << endl;
    return FW_STATUS_FAILURE;
  }

  syslog(LOG_CRIT, "Updating %s on %s. File: %s", get_component_name(fw_comp),
         fru().c_str(), image.c_str());

  auto vr = list.find(fw_comp)->second.begin();
  ret = vr_fw_update(vr->second.c_str(), (char *)image.c_str(), force);
  vr_remove();
  syslog(LOG_CRIT, "Updated %s on %s. File: %s. Result: %s", get_component_name(fw_comp),
         fru().c_str(), image.c_str(), (ret) ? "Fail" : "Success");
  if (ret) {
    cout << "ERROR: VR Firmware update failed!" << endl;
    return FW_STATUS_FAILURE;
  }

  return FW_STATUS_SUCCESS;
}

int VrComponent::update(const string image) {
  return update_internal(image, false);
}

int VrComponent::fupdate(const string image) {
  return update_internal(image, true);
}

int VrComponent::get_ver_str(const string& name, string& s) {
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

int VrComponent::print_version() {
  string ver("");
  map<uint8_t, map<uint8_t, string>>::iterator iter;

  if (fw_comp == FW_VR) {
    vr_printed = true;
    iter = list.begin();
  } else {
    if (vr_printed == true) {
      // "vr" is prior to "vr_xxx",
      // so "vr_xxx" does not need to print version
      return FW_STATUS_SUCCESS;
    }
    iter = list.find(fw_comp);
  }
  for (; iter != list.end(); ++iter) {
    auto vr = iter->second.begin();  // <addr, name>
    try {
      server.ready();
      if (get_ver_str(vr->second, ver) < 0) {
        throw "Error in getting the version of " + vr->second;
      }
      cout << vr->second << " Version: " << ver << endl;
    } catch (string& err) {
      printf("%s Version : NA (%s)\n", vr->second.c_str(), err.c_str());
    }
    if (fw_comp != FW_VR) {
      break;
    }
  }

  return FW_STATUS_SUCCESS;
}

void VrComponent::get_version(json& j) {
  string ver("");
  map<uint8_t, map<uint8_t, string>>::iterator iter;

  iter = (fw_comp == FW_VR) ? list.begin() : list.find(fw_comp);
  for (; iter != list.end(); ++iter) {
    auto vr = iter->second.begin();  // <addr, name>
    try {
      server.ready();
      if (get_ver_str(vr->second, ver) < 0) {
        throw "Error in getting the version of " + vr->second;
      }
      //For IFX and RNS, the str is $vendor $ver, Remaining Writes: $times
      //For TI, the str is $vendor_token1 $vendor_token2 $ver
      string tmp_str("");
      size_t start = 0;
      auto end = ver.find(',');
      bool is_TI_VR = false;
      if (end == string::npos) {
        is_TI_VR = true;
        start = ver.rfind(' ');
        end = ver.size();
      } else {
        start = ver.find(' ');
      }

      tmp_str = ver.substr(0, start);
      transform(tmp_str.begin(), tmp_str.end(), tmp_str.begin(), ::tolower);
      j["VERSION"][vr->second]["vendor"] = tmp_str;

      start++;
      tmp_str = ver.substr(start, end - start);
      transform(tmp_str.begin(), tmp_str.end(), tmp_str.begin(), ::tolower);
      j["VERSION"][vr->second]["version"] = tmp_str;
      if (is_TI_VR == true) {
        return;
      }

      start = ver.rfind(' ');
      end = ver.size();
      tmp_str = ver.substr(start, end - start);
      transform(tmp_str.begin(), tmp_str.end(), tmp_str.begin(), ::tolower);
      j["VERSION"][vr->second]["rmng_w"] = tmp_str;
    } catch (string& err) {
      if (err.find("empty") != string::npos) {
        j["VERSION"] = "not_present";
      } else {
        j["VERSION"] = "error_returned";
      }
    }
    if (fw_comp != FW_VR) {
      break;
    }
  }
}
