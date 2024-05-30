#include "bic_pldm_vr.hpp"

#include <openbmc/pal.h>
#include <openbmc/kv.hpp>
#include <facebook/bic.h>
#include <facebook/bic_ipmi.h>
#include <facebook/fby35_common.h>

#include <fmt/format.h>
#include <iostream>
#include <regex>

using namespace std;

int PldmVrComponent::update_internal(string image, bool force) {
  uint8_t slot_id = 0;
  uint8_t status;
  int rc;

  try {
    if (fby35_common_get_slot_id(const_cast<char*>(fru.c_str()), &slot_id)) {
      throw runtime_error(fru + " is invalid!");
    }

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
  if (PldmComponent::get_version(j)) {
    return FW_STATUS_FAILURE;
  }
  auto activeVersion = j[ACTIVE_VERSION].get<string>();
  auto pendingVersion = j[PENDING_VERSION].get<string>();

  // When the BIC fails to access this component, it returns "ERROR:%d."
  if (activeVersion.find("ERROR:") != string::npos || 
      pendingVersion.find("ERROR:") != string::npos) {
    throw std::runtime_error("BIC cannot access the component");
  }

  // If the active version is valid but the pending version is invalid, let 
  // the pending version equal the active version
  if (activeVersion.find(INVALID_VERSION) == string::npos && 
      pendingVersion.find(INVALID_VERSION) != string::npos) {
    pendingVersion = activeVersion;
    kv::set(pendingVersionKey, pendingVersion, kv::region::temp);
  }

  // VR active version format is "<DEVICE NAME> <VERSION>"
  // VR pending version format is "<DEVICE NAME> <VERSION>_<COMPONENT>"
  // Only extract the <VERSION> part
  regex pattern(R"(\s([^_]+)(?:_.*)?)");
  smatch matches;
  if (regex_search(activeVersion, matches, pattern)) {
    activeVersion = matches[1];
  }
  matches = smatch();
  if (regex_search(pendingVersion, matches, pattern)) {
    pendingVersion = matches[1];
  }
  
  j[ACTIVE_VERSION] = activeVersion;
  j[PENDING_VERSION] = pendingVersion;

  return FW_STATUS_SUCCESS;
}