#include <cstdio>
#include <cstring>
#include <openbmc/pal.h>
#include <openbmc/cpld.h>
#include "fw-util.h"

using namespace std;

// on-chip Flash IP
#define ON_CHIP_FLASH_IP_CSR_BASE  (0x00100020)
#define ON_CHIP_FLASH_IP_DATA_REG  (0x00000000)

// Dual-boot IP
#define DUAL_BOOT_IP_BASE          (0x00100000)
#define CFM0_START_ADDR            (0x00064000)
#define CFM0_END_ADDR              (0x000BFFFF)
#define CFM1_START_ADDR            (0x00008000)
#define CFM1_END_ADDR              (0x00063FFF)

#define CFM0_10M16_START_ADDR      (0x0004A000)
#define CFM0_10M16_END_ADDR        (0x0008BFFF)
#define CFM1_10M16_START_ADDR      (0x00008000)
#define CFM1_10M16_END_ADDR        (0x00049FFF)

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
    int print_version();
    int update(string image);
};

int CpldComponent::print_version() {
  int ret;
  uint8_t ver[4];

  if (cpld_intf_open(pld_type, INTF_I2C, &attr)) {
    printf("Cannot open i2c!\n");
    return -1;
  }

  ret = cpld_get_ver((uint32_t *)ver);
  cpld_intf_close(INTF_I2C);
  if (ret) {
    printf("%s CPLD Version: NA\n", pld_name.c_str());
  } else {
    printf("%s CPLD Version: v%02x.%02x.%02x.%02x\n", pld_name.c_str(), ver[3], ver[2], ver[1], ver[0]);
  }

  return 0;
}

int CpldComponent::update(string image) {
  int ret = -1;
  uint8_t i, cfm_cnt = 2, rev_id = 0xF;

  /// TBD. to check if CPLD file is valid
  if ((image.size() < 4) || strncasecmp(image.substr(image.size()-4).c_str(), ".rpd", 4)) {
    printf("Invalid file\n");
    return ret;
  }

  if (!pal_get_platform_id(BOARD_REV_ID, &rev_id) && (rev_id < 1)) {
    attr.img_type = CFM_IMAGE_1;
    attr.start_addr = CFM0_10M16_START_ADDR;
    attr.end_addr = CFM0_10M16_END_ADDR;
    cfm_cnt = 1;
  }

  for (i = 0; i < cfm_cnt; i++) {
    if (i == 1) {  // CFM1
      attr.img_type = CFM_IMAGE_2;
      attr.start_addr = CFM1_START_ADDR;
      attr.end_addr = CFM1_END_ADDR;
    }

    if (cpld_intf_open(pld_type, INTF_I2C, &attr)) {
      printf("Cannot open i2c!\n");
      return -1;
    }

    ret = cpld_program((char *)image.c_str());
    cpld_intf_close(INTF_I2C);
    if (ret) {
      printf("Error Occur at updating CPLD FW!\n");
      break;
    }
  }

  return ret;
}

CpldComponent pfr("mb", "cpld", "PFR", MAX10_10M25, 4, 0x5a);
