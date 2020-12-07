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
    {"MB_P12V_AUX:"	  , MB_P12V_AUX	,"V", FRU_MB, 1},
    {"MB_P3V3_STBY:"	, MB_P3V3_STBY,"V", FRU_MB, 1},
    {"MB_P5V_STBY:"	  , MB_P5V_STBY	,"V", FRU_MB, 1},
    {"MB_P3V3:"	      , MB_P3V3	    ,"V", FRU_MB, 2},
    {"MB_P3V3_PAX:"	  , MB_P3V3_PAX	,"V", FRU_MB, 2},
    {"HSC_VIN:"	      , PDB_HSC_VIN ,"V", FRU_PDB, 0},
    {"HSC_PIN:"	      , PDB_HSC_PIN ,"W", FRU_PDB, 0},
    {"FAN0_TACH_I:"	  , PDB_FAN0_TACH_I ,"RPM", FRU_PDB, 0},
    {"FAN0_TACH_O:"	  , PDB_FAN0_TACH_O ,"RPM", FRU_PDB, 0},
    {"FAN1_TACH_I:"	  , PDB_FAN1_TACH_I ,"RPM", FRU_PDB, 0},
    {"FAN1_TACH_O:"	  , PDB_FAN1_TACH_O ,"RPM", FRU_PDB, 0},
    {"FAN2_TACH_I:"	  , PDB_FAN2_TACH_I ,"RPM", FRU_PDB, 0},
    {"FAN2_TACH_O:"	  , PDB_FAN2_TACH_O ,"RPM", FRU_PDB, 0},
    {"FAN3_TACH_I:"	  , PDB_FAN3_TACH_I ,"RPM", FRU_PDB, 0},
    {"FAN3_TACH_O:"	  , PDB_FAN3_TACH_O ,"RPM", FRU_PDB, 0},
    {"Inlet_TEMP:"	  , PDB_INLET_TEMP_L ,"C", FRU_PDB, 0},
    {"Inlet_R_TEMP:"	, PDB_INLET_REMOTE_TEMP_L ,"C", FRU_PDB, 0},
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
