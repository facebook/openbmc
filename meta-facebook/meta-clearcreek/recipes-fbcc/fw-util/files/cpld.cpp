#include "fw-util.h"
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <syslog.h>
#include <openbmc/obmc-i2c.h>
#include <openbmc/pal.h>
#include <openbmc/cpld.h>

using namespace std;

// on-chip Flash IP
#define ON_CHIP_FLASH_IP_CSR_BASE  (0x00100020)
#define ON_CHIP_FLASH_IP_DATA_REG  (0x00000000)

// Dual-boot IP
#define DUAL_BOOT_IP_BASE          (0x00100000)
#define CFM0_START_ADDR            (0x0004A000)
#define CFM0_END_ADDR              (0x0008BFFF)
#define CFM1_START_ADDR            (0x00008000)
#define CFM1_END_ADDR              (0x00049FFF)

enum {
  CFM_IMAGE_NONE = 0,
  CFM_IMAGE_1,
  CFM_IMAGE_2,
};

class CpldComponent : public Component {
  string pld_name;
  uint8_t pld_type;
  altera_max10_attr_t attr;
  public:
    CpldComponent(string fru, string comp, string name, uint8_t type, uint8_t bus, uint8_t addr)
      : Component(fru, comp), pld_name(name), pld_type(type),
        attr{bus,addr,CFM_IMAGE_1,CFM0_START_ADDR,CFM0_END_ADDR,ON_CHIP_FLASH_IP_CSR_BASE,ON_CHIP_FLASH_IP_DATA_REG,DUAL_BOOT_IP_BASE} {}
    int print_version() {
      uint8_t var[4];

      if (!cpld_intf_open(pld_type, INTF_I2C, &attr)) {
        // Print CPLD Version
        if (cpld_get_ver((unsigned int *)&var)) {
          printf("CPLD Version: NA, ");
        } else {
          printf("CPLD Version: %02X%02X%02X%02X, ", var[3], var[2], var[1], var[0]);
        }

        cpld_intf_close(INTF_I2C);
      }

      return 0;
    }
    int update(string image) {
      int ret = -1;
      uint8_t i, cfm_cnt = 2;
      string comp = this->component();

      syslog(LOG_CRIT, "Component %s upgrade initiated", comp.c_str());
      for (i = 0; i < cfm_cnt; i++) {
        if (i == 1) {
          // workaround for EVT boards that CONFIG_SEL of main CPLD is floating,
          // so program both CFMs
          attr.img_type = CFM_IMAGE_2;
          attr.start_addr = CFM1_START_ADDR;
          attr.end_addr = CFM1_END_ADDR;
        }

        if (cpld_intf_open(pld_type, INTF_I2C, &attr)) {
          printf("Cannot open i2c!\n");
          return -1;
        }

        ret = cpld_program((char *)image.c_str(), (char *)pld_name.c_str(), false);
        cpld_intf_close(INTF_I2C);
        if (ret) {
          printf("Error Occur at updating CPLD FW!\n");
          break;
        }
      }
      if (ret == 0)
        syslog(LOG_CRIT, "Component %s upgrade completed", comp.c_str());

      return ret;
    }
};

CpldComponent pfr("mb", "cpld", "PFR", MAX10_10M16, 23, 0x5a);
