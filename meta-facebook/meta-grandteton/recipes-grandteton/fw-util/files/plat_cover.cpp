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
#include "vr_fw.h"
#include "bios.h"

using namespace std;

class palBiosComponent : public BiosComponent {
  public:
    palBiosComponent(const string &fru, const string &comp, const string &mtd,
                     const string &devpath, const string &dev, const string &shadow,
                     bool level, const string &verp) :
      BiosComponent(fru, comp, mtd, devpath, dev, shadow, level, verp) {}
    int update_finish(void) override;
    int reboot(uint8_t fruid) override;
};

int palBiosComponent::update_finish(void) {
  sys().runcmd(string("/sbin/fw_setenv por_ls on"));
  sys().output << "To complete the upgrade, please perform 'power-util sled-cycle'" << endl;
  return 0;
}

int palBiosComponent::reboot(uint8_t fruid) {
  return 0;
}


//VR Component
class SwbVrComponent : public VrComponent {
    std::string name;
    int update_proc(string image, bool force);
  public:
    SwbVrComponent(string fru, string comp, string dev_name)
        :VrComponent(fru, comp, dev_name), name(dev_name) {}
    int fupdate(string image);
    int update(string image);

};

//#define SWB_VR_BUS_ID  (3)
//#define SWB_BIC_EID (0x0a)

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


palBiosComponent bios("mb", "bios", "pnor", "/sys/bus/platform/drivers/aspeed-smc",
                      "1e631000.spi", "FM_BMC_MUX_CS_SPI_SEL_0", true, "(F0T_)(.*)");
VrComponent vr_cpu0_vccin("mb", "cpu0_vccin", "VR_CPU0_VCCIN/VCCFA_FIVRA");
VrComponent vr_cpu0_faon("mb", "cpu0_faon", "VR_CPU0_VCCFAEHV/FAON");
VrComponent vr_cpu0_vccd("mb", "cpu0_vccd", "VR_CPU0_VCCD");
VrComponent vr_cpu1_vccin("mb", "cpu1_vccin", "VR_CPU1_VCCIN/VCCFA_FIVRA");
VrComponent vr_cpu1_faon("mb", "cpu1_faon", "VR_CPU1_VCCFAEHV/FAON");
VrComponent vr_cpu1_vccd("mb", "cpu1_vccd", "VR_CPU1_VCCD");
SwbVrComponent vr_pex0_vcc("swb", "pex0_vcc", "VR_PEX0_VCC");
SwbVrComponent vr_pex1_vcc("swb", "pex1_vcc", "VR_PEX1_VCC");
