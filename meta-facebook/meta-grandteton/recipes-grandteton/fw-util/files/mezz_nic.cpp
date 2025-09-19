#include <string>
#include <syslog.h>
#include <libpldm-oem/pal_pldm.hpp>
#include <libpldm-oem/fw_update.hpp>
#include <openbmc/pal.h>
#include <openbmc/kv.hpp>
#include "mezz_nic.hpp"

using namespace std;

int PLDMNicComponent::get_version(json& j) {
  string ver{}, vendor{};
  string MLX_IANA = "33049";
  string BCM_IANA = "4413";

  try {
    if (fru() == "nic1") {
       vendor = kv::get("nic_vendor", kv::region::temp);
       if (vendor == MLX_IANA) {
         j["PRETTY_COMPONENT"] = "Mellanox " + _ver_key;
       }
       else if (vendor == BCM_IANA) {
         j["PRETTY_COMPONENT"] = "Broadcom " + _ver_key;
       }
       else {
         j["PRETTY_COMPONENT"] = _ver_key;
       }
    }
    else {
       vendor = kv::get("swb_nic_vendor", kv::region::temp);
       j["PRETTY_COMPONENT"] = kv::get("swb_nic_vendor", kv::region::temp) + " " + _ver_key;
    }
  } catch (std::exception& e) {
    j["PRETTY_COMPONENT"] = _ver_key;
  }

  try {
    if (fru() == "swb") {
      uint8_t lower_digit;
      std::string kv_path;

      lower_digit = _eid & 0xF;
      kv_path = std::string("swb_nic") + std::to_string(lower_digit) + "_present";
      if (kv::get(kv_path, kv::region::temp) == "0") {
        j["VERSION"] = get_pldm_active_ver(_bus_id, _eid, ver) ? "NA": ver;
      }
      else {
        j["VERSION"] = "Not present";
      }
    }
    else {
      j["VERSION"] = get_pldm_active_ver(_bus_id, _eid, ver) ? "NA": ver;
    }
  } catch (std::exception& e) {
    j["VERSION"] = "NA";
  }

  return FW_STATUS_SUCCESS;
}

int PLDMNicComponent::update(const string& image) {
  int ret;

  syslog(LOG_CRIT, "Component %s upgrade initiated", _component.c_str());

  //Since NIC PLDM update need to take more than 10 minutes, we extend the timeout.
  set_update_ongoing(60 * 20);
  ret = oem_pldm_fw_update(_bus_id, _eid, (char *)image.c_str(), false, _component.c_str(), 600);

  if (ret)
    syslog(LOG_CRIT, "Component %s upgrade fail", _component.c_str());
  else
    syslog(LOG_CRIT, "Component %s upgrade completed", _component.c_str());

  return ret;
}

PLDMNicComponent nic1("nic1", "nic1", "NIC", 0x22, 4);
