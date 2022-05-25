#include <string>
#include "fw-util.h"
#include "bios.h"
#include <openbmc/pal.h>
#include <thread>

using namespace std;

class BmcBiosComponent : public BiosComponent {
  public:
    BmcBiosComponent(const std::string &fru, const std::string &comp, const std::string &mtd,
                     const std::string &devpath, const std::string &dev, const std::string &shadow,
                     bool level, const std::string &verp) :
      BiosComponent(fru, comp, mtd, devpath, dev, shadow, level, verp) {}
    int update(std::string image) override;
    int fupdate(std::string image) override;
    int unbindDevice();
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

int BmcBiosComponent::update(string image) {
  int res = 0;
  res = unbindDevice();
  if (res < 0) {
    return -1;
  }
  return BiosComponent::update(image, false);
}

int BmcBiosComponent::fupdate(string image) {
  int res = 0;
  res = unbindDevice();
  if (res < 0) {
    return -1;
  }
  return BiosComponent::update(image, true);
}

BmcBiosComponent bios("server", "bios", "pnor", "/sys/bus/platform/drivers/aspeed-smc", "1e630000.spi", "SPI_MUX_SEL_R", true, "" /*TODO: Check image signature*/);

