#include "bic_pldm_retimer.hpp"

#include <openbmc/kv.hpp>
#include <openbmc/pal.h>
#include <openbmc/ipmi.h>
#include <facebook/bic_xfer.h>

#include <syslog.h>
#include <iostream>
#include <regex>

using namespace std;

static bool is_server_post_complete(const uint8_t& slot_id) {
  char host_ready_value[8] = {0};
  auto host_ready_key = fmt::format("fru{}_host_ready", slot_id);

  if (kv_get(host_ready_key.c_str(), host_ready_value, NULL, 0)) {
    return false;
  }
  return strcmp(host_ready_value, "1") == 0;
}

int get_server_retimer_type(const uint8_t& slot_id) {
  int retimer_type = RETIMER_UNKNOWN;
  string retimer_type_key(fmt::format("slot{}_retimer_type", slot_id));
  
  try {
    retimer_type = stoi(kv::get(retimer_type_key, kv::region::temp));
    // if the retimer type is unknown in the cache, rescan the retimer bus
    if (retimer_type == RETIMER_UNKNOWN) {
      throw runtime_error("the retimer type is unknown");
    }
  } catch(...) {
    try {
      int ret;
      uint8_t tbuf[16] = {0x00};
      uint8_t rbuf[16] = {0x00};
      uint8_t tlen = IANA_ID_SIZE;
      uint8_t rlen = 0;

      if (!is_server_post_complete(slot_id)) {
        throw runtime_error("the server has not POST complete");
      }
      
      ret = system("sv stop sensord > /dev/null 2>&1 &");
      if (ret < 0) {
        throw runtime_error("failed to stop sensord to get the retimer type");
      }

      // the switch needs to switch to channel 2 after EVT2 to access the retimer
      ret = fby35_common_get_sb_rev(slot_id);
      if (ret < 0) {
        throw runtime_error("failed to get server board revision");
      }
      if ((ret & 0x0F) >= JI_REV_EVT2) {
        tbuf[0] = 0x02; // channel 2
        ret = bic_master_write_read(slot_id, (RETIMER_SWITCH_BUS << 1) | 1, 
                                    RETIMER_SWITCH_ADDR, tbuf, 1, rbuf, 0);
        if (ret) {
          throw runtime_error("failed to switch channel for retimer");
        }
      }
      
      // scan the i2c bus to determine the retimer type
      memcpy(tbuf, (uint8_t *)&IANA_ID, IANA_ID_SIZE);
      tbuf[tlen++] = RETIMER_SWITCH_BUS; 
      ret = bic_data_send(slot_id, NETFN_OEM_1S_REQ, 0x60, 
                          tbuf, tlen, rbuf, &rlen, NONE_INTF);
      if (ret) {
        throw runtime_error("failed to scan retimer bus");
      }

      for (size_t i = 0; i < rlen; ++i) {
        if (rbuf[i] == AL_RETIMER_ADDR) {
          retimer_type = RETIMER_AL_PT4080L;
          break;
        }
        if (rbuf[i] == TI_RETIMER_ADDR) {
          retimer_type = RETIMER_TI_DS160PT801;
          break;
        }
      }
    } catch(const std::exception& e) {
      syslog(LOG_ERR, "%s() Slot%d, cannot get the retimer type, %s", 
             __func__, slot_id, e.what());
    }

    kv::set(retimer_type_key, to_string(retimer_type), kv::region::temp);

    if (system("sv start sensord > /dev/null 2>&1 &") < 0) {
      syslog(LOG_ERR, "%s(): Failed to start sensord", __func__);
    }
  }

  return retimer_type;
}

int PldmRetimerComponent::update_version_cache() {
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

int PldmRetimerComponent::update_internal(string image, bool force) {
  try {
    server.ready();
    uint8_t status;
    if (pal_get_server_power(slot_id, &status)) {
      throw string("Failed to get server power");
    }
    if (status != SERVER_POWER_ON) {
      throw string("The server power is not turned on");
    }
    if (!is_server_post_complete(slot_id)) {
      throw string("The server must POST complete before updating the retimer");
    }
  } catch (const string& e) {
    cerr << e << endl;
    return FW_STATUS_FAILURE;
  }

  return PldmComponent::update_internal(image, force);
}

int PldmRetimerComponent::get_version(json& j) {
  try {
    server.ready();
  } catch(const string& e) {
    throw runtime_error(e);
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

  // Retimer pending version format is "<DEVICE NAME> <VERSION>_<COMPONENT>"
  // Need to remove <DEVICE NAME> string from pending version
  regex pattern(R"((\S+)\s([^_]+)(?:_.*)?)");
  smatch matches;
  if (regex_search(pendingVersion, matches, pattern)) {
    pendingVersion = matches[2].str();
    kv::set(pendingVersionKey, pendingVersion, kv::region::temp);
  }
  
  j[VERSION] = activeVersion;

  return FW_STATUS_SUCCESS;
}