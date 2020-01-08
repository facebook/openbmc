#include <cstdio>
#include <cstring>
#include <openbmc/cpld.h>
#include "fw-util.h"

using namespace std;

// According to QSYS setting in FPGA project

// on-chip Flash IP
#define ON_CHIP_FLASH_IP_CSR_BASE        (0x00100020)
#define ON_CHIP_FLASH_IP_CSR_STATUS_REG  (ON_CHIP_FLASH_IP_CSR_BASE + 0x0)
#define ON_CHIP_FLASH_IP_CSR_CTRL_REG    (ON_CHIP_FLASH_IP_CSR_BASE + 0x4)

#define ON_CHIP_FLASH_IP_DATA_REG        (0x00000000)
// Dual-boot IP
#define DUAL_BOOT_IP_BASE                (0x00100000)

#define CFM0_10M25_START_ADDR            (0x00064000)
#define CFM0_10M25_END_ADDR              (0x000BFFFF)
#define CFM1_10M25_START_ADDR            (0x00008000)
#define CFM1_10M25_END_ADDR              (0x00063FFF)

#define CFM0_10M16_START_ADDR            (0x0004A000)
#define CFM0_10M16_END_ADDR              (0x0008BFFF)
#define CFM1_10M16_START_ADDR            (0x00008000)
#define CFM1_10M16_END_ADDR              (0x00049FFF)


enum {
  CFM_IMAGE_NONE = 0,
  CFM_IMAGE_1,
  CFM_IMAGE_2,
};


//CPLD 10M25
class CPLD10m25cfm0 : public Component {
  string pld_name;
  uint8_t pld_type;

  altera_max10_attr_t attr;
  public:
    CPLD10m25cfm0(string fru, string comp, string name, uint8_t type, uint8_t bus, uint8_t addr)
      : Component(fru, comp), pld_name(name), pld_type(type), attr{bus, addr, CFM_IMAGE_1, CFM0_10M25_START_ADDR, CFM0_10M25_END_ADDR, ON_CHIP_FLASH_IP_CSR_BASE, ON_CHIP_FLASH_IP_DATA_REG, DUAL_BOOT_IP_BASE} {}
    int print_version();
    int update(string image);
};

class CPLD10m25cfm1 : public Component {
  string pld_name;
  uint8_t pld_type;

  altera_max10_attr_t attr;
  public:
    CPLD10m25cfm1(string fru, string comp, string name, uint8_t type, uint8_t bus, uint8_t addr)
      : Component(fru, comp), pld_name(name), pld_type(type), attr{bus, addr, CFM_IMAGE_2, CFM1_10M25_START_ADDR, CFM1_10M25_END_ADDR, ON_CHIP_FLASH_IP_CSR_BASE, ON_CHIP_FLASH_IP_DATA_REG, DUAL_BOOT_IP_BASE} {}
    int print_version();
    int update(string image);
};

//CPLD 10M16
class CPLD10m16cfm0 : public Component {
  string pld_name;
  uint8_t pld_type;

  altera_max10_attr_t attr;
  public:
    CPLD10m16cfm0(string fru, string comp, string name, uint8_t type, uint8_t bus, uint8_t addr)
      : Component(fru, comp), pld_name(name), pld_type(type), attr{bus, addr, CFM_IMAGE_1, CFM0_10M16_START_ADDR, CFM0_10M16_END_ADDR, ON_CHIP_FLASH_IP_CSR_BASE, ON_CHIP_FLASH_IP_DATA_REG, DUAL_BOOT_IP_BASE} {}
    int print_version();
    int update(string image);
};

class CPLD10m16cfm1 : public Component {
  string pld_name;
  uint8_t pld_type;

  altera_max10_attr_t attr;
  public:
    CPLD10m16cfm1(string fru, string comp, string name, uint8_t type, uint8_t bus, uint8_t addr)
      : Component(fru, comp), pld_name(name), pld_type(type), attr{bus, addr, CFM_IMAGE_2, CFM1_10M16_START_ADDR, CFM1_10M16_END_ADDR, ON_CHIP_FLASH_IP_CSR_BASE, ON_CHIP_FLASH_IP_DATA_REG, DUAL_BOOT_IP_BASE} {}
    int print_version();
    int update(string image);
};

//10M25 Version
int CPLD10m25cfm0::print_version() {
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

int CPLD10m25cfm1::print_version() {
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

//10M16 Version
int CPLD10m16cfm0::print_version() {
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

int CPLD10m16cfm1::print_version() {
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

//10M25 update
int CPLD10m25cfm0::update(string image) {
  int ret;

  if (cpld_intf_open(pld_type, INTF_I2C, &attr)) 
    printf("Cannot open i2c!\n");
  ret = cpld_program((char *)image.c_str());
  cpld_intf_close(INTF_I2C);
  if (ret) {
    printf("Error Occur at updating CPLD FW!\n");
  }

  return ret;
}

int CPLD10m25cfm1::update(string image) {
  int ret;

  if (cpld_intf_open(pld_type, INTF_I2C, &attr)) 
    printf("Cannot open i2c!\n");
  ret = cpld_program((char *)image.c_str());
  cpld_intf_close(INTF_I2C);
  if (ret) {
    printf("Error Occur at updating CPLD FW!\n");
  }

  return ret;
}

//10M16 update
int CPLD10m16cfm0::update(string image) {
  int ret;

  if (cpld_intf_open(pld_type, INTF_I2C, &attr))
    printf("Cannot open i2c!\n");
  ret = cpld_program((char *)image.c_str());
  cpld_intf_close(INTF_I2C);
  if (ret) {
    printf("Error Occur at updating CPLD FW!\n");
  }

  return ret;
}

int CPLD10m16cfm1::update(string image) {
  int ret;

  if (cpld_intf_open(pld_type, INTF_I2C, &attr))
    printf("Cannot open i2c!\n");
  ret = cpld_program((char *)image.c_str());
  cpld_intf_close(INTF_I2C);
  if (ret) {
    printf("Error Occur at updating CPLD FW!\n");
  }

  return ret;
}

CPLD10m25cfm0 pfr_cfm0("cpld", "pfr_cfm0", "PFR_CFM0", MAX10_10M25, 4, 0x5a);
CPLD10m25cfm1 pfr_cfm1("cpld", "pfr_cfm1", "PFR_CFM1", MAX10_10M25, 4, 0x5a);
CPLD10m16cfm0 mod_cfm0("cpld", "mod_cfm0", "MOD_CFM0", MAX10_10M16, 4, 0x55);
CPLD10m16cfm1 mod_cfm1("cpld", "mod_cfm1", "MOD_CFM1", MAX10_10M16, 4, 0x55);
CPLD10m16cfm0 glb_cfm0("cpld", "glb_cfm0", "GLB_CFM0", MAX10_10M16, 23, 0x55);
CPLD10m16cfm1 glb_cfm1("cpld", "glb_cfm1", "GLB_CFM1", MAX10_10M16, 23, 0x55);
