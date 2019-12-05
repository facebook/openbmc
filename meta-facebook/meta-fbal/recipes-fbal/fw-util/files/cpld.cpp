#include <cstdio>
#include <cstring>
#include <openbmc/altera.h>
#include <openbmc/cpld.h>
#include "fw-util.h"

using namespace std;

class CpldComponent : public Component {
  string pld_name;
  uint8_t pld_type;
  uint8_t bus_id;
  uint8_t slv_addr;
  public:
    CpldComponent(string fru, string comp, string name, uint8_t type, uint8_t bus, uint8_t addr)
      : Component(fru, comp), pld_name(name), pld_type(type), bus_id(bus), slv_addr(addr) {}
    int print_version();
    int update(string image);
};

int CpldComponent::print_version() {
  int ret;
  uint8_t ver[4];

  max10_iic_init(bus_id, slv_addr);
  if (cpld_intf_open(pld_type, INTF_I2C)) {
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
  int ret;

  max10_iic_init(bus_id, slv_addr);
  if (cpld_intf_open(pld_type, INTF_I2C)) {
    printf("Cannot open i2c!\n");
    return -1;
  }

  ret = cpld_program((char *)image.c_str());
  cpld_intf_close(INTF_I2C);
  if (ret) {
    printf("Error Occur at updating CPLD FW!\n");
  }

  return ret;
}

CpldComponent pfr("cpld", "pfr", "PFR", MAX10_10M25, 4, 0x5a);
CpldComponent mod("cpld", "mod", "Modular", MAX10_10M16, 4, 0x55);
