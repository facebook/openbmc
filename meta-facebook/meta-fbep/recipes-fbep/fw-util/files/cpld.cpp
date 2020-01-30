#include "fw-util.h"
#include <stdlib.h>
#include <stdio.h>
#include <openbmc/cpld.h>

using namespace std;

class CpldComponent : public Component {
  public:
    CpldComponent(string fru, string comp)
      : Component(fru, comp) {}
    int print_version() {
      uint8_t cpld_var[4] = {0};
      if (!cpld_intf_open(LCMXO2_7000HC, INTF_JTAG, NULL)) {
        // Print CPLD Version
        if (cpld_get_ver((unsigned int *)&cpld_var)) {
          printf("CPLD Version: NA, ");
        } else {
          printf("CPLD Version: %02X%02X%02X%02X, ", cpld_var[3], cpld_var[2],
		        cpld_var[1], cpld_var[0]);
        }

        // Print CPLD Device ID
        if (cpld_get_device_id((unsigned int *)&cpld_var)) {
          printf("CPLD DeviceID: NA\n");
        } else {
          printf("CPLD DeviceID: %02X%02X%02X%02X\n", cpld_var[3], cpld_var[2],
		        cpld_var[1], cpld_var[0]);
        }
        cpld_intf_close(INTF_JTAG);
      }
      return 0;
    }
    int update(string image) {
      int ret;
      char key[32]= {0};

      if (!cpld_intf_open(LCMXO2_7000HC, INTF_JTAG, NULL)) {
        ret = cpld_program((char *)image.c_str(), key, false);
        cpld_intf_close(INTF_JTAG);
        if ( ret < 0 ) {
          printf("Error Occur at updating CPLD FW!\n");
        }
      } else {
        printf("Cannot open JTAG!\n");
        ret = -1;
      }
      return ret;
    }
};

CpldComponent cpld("mb", "cpld");
