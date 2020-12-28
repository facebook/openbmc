/*
 *
 * Copyright 2020-present Facebook. All Rights Reserved.
 *
 * This file contains code to support IPMI2.0 Specificaton available @
 * http://www.intel.com/content/www/us/en/servers/ipmi/ipmi-specifications.html
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

 /*
  * This file contains functions and logics that depends on Cloudripper specific
  * hardware and kernel drivers. Here, some of the empty "default" functions
  * are overridden with simple functions that returns non-zero error code.
  * This is for preventing any potential escape of failures through the
  * default functions that will return 0 no matter what.
  */

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <syslog.h>
#include <time.h>
#include <unistd.h>
#include <pthread.h>
#include <ctype.h>
#include <linux/limits.h>
#include <linux/version.h>
#include <facebook/bic.h>
#include <openbmc/obmc-i2c.h>
#include <openbmc/misc-utils.h>
#include <openbmc/log.h>
#include "pal-sensors.h"
#include "pal.h"

#define RUN_SHELL_CMD(_cmd)                              \
  do {                                                   \
    int _ret = system(_cmd);                             \
    if (_ret != 0)                                       \
      OBMC_WARN("'%s' command returned %d", _cmd, _ret); \
  } while (0)

#define SENSOR_NAME_ERR "---- It should not be show ----"

typedef struct {
  char name[32];
} sensor_desc_t;

typedef struct {
  uint16_t flag;
  float ucr;
  float unc;
  float unr;
  float lcr;
  float lnc;
  float lnr;
  float pos_hyst;
  float neg_hyst;
  int curr_st;
  char name[32];
} _sensor_thresh_t;

static sensor_desc_t m_snr_desc[MAX_NUM_FRUS][MAX_SENSOR_NUM] = {0};

/* List of BIC Discrete sensors to be monitored */
const uint8_t bic_discrete_list[] = {
  BIC_SENSOR_SYSTEM_STATUS,
  BIC_SENSOR_PROC_FAIL,
  BIC_SENSOR_SYS_BOOT_STAT,
  BIC_SENSOR_CPU_DIMM_HOT,
  BIC_SENSOR_VR_HOT,
  BIC_SENSOR_POWER_THRESH_EVENT,
  BIC_SENSOR_POST_ERR,
  BIC_SENSOR_POWER_ERR,
  BIC_SENSOR_PROC_HOT_EXT,
  BIC_SENSOR_MACHINE_CHK_ERR,
  BIC_SENSOR_PCIE_ERR,
  BIC_SENSOR_OTHER_IIO_ERR,
  BIC_SENSOR_MEM_ECC_ERR,
  BIC_SENSOR_SPS_FW_HLTH,
  BIC_SENSOR_CAT_ERR,
};

// List of BIC sensors which need to do negative reading handle
const uint8_t bic_neg_reading_sensor_support_list[] = {
  /* Temperature sensors*/
  BIC_SENSOR_MB_OUTLET_TEMP,
  BIC_SENSOR_MB_INLET_TEMP,
  BIC_SENSOR_PCH_TEMP,
  BIC_SENSOR_SOC_TEMP,
  BIC_SENSOR_SOC_DIMMA_TEMP,
  BIC_SENSOR_SOC_DIMMB_TEMP,
  BIC_SENSOR_VCCIN_VR_CURR,
};

/* List of SCM sensors to be monitored */
const uint8_t scm_sensor_list[] = {
  SCM_SENSOR_OUTLET_TEMP,
  SCM_SENSOR_INLET_TEMP,
  SCM_SENSOR_HSC_OUT_VOLT,
  SCM_SENSOR_HSC_OUT_CURR,
};

/* List of SCM and BIC sensors to be monitored */
const uint8_t scm_all_sensor_list[] = {
  SCM_SENSOR_OUTLET_TEMP,
  SCM_SENSOR_INLET_TEMP,
  SCM_SENSOR_HSC_OUT_VOLT,
  SCM_SENSOR_HSC_OUT_CURR,
  BIC_SENSOR_MB_OUTLET_TEMP,
  BIC_SENSOR_MB_INLET_TEMP,
  BIC_SENSOR_PCH_TEMP,
  BIC_SENSOR_VCCIN_VR_TEMP,
  BIC_SENSOR_1V05COMB_VR_TEMP,
  BIC_SENSOR_SOC_TEMP,
  BIC_SENSOR_SOC_THERM_MARGIN,
  BIC_SENSOR_VDDR_VR_TEMP,
  BIC_SENSOR_SOC_DIMMA_TEMP,
  BIC_SENSOR_SOC_DIMMB_TEMP,
  BIC_SENSOR_SOC_PACKAGE_PWR,
  BIC_SENSOR_VCCIN_VR_POUT,
  BIC_SENSOR_VDDR_VR_POUT,
  BIC_SENSOR_SOC_TJMAX,
  BIC_SENSOR_P3V3_MB,
  BIC_SENSOR_P12V_MB,
  BIC_SENSOR_P1V05_PCH,
  BIC_SENSOR_P3V3_STBY_MB,
  BIC_SENSOR_P5V_STBY_MB,
  BIC_SENSOR_PV_BAT,
  BIC_SENSOR_PVDDR,
  BIC_SENSOR_P1V05_COMB,
  BIC_SENSOR_1V05COMB_VR_CURR,
  BIC_SENSOR_VDDR_VR_CURR,
  BIC_SENSOR_VCCIN_VR_CURR,
  BIC_SENSOR_VCCIN_VR_VOL,
  BIC_SENSOR_VDDR_VR_VOL,
  BIC_SENSOR_P1V05COMB_VR_VOL,
  BIC_SENSOR_P1V05COMB_VR_POUT,
  BIC_SENSOR_INA230_POWER,
  BIC_SENSOR_INA230_VOL,
};

/* List of SMB sensors to be monitored */
const uint8_t smb_sensor_list[] = {
  SMB_SENSOR_1220_VMON1,
  SMB_SENSOR_1220_VMON2,
  SMB_SENSOR_1220_VMON3,
  SMB_SENSOR_1220_VMON4,
  SMB_SENSOR_1220_VMON5,
  SMB_SENSOR_1220_VMON6,
  SMB_SENSOR_1220_VMON7,
  SMB_SENSOR_1220_VMON8,
  SMB_SENSOR_1220_VMON9,
  SMB_SENSOR_1220_VMON10,
  SMB_SENSOR_1220_VMON11,
  SMB_SENSOR_1220_VMON12,
  SMB_SENSOR_1220_VCCA,
  SMB_SENSOR_1220_VCCINP,
  SMB_SENSOR_VDDA_IN_VOLT,
  SMB_SENSOR_VDDA_IN_CURR,
  SMB_SENSOR_VDDA_IN_POWER,
  SMB_SENSOR_VDDA_OUT_VOLT,
  SMB_SENSOR_VDDA_OUT_CURR,
  SMB_SENSOR_VDDA_OUT_POWER,
  SMB_SENSOR_VDDA_TEMP1,
  SMB_SENSOR_PCIE_IN_VOLT,
  SMB_SENSOR_PCIE_IN_CURR,
  SMB_SENSOR_PCIE_IN_POWER,
  SMB_SENSOR_PCIE_OUT_VOLT,
  SMB_SENSOR_PCIE_OUT_CURR,
  SMB_SENSOR_PCIE_OUT_POWER,
  SMB_SENSOR_PCIE_TEMP1,
  SMB_SENSOR_IR3R3V_LEFT_IN_VOLT,
  SMB_SENSOR_IR3R3V_LEFT_IN_CURR,
  SMB_SENSOR_IR3R3V_LEFT_IN_POWER,
  SMB_SENSOR_IR3R3V_LEFT_OUT_VOLT,
  SMB_SENSOR_IR3R3V_LEFT_OUT_CURR,
  SMB_SENSOR_IR3R3V_LEFT_OUT_POWER,
  SMB_SENSOR_IR3R3V_LEFT_TEMP,
  SMB_SENSOR_IR3R3V_RIGHT_IN_VOLT,
  SMB_SENSOR_IR3R3V_RIGHT_IN_CURR,
  SMB_SENSOR_IR3R3V_RIGHT_IN_POWER,
  SMB_SENSOR_IR3R3V_RIGHT_OUT_VOLT,
  SMB_SENSOR_IR3R3V_RIGHT_OUT_CURR,
  SMB_SENSOR_IR3R3V_RIGHT_OUT_POWER,
  SMB_SENSOR_IR3R3V_RIGHT_TEMP,
  SMB_SENSOR_SW_CORE_IN_VOLT,
  SMB_SENSOR_SW_CORE_IN_CURR,
  SMB_SENSOR_SW_CORE_IN_POWER,
  SMB_SENSOR_SW_CORE_OUT_VOLT,
  SMB_SENSOR_SW_CORE_OUT_CURR,
  SMB_SENSOR_SW_CORE_OUT_POWER,
  SMB_SENSOR_SW_CORE_TEMP1,
  SMB_SENSOR_XPDE_HBM_IN_VOLT,
  SMB_SENSOR_XPDE_HBM_IN_CURR,
  SMB_SENSOR_XPDE_HBM_IN_POWER,
  SMB_SENSOR_XPDE_HBM_OUT_VOLT,
  SMB_SENSOR_XPDE_HBM_OUT_CURR,
  SMB_SENSOR_XPDE_HBM_OUT_POWER,
  SMB_SENSOR_XPDE_HBM_TEMP1,

  /* PXE1211C */
  SMB_SENSOR_VDDCK_0_IN_VOLT,
  SMB_SENSOR_VDDCK_0_IN_CURR,
  SMB_SENSOR_VDDCK_0_IN_POWER,
  SMB_SENSOR_VDDCK_0_OUT_VOLT,
  SMB_SENSOR_VDDCK_0_OUT_CURR,
  SMB_SENSOR_VDDCK_0_OUT_POWER,
  SMB_SENSOR_VDDCK_0_TEMP,
  SMB_SENSOR_VDDCK_1_IN_VOLT,
  SMB_SENSOR_VDDCK_1_IN_CURR,
  SMB_SENSOR_VDDCK_1_IN_POWER,
  SMB_SENSOR_VDDCK_1_OUT_VOLT,
  SMB_SENSOR_VDDCK_1_OUT_CURR,
  SMB_SENSOR_VDDCK_1_OUT_POWER,
  SMB_SENSOR_VDDCK_1_TEMP,
  SMB_SENSOR_VDDCK_2_IN_VOLT,
  SMB_SENSOR_VDDCK_2_IN_CURR,
  SMB_SENSOR_VDDCK_2_IN_POWER,
  SMB_SENSOR_VDDCK_2_OUT_VOLT,
  SMB_SENSOR_VDDCK_2_OUT_CURR,
  SMB_SENSOR_VDDCK_2_OUT_POWER,
  SMB_SENSOR_VDDCK_2_TEMP,

  SMB_SENSOR_XDPE_LEFT_1_IN_VOLT,
  SMB_SENSOR_XDPE_LEFT_1_IN_CURR,
  SMB_SENSOR_XDPE_LEFT_1_IN_POWER,
  SMB_SENSOR_XDPE_LEFT_1_OUT_VOLT,
  SMB_SENSOR_XDPE_LEFT_1_OUT_CURR,
  SMB_SENSOR_XDPE_LEFT_1_OUT_POWER,
  SMB_SENSOR_XDPE_LEFT_1_TEMP,
  SMB_SENSOR_XDPE_LEFT_2_IN_VOLT,
  SMB_SENSOR_XDPE_LEFT_2_IN_CURR,
  SMB_SENSOR_XDPE_LEFT_2_IN_POWER,
  SMB_SENSOR_XDPE_LEFT_2_OUT_VOLT,
  SMB_SENSOR_XDPE_LEFT_2_OUT_CURR,
  SMB_SENSOR_XDPE_LEFT_2_OUT_POWER,
  SMB_SENSOR_XDPE_LEFT_2_TEMP,
  SMB_SENSOR_XDPE_RIGHT_1_IN_VOLT,
  SMB_SENSOR_XDPE_RIGHT_1_IN_CURR,
  SMB_SENSOR_XDPE_RIGHT_1_IN_POWER,
  SMB_SENSOR_XDPE_RIGHT_1_OUT_VOLT,
  SMB_SENSOR_XDPE_RIGHT_1_OUT_CURR,
  SMB_SENSOR_XDPE_RIGHT_1_OUT_POWER,
  SMB_SENSOR_XDPE_RIGHT_1_TEMP,
  SMB_SENSOR_XDPE_RIGHT_2_IN_VOLT,
  SMB_SENSOR_XDPE_RIGHT_2_IN_CURR,
  SMB_SENSOR_XDPE_RIGHT_2_IN_POWER,
  SMB_SENSOR_XDPE_RIGHT_2_OUT_VOLT,
  SMB_SENSOR_XDPE_RIGHT_2_OUT_CURR,
  SMB_SENSOR_XDPE_RIGHT_2_OUT_POWER,
  SMB_SENSOR_XDPE_RIGHT_2_TEMP,
  SMB_SENSOR_LM75B_U28_TEMP,
  SMB_SENSOR_LM75B_U25_TEMP,
  SMB_SENSOR_LM75B_U56_TEMP,
  SMB_SENSOR_LM75B_U55_TEMP,
  SMB_SENSOR_LM75B_U2_TEMP,
  SMB_SENSOR_LM75B_U13_TEMP,
  SMB_SENSOR_TMP421_U62_TEMP,
  SMB_SENSOR_TMP421_U63_TEMP,
  SMB_SENSOR_BMC_LM75B_TEMP,
  SMB_DOM1_MAX_TEMP,
  SMB_DOM2_MAX_TEMP,
  // /* Sensors on FCM */
  SMB_SENSOR_FCM_LM75B_U1_TEMP,
  SMB_SENSOR_FCM_LM75B_U2_TEMP,
  SMB_SENSOR_FCM_HSC_IN_VOLT,
  SMB_SENSOR_FCM_HSC_IN_POWER,
  SMB_SENSOR_FCM_HSC_OUT_VOLT,
  SMB_SENSOR_FCM_HSC_OUT_CURR,
  // /* Sensors FAN Speed */
  SMB_SENSOR_FAN1_FRONT_TACH,
  SMB_SENSOR_FAN1_REAR_TACH,
  SMB_SENSOR_FAN2_FRONT_TACH,
  SMB_SENSOR_FAN2_REAR_TACH,
  SMB_SENSOR_FAN3_FRONT_TACH,
  SMB_SENSOR_FAN3_REAR_TACH,
  SMB_SENSOR_FAN4_FRONT_TACH,
  SMB_SENSOR_FAN4_REAR_TACH,
  /* BMC ADC Sensors  */
  SMB_BMC_ADC0_VSEN,
  SMB_BMC_ADC1_VSEN,
  SMB_BMC_ADC2_VSEN,
  SMB_BMC_ADC3_VSEN,
  SMB_BMC_ADC4_VSEN,
  SMB_BMC_ADC5_VSEN,
  SMB_BMC_ADC6_VSEN,
  SMB_BMC_ADC7_VSEN,
  SMB_BMC_ADC8_VSEN,
  SMB_BMC_ADC9_VSEN,
  SMB_BMC_ADC10_VSEN,
  SMB_BMC_ADC11_VSEN,

  // /* GB switch internal sensors */
  SMB_SENSOR_GB_HIGH_TEMP,
  SMB_SENSOR_GB_TEMP1,
  SMB_SENSOR_GB_TEMP2,
  SMB_SENSOR_GB_TEMP3,
  SMB_SENSOR_GB_TEMP4,
  SMB_SENSOR_GB_TEMP5,
  SMB_SENSOR_GB_TEMP6,
  SMB_SENSOR_GB_TEMP7,
  SMB_SENSOR_GB_TEMP8,
  SMB_SENSOR_GB_TEMP9,
  SMB_SENSOR_GB_TEMP10,
  SMB_SENSOR_GB_HBM_TEMP1,
  SMB_SENSOR_GB_HBM_TEMP2,
};

const uint8_t psu1_sensor_list[] = {
  PSU1_SENSOR_IN_VOLT,
  PSU1_SENSOR_12V_VOLT,
  PSU1_SENSOR_STBY_VOLT,
  PSU1_SENSOR_IN_CURR,
  PSU1_SENSOR_12V_CURR,
  PSU1_SENSOR_STBY_CURR,
  PSU1_SENSOR_IN_POWER,
  PSU1_SENSOR_12V_POWER,
  PSU1_SENSOR_STBY_POWER,
  PSU1_SENSOR_FAN_TACH,
  PSU1_SENSOR_TEMP1,
  PSU1_SENSOR_TEMP2,
  PSU1_SENSOR_TEMP3,
};

const uint8_t psu2_sensor_list[] = {
  PSU2_SENSOR_IN_VOLT,
  PSU2_SENSOR_12V_VOLT,
  PSU2_SENSOR_STBY_VOLT,
  PSU2_SENSOR_IN_CURR,
  PSU2_SENSOR_12V_CURR,
  PSU2_SENSOR_STBY_CURR,
  PSU2_SENSOR_IN_POWER,
  PSU2_SENSOR_12V_POWER,
  PSU2_SENSOR_STBY_POWER,
  PSU2_SENSOR_FAN_TACH,
  PSU2_SENSOR_TEMP1,
  PSU2_SENSOR_TEMP2,
  PSU2_SENSOR_TEMP3,
};


float scm_sensor_threshold[MAX_SENSOR_NUM][MAX_SENSOR_THRESHOLD + 1] = {0};
float smb_sensor_threshold[MAX_SENSOR_NUM][MAX_SENSOR_THRESHOLD + 1] = {0};
float psu_sensor_threshold[MAX_SENSOR_NUM][MAX_SENSOR_THRESHOLD + 1] = {0};

size_t bic_discrete_cnt = sizeof(bic_discrete_list)/sizeof(uint8_t);
size_t scm_sensor_cnt = sizeof(scm_sensor_list)/sizeof(uint8_t);
size_t scm_all_sensor_cnt = sizeof(scm_all_sensor_list)/sizeof(uint8_t);
size_t smb_sensor_cnt = sizeof(smb_sensor_list)/sizeof(uint8_t);
size_t psu1_sensor_cnt = sizeof(psu1_sensor_list)/sizeof(uint8_t);
size_t psu2_sensor_cnt = sizeof(psu2_sensor_list)/sizeof(uint8_t);

static sensor_info_t g_sinfo[MAX_NUM_FRUS][MAX_SENSOR_NUM] = {0};

int pal_get_sensor_util_timeout(uint8_t fru) {
  size_t cnt = 0;
  switch (fru) {
  case FRU_SCM:
    cnt = scm_all_sensor_cnt;
    break;
  case FRU_SMB:
    cnt = smb_sensor_cnt;
    break;
  case FRU_PSU1:
    cnt = psu1_sensor_cnt;
    break;
  case FRU_PSU2:
    cnt = psu2_sensor_cnt;
    break;
  default:
    if (fru > MAX_NUM_FRUS)
    cnt = 5;
    break;
  }
  return (READ_UNIT_SENSOR_TIMEOUT * cnt);
}

int pal_get_fru_sensor_list(uint8_t fru, uint8_t **sensor_list, int *cnt) {
  switch (fru) {
  case FRU_SCM:
    *sensor_list = (uint8_t *) scm_all_sensor_list;
    *cnt = scm_all_sensor_cnt;
    break;
  case FRU_SMB:
    *sensor_list = (uint8_t *) smb_sensor_list;
    *cnt = smb_sensor_cnt;
    break;
  case FRU_PSU1:
    *sensor_list = (uint8_t *) psu1_sensor_list;
    *cnt = psu1_sensor_cnt;
    break;
  case FRU_PSU2:
    *sensor_list = (uint8_t *) psu2_sensor_list;
    *cnt = psu2_sensor_cnt;
    break;
  default:
    if (fru > MAX_NUM_FRUS)
      return -1;
    // Nothing to read yet.
    *sensor_list = NULL;
    *cnt = 0;
  }
  return 0;
}

int pal_get_fru_discrete_list(uint8_t fru, uint8_t **sensor_list, int *cnt) {
  switch (fru) {
  case FRU_SCM:
    *sensor_list = (uint8_t *) bic_discrete_list;
    *cnt = bic_discrete_cnt;
    break;
  default:
    if (fru > MAX_NUM_FRUS)
      return -1;
    // Nothing to read yet.
    *sensor_list = NULL;
    *cnt = 0;
  }
    return 0;
}

static void _print_sensor_discrete_log(uint8_t fru, uint8_t snr_num, char *snr_name,
                                       uint8_t val, char *event) {
  if (val) {
    OBMC_CRIT("ASSERT: %s discrete - raised - FRU: %d, num: 0x%X,"
        " snr: %-16s val: %d", event, fru, snr_num, snr_name, val);
  } else {
    OBMC_CRIT("DEASSERT: %s discrete - settled - FRU: %d, num: 0x%X,"
        " snr: %-16s val: %d", event, fru, snr_num, snr_name, val);
  }
  pal_update_ts_sled();
}

int pal_sensor_discrete_check(uint8_t fru, uint8_t snr_num, char *snr_name,
    uint8_t o_val, uint8_t n_val) {
  char name[32], crisel[128];
  bool valid = false;
  uint8_t diff = o_val ^ n_val;

  if (GETBIT(diff, 0)) {
    switch (snr_num) {
    case BIC_SENSOR_SYSTEM_STATUS:
      sprintf(name, "SOC_Thermal_Trip");
      valid = true;
      sprintf(crisel, "%s - %s,FRU:%u",
                      name, GETBIT(n_val, 0)?"ASSERT":"DEASSERT", fru);
      pal_add_cri_sel(crisel);
      break;
    case BIC_SENSOR_VR_HOT:
      sprintf(name, "SOC_VR_Hot");
      valid = true;
      break;
    }
    if (valid) {
      _print_sensor_discrete_log(fru, snr_num, snr_name, GETBIT(n_val, 0), name);
      valid = false;
    }
  }

  if (GETBIT(diff, 1)) {
    switch (snr_num) {
    case BIC_SENSOR_SYSTEM_STATUS:
      sprintf(name, "SOC_FIVR_Fault");
      valid = true;
      break;
    case BIC_SENSOR_VR_HOT:
      sprintf(name, "SOC_DIMM_AB_VR_Hot");
      valid = true;
      break;
    case BIC_SENSOR_CPU_DIMM_HOT:
      sprintf(name, "SOC_MEMHOT");
      valid = true;
      break;
    }
    if (valid) {
      _print_sensor_discrete_log(fru, snr_num, snr_name, GETBIT(n_val, 1), name);
      valid = false;
    }
  }

  if (GETBIT(diff, 2)) {
    switch (snr_num) {
    case BIC_SENSOR_SYSTEM_STATUS:
      sprintf(name, "SYS_Throttle");
      valid = true;
      break;
    case BIC_SENSOR_VR_HOT:
      sprintf(name, "SOC_DIMM_DE_VR_Hot");
      valid = true;
      break;
    }
    if (valid) {
      _print_sensor_discrete_log(fru, snr_num, snr_name, GETBIT(n_val, 2), name);
      valid = false;
    }
  }

  if (GETBIT(diff, 4)) {
    if (snr_num == BIC_SENSOR_PROC_FAIL) {
      _print_sensor_discrete_log(fru, snr_num, snr_name, GETBIT(n_val, 4), "FRB3");
    }
  }

  return 0;
}

static int get_current_dir(const char *device, char *dir_name) {
  char cmd[LARGEST_DEVICE_NAME + 1];
  FILE *fp;
  int ret=-1;
  int size;

  // Get current working directory
  snprintf(cmd, sizeof(cmd), "cd %s;pwd", device);

  fp = popen(cmd, "r");
  if (NULL == fp)
     return -1;
  if (fgets(dir_name, LARGEST_DEVICE_NAME, fp) == NULL) {
    pclose(fp);
    return -1;
  }

  ret = pclose(fp);
  if (-1 == ret)
     OBMC_ERROR(-1, "%s pclose() fail ", __func__);

  // Remove the newline character at the end
  size = strlen(dir_name);
  dir_name[size-1] = '\0';

  return 0;
}

static int read_attr_integer(const char *device, const char *attr, int *value) {
  char full_name[LARGEST_DEVICE_NAME * 2];
  char dir_name[LARGEST_DEVICE_NAME + 1];

  // Get current working directory
  if (get_current_dir(device, dir_name)) {
    return -1;
  }

  snprintf(full_name, sizeof(full_name), "%s/%s", dir_name, attr);

  if (device_read(full_name, value)) {
    return -1;
  }

  return 0;
}

static int read_attr(const char *device, const char *attr, float *value) {
  char full_name[LARGEST_DEVICE_NAME * 2];
  char dir_name[LARGEST_DEVICE_NAME + 1];
  int tmp;

  // Get current working directory
  if (get_current_dir(device, dir_name)) {
    return -1;
  }

  snprintf(full_name, sizeof(full_name), "%s/%s", dir_name, attr);

  if (device_read(full_name, &tmp)) {
    return -1;
  }

  *value = ((float)tmp)/UNIT_DIV;

  return 0;
}

static int read_hsc_attr(const char *device, const char* attr,
                         float r_sense, float *value) {
  char full_dir_name[LARGEST_DEVICE_NAME * 2];
  char dir_name[LARGEST_DEVICE_NAME + 1];
  int tmp;

  // Get current working directory
  if (get_current_dir(device, dir_name))
  {
    return -1;
  }
  snprintf(full_dir_name, sizeof(full_dir_name), "%s/%s", dir_name, attr);

  if (device_read(full_dir_name, &tmp)) {
    return -1;
  }

  *value = ((float)tmp)/r_sense/UNIT_DIV;

  return 0;
}

static int read_hsc_volt_1(const char *device, float r_sense, float *value) {
  return read_hsc_attr(device, VOLT(1), r_sense, value);
}

static int read_hsc_volt_2(const char *device, float r_sense, float *value) {
  return read_hsc_attr(device, VOLT(2), r_sense, value);
}

static int read_hsc_curr(const char *device, float r_sense, float *value) {
  return read_hsc_attr(device, CURR(1), r_sense, value);
}

static int read_hsc_power(const char *device, float r_sense, float *value) {
  return read_hsc_attr(device, POWER(1), 1000, value);
}

static int read_fan_rpm_f(const char *device, uint8_t fan, float *value) {
  char full_name[LARGEST_DEVICE_NAME * 2];
  char dir_name[LARGEST_DEVICE_NAME + 1];
  char device_name[11];
  int tmp;

  /* Get current working directory */
  if (get_current_dir(device, dir_name)) {
    return -1;
  }

  snprintf(device_name, sizeof(device_name), "fan%d_input", fan);
  snprintf(full_name, sizeof(full_name), "%s/%s", dir_name, device_name);
  if (device_read(full_name, &tmp)) {
    return -1;
  }

  *value = (float)tmp;

  return 0;
}

static int read_fan_rpm(const char *device, uint8_t fan, int *value) {
  char full_name[LARGEST_DEVICE_NAME * 2];
  char dir_name[LARGEST_DEVICE_NAME + 1];
  char device_name[11];
  int tmp;

  /* Get current working directory */
  if (get_current_dir(device, dir_name)) {
    return -1;
  }

  snprintf(device_name, sizeof(device_name), "fan%d_input", fan);
  snprintf(full_name, sizeof(full_name), "%s/%s", dir_name, device_name);
  if (device_read(full_name, &tmp)) {
    return -1;
  }

  *value = tmp;

  return 0;
}

int pal_get_fan_speed(uint8_t fan, int *rpm) {
  if (fan >= MAX_NUM_FAN * 2) {
    OBMC_INFO("get_fan_speed: invalid fan#:%d", fan);
    return -1;
  }

  return read_fan_rpm(SMB_FCM_TACH_DEVICE, (fan + 1), rpm);
}

static int bic_sensor_sdr_path(uint8_t fru, char *path) {
  char fru_name[16];

  switch (fru) {
    case FRU_SCM:
      snprintf(fru_name, sizeof(fru_name), "%s", "scm");
      break;
    default:
      PAL_DEBUG("bic_sensor_sdr_path: Wrong Slot ID\n");
      return -1;
  }

  sprintf(path, SCM_BIC_SDR_PATH, fru_name);

  if (access(path, F_OK) == -1) {
    return -1;
  }

  return 0;
}

/* Populates all sensor_info_t struct using the path to SDR dump */
static int sdr_init(char *path, sensor_info_t *sinfo) {
  int fd;
  int ret = 0;
  uint8_t buf[MAX_SDR_LEN] = {0};
  uint8_t bytes_rd = 0;
  uint8_t snr_num = 0;
  sdr_full_t *sdr;

  while (access(path, F_OK) == -1) {
    sleep(5);
  }

  fd = open(path, O_RDONLY);
  if (fd < 0) {
    OBMC_ERROR(fd, "%s: open failed for %s\n", __func__, path);
    return -1;
  }

  ret = pal_flock_retry(fd);
  if (ret == -1) {
   OBMC_WARN("%s: failed to flock on %s", __func__, path);
   close(fd);
   return -1;
  }

  while ((bytes_rd = read(fd, buf, sizeof(sdr_full_t))) > 0) {
    if (bytes_rd != sizeof(sdr_full_t)) {
      OBMC_ERROR(bytes_rd, "%s: read returns %d bytes\n", __func__, bytes_rd);
      pal_unflock_retry(fd);
      close(fd);
      return -1;
    }

    sdr = (sdr_full_t *) buf;
    snr_num = sdr->sensor_num;
    sinfo[snr_num].valid = true;
    memcpy(&sinfo[snr_num].sdr, sdr, sizeof(sdr_full_t));
  }

  ret = pal_unflock_retry(fd);
  if (ret == -1) {
    OBMC_WARN("%s: failed to unflock on %s", __func__, path);
    close(fd);
    return -1;
  }

  close(fd);
  return 0;
}

static int bic_sensor_sdr_init(uint8_t fru, sensor_info_t *sinfo) {
  char path[64];
  int retry = 0;

  switch (fru) {
    case FRU_SCM:
      if (bic_sensor_sdr_path(fru, path) < 0) {
        OBMC_ERROR(-1, "bic_sensor_sdr_path: get_fru_sdr_path failed\n");
        return ERR_NOT_READY;
      }
      while (retry <= 3) {
        if (sdr_init(path, sinfo) < 0) {
          if (retry == 3) { //if the third retry still failed, return -1
            OBMC_ERROR(-1, "bic_sensor_sdr_init: sdr_init failed for FRU %d", fru);
            return -1;
          }
          retry++;
          sleep(1);
        } else {
          break;
        }
      }
      break;
  }

  return 0;
}

static int bic_sdr_init(uint8_t fru, bool reinit) {
  static bool init_done[MAX_NUM_FRUS] = {false};

  if (reinit) {
    init_done[fru - 1] = false;
  }
  if (!init_done[fru - 1]) {
    sensor_info_t *sinfo = g_sinfo[fru-1];

    if (bic_sensor_sdr_init(fru, sinfo) < 0)
      return ERR_NOT_READY;

    init_done[fru - 1] = true;
  }

  return 0;
}

/* Get the threshold values from the SDRs */
static int bic_get_sdr_thresh_val(uint8_t fru, uint8_t snr_num,
                                  uint8_t thresh, void *value) {
  int ret, retry = 0;
  int8_t b_exp, r_exp;
  uint8_t x, m_lsb, m_msb, b_lsb, b_msb, thresh_val;
  uint16_t m = 0, b = 0;
  sensor_info_t sinfo[MAX_SENSOR_NUM] = {0};
  sdr_full_t *sdr;

  while ((ret = bic_sensor_sdr_init(fru, sinfo)) == ERR_NOT_READY &&
    retry++ < MAX_RETRIES_SDR_INIT) {
    sleep(1);
  }
  if (ret < 0) {
    OBMC_WARN("bic_get_sdr_thresh_val: failed for fru: %d", fru);
    return -1;
  }
  sdr = &sinfo[snr_num].sdr;

  switch (thresh) {
  case UCR_THRESH:
    thresh_val = sdr->uc_thresh;
    break;
  case UNC_THRESH:
    thresh_val = sdr->unc_thresh;
    break;
  case UNR_THRESH:
    thresh_val = sdr->unr_thresh;
    break;
  case LCR_THRESH:
    thresh_val = sdr->lc_thresh;
    break;
  case LNC_THRESH:
    thresh_val = sdr->lnc_thresh;
    break;
  case LNR_THRESH:
    thresh_val = sdr->lnr_thresh;
    break;
  case POS_HYST:
    thresh_val = sdr->pos_hyst;
    break;
  case NEG_HYST:
    thresh_val = sdr->neg_hyst;
    break;
  default:
    PAL_DEBUG("bic_get_sdr_thresh_val: reading unknown threshold val");
    return -1;
  }

  // y = (mx + b * 10^b_exp) * 10^r_exp
  x = thresh_val;

  m_lsb = sdr->m_val;
  m_msb = sdr->m_tolerance >> 6;
  m = (m_msb << 8) | m_lsb;

  b_lsb = sdr->b_val;
  b_msb = sdr->b_accuracy >> 6;
  b = (b_msb << 8) | b_lsb;

  // exponents are 2's complement 4-bit number
  b_exp = sdr->rb_exp & 0xF;
  if (b_exp > 7) {
    b_exp = (~b_exp + 1) & 0xF;
    b_exp = -b_exp;
  }

  r_exp = (sdr->rb_exp >> 4) & 0xF;
  if (r_exp > 7) {
    r_exp = (~r_exp + 1) & 0xF;
    r_exp = -r_exp;
  }
  *(float *)value = ((m * x) + (b * pow(10, b_exp))) * (pow(10, r_exp));

  return 0;
}

static int bic_read_sensor_wrapper(uint8_t fru, uint8_t sensor_num,
                                   bool discrete, void *value) {
  int ret, i;
  ipmi_sensor_reading_t sensor;
  sdr_full_t *sdr;

  ret = bic_read_sensor(IPMB_BUS, sensor_num, &sensor);
  if (ret) {
    return ret;
  }

  if (sensor.flags & BIC_SENSOR_READ_NA) {
    PAL_DEBUG("bic_read_sensor_wrapper: Reading Not Available");
    PAL_DEBUG("bic_read_sensor_wrapper: sensor_num: 0x%X, flag: 0x%X",
               sensor_num, sensor.flags);
    return -1;
  }

  if (discrete) {
    *(float *) value = (float) sensor.status;
    return 0;
  }

  sdr = &g_sinfo[fru-1][sensor_num].sdr;

  // If the SDR is not type1, no need for conversion
  if (sdr->type !=1) {
    *(float *) value = sensor.value;
    return 0;
  }

  // y = (mx + b * 10^b_exp) * 10^r_exp
  uint8_t x;
  uint8_t m_lsb, m_msb;
  uint16_t m = 0;
  uint8_t b_lsb, b_msb;
  uint16_t b = 0;
  int8_t b_exp, r_exp;

  x = sensor.value;

  m_lsb = sdr->m_val;
  m_msb = sdr->m_tolerance >> 6;
  m = (m_msb << 8) | m_lsb;

  b_lsb = sdr->b_val;
  b_msb = sdr->b_accuracy >> 6;
  b = (b_msb << 8) | b_lsb;

  // exponents are 2's complement 4-bit number
  b_exp = sdr->rb_exp & 0xF;
  if (b_exp > 7) {
    b_exp = (~b_exp + 1) & 0xF;
    b_exp = -b_exp;
  }
  r_exp = (sdr->rb_exp >> 4) & 0xF;
  if (r_exp > 7) {
    r_exp = (~r_exp + 1) & 0xF;
    r_exp = -r_exp;
  }

  * (float *) value = ((m * x) + (b * pow(10, b_exp))) * (pow(10, r_exp));

  if ((sensor_num == BIC_SENSOR_SOC_THERM_MARGIN) && (* (float *) value > 0)) {
   * (float *) value -= (float) THERMAL_CONSTANT;
  }

  if (*(float *) value > MAX_POS_READING_MARGIN) {     //Negative reading handle
    for (i = 0; i < sizeof(bic_neg_reading_sensor_support_list) / sizeof(uint8_t); i++) {
      if (sensor_num == bic_neg_reading_sensor_support_list[i]) {
        * (float *) value -= (float) THERMAL_CONSTANT;
      }
    }
  }

  return 0;
}


static int scm_sensor_read(uint8_t sensor_num, float *value) {
  int ret = -1;
  int i = 0;
  int j = 0;
  bool discrete = false;
  bool scm_sensor = false;

  while (i < scm_sensor_cnt) {
    if (sensor_num == scm_sensor_list[i++]) {
      scm_sensor = true;
      break;
    }
  }
  if (scm_sensor) {
    switch (sensor_num) {
      case SCM_SENSOR_OUTLET_TEMP:
        ret = read_attr(SCM_OUTLET_TEMP_DEVICE, TEMP(1), value);
        break;
      case SCM_SENSOR_INLET_TEMP:
        ret = read_attr(SCM_INLET_TEMP_DEVICE, TEMP(1), value);
        break;
      case SCM_SENSOR_HSC_OUT_CURR:
        ret = read_hsc_curr(SCM_HSC_DEVICE, SCM_RSENSE, value);
        *value = *value * 1.0036 - 0.1189;
        if (*value < 0)
          *value = 0;
        break;
      case SCM_SENSOR_HSC_OUT_VOLT:
        ret = read_hsc_volt_2(SCM_HSC_DEVICE, 1, value);
        break;
      default:
        ret = READING_NA;
        break;
    }
  } else if (!scm_sensor && is_server_on()) {
    ret = bic_sdr_init(FRU_SCM, false);
    if (ret < 0) {
      PAL_DEBUG("bic_sdr_init fail\n");
      return ret;
    }

    while (j < bic_discrete_cnt) {
      if (sensor_num == bic_discrete_list[j++]) {
        discrete = true;
        break;
      }
    }

    if (!g_sinfo[FRU_SCM - 1][sensor_num].valid) {
      ret = bic_sdr_init(FRU_SCM, true); //reinit g_sinfo
      if (!g_sinfo[FRU_SCM - 1][sensor_num].valid) {
        return READING_NA;
      }
    }

    ret = bic_read_sensor_wrapper(FRU_SCM, sensor_num, discrete, value);
  } else {
    ret = READING_NA;
  }
  return ret;
}

static int cor_gb_volt(uint8_t board_type) {
  /*
   * Currently, skip to set vdd core with sysfs nodes because sysfs node exposed
   * has no write permission. Laterly we will fix this.
   */
  return 0;
}

static int smb_sensor_read(uint8_t sensor_num, float *value) {
  int ret = -1, th3_ret = -1;
  static uint8_t bootup_check = 0;
  char path[32];
  int num = 0;
  float total = 0;
  float read_val = 0;

  switch (sensor_num) {
    case SMB_SENSOR_VDDA_TEMP1:
      ret = read_attr(SMB_VDDA_DEVICE, TEMP(1), value);
      break;
    case SMB_SENSOR_PCIE_TEMP1:
      ret = read_attr(SMB_PCIE_DEVICE, TEMP(1), value);
      break;
    case SMB_SENSOR_IR3R3V_RIGHT_TEMP:
      ret = read_attr(SMB_PCIE_DEVICE, TEMP(2), value);
      break;
    case SMB_SENSOR_IR3R3V_LEFT_TEMP:
      ret = read_attr(SMB_VDDA_DEVICE, TEMP(2), value);
      break;
    case SMB_SENSOR_SW_CORE_TEMP1:
      ret = read_attr(SMB_XPDE_DEVICE, TEMP(1), value);
      break;
    case SMB_SENSOR_XPDE_HBM_TEMP1:
      ret = read_attr(SMB_XPDE_DEVICE, TEMP(2), value);
      break;
    case SMB_SENSOR_LM75B_U28_TEMP:
      ret = read_attr(SMB_LM75B_U28_DEVICE, TEMP(1), value);
      break;
    case SMB_SENSOR_LM75B_U25_TEMP:
      ret = read_attr(SMB_LM75B_U25_DEVICE, TEMP(1), value);
      break;
    case SMB_SENSOR_LM75B_U56_TEMP:
      ret = read_attr(SMB_LM75B_U56_DEVICE, TEMP(1), value);
      break;
    case SMB_SENSOR_LM75B_U55_TEMP:
      ret = read_attr(SMB_LM75B_U55_DEVICE, TEMP(1), value);
      break;
    case SMB_SENSOR_LM75B_U2_TEMP:
      ret = read_attr(SMB_LM75B_U2_DEVICE, TEMP(1), value);
      break;
    case SMB_SENSOR_LM75B_U13_TEMP:
      ret = read_attr(SMB_LM75B_U13_DEVICE, TEMP(1), value);
      break;
    case SMB_SENSOR_TMP421_U62_TEMP:
      ret = read_attr(SMB_TMP421_U62_DEVICE, TEMP(1), value);
      break;
    case SMB_SENSOR_TMP421_U63_TEMP:
      ret = read_attr(SMB_TMP421_U63_DEVICE, TEMP(1), value);
      break;
    case SMB_SENSOR_BMC_LM75B_TEMP:
      ret = read_attr(SMB_LM75B_BMC_DEVICE, TEMP(1), value);
      break;
    case SMB_SENSOR_GB_HIGH_TEMP:
      *value = 0;
      for (int id = SMB_SENSOR_GB_TEMP1; id <= SMB_SENSOR_GB_TEMP10; id++) {
        snprintf(path, sizeof(path), "temp%d_input", id - SMB_SENSOR_GB_TEMP1 + 1);
        ret = read_attr(SMB_GB_TEMP_DEVICE, path, &read_val);
        if (ret) {
          continue;
        } else {
          num++;
        }
        if (read_val > *value) {
          *value = read_val;
        }
      }
      if (num) {
        ret = 0;
      } else {
        ret = READING_NA;
      }
      break;
    case SMB_SENSOR_GB_TEMP1:
      ret = read_attr(SMB_GB_TEMP_DEVICE, TEMP(1), value);
      break;
    case SMB_SENSOR_GB_TEMP2:
      ret = read_attr(SMB_GB_TEMP_DEVICE, TEMP(2), value);
      break;
    case SMB_SENSOR_GB_TEMP3:
      ret = read_attr(SMB_GB_TEMP_DEVICE, TEMP(3), value);
      break;
    case SMB_SENSOR_GB_TEMP4:
      ret = read_attr(SMB_GB_TEMP_DEVICE, TEMP(4), value);
      break;
    case SMB_SENSOR_GB_TEMP5:
      ret = read_attr(SMB_GB_TEMP_DEVICE, TEMP(5), value);
      break;
    case SMB_SENSOR_GB_TEMP6:
      ret = read_attr(SMB_GB_TEMP_DEVICE, TEMP(6), value);
      break;
    case SMB_SENSOR_GB_TEMP7:
      ret = read_attr(SMB_GB_TEMP_DEVICE, TEMP(7), value);
      break;
    case SMB_SENSOR_GB_TEMP8:
      ret = read_attr(SMB_GB_TEMP_DEVICE, TEMP(8), value);
      break;
    case SMB_SENSOR_GB_TEMP9:
      ret = read_attr(SMB_GB_TEMP_DEVICE, TEMP(9), value);
      break;
    case SMB_SENSOR_GB_TEMP10:
      ret = read_attr(SMB_GB_TEMP_DEVICE, TEMP(10), value);
      break;
    case SMB_SENSOR_GB_HBM_TEMP1:
      ret = read_attr(SMB_GB_TEMP_DEVICE, TEMP(11), value);
      break;
    case SMB_SENSOR_GB_HBM_TEMP2:
      ret = read_attr(SMB_GB_TEMP_DEVICE, TEMP(12), value);
      break;
    case SMB_DOM1_MAX_TEMP:
      ret = read_attr(SMB_DOM1_DEVICE, TEMP(1), value);
      break;
    case SMB_DOM2_MAX_TEMP:
      ret = read_attr(SMB_DOM2_DEVICE, TEMP(1), value);
      break;
    case SMB_SENSOR_FCM_LM75B_U1_TEMP:
      ret = read_attr(SMB_FCM_LM75B_U1_DEVICE, TEMP(1), value);
      break;
    case SMB_SENSOR_FCM_LM75B_U2_TEMP:
      ret = read_attr(SMB_FCM_LM75B_U2_DEVICE, TEMP(1), value);
      break;
    case SMB_SENSOR_1220_VMON1:
      ret = read_attr(SMB_1220_DEVICE, VOLT(0), value);
      *value *= 4.3;
      break;
    case SMB_SENSOR_1220_VMON2:
      ret = read_attr(SMB_1220_DEVICE, VOLT(1), value);
      break;
    case SMB_SENSOR_1220_VMON3:
      ret = read_attr(SMB_1220_DEVICE, VOLT(2), value);
      break;
    case SMB_SENSOR_1220_VMON4:
      ret = read_attr(SMB_1220_DEVICE, VOLT(3), value);
      break;
    case SMB_SENSOR_1220_VMON5:
      ret = read_attr(SMB_1220_DEVICE, VOLT(4), value);
      break;
    case SMB_SENSOR_1220_VMON6:
      ret = read_attr(SMB_1220_DEVICE, VOLT(5), value);
      break;
    case SMB_SENSOR_1220_VMON7:
      ret = read_attr(SMB_1220_DEVICE, VOLT(6), value);
      break;
    case SMB_SENSOR_1220_VMON8:
      ret = read_attr(SMB_1220_DEVICE, VOLT(7), value);
      break;
    case SMB_SENSOR_1220_VMON9:
      ret = read_attr(SMB_1220_DEVICE, VOLT(8), value);
      break;
    case SMB_SENSOR_1220_VMON10:
      ret = read_attr(SMB_1220_DEVICE, VOLT(9), value);
      break;
    case SMB_SENSOR_1220_VMON11:
      ret = read_attr(SMB_1220_DEVICE, VOLT(10), value);
      break;
    case SMB_SENSOR_1220_VMON12:
      ret = read_attr(SMB_1220_DEVICE, VOLT(11), value);
      break;
    case SMB_SENSOR_1220_VCCA:
      ret = read_attr(SMB_1220_DEVICE, VOLT(12), value);
      break;
    case SMB_SENSOR_1220_VCCINP:
      ret = read_attr(SMB_1220_DEVICE, VOLT(13), value);
      break;
    case SMB_SENSOR_VDDA_IN_VOLT:
      ret = read_attr(SMB_VDDA_DEVICE, VOLT(1), value);
      break;
    case SMB_SENSOR_PCIE_IN_VOLT:
      ret = read_attr(SMB_PCIE_DEVICE, VOLT(1), value);
      break;
    case SMB_SENSOR_VDDA_OUT_VOLT:
      ret = read_attr(SMB_VDDA_DEVICE, VOLT(3), value);
      break;
    case SMB_SENSOR_PCIE_OUT_VOLT:
      ret = read_attr(SMB_PCIE_DEVICE, VOLT(3), value);
      break;
    case SMB_SENSOR_IR3R3V_RIGHT_IN_VOLT:
      ret = read_attr(SMB_PCIE_DEVICE, VOLT(2), value);
      break;
    case SMB_SENSOR_IR3R3V_LEFT_IN_VOLT:
      ret = read_attr(SMB_VDDA_DEVICE, VOLT(2), value);
      break;
    case SMB_SENSOR_IR3R3V_RIGHT_OUT_VOLT:
      ret = read_attr(SMB_PCIE_DEVICE, VOLT(4), value);
      *value *= 2;
      break;
    case SMB_SENSOR_IR3R3V_LEFT_OUT_VOLT:
      ret = read_attr(SMB_VDDA_DEVICE, VOLT(4), value);
      *value *= 2;
      break;
    case SMB_SENSOR_SW_CORE_IN_VOLT:
      ret = read_attr(SMB_XPDE_DEVICE, VOLT(1), value);
      break;
    case SMB_SENSOR_SW_CORE_OUT_VOLT:
      ret = read_attr(SMB_XPDE_DEVICE, VOLT(3), value);
      break;
    case SMB_SENSOR_XPDE_HBM_IN_VOLT:
      ret = read_attr(SMB_XPDE_DEVICE, VOLT(2), value);
      break;
    case SMB_SENSOR_XPDE_HBM_OUT_VOLT:
      ret = read_attr(SMB_XPDE_DEVICE, VOLT(4), value);
      break;
    case SMB_SENSOR_FCM_HSC_IN_VOLT:
      ret = read_hsc_volt_1(SMB_FCM_HSC_DEVICE, 1, value);
      break;
    case SMB_SENSOR_FCM_HSC_OUT_VOLT:
      ret = read_hsc_volt_2(SMB_FCM_HSC_DEVICE, 1, value);
      break;
    case SMB_SENSOR_VDDA_IN_CURR:
      ret = read_attr(SMB_VDDA_DEVICE, CURR(1), value);
      break;
    case SMB_SENSOR_PCIE_IN_CURR:
      ret = read_attr(SMB_PCIE_DEVICE, CURR(1), value);
      break;
    case SMB_SENSOR_VDDA_OUT_CURR:
      ret = read_attr(SMB_VDDA_DEVICE, CURR(3), value);
      *value = *value * 1.0433 + 0.3926;
      break;
    case SMB_SENSOR_PCIE_OUT_CURR:
      ret = read_attr(SMB_PCIE_DEVICE, CURR(3), value);
      *value = *value * 0.9994 + 1.0221;
      break;
    case SMB_SENSOR_IR3R3V_RIGHT_IN_CURR:
      ret = read_attr(SMB_PCIE_DEVICE, CURR(2), value);
      break;
    case SMB_SENSOR_IR3R3V_LEFT_IN_CURR:
      ret = read_attr(SMB_VDDA_DEVICE, CURR(2), value);
      break;
    case SMB_SENSOR_IR3R3V_RIGHT_OUT_CURR:
      ret = read_attr(SMB_PCIE_DEVICE, CURR(4), value);
      break;
    case SMB_SENSOR_IR3R3V_LEFT_OUT_CURR:
      ret = read_attr(SMB_VDDA_DEVICE, CURR(4), value);
      break;
    case SMB_SENSOR_SW_CORE_IN_CURR:
      ret = read_attr(SMB_XPDE_DEVICE, CURR(1), value);
      break;
    case SMB_SENSOR_SW_CORE_OUT_CURR:
      ret = read_attr(SMB_XPDE_DEVICE, CURR(3), value);
      break;
    case SMB_SENSOR_XPDE_HBM_IN_CURR:
      ret = read_attr(SMB_XPDE_DEVICE, CURR(2), value);
      break;
    case SMB_SENSOR_XPDE_HBM_OUT_CURR:
      ret = read_attr(SMB_XPDE_DEVICE, CURR(4), value);
      break;
    case SMB_SENSOR_FCM_HSC_OUT_CURR:
      ret = read_hsc_curr(SMB_FCM_HSC_DEVICE, 1, value);
      *value = *value * 4.4254 - 0.2048;
      if (*value < 0)
        *value = 0;
      break;
    case SMB_SENSOR_VDDA_IN_POWER:
      ret = read_attr(SMB_VDDA_DEVICE,  POWER(1), value);
      *value /= 1000;
      break;
    case SMB_SENSOR_PCIE_IN_POWER:
      ret = read_attr(SMB_PCIE_DEVICE,  POWER(1), value);
      *value /= 1000;
      break;
    case SMB_SENSOR_VDDA_OUT_POWER:
      ret = read_attr(SMB_VDDA_DEVICE,  POWER(3), value);
      *value /= 1000;
      break;
    case SMB_SENSOR_PCIE_OUT_POWER:
      ret = read_attr(SMB_PCIE_DEVICE,  POWER(3), value);
      *value /= 1000;
      break;
    case SMB_SENSOR_IR3R3V_RIGHT_IN_POWER:
      ret = read_attr(SMB_PCIE_DEVICE,  POWER(2), value);
      *value = *value / 1000;
      break;
    case SMB_SENSOR_IR3R3V_LEFT_IN_POWER:
      ret = read_attr(SMB_VDDA_DEVICE,  POWER(2), value);
      *value = *value / 1000;
      break;
    case SMB_SENSOR_IR3R3V_RIGHT_OUT_POWER:
      ret = read_attr(SMB_PCIE_DEVICE,  POWER(4), value);
      *value = *value / 1000 * 2;
      break;
    case SMB_SENSOR_IR3R3V_LEFT_OUT_POWER:
      ret = read_attr(SMB_VDDA_DEVICE,  POWER(4), value);
      *value = *value / 1000 * 2;
      break;
    case SMB_SENSOR_SW_CORE_IN_POWER:
      ret = read_attr(SMB_XPDE_DEVICE, POWER(1), value);
      *value /= 1000;
      break;
    case SMB_SENSOR_SW_CORE_OUT_POWER:
      ret = read_attr(SMB_XPDE_DEVICE, POWER(3), value);
      *value /= 1000;
      break;
    case SMB_SENSOR_XPDE_HBM_IN_POWER:
      ret = read_attr(SMB_XPDE_DEVICE, POWER(2), value);
      *value /= 1000;
      break;
    case SMB_SENSOR_XPDE_HBM_OUT_POWER:
      ret = read_attr(SMB_XPDE_DEVICE, POWER(4), value);
      *value /= 1000;
      break;
    case SMB_SENSOR_FCM_HSC_IN_POWER:
      ret = read_hsc_power(SMB_FCM_HSC_DEVICE, 1, value);
      *value /= 1000;
      break;
    case SMB_SENSOR_XDPE_LEFT_1_IN_VOLT:
      ret = read_attr(SMB_XPDE_LEFT_DEVICE, VOLT(1), value);
      break;
    case SMB_SENSOR_XDPE_LEFT_1_IN_CURR:
      ret = read_attr(SMB_XPDE_LEFT_DEVICE, CURR(1), value);
      break;
    case SMB_SENSOR_XDPE_LEFT_1_IN_POWER:
      ret = read_attr(SMB_XPDE_LEFT_DEVICE, POWER(1), value);
      *value /= 1000;
      break;
    case SMB_SENSOR_XDPE_LEFT_1_OUT_VOLT:
      ret = read_attr(SMB_XPDE_LEFT_DEVICE, VOLT(3), value);
      break;
    case SMB_SENSOR_XDPE_LEFT_1_OUT_CURR:
      ret = read_attr(SMB_XPDE_LEFT_DEVICE, CURR(3), value);
      break;
    case SMB_SENSOR_XDPE_LEFT_1_OUT_POWER:
      ret = read_attr(SMB_XPDE_LEFT_DEVICE, POWER(3), value);
      *value /= 1000;
      break;
    case SMB_SENSOR_XDPE_LEFT_1_TEMP:
      ret = read_attr(SMB_XPDE_LEFT_DEVICE, TEMP(1), value);
      break;
    case SMB_SENSOR_XDPE_LEFT_2_IN_VOLT:
      ret = read_attr(SMB_XPDE_LEFT_DEVICE, VOLT(2), value);
      break;
    case SMB_SENSOR_XDPE_LEFT_2_IN_CURR:
      ret = read_attr(SMB_XPDE_LEFT_DEVICE, CURR(2), value);
      break;
    case SMB_SENSOR_XDPE_LEFT_2_IN_POWER:
      ret = read_attr(SMB_XPDE_LEFT_DEVICE, POWER(2), value);
      *value /= 1000;
      break;
    case SMB_SENSOR_XDPE_LEFT_2_OUT_VOLT:
      ret = read_attr(SMB_XPDE_LEFT_DEVICE, VOLT(4), value);
      break;
    case SMB_SENSOR_XDPE_LEFT_2_OUT_CURR:
      ret = read_attr(SMB_XPDE_LEFT_DEVICE, CURR(4), value);
      break;
    case SMB_SENSOR_XDPE_LEFT_2_OUT_POWER:
      ret = read_attr(SMB_XPDE_LEFT_DEVICE, POWER(4), value);
      *value /= 1000;
      break;
    case SMB_SENSOR_XDPE_LEFT_2_TEMP:
      ret = read_attr(SMB_XPDE_LEFT_DEVICE, TEMP(2), value);
      break;
    case SMB_SENSOR_XDPE_RIGHT_1_IN_VOLT:
      ret = read_attr(SMB_XPDE_LEFT_DEVICE, VOLT(1), value);
      break;
    case SMB_SENSOR_XDPE_RIGHT_1_IN_CURR:
      ret = read_attr(SMB_XPDE_LEFT_DEVICE, CURR(1), value);
      break;
    case SMB_SENSOR_XDPE_RIGHT_1_IN_POWER:
      ret = read_attr(SMB_XPDE_LEFT_DEVICE, POWER(1), value);
      *value /= 1000;
      break;
    case SMB_SENSOR_XDPE_RIGHT_1_OUT_VOLT:
      ret = read_attr(SMB_XPDE_LEFT_DEVICE, VOLT(3), value);
      break;
    case SMB_SENSOR_XDPE_RIGHT_1_OUT_CURR:
      ret = read_attr(SMB_XPDE_LEFT_DEVICE, CURR(3), value);
      break;
    case SMB_SENSOR_XDPE_RIGHT_1_OUT_POWER:
      ret = read_attr(SMB_XPDE_LEFT_DEVICE, POWER(3), value);
      *value /= 1000;
      break;
    case SMB_SENSOR_XDPE_RIGHT_1_TEMP:
      ret = read_attr(SMB_XPDE_LEFT_DEVICE, TEMP(1), value);
      break;
    case SMB_SENSOR_XDPE_RIGHT_2_IN_VOLT:
      ret = read_attr(SMB_XPDE_LEFT_DEVICE, VOLT(2), value);
      break;
    case SMB_SENSOR_XDPE_RIGHT_2_IN_CURR:
      ret = read_attr(SMB_XPDE_LEFT_DEVICE, CURR(2), value);
      break;
    case SMB_SENSOR_XDPE_RIGHT_2_IN_POWER:
      ret = read_attr(SMB_XPDE_LEFT_DEVICE, POWER(2), value);
      *value /= 1000;
      break;
    case SMB_SENSOR_XDPE_RIGHT_2_OUT_VOLT:
      ret = read_attr(SMB_XPDE_LEFT_DEVICE, VOLT(4), value);
      break;
    case SMB_SENSOR_XDPE_RIGHT_2_OUT_CURR:
      ret = read_attr(SMB_XPDE_LEFT_DEVICE, CURR(4), value);
      break;
    case SMB_SENSOR_XDPE_RIGHT_2_OUT_POWER:
      ret = read_attr(SMB_XPDE_LEFT_DEVICE, POWER(4), value);
      *value /= 1000;
      break;
    case SMB_SENSOR_XDPE_RIGHT_2_TEMP:
      ret = read_attr(SMB_XPDE_LEFT_DEVICE, TEMP(2), value);
      break;
    case SMB_SENSOR_FAN1_FRONT_TACH:
      ret = read_fan_rpm_f(SMB_FCM_TACH_DEVICE, 1, value);
      break;
    case SMB_SENSOR_FAN1_REAR_TACH:
      ret = read_fan_rpm_f(SMB_FCM_TACH_DEVICE, 2, value);
      break;
    case SMB_SENSOR_FAN2_FRONT_TACH:
      ret = read_fan_rpm_f(SMB_FCM_TACH_DEVICE, 3, value);
      break;
    case SMB_SENSOR_FAN2_REAR_TACH:
      ret = read_fan_rpm_f(SMB_FCM_TACH_DEVICE, 4, value);
      break;
    case SMB_SENSOR_FAN3_FRONT_TACH:
      ret = read_fan_rpm_f(SMB_FCM_TACH_DEVICE, 5, value);
      break;
    case SMB_SENSOR_FAN3_REAR_TACH:
      ret = read_fan_rpm_f(SMB_FCM_TACH_DEVICE, 6, value);
      break;
    case SMB_SENSOR_FAN4_FRONT_TACH:
      ret = read_fan_rpm_f(SMB_FCM_TACH_DEVICE, 7, value);
      break;
    case SMB_SENSOR_FAN4_REAR_TACH:
      ret = read_fan_rpm_f(SMB_FCM_TACH_DEVICE, 8, value);
      break;
    case SMB_BMC_ADC0_VSEN:
      ret = read_attr(AST_ADC_DEVICE, VOLT(1), value);
      break;
    case SMB_BMC_ADC1_VSEN:
      ret = read_attr(AST_ADC_DEVICE, VOLT(2), value);
      break;
    case SMB_BMC_ADC2_VSEN:
      ret = read_attr(AST_ADC_DEVICE, VOLT(3), value);
      *value = *value * 3.2;
      break;
    case SMB_BMC_ADC3_VSEN:
      ret = read_attr(AST_ADC_DEVICE, VOLT(4), value);
      *value = *value * 3.2;
      break;
    case SMB_BMC_ADC4_VSEN:
      ret = read_attr(AST_ADC_DEVICE, VOLT(5), value);
      break;
    case SMB_BMC_ADC5_VSEN:
      ret = read_attr(AST_ADC_DEVICE, VOLT(6), value);
      *value = *value * 2;
      break;
    case SMB_BMC_ADC6_VSEN:
      ret = read_attr(AST_ADC_DEVICE, VOLT(7), value);
      break;
    case SMB_BMC_ADC7_VSEN:
      ret = read_attr(AST_ADC_DEVICE, VOLT(8), value);
      *value = *value * 2;
      break;
    case SMB_BMC_ADC8_VSEN:
      ret = read_attr(AST_ADC_DEVICE, VOLT(9), value);
      *value = *value * 2;
      break;
    case SMB_BMC_ADC9_VSEN:
      ret = read_attr(AST_ADC_DEVICE, VOLT(10), value);
      break;
    case SMB_BMC_ADC10_VSEN:
      ret = read_attr(AST_ADC_DEVICE, VOLT(11), value);
      break;
    case SMB_BMC_ADC11_VSEN:
      ret = read_attr(AST_ADC_DEVICE, VOLT(12), value);
      *value = *value * 2;
      break;
    case SMB_SENSOR_VDDCK_0_IN_VOLT:
      ret = read_attr(SMB_PXE1211_DEVICE, VOLT(1), value);
      break;
    case SMB_SENSOR_VDDCK_0_OUT_VOLT:
      ret = read_attr(SMB_PXE1211_DEVICE, VOLT(4), value);
      break;
    case SMB_SENSOR_VDDCK_0_IN_CURR:
      ret = read_attr(SMB_PXE1211_DEVICE, CURR(1), value);
      break;
    case SMB_SENSOR_VDDCK_0_OUT_CURR:
      ret = read_attr(SMB_PXE1211_DEVICE, CURR(4), value);
      break;
    case SMB_SENSOR_VDDCK_0_IN_POWER:
      ret = read_attr(SMB_PXE1211_DEVICE, POWER(1), value);
      *value /= 1000;
      break;
    case SMB_SENSOR_VDDCK_0_OUT_POWER:
      ret = read_attr(SMB_PXE1211_DEVICE, POWER(4), value);
      *value /= 1000;
      break;
    case SMB_SENSOR_VDDCK_0_TEMP:
      ret = read_attr(SMB_PXE1211_DEVICE, TEMP(1), value);
      break;
    case SMB_SENSOR_VDDCK_1_IN_VOLT:
      ret = read_attr(SMB_PXE1211_DEVICE, VOLT(2), value);
      break;
    case SMB_SENSOR_VDDCK_1_OUT_VOLT:
      ret = read_attr(SMB_PXE1211_DEVICE, VOLT(5), value);
      break;
    case SMB_SENSOR_VDDCK_1_IN_CURR:
      ret = read_attr(SMB_PXE1211_DEVICE, CURR(2), value);
      break;
    case SMB_SENSOR_VDDCK_1_OUT_CURR:
      ret = read_attr(SMB_PXE1211_DEVICE, CURR(5), value);
      break;
    case SMB_SENSOR_VDDCK_1_IN_POWER:
      ret = read_attr(SMB_PXE1211_DEVICE, POWER(2), value);
      *value /= 1000;
      break;
    case SMB_SENSOR_VDDCK_1_OUT_POWER:
      ret = read_attr(SMB_PXE1211_DEVICE, POWER(5), value);
      *value /= 1000;
      break;
    case SMB_SENSOR_VDDCK_1_TEMP:
      ret = read_attr(SMB_PXE1211_DEVICE, TEMP(2), value);
      break;
    case SMB_SENSOR_VDDCK_2_IN_VOLT:
      ret = read_attr(SMB_PXE1211_DEVICE, VOLT(3), value);
      break;
    case SMB_SENSOR_VDDCK_2_OUT_VOLT:
      ret = read_attr(SMB_PXE1211_DEVICE, VOLT(6), value);
      break;
    case SMB_SENSOR_VDDCK_2_IN_CURR:
      ret = read_attr(SMB_PXE1211_DEVICE, CURR(3), value);
      break;
    case SMB_SENSOR_VDDCK_2_OUT_CURR:
      ret = read_attr(SMB_PXE1211_DEVICE, CURR(6), value);
      break;
    case SMB_SENSOR_VDDCK_2_IN_POWER:
      ret = read_attr(SMB_PXE1211_DEVICE, POWER(3), value);
      *value /= 1000;
      break;
    case SMB_SENSOR_VDDCK_2_OUT_POWER:
      ret = read_attr(SMB_PXE1211_DEVICE, POWER(6), value);
      *value /= 1000;
      break;
    case SMB_SENSOR_VDDCK_2_TEMP:
      ret = read_attr(SMB_PXE1211_DEVICE, TEMP(3), value);
      break;
    default:
      ret = READING_NA;
      break;
  }
  return ret;
}

static int psu_sensor_read(uint8_t sensor_num, float *value) {
  int ret = -1;

  switch (sensor_num) {
    case PSU1_SENSOR_IN_VOLT:
      ret = read_attr(PSU1_DEVICE, VOLT(0), value);
      break;
    case PSU1_SENSOR_12V_VOLT:
      ret = read_attr(PSU1_DEVICE, VOLT(1), value);
      break;
    case PSU1_SENSOR_STBY_VOLT:
      ret = read_attr(PSU1_DEVICE, VOLT(2), value);
      break;
    case PSU1_SENSOR_IN_CURR:
      ret = read_attr(PSU1_DEVICE, CURR(1), value);
      break;
    case PSU1_SENSOR_12V_CURR:
      ret = read_attr(PSU1_DEVICE, CURR(2), value);
      break;
    case PSU1_SENSOR_STBY_CURR:
      ret = read_attr(PSU1_DEVICE, CURR(3), value);
      break;
    case PSU1_SENSOR_IN_POWER:
      ret = read_attr(PSU1_DEVICE, POWER(1), value);
      *value = *value / UNIT_DIV;
      break;
    case PSU1_SENSOR_12V_POWER:
      ret = read_attr(PSU1_DEVICE, POWER(2), value);
      *value = *value / UNIT_DIV;
      break;
    case PSU1_SENSOR_STBY_POWER:
      ret = read_attr(PSU1_DEVICE, POWER(3), value);
      *value = *value / UNIT_DIV;
      break;
    case PSU1_SENSOR_FAN_TACH:
      ret = read_fan_rpm_f(PSU1_DEVICE, 1, value);
      break;
    case PSU1_SENSOR_TEMP1:
      ret = read_attr(PSU1_DEVICE, TEMP(1), value);
      break;
    case PSU1_SENSOR_TEMP2:
      ret = read_attr(PSU1_DEVICE, TEMP(2), value);
      break;
    case PSU1_SENSOR_TEMP3:
      ret = read_attr(PSU1_DEVICE, TEMP(3), value);
      break;
    case PSU2_SENSOR_IN_VOLT:
      ret = read_attr(PSU2_DEVICE, VOLT(0), value);
      break;
    case PSU2_SENSOR_12V_VOLT:
      ret = read_attr(PSU2_DEVICE, VOLT(1), value);
      break;
    case PSU2_SENSOR_STBY_VOLT:
      ret = read_attr(PSU2_DEVICE, VOLT(2), value);
      break;
    case PSU2_SENSOR_IN_CURR:
      ret = read_attr(PSU2_DEVICE, CURR(1), value);
      break;
    case PSU2_SENSOR_12V_CURR:
      ret = read_attr(PSU2_DEVICE, CURR(2), value);
      break;
    case PSU2_SENSOR_STBY_CURR:
      ret = read_attr(PSU2_DEVICE, CURR(3), value);
      break;
    case PSU2_SENSOR_IN_POWER:
      ret = read_attr(PSU2_DEVICE, POWER(1), value);
      *value = *value / UNIT_DIV;
      break;
    case PSU2_SENSOR_12V_POWER:
      ret = read_attr(PSU2_DEVICE, POWER(2), value);
      *value = *value / UNIT_DIV;
      break;
    case PSU2_SENSOR_STBY_POWER:
      ret = read_attr(PSU2_DEVICE, POWER(3), value);
      *value = *value / UNIT_DIV;
      break;
    case PSU2_SENSOR_FAN_TACH:
      ret = read_fan_rpm_f(PSU2_DEVICE, 1, value);
      break;
    case PSU2_SENSOR_TEMP1:
      ret = read_attr(PSU2_DEVICE, TEMP(1), value);
      break;
    case PSU2_SENSOR_TEMP2:
      ret = read_attr(PSU2_DEVICE, TEMP(2), value);
      break;
    case PSU2_SENSOR_TEMP3:
      ret = read_attr(PSU2_DEVICE, TEMP(3), value);
      break;
    default:
      ret = READING_NA;
      break;
  }
  return ret;
}

int pal_sensor_read_raw(uint8_t fru, uint8_t sensor_num, void *value) {
  char key[MAX_KEY_LEN];
  char fru_name[32];
  int ret, delay = 500;
  uint8_t prsnt = 0;
  uint8_t status = 0;

  pal_get_fru_name(fru, fru_name);

  ret = pal_is_fru_prsnt(fru, &prsnt);
  if (ret) {
    return ret;
  }
  if (!prsnt) {
    PAL_DEBUG("pal_sensor_read_raw(): %s is not present\n", fru_name);
    return -1;
  }

  ret = pal_is_fru_ready(fru, &status);
  if (ret) {
    return ret;
  }
  if (!status) {
    PAL_DEBUG("pal_sensor_read_raw(): %s is not ready\n", fru_name);
    return -1;
  }

  sprintf(key, "%s_sensor%d", fru_name, sensor_num);
  switch (fru) {
    case FRU_SCM:
      ret = scm_sensor_read(sensor_num, value);
      if (sensor_num == SCM_SENSOR_INLET_TEMP) {
        delay = 100;
      }
      break;
    case FRU_SMB:
      ret = smb_sensor_read(sensor_num, value);
      if (sensor_num >= SMB_SENSOR_FAN1_FRONT_TACH &&
           sensor_num <= SMB_SENSOR_FAN4_REAR_TACH) {
        delay = 100;
      }
      break;
    case FRU_PSU1:
    case FRU_PSU2:
      ret = psu_sensor_read(sensor_num, value);
      break;
    default:
      return -1;
  }

  if (ret == READING_NA || ret == -1) {
    return READING_NA;
  }
  msleep(delay);

  return 0;
}

int pal_sensor_discrete_read_raw(uint8_t fru, uint8_t sensor_num, void *value) {
  char key[MAX_KEY_LEN];
  char fru_name[32];
  int ret , delay = 500;
  uint8_t prsnt = 0;
  uint8_t status = 0;

  pal_get_fru_name(fru, fru_name);

  ret = pal_is_fru_prsnt(fru, &prsnt);
  if (ret) {
    return ret;
  }
  if (!prsnt) {
    PAL_DEBUG("pal_sensor_discrete_read_raw(): %s is not present\n", fru_name);
    return -1;
  }

  ret = pal_is_fru_ready(fru, &status);
  if (ret) {
    return ret;
  }
  if (!status) {
    PAL_DEBUG("pal_sensor_discrete_read_raw(): %s is not ready\n", fru_name);
    return -1;
  }

  sprintf(key, "%s_sensor%d", fru_name, sensor_num);
  switch (fru) {
    default:
      return -1;
  }

  if (ret == READING_NA || ret == -1) {
    return READING_NA;
  }
  msleep(delay);

  return 0;
};

static int get_scm_sensor_name(uint8_t sensor_num, char *name) {
  switch (sensor_num) {
    case SCM_SENSOR_OUTLET_TEMP:
      sprintf(name, "SCM_OUTLET_TEMP");
      break;
    case SCM_SENSOR_INLET_TEMP:
      sprintf(name, "SCM_INLET_TEMP");
      break;
    case SCM_SENSOR_HSC_OUT_VOLT:
      sprintf(name, "SCM_HSC_OUTPUT_12V_VOLT");
      break;
    case SCM_SENSOR_HSC_OUT_CURR:
      sprintf(name, "SCM_HSC_OUTPUT_12V_CURR");
      break;
    case BIC_SENSOR_MB_OUTLET_TEMP:
      sprintf(name, "MB_OUTLET_TEMP");
      break;
    case BIC_SENSOR_MB_INLET_TEMP:
      sprintf(name, "MB_INLET_TEMP");
      break;
    case BIC_SENSOR_PCH_TEMP:
      sprintf(name, "PCH_TEMP");
      break;
    case BIC_SENSOR_VCCIN_VR_TEMP:
      sprintf(name, "VCCIN_VR_TEMP");
      break;
    case BIC_SENSOR_1V05COMB_VR_TEMP:
      sprintf(name, "1V05COMB_VR_TEMP");
      break;
    case BIC_SENSOR_SOC_TEMP:
      sprintf(name, "SOC_TEMP");
      break;
    case BIC_SENSOR_SOC_THERM_MARGIN:
      sprintf(name, "SOC_THERM_MARGIN_TEMP");
      break;
    case BIC_SENSOR_VDDR_VR_TEMP:
      sprintf(name, "VDDR_VR_TEMP");
      break;
    case BIC_SENSOR_SOC_DIMMA_TEMP:
      sprintf(name, "SOC_DIMMA_TEMP");
      break;
    case BIC_SENSOR_SOC_DIMMB_TEMP:
      sprintf(name, "SOC_DIMMB_TEMP");
      break;
    case BIC_SENSOR_SOC_PACKAGE_PWR:
      sprintf(name, "SOC_PACKAGE_POWER");
      break;
    case BIC_SENSOR_VCCIN_VR_POUT:
      sprintf(name, "VCCIN_VR_OUT_POWER");
      break;
      case BIC_SENSOR_VDDR_VR_POUT:
      sprintf(name, "VDDR_VR_OUT_POWER");
      break;
    case BIC_SENSOR_SOC_TJMAX:
      sprintf(name, "SOC_TJMAX_TEMP");
      break;
    case BIC_SENSOR_P3V3_MB:
      sprintf(name, "P3V3_MB_VOLT");
      break;
    case BIC_SENSOR_P12V_MB:
      sprintf(name, "P12V_MB_VOLT");
      break;
    case BIC_SENSOR_P1V05_PCH:
      sprintf(name, "P1V05_PCH_VOLT");
      break;
    case BIC_SENSOR_P3V3_STBY_MB:
      sprintf(name, "P3V3_STBY_MB_VOLT");
      break;
    case BIC_SENSOR_P5V_STBY_MB:
      sprintf(name, "P5V_STBY_MB_VOLT");
      break;
    case BIC_SENSOR_PV_BAT:
      sprintf(name, "PV_BAT_VOLT");
      break;
    case BIC_SENSOR_PVDDR:
      sprintf(name, "PVDDR_VOLT");
      break;
    case BIC_SENSOR_P1V05_COMB:
      sprintf(name, "P1V05_COMB_VOLT");
      break;
    case BIC_SENSOR_1V05COMB_VR_CURR:
      sprintf(name, "1V05COMB_VR_CURR");
      break;
    case BIC_SENSOR_VDDR_VR_CURR:
      sprintf(name, "VDDR_VR_CURR");
      break;
    case BIC_SENSOR_VCCIN_VR_CURR:
      sprintf(name, "VCCIN_VR_CURR");
      break;
    case BIC_SENSOR_VCCIN_VR_VOL:
      sprintf(name, "VCCIN_VR_VOLT");
      break;
    case BIC_SENSOR_VDDR_VR_VOL:
      sprintf(name, "VDDR_VR_VOLT");
      break;
    case BIC_SENSOR_P1V05COMB_VR_VOL:
      sprintf(name, "1V05COMB_VR_VOLT");
      break;
    case BIC_SENSOR_P1V05COMB_VR_POUT:
      sprintf(name, "1V05COMB_VR_OUT_POWER");
      break;
    case BIC_SENSOR_INA230_POWER:
      sprintf(name, "INA230_POWER");
      break;
    case BIC_SENSOR_INA230_VOL:
      sprintf(name, "INA230_VOLT");
      break;
    case BIC_SENSOR_SYSTEM_STATUS:
      sprintf(name, "SYSTEM_STATUS");
      break;
    case BIC_SENSOR_SYS_BOOT_STAT:
      sprintf(name, "SYS_BOOT_STAT");
      break;
    case BIC_SENSOR_CPU_DIMM_HOT:
      sprintf(name, "CPU_DIMM_HOT");
      break;
    case BIC_SENSOR_PROC_FAIL:
      sprintf(name, "PROC_FAIL");
      break;
    case BIC_SENSOR_VR_HOT:
      sprintf(name, "VR_HOT");
      break;
    default:
      return -1;
  }
  return 0;
}

static int get_smb_sensor_name(uint8_t sensor_num, char *name) {
  sprintf(name,SENSOR_NAME_ERR);
  switch (sensor_num) {
    case SMB_SENSOR_VDDA_TEMP1:
      sprintf(name, "SMB_XP0R94V_VDDA_TEMP");
      break;
    case SMB_SENSOR_PCIE_TEMP1:
      sprintf(name, "SMB_XP0R75V_PCIE_TEMP");
      break;
    case SMB_SENSOR_IR3R3V_LEFT_TEMP:
      sprintf(name, "SMB_XP3R3V_LEFT_TEMP");
      break;
    case SMB_SENSOR_IR3R3V_RIGHT_TEMP:
      sprintf(name, "SMB_XP3R3V_RIGHT_TEMP");
      break;
    case SMB_SENSOR_SW_CORE_TEMP1:
      sprintf(name, "SMB_VDD_CORE_TEMP");
      break;
    case SMB_SENSOR_XPDE_HBM_TEMP1:
      sprintf(name, "SMB_XP1R2V_HBM_TEMP");
      break;
    case SMB_SENSOR_LM75B_U28_TEMP:
      sprintf(name, "SMB_01_SYS_INLET_TOP_U28_TEMP");
      break;
    case SMB_SENSOR_LM75B_U25_TEMP:
      sprintf(name, "SMB_10_SWITCH1_U25_TEMP");
      break;
    case SMB_SENSOR_LM75B_U56_TEMP:
      sprintf(name, "SMB_06_FAN_INLET_U56_TEMP");
      break;
    case SMB_SENSOR_LM75B_U55_TEMP:
      sprintf(name, "SMB_04_MACSEC_OUTLET1_U55_TEMP");
      break;
    case SMB_SENSOR_LM75B_U2_TEMP:
      sprintf(name, "SMB_01_SYS_INLET_BTM_U2_TEMP");
      break;
    case SMB_SENSOR_LM75B_U13_TEMP:
      sprintf(name, "SMB_03_MACSEC_VR_INLET_U13_TEMP");
      break;
    case SMB_SENSOR_TMP421_U62_TEMP:
      sprintf(name, "SMB_05_MACSEC_OUTLET2_U62_TEMP");
      break;
    case SMB_SENSOR_TMP421_U63_TEMP:
      sprintf(name, "SMB_07_PSU_AMBIENT_U63_TEMP");
      break;
    case SMB_SENSOR_BMC_LM75B_TEMP:
      sprintf(name, "BMC_LM75B_TEMP");
      break;
    case SMB_SENSOR_GB_HIGH_TEMP:
      sprintf(name, "SMB_GB_HIGH_TEMP");
      break;
    case SMB_SENSOR_GB_TEMP1:
      sprintf(name, "SMB_GB_TEMP1");
      break;
    case SMB_SENSOR_GB_TEMP2:
      sprintf(name, "SMB_GB_TEMP2");
      break;
    case SMB_SENSOR_GB_TEMP3:
      sprintf(name, "SMB_GB_TEMP3");
      break;
    case SMB_SENSOR_GB_TEMP4:
      sprintf(name, "SMB_GB_TEMP4");
      break;
    case SMB_SENSOR_GB_TEMP5:
      sprintf(name, "SMB_GB_TEMP5");
      break;
    case SMB_SENSOR_GB_TEMP6:
      sprintf(name, "SMB_GB_TEMP6");
      break;
    case SMB_SENSOR_GB_TEMP7:
      sprintf(name, "SMB_GB_TEMP7");
      break;
    case SMB_SENSOR_GB_TEMP8:
      sprintf(name, "SMB_GB_TEMP8");
      break;
    case SMB_SENSOR_GB_TEMP9:
      sprintf(name, "SMB_GB_TEMP9");
      break;
    case SMB_SENSOR_GB_TEMP10:
      sprintf(name, "SMB_GB_TEMP10");
      break;
    case SMB_SENSOR_GB_HBM_TEMP1:
      sprintf(name, "SMB_GB_HBM_TEMP1");
      break;
    case SMB_SENSOR_GB_HBM_TEMP2:
      sprintf(name, "SMB_GB_HBM_TEMP2");
      break;
    case SMB_DOM1_MAX_TEMP:
      sprintf(name, "SMB_DOM_FPGA1_MAX_TEMP");
      break;
    case SMB_DOM2_MAX_TEMP:
      sprintf(name, "SMB_DOM_FPGA2_MAX_TEMP");
      break;
    case SMB_SENSOR_FCM_LM75B_U1_TEMP:
      sprintf(name, "FCM_LM75B_RIGHT_TEMP");
      break;
    case SMB_SENSOR_FCM_LM75B_U2_TEMP:
      sprintf(name, "FCM_LM75B_LEFT_TEMP");
      break;
    case SMB_SENSOR_1220_VMON1:
      sprintf(name, "SMB_XP12R0V(12V)");
      break;
    case SMB_SENSOR_1220_VMON2:
      sprintf(name, "SMB_XP5R0V(5V)");
      break;
    case SMB_SENSOR_1220_VMON3:
      sprintf(name, "SMB_XP3R3V_C(3.3V)");
      break;
    case SMB_SENSOR_1220_VMON4:
      sprintf(name, "SMB_XP3R3V_FPGA(3.3V)");
      break;
    case SMB_SENSOR_1220_VMON5:
      sprintf(name, "SMB_XP1R2V(1.2V)");
      break;
    case SMB_SENSOR_1220_VMON6:
      sprintf(name, "SMB_XP1R8V_FPGA(1.8V)");
      break;
    case SMB_SENSOR_1220_VMON7:
      sprintf(name, "SMB_XP1R8V_IO(1.8V)");
      break;
    case SMB_SENSOR_1220_VMON8:
      sprintf(name, "SMB_XP2R5V_HBM(2.5V)");
      break;
    case SMB_SENSOR_1220_VMON9:
      sprintf(name, "SMB_XP0R94V_VDDA(0.94V)");
      break;
    case SMB_SENSOR_1220_VMON10:
      sprintf(name, "SMB_VDD_CORE_GB(0.825V)");
      break;
    case SMB_SENSOR_1220_VMON11:
      sprintf(name, "SMB_XP0R75V_PCIE(0.75V)");
      break;
    case SMB_SENSOR_1220_VMON12:
      sprintf(name, "SMB_XP1R15V_VDDCK_2(1.15V)");
      break;
    case SMB_SENSOR_1220_VCCA:
      sprintf(name, "SMB_POWR1220_VCCA(3.3V)");
      break;
    case SMB_SENSOR_1220_VCCINP:
      sprintf(name, "SMB_POWR1220_VCCINP(3.3V)");
      break;
    case SMB_SENSOR_VDDA_IN_VOLT:
      sprintf(name, "SMB_XP0R94V_VDDA_IN_VOLT");
      break;
    case SMB_SENSOR_VDDA_OUT_VOLT:
      sprintf(name, "SMB_XP0R94V_VDDA_OUT_VOLT");
      break;
    case SMB_SENSOR_PCIE_IN_VOLT:
      sprintf(name, "SMB_XP0R75V_PCIE_IN_VOLT");
      break;
    case SMB_SENSOR_PCIE_OUT_VOLT:
      sprintf(name, "SMB_XP0R75V_PCIE_OUT_VOLT");
      break;
    case SMB_SENSOR_IR3R3V_LEFT_IN_VOLT:
      sprintf(name, "SMB_XP3R3V_LEFT_IN_VOLT");
      break;
    case SMB_SENSOR_IR3R3V_LEFT_OUT_VOLT:
      sprintf(name, "SMB_XP3R3V_LEFT_OUT_VOLT");
      break;
    case SMB_SENSOR_IR3R3V_RIGHT_IN_VOLT:
      sprintf(name, "SMB_XP3R3V_RIGHT_IN_VOLT");
      break;
    case SMB_SENSOR_IR3R3V_RIGHT_OUT_VOLT:
      sprintf(name, "SMB_XP3R3V_RIGHT_OUT_VOLT");
      break;
    case SMB_SENSOR_SW_CORE_IN_VOLT:
      sprintf(name, "SMB_VDD_CORE_IN_VOLT");
      break;
    case SMB_SENSOR_SW_CORE_OUT_VOLT:
      sprintf(name, "SMB_VDD_CORE_OUT_VOLT");
      break;
    case SMB_SENSOR_XPDE_HBM_IN_VOLT:
      sprintf(name, "SMB_XP1R2V_HBM_IN_VOLT");
      break;
    case SMB_SENSOR_XPDE_HBM_OUT_VOLT:
      sprintf(name, "SMB_XP1R2V_HBM_OUT_VOLT");
      break;
    case SMB_SENSOR_FCM_HSC_IN_VOLT:
      sprintf(name, "FCM_HSC_INPUT_12V_VOLT");
      break;
    case SMB_SENSOR_FCM_HSC_OUT_VOLT:
      sprintf(name, "FCM_HSC_OUTPUT_12V_VOLT");
      break;
    case SMB_SENSOR_VDDA_IN_CURR:
      sprintf(name, "SMB_XP0R94V_VDDA_IN_CURR");
      break;
    case SMB_SENSOR_VDDA_OUT_CURR:
      sprintf(name, "SMB_XP0R94V_VDDA_OUT_CURR");
      break;
    case SMB_SENSOR_PCIE_IN_CURR:
      sprintf(name, "SMB_XP0R75V_PCIE_IN_CURR");
      break;
    case SMB_SENSOR_PCIE_OUT_CURR:
      sprintf(name, "SMB_XP0R75V_PCIE_OUT_CURR");
      break;
    case SMB_SENSOR_IR3R3V_LEFT_IN_CURR:
      sprintf(name, "SMB_XP3R3V_LEFT_IN_CURR");
      break;
    case SMB_SENSOR_IR3R3V_LEFT_OUT_CURR:
      sprintf(name, "SMB_XP3R3V_LEFT_OUT_CURR");
      break;
    case SMB_SENSOR_IR3R3V_RIGHT_IN_CURR:
      sprintf(name, "SMB_XP3R3V_RIGHT_IN_CURR");
      break;
    case SMB_SENSOR_IR3R3V_RIGHT_OUT_CURR:
      sprintf(name, "SMB_XP3R3V_RIGHT_OUT_CURR");
      break;
    case SMB_SENSOR_SW_CORE_IN_CURR:
      sprintf(name, "SMB_VDD_CORE_IN_CURR");
      break;
    case SMB_SENSOR_SW_CORE_OUT_CURR:
      sprintf(name, "SMB_VDD_CORE_OUT_CURR");
      break;
    case SMB_SENSOR_XPDE_HBM_IN_CURR:
      sprintf(name, "SMB_XP1R2V_HBM_IN_CURR");
      break;
    case SMB_SENSOR_XPDE_HBM_OUT_CURR:
      sprintf(name, "SMB_XP1R2V_HBM_OUT_CURR");
      break;
    case SMB_SENSOR_FCM_HSC_OUT_CURR:
      sprintf(name, "FCM_HSC_OUTPUT_12V_CURR");
      break;
    case SMB_SENSOR_VDDA_IN_POWER:
      sprintf(name, "SMB_XP0R94V_VDDA_IN_POWER");
      break;
    case SMB_SENSOR_VDDA_OUT_POWER:
      sprintf(name, "SMB_XP0R94V_VDDA_OUT_POWER");
      break;
    case SMB_SENSOR_PCIE_IN_POWER:
      sprintf(name, "SMB_XP0R75V_PCIE_IN_POWER");
      break;
    case SMB_SENSOR_PCIE_OUT_POWER:
      sprintf(name, "SMB_XP0R75V_PCIE_OUT_POWER");
      break;
    case SMB_SENSOR_IR3R3V_LEFT_IN_POWER:
      sprintf(name, "SMB_XP3R3V_LEFT_IN_POWER");
      break;
    case SMB_SENSOR_IR3R3V_LEFT_OUT_POWER:
      sprintf(name, "SMB_XP3R3V_LEFT_OUT_POWER");
      break;
    case SMB_SENSOR_IR3R3V_RIGHT_IN_POWER:
      sprintf(name, "SMB_XP3R3V_RIGHT_IN_POWER");
      break;
    case SMB_SENSOR_IR3R3V_RIGHT_OUT_POWER:
      sprintf(name, "SMB_XP3R3V_RIGHT_OUT_POWER");
      break;
    case SMB_SENSOR_SW_CORE_IN_POWER:
      sprintf(name, "SMB_VDD_CORE_IN_POWER");
      break;
    case SMB_SENSOR_SW_CORE_OUT_POWER:
      sprintf(name, "SMB_VDD_CORE_OUT_POWER");
      break;
    case SMB_SENSOR_XPDE_HBM_IN_POWER:
      sprintf(name, "SMB_XP1R2V_HBM_IN_POWER");
      break;
    case SMB_SENSOR_XPDE_HBM_OUT_POWER:
      sprintf(name, "SMB_XP1R2V_HBM_OUT_POWER");
      break;
    case SMB_SENSOR_FCM_HSC_IN_POWER:
      sprintf(name, "FCM_HSC_INPUT_12V_POWER");
      break;
    case SMB_SENSOR_FAN1_FRONT_TACH:
      sprintf(name, "FAN1_FRONT_SPEED");
      break;
    case SMB_SENSOR_FAN1_REAR_TACH:
      sprintf(name, "FAN1_REAR_SPEED");
      break;
    case SMB_SENSOR_FAN2_FRONT_TACH:
      sprintf(name, "FAN2_FRONT_SPEED");
      break;
    case SMB_SENSOR_FAN2_REAR_TACH:
      sprintf(name, "FAN2_REAR_SPEED");
      break;
    case SMB_SENSOR_FAN3_FRONT_TACH:
      sprintf(name, "FAN3_FRONT_SPEED");
      break;
    case SMB_SENSOR_FAN3_REAR_TACH:
      sprintf(name, "FAN3_REAR_SPEED");
      break;
    case SMB_SENSOR_FAN4_FRONT_TACH:
      sprintf(name, "FAN4_FRONT_SPEED");
      break;
    case SMB_SENSOR_FAN4_REAR_TACH:
      sprintf(name, "FAN4_REAR_SPEED");
      break;
    case SMB_BMC_ADC0_VSEN:
      sprintf(name, "SMB_XP1R0V_FPGA");
      break;
    case SMB_BMC_ADC1_VSEN:
      sprintf(name, "SMB_XP1R2V_HBM");
      break;
    case SMB_BMC_ADC2_VSEN:
      sprintf(name, "SMB_XP3R3V_LEFT");
      break;
    case SMB_BMC_ADC3_VSEN:
      sprintf(name, "SMB_XP3R3V_RIGHT");
      break;
    case SMB_BMC_ADC4_VSEN:
      sprintf(name, "SMB_XP0R9V_LEFT");
      break;
    case SMB_BMC_ADC5_VSEN:
      sprintf(name, "SMB_XP1R85V_LEFT");
      break;
    case SMB_BMC_ADC6_VSEN:
      sprintf(name, "SMB_XP0R9V_RIGHT");
      break;
    case SMB_BMC_ADC7_VSEN:
      sprintf(name, "SMB_XP1R85V_RIGHT");
      break;
    case SMB_BMC_ADC8_VSEN:
      sprintf(name, "SMB_XP1R8V_ALG");
      break;
    case SMB_BMC_ADC9_VSEN:
      sprintf(name, "SMB_XP0R75V_VDDA");
      break;
    case SMB_BMC_ADC10_VSEN:
      sprintf(name, "SMB_XP1R2V_VDDH");
      break;
    case SMB_BMC_ADC11_VSEN:
      sprintf(name, "SMB_XP2R5V_HBM");
      break;
    case SMB_SENSOR_VDDCK_0_IN_VOLT:
      sprintf(name, "SMB_VDDCK_1P15V_0_IN_VOLT");
      break;
    case SMB_SENSOR_VDDCK_0_IN_CURR:
      sprintf(name, "SMB_VDDCK_1P15V_0_IN_CURR");
      break;
    case SMB_SENSOR_VDDCK_0_IN_POWER:
      sprintf(name, "SMB_VDDCK_1P15V_0_IN_POWER");
      break;
    case SMB_SENSOR_VDDCK_0_OUT_VOLT:
      sprintf(name, "SMB_VDDCK_1P15V_0_OUT_VOLT");
      break;
    case SMB_SENSOR_VDDCK_0_OUT_CURR:
      sprintf(name, "SMB_VDDCK_1P15V_0_OUT_CURR");
      break;
    case SMB_SENSOR_VDDCK_0_OUT_POWER:
      sprintf(name, "SMB_VDDCK_1P15V_0_OUT_POWER");
      break;
    case SMB_SENSOR_VDDCK_0_TEMP:
      sprintf(name, "SMB_VDDCK_1P15V_0_TEMP");
      break;
    case SMB_SENSOR_VDDCK_1_IN_VOLT:
      sprintf(name, "SMB_VDDCK_1P15V_1_IN_VOLT");
      break;
    case SMB_SENSOR_VDDCK_1_IN_CURR:
      sprintf(name, "SMB_VDDCK_1P15V_1_IN_CURR");
      break;
    case SMB_SENSOR_VDDCK_1_IN_POWER:
      sprintf(name, "SMB_VDDCK_1P15V_1_IN_POWER");
      break;
    case SMB_SENSOR_VDDCK_1_OUT_VOLT:
      sprintf(name, "SMB_VDDCK_1P15V_1_OUT_VOLT");
      break;
    case SMB_SENSOR_VDDCK_1_OUT_CURR:
      sprintf(name, "SMB_VDDCK_1P15V_1_OUT_CURR");
      break;
    case SMB_SENSOR_VDDCK_1_OUT_POWER:
      sprintf(name, "SMB_VDDCK_1P15V_1_OUT_POWER");
      break;
    case SMB_SENSOR_VDDCK_1_TEMP:
      sprintf(name, "SMB_VDDCK_1P15V_1_TEMP");
      break;
    case SMB_SENSOR_VDDCK_2_IN_VOLT:
      sprintf(name, "SMB_VDDCK_1P15V_2_IN_VOLT");
      break;
    case SMB_SENSOR_VDDCK_2_IN_CURR:
      sprintf(name, "SMB_VDDCK_1P15V_2_IN_CURR");
      break;
    case SMB_SENSOR_VDDCK_2_IN_POWER:
      sprintf(name, "SMB_VDDCK_1P15V_2_IN_POWER");
      break;
    case SMB_SENSOR_VDDCK_2_OUT_VOLT:
      sprintf(name, "SMB_VDDCK_1P15V_2_OUT_VOLT");
      break;
    case SMB_SENSOR_VDDCK_2_OUT_CURR:
      sprintf(name, "SMB_VDDCK_1P15V_2_OUT_CURR");
      break;
    case SMB_SENSOR_VDDCK_2_OUT_POWER:
      sprintf(name, "SMB_VDDCK_1P15V_2_OUT_POWER");
      break;
    case SMB_SENSOR_VDDCK_2_TEMP:
      sprintf(name, "SMB_VDDCK_1P15V_2_TEMP");
      break;
    case SMB_SENSOR_XDPE_LEFT_1_IN_VOLT:
      sprintf(name, "SMB_XP0R9V_LEFT_IN_VOLT");
      break;
    case SMB_SENSOR_XDPE_LEFT_1_IN_CURR:
      sprintf(name, "SMB_XP0R9V_LEFT_IN_CURR");
      break;
    case SMB_SENSOR_XDPE_LEFT_1_IN_POWER:
      sprintf(name, "SMB_XP0R9V_LEFT_IN_POWER");
      break;
    case SMB_SENSOR_XDPE_LEFT_1_OUT_VOLT:
      sprintf(name, "SMB_XP0R9V_LEFT_OUT_VOLT");
      break;
    case SMB_SENSOR_XDPE_LEFT_1_OUT_CURR:
      sprintf(name, "SMB_XP0R9V_LEFT_OUT_CURR");
      break;
    case SMB_SENSOR_XDPE_LEFT_1_OUT_POWER:
      sprintf(name, "SMB_XP0R9V_LEFT_OUT_POWER");
      break;
    case SMB_SENSOR_XDPE_LEFT_1_TEMP:
      sprintf(name, "SMB_XP0R9V_LEFT_TEMP");
      break;
    case SMB_SENSOR_XDPE_LEFT_2_IN_VOLT:
      sprintf(name, "SMB_XP1R85V_LEFT_IN_VOLT");
      break;
    case SMB_SENSOR_XDPE_LEFT_2_IN_CURR:
      sprintf(name, "SMB_XP1R85V_LEFT_IN_CURR");
      break;
    case SMB_SENSOR_XDPE_LEFT_2_IN_POWER:
      sprintf(name, "SMB_XP1R85V_LEFT_IN_POWER");
      break;
    case SMB_SENSOR_XDPE_LEFT_2_OUT_VOLT:
      sprintf(name, "SMB_XP1R85V_LEFT_OUT_VOLT");
      break;
    case SMB_SENSOR_XDPE_LEFT_2_OUT_CURR:
      sprintf(name, "SMB_XP1R85V_LEFT_OUT_CURR");
      break;
    case SMB_SENSOR_XDPE_LEFT_2_OUT_POWER:
      sprintf(name, "SMB_XP1R85V_LEFT_OUT_POWER");
      break;
    case SMB_SENSOR_XDPE_LEFT_2_TEMP:
      sprintf(name, "SMB_XP1R85V_LEFT_TEMP");
      break;
    case SMB_SENSOR_XDPE_RIGHT_1_IN_VOLT:
      sprintf(name, "SMB_XP0R9V_RIGHT_IN_VOLT");
      break;
    case SMB_SENSOR_XDPE_RIGHT_1_IN_CURR:
      sprintf(name, "SMB_XP0R9V_RIGHT_IN_CURR");
      break;
    case SMB_SENSOR_XDPE_RIGHT_1_IN_POWER:
      sprintf(name, "SMB_XP0R9V_RIGHT_IN_POWER");
      break;
    case SMB_SENSOR_XDPE_RIGHT_1_OUT_VOLT:
      sprintf(name, "SMB_XP0R9V_RIGHT_OUT_VOLT");
      break;
    case SMB_SENSOR_XDPE_RIGHT_1_OUT_CURR:
      sprintf(name, "SMB_XP0R9V_RIGHT_OUT_CURR");
      break;
    case SMB_SENSOR_XDPE_RIGHT_1_OUT_POWER:
      sprintf(name, "SMB_XP0R9V_RIGHT_OUT_POWER");
      break;
    case SMB_SENSOR_XDPE_RIGHT_1_TEMP:
      sprintf(name, "SMB_XP0R9V_RIGHT_TEMP");
      break;
    case SMB_SENSOR_XDPE_RIGHT_2_IN_VOLT:
      sprintf(name, "SMB_XP1R85V_RIGHT_IN_VOLT");
      break;
    case SMB_SENSOR_XDPE_RIGHT_2_IN_CURR:
      sprintf(name, "SMB_XP1R85V_RIGHT_IN_CURR");
      break;
    case SMB_SENSOR_XDPE_RIGHT_2_IN_POWER:
      sprintf(name, "SMB_XP1R85V_RIGHT_IN_POWER");
      break;
    case SMB_SENSOR_XDPE_RIGHT_2_OUT_VOLT:
      sprintf(name, "SMB_XP1R85V_RIGHT_OUT_VOLT");
      break;
    case SMB_SENSOR_XDPE_RIGHT_2_OUT_CURR:
      sprintf(name, "SMB_XP1R85V_RIGHT_OUT_CURR");
      break;
    case SMB_SENSOR_XDPE_RIGHT_2_OUT_POWER:
      sprintf(name, "SMB_XP1R85V_RIGHT_OUT_POWER");
      break;
    case SMB_SENSOR_XDPE_RIGHT_2_TEMP:
      sprintf(name, "SMB_XP1R85V_RIGHT_TEMP");
      break;
    default:
      return -1;
  }
  return 0;
}

static int get_psu_sensor_name(uint8_t sensor_num, char *name) {
  switch (sensor_num) {
    case PSU1_SENSOR_IN_VOLT:
      sprintf(name, "PSU1_INPUT_220V_VOLT");
      break;
    case PSU1_SENSOR_12V_VOLT:
      sprintf(name, "PSU1_OUTPUT_12V_VOLT");
      break;
    case PSU1_SENSOR_STBY_VOLT:
      sprintf(name, "PSU1_OUTPUT_3V3STBY_VOLT");
      break;
    case PSU1_SENSOR_IN_CURR:
      sprintf(name, "PSU1_INPUT_220V_CURR");
      break;
    case PSU1_SENSOR_12V_CURR:
      sprintf(name, "PSU1_OUTPUT_12V_CURR");
      break;
    case PSU1_SENSOR_STBY_CURR:
      sprintf(name, "PSU1_OUTPUT_3V3STBY_CURR");
      break;
    case PSU1_SENSOR_IN_POWER:
      sprintf(name, "PSU1_INPUT_220V_POWER");
      break;
    case PSU1_SENSOR_12V_POWER:
      sprintf(name, "PSU1_OUTPUT_12V_POWER");
      break;
    case PSU1_SENSOR_STBY_POWER:
      sprintf(name, "PSU1_OUTPUT_3V3STBY_POWER");
      break;
    case PSU1_SENSOR_FAN_TACH:
      sprintf(name, "PSU1_FAN_SPEED");
      break;
    case PSU1_SENSOR_TEMP1:
      sprintf(name, "PSU1_TEMP1");
      break;
    case PSU1_SENSOR_TEMP2:
      sprintf(name, "PSU1_TEMP2");
      break;
    case PSU1_SENSOR_TEMP3:
      sprintf(name, "PSU1_TEMP3");
      break;
    case PSU2_SENSOR_IN_VOLT:
      sprintf(name, "PSU2_INPUT_220V_VOLT");
      break;
    case PSU2_SENSOR_12V_VOLT:
      sprintf(name, "PSU2_OUTPUT_12V_VOLT");
      break;
    case PSU2_SENSOR_STBY_VOLT:
      sprintf(name, "PSU2_OUTPUT_3V3STBY_VOLT");
      break;
    case PSU2_SENSOR_IN_CURR:
      sprintf(name, "PSU2_INPUT_220V_CURR");
      break;
    case PSU2_SENSOR_12V_CURR:
      sprintf(name, "PSU2_OUTPUT_12V_CURR");
      break;
    case PSU2_SENSOR_STBY_CURR:
      sprintf(name, "PSU2_OUTPUT_3V3STBY_CURR");
      break;
    case PSU2_SENSOR_IN_POWER:
      sprintf(name, "PSU2_INPUT_220V_POWER");
      break;
    case PSU2_SENSOR_12V_POWER:
      sprintf(name, "PSU2_OUTPUT_12V_POWER");
      break;
    case PSU2_SENSOR_STBY_POWER:
      sprintf(name, "PSU2_OUTPUT_3V3STBY_POWER");
      break;
    case PSU2_SENSOR_FAN_TACH:
      sprintf(name, "PSU2_FAN_SPEED");
      break;
    case PSU2_SENSOR_TEMP1:
      sprintf(name, "PSU2_TEMP1");
      break;
    case PSU2_SENSOR_TEMP2:
      sprintf(name, "PSU2_TEMP2");
      break;
    case PSU2_SENSOR_TEMP3:
      sprintf(name, "PSU2_TEMP3");
      break;
    default:
      return -1;
  }
  return 0;
}

int pal_get_sensor_name(uint8_t fru, uint8_t sensor_num, char *name) {
  int ret = -1;

  switch (fru) {
    case FRU_SCM:
      ret = get_scm_sensor_name(sensor_num, name);
      break;
    case FRU_SMB:
      ret = get_smb_sensor_name(sensor_num, name);
      break;
    case FRU_PSU1:
    case FRU_PSU2:
      ret = get_psu_sensor_name(sensor_num, name);
      break;
    default:
      return -1;
  }
  return ret;
}

static int get_scm_sensor_units(uint8_t sensor_num, char *units) {
  switch (sensor_num) {
    case SCM_SENSOR_OUTLET_TEMP:
    case SCM_SENSOR_INLET_TEMP:
    case BIC_SENSOR_MB_OUTLET_TEMP:
    case BIC_SENSOR_MB_INLET_TEMP:
    case BIC_SENSOR_PCH_TEMP:
    case BIC_SENSOR_VCCIN_VR_TEMP:
    case BIC_SENSOR_1V05COMB_VR_TEMP:
    case BIC_SENSOR_SOC_TEMP:
    case BIC_SENSOR_SOC_THERM_MARGIN:
    case BIC_SENSOR_VDDR_VR_TEMP:
    case BIC_SENSOR_SOC_DIMMA_TEMP:
    case BIC_SENSOR_SOC_DIMMB_TEMP:
    case BIC_SENSOR_SOC_TJMAX:
      sprintf(units, "C");
      break;
    case SCM_SENSOR_HSC_OUT_VOLT:
    case BIC_SENSOR_P3V3_MB:
    case BIC_SENSOR_P12V_MB:
    case BIC_SENSOR_P1V05_PCH:
    case BIC_SENSOR_P3V3_STBY_MB:
    case BIC_SENSOR_P5V_STBY_MB:
    case BIC_SENSOR_PV_BAT:
    case BIC_SENSOR_PVDDR:
    case BIC_SENSOR_P1V05_COMB:
    case BIC_SENSOR_VCCIN_VR_VOL:
    case BIC_SENSOR_VDDR_VR_VOL:
    case BIC_SENSOR_P1V05COMB_VR_VOL:
    case BIC_SENSOR_INA230_VOL:
      sprintf(units, "Volts");
      break;
    case SCM_SENSOR_HSC_OUT_CURR:
    case BIC_SENSOR_1V05COMB_VR_CURR:
    case BIC_SENSOR_VDDR_VR_CURR:
    case BIC_SENSOR_VCCIN_VR_CURR:
      sprintf(units, "Amps");
      break;
    case BIC_SENSOR_SOC_PACKAGE_PWR:
    case BIC_SENSOR_VCCIN_VR_POUT:
    case BIC_SENSOR_VDDR_VR_POUT:
    case BIC_SENSOR_P1V05COMB_VR_POUT:
    case BIC_SENSOR_INA230_POWER:
      sprintf(units, "Watts");
      break;
    default:
      return -1;
  }
  return 0;
}

static int get_smb_sensor_units(uint8_t sensor_num, char *units) {
  switch (sensor_num) {
    case SMB_SENSOR_VDDA_TEMP1:
    case SMB_SENSOR_PCIE_TEMP1:
    case SMB_SENSOR_IR3R3V_LEFT_TEMP:
    case SMB_SENSOR_IR3R3V_RIGHT_TEMP:
    case SMB_SENSOR_SW_CORE_TEMP1:
    case SMB_SENSOR_XPDE_HBM_TEMP1:
    case SMB_SENSOR_LM75B_U28_TEMP:
    case SMB_SENSOR_LM75B_U25_TEMP:
    case SMB_SENSOR_LM75B_U56_TEMP:
    case SMB_SENSOR_LM75B_U55_TEMP:
    case SMB_SENSOR_LM75B_U2_TEMP:
    case SMB_SENSOR_LM75B_U13_TEMP:
    case SMB_SENSOR_TMP421_U62_TEMP:
    case SMB_SENSOR_TMP421_U63_TEMP:
    case SMB_SENSOR_BMC_LM75B_TEMP:
    case SMB_SENSOR_GB_HIGH_TEMP:
    case SMB_SENSOR_GB_TEMP1:
    case SMB_SENSOR_GB_TEMP2:
    case SMB_SENSOR_GB_TEMP3:
    case SMB_SENSOR_GB_TEMP4:
    case SMB_SENSOR_GB_TEMP5:
    case SMB_SENSOR_GB_TEMP6:
    case SMB_SENSOR_GB_TEMP7:
    case SMB_SENSOR_GB_TEMP8:
    case SMB_SENSOR_GB_TEMP9:
    case SMB_SENSOR_GB_TEMP10:
    case SMB_SENSOR_GB_HBM_TEMP1:
    case SMB_SENSOR_GB_HBM_TEMP2:
    case SMB_SENSOR_FCM_LM75B_U1_TEMP:
    case SMB_SENSOR_FCM_LM75B_U2_TEMP:
    case SMB_DOM1_MAX_TEMP:
    case SMB_DOM2_MAX_TEMP:
    case SMB_SENSOR_VDDCK_0_TEMP:
    case SMB_SENSOR_VDDCK_1_TEMP:
    case SMB_SENSOR_VDDCK_2_TEMP:
    case SMB_SENSOR_XDPE_LEFT_1_TEMP:
    case SMB_SENSOR_XDPE_LEFT_2_TEMP:
    case SMB_SENSOR_XDPE_RIGHT_1_TEMP:
    case SMB_SENSOR_XDPE_RIGHT_2_TEMP:
      sprintf(units, "C");
      break;
    case SMB_SENSOR_1220_VMON1:
    case SMB_SENSOR_1220_VMON2:
    case SMB_SENSOR_1220_VMON3:
    case SMB_SENSOR_1220_VMON4:
    case SMB_SENSOR_1220_VMON5:
    case SMB_SENSOR_1220_VMON6:
    case SMB_SENSOR_1220_VMON7:
    case SMB_SENSOR_1220_VMON8:
    case SMB_SENSOR_1220_VMON9:
    case SMB_SENSOR_1220_VMON10:
    case SMB_SENSOR_1220_VMON11:
    case SMB_SENSOR_1220_VMON12:
    case SMB_SENSOR_1220_VCCA:
    case SMB_SENSOR_1220_VCCINP:
    case SMB_SENSOR_VDDA_IN_VOLT:
    case SMB_SENSOR_VDDA_OUT_VOLT:
    case SMB_SENSOR_PCIE_IN_VOLT:
    case SMB_SENSOR_PCIE_OUT_VOLT:
    case SMB_SENSOR_IR3R3V_LEFT_IN_VOLT:
    case SMB_SENSOR_IR3R3V_LEFT_OUT_VOLT:
    case SMB_SENSOR_IR3R3V_RIGHT_IN_VOLT:
    case SMB_SENSOR_IR3R3V_RIGHT_OUT_VOLT:
    case SMB_SENSOR_SW_CORE_IN_VOLT:
    case SMB_SENSOR_SW_CORE_OUT_VOLT:
    case SMB_SENSOR_XPDE_HBM_IN_VOLT:
    case SMB_SENSOR_XPDE_HBM_OUT_VOLT:
    case SMB_SENSOR_FCM_HSC_IN_VOLT:
    case SMB_SENSOR_FCM_HSC_OUT_VOLT:
    case SMB_BMC_ADC0_VSEN:
    case SMB_BMC_ADC1_VSEN:
    case SMB_BMC_ADC2_VSEN:
    case SMB_BMC_ADC3_VSEN:
    case SMB_BMC_ADC4_VSEN:
    case SMB_BMC_ADC5_VSEN:
    case SMB_BMC_ADC6_VSEN:
    case SMB_BMC_ADC7_VSEN:
    case SMB_BMC_ADC8_VSEN:
    case SMB_BMC_ADC9_VSEN:
    case SMB_BMC_ADC10_VSEN:
    case SMB_BMC_ADC11_VSEN:
    case SMB_SENSOR_VDDCK_0_IN_VOLT:
    case SMB_SENSOR_VDDCK_0_OUT_VOLT:
    case SMB_SENSOR_VDDCK_1_IN_VOLT:
    case SMB_SENSOR_VDDCK_1_OUT_VOLT:
    case SMB_SENSOR_VDDCK_2_IN_VOLT:
    case SMB_SENSOR_VDDCK_2_OUT_VOLT:
    case SMB_SENSOR_XDPE_LEFT_1_IN_VOLT:
    case SMB_SENSOR_XDPE_LEFT_1_OUT_VOLT:
    case SMB_SENSOR_XDPE_LEFT_2_IN_VOLT:
    case SMB_SENSOR_XDPE_LEFT_2_OUT_VOLT:
    case SMB_SENSOR_XDPE_RIGHT_1_IN_VOLT:
    case SMB_SENSOR_XDPE_RIGHT_1_OUT_VOLT:
    case SMB_SENSOR_XDPE_RIGHT_2_IN_VOLT:
    case SMB_SENSOR_XDPE_RIGHT_2_OUT_VOLT:
      sprintf(units, "Volts");
      break;
    case SMB_SENSOR_VDDA_IN_CURR:
    case SMB_SENSOR_VDDA_OUT_CURR:
    case SMB_SENSOR_PCIE_IN_CURR:
    case SMB_SENSOR_PCIE_OUT_CURR:
    case SMB_SENSOR_IR3R3V_LEFT_IN_CURR:
    case SMB_SENSOR_IR3R3V_LEFT_OUT_CURR:
    case SMB_SENSOR_IR3R3V_RIGHT_IN_CURR:
    case SMB_SENSOR_IR3R3V_RIGHT_OUT_CURR:
    case SMB_SENSOR_SW_CORE_IN_CURR:
    case SMB_SENSOR_SW_CORE_OUT_CURR:
    case SMB_SENSOR_XPDE_HBM_IN_CURR:
    case SMB_SENSOR_XPDE_HBM_OUT_CURR:
    case SMB_SENSOR_FCM_HSC_OUT_CURR:
    case SMB_SENSOR_VDDCK_0_IN_CURR:
    case SMB_SENSOR_VDDCK_0_OUT_CURR:
    case SMB_SENSOR_VDDCK_1_IN_CURR:
    case SMB_SENSOR_VDDCK_1_OUT_CURR:
    case SMB_SENSOR_VDDCK_2_IN_CURR:
    case SMB_SENSOR_VDDCK_2_OUT_CURR:
    case SMB_SENSOR_XDPE_LEFT_1_IN_CURR:
    case SMB_SENSOR_XDPE_LEFT_1_OUT_CURR:
    case SMB_SENSOR_XDPE_LEFT_2_IN_CURR:
    case SMB_SENSOR_XDPE_LEFT_2_OUT_CURR:
    case SMB_SENSOR_XDPE_RIGHT_1_IN_CURR:
    case SMB_SENSOR_XDPE_RIGHT_1_OUT_CURR:
    case SMB_SENSOR_XDPE_RIGHT_2_IN_CURR:
    case SMB_SENSOR_XDPE_RIGHT_2_OUT_CURR:
      sprintf(units, "Amps");
      break;
    case SMB_SENSOR_VDDA_IN_POWER:
    case SMB_SENSOR_VDDA_OUT_POWER:
    case SMB_SENSOR_PCIE_IN_POWER:
    case SMB_SENSOR_PCIE_OUT_POWER:
    case SMB_SENSOR_IR3R3V_LEFT_IN_POWER:
    case SMB_SENSOR_IR3R3V_LEFT_OUT_POWER:
    case SMB_SENSOR_IR3R3V_RIGHT_IN_POWER:
    case SMB_SENSOR_IR3R3V_RIGHT_OUT_POWER:
    case SMB_SENSOR_SW_CORE_IN_POWER:
    case SMB_SENSOR_SW_CORE_OUT_POWER:
    case SMB_SENSOR_XPDE_HBM_IN_POWER:
    case SMB_SENSOR_XPDE_HBM_OUT_POWER:
    case SMB_SENSOR_VDDCK_0_IN_POWER:
    case SMB_SENSOR_VDDCK_0_OUT_POWER:
    case SMB_SENSOR_VDDCK_1_IN_POWER:
    case SMB_SENSOR_VDDCK_1_OUT_POWER:
    case SMB_SENSOR_VDDCK_2_IN_POWER:
    case SMB_SENSOR_VDDCK_2_OUT_POWER:
    case SMB_SENSOR_XDPE_LEFT_1_IN_POWER:
    case SMB_SENSOR_XDPE_LEFT_1_OUT_POWER:
    case SMB_SENSOR_XDPE_LEFT_2_IN_POWER:
    case SMB_SENSOR_XDPE_LEFT_2_OUT_POWER:
    case SMB_SENSOR_XDPE_RIGHT_1_IN_POWER:
    case SMB_SENSOR_XDPE_RIGHT_1_OUT_POWER:
    case SMB_SENSOR_XDPE_RIGHT_2_IN_POWER:
    case SMB_SENSOR_XDPE_RIGHT_2_OUT_POWER:
    case SMB_SENSOR_FCM_HSC_IN_POWER:
      sprintf(units, "Watts");
      break;
    case SMB_SENSOR_FAN1_FRONT_TACH:
    case SMB_SENSOR_FAN1_REAR_TACH:
    case SMB_SENSOR_FAN2_FRONT_TACH:
    case SMB_SENSOR_FAN2_REAR_TACH:
    case SMB_SENSOR_FAN3_FRONT_TACH:
    case SMB_SENSOR_FAN3_REAR_TACH:
    case SMB_SENSOR_FAN4_FRONT_TACH:
    case SMB_SENSOR_FAN4_REAR_TACH:
      sprintf(units, "RPM");
      break;
    default:
      return -1;
  }
  return 0;
}

static int get_psu_sensor_units(uint8_t sensor_num, char *units) {
  switch (sensor_num) {
    case PSU1_SENSOR_IN_VOLT:
    case PSU1_SENSOR_12V_VOLT:
    case PSU1_SENSOR_STBY_VOLT:
    case PSU2_SENSOR_IN_VOLT:
    case PSU2_SENSOR_12V_VOLT:
    case PSU2_SENSOR_STBY_VOLT:
      sprintf(units, "Volts");
      break;
    case PSU1_SENSOR_IN_CURR:
    case PSU1_SENSOR_12V_CURR:
    case PSU1_SENSOR_STBY_CURR:
    case PSU2_SENSOR_IN_CURR:
    case PSU2_SENSOR_12V_CURR:
    case PSU2_SENSOR_STBY_CURR:
      sprintf(units, "Amps");
      break;
    case PSU1_SENSOR_IN_POWER:
    case PSU1_SENSOR_12V_POWER:
    case PSU1_SENSOR_STBY_POWER:
    case PSU2_SENSOR_IN_POWER:
    case PSU2_SENSOR_12V_POWER:
    case PSU2_SENSOR_STBY_POWER:
      sprintf(units, "Watts");
      break;
    case PSU1_SENSOR_FAN_TACH:
    case PSU2_SENSOR_FAN_TACH:
      sprintf(units, "RPM");
      break;
    case PSU1_SENSOR_TEMP1:
    case PSU1_SENSOR_TEMP2:
    case PSU1_SENSOR_TEMP3:
    case PSU2_SENSOR_TEMP1:
    case PSU2_SENSOR_TEMP2:
    case PSU2_SENSOR_TEMP3:
      sprintf(units, "C");
      break;
     default:
      return -1;
  }
  return 0;
}

int pal_get_sensor_units(uint8_t fru, uint8_t sensor_num, char *units) {
  int ret = -1;

  switch (fru) {
    case FRU_SCM:
      ret = get_scm_sensor_units(sensor_num, units);
      break;
    case FRU_SMB:
      ret = get_smb_sensor_units(sensor_num, units);
      break;
    case FRU_PSU1:
    case FRU_PSU2:
      ret = get_psu_sensor_units(sensor_num, units);
      break;
    default:
      return -1;
  }
  return ret;
}

static void sensor_thresh_array_init(uint8_t fru) {
  static bool init_done[MAX_NUM_FRUS] = {false};
  int fru_offset;
  float fvalue;

  if (init_done[fru])
    return;

  switch (fru) {
    case FRU_SCM:
      scm_sensor_threshold[SCM_SENSOR_OUTLET_TEMP][UCR_THRESH] = 90;
      scm_sensor_threshold[SCM_SENSOR_INLET_TEMP][UCR_THRESH] = 60;
      scm_sensor_threshold[SCM_SENSOR_HSC_IN_VOLT][UCR_THRESH] = 12.6;
      scm_sensor_threshold[SCM_SENSOR_HSC_IN_VOLT][LCR_THRESH] = 11.4;
      scm_sensor_threshold[SCM_SENSOR_HSC_OUT_VOLT][UCR_THRESH] = 12.6;
      scm_sensor_threshold[SCM_SENSOR_HSC_OUT_VOLT][LCR_THRESH] = 11.4;
      scm_sensor_threshold[SCM_SENSOR_HSC_OUT_CURR][UCR_THRESH] = 5;
      for (int sensor_index = scm_sensor_cnt; sensor_index < scm_all_sensor_cnt; sensor_index++) {
        for (int threshold_type = 1; threshold_type <= MAX_SENSOR_THRESHOLD + 1; threshold_type++) {
          if (!bic_get_sdr_thresh_val(fru, scm_all_sensor_list[sensor_index], threshold_type, &fvalue)){
            scm_sensor_threshold[scm_all_sensor_list[sensor_index]][threshold_type] = fvalue;
          }
        }
      }
      break;
    case FRU_SMB:
      smb_sensor_threshold[SMB_SENSOR_1220_VMON1][UCR_THRESH] = 13.2;
      smb_sensor_threshold[SMB_SENSOR_1220_VMON1][LCR_THRESH] = 10.8;
      smb_sensor_threshold[SMB_SENSOR_1220_VMON2][UCR_THRESH] = 5.5;
      smb_sensor_threshold[SMB_SENSOR_1220_VMON2][LCR_THRESH] = 4.5;
      smb_sensor_threshold[SMB_SENSOR_1220_VMON3][UCR_THRESH] = 3.63;
      smb_sensor_threshold[SMB_SENSOR_1220_VMON3][LCR_THRESH] = 2.97;
      smb_sensor_threshold[SMB_SENSOR_1220_VMON4][UCR_THRESH] = 3.63;
      smb_sensor_threshold[SMB_SENSOR_1220_VMON4][LCR_THRESH] = 2.97;
      smb_sensor_threshold[SMB_SENSOR_1220_VMON5][UCR_THRESH] = 1.32;
      smb_sensor_threshold[SMB_SENSOR_1220_VMON5][LCR_THRESH] = 1.08;
      smb_sensor_threshold[SMB_SENSOR_1220_VMON6][UCR_THRESH] = 1.98;
      smb_sensor_threshold[SMB_SENSOR_1220_VMON6][LCR_THRESH] = 1.62;
      smb_sensor_threshold[SMB_SENSOR_1220_VMON7][UCR_THRESH] = 1.98;
      smb_sensor_threshold[SMB_SENSOR_1220_VMON7][LCR_THRESH] = 1.62;
      smb_sensor_threshold[SMB_SENSOR_1220_VMON8][UCR_THRESH] = 2.75;
      smb_sensor_threshold[SMB_SENSOR_1220_VMON8][LCR_THRESH] = 2.25;
      smb_sensor_threshold[SMB_SENSOR_1220_VMON9][UCR_THRESH] = 1.06;
      smb_sensor_threshold[SMB_SENSOR_1220_VMON9][LCR_THRESH] = 0.86;
      smb_sensor_threshold[SMB_SENSOR_1220_VMON10][UCR_THRESH] = 0.91;
      smb_sensor_threshold[SMB_SENSOR_1220_VMON10][LCR_THRESH] = 0.74;
      smb_sensor_threshold[SMB_SENSOR_1220_VMON11][UCR_THRESH] = 0.83;
      smb_sensor_threshold[SMB_SENSOR_1220_VMON11][LCR_THRESH] = 0.67;
      smb_sensor_threshold[SMB_SENSOR_1220_VMON12][UCR_THRESH] = 1.27;
      smb_sensor_threshold[SMB_SENSOR_1220_VMON12][LCR_THRESH] = 1.03;
      smb_sensor_threshold[SMB_SENSOR_1220_VCCA][UCR_THRESH] = 3.63;
      smb_sensor_threshold[SMB_SENSOR_1220_VCCA][LCR_THRESH] = 2.97;
      smb_sensor_threshold[SMB_SENSOR_1220_VCCINP][UCR_THRESH] = 3.63;
      smb_sensor_threshold[SMB_SENSOR_1220_VCCINP][LCR_THRESH] = 2.97;

      /* BMC ADC Sensors */
      smb_sensor_threshold[SMB_BMC_ADC0_VSEN][UCR_THRESH] = 1.1;
      smb_sensor_threshold[SMB_BMC_ADC0_VSEN][LCR_THRESH] = 0.9;
      smb_sensor_threshold[SMB_BMC_ADC1_VSEN][UCR_THRESH] = 1.32;
      smb_sensor_threshold[SMB_BMC_ADC1_VSEN][LCR_THRESH] = 1.08;
      smb_sensor_threshold[SMB_BMC_ADC2_VSEN][UCR_THRESH] = 3.63;
      smb_sensor_threshold[SMB_BMC_ADC2_VSEN][LCR_THRESH] = 2.97;
      smb_sensor_threshold[SMB_BMC_ADC3_VSEN][UCR_THRESH] = 3.63;
      smb_sensor_threshold[SMB_BMC_ADC3_VSEN][LCR_THRESH] = 2.97;
      smb_sensor_threshold[SMB_BMC_ADC4_VSEN][UCR_THRESH] = 0.99;
      smb_sensor_threshold[SMB_BMC_ADC4_VSEN][LCR_THRESH] = 0.81;
      smb_sensor_threshold[SMB_BMC_ADC5_VSEN][UCR_THRESH] = 2.04;
      smb_sensor_threshold[SMB_BMC_ADC5_VSEN][LCR_THRESH] = 1.66;
      smb_sensor_threshold[SMB_BMC_ADC6_VSEN][UCR_THRESH] = 0.99;
      smb_sensor_threshold[SMB_BMC_ADC6_VSEN][LCR_THRESH] = 0.81;
      smb_sensor_threshold[SMB_BMC_ADC7_VSEN][UCR_THRESH] = 2.04;
      smb_sensor_threshold[SMB_BMC_ADC7_VSEN][LCR_THRESH] = 1.66;
      smb_sensor_threshold[SMB_BMC_ADC8_VSEN][UCR_THRESH] = 1.98;
      smb_sensor_threshold[SMB_BMC_ADC8_VSEN][LCR_THRESH] = 1.62;
      smb_sensor_threshold[SMB_BMC_ADC9_VSEN][UCR_THRESH] = 0.83;
      smb_sensor_threshold[SMB_BMC_ADC9_VSEN][LCR_THRESH] = 0.67;
      smb_sensor_threshold[SMB_BMC_ADC10_VSEN][UCR_THRESH] = 1.32;
      smb_sensor_threshold[SMB_BMC_ADC10_VSEN][LCR_THRESH] = 1.08;
      smb_sensor_threshold[SMB_BMC_ADC11_VSEN][UCR_THRESH] = 2.75;
      smb_sensor_threshold[SMB_BMC_ADC11_VSEN][LCR_THRESH] = 2.25;

      smb_sensor_threshold[SMB_SENSOR_VDDA_IN_VOLT][UCR_THRESH] = 12.6;
      smb_sensor_threshold[SMB_SENSOR_VDDA_IN_VOLT][LCR_THRESH] = 11.4;
      smb_sensor_threshold[SMB_SENSOR_VDDA_OUT_VOLT][UCR_THRESH] = 0.99;
      smb_sensor_threshold[SMB_SENSOR_VDDA_OUT_VOLT][LCR_THRESH] = 0.93;
      smb_sensor_threshold[SMB_SENSOR_VDDA_OUT_CURR][UCR_THRESH] = 54;
      smb_sensor_threshold[SMB_SENSOR_VDDA_OUT_POWER][UCR_THRESH] = 53.46;
      smb_sensor_threshold[SMB_SENSOR_VDDA_TEMP1][UCR_THRESH] = 85;
      smb_sensor_threshold[SMB_SENSOR_VDDA_TEMP1][LCR_THRESH] = -40;

      smb_sensor_threshold[SMB_SENSOR_PCIE_IN_VOLT][UCR_THRESH] = 12.6;
      smb_sensor_threshold[SMB_SENSOR_PCIE_IN_VOLT][LCR_THRESH] = 11.4;
      smb_sensor_threshold[SMB_SENSOR_PCIE_OUT_VOLT][UCR_THRESH] = 0.78;
      smb_sensor_threshold[SMB_SENSOR_PCIE_OUT_VOLT][LCR_THRESH] = 0.72;
      smb_sensor_threshold[SMB_SENSOR_PCIE_OUT_CURR][UCR_THRESH] = 52.4;
      smb_sensor_threshold[SMB_SENSOR_PCIE_OUT_POWER][UCR_THRESH] = 40.88;
      smb_sensor_threshold[SMB_SENSOR_PCIE_TEMP1][UCR_THRESH] = 85;
      smb_sensor_threshold[SMB_SENSOR_PCIE_TEMP1][LCR_THRESH] = -40;

      smb_sensor_threshold[SMB_SENSOR_IR3R3V_LEFT_IN_VOLT][UCR_THRESH] = 12.6;
      smb_sensor_threshold[SMB_SENSOR_IR3R3V_LEFT_IN_VOLT][LCR_THRESH] = 11.4;
      smb_sensor_threshold[SMB_SENSOR_IR3R3V_LEFT_OUT_VOLT][UCR_THRESH] = 3.47;
      smb_sensor_threshold[SMB_SENSOR_IR3R3V_LEFT_OUT_VOLT][LCR_THRESH] = 3.13;
      smb_sensor_threshold[SMB_SENSOR_IR3R3V_LEFT_OUT_CURR][UCR_THRESH] = 97;
      smb_sensor_threshold[SMB_SENSOR_IR3R3V_LEFT_OUT_POWER][UCR_THRESH] = 336.59;
      smb_sensor_threshold[SMB_SENSOR_IR3R3V_LEFT_TEMP][UCR_THRESH] = 85;
      smb_sensor_threshold[SMB_SENSOR_IR3R3V_LEFT_TEMP][LCR_THRESH] = -40;

      smb_sensor_threshold[SMB_SENSOR_IR3R3V_RIGHT_IN_VOLT][UCR_THRESH] = 12.6;
      smb_sensor_threshold[SMB_SENSOR_IR3R3V_RIGHT_IN_VOLT][LCR_THRESH] = 11.4;
      smb_sensor_threshold[SMB_SENSOR_IR3R3V_RIGHT_OUT_VOLT][UCR_THRESH] = 3.47;
      smb_sensor_threshold[SMB_SENSOR_IR3R3V_RIGHT_OUT_VOLT][LCR_THRESH] = 3.13;
      smb_sensor_threshold[SMB_SENSOR_IR3R3V_RIGHT_OUT_CURR][UCR_THRESH] = 97;
      smb_sensor_threshold[SMB_SENSOR_IR3R3V_RIGHT_OUT_POWER][UCR_THRESH] = 336.59;
      smb_sensor_threshold[SMB_SENSOR_IR3R3V_RIGHT_TEMP][UCR_THRESH] = 85;
      smb_sensor_threshold[SMB_SENSOR_IR3R3V_RIGHT_TEMP][LCR_THRESH] = -40;

      smb_sensor_threshold[SMB_SENSOR_SW_CORE_IN_VOLT][UCR_THRESH] = 12.6;
      smb_sensor_threshold[SMB_SENSOR_SW_CORE_IN_VOLT][LCR_THRESH] = 11.4;
      smb_sensor_threshold[SMB_SENSOR_SW_CORE_TEMP1][UCR_THRESH] = 85;
      smb_sensor_threshold[SMB_SENSOR_SW_CORE_TEMP1][LCR_THRESH] = -40;
      smb_sensor_threshold[SMB_SENSOR_SW_CORE_OUT_VOLT][UCR_THRESH] = 0.85;
      smb_sensor_threshold[SMB_SENSOR_SW_CORE_OUT_VOLT][LCR_THRESH] = 0.8;
      smb_sensor_threshold[SMB_SENSOR_SW_CORE_OUT_CURR][UCR_THRESH] = 370;
      smb_sensor_threshold[SMB_SENSOR_SW_CORE_OUT_POWER][UCR_THRESH] = 314.5;

      smb_sensor_threshold[SMB_SENSOR_XPDE_HBM_IN_VOLT][UCR_THRESH] = 12.6;
      smb_sensor_threshold[SMB_SENSOR_XPDE_HBM_IN_VOLT][LCR_THRESH] = 11.4;
      smb_sensor_threshold[SMB_SENSOR_XPDE_HBM_TEMP1][UCR_THRESH] = 85;
      smb_sensor_threshold[SMB_SENSOR_XPDE_HBM_TEMP1][LCR_THRESH] = -40;
      smb_sensor_threshold[SMB_SENSOR_XPDE_HBM_OUT_VOLT][UCR_THRESH] = 1.24;
      smb_sensor_threshold[SMB_SENSOR_XPDE_HBM_OUT_VOLT][LCR_THRESH] = 1.16;
      smb_sensor_threshold[SMB_SENSOR_XPDE_HBM_OUT_CURR][UCR_THRESH] = 36;
      smb_sensor_threshold[SMB_SENSOR_XPDE_HBM_OUT_POWER][UCR_THRESH] = 44.64;

      smb_sensor_threshold[SMB_SENSOR_VDDCK_0_IN_VOLT][UCR_THRESH] = 12.6;
      smb_sensor_threshold[SMB_SENSOR_VDDCK_0_IN_VOLT][LCR_THRESH] = 11.4;
      smb_sensor_threshold[SMB_SENSOR_VDDCK_0_OUT_VOLT][UCR_THRESH] = 1.19;
      smb_sensor_threshold[SMB_SENSOR_VDDCK_0_OUT_VOLT][LCR_THRESH] = 1.11;
      smb_sensor_threshold[SMB_SENSOR_VDDCK_0_OUT_CURR][UCR_THRESH] = 8.5;
      smb_sensor_threshold[SMB_SENSOR_VDDCK_0_OUT_POWER][UCR_THRESH] = 9.86;
      smb_sensor_threshold[SMB_SENSOR_VDDCK_0_TEMP][UCR_THRESH] = 85;
      smb_sensor_threshold[SMB_SENSOR_VDDCK_0_TEMP][LCR_THRESH] = -5;
      smb_sensor_threshold[SMB_SENSOR_VDDCK_1_IN_VOLT][UCR_THRESH] = 12.6;
      smb_sensor_threshold[SMB_SENSOR_VDDCK_1_IN_VOLT][LCR_THRESH] = 11.4;
      smb_sensor_threshold[SMB_SENSOR_VDDCK_1_OUT_VOLT][UCR_THRESH] = 1.19;
      smb_sensor_threshold[SMB_SENSOR_VDDCK_1_OUT_VOLT][LCR_THRESH] = 1.11;
      smb_sensor_threshold[SMB_SENSOR_VDDCK_1_OUT_CURR][UCR_THRESH] = 8.5;
      smb_sensor_threshold[SMB_SENSOR_VDDCK_1_OUT_POWER][UCR_THRESH] = 9.86;
      smb_sensor_threshold[SMB_SENSOR_VDDCK_1_TEMP][UCR_THRESH] = 85;
      smb_sensor_threshold[SMB_SENSOR_VDDCK_1_TEMP][LCR_THRESH] = -5;
      smb_sensor_threshold[SMB_SENSOR_VDDCK_2_IN_VOLT][UCR_THRESH] = 12.6;
      smb_sensor_threshold[SMB_SENSOR_VDDCK_2_IN_VOLT][LCR_THRESH] = 11.4;
      smb_sensor_threshold[SMB_SENSOR_VDDCK_2_OUT_VOLT][UCR_THRESH] = 1.19;
      smb_sensor_threshold[SMB_SENSOR_VDDCK_2_OUT_VOLT][LCR_THRESH] = 1.11;
      smb_sensor_threshold[SMB_SENSOR_VDDCK_2_OUT_CURR][UCR_THRESH] = 8.5;
      smb_sensor_threshold[SMB_SENSOR_VDDCK_2_OUT_POWER][UCR_THRESH] = 9.86;
      smb_sensor_threshold[SMB_SENSOR_VDDCK_2_TEMP][UCR_THRESH] = 85;
      smb_sensor_threshold[SMB_SENSOR_VDDCK_2_TEMP][LCR_THRESH] = -5;

      smb_sensor_threshold[SMB_SENSOR_XDPE_LEFT_1_IN_VOLT][UCR_THRESH] = 12.6;
      smb_sensor_threshold[SMB_SENSOR_XDPE_LEFT_1_IN_VOLT][LCR_THRESH] = 11.4;
      smb_sensor_threshold[SMB_SENSOR_XDPE_LEFT_1_OUT_VOLT][UCR_THRESH] = 0.93;
      smb_sensor_threshold[SMB_SENSOR_XDPE_LEFT_1_OUT_VOLT][LCR_THRESH] = 0.87;
      smb_sensor_threshold[SMB_SENSOR_XDPE_LEFT_1_OUT_CURR][UCR_THRESH] = 243;
      smb_sensor_threshold[SMB_SENSOR_XDPE_LEFT_1_OUT_POWER][UCR_THRESH] = 225.99;
      smb_sensor_threshold[SMB_SENSOR_XDPE_LEFT_1_TEMP][UCR_THRESH] = 85;
      smb_sensor_threshold[SMB_SENSOR_XDPE_LEFT_1_TEMP][LCR_THRESH] = -40;
      smb_sensor_threshold[SMB_SENSOR_XDPE_LEFT_2_IN_VOLT][UCR_THRESH] = 12.6;
      smb_sensor_threshold[SMB_SENSOR_XDPE_LEFT_2_IN_VOLT][LCR_THRESH] = 11.4;
      smb_sensor_threshold[SMB_SENSOR_XDPE_LEFT_2_OUT_VOLT][UCR_THRESH] = 1.91;
      smb_sensor_threshold[SMB_SENSOR_XDPE_LEFT_2_OUT_VOLT][LCR_THRESH] = 1.79;
      smb_sensor_threshold[SMB_SENSOR_XDPE_LEFT_2_OUT_CURR][UCR_THRESH] = 53.6;
      smb_sensor_threshold[SMB_SENSOR_XDPE_LEFT_2_OUT_POWER][UCR_THRESH] = 102.38;
      smb_sensor_threshold[SMB_SENSOR_XDPE_LEFT_2_TEMP][UCR_THRESH] = 85;
      smb_sensor_threshold[SMB_SENSOR_XDPE_LEFT_2_TEMP][LCR_THRESH] = -40;

      smb_sensor_threshold[SMB_SENSOR_XDPE_RIGHT_1_IN_VOLT][UCR_THRESH] = 12.6;
      smb_sensor_threshold[SMB_SENSOR_XDPE_RIGHT_1_IN_VOLT][LCR_THRESH] = 11.4;
      smb_sensor_threshold[SMB_SENSOR_XDPE_RIGHT_1_OUT_VOLT][UCR_THRESH] = 0.93;
      smb_sensor_threshold[SMB_SENSOR_XDPE_RIGHT_1_OUT_VOLT][LCR_THRESH] = 0.87;
      smb_sensor_threshold[SMB_SENSOR_XDPE_RIGHT_1_OUT_CURR][UCR_THRESH] = 243;
      smb_sensor_threshold[SMB_SENSOR_XDPE_RIGHT_1_OUT_POWER][UCR_THRESH] = 225.99;
      smb_sensor_threshold[SMB_SENSOR_XDPE_RIGHT_1_TEMP][UCR_THRESH] = 85;
      smb_sensor_threshold[SMB_SENSOR_XDPE_RIGHT_1_TEMP][LCR_THRESH] = -40;
      smb_sensor_threshold[SMB_SENSOR_XDPE_RIGHT_2_IN_VOLT][UCR_THRESH] = 12.6;
      smb_sensor_threshold[SMB_SENSOR_XDPE_RIGHT_2_IN_VOLT][LCR_THRESH] = 11.4;
      smb_sensor_threshold[SMB_SENSOR_XDPE_RIGHT_2_OUT_VOLT][UCR_THRESH] = 1.91;
      smb_sensor_threshold[SMB_SENSOR_XDPE_RIGHT_2_OUT_VOLT][LCR_THRESH] = 1.79;
      smb_sensor_threshold[SMB_SENSOR_XDPE_RIGHT_2_OUT_CURR][UCR_THRESH] = 53.6;
      smb_sensor_threshold[SMB_SENSOR_XDPE_RIGHT_2_OUT_POWER][UCR_THRESH] = 102.38;
      smb_sensor_threshold[SMB_SENSOR_XDPE_RIGHT_2_TEMP][UCR_THRESH] = 85;
      smb_sensor_threshold[SMB_SENSOR_XDPE_RIGHT_2_TEMP][LCR_THRESH] = -40;

      smb_sensor_threshold[SMB_SENSOR_LM75B_U28_TEMP][UCR_THRESH] = 80;
      smb_sensor_threshold[SMB_SENSOR_LM75B_U25_TEMP][UCR_THRESH] = 80;
      smb_sensor_threshold[SMB_SENSOR_LM75B_U56_TEMP][UCR_THRESH] = 80;
      smb_sensor_threshold[SMB_SENSOR_LM75B_U55_TEMP][UCR_THRESH] = 80;
      smb_sensor_threshold[SMB_SENSOR_LM75B_U2_TEMP][UCR_THRESH] = 80;
      smb_sensor_threshold[SMB_SENSOR_LM75B_U13_TEMP][UCR_THRESH] = 80;
      smb_sensor_threshold[SMB_SENSOR_TMP421_U62_TEMP][UCR_THRESH] = 80;
      smb_sensor_threshold[SMB_SENSOR_TMP421_U63_TEMP][UCR_THRESH] = 80;
      smb_sensor_threshold[SMB_SENSOR_BMC_LM75B_TEMP][UCR_THRESH] = 80;
      smb_sensor_threshold[SMB_DOM1_MAX_TEMP][UCR_THRESH] = 65;
      smb_sensor_threshold[SMB_DOM2_MAX_TEMP][UCR_THRESH] = 65;

      smb_sensor_threshold[SMB_SENSOR_GB_HIGH_TEMP][UCR_THRESH] = 105;
      smb_sensor_threshold[SMB_SENSOR_GB_TEMP1][UCR_THRESH] = 105;
      smb_sensor_threshold[SMB_SENSOR_GB_TEMP2][UCR_THRESH] = 105;
      smb_sensor_threshold[SMB_SENSOR_GB_TEMP3][UCR_THRESH] = 105;
      smb_sensor_threshold[SMB_SENSOR_GB_TEMP4][UCR_THRESH] = 105;
      smb_sensor_threshold[SMB_SENSOR_GB_TEMP5][UCR_THRESH] = 105;
      smb_sensor_threshold[SMB_SENSOR_GB_TEMP6][UCR_THRESH] = 105;
      smb_sensor_threshold[SMB_SENSOR_GB_TEMP7][UCR_THRESH] = 105;
      smb_sensor_threshold[SMB_SENSOR_GB_TEMP8][UCR_THRESH] = 105;
      smb_sensor_threshold[SMB_SENSOR_GB_TEMP9][UCR_THRESH] = 105;
      smb_sensor_threshold[SMB_SENSOR_GB_TEMP10][UCR_THRESH] = 105;
      smb_sensor_threshold[SMB_SENSOR_GB_HBM_TEMP1][UCR_THRESH] = 105;
      smb_sensor_threshold[SMB_SENSOR_GB_HBM_TEMP2][UCR_THRESH] = 105;

      /* FCM BOARD */
      smb_sensor_threshold[SMB_SENSOR_FCM_LM75B_U1_TEMP][UCR_THRESH] = 80;
      smb_sensor_threshold[SMB_SENSOR_FCM_LM75B_U2_TEMP][UCR_THRESH] = 80;
      smb_sensor_threshold[SMB_SENSOR_FCM_HSC_IN_VOLT][UCR_THRESH] = 12.6;
      smb_sensor_threshold[SMB_SENSOR_FCM_HSC_IN_VOLT][LCR_THRESH] = 11.4;
      smb_sensor_threshold[SMB_SENSOR_FCM_HSC_IN_POWER][UCR_THRESH] = 302.4;
      smb_sensor_threshold[SMB_SENSOR_FCM_HSC_OUT_VOLT][UCR_THRESH] =  12.6;
      smb_sensor_threshold[SMB_SENSOR_FCM_HSC_OUT_VOLT][LCR_THRESH] = 11.4;
      smb_sensor_threshold[SMB_SENSOR_FCM_HSC_OUT_CURR][UCR_THRESH] =  24;
      smb_sensor_threshold[SMB_SENSOR_FAN1_FRONT_TACH][UCR_THRESH] = 14000;
      smb_sensor_threshold[SMB_SENSOR_FAN1_FRONT_TACH][LCR_THRESH] = 1000;
      smb_sensor_threshold[SMB_SENSOR_FAN1_REAR_TACH][UCR_THRESH] = 14000;
      smb_sensor_threshold[SMB_SENSOR_FAN1_REAR_TACH][LCR_THRESH] = 1000;
      smb_sensor_threshold[SMB_SENSOR_FAN2_FRONT_TACH][UCR_THRESH] = 14000;
      smb_sensor_threshold[SMB_SENSOR_FAN2_FRONT_TACH][LCR_THRESH] = 1000;
      smb_sensor_threshold[SMB_SENSOR_FAN2_REAR_TACH][UCR_THRESH] = 14000;
      smb_sensor_threshold[SMB_SENSOR_FAN2_REAR_TACH][LCR_THRESH] = 1000;
      smb_sensor_threshold[SMB_SENSOR_FAN3_FRONT_TACH][UCR_THRESH] = 14000;
      smb_sensor_threshold[SMB_SENSOR_FAN3_FRONT_TACH][LCR_THRESH] = 1000;
      smb_sensor_threshold[SMB_SENSOR_FAN3_REAR_TACH][UCR_THRESH] = 14000;
      smb_sensor_threshold[SMB_SENSOR_FAN3_REAR_TACH][LCR_THRESH] = 1000;
      smb_sensor_threshold[SMB_SENSOR_FAN4_FRONT_TACH][UCR_THRESH] = 14000;
      smb_sensor_threshold[SMB_SENSOR_FAN4_FRONT_TACH][LCR_THRESH] = 1000;
      smb_sensor_threshold[SMB_SENSOR_FAN4_REAR_TACH][UCR_THRESH] = 14000;
      smb_sensor_threshold[SMB_SENSOR_FAN4_REAR_TACH][LCR_THRESH] = 1000;
      break;
    case FRU_PSU1:
    case FRU_PSU2:
      fru_offset = fru - FRU_PSU1;
      psu_sensor_threshold[PSU1_SENSOR_IN_VOLT + (fru_offset * PSU1_SENSOR_CNT)][UCR_THRESH] = 264;
      psu_sensor_threshold[PSU1_SENSOR_IN_VOLT + (fru_offset * PSU1_SENSOR_CNT)][LCR_THRESH] = 180;
      psu_sensor_threshold[PSU1_SENSOR_12V_VOLT + (fru_offset * PSU1_SENSOR_CNT)][UCR_THRESH] = 12.6;
      psu_sensor_threshold[PSU1_SENSOR_12V_VOLT + (fru_offset * PSU1_SENSOR_CNT)][LCR_THRESH] = 11.4;
      psu_sensor_threshold[PSU1_SENSOR_STBY_VOLT + (fru_offset * PSU1_SENSOR_CNT)][UCR_THRESH] = 3.46;
      psu_sensor_threshold[PSU1_SENSOR_STBY_VOLT + (fru_offset * PSU1_SENSOR_CNT)][LCR_THRESH] = 3.2;
      psu_sensor_threshold[PSU1_SENSOR_IN_CURR + (fru_offset * PSU1_SENSOR_CNT)][UCR_THRESH] = 10.04;
      psu_sensor_threshold[PSU1_SENSOR_12V_CURR + (fru_offset * PSU1_SENSOR_CNT)][UCR_THRESH] = 166.7;
      psu_sensor_threshold[PSU1_SENSOR_STBY_CURR + (fru_offset * PSU1_SENSOR_CNT)][UCR_THRESH] = 3;
      psu_sensor_threshold[PSU1_SENSOR_IN_POWER + (fru_offset * PSU1_SENSOR_CNT)][UCR_THRESH] = 2208.9;
      psu_sensor_threshold[PSU1_SENSOR_12V_POWER + (fru_offset * PSU1_SENSOR_CNT)][UCR_THRESH] = 2100.42;
      psu_sensor_threshold[PSU1_SENSOR_STBY_POWER + (fru_offset * PSU1_SENSOR_CNT)][UCR_THRESH] = 10.38;
      psu_sensor_threshold[PSU1_SENSOR_FAN_TACH + (fru_offset * PSU1_SENSOR_CNT)][UCR_THRESH] = 26000;
      psu_sensor_threshold[PSU1_SENSOR_TEMP1 + (fru_offset * PSU1_SENSOR_CNT)][UCR_THRESH] = 65;
      psu_sensor_threshold[PSU1_SENSOR_TEMP2 + (fru_offset * PSU1_SENSOR_CNT)][UCR_THRESH] = 100;
      psu_sensor_threshold[PSU1_SENSOR_TEMP3 + (fru_offset * PSU1_SENSOR_CNT)][UCR_THRESH] = 125;
      break;
  }
  init_done[fru] = true;
}

int pal_get_sensor_threshold(uint8_t fru, uint8_t sensor_num,
                             uint8_t thresh, void *value) {
  float *val = (float*) value;

  sensor_thresh_array_init(fru);

  switch (fru) {
  case FRU_SCM:
    *val = scm_sensor_threshold[sensor_num][thresh];
    break;
  case FRU_SMB:
    *val = smb_sensor_threshold[sensor_num][thresh];
    break;
  case FRU_PSU1:
  case FRU_PSU2:
    *val = psu_sensor_threshold[sensor_num][thresh];
    break;
  default:
    return -1;
  }
  return 0;
}

int pal_get_fw_info(uint8_t fru, unsigned char target,
                    unsigned char* res, unsigned char* res_len) {
  return -1;
}

static sensor_desc_t *
get_sensor_desc(uint8_t fru, uint8_t snr_num) {
  if (fru < 1 || fru > MAX_NUM_FRUS) {
    OBMC_WARN("get_sensor_desc: Wrong FRU ID %d\n", fru);
    return NULL;
  }

  return &m_snr_desc[fru-1][snr_num];
}

void pal_sensor_assert_handle(uint8_t fru, uint8_t snr_num,
                              float val, uint8_t thresh) {
  char crisel[128];
  char thresh_name[10];
  sensor_desc_t *snr_desc;

  switch (thresh) {
    case UNR_THRESH:
        sprintf(thresh_name, "UNR");
      break;
    case UCR_THRESH:
        sprintf(thresh_name, "UCR");
      break;
    case UNC_THRESH:
        sprintf(thresh_name, "UNCR");
      break;
    case LNR_THRESH:
        sprintf(thresh_name, "LNR");
      break;
    case LCR_THRESH:
        sprintf(thresh_name, "LCR");
      break;
    case LNC_THRESH:
        sprintf(thresh_name, "LNCR");
      break;
    default:
      OBMC_WARN("pal_sensor_assert_handle: wrong thresh enum value");
      exit(-1);
  }

  switch (snr_num) {
    case BIC_SENSOR_P3V3_MB:
    case BIC_SENSOR_P12V_MB:
    case BIC_SENSOR_P1V05_PCH:
    case BIC_SENSOR_P3V3_STBY_MB:
    case BIC_SENSOR_PV_BAT:
    case BIC_SENSOR_PVDDR:
    case BIC_SENSOR_VCCIN_VR_VOL:
    case BIC_SENSOR_VDDR_VR_VOL:
    case BIC_SENSOR_P1V05COMB_VR_VOL:
    case BIC_SENSOR_INA230_VOL:
      snr_desc = get_sensor_desc(fru, snr_num);
      sprintf(crisel, "%s %s %.2fV - ASSERT,FRU:%u",
                      snr_desc->name, thresh_name, val, fru);
      break;
    default:
      return;
  }
  pal_add_cri_sel(crisel);
  return;
}

void pal_sensor_deassert_handle(uint8_t fru, uint8_t snr_num,
                                float val, uint8_t thresh) {
  char crisel[128];
  char thresh_name[8];
  sensor_desc_t *snr_desc;

  switch (thresh) {
    case UNR_THRESH:
      sprintf(thresh_name, "UNR");
      break;
    case UCR_THRESH:
      sprintf(thresh_name, "UCR");
      break;
    case UNC_THRESH:
      sprintf(thresh_name, "UNCR");
      break;
    case LNR_THRESH:
      sprintf(thresh_name, "LNR");
      break;
    case LCR_THRESH:
      sprintf(thresh_name, "LCR");
      break;
    case LNC_THRESH:
      sprintf(thresh_name, "LNCR");
      break;
    default:
      OBMC_WARN(
             "pal_sensor_deassert_handle: wrong thresh enum value");
      return;
  }
  switch (fru) {
    case FRU_SCM:
      switch (snr_num) {
        case BIC_SENSOR_SOC_TEMP:
          sprintf(crisel, "SOC Temp %s %.0fC - DEASSERT,FRU:%u",
                          thresh_name, val, fru);
          break;
        case BIC_SENSOR_P3V3_MB:
        case BIC_SENSOR_P12V_MB:
        case BIC_SENSOR_P1V05_PCH:
        case BIC_SENSOR_P3V3_STBY_MB:
        case BIC_SENSOR_PV_BAT:
        case BIC_SENSOR_PVDDR:
        case BIC_SENSOR_VCCIN_VR_VOL:
        case BIC_SENSOR_VDDR_VR_VOL:
        case BIC_SENSOR_P1V05COMB_VR_VOL:
        case BIC_SENSOR_INA230_VOL:
          snr_desc = get_sensor_desc(FRU_SCM, snr_num);
          sprintf(crisel, "%s %s %.2fV - DEASSERT,FRU:%u",
                          snr_desc->name, thresh_name, val, fru);
          break;
        default:
          return;
      }
      break;
  }
  pal_add_cri_sel(crisel);
  return;
}

int pal_sensor_threshold_flag(uint8_t fru, uint8_t snr_num, uint16_t *flag) {
  switch (fru) {
    case FRU_SCM:
      if (snr_num == BIC_SENSOR_SOC_THERM_MARGIN)
        *flag = GETMASK(SENSOR_VALID) | GETMASK(UCR_THRESH);
      else if (snr_num == BIC_SENSOR_SOC_PACKAGE_PWR)
        *flag = GETMASK(SENSOR_VALID);
      else if (snr_num == BIC_SENSOR_SOC_TJMAX)
        *flag = GETMASK(SENSOR_VALID);
      break;
    default:
      break;
  }
  return 0;
}

static void scm_sensor_poll_interval(uint8_t sensor_num, uint32_t *value) {
  switch (sensor_num) {
    case SCM_SENSOR_OUTLET_TEMP:
    case SCM_SENSOR_INLET_TEMP:
      *value = 30;
      break;
    case SCM_SENSOR_HSC_IN_VOLT:
    case SCM_SENSOR_HSC_OUT_VOLT:
    case SCM_SENSOR_HSC_OUT_CURR:
      *value = 30;
      break;
    case BIC_SENSOR_SOC_TEMP:
    case BIC_SENSOR_SOC_THERM_MARGIN:
    case BIC_SENSOR_SOC_TJMAX:
      *value = 2;
      break;
    default:
      *value = 30;
      break;
  }
}

static void smb_sensor_poll_interval(uint8_t sensor_num, uint32_t *value) {
  switch (sensor_num) {
    case SMB_SENSOR_SW_CORE_TEMP1:
    case SMB_SENSOR_XPDE_HBM_TEMP1:
    case SMB_SENSOR_VDDA_TEMP1:
    case SMB_SENSOR_PCIE_TEMP1:
    case SMB_SENSOR_IR3R3V_LEFT_TEMP:
    case SMB_SENSOR_IR3R3V_RIGHT_TEMP:
    case SMB_SENSOR_LM75B_U28_TEMP:
    case SMB_SENSOR_LM75B_U25_TEMP:
    case SMB_SENSOR_LM75B_U56_TEMP:
    case SMB_SENSOR_LM75B_U55_TEMP:
    case SMB_SENSOR_LM75B_U2_TEMP:
    case SMB_SENSOR_LM75B_U13_TEMP:
    case SMB_SENSOR_TMP421_U62_TEMP:
    case SMB_SENSOR_TMP421_U63_TEMP:
    case SMB_SENSOR_BMC_LM75B_TEMP:
    case SMB_SENSOR_FCM_LM75B_U1_TEMP:
    case SMB_SENSOR_FCM_LM75B_U2_TEMP:
    case SMB_DOM1_MAX_TEMP:
    case SMB_DOM2_MAX_TEMP:
    case SMB_SENSOR_VDDCK_0_TEMP:
    case SMB_SENSOR_VDDCK_1_TEMP:
    case SMB_SENSOR_VDDCK_2_TEMP:
    case SMB_SENSOR_XDPE_LEFT_1_TEMP:
    case SMB_SENSOR_XDPE_LEFT_2_TEMP:
    case SMB_SENSOR_XDPE_RIGHT_1_TEMP:
    case SMB_SENSOR_XDPE_RIGHT_2_TEMP:
    case SMB_SENSOR_GB_HIGH_TEMP:
    case SMB_SENSOR_GB_TEMP1:
    case SMB_SENSOR_GB_TEMP2:
    case SMB_SENSOR_GB_TEMP3:
    case SMB_SENSOR_GB_TEMP4:
    case SMB_SENSOR_GB_TEMP5:
    case SMB_SENSOR_GB_TEMP6:
    case SMB_SENSOR_GB_TEMP7:
    case SMB_SENSOR_GB_TEMP8:
    case SMB_SENSOR_GB_TEMP9:
    case SMB_SENSOR_GB_TEMP10:
    case SMB_SENSOR_GB_HBM_TEMP1:
    case SMB_SENSOR_GB_HBM_TEMP2:
      *value = 30;
      break;
    case SMB_SENSOR_1220_VMON1:
    case SMB_SENSOR_1220_VMON2:
    case SMB_SENSOR_1220_VMON3:
    case SMB_SENSOR_1220_VMON4:
    case SMB_SENSOR_1220_VMON5:
    case SMB_SENSOR_1220_VMON6:
    case SMB_SENSOR_1220_VMON7:
    case SMB_SENSOR_1220_VMON8:
    case SMB_SENSOR_1220_VMON9:
    case SMB_SENSOR_1220_VMON10:
    case SMB_SENSOR_1220_VMON11:
    case SMB_SENSOR_1220_VMON12:
    case SMB_SENSOR_1220_VCCA:
    case SMB_SENSOR_1220_VCCINP:
    case SMB_SENSOR_VDDA_IN_VOLT:
    case SMB_SENSOR_VDDA_IN_CURR:
    case SMB_SENSOR_VDDA_OUT_VOLT:
    case SMB_SENSOR_VDDA_OUT_CURR:
    case SMB_SENSOR_PCIE_IN_VOLT:
    case SMB_SENSOR_PCIE_IN_CURR:
    case SMB_SENSOR_PCIE_OUT_VOLT:
    case SMB_SENSOR_PCIE_OUT_CURR:
    case SMB_SENSOR_IR3R3V_LEFT_IN_VOLT:
    case SMB_SENSOR_IR3R3V_LEFT_IN_CURR:
    case SMB_SENSOR_IR3R3V_LEFT_OUT_VOLT:
    case SMB_SENSOR_IR3R3V_LEFT_OUT_CURR:
    case SMB_SENSOR_IR3R3V_RIGHT_IN_VOLT:
    case SMB_SENSOR_IR3R3V_RIGHT_IN_CURR:
    case SMB_SENSOR_IR3R3V_RIGHT_OUT_VOLT:
    case SMB_SENSOR_IR3R3V_RIGHT_OUT_CURR:
    case SMB_SENSOR_SW_CORE_IN_VOLT:
    case SMB_SENSOR_SW_CORE_OUT_VOLT:
    case SMB_SENSOR_XPDE_HBM_IN_VOLT:
    case SMB_SENSOR_XPDE_HBM_OUT_VOLT:
    case SMB_SENSOR_FCM_HSC_IN_VOLT:
    case SMB_SENSOR_FCM_HSC_OUT_VOLT:
    case SMB_SENSOR_SW_CORE_IN_CURR:
    case SMB_SENSOR_SW_CORE_OUT_CURR:
    case SMB_SENSOR_XPDE_HBM_IN_CURR:
    case SMB_SENSOR_XPDE_HBM_OUT_CURR:
    case SMB_SENSOR_FCM_HSC_OUT_CURR:
    case SMB_BMC_ADC0_VSEN:
    case SMB_BMC_ADC1_VSEN:
    case SMB_BMC_ADC2_VSEN:
    case SMB_BMC_ADC3_VSEN:
    case SMB_BMC_ADC4_VSEN:
    case SMB_BMC_ADC5_VSEN:
    case SMB_BMC_ADC6_VSEN:
    case SMB_BMC_ADC7_VSEN:
    case SMB_BMC_ADC8_VSEN:
    case SMB_BMC_ADC9_VSEN:
    case SMB_BMC_ADC10_VSEN:
    case SMB_BMC_ADC11_VSEN:
    case SMB_SENSOR_VDDCK_0_IN_VOLT:
    case SMB_SENSOR_VDDCK_0_OUT_VOLT:
    case SMB_SENSOR_VDDCK_0_IN_CURR:
    case SMB_SENSOR_VDDCK_0_OUT_CURR:
    case SMB_SENSOR_VDDCK_1_IN_VOLT:
    case SMB_SENSOR_VDDCK_1_OUT_VOLT:
    case SMB_SENSOR_VDDCK_1_IN_CURR:
    case SMB_SENSOR_VDDCK_1_OUT_CURR:
    case SMB_SENSOR_VDDCK_2_IN_VOLT:
    case SMB_SENSOR_VDDCK_2_OUT_VOLT:
    case SMB_SENSOR_VDDCK_2_IN_CURR:
    case SMB_SENSOR_VDDCK_2_OUT_CURR:
    case SMB_SENSOR_XDPE_LEFT_1_IN_VOLT:
    case SMB_SENSOR_XDPE_LEFT_1_IN_CURR:
    case SMB_SENSOR_XDPE_LEFT_1_OUT_VOLT:
    case SMB_SENSOR_XDPE_LEFT_1_OUT_CURR:
    case SMB_SENSOR_XDPE_LEFT_2_IN_VOLT:
    case SMB_SENSOR_XDPE_LEFT_2_IN_CURR:
    case SMB_SENSOR_XDPE_LEFT_2_OUT_VOLT:
    case SMB_SENSOR_XDPE_LEFT_2_OUT_CURR:
    case SMB_SENSOR_XDPE_RIGHT_1_IN_VOLT:
    case SMB_SENSOR_XDPE_RIGHT_1_IN_CURR:
    case SMB_SENSOR_XDPE_RIGHT_1_OUT_VOLT:
    case SMB_SENSOR_XDPE_RIGHT_1_OUT_CURR:
    case SMB_SENSOR_XDPE_RIGHT_2_IN_VOLT:
    case SMB_SENSOR_XDPE_RIGHT_2_IN_CURR:
    case SMB_SENSOR_XDPE_RIGHT_2_OUT_VOLT:
    case SMB_SENSOR_XDPE_RIGHT_2_OUT_CURR:
      *value = 30;
      break;
    case SMB_SENSOR_FAN1_FRONT_TACH:
    case SMB_SENSOR_FAN1_REAR_TACH:
    case SMB_SENSOR_FAN2_FRONT_TACH:
    case SMB_SENSOR_FAN2_REAR_TACH:
    case SMB_SENSOR_FAN3_FRONT_TACH:
    case SMB_SENSOR_FAN3_REAR_TACH:
    case SMB_SENSOR_FAN4_FRONT_TACH:
    case SMB_SENSOR_FAN4_REAR_TACH:
      *value = 2;
      break;
    default:
      *value = 10;
      break;
  }
}

static void psu_sensor_poll_interval(uint8_t sensor_num, uint32_t *value) {
  switch (sensor_num) {
    case PSU1_SENSOR_IN_VOLT:
    case PSU1_SENSOR_12V_VOLT:
    case PSU1_SENSOR_STBY_VOLT:
    case PSU1_SENSOR_IN_CURR:
    case PSU1_SENSOR_12V_CURR:
    case PSU1_SENSOR_STBY_CURR:
    case PSU1_SENSOR_IN_POWER:
    case PSU1_SENSOR_12V_POWER:
    case PSU1_SENSOR_STBY_POWER:
    case PSU1_SENSOR_FAN_TACH:
    case PSU1_SENSOR_TEMP1:
    case PSU1_SENSOR_TEMP2:
    case PSU1_SENSOR_TEMP3:
    case PSU2_SENSOR_IN_VOLT:
    case PSU2_SENSOR_12V_VOLT:
    case PSU2_SENSOR_STBY_VOLT:
    case PSU2_SENSOR_IN_CURR:
    case PSU2_SENSOR_12V_CURR:
    case PSU2_SENSOR_STBY_CURR:
    case PSU2_SENSOR_IN_POWER:
    case PSU2_SENSOR_12V_POWER:
    case PSU2_SENSOR_STBY_POWER:
    case PSU2_SENSOR_FAN_TACH:
    case PSU2_SENSOR_TEMP1:
    case PSU2_SENSOR_TEMP2:
    case PSU2_SENSOR_TEMP3:
      *value = 30;
      break;
    default:
      *value = 10;
      break;
  }
}

int pal_get_sensor_poll_interval(uint8_t fru, uint8_t sensor_num, uint32_t *value) {
  switch (fru) {
    case FRU_SCM:
      scm_sensor_poll_interval(sensor_num, value);
      break;
    case FRU_SMB:
      smb_sensor_poll_interval(sensor_num, value);
      break;
    case FRU_PSU1:
    case FRU_PSU2:
      psu_sensor_poll_interval(sensor_num, value);
      break;
    default:
      *value = 2;
      break;
  }
  return 0;
}
