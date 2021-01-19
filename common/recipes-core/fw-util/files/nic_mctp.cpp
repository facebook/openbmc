#include <string>
#include <unistd.h>
#include <syslog.h>
#include <openbmc/obmc-pal.h>
#include <openbmc/obmc-mctp.h>
#include "nic_mctp.h"

using namespace std;

static int get_nic_ver(uint8_t bus, uint8_t dst_eid)
{
  int ret = -1;
  uint8_t tag = 0;
  uint8_t iid = 0;
  uint16_t addr;
  uint8_t src_eid = 0x8;
  Get_Version_ID_Response rsp;
  struct obmc_mctp_binding *mctp_binding;

  pal_get_bmc_ipmb_slave_addr(&addr, bus);
  mctp_binding = obmc_mctp_smbus_init(bus, addr, src_eid);
  if (mctp_binding == NULL) {
    return -1;
  }

  if (obmc_mctp_clear_init_state(mctp_binding, dst_eid, tag, iid) < 0)
    goto bail;

  iid++;
  if (obmc_mctp_get_version_id(mctp_binding, dst_eid, tag, iid, &rsp) < 0)
    goto bail;

  if (rsp.IANA == MLX_MFG_ID) {
    cout << "MellanoxNIC firmware version: "
         << (int)rsp.fw_ver[0] << "." << (int)rsp.fw_ver[1] << "."
         << (int)((rsp.fw_ver[2] << 8) | rsp.fw_ver[3]) << endl;
  } else if (rsp.IANA == BCM_MFG_ID) {
    cout << "BroadcomNIC firmware version: "
         << string(rsp.fw_name) << endl;
  } else {
    cout << "NIC firmware version: NA (Unknown Manufacture ID: 0x"
         << hex << rsp.IANA  << ")" << endl;
  }
  ret = 0;
bail:
  obmc_mctp_smbus_free(mctp_binding);
  return ret;
}

int MCTPOverSMBusNicComponent::print_version()
{
  uint8_t prsnt;
  uint8_t fru_id;

  if (pal_get_fru_id((char *)this->component().c_str(), &fru_id) < 0)
    return -1;
  if (pal_is_fru_prsnt(fru_id, &prsnt) || !prsnt) {
    syslog(LOG_WARNING, "%s not present", this->component().c_str());
    return -1;
  }

  // TODO:
  // 	Usually, bus owner has to assign EID to each MCTP device before communication
  // 	But we use the EID assigned by NIC itself for now
  return get_nic_ver(_bus_id, _eid);
}
