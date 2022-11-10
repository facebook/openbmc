#include <cstdio>
#include <cstring>
#include <fstream>
#include <unistd.h>
#include <chrono>
#include <thread>
#include <openbmc/pal.h>
#include <openbmc/obmc-i2c.h>
#include <openbmc/vr.h>
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

int palBiosComponent::reboot(uint8_t) {
  return 0;
}




palBiosComponent bios("mb", "bios", "pnor", "/sys/bus/platform/drivers/aspeed-smc",
                      "1e631000.spi", "FM_BMC_MUX_CS_SPI_SEL_0", true, "(F0T_)(.*)");
VrComponent vr_cpu0_vccin("mb", "cpu0_vccin", "VR_CPU0_VCCIN/VCCFA_FIVRA");
VrComponent vr_cpu0_faon("mb", "cpu0_faon", "VR_CPU0_VCCFAEHV/FAON");
VrComponent vr_cpu0_vccd("mb", "cpu0_vccd", "VR_CPU0_VCCD");
VrComponent vr_cpu1_vccin("mb", "cpu1_vccin", "VR_CPU1_VCCIN/VCCFA_FIVRA");
VrComponent vr_cpu1_faon("mb", "cpu1_faon", "VR_CPU1_VCCFAEHV/FAON");
VrComponent vr_cpu1_vccd("mb", "cpu1_vccd", "VR_CPU1_VCCD");

