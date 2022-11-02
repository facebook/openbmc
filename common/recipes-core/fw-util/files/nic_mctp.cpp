#include <string>
#include <unistd.h>
#include <syslog.h>
#include <sstream>
#include <openbmc/obmc-pal.h>
#include <openbmc/obmc-mctp.h>
#include "nic_mctp.h"

using namespace std;

#define DEFAULT_EID 0x8

int MCTPOverSMBusNicComponent::get_version(json& j)
{
  // TODO:
  // 	Usually, bus owner has to assign EID to each MCTP device before communication
  // 	But we use the EID assigned by NIC itself for now

  int ret = -1;
  uint8_t tag = 0;
  uint8_t iid = 0;
  uint16_t addr;
  uint8_t src_eid = DEFAULT_EID;
  Get_Version_ID_Response rsp;
  struct obmc_mctp_binding *mctp_binding;

  pal_get_bmc_ipmb_slave_addr(&addr, _bus_id);
  mctp_binding = obmc_mctp_smbus_init(_bus_id, addr, NIC_SLAVE_ADDR, src_eid, NCSI_MAX_PAYLOAD);
  if (mctp_binding == NULL) {
    return -1;
  }

  if (obmc_mctp_clear_init_state(mctp_binding, _eid, tag, iid) < 0)
    goto bail;

  iid++;
  if (obmc_mctp_get_version_id(mctp_binding, _eid, tag, iid, &rsp) < 0)
    goto bail;

  if (rsp.IANA == MLX_MFG_ID) {
    j["PRETTY_COMPONENT"] = "MellanoxNIC firmware";
    j["vendor"] = "Mellanox";
    stringstream ss;
    ss << (int)rsp.fw_ver[0] << "." << (int)rsp.fw_ver[1] << "."
         << (int)((rsp.fw_ver[2] << 8) | rsp.fw_ver[3]);
    j["VERSION"] = ss.str();
  } else if (rsp.IANA == BCM_MFG_ID) {
    j["PRETTY_COMPONENT"] = "BroadcomNIC firmware";
    j["VERSION"] = rsp.fw_name;
  } else {
    j["PRETTY_COMPONENT"] = "NIC firmware";
    stringstream ss;
    ss << "NA (Unknown Manufacture ID: 0x"
         << hex << rsp.IANA  << ")" << endl;
    j["VERSION"] = ss.str();
  }
  ret = 0;
bail:
  obmc_mctp_smbus_free(mctp_binding);
  return ret;
}

int MCTPOverSMBusNicComponent::update(string image)
{
  int ret = -1;
  uint8_t tag = 0;
  uint16_t addr;
  uint8_t src_eid = DEFAULT_EID;
  struct obmc_mctp_binding *mctp_binding;

  pal_get_bmc_ipmb_slave_addr(&addr, _bus_id);
  mctp_binding = obmc_mctp_smbus_init(_bus_id, addr, NIC_SLAVE_ADDR, src_eid, -1); // Default size
  if (mctp_binding == NULL) {
    return -1;
  }

  ret = obmc_mctp_fw_update(mctp_binding, _eid, tag, (char *)image.c_str());

  obmc_mctp_smbus_free(mctp_binding);
  return ret;
}
