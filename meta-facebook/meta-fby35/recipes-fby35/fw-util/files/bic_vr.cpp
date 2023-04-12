#include <cstdio>
#include <openbmc/kv.hpp>
#include <fmt/format.h>
#include <syslog.h>
#include <openbmc/vr.h>
#include <facebook/fby35_common.h>
#include <facebook/bic.h>

#include "bic_vr.h"

extern "C" {
extern void plat_vr_preinit(uint8_t slot, const char *name);
extern int hd_vr_addr_table[][3];
}

using namespace std;

static map<uint8_t, map<uint8_t, string>> crater_lake_vr_list = {
  {FW_VR_VCCIN,     {{VCCIN_ADDR, "VCCIN/VCCFA_EHV_FIVRA"}}},
  {FW_VR_VCCD,      {{VCCD_ADDR, "VCCD"}}},
  {FW_VR_VCCINFAON, {{VCCINFAON_ADDR, "VCCINFAON/VCCFA_EHV"}}}
};

/*
 * halfdome vr address are different from each vr vendor
 * each slot obtain its own vr list according to vr vendor
 * */
static map<uint8_t, map<uint8_t, map<uint8_t, string>>> halfdome_vr_table;
static vector<string> halfdome_vr_names = {
    "VDDCR_CPU0/VDDCR_SOC", "VDDCR_CPU1/VDDIO", "VDD11_S3"};

static map<uint8_t, map<uint8_t, string>> great_lake_vr_list = {
  {FW_VR_VCCIN_EHV, {{GL_VCCIN_ADDR, "VCCIN/VCCFA_EHV"}}},
  {FW_VR_VCCD_HV, {{GL_VCCD_ADDR, "VCCD_HV"}}},
  {FW_VR_VCCINF, {{GL_VCCINF_ADDR, "VCCINF/VCCFA_EHV_FIVRA"}}}
};

static map<uint8_t, map<uint8_t, string>> rainbow_falls_vr_list = {
  {FW_1OU_VR_V9_ASICA, {{VR_1OU_V9_ASICA_ADDR, "1OU_VR_P0V9/P0V8_ASICA"}}},
  {FW_1OU_VR_VDDQAB, {{VR_1OU_VDDQAB_ADDR, "1OU_VR_VDDQAB/D0V8"}}},
  {FW_1OU_VR_VDDQCD, {{VR_1OU_VDDQCD_ADDR, "1OU_VR_VDDQCD"}}}
};

bool VrComponent::vr_printed = false;
bool VrComponent::rbf_vr_printed = false;

int VrComponent::update_internal(const string& image, bool force) {
  int ret = 0;

  if (fw_comp == FW_VR || fw_comp == FW_1OU_VR) {
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

  auto& list = get_vr_list();
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
  constexpr auto key_format = "slot{}_{}vr_{:x}h_new_crc";
  string ver("");
  map<uint8_t, map<uint8_t, string>>::iterator iter;
  bool is_1ou = false;
  auto& list = get_vr_list();

  switch (fw_comp) {
    case FW_VR:
      vr_printed = true;
      iter = list.begin();
      break;
    case FW_1OU_VR:
      rbf_vr_printed = true;
      is_1ou = true;
      iter = list.begin();
      break;
    case FW_1OU_VR_V9_ASICA:
    case FW_1OU_VR_VDDQAB:
    case FW_1OU_VR_VDDQCD:
      if (rbf_vr_printed) {
        // "1ou_vr" is prior to "1ou_vr_xxx",
        // so "1ou_vr_xxx" does not need to print version
        return FW_STATUS_SUCCESS;
      }
      is_1ou = true;
      iter = list.find(fw_comp);
      break;
    default:
      if (vr_printed) {
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

      auto key = fmt::format(key_format, slot_id, is_1ou ? "1ou_" : "", vr->first);
      //The key will be set, if VR firmware updating is success.
      try {
        auto value = kv::get(key, kv::region::persist);
        cout << vr->second << " Version after activation: " << value << endl;
      } catch (const std::exception& e) { // no update before
        cout << vr->second << " Version after activation: " << ver << endl;
      }
    } catch (string& err) {
      cout << fmt::format("{} Version : NA ({})",vr->second, err)  << endl;
    }
    if (fw_comp != FW_VR && fw_comp != FW_1OU_VR) {
      break;
    }
  }

  return FW_STATUS_SUCCESS;
}

int VrComponent::get_version(json& j) {
  string ver("");
  map<uint8_t, map<uint8_t, string>>::iterator iter;
  auto& list = get_vr_list();

  iter = (fw_comp == FW_VR || fw_comp == FW_1OU_VR) ? list.begin() : list.find(fw_comp);
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
        return FW_STATUS_SUCCESS;
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
    if (fw_comp != FW_VR && fw_comp != FW_1OU_VR) {
      break;
    }
  }
  return FW_STATUS_SUCCESS;
}

map<uint8_t, map<uint8_t, string>>&  VrComponent::get_vr_list() {

  if( fw_comp == FW_1OU_VR || rainbow_falls_vr_list.count(fw_comp) > 0) {
      return rainbow_falls_vr_list;
  }

  if (fby35_common_get_slot_type(slot_id) == SERVER_TYPE_HD) {
    if (!halfdome_vr_table.contains(slot_id)) {
      //Generate vr list of a slot if not exist
      uint8_t board_rev = 0, vr_type = 0;
      get_board_rev(slot_id, BOARD_ID_SB, &board_rev);
      vr_type = get_vr_type(board_rev);

      for (int vr_comp = FW_VR_VDDCRCPU0; vr_comp <= FW_VR_VDD11S3; vr_comp++ ) {
        auto idx = vr_comp - FW_VR_VDDCRCPU0;
        halfdome_vr_table[slot_id][vr_comp] = {{hd_vr_addr_table[vr_type][idx], halfdome_vr_names[idx]}};
      }
    }
    return halfdome_vr_table[slot_id];
  } else if (fby35_common_get_slot_type(slot_id) == SERVER_TYPE_GL) {
    return great_lake_vr_list;
  } else {
    return  crater_lake_vr_list;
  }
}
