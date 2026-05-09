#include <string>
#include "fw-util.h"
#include "bios.h"
#include <openbmc/pal.h>
#include <thread>
#include <facebook/netlakenext_common.h>
#include <syslog.h>
#include <openbmc/obmc-i2c.h>

using namespace std;

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
};

int BmcBiosComponent::setMeRecovery(uint8_t retry) {
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
  pal_power_button_override(fruid);
  std::this_thread::sleep_for(std::chrono::seconds(10));
  return pal_set_server_power(fruid, SERVER_POWER_ON);
}

BmcBiosComponent bios("server", "bios", "pnor", "/sys/bus/platform/drivers/aspeed-smc", "1e630000.spi", "SPI_MUX_SEL_R", true, "" /*TODO: Check image signature*/, "flashrom");
