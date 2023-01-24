#include <cstdio>
#include <cstring>
#include <fstream>
#include <unistd.h>
#include <chrono>
#include <thread>
#include <openbmc/pal.h>
#include <openbmc/obmc-i2c.h>
#include <openbmc/vr.h>
#include <libpldm/base.h>
#include <libpldm-oem/pldm.h>
#include <syslog.h>
#include "usbdbg.h"
#include "nic_ext.h"
#include "bios.h"
#include "vr_fw.h"

using namespace std;

//SWB VR Component
class SwbVrComponent : public VrComponent {
    std::string name;
    int update_proc(string image, bool force);
  public:
    SwbVrComponent(string fru, string comp, string dev_name)
        :VrComponent(fru, comp, dev_name), name(dev_name) {}
    int fupdate(string image);
    int update(string image);

};

static
int set_swb_snr_polling (uint8_t status) {
  uint8_t tbuf[255] = {0};
  uint8_t rbuf[255] = {0};
  uint8_t tlen=0;
  size_t rlen = 0;
  int rc;

  tbuf[tlen++] = status;

  rc = pldm_oem_ipmi_send_recv(SWB_BUS_ID, SWB_BIC_EID,
                               NETFN_OEM_1S_REQ,
                               CMD_OEM_1S_DISABLE_SEN_MON,
                               tbuf, tlen,
                               rbuf, &rlen);
  printf("%s rc=%d", __func__, rc);
  return rc;
}


int SwbVrComponent::update_proc(string image, bool force) {
  int ret;
  string comp = this->component();

  if (vr_probe() < 0) {
    cout << "VR probe failed!" << endl;
    return -1;
  }

  syslog(LOG_CRIT, "Component %s upgrade initiated", comp.c_str());
  ret = vr_fw_update(name.c_str(), (char *)image.c_str(), force);
  if (ret < 0) {
    cout << "ERROR: VR Firmware update failed!" << endl;
  } else {
    syslog(LOG_CRIT, "Component %s %s completed", comp.c_str(), force? "force upgrade": "upgrade");
  }

  vr_remove();
  return ret;
}
 

int SwbVrComponent::update(string image) {
  int ret;

  ret = set_swb_snr_polling(0x00);
  if (ret)
   return ret;

  ret = update_proc(image, 0);

  if(set_swb_snr_polling(0x01))
    printf("set snr polling start fail\n");

  return ret;
}

int SwbVrComponent::fupdate(string image) {
  int ret;

  ret = set_swb_snr_polling(0x00);
  if (ret)
   return ret;

  ret = update_proc(image, 1);

  if(set_swb_snr_polling(0x01))
    printf("set snr polling start fail\n");

  return ret;
}

class fw_common_config {
  public:
    fw_common_config() {
      static NicExtComponent nic0("nic0", "nic0", "nic0_fw_ver", FRU_NIC0, 0);
      if (pal_is_artemis()) {
        static SwbVrComponent vr_pesw_vcc("acb", "pesw_vr", "VR_PESW_VCC");
      } else {
        static UsbDbgComponent usbdbg("ocpdbg", "mcu", "F0T", 14, 0x60, false);
        static UsbDbgBlComponent usbdbgbl("ocpdbg", "mcubl", 14, 0x60, 0x02);  // target ID of bootloader = 0x02
        static SwbVrComponent vr_pex0_vcc("swb", "pex01_vcc", "VR_PEX01_VCC");
        static SwbVrComponent vr_pex1_vcc("swb", "pex23_vcc", "VR_PEX23_VCC");
      }
    }
};

fw_common_config _fw_common_config;
