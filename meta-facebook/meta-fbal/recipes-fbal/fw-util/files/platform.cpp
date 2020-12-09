#include <cstdio>
#include <cstring>
#include <unistd.h>
#include <openbmc/pal.h>
#include <openbmc/obmc-i2c.h>
#include <syslog.h>
#include "usbdbg.h"
#include "nic_ext.h"
#include "vr_fw.h"
#include "bios.h"

class palBiosComponent : public BiosComponent {
  public:
    palBiosComponent(std::string fru, std::string comp, std::string mtd, 
                     std::string devpath, std::string dev, std::string shadow,
                     bool level, std::string verp) :
      BiosComponent(fru, comp, mtd, devpath, dev, shadow, level, verp) {}
    int update_finish(void) override;
    int setDeepSleepWell(bool setting) override;
    int reboot(uint8_t fruid) override;
};

int palBiosComponent::update_finish(void) {
  int ret = 0;
  if(pal_get_config_is_master()){
    ret = pal_bios_update_ac();
  }
  return ret;
}

int palBiosComponent::setDeepSleepWell(bool setting) {
  int fd = 0, retCode = 0;
  uint8_t bus = 0x00;
  uint8_t addr = 0x27;
  uint8_t tbuf[16] = {0}, tlen = 2;
  uint8_t rbuf[16] = {0}, rlen = 0;
  bool setLow = true;
  
  fd = i2c_cdev_slave_open(bus, addr, I2C_SLAVE_FORCE_CLAIM);
  if (fd < 0) {
    syslog(LOG_WARNING, "%s() Failed to open %d", __func__, bus);
    i2c_cdev_slave_close(fd);
    return retCode;
  }
  
  if (setting == setLow) {
    // Set DeepSleepWell pin's direction to output and value to 0
    tbuf[0] = 0x07; tbuf[1] = 0xEF;
    retCode |= i2c_rdwr_msg_transfer(fd, addr << 1, tbuf, tlen, rbuf, rlen);
    tbuf[0] = 0x03; tbuf[1] = 0xEF;
    retCode |= i2c_rdwr_msg_transfer(fd, addr << 1, tbuf, tlen, rbuf, rlen);
  } else {
    // Reset DeepSleepWell, set DeepSleepWell pin's direction to input
    tbuf[0] = 0x07; tbuf[1] = 0xFF;
    retCode |= i2c_rdwr_msg_transfer(fd, addr << 1, tbuf, tlen, rbuf, rlen);
  }
  i2c_cdev_slave_close(fd);
  return retCode;
} 

int palBiosComponent::reboot(uint8_t fruid) {
  
  // Let mb will be ON after AC cycle
  if (sys.runcmd("/usr/bin/killall gpiod > /dev/null") != 0)
    syslog(LOG_DEBUG, "killall gpiod failed");
  if (sys.runcmd("/usr/local/bin/cfg-util pwr_server_last_state on > /dev/null") != 0)
    syslog(LOG_DEBUG, "Set pwr_server_last_state on failed");

  return 0;
}

NicExtComponent nic0("nic", "nic0", "nic0_fw_ver", FRU_NIC0, 0, 0x00);  // fru_name, component, kv, fru_id, eth_index, ch_id
NicExtComponent nic1("nic", "nic1", "nic1_fw_ver", FRU_NIC1, 1, 0x20);

UsbDbgComponent usbdbg("ocpdbg", "mcu", "F0C", 0, 0x60, false);
UsbDbgBlComponent usbdbgbl("ocpdbg", "mcubl", 0, 0x60, 0x02);  // target ID of bootloader = 0x02

VrComponent vr_pch_pvnn("vr", "pch_pvnn", "VR_PCH_PVNN/P1V05");
VrComponent vr_cpu0_vccin("vr", "cpu0_vccin", "VR_CPU0_VCCIN/VCCSA");
VrComponent vr_cpu0_vccio("vr", "cpu0_vccio", "VR_CPU0_VCCIO");
VrComponent vr_cpu0_vddq_abc("vr", "cpu0_vddq_abc", "VR_CPU0_VDDQ_ABC");
VrComponent vr_cpu0_vddq_def("vr", "cpu0_vddq_def", "VR_CPU0_VDDQ_DEF");
VrComponent vr_cpu1_vccin("vr", "cpu1_vccin", "VR_CPU1_VCCIN/VCCSA");
VrComponent vr_cpu1_vccio("vr", "cpu1_vccio", "VR_CPU1_VCCIO");
VrComponent vr_cpu1_vddq_abc("vr", "cpu1_vddq_abc", "VR_CPU1_VDDQ_ABC");
VrComponent vr_cpu1_vddq_def("vr", "cpu1_vddq_def", "VR_CPU1_VDDQ_DEF");

palBiosComponent bios("mb", "bios", "pnor", "/sys/bus/platform/drivers/aspeed-smc", "1e630000.spi", "FM_BIOS_SPI_BMC_CTRL", true, "(F0C_)(.*)");
