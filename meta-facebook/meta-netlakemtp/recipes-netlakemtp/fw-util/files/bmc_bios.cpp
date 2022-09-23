#include <string>
#include "fw-util.h"
#include "bios.h"
#include <openbmc/pal.h>
#include <thread>
#include <facebook/netlakemtp_common.h>

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
    int check_image(const char *path) override;
    void quit_ME_recovery();
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

void BmcBiosComponent::quit_ME_recovery() {
  int ret = 0;
  /*
    Byte 1:3 = Intel Manufacturer ID – 000157h, LS byte first.
    Byte 4 - Command = 01h Restart using Recovery Firmware.
             Command = 02h Restore Factory Default Variable values and restart the intel ME FW.
  */
  uint8_t tbuf[] = {0x57, 0x1, 0x0, 0x2};
  /*
    Byte 1 – Completion Code = 00h – Success
             Completion Code = 81h – Unsupported Command parameter value
    Byte 2:4 = Intel Manufacturer ID – 000157h, LS byte first.
  */
  uint8_t rlen = 4;
  uint8_t rbuf_quitMERecovery[rlen];

  ret = netlakemtp_common_me_ipmb_wrapper(NETFN_NM_REQ, CMD_NM_FORCE_ME_RECOVERY, tbuf, sizeof(tbuf), rbuf_quitMERecovery, &rlen);
  if (ret < 0) {
    cout << "Failed to switch ME from recovery to operational." << endl;
  }
}

int BmcBiosComponent::update(string image) {
  int res = 0;
  res = unbindDevice();
  if (res < 0) {
    return -1;
  }

  res = BiosComponent::update(image, false);
  quit_ME_recovery();
  return res;
}

int BmcBiosComponent::fupdate(string image) {
  int res = 0;
  res = unbindDevice();
  if (res < 0) {
    return -1;
  }

  res = BiosComponent::update(image, true);
  quit_ME_recovery();
  return res;
}

int BmcBiosComponent::check_image(const char *path){
  return 0;
}

BmcBiosComponent bios("server", "bios", "pnor", "/sys/bus/platform/drivers/aspeed-smc", "1e630000.spi", "SPI_MUX_SEL_R", true, "" /*TODO: Check image signature*/);

