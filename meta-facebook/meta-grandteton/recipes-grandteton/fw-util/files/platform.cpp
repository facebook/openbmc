#include <cstdio>
#include <cstring>
#include <fstream>
#include <unistd.h>
#include <chrono>
#include <thread>
#include <openbmc/pal.h>
#include <openbmc/obmc-i2c.h>
#include <syslog.h>
#include "usbdbg.h"
#include "nic_ext.h"
#include "bios.h"
//#include "vr_fw.h"
//#include "bic_fwupdate.h"

class palBiosComponent : public BiosComponent {
  public:
    palBiosComponent(const std::string &fru, const std::string &comp, const std::string &mtd,
                     const std::string &devpath, const std::string &dev, const std::string &shadow,
                     bool level, const std::string &verp) :
      BiosComponent(fru, comp, mtd, devpath, dev, shadow, level, verp) {}
    int update_finish(void) override;
    int reboot(uint8_t fruid) override;
};

int palBiosComponent::update_finish(void) {
  sys().runcmd(std::string("/sbin/fw_setenv por_ls on"));
  sys().output << "To complete the upgrade, please perform 'power-util sled-cycle'" << std::endl;
  return 0;
}

int palBiosComponent::reboot(uint8_t fruid) {
  return 0;
}

NicExtComponent nic0("nic", "nic0", "nic0_fw_ver", FRU_NIC0, 0, 0x00);  // fru_name, component, kv, fru_id, eth_index, ch_id

UsbDbgComponent usbdbg("ocpdbg", "mcu", "F0T", 14, 0x60, false);
UsbDbgBlComponent usbdbgbl("ocpdbg", "mcubl", 14, 0x60, 0x02);  // target ID of bootloader = 0x02

palBiosComponent bios("mb", "bios", "pnor", "/sys/bus/platform/drivers/aspeed-smc", 
                      "1e631000.spi", "FM_BMC_MUX_CS_SPI_SEL_0", true, "(F0T_)(.*)");

//VrComponent vr_cpu0_vccin("vr", "cpu0_vccin", "VR_CPU0_VCCIN/VCCFA_FIVRA");
//VrComponent vr_cpu0_faon("vr", "cpu0_faon", "VR_CPU0_VCCFAEHV/FAON");
//VrComponent vr_cpu0_vccd("vr", "cpu0_vccd", "VR_CPU0_VCCD");
//VrComponent vr_cpu1_vccin("vr", "cpu1_vccin", "VR_CPU1_VCCIN/VCCFA_FIRVA");
//VrComponent vr_cpu1_faon("vr", "cpu1_faon", "VR_CPU1_VCCFAEHV/FAON");
//VrComponent vr_cpu1_vccd("vr", "cpu1_vccd", "VR_CPU1_VCCD");
