#include <string>
#include "fw-util.h"
#include "bios.h"
#include <openbmc/pal.h>
#include <thread>
#include <facebook/netlakemtp_common.h>
#include <syslog.h>
#include <openbmc/obmc-i2c.h>

using namespace std;

#define ME_RECOVERY 0x1
#define ME_OPERATIONAL 0x2

#define SPI_MUX_SEL_SOC 0x0
#define SPI_MUX_SEL_BMC 0x1

class BmcBiosComponent : public BiosComponent {
  public:
    BmcBiosComponent(const std::string &fru, const std::string &comp, const std::string &mtd,
                     const std::string &devpath, const std::string &dev, const std::string &shadow,
                     bool level, const std::string &verp) :
      BiosComponent(fru, comp, mtd, devpath, dev, shadow, level, verp) {}
    int update(std::string image) override;
    int fupdate(std::string image) override;
    int unbindDevice();
    int check_image(const char *path) override;
    void set_ME_Mode(uint8_t mode);
    int reboot(uint8_t fruid) override;
    int setMeRecovery(uint8_t retry) override;
    int getSelftestResult();
    void spiMuxSelCPLD(uint8_t spiOwner);
};

int BmcBiosComponent::unbindDevice() {
  std::ofstream ofs;
  ofs.open(spipath + "/unbind");
  if (!ofs.is_open()) {
    sys().error << "ERROR: Cannot unbind " << spidev << std::endl;
    return -1;
  }
  ofs << spidev;
  ofs.close();
  return 0;
}

void BmcBiosComponent::set_ME_Mode(uint8_t mode) {
  int ret = 0;
  /*
    Byte 1:3 = Intel Manufacturer ID – 000157h, LS byte first.
    Byte 4 - Command = 01h Restart using Recovery Firmware.
             Command = 02h Restore Factory Default Variable values and restart the intel ME FW.
  */
  uint8_t tbuf[] = {0x57, 0x1, 0x0, mode};
  /*
    Byte 1 – Completion Code = 00h – Success
             Completion Code = 81h – Unsupported Command parameter value
    Byte 2:4 = Intel Manufacturer ID – 000157h, LS byte first.
  */
  uint8_t rlen = 4;
  uint8_t rbuf_quitMERecovery[rlen];

  ret = netlakemtp_common_me_ipmb_wrapper(NETFN_NM_REQ, CMD_NM_FORCE_ME_RECOVERY, tbuf, sizeof(tbuf), rbuf_quitMERecovery, &rlen);
  if (ret < 0) {
    syslog(LOG_WARNING, "Failed to switch ME status\n");
  }
}

int BmcBiosComponent::update(string image) {
  int res = 0;
  res = unbindDevice();
  if (res < 0) {
    return -1;
  }

  res = BiosComponent::update(image, false);
  return res;
}

int BmcBiosComponent::fupdate(string image) {
  int res = 0;
  res = unbindDevice();
  if (res < 0) {
    return -1;
  }

  res = BiosComponent::update(image, true);
  return res;
}

int BmcBiosComponent::check_image(const char *path){
  return 0;
}

int BmcBiosComponent::reboot(uint8_t fruid) {
  spiMuxSelCPLD(SPI_MUX_SEL_SOC);
  set_ME_Mode(ME_OPERATIONAL);
  std::this_thread::sleep_for(std::chrono::seconds(1));
  pal_power_button_override(fruid);
  std::this_thread::sleep_for(std::chrono::seconds(10));
  return pal_set_server_power(fruid, SERVER_POWER_ON);
}

int BmcBiosComponent::getSelftestResult() {
  int ret = 0;
    /*
      0x6 0x4: Get Self-Test Results
    Byte 1 - Completion Code
    Byte 2
      = 55h - No error. All Self-Tests Passed.
      = 81h - Firmware entered Recovery bootloader mode
    Byte 3 For byte 2 = 55h, 56h, FFh:
      =00h
      =02h - recovery mode entered by IPMI command "Force ME Recovery"
  */
  uint8_t rlen = 2;
  uint8_t rbuf_getSelftestResult[rlen];

  ret = netlakemtp_common_me_ipmb_wrapper(NETFN_APP_REQ, CMD_APP_GET_SELFTEST_RESULTS, NULL, 0, rbuf_getSelftestResult, &rlen);
  if (ret < 0) {
    syslog(LOG_INFO, "Getting selftest result is not ready, ret=%d\n", ret);
  } else if (rbuf_getSelftestResult[0] == 0x81 && rbuf_getSelftestResult[1] == 0x2) {
    cout << "Selftest result: ME recovery entered" << endl;
    return 0;
  }

  return -1;
}

int BmcBiosComponent::setMeRecovery(uint8_t retry) {
  int ret = -1;
  while (retry > 0) {
    set_ME_Mode(ME_RECOVERY);
    std::this_thread::sleep_for(std::chrono::seconds(10));
    ret = getSelftestResult();

    if (ret == 0) {
      break;
    }
    retry--;
  }

  if (retry == 0) {
      sys().error << "ERROR: unable to put ME in recovery mode!" << endl;
      syslog(LOG_ERR, "Unable to put ME in recovery mode!\n");
      return -1;
  }
  spiMuxSelCPLD(SPI_MUX_SEL_BMC);

  return 0;
}

void BmcBiosComponent::spiMuxSelCPLD(uint8_t spiOwner) {
  int fd = 0, ret = -1;
  uint8_t bus = CPLD_SPI_MUX_BUS;
  uint8_t addr = CPLD_SPI_MUX_ADDR;
  uint8_t tbufGetSPI = CPLD_SPI_MUX_REG;
  uint8_t tlenGetSPI = sizeof(tbufGetSPI);
  uint8_t rbuf;
  uint8_t rlen = sizeof(rbuf);

  fd = i2c_cdev_slave_open(bus, addr >> 1, I2C_SLAVE_FORCE_CLAIM);
  if (fd < 0) {
    syslog(LOG_WARNING, "%s() Failed to open I2C bus %d\n", __func__, bus);
    return;
  }

  ret = i2c_rdwr_msg_transfer(fd, addr, &tbufGetSPI, tlenGetSPI, &rbuf, rlen);
  if (ret < 0) {
    syslog(LOG_WARNING, "%s() Failed to get SPI MUX status from CPLD\n", __func__);
  }

  // Set bit0 for SPI_CS0_MUX_SEL2
  spiOwner += rbuf & 0xFE;
  uint8_t tbufSetSPI[] = {CPLD_SPI_MUX_REG, spiOwner};
  uint8_t tlenSetSPI = sizeof(tbufSetSPI);
  ret = i2c_rdwr_msg_transfer(fd, addr, tbufSetSPI, tlenSetSPI, NULL, 0);
  if (ret < 0) {
    syslog(LOG_WARNING, "%s() Failed to set SPI MUX status from CPLD\n", __func__);
  }
  close(fd);

  return;
}

BmcBiosComponent bios("server", "bios", "pnor", "/sys/bus/platform/drivers/aspeed-smc", "1e630000.spi", "SPI_MUX_SEL_R", true, "" /*TODO: Check image signature*/);

