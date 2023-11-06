#include <string>
#include <syslog.h>
#include <libpldm-oem/pal_pldm.hpp>
#include <libpldm-oem/fw_update.hpp>
#include <openbmc/pal.h>
#include "mezz_nic.hpp"

using namespace std;

int PLDMNicComponent::get_version(json& j) {

  string ver{};
  j["PRETTY_COMPONENT"] = "Mellanox " + _ver_key;
  j["VERSION"] = get_pldm_active_ver(_bus_id, _eid, ver) ? "NA": ver;

  return FW_STATUS_SUCCESS;
}

int PLDMNicComponent::update(string image) {
  int ret;

  syslog(LOG_CRIT, "Component %s upgrade initiated", _component.c_str());

  //Since NIC PLDM update need to take more than 10 minutes, we extend the timeout.
  pal_set_fw_update_ongoing(FRU_NIC1, 60 * 20);
  ret = oem_pldm_fw_update(_bus_id, _eid, (char *)image.c_str());

  if (ret)
    syslog(LOG_CRIT, "Component %s upgrade fail", _component.c_str());
  else
    syslog(LOG_CRIT, "Component %s upgrade completed", _component.c_str());

  return ret;
}

PLDMNicComponent nic1("nic1", "nic1", "NIC", 0x22, 4);
