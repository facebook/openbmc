#include "fw-util.h"
#include <stdlib.h>
#include <stdio.h>
#include <openbmc/cpld.h>

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
};



PfrComponent pfr("fru", "pfr");
ModComponent mod("fru", "mod");

