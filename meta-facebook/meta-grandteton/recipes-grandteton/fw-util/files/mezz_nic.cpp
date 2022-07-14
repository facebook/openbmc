#include <string>
#include <unistd.h>
#include <syslog.h>
#include <libpldm/base.h>
#include <libpldm/firmware_update.h>
#include <libpldm-oem/pldm.h>
#include <libpldm-oem/fw_update.h>
#include "mezz_nic.hpp"

using namespace std;

int PLDMNicComponent::print_version() {

  uint8_t tbuf[255] = {0};
  auto pldmbuf = (pldm_msg *)tbuf;
  pldmbuf->hdr.request = 1;
  pldmbuf->hdr.type    = PLDM_FWUP;
  pldmbuf->hdr.command = PLDM_GET_FIRMWARE_PARAMETERS;

  uint8_t *rbuf = nullptr;
  size_t rlen = 0;
  size_t tlen = PLDM_HEADER_SIZE;
  int rc = oem_pldm_send_recv(_bus_id, _eid, tbuf, tlen, &rbuf, &rlen);

  std::string ret = "N/A";
  if (rc == 0) {
    ret = "";
    for (int i = 0; i < 10; ++i)
      ret += (char)*(rbuf + i + 14);
  }
  printf("Mellanox %s firmware version : %s\n", _ver_key.c_str(), ret.c_str());

  return rc;
}

int PLDMNicComponent::update(string image) {
  return obmc_pldm_fw_update(_bus_id, _eid, (char *)image.c_str());
}

PLDMNicComponent nic1("nic1", "nic1", "NIC1", 0x0, 4);
