#include <cstdio>
#include <cstring>
#include <openbmc/pal.h>
#include <openbmc/mcu.h>
#include "mcu_fw.h"

using namespace std;

class CmComponent : public McuFwComponent {
  string pld_name;
  uint8_t bus_id;
  uint8_t slv_addr;
  uint8_t type;
  public:
    CmComponent(string fru, string comp, string name, uint8_t bus, uint8_t addr, uint8_t is_signed)
      : McuFwComponent(fru, comp, name, bus, addr, is_signed), pld_name(name), bus_id(bus), slv_addr(addr), type(is_signed) {}
    int print_version();
};

class CmBlComponent : public McuFwBlComponent {
  uint8_t bus_id;
  uint8_t slv_addr;
  public:
    CmBlComponent(string fru, string comp, uint8_t bus, uint8_t addr, uint8_t target)
      : McuFwBlComponent(fru, comp, bus, addr, target), bus_id(bus), slv_addr(addr) {}
    int print_version();
};

int CmComponent::print_version() {
  uint8_t ver[8];

  try {
    if (!pal_is_mcu_ready(bus_id) || mcu_get_fw_ver(bus_id, slv_addr, MCU_FW_RUNTIME, ver)) {
      printf("CM Version: NA\n");
    }
    else {
      printf("CM Version: v%x.%02x\n", ver[0], ver[1]);
    }
  } catch(string err) {
    printf("CM Version: NA (%s)\n", err.c_str());
  }
  return 0;
}

int CmBlComponent::print_version() {
  uint8_t ver[8];

  try {
    if (!pal_is_mcu_ready(bus_id) || mcu_get_fw_ver(bus_id, slv_addr, MCU_FW_BOOTLOADER, ver)) {
      printf("CM Bootloader Version: NA\n");
    }
    else {
      printf("CM Bootloader Version: v%x.%02x\n", ver[0], ver[1]);
    }
  } catch(string err) {
    printf("CM Bootloader Version: NA (%s)\n", err.c_str());
  }
  return 0;
}

CmComponent pdbcm("pdb", "cm", "F0C", 8, 0x68, true);
CmBlComponent pdbcmbl("pdb", "cmbl", 8, 0x68, 0x02);  // target ID of bootloader = 0x02
