#include <cstdio>
#include <cstring>
#include <openbmc/pal.h>
#include <openbmc/mcu.h>
#include "mcu_fw.h"
#include <math.h>

using namespace std;

class CmComponent : public McuFwComponent {
  string pld_name;
  uint8_t bus_id;
  uint8_t slv_addr;
  uint8_t type;
  public:
    CmComponent(const string &fru, const string &comp, const string &name, uint8_t bus, uint8_t addr, uint8_t is_signed)
      : McuFwComponent(fru, comp, name, bus, addr, is_signed), pld_name(name), bus_id(bus), slv_addr(addr), type(is_signed) {}
    int print_version();
    int update(string image);
    void set_update_ongoing(int timeout);
};

class CmBlComponent : public McuFwBlComponent {
  uint8_t bus_id;
  uint8_t slv_addr;
  uint8_t target_id;
  public:
    CmBlComponent(const string &fru, const string &comp, uint8_t bus, uint8_t addr, uint8_t target)
      : McuFwBlComponent(fru, comp, bus, addr, target), bus_id(bus), 
                         slv_addr(addr), target_id(target) {}
    int print_version();
    int update(string image);
    void set_update_ongoing(int timeout);
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
  } catch(string &err) {
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
  } catch(string &err) {
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

static uint32_t i2c_clock_table (int clock) {
  switch (clock) {
    case 100:
      return 0xFFFFE303;
    case 400:
      return 0xFFFFE301;
  }

  // default 0xFFFFE301
  return 0xFFFFE301;
}

static int set_i2c_clock (uint8_t i2c_bus_num, int clock)
{
  char cmd[100] = {0};
  uint32_t reg_base = pal_get_reg_base(i2c_bus_num);
  uint32_t value = i2c_clock_table(clock);
  sprintf(cmd, "devmem 0x%x w 0x%X", reg_base, value);
  if (system(cmd)) {
    printf("set bus %d clk %dkhz failed\n", i2c_bus_num, (int)clock);
    return -1;
  }

  return 0;
}

static int set_fscd (bool setting)
{
  uint8_t tar_bmc_addr;
  uint8_t setting_uint8;
  char cmd[100] = {0};
  
  // Turn on/off itself's fsc demon.
  const char* setting_str = (setting) ? "start":"force-stop";
  sprintf(cmd, "sv %s fscd", setting_str);
  if (system(cmd)) {
    printf("set fscd %s failed\n", setting_str);
    return -1;
  }

  // Turn on/off the other tray fsc demon.
  setting_uint8 = (setting) ? 0x01 : 0x00;
  if ( pal_get_target_bmc_addr(&tar_bmc_addr) ) {
    printf("library: pal_get_target_bmc_addr failed\n");
    return -1;
  }
  if ( cmd_mb_set_fscd (tar_bmc_addr, setting_uint8) ) {
    printf("Set 0x%02X 's fscd 0x%02X failed\n", tar_bmc_addr, setting_uint8);
    return -1;
  }
  return 0;
}

int CmComponent::update(string image)
{
  
  int ret;

  set_fscd (false);
  set_i2c_clock (bus_id, 100);
  ret = mcu_update_firmware(bus_id, slv_addr, (const char *)image.c_str(), 
                           (const char *)pld_name.c_str(), type);

  set_i2c_clock (bus_id, 400);
  set_fscd (true); 
  if (ret != 0) {
     return FW_STATUS_FAILURE;
  }

  return FW_STATUS_SUCCESS;
}
 
int CmBlComponent::update(string image)
{
  
  int ret;

  set_i2c_clock (bus_id, 100);
  ret = mcu_update_bootloader(bus_id, slv_addr, target_id, (const char *)image.c_str());
  set_i2c_clock (bus_id, 400);

  if (ret != 0) {
     return FW_STATUS_FAILURE;
  }

  return FW_STATUS_SUCCESS;
}

static int set_pdb_update_ongoing (int timeout) 
{
  int ret;
  uint16_t timeout_16 = timeout;
  uint8_t tar_bmc_addr;

  if ( pal_get_target_bmc_addr(&tar_bmc_addr) ) {
    printf("library: set_fru_update_ongoing failed\n");
    return -1;
  }
  
  ret = cmd_mb_set_fw_update_ongoing (tar_bmc_addr, FRU_PDB, timeout_16);
  if ( ret ) {
    printf("Set 0x%02X 's fru(%d) fw update timeout (0x%04X) failed (Error Code : %d)\n", 
          tar_bmc_addr, FRU_PDB, timeout_16, ret);
    return -1;
  }
  
  return 0;
}

void CmComponent::set_update_ongoing (int timeout)
{
  Component::set_update_ongoing(timeout);
  set_pdb_update_ongoing (timeout);
}

void CmBlComponent::set_update_ongoing (int timeout)
{
  Component::set_update_ongoing(timeout);
  set_pdb_update_ongoing (timeout);
}

CmComponent pdbcm("pdb", "cm", "F0C", 8, 0x68, true);
CmBlComponent pdbcmbl("pdb", "cmbl", 8, 0x68, 0x02);  // target ID of bootloader = 0x02
