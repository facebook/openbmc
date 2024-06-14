#include "bic_pldm_vr.hpp"

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

  if (PldmComponent::get_version(j)) {
    return FW_STATUS_FAILURE;
  }
  auto activeVersion = j[ACTIVE_VERSION].get<string>();
  auto pendingVersion = j[PENDING_VERSION].get<string>();
  
  // If the active version is valid but the pending version is invalid, let 
  // the pending version equal the active version
  if (activeVersion.find(INVALID_VERSION) == string::npos && 
      pendingVersion.find(INVALID_VERSION) != string::npos) {
    pendingVersion = activeVersion;
    kv::set(pendingVersionKey, pendingVersion, kv::region::temp);
    j[PENDING_VERSION] = pendingVersion;
    return FW_STATUS_SUCCESS;
  }

  // VR active version format is "<VENDOR NAME> <VERSION>"
  // VR pending version format is "<DEVICE NAME> <VERSION>_<COMPONENT>"
  // Replace pending version <DEVICE NAME> with active version <VENDOR NAME>
  regex pattern(R"((\S+)\s([^_]+)(?:_.*)?)");
  smatch matches;
  string vendor;
  if (regex_search(activeVersion, matches, pattern)) {
    vendor = matches[1].str();
  }
  matches = smatch();
  if (regex_search(pendingVersion, matches, pattern)) {
    pendingVersion = vendor + " " + matches[2].str();
  }
  
  j[ACTIVE_VERSION] = activeVersion;
  j[PENDING_VERSION] = pendingVersion;

  return FW_STATUS_SUCCESS;
}