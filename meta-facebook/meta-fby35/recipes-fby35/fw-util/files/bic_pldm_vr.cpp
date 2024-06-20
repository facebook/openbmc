#include "bic_pldm_vr.hpp"
#include "signed_info.hpp"

#include <openbmc/pal.h>
#include <openbmc/kv.hpp>
#include <facebook/bic.h>
#include <facebook/bic_ipmi.h>

#include <fmt/format.h>
#include <iostream>
#include <regex>

using namespace std;

int PldmVrComponent::update_version_cache() {
  string activeVersion, pendingVersion;

  if (PldmComponent::update_version_cache()) {
    return FW_STATUS_FAILURE;
  }
  activeVersion = kv::get(activeVersionKey, kv::region::temp);
  pendingVersion = kv::get(pendingVersionKey, kv::region::temp);

  // When the BIC fails to access this component, it returns "ERROR:%d."
  if (activeVersion.find("ERROR:") != string::npos || 
      pendingVersion.find("ERROR:") != string::npos) {   
    // Set as INVALID_VERSION so that the update version cache is triggered next time
    kv::set(activeVersionKey, INVALID_VERSION, kv::region::temp);
    kv::set(pendingVersionKey, INVALID_VERSION, kv::region::temp);
  }
  return FW_STATUS_SUCCESS;
}

int PldmVrComponent::update_internal(string image, bool force) {
  uint8_t status;
  int rc;

  if (component_identifier == javaisland::ALL_VR) {
    return FW_STATUS_NOT_SUPPORTED;
  }

  try {
    server.ready();
  } catch (const string& e) {
    cerr << e << endl;
    return FW_STATUS_FAILURE;
  }
  
  try {
   if (pal_get_server_power(slot_id, &status)) {
      throw runtime_error("Failed to get server power");
    }

    if (status != SERVER_POWER_OFF) {
      throw runtime_error(
        "The sever power is not turned off. "
        "Power off (not 12V-off) before updating VR");
    }

    if (bic_set_gpio(slot_id, JI_FM_VR_FW_PROGRAM_L, GPIO_LOW)) {
      throw runtime_error("Setting GPIO FM_VR_FW_PROGRAM_L to LOW failed");
    } else {
      cout << "FM_VR_FW_PROGRAM_L is set to LOW successfully" << endl;
    }

    if (bic_set_gpio(slot_id, JI_BIC_CPLD_VRD_MUX_SEL, GPIO_LOW)) {
      throw runtime_error("Setting GPIO BIC_CPLD_VRD_MUX_SEL to LOW failed");
    } else {
      cout << "BIC_CPLD_VRD_MUX_SEL is set to LOW successfully" << endl;
    }

    cout << "Setting server power on..." << endl;
    if (bic_server_power_on(slot_id)) {
      throw runtime_error("Failed to set server power to on");
    }
    sleep(1); // wait to power on

    if (PldmComponent::update_internal(image, force)) {
      rc = FW_STATUS_FAILURE;
    } else {
      rc = FW_STATUS_SUCCESS;
    }

    cout << "Setting server power off..." << endl;
    if (bic_server_power_off(slot_id)) {
      throw runtime_error("Failed to set server power to off");
    }
    sleep(1); // wait to power off

  } catch (const exception& e) {
    cerr << e.what() << endl;
    rc = FW_STATUS_FAILURE;
  }

  if (bic_set_gpio(slot_id, JI_FM_VR_FW_PROGRAM_L, GPIO_HIGH)) {
    cerr << "Setting GPIO FM_VR_FW_PROGRAM_L to HIGH failed" << endl;
    rc = FW_STATUS_FAILURE;
  } else {
    cout << "FM_VR_FW_PROGRAM_L is set to HIGH successfully" << endl;
  }

  if (bic_set_gpio(slot_id, JI_BIC_CPLD_VRD_MUX_SEL, GPIO_HIGH)) {
    cerr << "Setting GPIO BIC_CPLD_VRD_MUX_SEL to HIGH failed" << endl;
    rc = FW_STATUS_FAILURE;
  } else {
    cout << "BIC_CPLD_VRD_MUX_SEL is set to HIGH successfully" << endl;
  }

  return rc;
}

int PldmVrComponent::get_version(json& j) {
  try {
    server.ready();
  } catch(const string& e) {
    throw runtime_error(e);
  }

  if (component_identifier == javaisland::ALL_VR) {
    json j_temp;
    for (auto& [vr_comp_id, vr_name]: get_vr_list()) {
      // replace component name, component ID and version key 
      // to let get_version() get the version of a specific VR 
      component = vr_name;
      component_identifier = vr_comp_id;
      activeVersionKey = fmt::format("{}_{}_active_ver", fru, vr_name);
      pendingVersionKey = fmt::format("{}_{}_pending_ver", fru, vr_name);
      if (get_version(j_temp)) {
        return FW_STATUS_FAILURE;
      }
      transform(vr_name.begin(), vr_name.end(), vr_name.begin(), ::toupper);
      j[VERSION][vr_name][VERSION] = j_temp[VERSION];
      j[VERSION][vr_name][VENDOR] = j_temp[VENDOR];
      if (j_temp.contains(RMNG_W)) {
        j[VERSION][vr_name][RMNG_W] = j_temp[RMNG_W];
      }
      j_temp.clear();
    }
    return FW_STATUS_SUCCESS;
  }

  if (PldmComponent::get_version(j)) {
    return FW_STATUS_FAILURE;
  }
  auto activeVersion = kv::get(activeVersionKey, kv::region::temp);
  auto pendingVersion = kv::get(pendingVersionKey, kv::region::temp);
  
  // If the active version is valid but the pending version is invalid, let 
  // the pending version equal the active version
  if (activeVersion.find(INVALID_VERSION) == string::npos && 
      pendingVersion.find(INVALID_VERSION) != string::npos) {
    pendingVersion = activeVersion;
    kv::set(pendingVersionKey, pendingVersion, kv::region::temp);
  }

  // VR active version format is "<VENDOR NAME> <VERSION>" or
  // <VENDOR NAME> <VERSION>, Remaining Write: <REMAINING WRITE>
  // VR pending version format is "<DEVICE NAME> <VERSION>"
  // Replace pending version <DEVICE NAME> with active version <VENDOR NAME>
  regex pattern(R"((\S+)\s([^\s,]+)(?:,\s*Remaining Write:\s*(\d+))?)");
  smatch matches;
  string vendor = INVALID_VERSION;
  if (regex_search(activeVersion, matches, pattern)) {
    vendor = matches[1].str();
    activeVersion = matches[2].str();
    if (matches[3].matched) {
      j[RMNG_W] = matches[3].str();
    }
  }
  matches = smatch();
  if (regex_search(pendingVersion, matches, pattern) && !vendor.empty()) {
    pendingVersion = vendor + " " + matches[2].str();
    kv::set(pendingVersionKey, pendingVersion, kv::region::temp);
  }
  
  j[VERSION] = activeVersion;
  j[VENDOR] = vendor;

  return FW_STATUS_SUCCESS;
}

map<uint8_t, string> PldmVrComponent::get_vr_list() {
  map<uint8_t, string> vr_lsit;

  if (GETBIT(fby35_common_get_sb_rev(slot_id), javaisland::HSC_VR_VENDOR_BIT)) {
    vr_lsit = {
      {javaisland::VR_CPUDVDD, "vr_cpudvdd"},
      {javaisland::VR_FBVDDP2, "vr_fbvddp2"},
      {javaisland::VR_1V2, "vr_1v2"},
    };
  } else {
    vr_lsit = {
      {javaisland::VR_CPUDVDD, "vr_cpudvdd"},
      {javaisland::VR_CPUVDD, "vr_cpuvdd"},
      {javaisland::VR_SOCVDD, "vr_socvdd"},
    };
  }

  return vr_lsit;
}

int PldmVrComponent::print_version() {
  if (component_identifier == javaisland::ALL_VR) {
    for (const auto& [vr_comp_id, vr_name]: get_vr_list()) {
      // replace component name, component ID and version key 
      // to let get_version() get the version of a specific VR 
      component = vr_name;
      component_identifier = vr_comp_id;
      activeVersionKey = fmt::format("{}_{}_active_ver", fru, vr_name);
      pendingVersionKey = fmt::format("{}_{}_pending_ver", fru, vr_name);
      PldmComponent::print_version();
    }
    return FW_STATUS_SUCCESS;
  } else {
    return PldmComponent::print_version();
  }
}