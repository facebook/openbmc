#include <cstdio>
#include <cstring>
#include <openbmc/obmc-i2c.h>
#include <openbmc/pal.h>
#include "pfr_bios.h"

enum {
  PCH_PFM_MAJOR    = 0x15,
  PCH_PFM_MINOR    = 0x16,
  PCH_PFM_RC_MAJOR = 0x1B,
  PCH_PFM_RC_MINOR = 0x1C,
};

using namespace std;

int PfrBiosComponent::update(std::string image, bool force) {
  int ret = -1;
  string dev, cmd;

  if (!force) {
    ret = check_image(image.c_str());
    if (ret) {
      sys.error << "Invalid image. Stopping the update!" << endl;
      return -1;
    }
  }

  if (!sys.get_mtd_name(string("stg-pch"), dev)) {
    return FW_STATUS_FAILURE;
  }

  sys.output << "Flashing to device: " << dev << endl;
  cmd = "flashcp -v " + image + " " + dev;
  ret = sys.runcmd(cmd);
  ret = pal_fw_update_finished(0, _component.c_str(), ret);

  return ret;
}

int PfrBiosComponent::update(string image) {
  return update(image, 0);
}

int PfrBiosComponent::fupdate(string image) {
  return update(image, 1);
}

int PfrBiosComponent::print_version() {
  int ret = -1, ifd = 0;
  uint8_t i, end, ver[64] = {0};
  uint8_t tbuf[8], rbuf[8], cmds[2];
  uint8_t fruid = 1, bus, addr;
  char dev_i2c[16], pfm_ver[8], verstr[64];
  bool bridged;
  string sysfw;

  pal_get_fru_id((char *)_fru.c_str(), &fruid);

  if (pfm_type == PCH_PFM_ACTIVE) {
    if (!pal_get_sysfw_ver(fruid, ver)) {
      if ((end = 3+ver[2]) > sizeof(ver)) {
        end = sizeof(ver);
      }
      for (i = 3; i < end; i++) {
        sysfw.append(1, ver[i]);
      }
    } else {
      sysfw.assign("NA");
    }

    cmds[0] = PCH_PFM_MAJOR;
    cmds[1] = PCH_PFM_MINOR;
  } else {
    cmds[0] = PCH_PFM_RC_MAJOR;
    cmds[1] = PCH_PFM_RC_MINOR;
  }

  if (!pal_get_pfr_address(fruid, &bus, &addr, &bridged)) {
    sprintf(dev_i2c, "/dev/i2c-%d", bus);
    if ((ifd = open(dev_i2c, O_RDWR)) >= 0) {
      for (i = 0; i < sizeof(cmds); i++) {
        tbuf[0] = cmds[i];
        if ((ret = i2c_rdwr_msg_transfer(ifd, addr, tbuf, 1, rbuf, 1))) {
          break;
        }
        ver[i] = rbuf[0];
      }
      close(ifd);
    }
  }

  if (ret) {
    sprintf(pfm_ver, "NA");
  } else {
    sprintf(pfm_ver, "%02X%02X", ver[0], ver[1]);
  }

  if (sysfw.size()) {
    sprintf(verstr, " Version: %s (%s)", sysfw.c_str(), pfm_ver);
  } else {
    sprintf(verstr, " Version: %s", pfm_ver);
  }

  sysfw = _component;
  transform(sysfw.begin(), sysfw.end(),sysfw.begin(), ::toupper);
  sys.output << sysfw << string(verstr) << endl;

  return 0;
}
