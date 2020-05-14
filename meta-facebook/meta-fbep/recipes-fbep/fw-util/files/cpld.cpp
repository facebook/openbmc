#include "fw-util.h"
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <openbmc/obmc-i2c.h>
#include <openbmc/pal.h>
#include <openbmc/cpld.h>

using namespace std;

class CpldComponent : public Component {
  uint8_t pld_type;
  public:
    CpldComponent(string fru, string comp, uint8_t type)
      : Component(fru, comp), pld_type(type) {}
    int print_version() {
      int ret = -1;
      int ifd = 0;
      uint8_t i, tbuf[8], rbuf[8], cmds[4] = {0x7f, 0x01, 0x7e, 0x7d};
      uint8_t var[4];
      char dev_i2c[16];

      if (pld_type == MAX10_10M16) {
        if (pal_is_pfr_active()) {
          do {
            sprintf(dev_i2c, "/dev/i2c-%d", PFR_MAILBOX_BUS);
            ifd = open(dev_i2c, O_RDWR);
            if (ifd < 0) {
              break;
            }

            for (i = 0; i < sizeof(cmds); i++) {
              tbuf[0] = cmds[i];
              if ((ret = i2c_rdwr_msg_transfer(ifd, PFR_MAILBOX_ADDR, tbuf, 1, rbuf, 1))) {
                break;
              }
              var[i] = rbuf[0];
            }
          } while (0);

          if (ifd > 0) {
            close(ifd);
          }
        }

        if (ret) {
          printf("PFR CPLD Version: NA\n");
        } else {
          printf("PFR CPLD Version: %02X%02X%02X%02X\n", var[3], var[2], var[1], var[0]);
        }
      } else {
        if (!cpld_intf_open(LCMXO2_7000HC, INTF_JTAG, NULL)) {
          // Print CPLD Version
          if (cpld_get_ver((unsigned int *)&var)) {
            printf("CPLD Version: NA, ");
          } else {
            printf("CPLD Version: %02X%02X%02X%02X, ", var[3], var[2], var[1], var[0]);
          }

          // Print CPLD Device ID
          if (cpld_get_device_id((unsigned int *)&var)) {
            printf("CPLD DeviceID: NA\n");
          } else {
            printf("CPLD DeviceID: %02X%02X%02X%02X\n", var[3], var[2], var[1], var[0]);
          }
          cpld_intf_close(INTF_JTAG);
        }
      }

      return 0;
    }
    int update(string image) {
      int ret = -1, pfr_active;

      if (pld_type == MAX10_10M16) {
        pfr_active = pal_is_pfr_active();
        if (pfr_active == PFR_UNPROVISIONED) {
          return FW_STATUS_NOT_SUPPORTED;
        }

        if (pfr_active == PFR_ACTIVE) {
          string dev;
          string cmd;

          if (!sys.get_mtd_name(string("stg-cpld"), dev)) {
            return FW_STATUS_FAILURE;
          }

          sys.output << "Flashing to device: " << dev << endl;
          cmd = "flashcp -v " + image + " " + dev;
          ret = sys.runcmd(cmd);
          return pal_fw_update_finished(0, _component.c_str(), ret);
        }

        return FW_STATUS_NOT_SUPPORTED;
      }

      if (!cpld_intf_open(LCMXO2_7000HC, INTF_JTAG, NULL)) {
        ret = cpld_program((char *)image.c_str(), NULL, false);
        cpld_intf_close(INTF_JTAG);
        if (ret < 0) {
          printf("Error Occur at updating CPLD FW!\n");
        }
      } else {
        printf("Cannot open JTAG!\n");
      }
      return ret;
    }
};

CpldComponent cpld("mb", "cpld", LCMXO2_7000HC);
CpldComponent pfr_cpld("mb", "pfr_cpld", MAX10_10M16);
