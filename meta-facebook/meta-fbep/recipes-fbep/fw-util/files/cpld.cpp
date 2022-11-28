#include "fw-util.h"
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <syslog.h>
#include <openbmc/obmc-i2c.h>
#include <openbmc/pal.h>
#include <openbmc/cpld.h>

using namespace std;

enum {
  CFM_IMAGE_NONE = 0,
  CFM_IMAGE_1,
  CFM_IMAGE_2,
};

enum {
  CFM0_10M16 = 0,
  CFM1_10M16
};

class CpldComponent : public Component {
  public:
    CpldComponent(const string &fru, const string &comp)
      : Component(fru, comp) {}
    int print_version() {
      char strbuf[MAX_VALUE_LEN] = {0};
      uint8_t ver[4];
      int ret = -1;
      string comp = this->component();

      do {
        if (!kv_get(comp.c_str(), strbuf, NULL, 0))
          break;

        if (!cpld_intf_open(LCMXO2_7000HC, INTF_JTAG, NULL)) {
          ret = cpld_get_ver((uint32_t *)ver);
          cpld_intf_close();
        }

        if (ret) {
          sprintf(strbuf, "NA");
          break;
        }

        sprintf(strbuf, "%02X%02X%02X%02X", ver[3], ver[2], ver[1], ver[0]);
        kv_set(comp.c_str(), strbuf, 0, 0);
      } while (0);

      transform(comp.begin(), comp.end(),comp.begin(), ::toupper);
      sys().output << comp << " Version: " << string(strbuf) << endl;

      return 0;
    }
    int update(string image) {
      int ret = -1;
      string comp = this->component();

      if (!cpld_intf_open(LCMXO2_7000HC, INTF_JTAG, NULL)) {
        syslog(LOG_CRIT, "Component %s upgrade initiated", comp.c_str());
        ret = cpld_program((char *)image.c_str(), NULL, false);
        cpld_intf_close();
        if (ret < 0) {
          printf("Error Occur at updating CPLD FW!\n");
        } else {
          syslog(LOG_CRIT, "Component %s upgrade completed", comp.c_str());
        }
      } else {
        printf("Cannot open JTAG!\n");
      }

      return ret;
    }
};

class DumbCpldComponent : public Component {
  uint8_t pld_type;
  altera_max10_attr_t attr;
  public:
    DumbCpldComponent(const string &fru, const string &comp, uint8_t type, uint8_t /*ctype*/, uint8_t bus, uint8_t addr)
      : Component(fru, comp), pld_type(type), attr{bus, addr, 0, 0, 0, 0, 0, 0} {}
    int print_version();
};

int DumbCpldComponent::print_version() {
  int ret = -1;
  uint8_t ver[4];
  char strbuf[16];
  string comp;

  if (!cpld_intf_open(pld_type, INTF_I2C, &attr)) {
    ret = cpld_get_ver((uint32_t *)ver);
    cpld_intf_close();
  }

  if (ret) {
    sprintf(strbuf, "NA");
  } else {
    sprintf(strbuf, "%02X%02X%02X%02X", ver[3], ver[2], ver[1], ver[0]);
  }

  comp = _component;
  transform(comp.begin(), comp.end(),comp.begin(), ::toupper);
  sys().output << comp << " Version: " << string(strbuf) << endl;

  return 0;
}

CpldComponent cpld("mb", "cpld");
DumbCpldComponent pfr_cpld("mb", "pfr_cpld", MAX10_10M16, CFM0_10M16, 4, 0x5a);

/*
class PfrCpldComponent : public Component {
  public:
    PfrCpldComponent(string fru, string comp)
      : Component(fru, comp) {}
    int update(string image);
    int print_version();
};

int PfrCpldComponent::update(string image) {
  int ret;
  string dev, cmd;

  if (pal_is_pfr_active() != PFR_ACTIVE) {
    return FW_STATUS_NOT_SUPPORTED;
  }

  if (!sys().get_mtd_name(string("stg-cpld"), dev)) {
    return FW_STATUS_FAILURE;
  }

  sys().output << "Flashing to device: " << dev << endl;
  cmd = "flashcp -v " + image + " " + dev;
  ret = sys().runcmd(cmd);
  ret = pal_fw_update_finished(0, _component.c_str(), ret);

  return ret;
}

int PfrCpldComponent::print_version() {
  int ret = -1;
  int ifd = 0;
  uint8_t i, tbuf[8], rbuf[8], cmds[4] = {0x7f, 0x01, 0x7e, 0x7d};
  uint8_t ver[4];
  char strbuf[16];
  string comp;

  if (_component == "pfr_cpld_rc") {
    return 0;
  }

  do {
    sprintf(strbuf, "/dev/i2c-%d", PFR_MAILBOX_BUS);
    ifd = open(strbuf, O_RDWR);
    if (ifd < 0) {
      break;
    }

    for (i = 0; i < sizeof(cmds); i++) {
      tbuf[0] = cmds[i];
      if ((ret = i2c_rdwr_msg_transfer(ifd, PFR_MAILBOX_ADDR, tbuf, 1, rbuf, 1))) {
        break;
      }
      ver[i] = rbuf[0];
    }
  } while (0);
  if (ifd > 0) {
    close(ifd);
  }

  if (ret) {
    sprintf(strbuf, "NA");
  } else {
    sprintf(strbuf, "%02X%02X%02X%02X", ver[3], ver[2], ver[1], ver[0]);
  }

  comp = _component;
  transform(comp.begin(), comp.end(),comp.begin(), ::toupper);
  sys().output << comp << " Version: " << string(strbuf) << endl;

  return 0;
}

class CpldConfig {
  public:
  CpldConfig() {
    static CpldComponent cpld("mb", "cpld");

    if (pal_is_pfr_active()) {
      static PfrCpldComponent pfr_cpld("mb", "pfr_cpld");
      static PfrCpldComponent pfr_cpld_rc("mb", "pfr_cpld_rc");
    }
  }
};

CpldConfig cpld_conf;
*/
