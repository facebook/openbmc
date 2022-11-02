#include <string>
#include <unistd.h>
#include <syslog.h>
#include <libpldm/base.h>
#include <libpldm/firmware_update.h>
#include <libpldm-oem/pldm.h>
#include <libpldm-oem/fw_update.h>
#include "mezz_nic.hpp"
#include "fw-util.h"

using namespace std;

int PLDMNicComponent::get_version(json& j) {

  uint8_t tbuf[255] = {0};
  auto pldmbuf = (pldm_msg *)tbuf;
  pldmbuf->hdr.request = 1;
  pldmbuf->hdr.type    = PLDM_FWUP;
  pldmbuf->hdr.command = PLDM_GET_FIRMWARE_PARAMETERS;

  uint8_t *rbuf = nullptr;
  size_t rlen = 0;
  size_t tlen = PLDM_HEADER_SIZE;
  j["PRETTY_COMPONENT"] = "Mellanox " + _ver_key;

  int rc = oem_pldm_send_recv(_bus_id, _eid, tbuf, tlen, &rbuf, &rlen);
  if (rc == 0) {
    std::string ret{};
    for (int i = 0; i < 10; ++i)
      ret += (char)*(rbuf + i + 14);
    j["VERSION"] = ret;
  } else {
    j["VERSION"] = "NA";
  }
  return FW_STATUS_SUCCESS;
}

int PLDMNicComponent::update(string image) {
  int ret;

  syslog(LOG_CRIT, "Component %s upgrade initiated", _component.c_str());

  ret = obmc_pldm_fw_update(_bus_id, _eid, (char *)image.c_str());

  if (ret)
    syslog(LOG_CRIT, "Component %s upgrade fail", _component.c_str());
  else
    syslog(LOG_CRIT, "Component %s upgrade completed", _component.c_str());

  return ret;
}

PLDMNicComponent nic1("nic1", "nic1", "NIC", 0x0, 4);
