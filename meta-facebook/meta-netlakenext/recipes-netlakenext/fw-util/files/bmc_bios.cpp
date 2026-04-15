#include <string>
#include "fw-util.h"
#include "bios.h"
#include <openbmc/pal.h>
#include <thread>
#include <facebook/netlakenext_common.h>
#include <syslog.h>
#include <openbmc/obmc-i2c.h>

using namespace std;

#define SPI_MUX_SEL_SOC 0x0
#define SPI_MUX_SEL_BMC 0x1

class BmcBiosComponent : public BiosComponent {
  public:
    BmcBiosComponent(const std::string &fru, const std::string &comp, const std::string &mtd,
                     const std::string &devpath, const std::string &dev, const std::string &shadow,
                     bool level, const std::string &verp , const std::string &method) :
      BiosComponent(fru, comp, mtd, devpath, dev, shadow, level, verp , method) {}
    int update(const std::string& image) override;
    int fupdate(const std::string& image) override;
    int unbindDevice();
    int check_image(const char *path) override;
    int reboot(uint8_t fruid) override;
    int setMeRecovery(uint8_t retry) override;
    void spiMuxSelCPLD(uint8_t spiOwner);
};

int BmcBiosComponent::setMeRecovery(uint8_t retry) {
  spiMuxSelCPLD(SPI_MUX_SEL_BMC);

  return 0;
}

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

int BmcBiosComponent::update(const string& image) {
  int res = 0;
  res = unbindDevice();
  if (res < 0) {
    return -1;
  }

  res = BiosComponent::update(image, false);
  return res;
}

int BmcBiosComponent::fupdate(const string& image) {
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
  std::this_thread::sleep_for(std::chrono::seconds(1));
  pal_power_button_override(fruid);
  std::this_thread::sleep_for(std::chrono::seconds(10));
  return pal_set_server_power(fruid, SERVER_POWER_ON);
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

BmcBiosComponent bios("server", "bios", "pnor", "/sys/bus/platform/drivers/aspeed-smc", "1e630000.spi", "SPI_MUX_SEL_R", true, "" /*TODO: Check image signature*/, "flashrom");
