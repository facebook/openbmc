#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <openbmc/pal.h>

#include "usb-dbg-conf.h"

static gpio_desc_t gdesc[] = {
  { 0x10, 0, 2, "FM_DBG_RST_BTN" },
  { 0x11, 0, 1, "FM_PWR_BTN" },
  { 0x12, 0, 0, "SYS_PWROK" },
  { 0x13, 0, 0, "RST_PLTRST" },
  { 0x14, 0, 0, "DSW_PWROK" },
  { 0x15, 0, 0, "FM_CATERR_MSMI" },
  { 0x16, 0, 0, "FM_SLPS3" },
  { 0x17, 0, 3, "FM_UART_SWITCH" },
};

static sensor_desc_t cri_sensor[]  =
{
    {"HSC_AUX_PWR:"	, PDB_HSC_P12V_AUX_PWR	,"W", FRU_PDB, 1},
    {"HSC_1_PWR:"	, PDB_HSC_P12V_1_PWR	,"W", FRU_PDB, 1},
    {"HSC_2_PWR:"	, PDB_HSC_P12V_2_PWR	,"W", FRU_PDB, 1},
    {"HSC_AUX_VOL:"	, PDB_HSC_P12V_AUX_VIN	,"V", FRU_PDB, 2},
    {"HSC_1_VOL:"	, PDB_HSC_P12V_1_VIN	,"V", FRU_PDB, 2},
    {"HSC_2_VOL:"	, PDB_HSC_P12V_2_VIN	,"V", FRU_PDB, 2},
    {"Fan0:"		, MB_FAN0_TACH_I	,"RPM", FRU_MB, 0},
    {"Fan1:"		, MB_FAN1_TACH_I	,"RPM", FRU_MB, 0},
    {"Fan2:"		, MB_FAN2_TACH_I	,"RPM", FRU_MB, 0},
    {"Fan3:"		, MB_FAN3_TACH_I	,"RPM", FRU_MB, 0},
    {"Inlet_TEMP:"	, MB_SENSOR_GPU_INLET_REMOTE ,"C", FRU_MB, 0},
    {"Outlet_TEMP:"	, PDB_SENSOR_OUTLET_TEMP_REMOTE ,"C", FRU_PDB, 0},
};

bool plat_supported(void)
{
  return true;
}

int plat_get_post_phase(uint8_t fru, post_phase_desc_t **desc, size_t *desc_count)
{
  // Not supported
  return -1;
}

int plat_get_gdesc(uint8_t fru, gpio_desc_t **desc, size_t *desc_count)
{
  if (!desc || !desc_count) {
    return -1;
  }
  *desc = gdesc;
  *desc_count = sizeof(gdesc) / sizeof(gdesc[0]);
  return 0;
}

int plat_get_sensor_desc(uint8_t fru, sensor_desc_t **desc, size_t *desc_count)
{
  if (!desc || !desc_count) {
    return -1;
  }
  *desc = cri_sensor;
  *desc_count = sizeof(cri_sensor) / sizeof(cri_sensor[0]);
  return 0;
}

uint8_t plat_get_fru_sel(void)
{
  return FRU_MB;
}

int plat_get_me_status(uint8_t fru, char *status)
{
  // Not supported
  return -1;
}

int plat_get_board_id(char *id)
{
  uint8_t byte;

  if (!pal_get_platform_id(&byte)) {
    sprintf(id, "%d%d%d%d%d",
      (byte & (1<<4))?1:0,
      (byte & (1<<3))?1:0,
      (byte & (1<<2))?1:0,
      (byte & (1<<1))?1:0,
      (byte & (1<<0))?1:0);
    return 0;
  }
  return -1;
}

int plat_get_syscfg_text(uint8_t fru, char *syscfg)
{
  // Not supported
  return -1;
}

int plat_get_etra_fw_version(uint8_t slot_id, char *fw_text)
{
  // Not supported
  return -1;
}

int plat_get_extra_sysinfo(uint8_t fru, char *info)
{
  // Not supported
  return -1;
}
