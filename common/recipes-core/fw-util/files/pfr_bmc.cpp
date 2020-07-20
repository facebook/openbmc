#include <cstdio>
#include <cstring>
#include <syslog.h>
#include <openbmc/obmc-i2c.h>
#include <openbmc/pal.h>
#include "pfr_bmc.h"

using namespace std;

int PfrBmcComponent::update(string image) {
  int ret, ifd, retry = 3;
  uint8_t fruid = 1, bus, addr, buf[8];
  char dev_i2c[16];
  bool bridged;
  string dev, cmd;

  if (is_valid(image, true) == false) {
    sys.error << image << " is not a valid BMC image for " << sys.name() << endl;
    return FW_STATUS_FAILURE;
  }

  if (!sys.get_mtd_name(_mtd_name, dev)) {
    sys.error << "Failed to get device for " << _mtd_name << endl;
    return FW_STATUS_FAILURE;
  }

  syslog(LOG_CRIT, "BMC %s upgrade initiated", (_vers_mtd == "rc") ? "Recovery" : "Active");

  sys.output << "Flashing to device: " << dev << endl;
  cmd = "flashcp -v " + image + " " + dev;
  ret = sys.runcmd(cmd);
  if (ret) {
    return ret;
  }

  sync();
  sleep(2);
  syslog(LOG_CRIT, "BMC capsule copy completed. Version: %s", get_bmc_version(dev).c_str());
  sys.output << "sending update intent to CPLD..." << endl;

  ret = -1;
  pal_get_fru_id((char *)_fru.c_str(), &fruid);
  if (!pal_get_pfr_address(fruid, &bus, &addr, &bridged)) {
    sprintf(dev_i2c, "/dev/i2c-%d", bus);
    if ((ifd = open(dev_i2c, O_RDWR)) >= 0) {
      buf[0] = 0x13;  // BMC update intent
      buf[1] = (_vers_mtd == "rc") ? UPDATE_BMC_RECOVERY : UPDATE_BMC_ACTIVE;
      buf[1] |= UPDATE_AT_RESET;

      do {
        ret = i2c_rdwr_msg_transfer(ifd, addr, buf, 2, NULL, 0);
        if (ret) {
          syslog(LOG_WARNING, "i2c%u xfer failed, addr=%02x, cmd: %02x %02x", bus, addr, buf[0], buf[1]);
          if (--retry > 0)
            msleep(100);
        }
      } while (ret && retry > 0);
      close(ifd);
    }
  }

  return ret;
}
