#include <cstdio>
#include <facebook/bic_ipmi.h>
#include <facebook/bic_xfer.h>
#include "bic_vr.h"

using namespace std;

static map<uint8_t, map<uint8_t, string>> list = {
  {FW_VR_VCCIN,     {{VCCIN_ADDR, "VCCIN/VCCFA_EHV_FIVRA"}}},
  {FW_VR_VCCD,      {{VCCD_ADDR, "VCCD"}}},
  {FW_VR_VCCINFAON, {{VCCINFAON_ADDR, "VCCINFAON/VCCFA_EHV"}}}
};

bool VrComponent::vr_printed = false;

int VrComponent::update(string image) {
  int ret = 0;

  if (fw_comp == FW_VR) {
    return FW_STATUS_NOT_SUPPORTED;
  }

  try {
    server.ready();
  } catch (string& err) {
    printf("%s\n", err.c_str());
    return FW_STATUS_NOT_SUPPORTED;
  }

  ret = bic_update_fw(slot_id, fw_comp, (char *)image.c_str(), FORCE_UPDATE_UNSET);
  if (ret < 0) {
    return FW_STATUS_FAILURE;
  }

  return FW_STATUS_SUCCESS;
}

int VrComponent::get_ver_str(uint8_t addr, string& s) {
  int ret = 0;
  uint8_t bus = 0x5;
  char ver[MAX_VER_STR_LEN] = {0};

  ret = bic_get_vr_ver_cache(slot_id, NONE_INTF, bus, addr, ver);
  if (!ret) {
    s = string(ver);
  }

  return ret;
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
      if (get_ver_str(vr->first, ver) < 0) {
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
      if (get_ver_str(vr->first, ver) < 0) {
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
