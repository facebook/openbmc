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
    int update(string image);
};

class CmBlComponent : public McuFwBlComponent {
  uint8_t bus_id;
  uint8_t slv_addr;
  uint8_t target_id;
  public:
    CmBlComponent(string fru, string comp, uint8_t bus, uint8_t addr, uint8_t target)
      : McuFwBlComponent(fru, comp, bus, addr, target), bus_id(bus), 
                         slv_addr(addr), target_id(target) {}
    int print_version();
    int update(string image);
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

static uint32_t 
pal_get_reg_base(uint8_t i2c_bus_num)
{
  if (i2c_bus_num < 7) {
    return 0x1e78a044 + (i2c_bus_num * 0x40);
  } else {
    return 0x1e78a304 + ((i2c_bus_num - 7)  * 0x40);
  }
}

int set_i2c_clock_100k(uint8_t i2c_bus_num)
{
  char cmd[100] = {0};
  uint32_t reg_base = pal_get_reg_base(i2c_bus_num);
  sprintf(cmd, "devmem 0x%x w 0xFFFFE303", reg_base);
  
  if (system(cmd)) {
    printf("set bus %d clk 100khz failed\n", i2c_bus_num);
    return -1;
  }

  return 0;
}

int set_i2c_clock_400k(uint8_t i2c_bus_num)
{
  char cmd[100] = {0};
  uint32_t reg_base = pal_get_reg_base(i2c_bus_num);
  sprintf(cmd, "devmem 0x%x w 0xFFFFE301", reg_base);
  
  if (system(cmd)) {
    printf("set bus %d clk 400khz failed\n", i2c_bus_num);
    return -1;
  }
  return 0;
}

int set_fscd(bool setting)
{
  char cmd[100] = {0};
  const char* setting_str = (setting) ? "start":"stop";
  sprintf(cmd, "sv %s fscd", setting_str);
  if (system(cmd)) {
    printf("set fscd %s failed\n", setting_str);
    return -1;
  }
  return 0;
}

int CmComponent::update(string image)
{
  
  int ret;

  set_i2c_clock_100k(bus_id);
  set_fscd(false);
  ret = mcu_update_firmware(bus_id, slv_addr, (const char *)image.c_str(), 
                           (const char *)pld_name.c_str(), type);


  set_i2c_clock_400k(bus_id);
  set_fscd(true); 
  if (ret != 0) {
     return FW_STATUS_FAILURE;
  }

  return FW_STATUS_SUCCESS;
}
 
int CmBlComponent::update(string image)
{
  
  int ret;

  set_i2c_clock_100k(bus_id);
  set_fscd(false);
  ret = mcu_update_bootloader(bus_id, slv_addr, target_id, (const char *)image.c_str());

  set_i2c_clock_400k(bus_id);
  set_fscd(true); 
  if (ret != 0) {
     return FW_STATUS_FAILURE;
  }

  return FW_STATUS_SUCCESS;
}


CmComponent pdbcm("pdb", "cm", "F0C", 8, 0x68, true);
CmBlComponent pdbcmbl("pdb", "cmbl", 8, 0x68, 0x02);  // target ID of bootloader = 0x02
