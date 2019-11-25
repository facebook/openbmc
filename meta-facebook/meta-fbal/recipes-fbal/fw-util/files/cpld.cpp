#include "fw-util.h"
#include <stdlib.h>
#include <stdio.h>
#include <openbmc/cpld.h>
#include <openbmc/altera.h>

using namespace std;

class PfrComponent : public Component {
  public:
    PfrComponent(string fru, string comp)
      : Component(fru, comp) {}
    int update(string image) {
      int ret;
      if ( !cpld_intf_open(MAX10_10M16_PFR, INTF_I2C) ) {
        ret = cpld_program((char *)image.c_str());
        cpld_intf_close(INTF_I2C);
        if ( ret < 0 ) {
          printf("Error Occur at updating PFR FW!\n");
        }
      } else {
        printf("Cannot open i2c!\n");
        ret = -1;
      }
      return ret;
    }

    int print_version() {
      uint8_t ver[8];
      if (max10_iic_get_fw_version(MAX10_10M16_PFR, ver)) {
        printf("PFR CPLD Version: NA\n");
      } else {
        printf("PFR CPLD Version: v%02x.%02x.%02x.%02x\n", ver[3], ver[2], ver[1], ver[0]);
      }
      return 0;
    }


};

class ModComponent : public Component {
  public:
    ModComponent(string fru, string comp)
      : Component(fru, comp) {}
    int update(string image) {
      int ret;
      if ( !cpld_intf_open(MAX10_10M16_MOD, INTF_I2C) ) {
        ret = cpld_program((char *)image.c_str());
        cpld_intf_close(INTF_I2C);
        if ( ret < 0 ) {
          printf("Error Occur at updating PFR FW!\n");
        }
      } else {
        printf("Cannot open i2c!\n");
        ret = -1;
      }
      return ret;
    }

    int print_version() {
      uint8_t ver[8];
      if (max10_iic_get_fw_version(MAX10_10M16_MOD, ver)) {
        printf("Module CPLD Version: NA\n");
      } else {
        printf("Module CPLD Version: v%02x.%02x.%02x.%02x\n", ver[3], ver[2], ver[1], ver[0]);
      }
      return 0;
    }
};

PfrComponent pfr("fru", "pfr");
ModComponent mod("fru", "mod");
