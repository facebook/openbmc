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
  * This file contains functions and logics that depends on Wedge100 specific
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
#include "pal.h"
#include "pal_sensors.h"
#include <math.h>
#include <facebook/bic.h>
#include <openbmc/obmc-i2c.h>
#include <openbmc/misc-utils.h>
#include <openbmc/log.h>

#define RUN_SHELL_CMD(_cmd)                              \
  do {                                                   \
    int _ret = system(_cmd);                             \
    if (_ret != 0)                                       \
      OBMC_WARN("'%s' command returned %d", _cmd, _ret); \
  } while (0)

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

typedef struct {
  char name[NAME_MAX];
} sensor_path_t;

static sensor_desc_t m_snr_desc[MAX_NUM_FRUS][MAX_SENSOR_NUM] = {0};
static sensor_path_t snr_path[MAX_NUM_FRUS][MAX_SENSOR_NUM] = {0};
static uint8_t sdr_fru_update_flag[MAX_NUM_FRUS] = {0};
static bool init_threshold_done[MAX_NUM_FRUS+1] = {false};

// List of BIC sensors which need to do negative reading handle
const uint8_t bic_neg_reading_sensor_support_list[] = {
  /* Temperature sensors*/
  BIC_SENSOR_MB_OUTLET_TEMP,
  BIC_SENSOR_MB_INLET_TEMP,
  BIC_SENSOR_PCH_TEMP,
  BIC_SENSOR_SOC_TEMP,
  BIC_SENSOR_SOC_DIMMA0_TEMP,
  BIC_SENSOR_SOC_DIMMB0_TEMP,
  BIC_SENSOR_VCCIN_VR_CURR,
};

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

/* List of SCM sensors to be monitored */
const uint8_t scm_sensor_list[] = {
  SCM_SENSOR_OUTLET_U7_TEMP,
  SCM_SENSOR_INLET_U8_TEMP,
  SCM_SENSOR_HSC_VOLT,
  SCM_SENSOR_POWER_VOLT,
  SCM_SENSOR_HSC_CURR,
  SCM_SENSOR_HSC_POWER,
  SCM_SENSOR_BMC_LM75_U9_TEMP,
};

/* List of SCM and BIC sensors to be monitored */
const uint8_t scm_all_sensor_list[] = {
  SCM_SENSOR_OUTLET_U7_TEMP,
  SCM_SENSOR_INLET_U8_TEMP,
  SCM_SENSOR_HSC_VOLT,
  SCM_SENSOR_POWER_VOLT,
  SCM_SENSOR_HSC_CURR,
  SCM_SENSOR_HSC_POWER,
  SCM_SENSOR_BMC_LM75_U9_TEMP,
  BIC_SENSOR_MB_OUTLET_TEMP,
  BIC_SENSOR_MB_INLET_TEMP,
  BIC_SENSOR_PCH_TEMP,
  BIC_SENSOR_VCCIN_VR_TEMP,
  BIC_SENSOR_1V05MIX_VR_TEMP,
  BIC_SENSOR_SOC_TEMP,
  BIC_SENSOR_SOC_THERM_MARGIN,
  BIC_SENSOR_VDDR_VR_TEMP,
  BIC_SENSOR_SOC_DIMMA0_TEMP,
  BIC_SENSOR_SOC_DIMMB0_TEMP,
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
  BIC_SENSOR_P1V05_MIX,
  BIC_SENSOR_1V05MIX_VR_CURR,
  BIC_SENSOR_VDDR_VR_CURR,
  BIC_SENSOR_VCCIN_VR_CURR,
  BIC_SENSOR_VCCIN_VR_VOL,
  BIC_SENSOR_VDDR_VR_VOL,
  BIC_SENSOR_P1V05MIX_VR_VOL,
  BIC_SENSOR_P1V05MIX_VR_POUT,
  BIC_SENSOR_INA230_POWER,
  BIC_SENSOR_INA230_VOL,
};

/* List of SMB sensors to be monitored */
const uint8_t smb_sensor_list_evt[] = {
  SMB_XP3R3V_BMC,
  SMB_XP2R5V_BMC,
  SMB_XP1R8V_BMC,
  SMB_XP1R2V_BMC,
  SMB_XP1R0V_FPGA,
  SMB_XP3R3V_USB,
  SMB_XP5R0V,
  SMB_XP3R3V_EARLY,
  SMB_LM57_VTEMP,
  SMB_XP1R8,
  SMB_XP1R2,
  SMB_VDDC_SW,
  SMB_XP3R3V,
  SMB_XP1R8V_AVDD,
  SMB_XP1R2V_TVDD,
  SMB_XP0R75V_1_PVDD,
  SMB_XP0R75V_2_PVDD,
  SMB_XP0R75V_3_PVDD,
  SMB_VDD_PCIE,
  SMB_XP0R84V_DCSU,
  SMB_XP0R84V_CSU,
  SMB_XP1R84V_CSU,
  SMB_XP3R3V_TCXO,
  SMB_OUTPUT_VOLTAGE_XP0R75V_1,
  SMB_OUTPUT_CURRENT_XP0R75V_1,
  SMB_INPUT_VOLTAGE_1,
  SMB_OUTPUT_VOLTAGE_XP1R2V,
  SMB_OUTPUT_CURRENT_XP1R2V,
  SMB_OUTPUT_VOLTAGE_XP0R75V_2,
  SMB_OUTPUT_CURRENT_XP0R75V_2,
  SMB_INPUT_VOLTAGE_2,
  SMB_TMP422_U20_1_TEMP,
  SMB_TMP422_U20_2_TEMP,
  SMB_TMP422_U20_3_TEMP,
  SIM_LM75_U1_TEMP,
  SMB_SENSOR_TEMP1,
  SMB_SENSOR_TEMP2,
  SMB_SENSOR_TEMP3,
  SMB_VDDC_SW_TEMP,
  SMB_XP12R0V_VDDC_SW_IN,
  SMB_VDDC_SW_IN_SENS,
  SMB_VDDC_SW_POWER_IN,
  SMB_VDDC_SW_POWER_OUT,
  SMB_VDDC_SW_CURR_IN,
  SMB_VDDC_SW_CURR_OUT,
  /* Sensors on PDB */
  SMB_SENSOR_PDB_L_TEMP1,
  SMB_SENSOR_PDB_L_TEMP2,
  SMB_SENSOR_PDB_R_TEMP1,
  SMB_SENSOR_PDB_R_TEMP2,
  /* Sensors on FCM */
  SMB_SENSOR_FCM_T_TEMP1,
  SMB_SENSOR_FCM_T_TEMP2,
  SMB_SENSOR_FCM_B_TEMP1,
  SMB_SENSOR_FCM_B_TEMP2,
  SMB_SENSOR_FCM_T_HSC_VOLT,
  SMB_SENSOR_FCM_T_HSC_CURR,
  SMB_SENSOR_FCM_T_HSC_POWER_VOLT,
  SMB_SENSOR_FCM_B_HSC_VOLT,
  SMB_SENSOR_FCM_B_HSC_CURR,
  SMB_SENSOR_FCM_B_HSC_POWER_VOLT,
  /* Sensors FAN Speed */
  SMB_SENSOR_FAN1_FRONT_TACH,
  SMB_SENSOR_FAN1_REAR_TACH,
  SMB_SENSOR_FAN2_FRONT_TACH,
  SMB_SENSOR_FAN2_REAR_TACH,
  SMB_SENSOR_FAN3_FRONT_TACH,
  SMB_SENSOR_FAN3_REAR_TACH,
  SMB_SENSOR_FAN4_FRONT_TACH,
  SMB_SENSOR_FAN4_REAR_TACH,
  SMB_SENSOR_FAN5_FRONT_TACH,
  SMB_SENSOR_FAN5_REAR_TACH,
  SMB_SENSOR_FAN6_FRONT_TACH,
  SMB_SENSOR_FAN6_REAR_TACH,
  SMB_SENSOR_FAN7_FRONT_TACH,
  SMB_SENSOR_FAN7_REAR_TACH,
  SMB_SENSOR_FAN8_FRONT_TACH,
  SMB_SENSOR_FAN8_REAR_TACH,
};

const uint8_t smb_sensor_list_dvt[] = {
  SMB_XP3R3V_BMC,
  SMB_XP2R5V_BMC,
  SMB_XP1R8V_BMC,
  SMB_XP1R2V_BMC,
  SMB_XP1R0V_FPGA,
  SMB_XP3R3V_USB,
  SMB_XP5R0V,
  SMB_XP3R3V_EARLY,
  SMB_LM57_VTEMP,
  SMB_XP1R8,
  SMB_XP1R2,
  SMB_VDDC_SW,
  SMB_XP3R3V,
  SMB_XP1R8V_AVDD,
  SMB_XP1R2V_TVDD,
  SMB_XP0R75V_1_PVDD,
  SMB_XP0R75V_2_PVDD,
  SMB_XP0R75V_3_PVDD,
  SMB_VDD_PCIE,
  SMB_XP0R84V_DCSU,
  SMB_XP0R84V_CSU,
  SMB_XP1R84V_CSU,
  SMB_XP3R3V_TCXO,
  SMB_OUTPUT_VOLTAGE_XP0R75V_1,
  SMB_OUTPUT_CURRENT_XP0R75V_1,
  SMB_INPUT_VOLTAGE_1,
  SMB_OUTPUT_VOLTAGE_XP1R2V,
  SMB_OUTPUT_CURRENT_XP1R2V,
  SMB_OUTPUT_VOLTAGE_XP0R75V_2,
  SMB_OUTPUT_CURRENT_XP0R75V_2,
  SMB_INPUT_VOLTAGE_2,
  SMB_TMP422_U20_1_TEMP,
  SMB_TMP422_U20_2_TEMP,
  SMB_TMP422_U20_3_TEMP,
  SIM_LM75_U1_TEMP,
  SMB_SENSOR_TEMP1,
  SMB_SENSOR_TEMP2,
  SMB_SENSOR_TEMP3,
  SMB_VDDC_SW_TEMP,
  SMB_XP12R0V_VDDC_SW_IN,
  SMB_VDDC_SW_IN_SENS,
  SMB_VDDC_SW_POWER_IN,
  SMB_VDDC_SW_POWER_OUT,
  SMB_VDDC_SW_CURR_IN,
  SMB_VDDC_SW_CURR_OUT,
  /* Sensors on PDB */
  SMB_SENSOR_PDB_L_TEMP1,
  SMB_SENSOR_PDB_L_TEMP2,
  SMB_SENSOR_PDB_R_TEMP1,
  SMB_SENSOR_PDB_R_TEMP2,
  /* Sensors on FCM */
  SMB_SENSOR_FCM_T_TEMP1,
  SMB_SENSOR_FCM_T_TEMP2,
  SMB_SENSOR_FCM_B_TEMP1,
  SMB_SENSOR_FCM_B_TEMP2,
  SMB_SENSOR_FCM_T_HSC_VOLT,
  SMB_SENSOR_FCM_T_HSC_CURR,
  SMB_SENSOR_FCM_T_HSC_POWER_VOLT,
  SMB_SENSOR_FCM_B_HSC_VOLT,
  SMB_SENSOR_FCM_B_HSC_CURR,
  SMB_SENSOR_FCM_B_HSC_POWER_VOLT,
  /* Sensors FAN Speed */
  SMB_SENSOR_FAN1_FRONT_TACH,
  SMB_SENSOR_FAN1_REAR_TACH,
  SMB_SENSOR_FAN2_FRONT_TACH,
  SMB_SENSOR_FAN2_REAR_TACH,
  SMB_SENSOR_FAN3_FRONT_TACH,
  SMB_SENSOR_FAN3_REAR_TACH,
  SMB_SENSOR_FAN4_FRONT_TACH,
  SMB_SENSOR_FAN4_REAR_TACH,
  SMB_SENSOR_FAN5_FRONT_TACH,
  SMB_SENSOR_FAN5_REAR_TACH,
  SMB_SENSOR_FAN6_FRONT_TACH,
  SMB_SENSOR_FAN6_REAR_TACH,
  SMB_SENSOR_FAN7_FRONT_TACH,
  SMB_SENSOR_FAN7_REAR_TACH,
  SMB_SENSOR_FAN8_FRONT_TACH,
  SMB_SENSOR_FAN8_REAR_TACH,
  SMB_SENSOR_TH4_HIGH,
};

const uint8_t pim16q_sensor_list[8][PIM_SENSOR_CNT+1] = {
  {
    PIM1_LM75_TEMP_4A,
    PIM1_LM75_TEMP_4B,
    PIM1_LM75_TEMP_48,
    PIM1_SENSOR_QSFP_TEMP,
    PIM1_SENSOR_HSC_VOLT,
    PIM1_POWER_VOLTAGE,
    PIM1_SENSOR_HSC_CURR,
    PIM1_SENSOR_HSC_POWER,
    PIM1_UCD90160_VOLT1,
    PIM1_UCD90160_VOLT2,
    PIM1_UCD90160_VOLT3,
    PIM1_UCD90160_VOLT4,
    PIM1_UCD90160_VOLT5,
    PIM1_UCD90160_VOLT6,
    PIM1_UCD90160_VOLT7,
    PIM1_UCD90160_VOLT8,
    PIM1_UCD90160_VOLT9,
    PIM1_UCD90160_VOLT10,
    PIM1_UCD90160_VOLT11,
    PIM1_UCD90160_VOLT12,
    PIM1_UCD90160_VOLT13,
    PIM1_MP2975_INPUT_VOLTAGE,
    PIM1_MP2975_OUTPUT_VOLTAGE_XP0R8V,
    PIM1_MP2975_OUTPUT_CURRENT_XP0R8V,
    PIM1_MP2975_OUTPUT_VOLTAGE_XP3R3V,
    PIM1_MP2975_OUTPUT_CURRENT_XP3R3V
  },{
    PIM2_LM75_TEMP_4A,
    PIM2_LM75_TEMP_4B,
    PIM2_LM75_TEMP_48,
    PIM2_SENSOR_QSFP_TEMP,
    PIM2_SENSOR_HSC_VOLT,
    PIM2_POWER_VOLTAGE,
    PIM2_SENSOR_HSC_CURR,
    PIM2_SENSOR_HSC_POWER,
    PIM2_UCD90160_VOLT1,
    PIM2_UCD90160_VOLT2,
    PIM2_UCD90160_VOLT3,
    PIM2_UCD90160_VOLT4,
    PIM2_UCD90160_VOLT5,
    PIM2_UCD90160_VOLT6,
    PIM2_UCD90160_VOLT7,
    PIM2_UCD90160_VOLT8,
    PIM2_UCD90160_VOLT9,
    PIM2_UCD90160_VOLT10,
    PIM2_UCD90160_VOLT11,
    PIM2_UCD90160_VOLT12,
    PIM2_UCD90160_VOLT13,
    PIM2_MP2975_INPUT_VOLTAGE,
    PIM2_MP2975_OUTPUT_VOLTAGE_XP0R8V,
    PIM2_MP2975_OUTPUT_CURRENT_XP0R8V,
    PIM2_MP2975_OUTPUT_VOLTAGE_XP3R3V,
    PIM2_MP2975_OUTPUT_CURRENT_XP3R3V
  },{
    PIM3_LM75_TEMP_4A,
    PIM3_LM75_TEMP_4B,
    PIM3_LM75_TEMP_48,
    PIM3_SENSOR_QSFP_TEMP,
    PIM3_SENSOR_HSC_VOLT,
    PIM3_POWER_VOLTAGE,
    PIM3_SENSOR_HSC_CURR,
    PIM3_SENSOR_HSC_POWER,
    PIM3_UCD90160_VOLT1,
    PIM3_UCD90160_VOLT2,
    PIM3_UCD90160_VOLT3,
    PIM3_UCD90160_VOLT4,
    PIM3_UCD90160_VOLT5,
    PIM3_UCD90160_VOLT6,
    PIM3_UCD90160_VOLT7,
    PIM3_UCD90160_VOLT8,
    PIM3_UCD90160_VOLT9,
    PIM3_UCD90160_VOLT10,
    PIM3_UCD90160_VOLT11,
    PIM3_UCD90160_VOLT12,
    PIM3_UCD90160_VOLT13,
    PIM3_MP2975_INPUT_VOLTAGE,
    PIM3_MP2975_OUTPUT_VOLTAGE_XP0R8V,
    PIM3_MP2975_OUTPUT_CURRENT_XP0R8V,
    PIM3_MP2975_OUTPUT_VOLTAGE_XP3R3V,
    PIM3_MP2975_OUTPUT_CURRENT_XP3R3V
  },{
    PIM4_LM75_TEMP_4A,
    PIM4_LM75_TEMP_4B,
    PIM4_LM75_TEMP_48,
    PIM4_SENSOR_QSFP_TEMP,
    PIM4_SENSOR_HSC_VOLT,
    PIM4_POWER_VOLTAGE,
    PIM4_SENSOR_HSC_CURR,
    PIM4_SENSOR_HSC_POWER,
    PIM4_UCD90160_VOLT1,
    PIM4_UCD90160_VOLT2,
    PIM4_UCD90160_VOLT3,
    PIM4_UCD90160_VOLT4,
    PIM4_UCD90160_VOLT5,
    PIM4_UCD90160_VOLT6,
    PIM4_UCD90160_VOLT7,
    PIM4_UCD90160_VOLT8,
    PIM4_UCD90160_VOLT9,
    PIM4_UCD90160_VOLT10,
    PIM4_UCD90160_VOLT11,
    PIM4_UCD90160_VOLT12,
    PIM4_UCD90160_VOLT13,
    PIM4_MP2975_INPUT_VOLTAGE,
    PIM4_MP2975_OUTPUT_VOLTAGE_XP0R8V,
    PIM4_MP2975_OUTPUT_CURRENT_XP0R8V,
    PIM4_MP2975_OUTPUT_VOLTAGE_XP3R3V,
    PIM4_MP2975_OUTPUT_CURRENT_XP3R3V
  },{
    PIM5_LM75_TEMP_4A,
    PIM5_LM75_TEMP_4B,
    PIM5_LM75_TEMP_48,
    PIM5_SENSOR_QSFP_TEMP,
    PIM5_SENSOR_HSC_VOLT,
    PIM5_POWER_VOLTAGE,
    PIM5_SENSOR_HSC_CURR,
    PIM5_SENSOR_HSC_POWER,
    PIM5_UCD90160_VOLT1,
    PIM5_UCD90160_VOLT2,
    PIM5_UCD90160_VOLT3,
    PIM5_UCD90160_VOLT4,
    PIM5_UCD90160_VOLT5,
    PIM5_UCD90160_VOLT6,
    PIM5_UCD90160_VOLT7,
    PIM5_UCD90160_VOLT8,
    PIM5_UCD90160_VOLT9,
    PIM5_UCD90160_VOLT10,
    PIM5_UCD90160_VOLT11,
    PIM5_UCD90160_VOLT12,
    PIM5_UCD90160_VOLT13,
    PIM5_MP2975_INPUT_VOLTAGE,
    PIM5_MP2975_OUTPUT_VOLTAGE_XP0R8V,
    PIM5_MP2975_OUTPUT_CURRENT_XP0R8V,
    PIM5_MP2975_OUTPUT_VOLTAGE_XP3R3V,
    PIM5_MP2975_OUTPUT_CURRENT_XP3R3V
  },{
    PIM6_LM75_TEMP_4A,
    PIM6_LM75_TEMP_4B,
    PIM6_LM75_TEMP_48,
    PIM6_SENSOR_QSFP_TEMP,
    PIM6_SENSOR_HSC_VOLT,
    PIM6_POWER_VOLTAGE,
    PIM6_SENSOR_HSC_CURR,
    PIM6_SENSOR_HSC_POWER,
    PIM6_UCD90160_VOLT1,
    PIM6_UCD90160_VOLT2,
    PIM6_UCD90160_VOLT3,
    PIM6_UCD90160_VOLT4,
    PIM6_UCD90160_VOLT5,
    PIM6_UCD90160_VOLT6,
    PIM6_UCD90160_VOLT7,
    PIM6_UCD90160_VOLT8,
    PIM6_UCD90160_VOLT9,
    PIM6_UCD90160_VOLT10,
    PIM6_UCD90160_VOLT11,
    PIM6_UCD90160_VOLT12,
    PIM6_UCD90160_VOLT13,
    PIM6_MP2975_INPUT_VOLTAGE,
    PIM6_MP2975_OUTPUT_VOLTAGE_XP0R8V,
    PIM6_MP2975_OUTPUT_CURRENT_XP0R8V,
    PIM6_MP2975_OUTPUT_VOLTAGE_XP3R3V,
    PIM6_MP2975_OUTPUT_CURRENT_XP3R3V
  },{
    PIM7_LM75_TEMP_4A,
    PIM7_LM75_TEMP_4B,
    PIM7_LM75_TEMP_48,
    PIM7_SENSOR_QSFP_TEMP,
    PIM7_SENSOR_HSC_VOLT,
    PIM7_POWER_VOLTAGE,
    PIM7_SENSOR_HSC_CURR,
    PIM7_SENSOR_HSC_POWER,
    PIM7_UCD90160_VOLT1,
    PIM7_UCD90160_VOLT2,
    PIM7_UCD90160_VOLT3,
    PIM7_UCD90160_VOLT4,
    PIM7_UCD90160_VOLT5,
    PIM7_UCD90160_VOLT6,
    PIM7_UCD90160_VOLT7,
    PIM7_UCD90160_VOLT8,
    PIM7_UCD90160_VOLT9,
    PIM7_UCD90160_VOLT10,
    PIM7_UCD90160_VOLT11,
    PIM7_UCD90160_VOLT12,
    PIM7_UCD90160_VOLT13,
    PIM7_MP2975_INPUT_VOLTAGE,
    PIM7_MP2975_OUTPUT_VOLTAGE_XP0R8V,
    PIM7_MP2975_OUTPUT_CURRENT_XP0R8V,
    PIM7_MP2975_OUTPUT_VOLTAGE_XP3R3V,
    PIM7_MP2975_OUTPUT_CURRENT_XP3R3V
  },{
    PIM8_LM75_TEMP_4A,
    PIM8_LM75_TEMP_4B,
    PIM8_LM75_TEMP_48,
    PIM8_SENSOR_QSFP_TEMP,
    PIM8_SENSOR_HSC_VOLT,
    PIM8_POWER_VOLTAGE,
    PIM8_SENSOR_HSC_CURR,
    PIM8_SENSOR_HSC_POWER,
    PIM8_UCD90160_VOLT1,
    PIM8_UCD90160_VOLT2,
    PIM8_UCD90160_VOLT3,
    PIM8_UCD90160_VOLT4,
    PIM8_UCD90160_VOLT5,
    PIM8_UCD90160_VOLT6,
    PIM8_UCD90160_VOLT7,
    PIM8_UCD90160_VOLT8,
    PIM8_UCD90160_VOLT9,
    PIM8_UCD90160_VOLT10,
    PIM8_UCD90160_VOLT11,
    PIM8_UCD90160_VOLT12,
    PIM8_UCD90160_VOLT13,
    PIM8_MP2975_INPUT_VOLTAGE,
    PIM8_MP2975_OUTPUT_VOLTAGE_XP0R8V,
    PIM8_MP2975_OUTPUT_CURRENT_XP0R8V,
    PIM8_MP2975_OUTPUT_VOLTAGE_XP3R3V,
    PIM8_MP2975_OUTPUT_CURRENT_XP3R3V
  },
};

const uint8_t pim16o_sensor_list[8][19] = {
  {
    PIM1_LM75_TEMP_4B,
    PIM1_LM75_TEMP_48,
    PIM1_SENSOR_QSFP_TEMP,
    PIM1_SENSOR_HSC_VOLT,
    PIM1_POWER_VOLTAGE,
    PIM1_SENSOR_HSC_CURR,
    PIM1_SENSOR_HSC_POWER,
    PIM1_UCD90160_VOLT1,
    PIM1_UCD90160_VOLT2,
    PIM1_UCD90160_VOLT3,
    PIM1_UCD90160_VOLT4,
    PIM1_UCD90160_VOLT5,
    PIM1_UCD90160_VOLT6,
    PIM1_UCD90160_VOLT7,
    PIM1_UCD90160_VOLT8,
    PIM1_UCD90160_VOLT9,
    PIM1_UCD90160_VOLT10,
    PIM1_UCD90160_VOLT11,
    PIM1_UCD90160_VOLT12,
  },{
    PIM2_LM75_TEMP_4B,
    PIM2_LM75_TEMP_48,
    PIM2_SENSOR_QSFP_TEMP,
    PIM2_SENSOR_HSC_VOLT,
    PIM2_POWER_VOLTAGE,
    PIM2_SENSOR_HSC_CURR,
    PIM2_SENSOR_HSC_POWER,
    PIM2_UCD90160_VOLT1,
    PIM2_UCD90160_VOLT2,
    PIM2_UCD90160_VOLT3,
    PIM2_UCD90160_VOLT4,
    PIM2_UCD90160_VOLT5,
    PIM2_UCD90160_VOLT6,
    PIM2_UCD90160_VOLT7,
    PIM2_UCD90160_VOLT8,
    PIM2_UCD90160_VOLT9,
    PIM2_UCD90160_VOLT10,
    PIM2_UCD90160_VOLT11,
    PIM2_UCD90160_VOLT12,
  },{
    PIM3_LM75_TEMP_4B,
    PIM3_LM75_TEMP_48,
    PIM3_SENSOR_QSFP_TEMP,
    PIM3_SENSOR_HSC_VOLT,
    PIM3_POWER_VOLTAGE,
    PIM3_SENSOR_HSC_CURR,
    PIM3_SENSOR_HSC_POWER,
    PIM3_UCD90160_VOLT1,
    PIM3_UCD90160_VOLT2,
    PIM3_UCD90160_VOLT3,
    PIM3_UCD90160_VOLT4,
    PIM3_UCD90160_VOLT5,
    PIM3_UCD90160_VOLT6,
    PIM3_UCD90160_VOLT7,
    PIM3_UCD90160_VOLT8,
    PIM3_UCD90160_VOLT9,
    PIM3_UCD90160_VOLT10,
    PIM3_UCD90160_VOLT11,
    PIM3_UCD90160_VOLT12,
  },{
    PIM4_LM75_TEMP_4B,
    PIM4_LM75_TEMP_48,
    PIM4_SENSOR_QSFP_TEMP,
    PIM4_SENSOR_HSC_VOLT,
    PIM4_POWER_VOLTAGE,
    PIM4_SENSOR_HSC_CURR,
    PIM4_SENSOR_HSC_POWER,
    PIM4_UCD90160_VOLT1,
    PIM4_UCD90160_VOLT2,
    PIM4_UCD90160_VOLT3,
    PIM4_UCD90160_VOLT4,
    PIM4_UCD90160_VOLT5,
    PIM4_UCD90160_VOLT6,
    PIM4_UCD90160_VOLT7,
    PIM4_UCD90160_VOLT8,
    PIM4_UCD90160_VOLT9,
    PIM4_UCD90160_VOLT10,
    PIM4_UCD90160_VOLT11,
    PIM4_UCD90160_VOLT12,
  },{
    PIM5_LM75_TEMP_4B,
    PIM5_LM75_TEMP_48,
    PIM5_SENSOR_QSFP_TEMP,
    PIM5_SENSOR_HSC_VOLT,
    PIM5_POWER_VOLTAGE,
    PIM5_SENSOR_HSC_CURR,
    PIM5_SENSOR_HSC_POWER,
    PIM5_UCD90160_VOLT1,
    PIM5_UCD90160_VOLT2,
    PIM5_UCD90160_VOLT3,
    PIM5_UCD90160_VOLT4,
    PIM5_UCD90160_VOLT5,
    PIM5_UCD90160_VOLT6,
    PIM5_UCD90160_VOLT7,
    PIM5_UCD90160_VOLT8,
    PIM5_UCD90160_VOLT9,
    PIM5_UCD90160_VOLT10,
    PIM5_UCD90160_VOLT11,
    PIM5_UCD90160_VOLT12,
  },{
    PIM6_LM75_TEMP_4B,
    PIM6_LM75_TEMP_48,
    PIM6_SENSOR_QSFP_TEMP,
    PIM6_SENSOR_HSC_VOLT,
    PIM6_POWER_VOLTAGE,
    PIM6_SENSOR_HSC_CURR,
    PIM6_SENSOR_HSC_POWER,
    PIM6_UCD90160_VOLT1,
    PIM6_UCD90160_VOLT2,
    PIM6_UCD90160_VOLT3,
    PIM6_UCD90160_VOLT4,
    PIM6_UCD90160_VOLT5,
    PIM6_UCD90160_VOLT6,
    PIM6_UCD90160_VOLT7,
    PIM6_UCD90160_VOLT8,
    PIM6_UCD90160_VOLT9,
    PIM6_UCD90160_VOLT10,
    PIM6_UCD90160_VOLT11,
    PIM6_UCD90160_VOLT12,
  },{
    PIM7_LM75_TEMP_4B,
    PIM7_LM75_TEMP_48,
    PIM7_SENSOR_QSFP_TEMP,
    PIM7_SENSOR_HSC_VOLT,
    PIM7_POWER_VOLTAGE,
    PIM7_SENSOR_HSC_CURR,
    PIM7_SENSOR_HSC_POWER,
    PIM7_UCD90160_VOLT1,
    PIM7_UCD90160_VOLT2,
    PIM7_UCD90160_VOLT3,
    PIM7_UCD90160_VOLT4,
    PIM7_UCD90160_VOLT5,
    PIM7_UCD90160_VOLT6,
    PIM7_UCD90160_VOLT7,
    PIM7_UCD90160_VOLT8,
    PIM7_UCD90160_VOLT9,
    PIM7_UCD90160_VOLT10,
    PIM7_UCD90160_VOLT11,
    PIM7_UCD90160_VOLT12,
  },{
    PIM8_LM75_TEMP_4B,
    PIM8_LM75_TEMP_48,
    PIM8_SENSOR_QSFP_TEMP,
    PIM8_SENSOR_HSC_VOLT,
    PIM8_POWER_VOLTAGE,
    PIM8_SENSOR_HSC_CURR,
    PIM8_SENSOR_HSC_POWER,
    PIM8_UCD90160_VOLT1,
    PIM8_UCD90160_VOLT2,
    PIM8_UCD90160_VOLT3,
    PIM8_UCD90160_VOLT4,
    PIM8_UCD90160_VOLT5,
    PIM8_UCD90160_VOLT6,
    PIM8_UCD90160_VOLT7,
    PIM8_UCD90160_VOLT8,
    PIM8_UCD90160_VOLT9,
    PIM8_UCD90160_VOLT10,
    PIM8_UCD90160_VOLT11,
    PIM8_UCD90160_VOLT12,
  },
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

const uint8_t psu3_sensor_list[] = {
  PSU3_SENSOR_IN_VOLT,
  PSU3_SENSOR_12V_VOLT,
  PSU3_SENSOR_STBY_VOLT,
  PSU3_SENSOR_IN_CURR,
  PSU3_SENSOR_12V_CURR,
  PSU3_SENSOR_STBY_CURR,
  PSU3_SENSOR_IN_POWER,
  PSU3_SENSOR_12V_POWER,
  PSU3_SENSOR_STBY_POWER,
  PSU3_SENSOR_FAN_TACH,
  PSU3_SENSOR_TEMP1,
  PSU3_SENSOR_TEMP2,
  PSU3_SENSOR_TEMP3,
};

const uint8_t psu4_sensor_list[] = {
  PSU4_SENSOR_IN_VOLT,
  PSU4_SENSOR_12V_VOLT,
  PSU4_SENSOR_STBY_VOLT,
  PSU4_SENSOR_IN_CURR,
  PSU4_SENSOR_12V_CURR,
  PSU4_SENSOR_STBY_CURR,
  PSU4_SENSOR_IN_POWER,
  PSU4_SENSOR_12V_POWER,
  PSU4_SENSOR_STBY_POWER,
  PSU4_SENSOR_FAN_TACH,
  PSU4_SENSOR_TEMP1,
  PSU4_SENSOR_TEMP2,
  PSU4_SENSOR_TEMP3,
};

float scm_sensor_threshold[MAX_SENSOR_NUM][MAX_SENSOR_THRESHOLD + 1] = {0};
float smb_sensor_threshold[MAX_SENSOR_NUM][MAX_SENSOR_THRESHOLD + 1] = {0};
float pim_sensor_threshold[MAX_SENSOR_NUM][MAX_SENSOR_THRESHOLD + 1] = {0};
float psu_sensor_threshold[MAX_SENSOR_NUM][MAX_SENSOR_THRESHOLD + 1] = {0};

size_t bic_discrete_cnt = sizeof(bic_discrete_list)/sizeof(uint8_t);
size_t scm_sensor_cnt = sizeof(scm_sensor_list)/sizeof(uint8_t);
size_t scm_all_sensor_cnt = sizeof(scm_all_sensor_list)/sizeof(uint8_t);
size_t smb_sensor_cnt_evt = sizeof(smb_sensor_list_evt)/sizeof(uint8_t);
size_t smb_sensor_cnt_dvt = sizeof(smb_sensor_list_dvt)/sizeof(uint8_t);
size_t psu1_sensor_cnt = sizeof(psu1_sensor_list)/sizeof(uint8_t);
size_t psu2_sensor_cnt = sizeof(psu2_sensor_list)/sizeof(uint8_t);
size_t psu3_sensor_cnt = sizeof(psu3_sensor_list)/sizeof(uint8_t);
size_t psu4_sensor_cnt = sizeof(psu4_sensor_list)/sizeof(uint8_t);

static float hsc_rsense[MAX_NUM_FRUS] = {0};
static int hsc_power_div = 1;

int
pal_get_fru_discrete_list(uint8_t fru, uint8_t **sensor_list, int *cnt) {
  switch(fru) {
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

static void
pim_thresh_array_init(uint8_t fru) {
  int i = 0;
  int type, pedigree, pim_phy_type;

  syslog(LOG_WARNING,
            "pim_thresh_array_init: PIM %d init threshold array", fru - FRU_PIM1 + 1);
  switch (fru) {
    case FRU_PIM1:
    case FRU_PIM2:
    case FRU_PIM3:
    case FRU_PIM4:
    case FRU_PIM5:
    case FRU_PIM6:
    case FRU_PIM7:
    case FRU_PIM8:
      i = fru - FRU_PIM1;
      type = pal_get_pim_type_from_file(fru);
      if (type == PIM_TYPE_16Q) {
        pim_phy_type = pal_get_pim_phy_type_from_file(fru);
        pim_sensor_threshold[PIM1_SENSOR_QSFP_TEMP+i][UCR_THRESH] = 58;
        pim_sensor_threshold[PIM1_LM75_TEMP_4A+(i*PIM_SENSOR_CNT)][UCR_THRESH] = 80;
        pim_sensor_threshold[PIM1_LM75_TEMP_4B+(i*PIM_SENSOR_CNT)][UCR_THRESH] = 85;
        pim_sensor_threshold[PIM1_LM75_TEMP_48+(i*PIM_SENSOR_CNT)][UCR_THRESH] = 85;
        pim_sensor_threshold[PIM1_SENSOR_HSC_VOLT+(i*PIM_SENSOR_CNT)][UCR_THRESH] = 13.8;
        pim_sensor_threshold[PIM1_SENSOR_HSC_VOLT+(i*PIM_SENSOR_CNT)][UNC_THRESH] = 13.2;
        pim_sensor_threshold[PIM1_SENSOR_HSC_VOLT+(i*PIM_SENSOR_CNT)][LNC_THRESH] = 10.8;
        pim_sensor_threshold[PIM1_SENSOR_HSC_VOLT+(i*PIM_SENSOR_CNT)][LCR_THRESH] = 10.2;
        pim_sensor_threshold[PIM1_POWER_VOLTAGE+(i*PIM_SENSOR_CNT)][UCR_THRESH] = 13.8;
        pim_sensor_threshold[PIM1_POWER_VOLTAGE+(i*PIM_SENSOR_CNT)][UNC_THRESH] = 13.2;
        pim_sensor_threshold[PIM1_POWER_VOLTAGE+(i*PIM_SENSOR_CNT)][LNC_THRESH] = 10.8;
        pim_sensor_threshold[PIM1_POWER_VOLTAGE+(i*PIM_SENSOR_CNT)][LCR_THRESH] = 10.2;
        pim_sensor_threshold[PIM1_SENSOR_HSC_POWER+(i*PIM_SENSOR_CNT)][UCR_THRESH] = 346.654;
        pim_sensor_threshold[PIM1_SENSOR_HSC_POWER+(i*PIM_SENSOR_CNT)][UNC_THRESH] = 315.14;
        pim_sensor_threshold[PIM1_SENSOR_HSC_CURR+(i*PIM_SENSOR_CNT)][UCR_THRESH] = 28.89;
        pim_sensor_threshold[PIM1_SENSOR_HSC_CURR+(i*PIM_SENSOR_CNT)][UNC_THRESH] = 26.26;
        pim_sensor_threshold[PIM1_UCD90160_VOLT1+(i*PIM_SENSOR_CNT)][UCR_THRESH] = 3.795;
        pim_sensor_threshold[PIM1_UCD90160_VOLT1+(i*PIM_SENSOR_CNT)][UNC_THRESH] = 3.63;
        pim_sensor_threshold[PIM1_UCD90160_VOLT1+(i*PIM_SENSOR_CNT)][LNC_THRESH] = 2.97;
        pim_sensor_threshold[PIM1_UCD90160_VOLT1+(i*PIM_SENSOR_CNT)][LCR_THRESH] = 2.805;
        pim_sensor_threshold[PIM1_UCD90160_VOLT2+(i*PIM_SENSOR_CNT)][UCR_THRESH] = 3.795;
        pim_sensor_threshold[PIM1_UCD90160_VOLT2+(i*PIM_SENSOR_CNT)][UNC_THRESH] = 3.63;
        pim_sensor_threshold[PIM1_UCD90160_VOLT2+(i*PIM_SENSOR_CNT)][LNC_THRESH] = 2.97;
        pim_sensor_threshold[PIM1_UCD90160_VOLT2+(i*PIM_SENSOR_CNT)][LCR_THRESH] = 2.805;
        pim_sensor_threshold[PIM1_UCD90160_VOLT3+(i*PIM_SENSOR_CNT)][UCR_THRESH] = 2.875;
        pim_sensor_threshold[PIM1_UCD90160_VOLT3+(i*PIM_SENSOR_CNT)][UNC_THRESH] = 2.75;
        pim_sensor_threshold[PIM1_UCD90160_VOLT3+(i*PIM_SENSOR_CNT)][LNC_THRESH] = 2.25;
        pim_sensor_threshold[PIM1_UCD90160_VOLT3+(i*PIM_SENSOR_CNT)][LCR_THRESH] = 2.125;
        if (pim_phy_type == PIM_16Q_PHY_7NM) {
          pim_sensor_threshold[PIM1_UCD90160_VOLT4+(i*PIM_SENSOR_CNT)][UCR_THRESH] = 1.035;
          pim_sensor_threshold[PIM1_UCD90160_VOLT4+(i*PIM_SENSOR_CNT)][UNC_THRESH] = 0.99;
          pim_sensor_threshold[PIM1_UCD90160_VOLT4+(i*PIM_SENSOR_CNT)][LNC_THRESH] = 0.81;
          pim_sensor_threshold[PIM1_UCD90160_VOLT4+(i*PIM_SENSOR_CNT)][LCR_THRESH] = 0.765;
        } else if (pim_phy_type == PIM_16Q_PHY_16NM) {
          pim_sensor_threshold[PIM1_UCD90160_VOLT4+(i*PIM_SENSOR_CNT)][UCR_THRESH] = 0.92;
          pim_sensor_threshold[PIM1_UCD90160_VOLT4+(i*PIM_SENSOR_CNT)][UNC_THRESH] = 0.88;
          pim_sensor_threshold[PIM1_UCD90160_VOLT4+(i*PIM_SENSOR_CNT)][LNC_THRESH] = 0.72;
          pim_sensor_threshold[PIM1_UCD90160_VOLT4+(i*PIM_SENSOR_CNT)][LCR_THRESH] = 0.68;
        }
        if (pim_phy_type == PIM_16Q_PHY_7NM) {
          pim_sensor_threshold[PIM1_UCD90160_VOLT5+(i*PIM_SENSOR_CNT)][UCR_THRESH] = 0.8625;
          pim_sensor_threshold[PIM1_UCD90160_VOLT5+(i*PIM_SENSOR_CNT)][UNC_THRESH] = 0.825;
          pim_sensor_threshold[PIM1_UCD90160_VOLT5+(i*PIM_SENSOR_CNT)][LNC_THRESH] = 0.675;
          pim_sensor_threshold[PIM1_UCD90160_VOLT5+(i*PIM_SENSOR_CNT)][LCR_THRESH] = 0.6375;
        } else if (pim_phy_type == PIM_16Q_PHY_16NM) {
          pim_sensor_threshold[PIM1_UCD90160_VOLT5+(i*PIM_SENSOR_CNT)][UCR_THRESH] = 0.92;
          pim_sensor_threshold[PIM1_UCD90160_VOLT5+(i*PIM_SENSOR_CNT)][UNC_THRESH] = 0.88;
          pim_sensor_threshold[PIM1_UCD90160_VOLT5+(i*PIM_SENSOR_CNT)][LNC_THRESH] = 0.72;
          pim_sensor_threshold[PIM1_UCD90160_VOLT5+(i*PIM_SENSOR_CNT)][LCR_THRESH] = 0.68;
        }
        pim_sensor_threshold[PIM1_UCD90160_VOLT6+(i*PIM_SENSOR_CNT)][UCR_THRESH] = 1.265;
        pim_sensor_threshold[PIM1_UCD90160_VOLT6+(i*PIM_SENSOR_CNT)][UNC_THRESH] = 1.21;
        pim_sensor_threshold[PIM1_UCD90160_VOLT6+(i*PIM_SENSOR_CNT)][LNC_THRESH] = 0.99;
        pim_sensor_threshold[PIM1_UCD90160_VOLT6+(i*PIM_SENSOR_CNT)][LCR_THRESH] = 0.935;
        pim_sensor_threshold[PIM1_UCD90160_VOLT7+(i*PIM_SENSOR_CNT)][UCR_THRESH] = 0.92;
        pim_sensor_threshold[PIM1_UCD90160_VOLT7+(i*PIM_SENSOR_CNT)][UNC_THRESH] = 0.88;
        if (pim_phy_type == PIM_16Q_PHY_7NM) {
          pim_sensor_threshold[PIM1_UCD90160_VOLT7+(i*PIM_SENSOR_CNT)][LNC_THRESH] = 0.5;
          pim_sensor_threshold[PIM1_UCD90160_VOLT7+(i*PIM_SENSOR_CNT)][LCR_THRESH] = 0.45;
        } else if (pim_phy_type == PIM_16Q_PHY_16NM) {
          pim_sensor_threshold[PIM1_UCD90160_VOLT7+(i*PIM_SENSOR_CNT)][LNC_THRESH] = 0.72;
          pim_sensor_threshold[PIM1_UCD90160_VOLT7+(i*PIM_SENSOR_CNT)][LCR_THRESH] = 0.68;
        }
        pim_sensor_threshold[PIM1_UCD90160_VOLT8+(i*PIM_SENSOR_CNT)][UCR_THRESH] = 0.92;
        pim_sensor_threshold[PIM1_UCD90160_VOLT8+(i*PIM_SENSOR_CNT)][UNC_THRESH] = 0.88;
        if (pim_phy_type == PIM_16Q_PHY_7NM) {
          pim_sensor_threshold[PIM1_UCD90160_VOLT8+(i*PIM_SENSOR_CNT)][LNC_THRESH] = 0.5;
          pim_sensor_threshold[PIM1_UCD90160_VOLT8+(i*PIM_SENSOR_CNT)][LCR_THRESH] = 0.45;
        } else if (pim_phy_type == PIM_16Q_PHY_16NM) {
          pim_sensor_threshold[PIM1_UCD90160_VOLT8+(i*PIM_SENSOR_CNT)][LNC_THRESH] = 0.72;
          pim_sensor_threshold[PIM1_UCD90160_VOLT8+(i*PIM_SENSOR_CNT)][LCR_THRESH] = 0.68;
        }
        pim_sensor_threshold[PIM1_UCD90160_VOLT9+(i*PIM_SENSOR_CNT)][UCR_THRESH] = 0.92;
        pim_sensor_threshold[PIM1_UCD90160_VOLT9+(i*PIM_SENSOR_CNT)][UNC_THRESH] = 0.88;
        if (pim_phy_type == PIM_16Q_PHY_7NM) {
          pim_sensor_threshold[PIM1_UCD90160_VOLT9+(i*PIM_SENSOR_CNT)][LNC_THRESH] = 0.5;
          pim_sensor_threshold[PIM1_UCD90160_VOLT9+(i*PIM_SENSOR_CNT)][LCR_THRESH] = 0.45;
        } else if(pim_phy_type == PIM_16Q_PHY_16NM) {
          pim_sensor_threshold[PIM1_UCD90160_VOLT9+(i*PIM_SENSOR_CNT)][LNC_THRESH] = 0.72;
          pim_sensor_threshold[PIM1_UCD90160_VOLT9+(i*PIM_SENSOR_CNT)][LCR_THRESH] = 0.68;
        }
        pim_sensor_threshold[PIM1_UCD90160_VOLT10+(i*PIM_SENSOR_CNT)][UCR_THRESH] = 0.92;
        pim_sensor_threshold[PIM1_UCD90160_VOLT10+(i*PIM_SENSOR_CNT)][UNC_THRESH] = 0.88;
        if (pim_phy_type == PIM_16Q_PHY_7NM) {
          pim_sensor_threshold[PIM1_UCD90160_VOLT10+(i*PIM_SENSOR_CNT)][LNC_THRESH] = 0.5;
          pim_sensor_threshold[PIM1_UCD90160_VOLT10+(i*PIM_SENSOR_CNT)][LCR_THRESH] = 0.45;
        } else if (pim_phy_type == PIM_16Q_PHY_16NM) {
          pim_sensor_threshold[PIM1_UCD90160_VOLT10+(i*PIM_SENSOR_CNT)][LNC_THRESH] = 0.72;
          pim_sensor_threshold[PIM1_UCD90160_VOLT10+(i*PIM_SENSOR_CNT)][LCR_THRESH] = 0.68;
        }
        pim_sensor_threshold[PIM1_UCD90160_VOLT11+(i*PIM_SENSOR_CNT)][UCR_THRESH] = 2.07;
        pim_sensor_threshold[PIM1_UCD90160_VOLT11+(i*PIM_SENSOR_CNT)][UNC_THRESH] = 1.98;
        pim_sensor_threshold[PIM1_UCD90160_VOLT11+(i*PIM_SENSOR_CNT)][LNC_THRESH] = 1.62;
        pim_sensor_threshold[PIM1_UCD90160_VOLT11+(i*PIM_SENSOR_CNT)][LCR_THRESH] = 1.53;
        pim_sensor_threshold[PIM1_UCD90160_VOLT12+(i*PIM_SENSOR_CNT)][UCR_THRESH] = 2.07;
        pim_sensor_threshold[PIM1_UCD90160_VOLT12+(i*PIM_SENSOR_CNT)][UNC_THRESH] = 1.98;
        pim_sensor_threshold[PIM1_UCD90160_VOLT12+(i*PIM_SENSOR_CNT)][LNC_THRESH] = 1.62;
        pim_sensor_threshold[PIM1_UCD90160_VOLT12+(i*PIM_SENSOR_CNT)][LCR_THRESH] = 1.53;
        pim_sensor_threshold[PIM1_UCD90160_VOLT13+(i*PIM_SENSOR_CNT)][UCR_THRESH] = 2.07;
        pim_sensor_threshold[PIM1_UCD90160_VOLT13+(i*PIM_SENSOR_CNT)][UNC_THRESH] = 1.98;
        pim_sensor_threshold[PIM1_UCD90160_VOLT13+(i*PIM_SENSOR_CNT)][LNC_THRESH] = 1.62;
        pim_sensor_threshold[PIM1_UCD90160_VOLT13+(i*PIM_SENSOR_CNT)][LCR_THRESH] = 1.53;
        pim_sensor_threshold[PIM1_MP2975_INPUT_VOLTAGE+(i*PIM_SENSOR_CNT)][UCR_THRESH] = 13.8;
        pim_sensor_threshold[PIM1_MP2975_INPUT_VOLTAGE+(i*PIM_SENSOR_CNT)][UNC_THRESH] = 13.2;
        pim_sensor_threshold[PIM1_MP2975_INPUT_VOLTAGE+(i*PIM_SENSOR_CNT)][LNC_THRESH] = 10.8;
        pim_sensor_threshold[PIM1_MP2975_INPUT_VOLTAGE+(i*PIM_SENSOR_CNT)][LCR_THRESH] = 10.2;
        pim_sensor_threshold[PIM1_MP2975_OUTPUT_VOLTAGE_XP0R8V+(i*PIM_SENSOR_CNT)][UCR_THRESH] = 0.862;
        pim_sensor_threshold[PIM1_MP2975_OUTPUT_VOLTAGE_XP0R8V+(i*PIM_SENSOR_CNT)][UNC_THRESH] = 0.825;
        pim_sensor_threshold[PIM1_MP2975_OUTPUT_VOLTAGE_XP0R8V+(i*PIM_SENSOR_CNT)][LNC_THRESH] = 0.675;
        pim_sensor_threshold[PIM1_MP2975_OUTPUT_VOLTAGE_XP0R8V+(i*PIM_SENSOR_CNT)][LCR_THRESH] = 0.638;
        pim_sensor_threshold[PIM1_MP2975_OUTPUT_VOLTAGE_XP3R3V+(i*PIM_SENSOR_CNT)][UCR_THRESH] = 3.795;
        pim_sensor_threshold[PIM1_MP2975_OUTPUT_VOLTAGE_XP3R3V+(i*PIM_SENSOR_CNT)][UNC_THRESH] = 3.63;
        pim_sensor_threshold[PIM1_MP2975_OUTPUT_VOLTAGE_XP3R3V+(i*PIM_SENSOR_CNT)][LNC_THRESH] = 2.97;
        pim_sensor_threshold[PIM1_MP2975_OUTPUT_VOLTAGE_XP3R3V+(i*PIM_SENSOR_CNT)][LCR_THRESH] = 2.805;
        pim_sensor_threshold[PIM1_MP2975_OUTPUT_CURRENT_XP0R8V+(i*PIM_SENSOR_CNT)][UCR_THRESH] = 49.46;
        pim_sensor_threshold[PIM1_MP2975_OUTPUT_CURRENT_XP0R8V+(i*PIM_SENSOR_CNT)][UNC_THRESH] = 44.95;
        pim_sensor_threshold[PIM1_MP2975_OUTPUT_CURRENT_XP3R3V+(i*PIM_SENSOR_CNT)][UCR_THRESH] = 49.46;
        pim_sensor_threshold[PIM1_MP2975_OUTPUT_CURRENT_XP3R3V+(i*PIM_SENSOR_CNT)][UNC_THRESH] = 44.95;
      }else if (type == PIM_TYPE_16O) {
        pedigree = pal_get_pim_pedigree_from_file(fru);
        pim_sensor_threshold[PIM1_SENSOR_QSFP_TEMP+i][UCR_THRESH] = 60;
        pim_sensor_threshold[PIM1_LM75_TEMP_4B+(i*PIM_SENSOR_CNT)][UCR_THRESH] = 65;
        pim_sensor_threshold[PIM1_LM75_TEMP_4B+(i*PIM_SENSOR_CNT)][LCR_THRESH] = 10;
        pim_sensor_threshold[PIM1_LM75_TEMP_48+(i*PIM_SENSOR_CNT)][UCR_THRESH] = 65;
        pim_sensor_threshold[PIM1_LM75_TEMP_48+(i*PIM_SENSOR_CNT)][LCR_THRESH] = 10;
        pim_sensor_threshold[PIM1_SENSOR_HSC_VOLT+(i*PIM_SENSOR_CNT)][UCR_THRESH] = 13;
        pim_sensor_threshold[PIM1_SENSOR_HSC_VOLT+(i*PIM_SENSOR_CNT)][UNC_THRESH] = 0; // unset
        pim_sensor_threshold[PIM1_SENSOR_HSC_VOLT+(i*PIM_SENSOR_CNT)][LNC_THRESH] = 0; // unset
        pim_sensor_threshold[PIM1_SENSOR_HSC_VOLT+(i*PIM_SENSOR_CNT)][LCR_THRESH] = 11;
        pim_sensor_threshold[PIM1_POWER_VOLTAGE+(i*PIM_SENSOR_CNT)][UCR_THRESH] = 13;
        pim_sensor_threshold[PIM1_POWER_VOLTAGE+(i*PIM_SENSOR_CNT)][UNC_THRESH] = 0; // unset
        pim_sensor_threshold[PIM1_POWER_VOLTAGE+(i*PIM_SENSOR_CNT)][LNC_THRESH] = 0; // unset
        pim_sensor_threshold[PIM1_POWER_VOLTAGE+(i*PIM_SENSOR_CNT)][LCR_THRESH] = 11;
        pim_sensor_threshold[PIM1_SENSOR_HSC_POWER+(i*PIM_SENSOR_CNT)][UCR_THRESH] = 180;
        pim_sensor_threshold[PIM1_SENSOR_HSC_POWER+(i*PIM_SENSOR_CNT)][LCR_THRESH] = 0;
        pim_sensor_threshold[PIM1_SENSOR_HSC_CURR+(i*PIM_SENSOR_CNT)][UCR_THRESH] = 15;
        pim_sensor_threshold[PIM1_SENSOR_HSC_CURR+(i*PIM_SENSOR_CNT)][LCR_THRESH] = 0;
        // CHANNEL 1  PIMx_XP3R3V       3.3V
        pim_sensor_threshold[PIM1_UCD90160_VOLT1+(i*PIM_SENSOR_CNT)][UCR_THRESH] = 3.498;
        pim_sensor_threshold[PIM1_UCD90160_VOLT1+(i*PIM_SENSOR_CNT)][UNC_THRESH] = 0; // unset
        pim_sensor_threshold[PIM1_UCD90160_VOLT1+(i*PIM_SENSOR_CNT)][LNC_THRESH] = 0; // unset
        pim_sensor_threshold[PIM1_UCD90160_VOLT1+(i*PIM_SENSOR_CNT)][LCR_THRESH] = 3.102;
        // CHANNEL 2  PIMx_XP2R5V       2.5V
        pim_sensor_threshold[PIM1_UCD90160_VOLT2+(i*PIM_SENSOR_CNT)][UCR_THRESH] = 2.65;
        pim_sensor_threshold[PIM1_UCD90160_VOLT2+(i*PIM_SENSOR_CNT)][UNC_THRESH] = 0; // unset
        pim_sensor_threshold[PIM1_UCD90160_VOLT2+(i*PIM_SENSOR_CNT)][LNC_THRESH] = 0; // unset
        pim_sensor_threshold[PIM1_UCD90160_VOLT2+(i*PIM_SENSOR_CNT)][LCR_THRESH] = 2.35;
        // CHANNEL 3  PIMx_XP1R1V       1.1V
        pim_sensor_threshold[PIM1_UCD90160_VOLT3+(i*PIM_SENSOR_CNT)][UCR_THRESH] = 1.166;
        pim_sensor_threshold[PIM1_UCD90160_VOLT3+(i*PIM_SENSOR_CNT)][UNC_THRESH] = 0; // unset
        pim_sensor_threshold[PIM1_UCD90160_VOLT3+(i*PIM_SENSOR_CNT)][LNC_THRESH] = 0; // unset
        pim_sensor_threshold[PIM1_UCD90160_VOLT3+(i*PIM_SENSOR_CNT)][LCR_THRESH] = 1.034;
        // CHANNEL 4  PIMx_XP1R8V       1.8V
        pim_sensor_threshold[PIM1_UCD90160_VOLT4+(i*PIM_SENSOR_CNT)][UCR_THRESH] = 1.908;
        pim_sensor_threshold[PIM1_UCD90160_VOLT4+(i*PIM_SENSOR_CNT)][UNC_THRESH] = 0; // unset
        pim_sensor_threshold[PIM1_UCD90160_VOLT4+(i*PIM_SENSOR_CNT)][LNC_THRESH] = 0; // unset
        pim_sensor_threshold[PIM1_UCD90160_VOLT4+(i*PIM_SENSOR_CNT)][LCR_THRESH] = 1.692;
        // CHANNEL 5  PIMx_XP3R3V_OBO   3.3V
        pim_sensor_threshold[PIM1_UCD90160_VOLT5+(i*PIM_SENSOR_CNT)][UCR_THRESH] = 3.498;
        pim_sensor_threshold[PIM1_UCD90160_VOLT5+(i*PIM_SENSOR_CNT)][UNC_THRESH] = 0; // unset
        pim_sensor_threshold[PIM1_UCD90160_VOLT5+(i*PIM_SENSOR_CNT)][LNC_THRESH] = 0; // unset
        pim_sensor_threshold[PIM1_UCD90160_VOLT5+(i*PIM_SENSOR_CNT)][LCR_THRESH] = 3.102;
        // CHANNEL 6  PIMx_XP8R0V_OBO
        if(pedigree==PIM_16O_SIMULATE){ // 3.3V
          pim_sensor_threshold[PIM1_UCD90160_VOLT6+(i*PIM_SENSOR_CNT)][UCR_THRESH] = 3.498;
          pim_sensor_threshold[PIM1_UCD90160_VOLT6+(i*PIM_SENSOR_CNT)][LCR_THRESH] = 3.102;
        }else if(pedigree==PIM_16O_ALPHA1 || pedigree==PIM_16O_ALPHA2){ // 8V
          pim_sensor_threshold[PIM1_UCD90160_VOLT6+(i*PIM_SENSOR_CNT)][UCR_THRESH] = 8.48;
          pim_sensor_threshold[PIM1_UCD90160_VOLT6+(i*PIM_SENSOR_CNT)][LCR_THRESH] = 7.52;
        }
        pim_sensor_threshold[PIM1_UCD90160_VOLT6+(i*PIM_SENSOR_CNT)][UNC_THRESH] = 0; // unset
        pim_sensor_threshold[PIM1_UCD90160_VOLT6+(i*PIM_SENSOR_CNT)][LNC_THRESH] = 0; // unset
        // CHANNEL 7 PIMx_XP0R65V_OBO
        if(pedigree==PIM_16O_SIMULATE){ // 0.75V
          pim_sensor_threshold[PIM1_UCD90160_VOLT7+(i*PIM_SENSOR_CNT)][UCR_THRESH] = 0.776;
          pim_sensor_threshold[PIM1_UCD90160_VOLT7+(i*PIM_SENSOR_CNT)][LCR_THRESH] = 0.724;
        }else if(pedigree==PIM_16O_ALPHA1 || pedigree==PIM_16O_ALPHA2){ // 0.65V
          pim_sensor_threshold[PIM1_UCD90160_VOLT7+(i*PIM_SENSOR_CNT)][UCR_THRESH] = 0.67275;
          pim_sensor_threshold[PIM1_UCD90160_VOLT7+(i*PIM_SENSOR_CNT)][LCR_THRESH] = 0.62725;
        }
        pim_sensor_threshold[PIM1_UCD90160_VOLT7+(i*PIM_SENSOR_CNT)][UNC_THRESH] = 0; // unset
        pim_sensor_threshold[PIM1_UCD90160_VOLT7+(i*PIM_SENSOR_CNT)][LNC_THRESH] = 0; // unset
        // CHANNEL 8 PIMx_XP0R94V_OBO
        if(pedigree==PIM_16O_SIMULATE){ // 1.1V
          pim_sensor_threshold[PIM1_UCD90160_VOLT8+(i*PIM_SENSOR_CNT)][UCR_THRESH] = 1.139;
          pim_sensor_threshold[PIM1_UCD90160_VOLT8+(i*PIM_SENSOR_CNT)][LCR_THRESH] = 1.062;
        }else if(pedigree==PIM_16O_ALPHA1 || pedigree==PIM_16O_ALPHA2){ // 0.946V
          pim_sensor_threshold[PIM1_UCD90160_VOLT8+(i*PIM_SENSOR_CNT)][UCR_THRESH] = 0.97911;
          pim_sensor_threshold[PIM1_UCD90160_VOLT8+(i*PIM_SENSOR_CNT)][LCR_THRESH] = 0.91289;
        }
        pim_sensor_threshold[PIM1_UCD90160_VOLT8+(i*PIM_SENSOR_CNT)][UNC_THRESH] = 0; // unset
        pim_sensor_threshold[PIM1_UCD90160_VOLT8+(i*PIM_SENSOR_CNT)][LNC_THRESH] = 0; // unset
        // CHANNEL 9  PIMx_XP1R15V_OBO  1.15V
        pim_sensor_threshold[PIM1_UCD90160_VOLT9+(i*PIM_SENSOR_CNT)][UCR_THRESH] = 1.19;
        pim_sensor_threshold[PIM1_UCD90160_VOLT9+(i*PIM_SENSOR_CNT)][UNC_THRESH] = 0; // unset
        pim_sensor_threshold[PIM1_UCD90160_VOLT9+(i*PIM_SENSOR_CNT)][LNC_THRESH] = 0; // unset
        pim_sensor_threshold[PIM1_UCD90160_VOLT9+(i*PIM_SENSOR_CNT)][LCR_THRESH] = 1.1;
        // CHANNEL 10  PIMx_XP1R8V_OBO   1.8V
        pim_sensor_threshold[PIM1_UCD90160_VOLT10+(i*PIM_SENSOR_CNT)][UCR_THRESH] = 1.863;
        pim_sensor_threshold[PIM1_UCD90160_VOLT10+(i*PIM_SENSOR_CNT)][UNC_THRESH] = 0; // unset
        pim_sensor_threshold[PIM1_UCD90160_VOLT10+(i*PIM_SENSOR_CNT)][LNC_THRESH] = 0; // unset
        pim_sensor_threshold[PIM1_UCD90160_VOLT10+(i*PIM_SENSOR_CNT)][LCR_THRESH] = 1.737;
        // CHANNEL 11  PIMx_XP2R0V_OBO   2.0V
        pim_sensor_threshold[PIM1_UCD90160_VOLT11+(i*PIM_SENSOR_CNT)][UCR_THRESH] = 2.07;
        pim_sensor_threshold[PIM1_UCD90160_VOLT11+(i*PIM_SENSOR_CNT)][UNC_THRESH] = 0; // unset
        pim_sensor_threshold[PIM1_UCD90160_VOLT11+(i*PIM_SENSOR_CNT)][LNC_THRESH] = 0; // unset
        pim_sensor_threshold[PIM1_UCD90160_VOLT11+(i*PIM_SENSOR_CNT)][LCR_THRESH] = 1.93;
        // CHANNEL 12  PIMx_XP4R0V_OBO
        if(pedigree==PIM_16O_SIMULATE){ // 3.6V
          pim_sensor_threshold[PIM1_UCD90160_VOLT12+(i*PIM_SENSOR_CNT)][UCR_THRESH] = 3.816;
          pim_sensor_threshold[PIM1_UCD90160_VOLT12+(i*PIM_SENSOR_CNT)][LCR_THRESH] = 3.384;
        }else if(pedigree==PIM_16O_ALPHA1){ // 4.0V
          pim_sensor_threshold[PIM1_UCD90160_VOLT12+(i*PIM_SENSOR_CNT)][UCR_THRESH] = 4.24;
          pim_sensor_threshold[PIM1_UCD90160_VOLT12+(i*PIM_SENSOR_CNT)][LCR_THRESH] = 3.76;
        }else if(pedigree==PIM_16O_ALPHA2){ // 3.0V
          pim_sensor_threshold[PIM1_UCD90160_VOLT12+(i*PIM_SENSOR_CNT)][UCR_THRESH] = 3.18;
          pim_sensor_threshold[PIM1_UCD90160_VOLT12+(i*PIM_SENSOR_CNT)][LCR_THRESH] = 2.82;
        }
        pim_sensor_threshold[PIM1_UCD90160_VOLT12+(i*PIM_SENSOR_CNT)][UNC_THRESH] = 0; // unset
        pim_sensor_threshold[PIM1_UCD90160_VOLT12+(i*PIM_SENSOR_CNT)][LNC_THRESH] = 0; // unset

        pim_sensor_threshold[PIM1_UCD90160_VOLT13+(i*PIM_SENSOR_CNT)][UCR_THRESH] = 0; // unset
        pim_sensor_threshold[PIM1_UCD90160_VOLT13+(i*PIM_SENSOR_CNT)][UNC_THRESH] = 0; // unset
        pim_sensor_threshold[PIM1_UCD90160_VOLT13+(i*PIM_SENSOR_CNT)][LNC_THRESH] = 0; // unset
        pim_sensor_threshold[PIM1_UCD90160_VOLT13+(i*PIM_SENSOR_CNT)][LCR_THRESH] = 0; // unset

        pim_sensor_threshold[PIM1_MP2975_INPUT_VOLTAGE+(i*PIM_SENSOR_CNT)][UCR_THRESH] = 0; //unset
        pim_sensor_threshold[PIM1_MP2975_INPUT_VOLTAGE+(i*PIM_SENSOR_CNT)][UNC_THRESH] = 0; //unset
        pim_sensor_threshold[PIM1_MP2975_INPUT_VOLTAGE+(i*PIM_SENSOR_CNT)][LNC_THRESH] = 0; //unset
        pim_sensor_threshold[PIM1_MP2975_INPUT_VOLTAGE+(i*PIM_SENSOR_CNT)][LCR_THRESH] = 0; //unset
        pim_sensor_threshold[PIM1_MP2975_OUTPUT_VOLTAGE_XP0R8V+(i*PIM_SENSOR_CNT)][UCR_THRESH] = 0; //unset
        pim_sensor_threshold[PIM1_MP2975_OUTPUT_VOLTAGE_XP0R8V+(i*PIM_SENSOR_CNT)][UNC_THRESH] = 0; //unset
        pim_sensor_threshold[PIM1_MP2975_OUTPUT_VOLTAGE_XP0R8V+(i*PIM_SENSOR_CNT)][LNC_THRESH] = 0; //unset
        pim_sensor_threshold[PIM1_MP2975_OUTPUT_VOLTAGE_XP0R8V+(i*PIM_SENSOR_CNT)][LCR_THRESH] = 0; //unset
        pim_sensor_threshold[PIM1_MP2975_OUTPUT_VOLTAGE_XP3R3V+(i*PIM_SENSOR_CNT)][UCR_THRESH] = 0; //unset
        pim_sensor_threshold[PIM1_MP2975_OUTPUT_VOLTAGE_XP3R3V+(i*PIM_SENSOR_CNT)][UNC_THRESH] = 0; //unset
        pim_sensor_threshold[PIM1_MP2975_OUTPUT_VOLTAGE_XP3R3V+(i*PIM_SENSOR_CNT)][LNC_THRESH] = 0; //unset
        pim_sensor_threshold[PIM1_MP2975_OUTPUT_VOLTAGE_XP3R3V+(i*PIM_SENSOR_CNT)][LCR_THRESH] = 0; //unset
        pim_sensor_threshold[PIM1_MP2975_OUTPUT_CURRENT_XP0R8V+(i*PIM_SENSOR_CNT)][UCR_THRESH] = 0; //unset
        pim_sensor_threshold[PIM1_MP2975_OUTPUT_CURRENT_XP0R8V+(i*PIM_SENSOR_CNT)][UNC_THRESH] = 0; //unset
        pim_sensor_threshold[PIM1_MP2975_OUTPUT_CURRENT_XP3R3V+(i*PIM_SENSOR_CNT)][UCR_THRESH] = 0; //unset
        pim_sensor_threshold[PIM1_MP2975_OUTPUT_CURRENT_XP3R3V+(i*PIM_SENSOR_CNT)][UNC_THRESH] = 0; //unset
      }
      break;
  }
}

int
pal_set_pim_thresh(uint8_t fru) {
  uint8_t snr_num;
  uint8_t snr_first_num;
  uint8_t snr_last_num;
  pim_thresh_array_init(fru);

  syslog(LOG_WARNING,
            "pal_set_pim_thresh: PIM %d set threshold value", fru-FRU_PIM1+1);
  switch(fru) {
    case FRU_PIM1:
      snr_first_num = PIM1_LM75_TEMP_4A;
      snr_last_num = PIM1_MP2975_OUTPUT_CURRENT_XP3R3V;
      break;
    case FRU_PIM2:
      snr_first_num = PIM2_LM75_TEMP_4A;
      snr_last_num = PIM2_MP2975_OUTPUT_CURRENT_XP3R3V;
      break;
    case FRU_PIM3:
      snr_first_num = PIM3_LM75_TEMP_4A;
      snr_last_num = PIM3_MP2975_OUTPUT_CURRENT_XP3R3V;
      break;
    case FRU_PIM4:
      snr_first_num = PIM4_LM75_TEMP_4A;
      snr_last_num = PIM4_MP2975_OUTPUT_CURRENT_XP3R3V;
      break;
    case FRU_PIM5:
      snr_first_num = PIM5_LM75_TEMP_4A;
      snr_last_num = PIM5_MP2975_OUTPUT_CURRENT_XP3R3V;
      break;
    case FRU_PIM6:
      snr_first_num = PIM6_LM75_TEMP_4A;
      snr_last_num = PIM6_MP2975_OUTPUT_CURRENT_XP3R3V;
      break;
    case FRU_PIM7:
      snr_first_num = PIM7_LM75_TEMP_4A;
      snr_last_num = PIM7_MP2975_OUTPUT_CURRENT_XP3R3V;
      break;
    case FRU_PIM8:
      snr_first_num = PIM8_LM75_TEMP_4A;
      snr_last_num = PIM8_MP2975_OUTPUT_CURRENT_XP3R3V;
      break;
    default:
      return -1;
  }

  for (snr_num = snr_first_num; snr_num <= snr_last_num; snr_num++) {
    if(pim_sensor_threshold[snr_num][UNR_THRESH] != 0)
      pal_sensor_thresh_modify(fru, snr_num, UNR_THRESH, pim_sensor_threshold[snr_num][UNR_THRESH]);
    if(pim_sensor_threshold[snr_num][UCR_THRESH] != 0)
      pal_sensor_thresh_modify(fru, snr_num, UCR_THRESH, pim_sensor_threshold[snr_num][UCR_THRESH]);
    if(pim_sensor_threshold[snr_num][UNC_THRESH] != 0)
      pal_sensor_thresh_modify(fru, snr_num, UNC_THRESH, pim_sensor_threshold[snr_num][UNC_THRESH]);
    if(pim_sensor_threshold[snr_num][LNC_THRESH] != 0)
      pal_sensor_thresh_modify(fru, snr_num, LNC_THRESH, pim_sensor_threshold[snr_num][LNC_THRESH]);
    if(pim_sensor_threshold[snr_num][LCR_THRESH] != 0)
      pal_sensor_thresh_modify(fru, snr_num, LCR_THRESH, pim_sensor_threshold[snr_num][LCR_THRESH]);
    if(pim_sensor_threshold[snr_num][LNR_THRESH] != 0)
      pal_sensor_thresh_modify(fru, snr_num, LNR_THRESH, pim_sensor_threshold[snr_num][LNR_THRESH]);
  }

  return 0;
}

int
pal_clear_thresh_value(uint8_t fru) {
  int ret;
  char fpath[64];
  char cmd[128];
  char fruname[16] = {0};

  ret = pal_get_fru_name(fru, fruname);
  if (ret < 0) {
    printf("%s: Fail to get fru%d name\n",__func__,fru);
    return ret;
  }

  snprintf(fpath, sizeof(fpath), THRESHOLD_BIN, fruname);
  snprintf(cmd, sizeof(cmd), "rm -rf %s",fpath);
  RUN_SHELL_CMD(cmd);

  snprintf(fpath, sizeof(fpath), THRESHOLD_RE_FLAG, fruname);
  snprintf(cmd, sizeof(cmd), "touch %s",fpath);
  RUN_SHELL_CMD(cmd);

  return 0;
}

int
pal_get_fru_sensor_list(uint8_t fru, uint8_t **sensor_list, int *cnt) {
  uint8_t pim_type, pim_id;
  int brd_type_rev;
  pal_get_board_rev(&brd_type_rev);

  switch(fru) {
  case FRU_SCM:
    *sensor_list = (uint8_t *) scm_all_sensor_list;
    *cnt = scm_all_sensor_cnt;
    break;
  case FRU_SMB:
    if(brd_type_rev == BOARD_FUJI_EVT1 ||
      brd_type_rev == BOARD_FUJI_EVT2 ||
      brd_type_rev == BOARD_FUJI_EVT3) {
      *sensor_list = (uint8_t *) smb_sensor_list_evt;
      *cnt = smb_sensor_cnt_evt;
    } else if (brd_type_rev == BOARD_FUJI_DVT1) {
      *sensor_list = (uint8_t *) smb_sensor_list_dvt;
      *cnt = smb_sensor_cnt_dvt;
    }
    break;
  case FRU_PIM1:
  case FRU_PIM2:
  case FRU_PIM3:
  case FRU_PIM4:
  case FRU_PIM5:
  case FRU_PIM6:
  case FRU_PIM7:
  case FRU_PIM8:
    pim_id = fru - FRU_PIM1;
    pim_type = pal_get_pim_type_from_file(fru);
    if ( pim_type == PIM_TYPE_16Q) {
      *sensor_list = (uint8_t *) pim16q_sensor_list[pim_id];
      *cnt = sizeof(pim16q_sensor_list[pim_id]) / sizeof(uint8_t);
    } else if ( pim_type == PIM_TYPE_16O) {
      *sensor_list = (uint8_t *) pim16o_sensor_list[pim_id];
      *cnt = sizeof(pim16o_sensor_list[pim_id]) / sizeof(uint8_t);
    } else {
      *sensor_list = NULL;
      *cnt = 0;
    }
    break;
  case FRU_PSU1:
    *sensor_list = (uint8_t *) psu1_sensor_list;
    *cnt = psu1_sensor_cnt;
    break;
  case FRU_PSU2:
    *sensor_list = (uint8_t *) psu2_sensor_list;
    *cnt = psu2_sensor_cnt;
    break;
  case FRU_PSU3:
    *sensor_list = (uint8_t *) psu3_sensor_list;
    *cnt = psu3_sensor_cnt;
    break;
  case FRU_PSU4:
    *sensor_list = (uint8_t *) psu4_sensor_list;
    *cnt = psu4_sensor_cnt;
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

static void
_print_sensor_discrete_log(uint8_t fru, uint8_t snr_num, char *snr_name,
    uint8_t val, char *event) {
  if (val) {
    syslog(LOG_CRIT, "ASSERT: %s discrete - raised - FRU: %d, num: 0x%X,"
        " snr: %-16s val: %d", event, fru, snr_num, snr_name, val);
  } else {
    syslog(LOG_CRIT, "DEASSERT: %s discrete - settled - FRU: %d, num: 0x%X,"
        " snr: %-16s val: %d", event, fru, snr_num, snr_name, val);
  }
  pal_update_ts_sled();
}

int
pal_sensor_discrete_check(uint8_t fru, uint8_t snr_num, char *snr_name,
    uint8_t o_val, uint8_t n_val) {

  char name[32], crisel[128];
  bool valid = false;
  uint8_t diff = o_val ^ n_val;

  if (GETBIT(diff, 0)) {
    if (fru == FRU_SCM) {
      switch(snr_num) {
        case BIC_SENSOR_SYSTEM_STATUS:
          snprintf(name, sizeof(name), "SOC_Thermal_Trip");
          valid = true;
          snprintf(crisel, sizeof(crisel), "%s - %s,FRU:%u",
                          name, GETBIT(n_val, 0)?"ASSERT":"DEASSERT", fru);
          pal_add_cri_sel(crisel);
          break;
        case BIC_SENSOR_VR_HOT:
          snprintf(name, sizeof(name), "SOC_VR_Hot");
          valid = true;
          break;
      }
      if (valid) {
        _print_sensor_discrete_log(fru, snr_num, snr_name,
                                        GETBIT(n_val, 0), name);
        valid = false;
      }
    }
  }

  if (GETBIT(diff, 1)) {
    if (fru == FRU_SCM) {
      switch(snr_num) {
        case BIC_SENSOR_SYSTEM_STATUS:
          snprintf(name, sizeof(name), "SOC_FIVR_Fault");
          valid = true;
          break;
        case BIC_SENSOR_VR_HOT:
          snprintf(name, sizeof(name), "SOC_DIMM_AB_VR_Hot");
          valid = true;
          break;
        case BIC_SENSOR_CPU_DIMM_HOT:
          snprintf(name, sizeof(name), "SOC_MEMHOT");
          valid = true;
          break;
      }
      if (valid) {
        _print_sensor_discrete_log(fru, snr_num, snr_name,
                                        GETBIT(n_val, 1), name);
        valid = false;
      }
    }
  }

  if (GETBIT(diff, 2)) {
    if (fru == FRU_SCM) {
      switch(snr_num) {
        case BIC_SENSOR_SYSTEM_STATUS:
          snprintf(name, sizeof(name), "SYS_Throttle");
          valid = true;
          break;
        case BIC_SENSOR_VR_HOT:
          snprintf(name, sizeof(name), "SOC_DIMM_DE_VR_Hot");
          valid = true;
          break;
      }
      if (valid) {
        _print_sensor_discrete_log(fru, snr_num, snr_name,
                                        GETBIT(n_val, 2), name);
        valid = false;
      }
    }
  }

  if (GETBIT(diff, 4)) {
    if (fru == FRU_SCM) {
      if (snr_num == BIC_SENSOR_PROC_FAIL) {
          _print_sensor_discrete_log(fru, snr_num, snr_name,
                                          GETBIT(n_val, 4), "FRB3");
      }
    }
  }

  return 0;
}

static int
get_current_dir(const char *device, char *dir_name) {
  char cmd[LARGEST_DEVICE_NAME + 1];
  FILE *fp;
  int size;

  // Get current working directory
  snprintf(cmd, sizeof(cmd), "cd %s;pwd", device);

  fp = popen(cmd, "r");
  if(NULL == fp)
     return -1;

  if (fgets(dir_name, LARGEST_DEVICE_NAME, fp) == NULL) {
    pclose(fp);
    return -1;
  }

  if (pclose(fp) == -1)
     OBMC_ERROR(errno, "pclose(%s) failed", cmd);

  // Remove the newline character at the end
  size = strlen(dir_name);
  if (size > 0 && isspace(dir_name[size - 1]))
    dir_name[size - 1] = '\0';

  return 0;
}


static int
check_and_read_sensor_value(uint8_t fru, uint8_t snr_num, const char *device,
                            const char *attr, int *value) {
  char dir_name[LARGEST_DEVICE_NAME + 1];

  if (snr_path[fru][snr_num].name != NULL) {
    if (!read_device(snr_path[fru][snr_num].name, value))
      return 0;
  }

  if (strstr(device, "hwmon*") != NULL) {
    /* Get current working directory */
    if (get_current_dir(device, dir_name)) {
      return -1;
    }
    snprintf(snr_path[fru][snr_num].name, sizeof(snr_path[fru][snr_num].name),
             "%s/%s", dir_name, attr);
  } else {
    snprintf(snr_path[fru][snr_num].name, sizeof(snr_path[fru][snr_num].name),
             "%s/%s", device, attr);
  }

  if (read_device(snr_path[fru][snr_num].name, value)) {
    return -1;
  }

  return 0;
}

static int
read_attr(uint8_t fru, uint8_t snr_num, const char *device,
          const char *attr, float *value) {
  int tmp;

  if (check_and_read_sensor_value(fru, snr_num, device, attr, &tmp))
    return -1;

  *value = ((float)tmp)/UNIT_DIV;

  return 0;
}

/*
 * Determine extra divisor of hotswap power output
 * - kernel 4.1.x:
 *   pmbus_core.c use milliwatt for direct format power output,
 *   thus we keep hsc_power_div equal to 1.
 * - Higher kernel versions:
 *   pmbus_core.c use microwatt for direct format power output,
 *   thus we need to set hsc_power_div equal to 1000.
 */
static int
hsc_power_div_init(void) {
  hsc_power_div = 1000;
  return 0;
}

static int
read_hsc_attr(uint8_t fru, uint8_t snr_num, const char *device,
              const char* attr, float r_sense, float *value) {
  int tmp;

  if (check_and_read_sensor_value(fru, snr_num, device, attr, &tmp))
    return -1;

  *value = ((float)tmp)/r_sense/UNIT_DIV;

  return 0;
}

static int
read_hsc_volt(uint8_t fru, uint8_t snr_num,
              const char *device, float r_sense, float *value) {
  return read_hsc_attr(fru, snr_num, device, VOLT(1), r_sense, value);
}

static int
read_hsc_power_volt(uint8_t fru, uint8_t snr_num,
              const char *device, float r_sense, float *value) {
  return read_hsc_attr(fru, snr_num, device, VOLT(2), r_sense, value);
}

static int
read_hsc_curr(uint8_t fru, uint8_t snr_num,
              const char *device, float r_sense, float *value) {
  return read_hsc_attr(fru, snr_num, device, CURR(1), r_sense, value);
}

static int
read_hsc_power(uint8_t fru, uint8_t snr_num,
               const char *device, float r_sense, float *value) {
  int ret = -1;
  static bool hsc_power_div_inited = false;

  if (!hsc_power_div_inited && !hsc_power_div_init()) {
    hsc_power_div_inited = true;
  }

  ret = read_hsc_attr(fru, snr_num, device, POWER(1), r_sense, value);
  if (!ret)
    *value = *value / hsc_power_div;

  return ret;
}

static int
read_fan_rpm_f(const char *device, uint8_t fan, float *value) {
  char full_name[LARGEST_DEVICE_NAME + 1];
  char device_name[11];
  int tmp;

  snprintf(device_name, 11, "fan%d_input", fan);
  snprintf(full_name, sizeof(full_name), "%s/%s", device, device_name);
  if (read_device(full_name, &tmp)) {
    return -1;
  }

  *value = (float)tmp;

  return 0;
}






static void
hsc_rsense_init(uint8_t hsc_id, const char* device) {
  static bool rsense_inited[MAX_NUM_FRUS] = {false};

  if (!rsense_inited[hsc_id]) {
    int brd_rev = -1;
    /*
     * Config FCM Rsense value at different hardware version.
     * Kernel driver use Rsense equal to 1 milliohm. We need to correct Rsense
     * value, and all values are from hardware team.
     */
    pal_get_cpld_board_rev(&brd_rev, device);
    switch (brd_rev) {
      case 0x7:
      case 0x2:
        /* R0A, R0B or R0C FCM */
        hsc_rsense[hsc_id] = 0.33;
        break;
      case 0x4:
      case 0x5:
        /* R0D or R01 FCM */
        if (hsc_id == HSC_FCM_T)
          hsc_rsense[hsc_id] = 0.377;
        else
          hsc_rsense[hsc_id] = 0.376;
        break;
      default:
        /*
         * Default, keep Rsense to 1, if we have new FCM version,
         * we can use default Rsense value as a base to correct real value.
         */
        hsc_rsense[hsc_id] = 1;
        break;
    }
    rsense_inited[hsc_id] = true;
  }
}

static int
scm_sensor_read(uint8_t fru, uint8_t sensor_num, float *value) {

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
    switch(sensor_num) {
      case SCM_SENSOR_OUTLET_U7_TEMP:
        ret = read_attr(fru, sensor_num, SCM_OUTLET_TEMP_DEVICE, TEMP(1), value);
        break;
      case SCM_SENSOR_BMC_LM75_U9_TEMP:
        ret = read_attr(fru, sensor_num, SCM_SENSOR_BMC_LM75_U9_DEVICE, TEMP(1), value);
        break;
      case SCM_SENSOR_INLET_U8_TEMP:
        ret = read_attr(fru, sensor_num, SCM_INLET_TEMP_DEVICE, TEMP(1), value);
        break;
      case SCM_SENSOR_HSC_VOLT:
        ret = read_hsc_volt(fru, sensor_num, SCM_HSC_DEVICE, 1, value);
        break;
      case SCM_SENSOR_POWER_VOLT:
        ret = read_hsc_power_volt(fru, sensor_num, SCM_HSC_DEVICE, 1, value);
        break;
      case SCM_SENSOR_HSC_CURR:
        ret = read_hsc_curr(fru, sensor_num, SCM_HSC_DEVICE, SCM_RSENSE, value);
        break;
      case SCM_SENSOR_HSC_POWER:
        ret = read_hsc_power(fru, sensor_num, SCM_HSC_DEVICE, SCM_RSENSE, value);
        break;
      default:
        ret = READING_NA;
        break;
    }
  } else if (!scm_sensor && is_server_on()) {
    ret = bic_sdr_init(FRU_SCM, false);
    if (ret < 0) {
#ifdef DEBUG
    syslog(LOG_INFO, "bic_sdr_init fail\n");
#endif
      return ret;
    }

    while (j < bic_discrete_cnt) {
      if (sensor_num == bic_discrete_list[j++]) {
        discrete = true;
        break;
      }
    }

    if (!g_sinfo[FRU_SCM-1][sensor_num].valid) {
      ret = bic_sdr_init(FRU_SCM, true); //reinit g_sinfo
      if (!g_sinfo[FRU_SCM-1][sensor_num].valid) {
        return READING_NA;
      }
    }

    ret = bic_read_sensor_wrapper(IPMB_BUS, FRU_SCM, sensor_num, discrete, value);
  } else {
    ret = READING_NA;
  }
  return ret;
}

static int
smb_sensor_read(uint8_t fru, uint8_t sensor_num, float *value) {

  int ret = -1;
  float read_val;
  int num = 0;
  char path[PATH_MAX ];

#define TH4_SENSOR_NUM  15

  switch(sensor_num) {
    case SMB_TMP422_U20_1_TEMP:
      ret = read_attr(fru, sensor_num, SMB_TMP422_DEVICE, TEMP(1), value);
      break;
    case SMB_TMP422_U20_2_TEMP:
      ret = read_attr(fru, sensor_num, SMB_TMP422_DEVICE, TEMP(2), value);
      break;
    case SMB_TMP422_U20_3_TEMP:
      ret = read_attr(fru, sensor_num, SMB_TMP422_DEVICE, TEMP(3), value);
      break;
    case SIM_LM75_U1_TEMP:
      ret = read_attr(fru, sensor_num, SIM_LM75_U1_DEVICE, TEMP(1), value);
    case SMB_SENSOR_TEMP1:
      ret = read_attr(fru, sensor_num, SMB_TEMP1_DEVICE, TEMP(1), value);
      break;
    case SMB_SENSOR_TEMP2:
      ret = read_attr(fru, sensor_num, SMB_TEMP2_DEVICE, TEMP(1), value);
      break;
    case SMB_SENSOR_TEMP3:
      ret = read_attr(fru, sensor_num, SMB_TEMP3_DEVICE, TEMP(1), value);
      break;
    case SMB_VDDC_SW_TEMP:
      ret = read_attr(fru, sensor_num, SMB_VDDC_SW_DEVICE, TEMP(1), value);
      break;
    case SMB_XP12R0V_VDDC_SW_IN:
      ret = read_attr(fru, sensor_num, SMB_VDDC_SW_DEVICE, VOLT(1), value);
      break;
    case SMB_VDDC_SW_IN_SENS:
      ret = read_attr(fru, sensor_num, SMB_VDDC_SW_DEVICE, VOLT(3), value);
      break;
    case SMB_VDDC_SW_POWER_IN:
      ret = read_attr(fru, sensor_num, SMB_VDDC_SW_DEVICE, POWER(1), value);
      *value = *value / hsc_power_div;
      break;
    case SMB_VDDC_SW_POWER_OUT:
      ret = read_attr(fru, sensor_num, SMB_VDDC_SW_DEVICE, POWER(3), value);
      *value = *value / hsc_power_div;
      break;
    case SMB_VDDC_SW_CURR_IN:
      ret = read_attr(fru, sensor_num, SMB_VDDC_SW_DEVICE, CURR(1), value);
      break;
    case SMB_VDDC_SW_CURR_OUT:
      ret = read_attr(fru, sensor_num, SMB_VDDC_SW_DEVICE, CURR(3), value);
      break;
    case SMB_SENSOR_PDB_L_TEMP1:
      ret = read_attr(fru, sensor_num, SMB_PDB_L_TEMP1_DEVICE, TEMP(1), value);
      break;
    case SMB_SENSOR_PDB_L_TEMP2:
      ret = read_attr(fru, sensor_num, SMB_PDB_L_TEMP2_DEVICE, TEMP(1), value);
      break;
    case SMB_SENSOR_PDB_R_TEMP1:
      ret = read_attr(fru, sensor_num, SMB_PDB_R_TEMP1_DEVICE, TEMP(1), value);
      break;
    case SMB_SENSOR_PDB_R_TEMP2:
      ret = read_attr(fru, sensor_num, SMB_PDB_R_TEMP2_DEVICE, TEMP(1), value);
      break;
    case SMB_SENSOR_FCM_T_TEMP1:
      ret = read_attr(fru, sensor_num, SMB_FCM_T_TEMP1_DEVICE, TEMP(1), value);
      break;
    case SMB_SENSOR_FCM_T_TEMP2:
      ret = read_attr(fru, sensor_num, SMB_FCM_T_TEMP2_DEVICE, TEMP(1), value);
      break;
    case SMB_SENSOR_FCM_B_TEMP1:
      ret = read_attr(fru, sensor_num, SMB_FCM_B_TEMP1_DEVICE, TEMP(1), value);
      break;
    case SMB_SENSOR_FCM_B_TEMP2:
      ret = read_attr(fru, sensor_num, SMB_FCM_B_TEMP2_DEVICE, TEMP(1), value);
      break;
    case SMB_XP3R3V_BMC:
      ret = read_attr(fru, sensor_num, SMB_UCD9012_1_DEVICE, VOLT(1), value);
      break;
    case SMB_XP2R5V_BMC:
      ret = read_attr(fru, sensor_num, SMB_UCD9012_1_DEVICE, VOLT(2), value);
      break;
    case SMB_XP1R8V_BMC:
      ret = read_attr(fru, sensor_num, SMB_UCD9012_1_DEVICE, VOLT(3), value);
      break;
    case SMB_XP1R2V_BMC:
      ret = read_attr(fru, sensor_num, SMB_UCD9012_1_DEVICE, VOLT(4), value);
      break;
    case SMB_XP1R0V_FPGA:
      ret = read_attr(fru, sensor_num, SMB_UCD9012_1_DEVICE, VOLT(5), value);
      break;
    case SMB_XP3R3V_USB:
      ret = read_attr(fru, sensor_num, SMB_UCD9012_1_DEVICE, VOLT(6), value);
      break;
    case SMB_XP5R0V:
      ret = read_attr(fru, sensor_num, SMB_UCD9012_1_DEVICE, VOLT(7), value);
      break;
    case SMB_XP3R3V_EARLY:
      ret = read_attr(fru, sensor_num, SMB_UCD9012_1_DEVICE, VOLT(8), value);
      break;
    case SMB_LM57_VTEMP:
      ret = read_attr(fru, sensor_num, SMB_UCD9012_1_DEVICE, VOLT(16), value);
      break;
    case SMB_XP1R8:
      ret = read_attr(fru, sensor_num, SMB_UCD9012_2_DEVICE, VOLT(1), value);
      break;
    case SMB_XP1R2:
      ret = read_attr(fru, sensor_num, SMB_UCD9012_2_DEVICE, VOLT(2), value);
      break;
    case SMB_VDDC_SW:
      ret = read_attr(fru, sensor_num, SMB_UCD9012_2_DEVICE, VOLT(3), value);
      break;
    case SMB_XP3R3V:
      ret = read_attr(fru, sensor_num, SMB_UCD9012_2_DEVICE, VOLT(4), value);
      break;
    case SMB_XP1R8V_AVDD:
      ret = read_attr(fru, sensor_num, SMB_UCD9012_2_DEVICE, VOLT(5), value);
      break;
    case SMB_XP1R2V_TVDD:
      ret = read_attr(fru, sensor_num, SMB_UCD9012_2_DEVICE, VOLT(6), value);
      break;
    case SMB_XP0R75V_1_PVDD:
      ret = read_attr(fru, sensor_num, SMB_UCD9012_2_DEVICE, VOLT(7), value);
      break;
    case SMB_XP0R75V_2_PVDD:
      ret = read_attr(fru, sensor_num, SMB_UCD9012_2_DEVICE, VOLT(8), value);
      break;
    case SMB_XP0R75V_3_PVDD:
      ret = read_attr(fru, sensor_num, SMB_UCD9012_2_DEVICE, VOLT(9), value);
      break;
    case SMB_VDD_PCIE:
      ret = read_attr(fru, sensor_num, SMB_UCD9012_2_DEVICE, VOLT(10), value);
      break;
    case SMB_XP0R84V_DCSU:
      ret = read_attr(fru, sensor_num, SMB_UCD9012_2_DEVICE, VOLT(11), value);
      break;
    case SMB_XP0R84V_CSU:
      ret = read_attr(fru, sensor_num, SMB_UCD9012_2_DEVICE, VOLT(12), value);
      break;
    case SMB_XP1R84V_CSU:
      ret = read_attr(fru, sensor_num, SMB_UCD9012_2_DEVICE, VOLT(13), value);
      break;
    case SMB_XP3R3V_TCXO:
      ret = read_attr(fru, sensor_num, SMB_UCD9012_2_DEVICE, VOLT(14), value);
      break;
    case SMB_OUTPUT_VOLTAGE_XP0R75V_1:
      ret = read_attr(fru, sensor_num, SMB_MP2978_1_DEVICE, VOLT(2), value);
      break;
    case SMB_OUTPUT_CURRENT_XP0R75V_1:
      ret = read_attr(fru, sensor_num, SMB_MP2978_1_DEVICE, CURR(2), value);
      break;
    case SMB_INPUT_VOLTAGE_1:
      ret = read_attr(fru, sensor_num, SMB_MP2978_1_DEVICE, VOLT(1), value);
      break;
    case SMB_OUTPUT_VOLTAGE_XP1R2V:
      ret = read_attr(fru, sensor_num, SMB_MP2978_1_DEVICE, VOLT(3), value);
      break;
    case SMB_OUTPUT_CURRENT_XP1R2V:
      ret = read_attr(fru, sensor_num, SMB_MP2978_1_DEVICE, CURR(3), value);
      break;
    case SMB_OUTPUT_VOLTAGE_XP0R75V_2:
      ret = read_attr(fru, sensor_num, SMB_MP2978_2_DEVICE, VOLT(2), value);
      break;
    case SMB_OUTPUT_CURRENT_XP0R75V_2:
      ret = read_attr(fru, sensor_num, SMB_MP2978_2_DEVICE, CURR(2), value);
      break;
    case SMB_INPUT_VOLTAGE_2:
      ret = read_attr(fru, sensor_num, SMB_MP2978_2_DEVICE, VOLT(1), value);
      break;
    case SMB_SENSOR_FCM_T_HSC_VOLT:
      ret = read_hsc_volt(fru, sensor_num, SMB_FCM_T_HSC_DEVICE, 1, value);
      break;
    case SMB_SENSOR_FCM_B_HSC_VOLT:
      ret = read_hsc_volt(fru, sensor_num, SMB_FCM_B_HSC_DEVICE, 1, value);
      break;
    case SMB_SENSOR_FCM_T_HSC_CURR:
      hsc_rsense_init(HSC_FCM_T, TOP_FCMCPLD_PATH_FMT);
      ret = read_hsc_curr(fru, sensor_num, SMB_FCM_T_HSC_DEVICE,
                          hsc_rsense[HSC_FCM_T], value);
      break;
    case SMB_SENSOR_FCM_B_HSC_CURR:
      hsc_rsense_init(HSC_FCM_B, BOTTOM_FCMCPLD_PATH_FMT);
      ret = read_hsc_curr(fru, sensor_num, SMB_FCM_B_HSC_DEVICE,
                          hsc_rsense[HSC_FCM_B], value);
      break;
    case SMB_SENSOR_FCM_T_HSC_POWER_VOLT:
      ret = read_hsc_power_volt(fru, sensor_num, SMB_FCM_T_HSC_DEVICE, 1, value);
      break;
    case SMB_SENSOR_FCM_B_HSC_POWER_VOLT:
      ret = read_hsc_power_volt(fru, sensor_num, SMB_FCM_B_HSC_DEVICE, 1, value);
      break;
    case SMB_SENSOR_FAN1_REAR_TACH:
      ret = read_fan_rpm_f(SMB_FCM_T_TACH_DEVICE, 1, value);
      break;
    case SMB_SENSOR_FAN1_FRONT_TACH:
      ret = read_fan_rpm_f(SMB_FCM_T_TACH_DEVICE, 2, value);
      break;
    case SMB_SENSOR_FAN2_REAR_TACH:
      ret = read_fan_rpm_f(SMB_FCM_B_TACH_DEVICE, 1, value);
      break;
    case SMB_SENSOR_FAN2_FRONT_TACH:
      ret = read_fan_rpm_f(SMB_FCM_B_TACH_DEVICE, 2, value);
      break;
    case SMB_SENSOR_FAN3_REAR_TACH:
      ret = read_fan_rpm_f(SMB_FCM_T_TACH_DEVICE, 3, value);
      break;
    case SMB_SENSOR_FAN3_FRONT_TACH:
      ret = read_fan_rpm_f(SMB_FCM_T_TACH_DEVICE, 4, value);
      break;
    case SMB_SENSOR_FAN4_REAR_TACH:
      ret = read_fan_rpm_f(SMB_FCM_B_TACH_DEVICE, 3, value);
      break;
    case SMB_SENSOR_FAN4_FRONT_TACH:
      ret = read_fan_rpm_f(SMB_FCM_B_TACH_DEVICE, 4, value);
      break;
    case SMB_SENSOR_FAN5_REAR_TACH:
      ret = read_fan_rpm_f(SMB_FCM_T_TACH_DEVICE, 5, value);
      break;
    case SMB_SENSOR_FAN5_FRONT_TACH:
      ret = read_fan_rpm_f(SMB_FCM_T_TACH_DEVICE, 6, value);
      break;
    case SMB_SENSOR_FAN6_REAR_TACH:
      ret = read_fan_rpm_f(SMB_FCM_B_TACH_DEVICE, 5, value);
      break;
    case SMB_SENSOR_FAN6_FRONT_TACH:
      ret = read_fan_rpm_f(SMB_FCM_B_TACH_DEVICE, 6, value);
      break;
    case SMB_SENSOR_FAN7_REAR_TACH:
      ret = read_fan_rpm_f(SMB_FCM_T_TACH_DEVICE, 7, value);
      break;
    case SMB_SENSOR_FAN7_FRONT_TACH:
      ret = read_fan_rpm_f(SMB_FCM_T_TACH_DEVICE, 8, value);
      break;
    case SMB_SENSOR_FAN8_REAR_TACH:
      ret = read_fan_rpm_f(SMB_FCM_B_TACH_DEVICE, 7, value);
      break;
    case SMB_SENSOR_FAN8_FRONT_TACH:
      ret = read_fan_rpm_f(SMB_FCM_B_TACH_DEVICE, 8, value);
      break;
    case SMB_SENSOR_TH4_HIGH:
      /* Enable the hardware lock in SMB CPLD to th4 access */
      snprintf(path, sizeof(path), SMBCPLD_PATH_FMT, "th4_read_N");
      write_device(path, "1");
      *value = 0;
      for ( int id = 1; id <= TH4_SENSOR_NUM; id++ ){
        sprintf(path,"temp%d_input",id);
        ret = read_attr(fru, sensor_num, SMB_TH4_DEVICE, path, &read_val);
        if(ret){
          continue;
        }else{
          num++;
        }

        if(read_val > *value){
          *value = read_val;
        }
      }
      /* Disable the hardware lock in SMB CPLD to avoid invalid th4 access */
      snprintf(path, sizeof(path), SMBCPLD_PATH_FMT, "th4_read_N");
      write_device(path, "0");
      if(num == TH4_SENSOR_NUM){
        ret = 0;
      }else{
        ret = READING_NA;
      }
      break;
    default:
      ret = READING_NA;
      break;
  }
  return ret;
}

static int
pim_sensor_read(uint8_t fru, uint8_t sensor_num, float *value) {
  char full_name[LARGEST_DEVICE_NAME + 1];
  int ret = -1;
  uint8_t type = pal_get_pim_type_from_file(fru);
  uint8_t pimid = fru-FRU_PIM1 + 1;
  uint8_t i2cbus = PIM1_I2CBUS + (pimid - 1) * 8;

#define dir_pim_sensor(str_buffer,i2c_bus,i2c_offset,addr) \
      sprintf(str_buffer, \
              I2C_SYSFS_DEVICES"/%d-00%02x/", \
              i2c_offset + i2c_bus, \
              addr)

#define dir_pim_sensor_hwmon(str_buffer,i2c_bus,i2c_offset,addr) \
      sprintf(str_buffer, \
              I2C_SYSFS_DEVICES"/%d-00%02x/hwmon/hwmon*/", \
              i2c_offset + i2c_bus, \
              addr)

  switch(sensor_num) {
    case PIM1_LM75_TEMP_4A:
    case PIM2_LM75_TEMP_4A:
    case PIM3_LM75_TEMP_4A:
    case PIM4_LM75_TEMP_4A:
    case PIM5_LM75_TEMP_4A:
    case PIM6_LM75_TEMP_4A:
    case PIM7_LM75_TEMP_4A:
    case PIM8_LM75_TEMP_4A:
      dir_pim_sensor_hwmon(full_name, i2cbus, PIM_LM75_TEMP_4A_DEVICE_CH, PIM_LM75_TEMP_4A_DEVICE_ADDR);
      ret = read_attr(fru, sensor_num, full_name, TEMP(1), value);
      break;
    case PIM1_LM75_TEMP_4B:
    case PIM2_LM75_TEMP_4B:
    case PIM3_LM75_TEMP_4B:
    case PIM4_LM75_TEMP_4B:
    case PIM5_LM75_TEMP_4B:
    case PIM6_LM75_TEMP_4B:
    case PIM7_LM75_TEMP_4B:
    case PIM8_LM75_TEMP_4B:
      dir_pim_sensor_hwmon(full_name, i2cbus, PIM_LM75_TEMP_4B_DEVICE_CH, PIM_LM75_TEMP_4B_DEVICE_ADDR);
      ret = read_attr(fru, sensor_num, full_name , TEMP(1), value);
      break;
    case PIM1_LM75_TEMP_48:
    case PIM2_LM75_TEMP_48:
    case PIM3_LM75_TEMP_48:
    case PIM4_LM75_TEMP_48:
    case PIM5_LM75_TEMP_48:
    case PIM6_LM75_TEMP_48:
    case PIM7_LM75_TEMP_48:
    case PIM8_LM75_TEMP_48:
      dir_pim_sensor_hwmon(full_name, i2cbus, PIM_LM75_TEMP_48_DEVICE_CH, PIM_LM75_TEMP_48_DEVICE_ADDR);
      ret = read_attr(fru, sensor_num, full_name , TEMP(1), value);
      break;
    case PIM1_SENSOR_QSFP_TEMP:
    case PIM2_SENSOR_QSFP_TEMP:
    case PIM3_SENSOR_QSFP_TEMP:
    case PIM4_SENSOR_QSFP_TEMP:
    case PIM5_SENSOR_QSFP_TEMP:
    case PIM6_SENSOR_QSFP_TEMP:
    case PIM7_SENSOR_QSFP_TEMP:
    case PIM8_SENSOR_QSFP_TEMP:
      dir_pim_sensor(full_name, i2cbus, PIM_DOM_DEVICE_CH, PIM_DOM_DEVICE_ADDR);
      ret = read_attr(fru, sensor_num, full_name , TEMP(1), value);
      break;
    case PIM1_SENSOR_HSC_VOLT:
    case PIM2_SENSOR_HSC_VOLT:
    case PIM3_SENSOR_HSC_VOLT:
    case PIM4_SENSOR_HSC_VOLT:
    case PIM5_SENSOR_HSC_VOLT:
    case PIM6_SENSOR_HSC_VOLT:
    case PIM7_SENSOR_HSC_VOLT:
    case PIM8_SENSOR_HSC_VOLT:
      dir_pim_sensor_hwmon(full_name, i2cbus, PIM_HSC_DEVICE_CH, PIM_HSC_DEVICE_ADDR);
      ret = read_hsc_volt(fru, sensor_num, full_name , 1, value);
      break;
    case PIM1_POWER_VOLTAGE:
    case PIM2_POWER_VOLTAGE:
    case PIM3_POWER_VOLTAGE:
    case PIM4_POWER_VOLTAGE:
    case PIM5_POWER_VOLTAGE:
    case PIM6_POWER_VOLTAGE:
    case PIM7_POWER_VOLTAGE:
    case PIM8_POWER_VOLTAGE:
      dir_pim_sensor_hwmon(full_name, i2cbus, PIM_HSC_DEVICE_CH, PIM_HSC_DEVICE_ADDR);
      ret = read_hsc_power_volt(fru, sensor_num, full_name , 1, value);
      break;
    case PIM1_SENSOR_HSC_CURR:
    case PIM2_SENSOR_HSC_CURR:
    case PIM3_SENSOR_HSC_CURR:
    case PIM4_SENSOR_HSC_CURR:
    case PIM5_SENSOR_HSC_CURR:
    case PIM6_SENSOR_HSC_CURR:
    case PIM7_SENSOR_HSC_CURR:
    case PIM8_SENSOR_HSC_CURR:
      dir_pim_sensor_hwmon(full_name, i2cbus, PIM_HSC_DEVICE_CH, PIM_HSC_DEVICE_ADDR);
      if (type == PIM_TYPE_16O) {
        ret = read_hsc_curr(fru, sensor_num, full_name, PIM16O_RSENSE, value);
      } else {
        ret = read_hsc_curr(fru, sensor_num, full_name, PIM_RSENSE, value);
      }
      break;
    case PIM1_SENSOR_HSC_POWER:
    case PIM2_SENSOR_HSC_POWER:
    case PIM3_SENSOR_HSC_POWER:
    case PIM4_SENSOR_HSC_POWER:
    case PIM5_SENSOR_HSC_POWER:
    case PIM6_SENSOR_HSC_POWER:
    case PIM7_SENSOR_HSC_POWER:
    case PIM8_SENSOR_HSC_POWER:
      dir_pim_sensor_hwmon(full_name, i2cbus, PIM_HSC_DEVICE_CH, PIM_HSC_DEVICE_ADDR);
      if (type == PIM_TYPE_16O) {
        ret = read_hsc_power(fru, sensor_num, full_name, PIM16O_RSENSE, value);
      } else {
        ret = read_hsc_power(fru, sensor_num, full_name, PIM_RSENSE, value);
      }
      break;
    case PIM1_UCD90160_VOLT1:
    case PIM2_UCD90160_VOLT1:
    case PIM3_UCD90160_VOLT1:
    case PIM4_UCD90160_VOLT1:
    case PIM5_UCD90160_VOLT1:
    case PIM6_UCD90160_VOLT1:
    case PIM7_UCD90160_VOLT1:
    case PIM8_UCD90160_VOLT1:
      dir_pim_sensor_hwmon(full_name, i2cbus, PIM_UCD90160_DEVICE_CH, PIM_UCD90160_DEVICE_ADDR);
      ret = read_attr(fru, sensor_num, full_name, VOLT(1), value);
      break;
    case PIM1_UCD90160_VOLT2:
    case PIM2_UCD90160_VOLT2:
    case PIM3_UCD90160_VOLT2:
    case PIM4_UCD90160_VOLT2:
    case PIM5_UCD90160_VOLT2:
    case PIM6_UCD90160_VOLT2:
    case PIM7_UCD90160_VOLT2:
    case PIM8_UCD90160_VOLT2:
      dir_pim_sensor_hwmon(full_name, i2cbus, PIM_UCD90160_DEVICE_CH, PIM_UCD90160_DEVICE_ADDR);
      ret = read_attr(fru, sensor_num, full_name, VOLT(2), value);
      break;
    case PIM1_UCD90160_VOLT3:
    case PIM2_UCD90160_VOLT3:
    case PIM3_UCD90160_VOLT3:
    case PIM4_UCD90160_VOLT3:
    case PIM5_UCD90160_VOLT3:
    case PIM6_UCD90160_VOLT3:
    case PIM7_UCD90160_VOLT3:
    case PIM8_UCD90160_VOLT3:
      dir_pim_sensor_hwmon(full_name, i2cbus, PIM_UCD90160_DEVICE_CH, PIM_UCD90160_DEVICE_ADDR);
      ret = read_attr(fru, sensor_num, full_name, VOLT(3), value);
      break;
    case PIM1_UCD90160_VOLT4:
    case PIM2_UCD90160_VOLT4:
    case PIM3_UCD90160_VOLT4:
    case PIM4_UCD90160_VOLT4:
    case PIM5_UCD90160_VOLT4:
    case PIM6_UCD90160_VOLT4:
    case PIM7_UCD90160_VOLT4:
    case PIM8_UCD90160_VOLT4:
      dir_pim_sensor_hwmon(full_name, i2cbus, PIM_UCD90160_DEVICE_CH, PIM_UCD90160_DEVICE_ADDR);
      ret = read_attr(fru, sensor_num, full_name, VOLT(4), value);
      break;
    case PIM1_UCD90160_VOLT5:
    case PIM2_UCD90160_VOLT5:
    case PIM3_UCD90160_VOLT5:
    case PIM4_UCD90160_VOLT5:
    case PIM5_UCD90160_VOLT5:
    case PIM6_UCD90160_VOLT5:
    case PIM7_UCD90160_VOLT5:
    case PIM8_UCD90160_VOLT5:
      dir_pim_sensor_hwmon(full_name, i2cbus, PIM_UCD90160_DEVICE_CH, PIM_UCD90160_DEVICE_ADDR);
      ret = read_attr(fru, sensor_num, full_name, VOLT(5), value);
      break;
    case PIM1_UCD90160_VOLT6:
    case PIM2_UCD90160_VOLT6:
    case PIM3_UCD90160_VOLT6:
    case PIM4_UCD90160_VOLT6:
    case PIM5_UCD90160_VOLT6:
    case PIM6_UCD90160_VOLT6:
    case PIM7_UCD90160_VOLT6:
    case PIM8_UCD90160_VOLT6:
      dir_pim_sensor_hwmon(full_name, i2cbus, PIM_UCD90160_DEVICE_CH, PIM_UCD90160_DEVICE_ADDR);
      ret = read_attr(fru, sensor_num, full_name, VOLT(6), value);
      break;
    case PIM1_UCD90160_VOLT7:
    case PIM2_UCD90160_VOLT7:
    case PIM3_UCD90160_VOLT7:
    case PIM4_UCD90160_VOLT7:
    case PIM5_UCD90160_VOLT7:
    case PIM6_UCD90160_VOLT7:
    case PIM7_UCD90160_VOLT7:
    case PIM8_UCD90160_VOLT7:
      dir_pim_sensor_hwmon(full_name, i2cbus, PIM_UCD90160_DEVICE_CH, PIM_UCD90160_DEVICE_ADDR);
      ret = read_attr(fru, sensor_num, full_name, VOLT(7), value);
      break;
    case PIM1_UCD90160_VOLT8:
    case PIM2_UCD90160_VOLT8:
    case PIM3_UCD90160_VOLT8:
    case PIM4_UCD90160_VOLT8:
    case PIM5_UCD90160_VOLT8:
    case PIM6_UCD90160_VOLT8:
    case PIM7_UCD90160_VOLT8:
    case PIM8_UCD90160_VOLT8:
      dir_pim_sensor_hwmon(full_name, i2cbus, PIM_UCD90160_DEVICE_CH, PIM_UCD90160_DEVICE_ADDR);
      ret = read_attr(fru, sensor_num, full_name, VOLT(8), value);
      break;
    case PIM1_UCD90160_VOLT9:
    case PIM2_UCD90160_VOLT9:
    case PIM3_UCD90160_VOLT9:
    case PIM4_UCD90160_VOLT9:
    case PIM5_UCD90160_VOLT9:
    case PIM6_UCD90160_VOLT9:
    case PIM7_UCD90160_VOLT9:
    case PIM8_UCD90160_VOLT9:
      dir_pim_sensor_hwmon(full_name, i2cbus, PIM_UCD90160_DEVICE_CH, PIM_UCD90160_DEVICE_ADDR);
      ret = read_attr(fru, sensor_num, full_name, VOLT(9), value);
      break;
    case PIM1_UCD90160_VOLT10:
    case PIM2_UCD90160_VOLT10:
    case PIM3_UCD90160_VOLT10:
    case PIM4_UCD90160_VOLT10:
    case PIM5_UCD90160_VOLT10:
    case PIM6_UCD90160_VOLT10:
    case PIM7_UCD90160_VOLT10:
    case PIM8_UCD90160_VOLT10:
      dir_pim_sensor_hwmon(full_name, i2cbus, PIM_UCD90160_DEVICE_CH, PIM_UCD90160_DEVICE_ADDR);
      ret = read_attr(fru, sensor_num, full_name, VOLT(10), value);
      break;
    case PIM1_UCD90160_VOLT11:
    case PIM2_UCD90160_VOLT11:
    case PIM3_UCD90160_VOLT11:
    case PIM4_UCD90160_VOLT11:
    case PIM5_UCD90160_VOLT11:
    case PIM6_UCD90160_VOLT11:
    case PIM7_UCD90160_VOLT11:
    case PIM8_UCD90160_VOLT11:
      dir_pim_sensor_hwmon(full_name, i2cbus, PIM_UCD90160_DEVICE_CH, PIM_UCD90160_DEVICE_ADDR);
      ret = read_attr(fru, sensor_num, full_name, VOLT(11), value);
      break;
    case PIM1_UCD90160_VOLT12:
    case PIM2_UCD90160_VOLT12:
    case PIM3_UCD90160_VOLT12:
    case PIM4_UCD90160_VOLT12:
    case PIM5_UCD90160_VOLT12:
    case PIM6_UCD90160_VOLT12:
    case PIM7_UCD90160_VOLT12:
    case PIM8_UCD90160_VOLT12:
      dir_pim_sensor_hwmon(full_name, i2cbus, PIM_UCD90160_DEVICE_CH, PIM_UCD90160_DEVICE_ADDR);
      ret = read_attr(fru, sensor_num, full_name, VOLT(12), value);
      break;
    case PIM1_UCD90160_VOLT13:
    case PIM2_UCD90160_VOLT13:
    case PIM3_UCD90160_VOLT13:
    case PIM4_UCD90160_VOLT13:
    case PIM5_UCD90160_VOLT13:
    case PIM6_UCD90160_VOLT13:
    case PIM7_UCD90160_VOLT13:
    case PIM8_UCD90160_VOLT13:
      dir_pim_sensor_hwmon(full_name, i2cbus, PIM_UCD90160_DEVICE_CH, PIM_UCD90160_DEVICE_ADDR);
      ret = read_attr(fru, sensor_num, full_name, VOLT(13), value);
      break;
    case PIM1_MP2975_INPUT_VOLTAGE:
    case PIM2_MP2975_INPUT_VOLTAGE:
    case PIM3_MP2975_INPUT_VOLTAGE:
    case PIM4_MP2975_INPUT_VOLTAGE:
    case PIM5_MP2975_INPUT_VOLTAGE:
    case PIM6_MP2975_INPUT_VOLTAGE:
    case PIM7_MP2975_INPUT_VOLTAGE:
    case PIM8_MP2975_INPUT_VOLTAGE:
       dir_pim_sensor_hwmon(full_name, i2cbus, PIM_MP2975_DEVICE_CH, PIM_MP2975_DEVICE_ADDR);
       ret = read_attr(fru, sensor_num, full_name, VOLT(1), value);
       break;
    case PIM1_MP2975_OUTPUT_VOLTAGE_XP0R8V:
    case PIM2_MP2975_OUTPUT_VOLTAGE_XP0R8V:
    case PIM3_MP2975_OUTPUT_VOLTAGE_XP0R8V:
    case PIM4_MP2975_OUTPUT_VOLTAGE_XP0R8V:
    case PIM5_MP2975_OUTPUT_VOLTAGE_XP0R8V:
    case PIM6_MP2975_OUTPUT_VOLTAGE_XP0R8V:
    case PIM7_MP2975_OUTPUT_VOLTAGE_XP0R8V:
    case PIM8_MP2975_OUTPUT_VOLTAGE_XP0R8V:
       dir_pim_sensor_hwmon(full_name, i2cbus, PIM_MP2975_DEVICE_CH, PIM_MP2975_DEVICE_ADDR);
       ret = read_attr(fru, sensor_num, full_name, VOLT(2), value);
       break;
    case PIM1_MP2975_OUTPUT_CURRENT_XP0R8V:
    case PIM2_MP2975_OUTPUT_CURRENT_XP0R8V:
    case PIM3_MP2975_OUTPUT_CURRENT_XP0R8V:
    case PIM4_MP2975_OUTPUT_CURRENT_XP0R8V:
    case PIM5_MP2975_OUTPUT_CURRENT_XP0R8V:
    case PIM6_MP2975_OUTPUT_CURRENT_XP0R8V:
    case PIM7_MP2975_OUTPUT_CURRENT_XP0R8V:
    case PIM8_MP2975_OUTPUT_CURRENT_XP0R8V:
       dir_pim_sensor_hwmon(full_name, i2cbus, PIM_MP2975_DEVICE_CH, PIM_MP2975_DEVICE_ADDR);
       ret = read_attr(fru, sensor_num, full_name, CURR(2), value);
       break;
    case PIM1_MP2975_OUTPUT_VOLTAGE_XP3R3V:
    case PIM2_MP2975_OUTPUT_VOLTAGE_XP3R3V:
    case PIM3_MP2975_OUTPUT_VOLTAGE_XP3R3V:
    case PIM4_MP2975_OUTPUT_VOLTAGE_XP3R3V:
    case PIM5_MP2975_OUTPUT_VOLTAGE_XP3R3V:
    case PIM6_MP2975_OUTPUT_VOLTAGE_XP3R3V:
    case PIM7_MP2975_OUTPUT_VOLTAGE_XP3R3V:
    case PIM8_MP2975_OUTPUT_VOLTAGE_XP3R3V:
       dir_pim_sensor_hwmon(full_name, i2cbus, PIM_MP2975_DEVICE_CH, PIM_MP2975_DEVICE_ADDR);
       ret = read_attr(fru, sensor_num, full_name, VOLT(3), value);
	   *value = *value * 3;
       break;
    case PIM1_MP2975_OUTPUT_CURRENT_XP3R3V:
    case PIM2_MP2975_OUTPUT_CURRENT_XP3R3V:
    case PIM3_MP2975_OUTPUT_CURRENT_XP3R3V:
    case PIM4_MP2975_OUTPUT_CURRENT_XP3R3V:
    case PIM5_MP2975_OUTPUT_CURRENT_XP3R3V:
    case PIM6_MP2975_OUTPUT_CURRENT_XP3R3V:
    case PIM7_MP2975_OUTPUT_CURRENT_XP3R3V:
    case PIM8_MP2975_OUTPUT_CURRENT_XP3R3V:
       dir_pim_sensor_hwmon(full_name, i2cbus, PIM_MP2975_DEVICE_CH, PIM_MP2975_DEVICE_ADDR);
       ret = read_attr(fru, sensor_num, full_name, CURR(3), value);
       break;
    default:
      ret = READING_NA;
      break;
  }
  return ret;
}

static int
psu_init_acok_key(uint8_t fru) {
  uint8_t psu_num = fru - FRU_PSU1 + 1;
  char key[MAX_KEY_LEN + 1];

  snprintf(key, MAX_KEY_LEN, "psu%d_acok_state", psu_num);
  kv_set(key, "1", 0, KV_FCREATE);

  return 0;
}

static int
psu_acok_log(uint8_t fru, uint8_t curr_state) {
  uint8_t psu_num = fru - FRU_PSU1 + 1;
  int ret, old_state;
  char key[MAX_KEY_LEN + 1];
  char cvalue[MAX_VALUE_LEN] = {0};

  snprintf(key, MAX_KEY_LEN, "psu%d_acok_state", psu_num);
  ret = kv_get(key, cvalue, NULL, 0);
  if (ret < 0) {
    return ret;
  }

  old_state = atoi(cvalue);

  if (curr_state != old_state) {
    if (curr_state == PSU_ACOK_UP) {
      pal_update_ts_sled();
      syslog(LOG_CRIT, "DEASSERT: PSU%d AC OK state - AC power up - FRU: %d",
             psu_num, fru);
      kv_set(key, "1", 0, 0);
    } else {
      pal_update_ts_sled();
      syslog(LOG_CRIT, "ASSERT: PSU%d AC OK state - AC power down - FRU: %d",
             psu_num, fru);
      kv_set(key, "0", 0, 0);
    }
  }

  return 0;
}

static int
psu_acok_check(uint8_t fru) {
  int val;
  char path[LARGEST_DEVICE_NAME + 1];

  if (fru == FRU_PSU1) {
    snprintf(path, LARGEST_DEVICE_NAME, SMBCPLD_PATH_FMT, "psu_L2_input_ok");
  } else if (fru == FRU_PSU2) {
    snprintf(path, LARGEST_DEVICE_NAME, SMBCPLD_PATH_FMT, "psu_L1_input_ok");
  } else if (fru == FRU_PSU3) {
    snprintf(path, LARGEST_DEVICE_NAME, SMBCPLD_PATH_FMT, "psu_R2_input_ok");
  } else if (fru == FRU_PSU4) {
    snprintf(path, LARGEST_DEVICE_NAME, SMBCPLD_PATH_FMT, "psu_R1_input_ok");
  }

  if (read_device(path, &val)) {
    syslog(LOG_ERR, "%s cannot get value from %s", __func__, path);
    return 0;
  }
  if (!val) {
    return READING_NA;
  }

  return val;
}

static int
psu_sensor_read(uint8_t fru, uint8_t sensor_num, float *value) {
  int ret = -1;

  ret = psu_acok_check(fru);
  if (ret == READING_NA) {
    psu_acok_log(fru, PSU_ACOK_DOWN);
    goto psu_out;
  }
  psu_acok_log(fru, PSU_ACOK_UP);

  switch(sensor_num) {
    case PSU1_SENSOR_IN_VOLT:
      ret = read_attr(fru, sensor_num, PSU1_DEVICE, VOLT(0), value);
      break;
    case PSU1_SENSOR_12V_VOLT:
      ret = read_attr(fru, sensor_num, PSU1_DEVICE, VOLT(1), value);
      break;
    case PSU1_SENSOR_STBY_VOLT:
      ret = read_attr(fru, sensor_num, PSU1_DEVICE, VOLT(2), value);
      break;
    case PSU1_SENSOR_IN_CURR:
      ret = read_attr(fru, sensor_num, PSU1_DEVICE, CURR(1), value);
      break;
    case PSU1_SENSOR_12V_CURR:
      ret = read_attr(fru, sensor_num, PSU1_DEVICE, CURR(2), value);
      break;
    case PSU1_SENSOR_STBY_CURR:
      ret = read_attr(fru, sensor_num, PSU1_DEVICE, CURR(3), value);
      break;
    case PSU1_SENSOR_IN_POWER:
      ret = read_attr(fru, sensor_num, PSU1_DEVICE, POWER(1), value);
      break;
    case PSU1_SENSOR_12V_POWER:
      ret = read_attr(fru, sensor_num, PSU1_DEVICE, POWER(2), value);
      break;
    case PSU1_SENSOR_STBY_POWER:
      ret = read_attr(fru, sensor_num, PSU1_DEVICE, POWER(3), value);
      break;
    case PSU1_SENSOR_FAN_TACH:
      ret = read_fan_rpm_f(PSU1_DEVICE, 1, value);
      break;
    case PSU1_SENSOR_TEMP1:
      ret = read_attr(fru, sensor_num, PSU1_DEVICE, TEMP(1), value);
      break;
    case PSU1_SENSOR_TEMP2:
      ret = read_attr(fru, sensor_num, PSU1_DEVICE, TEMP(2), value);
      break;
    case PSU1_SENSOR_TEMP3:
      ret = read_attr(fru, sensor_num, PSU1_DEVICE, TEMP(3), value);
      break;
    case PSU2_SENSOR_IN_VOLT:
      ret = read_attr(fru, sensor_num, PSU2_DEVICE, VOLT(0), value);
      break;
    case PSU2_SENSOR_12V_VOLT:
      ret = read_attr(fru, sensor_num, PSU2_DEVICE, VOLT(1), value);
      break;
    case PSU2_SENSOR_STBY_VOLT:
      ret = read_attr(fru, sensor_num, PSU2_DEVICE, VOLT(2), value);
      break;
    case PSU2_SENSOR_IN_CURR:
      ret = read_attr(fru, sensor_num, PSU2_DEVICE, CURR(1), value);
      break;
    case PSU2_SENSOR_12V_CURR:
      ret = read_attr(fru, sensor_num, PSU2_DEVICE, CURR(2), value);
      break;
    case PSU2_SENSOR_STBY_CURR:
      ret = read_attr(fru, sensor_num, PSU2_DEVICE, CURR(3), value);
      break;
    case PSU2_SENSOR_IN_POWER:
      ret = read_attr(fru, sensor_num, PSU2_DEVICE, POWER(1), value);
      break;
    case PSU2_SENSOR_12V_POWER:
      ret = read_attr(fru, sensor_num, PSU2_DEVICE, POWER(2), value);
      break;
    case PSU2_SENSOR_STBY_POWER:
      ret = read_attr(fru, sensor_num, PSU2_DEVICE, POWER(3), value);
      break;
    case PSU2_SENSOR_FAN_TACH:
      ret = read_fan_rpm_f(PSU2_DEVICE, 1, value);
      break;
    case PSU2_SENSOR_TEMP1:
      ret = read_attr(fru, sensor_num, PSU2_DEVICE, TEMP(1), value);
      break;
    case PSU2_SENSOR_TEMP2:
      ret = read_attr(fru, sensor_num, PSU2_DEVICE, TEMP(2), value);
      break;
    case PSU2_SENSOR_TEMP3:
      ret = read_attr(fru, sensor_num, PSU2_DEVICE, TEMP(3), value);
      break;
    case PSU3_SENSOR_IN_VOLT:
      ret = read_attr(fru, sensor_num, PSU3_DEVICE, VOLT(0), value);
      break;
    case PSU3_SENSOR_12V_VOLT:
      ret = read_attr(fru, sensor_num, PSU3_DEVICE, VOLT(1), value);
      break;
    case PSU3_SENSOR_STBY_VOLT:
      ret = read_attr(fru, sensor_num, PSU3_DEVICE, VOLT(2), value);
      break;
    case PSU3_SENSOR_IN_CURR:
      ret = read_attr(fru, sensor_num, PSU3_DEVICE, CURR(1), value);
      break;
    case PSU3_SENSOR_12V_CURR:
      ret = read_attr(fru, sensor_num, PSU3_DEVICE, CURR(2), value);
      break;
    case PSU3_SENSOR_STBY_CURR:
      ret = read_attr(fru, sensor_num, PSU3_DEVICE, CURR(3), value);
      break;
    case PSU3_SENSOR_IN_POWER:
      ret = read_attr(fru, sensor_num, PSU3_DEVICE, POWER(1), value);
      break;
    case PSU3_SENSOR_12V_POWER:
      ret = read_attr(fru, sensor_num, PSU3_DEVICE, POWER(2), value);
      break;
    case PSU3_SENSOR_STBY_POWER:
      ret = read_attr(fru, sensor_num, PSU3_DEVICE, POWER(3), value);
      break;
    case PSU3_SENSOR_FAN_TACH:
      ret = read_fan_rpm_f(PSU3_DEVICE, 1, value);
      break;
    case PSU3_SENSOR_TEMP1:
      ret = read_attr(fru, sensor_num, PSU3_DEVICE, TEMP(1), value);
      break;
    case PSU3_SENSOR_TEMP2:
      ret = read_attr(fru, sensor_num, PSU3_DEVICE, TEMP(2), value);
      break;
    case PSU3_SENSOR_TEMP3:
      ret = read_attr(fru, sensor_num, PSU3_DEVICE, TEMP(3), value);
      break;
    case PSU4_SENSOR_IN_VOLT:
      ret = read_attr(fru, sensor_num, PSU4_DEVICE, VOLT(0), value);
      break;
    case PSU4_SENSOR_12V_VOLT:
      ret = read_attr(fru, sensor_num, PSU4_DEVICE, VOLT(1), value);
      break;
    case PSU4_SENSOR_STBY_VOLT:
      ret = read_attr(fru, sensor_num, PSU4_DEVICE, VOLT(2), value);
      break;
    case PSU4_SENSOR_IN_CURR:
      ret = read_attr(fru, sensor_num, PSU4_DEVICE, CURR(1), value);
      break;
    case PSU4_SENSOR_12V_CURR:
      ret = read_attr(fru, sensor_num, PSU4_DEVICE, CURR(2), value);
      break;
    case PSU4_SENSOR_STBY_CURR:
      ret = read_attr(fru, sensor_num, PSU4_DEVICE, CURR(3), value);
      break;
    case PSU4_SENSOR_IN_POWER:
      ret = read_attr(fru, sensor_num, PSU4_DEVICE, POWER(1), value);
      break;
    case PSU4_SENSOR_12V_POWER:
      ret = read_attr(fru, sensor_num, PSU4_DEVICE, POWER(2), value);
      break;
    case PSU4_SENSOR_STBY_POWER:
      ret = read_attr(fru, sensor_num, PSU4_DEVICE, POWER(3), value);
      break;
    case PSU4_SENSOR_FAN_TACH:
      ret = read_fan_rpm_f(PSU4_DEVICE, 1, value);
      break;
    case PSU4_SENSOR_TEMP1:
      ret = read_attr(fru, sensor_num, PSU4_DEVICE, TEMP(1), value);
      break;
    case PSU4_SENSOR_TEMP2:
      ret = read_attr(fru, sensor_num, PSU4_DEVICE, TEMP(2), value);
      break;
    case PSU4_SENSOR_TEMP3:
      ret = read_attr(fru, sensor_num, PSU4_DEVICE, TEMP(3), value);
      break;
    default:
      ret = READING_NA;
      break;
  }

psu_out:
  return ret;
}

int
pal_sensor_read_raw(uint8_t fru, uint8_t sensor_num, void *value) {

  char key[MAX_KEY_LEN];
  char fru_name[32];
  int ret , delay = 500;
  uint8_t prsnt = 0;

  pal_get_fru_name(fru, fru_name);

  ret = pal_is_fru_prsnt(fru, &prsnt);
  if (ret) {
    return ret;
  }
  if (!prsnt) {
#ifdef DEBUG
  syslog(LOG_INFO, "pal_sensor_read_raw(): %s is not present\n", fru_name);
#endif
    return -1;
  }

  sprintf(key, "%s_sensor%d", fru_name, sensor_num);
  switch(fru) {
    case FRU_SCM:
      ret = scm_sensor_read(fru, sensor_num, value);
      break;
    case FRU_SMB:
      ret = smb_sensor_read(fru, sensor_num, value);
      break;
    case FRU_PIM1:
    case FRU_PIM2:
    case FRU_PIM3:
    case FRU_PIM4:
    case FRU_PIM5:
    case FRU_PIM6:
    case FRU_PIM7:
    case FRU_PIM8:
      ret = pim_sensor_read(fru, sensor_num, value);
      if (sensor_num >= PIM1_SENSOR_QSFP_TEMP &&
          sensor_num <= PIM8_SENSOR_QSFP_TEMP) {
        delay = 100;
      }
      break;
    case FRU_PSU1:
    case FRU_PSU2:
    case FRU_PSU3:
    case FRU_PSU4:
      ret = psu_sensor_read(fru, sensor_num, value);
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

static int
get_scm_sensor_name(uint8_t sensor_num, char *name) {

  switch(sensor_num) {
    case SCM_SENSOR_OUTLET_U7_TEMP:
      sprintf(name, "SCM_OUTLET_U7_TEMP");
      break;
    case SCM_SENSOR_INLET_U8_TEMP:
      sprintf(name, "SCM_INLET_U8_TEMP");
      break;
    case SCM_SENSOR_BMC_LM75_U9_TEMP:
      sprintf(name, "BMC_LM75_U9_TEMP");
      break;
    case SCM_SENSOR_HSC_VOLT:
      sprintf(name, "SCM_INPUT_VOLTAGE");
      break;
    case SCM_SENSOR_POWER_VOLT:
      sprintf(name, "SCM_SENSOR_POWER_VOLTAGE");
      break;
    case SCM_SENSOR_HSC_CURR:
      sprintf(name, "SCM_CURRENT");
      break;
    case SCM_SENSOR_HSC_POWER:
      sprintf(name, "SCM_POWER");
      break;
    case BIC_SENSOR_MB_OUTLET_TEMP:
      sprintf(name, "MB_OUTLET_TEMP");
      break;
    case PIM3_MP2975_OUTPUT_CURRENT_XP0R8V:
      sprintf(name, "PIM3_MP2975_OUTPUT_CURR_XP0R8V");
    case BIC_SENSOR_MB_INLET_TEMP:
      sprintf(name, "MB_INLET_TEMP");
      break;
    case BIC_SENSOR_PCH_TEMP:
      sprintf(name, "PCH_TEMP");
      break;
    case BIC_SENSOR_VCCIN_VR_TEMP:
      sprintf(name, "VCCIN_VR_TEMP");
      break;
    case BIC_SENSOR_1V05MIX_VR_TEMP:
      sprintf(name, "1V05MIX_VR_TEMP");
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
    case BIC_SENSOR_SOC_DIMMA0_TEMP:
      sprintf(name, "SOC_DIMMA0_TEMP");
      break;
    case BIC_SENSOR_SOC_DIMMB0_TEMP:
      sprintf(name, "SOC_DIMMB0_TEMP");
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
    case BIC_SENSOR_P1V05_MIX:
      sprintf(name, "P1V05_MIX_VOLT");
      break;
    case BIC_SENSOR_1V05MIX_VR_CURR:
      sprintf(name, "1V05MIX_VR_CURR");
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
    case BIC_SENSOR_P1V05MIX_VR_VOL:
      sprintf(name, "1V05MIX_VR_VOLT");
      break;
    case BIC_SENSOR_P1V05MIX_VR_POUT:
      sprintf(name, "1V05MIX_VR_OUT_POWER");
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

static int
get_smb_sensor_name(uint8_t sensor_num, char *name) {

  switch(sensor_num) {
    case SMB_TMP422_U20_1_TEMP:
      sprintf(name, "SMB_TMP422_U20_1_TEMP");
      break;
    case SMB_TMP422_U20_2_TEMP:
      sprintf(name, "SMB_TMP422_U20_2_TEMP");
      break;
    case SMB_TMP422_U20_3_TEMP:
      sprintf(name, "SMB_TMP422_U20_3_TEMP");
      break;
    case SIM_LM75_U1_TEMP:
      sprintf(name, "SIM_LM75_U1_TEMP");
      break;
    case SMB_SENSOR_TEMP1:
      sprintf(name, "SMB_LM75B_U51_1_TEMP");
      break;
    case SMB_SENSOR_TEMP2:
      sprintf(name, "SMB_LM75B_U57_TEMP");
      break;
    case SMB_SENSOR_TEMP3:
      sprintf(name, "SMB_LM75B_U39_TEMP");
      break;
    case SMB_VDDC_SW_TEMP:
      sprintf(name, "SMB_VDDC_SW_TEMP");
      break;
    case SMB_XP12R0V_VDDC_SW_IN:
      sprintf(name, "SMB_XP12R0V_VDDC_SW_IN");
      break;
    case SMB_VDDC_SW_IN_SENS:
      sprintf(name, "SMB_VDDC_SW_IN_SENS");
      break;
    case SMB_VDDC_SW_POWER_IN:
      sprintf(name, "SMB_VDDC_SW_POWER_IN");
      break;
    case SMB_VDDC_SW_POWER_OUT:
      sprintf(name, "SMB_VDDC_SW_POWER_OUT");
      break;
    case SMB_VDDC_SW_CURR_IN:
      sprintf(name, "SMB_VDDC_SW_CURR_IN");
      break;
    case SMB_VDDC_SW_CURR_OUT:
      sprintf(name, "SMB_VDDC_SW_CURR_OUT");
      break;
    case SMB_SENSOR_PDB_L_TEMP1:
      sprintf(name, "PDB_L_TMP75_U2_TEMP");
      break;
    case SMB_SENSOR_PDB_L_TEMP2:
      sprintf(name, "PDB_L_TMP75_U3_TEMP");
      break;
    case SMB_SENSOR_PDB_R_TEMP1:
      sprintf(name, "PDB_R_TMP75_U2_TEMP");
      break;
    case SMB_SENSOR_PDB_R_TEMP2:
      sprintf(name, "PDB_R_TMP75_U3_TEMP");
      break;
    case SMB_SENSOR_FCM_T_TEMP1:
      sprintf(name, "FCB_FCM1_TMP75_U1_TEMP");
      break;
    case SMB_SENSOR_FCM_T_TEMP2:
      sprintf(name, "FCB_FCM1_TMP75_U2_TEMP");
      break;
    case SMB_SENSOR_FCM_B_TEMP1:
      sprintf(name, "FCB_FCM2_TMP75_U1_TEMP");
      break;
    case SMB_SENSOR_FCM_B_TEMP2:
      sprintf(name, "FCB_FCM2_TMP75_U2_TEMP");
      break;
    case SMB_XP3R3V_BMC:
      sprintf(name, "SMB_XP3R3V_BMC");
      break;
    case SMB_XP2R5V_BMC:
      sprintf(name, "SMB_XP2R5V_BMC");
      break;
    case SMB_XP1R8V_BMC:
      sprintf(name, "SMB_XP1R8V_BMC");
      break;
    case SMB_XP1R2V_BMC:
      sprintf(name, "SMB_XP1R2V_BMC");
      break;
    case SMB_XP1R0V_FPGA:
      sprintf(name, "SMB_XP1R0V_FPGA");
      break;
    case SMB_XP3R3V_USB:
      sprintf(name, "SMB_XP3R3V_USB");
      break;
    case SMB_XP5R0V:
      sprintf(name, "SMB_XP5R0V");
      break;
    case SMB_XP3R3V_EARLY:
      sprintf(name, "SMB_XP3R3V_EARLY");
      break;
    case SMB_LM57_VTEMP:
      sprintf(name, "SMB_LM57_VTEMP");
      break;
    case SMB_XP1R8:
      sprintf(name, "SMB_XP1R8");
      break;
    case SMB_XP1R2:
      sprintf(name, "SMB_XP1R2");
      break;
    case SMB_VDDC_SW:
      sprintf(name, "SMB_VDDC_SW");
      break;
    case SMB_XP3R3V:
      sprintf(name, "SMB_XP3R3V");
      break;
    case SMB_XP1R8V_AVDD:
      sprintf(name, "SMB_XP1R8V_AVDD");
      break;
    case SMB_XP1R2V_TVDD:
      sprintf(name, "SMB_XP1R2V_TVDD");
      break;
    case SMB_XP0R75V_1_PVDD:
      sprintf(name, "SMB_XP0R75V_1_PVDD");
      break;
    case SMB_XP0R75V_2_PVDD:
      sprintf(name, "SMB_XP0R75V_2_PVDD");
      break;
    case SMB_XP0R75V_3_PVDD:
      sprintf(name, "SMB_XP0R75V_3_PVDD");
      break;
    case SMB_VDD_PCIE:
      sprintf(name, "SMB_VDD_PCIE");
      break;
    case SMB_XP0R84V_DCSU:
      sprintf(name, "SMB_XP0R84V_DCSU");
      break;
    case SMB_XP0R84V_CSU:
      sprintf(name, "SMB_XP0R84V_CSU");
      break;
    case SMB_XP1R84V_CSU:
      sprintf(name, "SMB_XP1R84V_CSU");
      break;
    case SMB_XP3R3V_TCXO:
      sprintf(name, "SMB_XP3R3V_TCXO");
      break;
    case SMB_OUTPUT_VOLTAGE_XP0R75V_1:
      sprintf(name, "SMB_OUTPUT_VOLTAGE_XP0R75V_1");
      break;
    case SMB_OUTPUT_CURRENT_XP0R75V_1:
      sprintf(name, "SMB_OUTPUT_CURRENT_XP0R75V_1");
      break;
    case SMB_INPUT_VOLTAGE_1:
      sprintf(name, "SMB_INPUT_VOLTAGE_1");
      break;
    case SMB_OUTPUT_VOLTAGE_XP1R2V:
      sprintf(name, "SMB_OUTPUT_VOLTAGE_XP1R2V");
      break;
    case SMB_OUTPUT_CURRENT_XP1R2V:
      sprintf(name, "SMB_OUTPUT_CURRENT_XP1R2V");
      break;
    case SMB_OUTPUT_VOLTAGE_XP0R75V_2:
      sprintf(name, "SMB_OUTPUT_VOLTAGE_XP0R75V_2");
      break;
    case SMB_OUTPUT_CURRENT_XP0R75V_2:
      sprintf(name, "SMB_OUTPUT_CURRENT_XP0R75V_2");
      break;
    case SMB_INPUT_VOLTAGE_2:
      sprintf(name, "SMB_INPUT_VOLTAGE_2");
      break;
    case SMB_SENSOR_FCM_T_HSC_VOLT:
      sprintf(name, "FCM-T_CHIP_INPUT_VOLTAGE");
      break;
    case SMB_SENSOR_FCM_B_HSC_VOLT:
      sprintf(name, "FCM-B_CHIP_INPUT_VOLTAGE");
      break;
    case SMB_SENSOR_FCM_T_HSC_CURR:
      sprintf(name, "FCM-T_POWER_CURRENT");
      break;
    case SMB_SENSOR_FCM_B_HSC_CURR:
      sprintf(name, "FCM-B_POWER_CURRENT");
      break;
    case SMB_SENSOR_FCM_T_HSC_POWER_VOLT:
      sprintf(name, "FCM-T_POWER_VOLTAGE");
      break;
    case SMB_SENSOR_FCM_B_HSC_POWER_VOLT:
      sprintf(name, "FCM-B_POWER_VOLTAGE");
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
    case SMB_SENSOR_FAN5_FRONT_TACH:
      sprintf(name, "FAN5_FRONT_SPEED");
      break;
    case SMB_SENSOR_FAN5_REAR_TACH:
      sprintf(name, "FAN5_REAR_SPEED");
      break;
    case SMB_SENSOR_FAN6_FRONT_TACH:
      sprintf(name, "FAN6_FRONT_SPEED");
      break;
    case SMB_SENSOR_FAN6_REAR_TACH:
      sprintf(name, "FAN6_REAR_SPEED");
      break;
    case SMB_SENSOR_FAN7_FRONT_TACH:
      sprintf(name, "FAN7_FRONT_SPEED");
      break;
    case SMB_SENSOR_FAN7_REAR_TACH:
      sprintf(name, "FAN7_REAR_SPEED");
      break;
    case SMB_SENSOR_FAN8_FRONT_TACH:
      sprintf(name, "FAN8_FRONT_SPEED");
      break;
    case SMB_SENSOR_FAN8_REAR_TACH:
      sprintf(name, "FAN8_REAR_SPEED");
      break;
    case SMB_SENSOR_TH4_HIGH:
      sprintf(name, "SMB_SENSOR_TH4_HIGH");
      break;
    default:
      return -1;
  }
  return 0;
}

static int
get_pim_sensor_name(uint8_t sensor_num, uint8_t fru, char *name) {
  uint8_t type = pal_get_pim_type_from_file(fru);
  uint8_t pimid = fru-FRU_PIM1+1;
  switch(sensor_num) {
    case PIM1_LM75_TEMP_4A:
    case PIM2_LM75_TEMP_4A:
    case PIM3_LM75_TEMP_4A:
    case PIM4_LM75_TEMP_4A:
    case PIM5_LM75_TEMP_4A:
    case PIM6_LM75_TEMP_4A:
    case PIM7_LM75_TEMP_4A:
    case PIM8_LM75_TEMP_4A:
      if (type==PIM_TYPE_16Q)
        sprintf(name, "PIM%d_LM75_U37_TEMP_BASE",pimid);
      else if (type==PIM_TYPE_16O)
        sprintf(name, "PIM%d_LM75_4A(unuse)",pimid);
      break;
    case PIM1_LM75_TEMP_4B:
    case PIM2_LM75_TEMP_4B:
    case PIM3_LM75_TEMP_4B:
    case PIM4_LM75_TEMP_4B:
    case PIM5_LM75_TEMP_4B:
    case PIM6_LM75_TEMP_4B:
    case PIM7_LM75_TEMP_4B:
    case PIM8_LM75_TEMP_4B:
      if (type==PIM_TYPE_16Q)
        sprintf(name, "PIM%d_LM75_U26_TEMP",pimid);
      else if (type==PIM_TYPE_16O)
        sprintf(name, "PIM%d_LM75_U27_TEMP_FRONT",pimid);
      break;
    case PIM1_LM75_TEMP_48:
    case PIM2_LM75_TEMP_48:
    case PIM3_LM75_TEMP_48:
    case PIM4_LM75_TEMP_48:
    case PIM5_LM75_TEMP_48:
    case PIM6_LM75_TEMP_48:
    case PIM7_LM75_TEMP_48:
    case PIM8_LM75_TEMP_48:
      if (type==PIM_TYPE_16Q)
        sprintf(name, "PIM%d_LM75_U37_TEMP_MEZZ",pimid);
      else if (type==PIM_TYPE_16O)
        sprintf(name, "PIM%d_LM75_U28_TEMP_REAR",pimid);
      break;
    case PIM1_SENSOR_QSFP_TEMP:
    case PIM2_SENSOR_QSFP_TEMP:
    case PIM3_SENSOR_QSFP_TEMP:
    case PIM4_SENSOR_QSFP_TEMP:
    case PIM5_SENSOR_QSFP_TEMP:
    case PIM6_SENSOR_QSFP_TEMP:
    case PIM7_SENSOR_QSFP_TEMP:
    case PIM8_SENSOR_QSFP_TEMP:
      sprintf(name, "PIM%d_QSFP_TEMP",sensor_num-PIM1_SENSOR_QSFP_TEMP+1);
      break;
    case PIM1_SENSOR_HSC_VOLT:
    case PIM2_SENSOR_HSC_VOLT:
    case PIM3_SENSOR_HSC_VOLT:
    case PIM4_SENSOR_HSC_VOLT:
    case PIM5_SENSOR_HSC_VOLT:
    case PIM6_SENSOR_HSC_VOLT:
    case PIM7_SENSOR_HSC_VOLT:
    case PIM8_SENSOR_HSC_VOLT:
      sprintf(name, "PIM%d_INPUT_VOLTAGE",pimid);
      break;
    case PIM1_POWER_VOLTAGE:
    case PIM2_POWER_VOLTAGE:
    case PIM3_POWER_VOLTAGE:
    case PIM4_POWER_VOLTAGE:
    case PIM5_POWER_VOLTAGE:
    case PIM6_POWER_VOLTAGE:
    case PIM7_POWER_VOLTAGE:
    case PIM8_POWER_VOLTAGE:
      sprintf(name, "PIM%d_POWER_VOLTAGE",pimid);
      break;
    case PIM1_SENSOR_HSC_CURR:
    case PIM2_SENSOR_HSC_CURR:
    case PIM3_SENSOR_HSC_CURR:
    case PIM4_SENSOR_HSC_CURR:
    case PIM5_SENSOR_HSC_CURR:
    case PIM6_SENSOR_HSC_CURR:
    case PIM7_SENSOR_HSC_CURR:
    case PIM8_SENSOR_HSC_CURR:
      sprintf(name, "PIM%d_CURRENT",pimid);
      break;
    case PIM1_SENSOR_HSC_POWER:
    case PIM2_SENSOR_HSC_POWER:
    case PIM3_SENSOR_HSC_POWER:
    case PIM4_SENSOR_HSC_POWER:
    case PIM5_SENSOR_HSC_POWER:
    case PIM6_SENSOR_HSC_POWER:
    case PIM7_SENSOR_HSC_POWER:
    case PIM8_SENSOR_HSC_POWER:
      sprintf(name, "PIM%d_POWER",pimid);
      break;
    case PIM1_UCD90160_VOLT1:
    case PIM2_UCD90160_VOLT1:
    case PIM3_UCD90160_VOLT1:
    case PIM4_UCD90160_VOLT1:
    case PIM5_UCD90160_VOLT1:
    case PIM6_UCD90160_VOLT1:
    case PIM7_UCD90160_VOLT1:
    case PIM8_UCD90160_VOLT1:
      sprintf(name, "PIM%d_XP3R3V",pimid);
      break;
    case PIM1_UCD90160_VOLT2:
    case PIM2_UCD90160_VOLT2:
    case PIM3_UCD90160_VOLT2:
    case PIM4_UCD90160_VOLT2:
    case PIM5_UCD90160_VOLT2:
    case PIM6_UCD90160_VOLT2:
    case PIM7_UCD90160_VOLT2:
    case PIM8_UCD90160_VOLT2:
      if (type==PIM_TYPE_16Q)
        sprintf(name, "PIM%d_XP3R3V_EARLY",pimid);
      else if (type==PIM_TYPE_16O)
        sprintf(name, "PIM%d_XP2R5V",pimid);
      break;
    case PIM1_UCD90160_VOLT3:
    case PIM2_UCD90160_VOLT3:
    case PIM3_UCD90160_VOLT3:
    case PIM4_UCD90160_VOLT3:
    case PIM5_UCD90160_VOLT3:
    case PIM6_UCD90160_VOLT3:
    case PIM7_UCD90160_VOLT3:
    case PIM8_UCD90160_VOLT3:
      if (type==PIM_TYPE_16Q)
        sprintf(name, "PIM%d_XP2R5V_EARLY",pimid);
      else if (type==PIM_TYPE_16O)
        sprintf(name, "PIM%d_XP1R1V",pimid);
      break;
    case PIM1_UCD90160_VOLT4:
    case PIM2_UCD90160_VOLT4:
    case PIM3_UCD90160_VOLT4:
    case PIM4_UCD90160_VOLT4:
    case PIM5_UCD90160_VOLT4:
    case PIM6_UCD90160_VOLT4:
    case PIM7_UCD90160_VOLT4:
    case PIM8_UCD90160_VOLT4:
      if (type==PIM_TYPE_16Q)
        sprintf(name, "PIM%d_TXDRV_PHY",pimid);
      else if (type==PIM_TYPE_16O)
        sprintf(name, "PIM%d_XP1R8V",pimid);
      break;
    case PIM1_UCD90160_VOLT5:
    case PIM2_UCD90160_VOLT5:
    case PIM3_UCD90160_VOLT5:
    case PIM4_UCD90160_VOLT5:
    case PIM5_UCD90160_VOLT5:
    case PIM6_UCD90160_VOLT5:
    case PIM7_UCD90160_VOLT5:
    case PIM8_UCD90160_VOLT5:
      if (type==PIM_TYPE_16Q)
        sprintf(name, "PIM%d_XP0R8V_PHY",pimid);
      else if (type==PIM_TYPE_16O)
        sprintf(name, "PIM%d_XP3R3V_OBO",pimid);
      break;
    case PIM1_UCD90160_VOLT6:
    case PIM2_UCD90160_VOLT6:
    case PIM3_UCD90160_VOLT6:
    case PIM4_UCD90160_VOLT6:
    case PIM5_UCD90160_VOLT6:
    case PIM6_UCD90160_VOLT6:
    case PIM7_UCD90160_VOLT6:
    case PIM8_UCD90160_VOLT6:
      if (type==PIM_TYPE_16Q)
        sprintf(name, "PIM%d_XP1R1V_EARLY",pimid);
      else if (type==PIM_TYPE_16O)
        sprintf(name, "PIM%d_XP8R0V_OBO",pimid);
      break;
    case PIM1_UCD90160_VOLT7:
    case PIM2_UCD90160_VOLT7:
    case PIM3_UCD90160_VOLT7:
    case PIM4_UCD90160_VOLT7:
    case PIM5_UCD90160_VOLT7:
    case PIM6_UCD90160_VOLT7:
    case PIM7_UCD90160_VOLT7:
    case PIM8_UCD90160_VOLT7:
      if (type==PIM_TYPE_16Q)
        sprintf(name, "PIM%d_DVDD_PHY4",pimid);
      else if (type==PIM_TYPE_16O)
        sprintf(name, "PIM%d_XP0R65V_OBO",pimid);
      break;
    case PIM1_UCD90160_VOLT8:
    case PIM2_UCD90160_VOLT8:
    case PIM3_UCD90160_VOLT8:
    case PIM4_UCD90160_VOLT8:
    case PIM5_UCD90160_VOLT8:
    case PIM6_UCD90160_VOLT8:
    case PIM7_UCD90160_VOLT8:
    case PIM8_UCD90160_VOLT8:
      if (type==PIM_TYPE_16Q)
        sprintf(name, "PIM%d_DVDD_PHY3",pimid);
      else if (type==PIM_TYPE_16O)
        sprintf(name, "PIM%d_XP0R94V_OBO",pimid);
      break;
    case PIM1_UCD90160_VOLT9:
    case PIM2_UCD90160_VOLT9:
    case PIM3_UCD90160_VOLT9:
    case PIM4_UCD90160_VOLT9:
    case PIM5_UCD90160_VOLT9:
    case PIM6_UCD90160_VOLT9:
    case PIM7_UCD90160_VOLT9:
    case PIM8_UCD90160_VOLT9:
      if (type==PIM_TYPE_16Q)
        sprintf(name, "PIM%d_DVDD_PHY2",pimid);
      else if (type==PIM_TYPE_16O)
        sprintf(name, "PIM%d_XP1R15V_OBO",pimid);
      break;
    case PIM1_UCD90160_VOLT10:
    case PIM2_UCD90160_VOLT10:
    case PIM3_UCD90160_VOLT10:
    case PIM4_UCD90160_VOLT10:
    case PIM5_UCD90160_VOLT10:
    case PIM6_UCD90160_VOLT10:
    case PIM7_UCD90160_VOLT10:
    case PIM8_UCD90160_VOLT10:
      if (type==PIM_TYPE_16Q)
        sprintf(name, "PIM%d_DVDD_PHY1",pimid);
      else if (type==PIM_TYPE_16O)
        sprintf(name, "PIM%d_XP1R8V_OBO",pimid);
      break;
    case PIM1_UCD90160_VOLT11:
    case PIM2_UCD90160_VOLT11:
    case PIM3_UCD90160_VOLT11:
    case PIM4_UCD90160_VOLT11:
    case PIM5_UCD90160_VOLT11:
    case PIM6_UCD90160_VOLT11:
    case PIM7_UCD90160_VOLT11:
    case PIM8_UCD90160_VOLT11:
      if (type==PIM_TYPE_16Q)
        sprintf(name, "PIM%d_XP1R8V_EARLY",pimid);
      else if (type==PIM_TYPE_16O)
        sprintf(name, "PIM%d_XP2R0V_OBO",pimid);
      break;
    case PIM1_UCD90160_VOLT12:
    case PIM2_UCD90160_VOLT12:
    case PIM3_UCD90160_VOLT12:
    case PIM4_UCD90160_VOLT12:
    case PIM5_UCD90160_VOLT12:
    case PIM6_UCD90160_VOLT12:
    case PIM7_UCD90160_VOLT12:
    case PIM8_UCD90160_VOLT12:
      if (type==PIM_TYPE_16Q)
        sprintf(name, "PIM%d_XP1R8V_PHYIO",pimid);
      else if (type==PIM_TYPE_16O)
        sprintf(name, "PIM%d_XP3R6V_OBO",pimid);
      break;
    case PIM1_UCD90160_VOLT13:
    case PIM2_UCD90160_VOLT13:
    case PIM3_UCD90160_VOLT13:
    case PIM4_UCD90160_VOLT13:
    case PIM5_UCD90160_VOLT13:
    case PIM6_UCD90160_VOLT13:
    case PIM7_UCD90160_VOLT13:
    case PIM8_UCD90160_VOLT13:
      if (type==PIM_TYPE_16Q)
        sprintf(name, "PIM%d_XP1R8V_PHYAVDD",pimid);
      else if (type==PIM_TYPE_16O)
        sprintf(name, "PIM%d_UCD90160_VOLT13(unuse)",pimid);
      break;
    case PIM1_MP2975_INPUT_VOLTAGE:
    case PIM2_MP2975_INPUT_VOLTAGE:
    case PIM3_MP2975_INPUT_VOLTAGE:
    case PIM4_MP2975_INPUT_VOLTAGE:
    case PIM5_MP2975_INPUT_VOLTAGE:
    case PIM6_MP2975_INPUT_VOLTAGE:
    case PIM7_MP2975_INPUT_VOLTAGE:
    case PIM8_MP2975_INPUT_VOLTAGE:
      sprintf(name, "PIM%d_MP2975_INPUT_VOLTAGE",pimid);
      break;
    case PIM1_MP2975_OUTPUT_VOLTAGE_XP0R8V:
    case PIM2_MP2975_OUTPUT_VOLTAGE_XP0R8V:
    case PIM3_MP2975_OUTPUT_VOLTAGE_XP0R8V:
    case PIM4_MP2975_OUTPUT_VOLTAGE_XP0R8V:
    case PIM5_MP2975_OUTPUT_VOLTAGE_XP0R8V:
    case PIM6_MP2975_OUTPUT_VOLTAGE_XP0R8V:
    case PIM7_MP2975_OUTPUT_VOLTAGE_XP0R8V:
    case PIM8_MP2975_OUTPUT_VOLTAGE_XP0R8V:
      sprintf(name, "PIM%d_MP2975_OUTPUT_VOL_XP0R8V",pimid);
      break;
    case PIM1_MP2975_OUTPUT_CURRENT_XP0R8V:
    case PIM2_MP2975_OUTPUT_CURRENT_XP0R8V:
    case PIM3_MP2975_OUTPUT_CURRENT_XP0R8V:
    case PIM4_MP2975_OUTPUT_CURRENT_XP0R8V:
    case PIM5_MP2975_OUTPUT_CURRENT_XP0R8V:
    case PIM6_MP2975_OUTPUT_CURRENT_XP0R8V:
    case PIM7_MP2975_OUTPUT_CURRENT_XP0R8V:
    case PIM8_MP2975_OUTPUT_CURRENT_XP0R8V:
      sprintf(name, "PIM%d_MP2975_OUTPUT_CURR_XP0R8V",pimid);
      break;
    case PIM1_MP2975_OUTPUT_CURRENT_XP3R3V:
    case PIM2_MP2975_OUTPUT_CURRENT_XP3R3V:
    case PIM3_MP2975_OUTPUT_CURRENT_XP3R3V:
    case PIM4_MP2975_OUTPUT_CURRENT_XP3R3V:
    case PIM5_MP2975_OUTPUT_CURRENT_XP3R3V:
    case PIM6_MP2975_OUTPUT_CURRENT_XP3R3V:
    case PIM7_MP2975_OUTPUT_CURRENT_XP3R3V:
    case PIM8_MP2975_OUTPUT_CURRENT_XP3R3V:
      sprintf(name, "PIM%d_MP2975_OUTPUT_CURR_XP3R3V",pimid);
      break;
    case PIM1_MP2975_OUTPUT_VOLTAGE_XP3R3V:
    case PIM2_MP2975_OUTPUT_VOLTAGE_XP3R3V:
    case PIM3_MP2975_OUTPUT_VOLTAGE_XP3R3V:
    case PIM4_MP2975_OUTPUT_VOLTAGE_XP3R3V:
    case PIM5_MP2975_OUTPUT_VOLTAGE_XP3R3V:
    case PIM6_MP2975_OUTPUT_VOLTAGE_XP3R3V:
    case PIM7_MP2975_OUTPUT_VOLTAGE_XP3R3V:
    case PIM8_MP2975_OUTPUT_VOLTAGE_XP3R3V:
      sprintf(name, "PIM%d_MP2975_OUTPUT_VOL_XP3R3V",pimid);
      break;
    default:
      return -1;
  }
  return 0;
}

static int
get_psu_sensor_name(uint8_t sensor_num, char *name) {

  switch(sensor_num) {
    case PSU1_SENSOR_IN_VOLT:
      sprintf(name, "PSU1_IN_VOLT");
      break;
    case PSU1_SENSOR_12V_VOLT:
      sprintf(name, "PSU1_12V_VOLT");
      break;
    case PSU1_SENSOR_STBY_VOLT:
      sprintf(name, "PSU1_STBY_VOLT");
      break;
    case PSU1_SENSOR_IN_CURR:
      sprintf(name, "PSU1_IN_CURR");
      break;
    case PSU1_SENSOR_12V_CURR:
      sprintf(name, "PSU1_12V_CURR");
      break;
    case PSU1_SENSOR_STBY_CURR:
      sprintf(name, "PSU1_STBY_CURR");
      break;
    case PSU1_SENSOR_IN_POWER:
      sprintf(name, "PSU1_IN_POWER");
      break;
    case PSU1_SENSOR_12V_POWER:
      sprintf(name, "PSU1_12V_POWER");
      break;
    case PSU1_SENSOR_STBY_POWER:
      sprintf(name, "PSU1_STBY_POWER");
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
      sprintf(name, "PSU2_IN_VOLT");
      break;
    case PSU2_SENSOR_12V_VOLT:
      sprintf(name, "PSU2_12V_VOLT");
      break;
    case PSU2_SENSOR_STBY_VOLT:
      sprintf(name, "PSU2_STBY_VOLT");
      break;
    case PSU2_SENSOR_IN_CURR:
      sprintf(name, "PSU2_IN_CURR");
      break;
    case PSU2_SENSOR_12V_CURR:
      sprintf(name, "PSU2_12V_CURR");
      break;
    case PSU2_SENSOR_STBY_CURR:
      sprintf(name, "PSU2_STBY_CURR");
      break;
    case PSU2_SENSOR_IN_POWER:
      sprintf(name, "PSU2_IN_POWER");
      break;
    case PSU2_SENSOR_12V_POWER:
      sprintf(name, "PSU2_12V_POWER");
      break;
    case PSU2_SENSOR_STBY_POWER:
      sprintf(name, "PSU2_STBY_POWER");
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
    case PSU3_SENSOR_IN_VOLT:
      sprintf(name, "PSU3_IN_VOLT");
      break;
    case PSU3_SENSOR_12V_VOLT:
      sprintf(name, "PSU3_12V_VOLT");
      break;
    case PSU3_SENSOR_STBY_VOLT:
      sprintf(name, "PSU3_STBY_VOLT");
      break;
    case PSU3_SENSOR_IN_CURR:
      sprintf(name, "PSU3_IN_CURR");
      break;
    case PSU3_SENSOR_12V_CURR:
      sprintf(name, "PSU3_12V_CURR");
      break;
    case PSU3_SENSOR_STBY_CURR:
      sprintf(name, "PSU3_STBY_CURR");
      break;
    case PSU3_SENSOR_IN_POWER:
      sprintf(name, "PSU3_IN_POWER");
      break;
    case PSU3_SENSOR_12V_POWER:
      sprintf(name, "PSU3_12V_POWER");
      break;
    case PSU3_SENSOR_STBY_POWER:
      sprintf(name, "PSU3_STBY_POWER");
      break;
    case PSU3_SENSOR_FAN_TACH:
      sprintf(name, "PSU3_FAN_SPEED");
      break;
    case PSU3_SENSOR_TEMP1:
      sprintf(name, "PSU3_TEMP1");
      break;
    case PSU3_SENSOR_TEMP2:
      sprintf(name, "PSU3_TEMP2");
      break;
    case PSU3_SENSOR_TEMP3:
      sprintf(name, "PSU3_TEMP3");
      break;
    case PSU4_SENSOR_IN_VOLT:
      sprintf(name, "PSU4_IN_VOLT");
      break;
    case PSU4_SENSOR_12V_VOLT:
      sprintf(name, "PSU4_12V_VOLT");
      break;
    case PSU4_SENSOR_STBY_VOLT:
      sprintf(name, "PSU4_STBY_VOLT");
      break;
    case PSU4_SENSOR_IN_CURR:
      sprintf(name, "PSU4_IN_CURR");
      break;
    case PSU4_SENSOR_12V_CURR:
      sprintf(name, "PSU4_12V_CURR");
      break;
    case PSU4_SENSOR_STBY_CURR:
      sprintf(name, "PSU4_STBY_CURR");
      break;
    case PSU4_SENSOR_IN_POWER:
      sprintf(name, "PSU4_IN_POWER");
      break;
    case PSU4_SENSOR_12V_POWER:
      sprintf(name, "PSU4_12V_POWER");
      break;
    case PSU4_SENSOR_STBY_POWER:
      sprintf(name, "PSU4_STBY_POWER");
      break;
    case PSU4_SENSOR_FAN_TACH:
      sprintf(name, "PSU4_FAN_SPEED");
      break;
    case PSU4_SENSOR_TEMP1:
      sprintf(name, "PSU4_TEMP1");
      break;
    case PSU4_SENSOR_TEMP2:
      sprintf(name, "PSU4_TEMP2");
      break;
    case PSU4_SENSOR_TEMP3:
      sprintf(name, "PSU4_TEMP3");
      break;
    default:
      return -1;
  }
  return 0;
}

int
pal_get_sensor_name(uint8_t fru, uint8_t sensor_num, char *name) {

  int ret = -1;

  switch(fru) {
    case FRU_SCM:
      ret = get_scm_sensor_name(sensor_num, name);
      break;
    case FRU_SMB:
      ret = get_smb_sensor_name(sensor_num, name);
      break;
    case FRU_PIM1:
    case FRU_PIM2:
    case FRU_PIM3:
    case FRU_PIM4:
    case FRU_PIM5:
    case FRU_PIM6:
    case FRU_PIM7:
    case FRU_PIM8:
      ret = get_pim_sensor_name(sensor_num, fru, name);
      break;
    case FRU_PSU1:
    case FRU_PSU2:
    case FRU_PSU3:
    case FRU_PSU4:
      ret = get_psu_sensor_name(sensor_num, name);
      break;
    default:
      return -1;
  }
  return ret;
}

static int
get_scm_sensor_units(uint8_t sensor_num, char *units) {

  switch(sensor_num) {
    case SCM_SENSOR_OUTLET_U7_TEMP:
    case SCM_SENSOR_INLET_U8_TEMP:
    case SCM_SENSOR_BMC_LM75_U9_TEMP:
    case BIC_SENSOR_MB_OUTLET_TEMP:
    case BIC_SENSOR_MB_INLET_TEMP:
    case BIC_SENSOR_PCH_TEMP:
    case BIC_SENSOR_VCCIN_VR_TEMP:
    case BIC_SENSOR_1V05MIX_VR_TEMP:
    case BIC_SENSOR_SOC_TEMP:
    case BIC_SENSOR_SOC_THERM_MARGIN:
    case BIC_SENSOR_VDDR_VR_TEMP:
    case BIC_SENSOR_SOC_DIMMA0_TEMP:
    case BIC_SENSOR_SOC_DIMMB0_TEMP:
    case BIC_SENSOR_SOC_TJMAX:
      sprintf(units, "C");
      break;
    case SCM_SENSOR_HSC_VOLT:
    case SCM_SENSOR_POWER_VOLT:
    case BIC_SENSOR_P3V3_MB:
    case BIC_SENSOR_P12V_MB:
    case BIC_SENSOR_P1V05_PCH:
    case BIC_SENSOR_P3V3_STBY_MB:
    case BIC_SENSOR_P5V_STBY_MB:
    case BIC_SENSOR_PV_BAT:
    case BIC_SENSOR_PVDDR:
    case BIC_SENSOR_P1V05_MIX:
    case BIC_SENSOR_VCCIN_VR_VOL:
    case BIC_SENSOR_VDDR_VR_VOL:
    case BIC_SENSOR_P1V05MIX_VR_VOL:
    case BIC_SENSOR_INA230_VOL:
      sprintf(units, "Volts");
      break;
    case SCM_SENSOR_HSC_CURR:
    case BIC_SENSOR_1V05MIX_VR_CURR:
    case BIC_SENSOR_VDDR_VR_CURR:
    case BIC_SENSOR_VCCIN_VR_CURR:
      sprintf(units, "Amps");
      break;
    case SCM_SENSOR_HSC_POWER:
    case BIC_SENSOR_SOC_PACKAGE_PWR:
    case BIC_SENSOR_VCCIN_VR_POUT:
    case BIC_SENSOR_VDDR_VR_POUT:
    case BIC_SENSOR_P1V05MIX_VR_POUT:
    case BIC_SENSOR_INA230_POWER:
      sprintf(units, "Watts");
      break;
    default:
      return -1;
  }
  return 0;
}

static int
get_smb_sensor_units(uint8_t sensor_num, char *units) {

  switch(sensor_num) {
    case SMB_TMP422_U20_1_TEMP:
    case SMB_TMP422_U20_2_TEMP:
    case SMB_TMP422_U20_3_TEMP:
    case SIM_LM75_U1_TEMP:
    case SMB_SENSOR_TEMP1:
    case SMB_SENSOR_TEMP2:
    case SMB_SENSOR_TEMP3:
    case SMB_SENSOR_PDB_L_TEMP1:
    case SMB_SENSOR_PDB_L_TEMP2:
    case SMB_SENSOR_PDB_R_TEMP1:
    case SMB_SENSOR_PDB_R_TEMP2:
    case SMB_SENSOR_FCM_T_TEMP1:
    case SMB_SENSOR_FCM_T_TEMP2:
    case SMB_SENSOR_FCM_B_TEMP1:
    case SMB_SENSOR_FCM_B_TEMP2:
    case SMB_SENSOR_TH4_HIGH:
      sprintf(units, "C");
      break;
    case SMB_XP3R3V_BMC:
    case SMB_XP2R5V_BMC:
    case SMB_XP1R8V_BMC:
    case SMB_XP1R2V_BMC:
    case SMB_XP1R0V_FPGA:
    case SMB_XP3R3V_USB:
    case SMB_XP5R0V:
    case SMB_XP3R3V_EARLY:
    case SMB_LM57_VTEMP:
    case SMB_XP1R8:
    case SMB_XP1R2:
    case SMB_VDDC_SW:
    case SMB_XP3R3V:
    case SMB_XP1R8V_AVDD:
    case SMB_XP1R2V_TVDD:
    case SMB_XP0R75V_1_PVDD:
    case SMB_XP0R75V_2_PVDD:
    case SMB_XP0R75V_3_PVDD:
    case SMB_VDD_PCIE:
    case SMB_XP0R84V_DCSU:
    case SMB_XP0R84V_CSU:
    case SMB_XP1R84V_CSU:
    case SMB_XP3R3V_TCXO:
    case SMB_SENSOR_FCM_T_HSC_VOLT:
    case SMB_SENSOR_FCM_B_HSC_VOLT:
    case SMB_SENSOR_FCM_T_HSC_POWER_VOLT:
    case SMB_SENSOR_FCM_B_HSC_POWER_VOLT:
    case SMB_XP12R0V_VDDC_SW_IN:
    case SMB_VDDC_SW_IN_SENS:
    case SMB_OUTPUT_VOLTAGE_XP0R75V_1:
    case SMB_INPUT_VOLTAGE_1:
    case SMB_OUTPUT_VOLTAGE_XP1R2V:
    case SMB_OUTPUT_VOLTAGE_XP0R75V_2:
    case SMB_INPUT_VOLTAGE_2:
      sprintf(units, "Volts");
      break;
    case SMB_SENSOR_FCM_T_HSC_CURR:
    case SMB_SENSOR_FCM_B_HSC_CURR:
    case SMB_VDDC_SW_CURR_IN:
    case SMB_VDDC_SW_CURR_OUT:
    case SMB_OUTPUT_CURRENT_XP0R75V_1:
    case SMB_OUTPUT_CURRENT_XP1R2V:
    case SMB_OUTPUT_CURRENT_XP0R75V_2:
      sprintf(units, "Amps");
      break;
    case SMB_VDDC_SW_POWER_IN:
    case SMB_VDDC_SW_POWER_OUT:
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
    case SMB_SENSOR_FAN5_FRONT_TACH:
    case SMB_SENSOR_FAN5_REAR_TACH:
    case SMB_SENSOR_FAN6_FRONT_TACH:
    case SMB_SENSOR_FAN6_REAR_TACH:
    case SMB_SENSOR_FAN7_FRONT_TACH:
    case SMB_SENSOR_FAN7_REAR_TACH:
    case SMB_SENSOR_FAN8_FRONT_TACH:
    case SMB_SENSOR_FAN8_REAR_TACH:
      sprintf(units, "RPM");
      break;
    default:
      return -1;
  }
  return 0;
}

static int
get_pim_sensor_units(uint8_t sensor_num, char *units) {

  switch(sensor_num) {
    case PIM1_LM75_TEMP_4A:
    case PIM2_LM75_TEMP_4A:
    case PIM3_LM75_TEMP_4A:
    case PIM4_LM75_TEMP_4A:
    case PIM5_LM75_TEMP_4A:
    case PIM6_LM75_TEMP_4A:
    case PIM7_LM75_TEMP_4A:
    case PIM8_LM75_TEMP_4A:
    case PIM1_LM75_TEMP_4B:
    case PIM2_LM75_TEMP_4B:
    case PIM3_LM75_TEMP_4B:
    case PIM4_LM75_TEMP_4B:
    case PIM5_LM75_TEMP_4B:
    case PIM6_LM75_TEMP_4B:
    case PIM7_LM75_TEMP_4B:
    case PIM8_LM75_TEMP_4B:
    case PIM1_LM75_TEMP_48:
    case PIM2_LM75_TEMP_48:
    case PIM3_LM75_TEMP_48:
    case PIM4_LM75_TEMP_48:
    case PIM5_LM75_TEMP_48:
    case PIM6_LM75_TEMP_48:
    case PIM7_LM75_TEMP_48:
    case PIM8_LM75_TEMP_48:
    case PIM1_SENSOR_QSFP_TEMP:
    case PIM2_SENSOR_QSFP_TEMP:
    case PIM3_SENSOR_QSFP_TEMP:
    case PIM4_SENSOR_QSFP_TEMP:
    case PIM5_SENSOR_QSFP_TEMP:
    case PIM6_SENSOR_QSFP_TEMP:
    case PIM7_SENSOR_QSFP_TEMP:
    case PIM8_SENSOR_QSFP_TEMP:
      sprintf(units, "C");
      break;
    case PIM1_SENSOR_HSC_CURR:
    case PIM2_SENSOR_HSC_CURR:
    case PIM3_SENSOR_HSC_CURR:
    case PIM4_SENSOR_HSC_CURR:
    case PIM5_SENSOR_HSC_CURR:
    case PIM6_SENSOR_HSC_CURR:
    case PIM7_SENSOR_HSC_CURR:
    case PIM8_SENSOR_HSC_CURR:
    case PIM1_MP2975_OUTPUT_CURRENT_XP3R3V:
    case PIM2_MP2975_OUTPUT_CURRENT_XP3R3V:
    case PIM3_MP2975_OUTPUT_CURRENT_XP3R3V:
    case PIM4_MP2975_OUTPUT_CURRENT_XP3R3V:
    case PIM5_MP2975_OUTPUT_CURRENT_XP3R3V:
    case PIM6_MP2975_OUTPUT_CURRENT_XP3R3V:
    case PIM7_MP2975_OUTPUT_CURRENT_XP3R3V:
    case PIM8_MP2975_OUTPUT_CURRENT_XP3R3V:
    case PIM1_MP2975_OUTPUT_CURRENT_XP0R8V:
    case PIM2_MP2975_OUTPUT_CURRENT_XP0R8V:
    case PIM3_MP2975_OUTPUT_CURRENT_XP0R8V:
    case PIM4_MP2975_OUTPUT_CURRENT_XP0R8V:
    case PIM5_MP2975_OUTPUT_CURRENT_XP0R8V:
    case PIM6_MP2975_OUTPUT_CURRENT_XP0R8V:
    case PIM7_MP2975_OUTPUT_CURRENT_XP0R8V:
    case PIM8_MP2975_OUTPUT_CURRENT_XP0R8V:
      sprintf(units, "Amps");
      break;
    case PIM1_SENSOR_HSC_POWER:
    case PIM2_SENSOR_HSC_POWER:
    case PIM3_SENSOR_HSC_POWER:
    case PIM4_SENSOR_HSC_POWER:
    case PIM5_SENSOR_HSC_POWER:
    case PIM6_SENSOR_HSC_POWER:
    case PIM7_SENSOR_HSC_POWER:
    case PIM8_SENSOR_HSC_POWER:
      sprintf(units, "Watts");
      break;
    case PIM1_SENSOR_HSC_VOLT:
    case PIM2_SENSOR_HSC_VOLT:
    case PIM3_SENSOR_HSC_VOLT:
    case PIM4_SENSOR_HSC_VOLT:
    case PIM5_SENSOR_HSC_VOLT:
    case PIM6_SENSOR_HSC_VOLT:
    case PIM7_SENSOR_HSC_VOLT:
    case PIM8_SENSOR_HSC_VOLT:
    case PIM1_POWER_VOLTAGE:
    case PIM2_POWER_VOLTAGE:
    case PIM3_POWER_VOLTAGE:
    case PIM4_POWER_VOLTAGE:
    case PIM5_POWER_VOLTAGE:
    case PIM6_POWER_VOLTAGE:
    case PIM7_POWER_VOLTAGE:
    case PIM8_POWER_VOLTAGE:
    case PIM1_UCD90160_VOLT1:
    case PIM1_UCD90160_VOLT2:
    case PIM1_UCD90160_VOLT3:
    case PIM1_UCD90160_VOLT4:
    case PIM1_UCD90160_VOLT5:
    case PIM1_UCD90160_VOLT6:
    case PIM1_UCD90160_VOLT7:
    case PIM1_UCD90160_VOLT8:
    case PIM1_UCD90160_VOLT9:
    case PIM1_UCD90160_VOLT10:
    case PIM1_UCD90160_VOLT11:
    case PIM1_UCD90160_VOLT12:
    case PIM1_UCD90160_VOLT13:
    case PIM2_UCD90160_VOLT1:
    case PIM2_UCD90160_VOLT2:
    case PIM2_UCD90160_VOLT3:
    case PIM2_UCD90160_VOLT4:
    case PIM2_UCD90160_VOLT5:
    case PIM2_UCD90160_VOLT6:
    case PIM2_UCD90160_VOLT7:
    case PIM2_UCD90160_VOLT8:
    case PIM2_UCD90160_VOLT9:
    case PIM2_UCD90160_VOLT10:
    case PIM2_UCD90160_VOLT11:
    case PIM2_UCD90160_VOLT12:
    case PIM2_UCD90160_VOLT13:
    case PIM3_UCD90160_VOLT1:
    case PIM3_UCD90160_VOLT2:
    case PIM3_UCD90160_VOLT3:
    case PIM3_UCD90160_VOLT4:
    case PIM3_UCD90160_VOLT5:
    case PIM3_UCD90160_VOLT6:
    case PIM3_UCD90160_VOLT7:
    case PIM3_UCD90160_VOLT8:
    case PIM3_UCD90160_VOLT9:
    case PIM3_UCD90160_VOLT10:
    case PIM3_UCD90160_VOLT11:
    case PIM3_UCD90160_VOLT12:
    case PIM3_UCD90160_VOLT13:
    case PIM4_UCD90160_VOLT1:
    case PIM4_UCD90160_VOLT2:
    case PIM4_UCD90160_VOLT3:
    case PIM4_UCD90160_VOLT4:
    case PIM4_UCD90160_VOLT5:
    case PIM4_UCD90160_VOLT6:
    case PIM4_UCD90160_VOLT7:
    case PIM4_UCD90160_VOLT8:
    case PIM4_UCD90160_VOLT9:
    case PIM4_UCD90160_VOLT10:
    case PIM4_UCD90160_VOLT11:
    case PIM4_UCD90160_VOLT12:
    case PIM4_UCD90160_VOLT13:
    case PIM5_UCD90160_VOLT1:
    case PIM5_UCD90160_VOLT2:
    case PIM5_UCD90160_VOLT3:
    case PIM5_UCD90160_VOLT4:
    case PIM5_UCD90160_VOLT5:
    case PIM5_UCD90160_VOLT6:
    case PIM5_UCD90160_VOLT7:
    case PIM5_UCD90160_VOLT8:
    case PIM5_UCD90160_VOLT9:
    case PIM5_UCD90160_VOLT10:
    case PIM5_UCD90160_VOLT11:
    case PIM5_UCD90160_VOLT12:
    case PIM5_UCD90160_VOLT13:
    case PIM6_UCD90160_VOLT1:
    case PIM6_UCD90160_VOLT2:
    case PIM6_UCD90160_VOLT3:
    case PIM6_UCD90160_VOLT4:
    case PIM6_UCD90160_VOLT5:
    case PIM6_UCD90160_VOLT6:
    case PIM6_UCD90160_VOLT7:
    case PIM6_UCD90160_VOLT8:
    case PIM6_UCD90160_VOLT9:
    case PIM6_UCD90160_VOLT10:
    case PIM6_UCD90160_VOLT11:
    case PIM6_UCD90160_VOLT12:
    case PIM6_UCD90160_VOLT13:
    case PIM7_UCD90160_VOLT1:
    case PIM7_UCD90160_VOLT2:
    case PIM7_UCD90160_VOLT3:
    case PIM7_UCD90160_VOLT4:
    case PIM7_UCD90160_VOLT5:
    case PIM7_UCD90160_VOLT6:
    case PIM7_UCD90160_VOLT7:
    case PIM7_UCD90160_VOLT8:
    case PIM7_UCD90160_VOLT9:
    case PIM7_UCD90160_VOLT10:
    case PIM7_UCD90160_VOLT11:
    case PIM7_UCD90160_VOLT12:
    case PIM7_UCD90160_VOLT13:
    case PIM8_UCD90160_VOLT1:
    case PIM8_UCD90160_VOLT2:
    case PIM8_UCD90160_VOLT3:
    case PIM8_UCD90160_VOLT4:
    case PIM8_UCD90160_VOLT5:
    case PIM8_UCD90160_VOLT6:
    case PIM8_UCD90160_VOLT7:
    case PIM8_UCD90160_VOLT8:
    case PIM8_UCD90160_VOLT9:
    case PIM8_UCD90160_VOLT10:
    case PIM8_UCD90160_VOLT11:
    case PIM8_UCD90160_VOLT12:
    case PIM8_UCD90160_VOLT13:
    case PIM1_MP2975_INPUT_VOLTAGE:
    case PIM2_MP2975_INPUT_VOLTAGE:
    case PIM3_MP2975_INPUT_VOLTAGE:
    case PIM4_MP2975_INPUT_VOLTAGE:
    case PIM5_MP2975_INPUT_VOLTAGE:
    case PIM6_MP2975_INPUT_VOLTAGE:
    case PIM7_MP2975_INPUT_VOLTAGE:
    case PIM8_MP2975_INPUT_VOLTAGE:
    case PIM1_MP2975_OUTPUT_VOLTAGE_XP0R8V:
    case PIM2_MP2975_OUTPUT_VOLTAGE_XP0R8V:
    case PIM3_MP2975_OUTPUT_VOLTAGE_XP0R8V:
    case PIM4_MP2975_OUTPUT_VOLTAGE_XP0R8V:
    case PIM5_MP2975_OUTPUT_VOLTAGE_XP0R8V:
    case PIM6_MP2975_OUTPUT_VOLTAGE_XP0R8V:
    case PIM7_MP2975_OUTPUT_VOLTAGE_XP0R8V:
    case PIM8_MP2975_OUTPUT_VOLTAGE_XP0R8V:
    case PIM1_MP2975_OUTPUT_VOLTAGE_XP3R3V:
    case PIM2_MP2975_OUTPUT_VOLTAGE_XP3R3V:
    case PIM3_MP2975_OUTPUT_VOLTAGE_XP3R3V:
    case PIM4_MP2975_OUTPUT_VOLTAGE_XP3R3V:
    case PIM5_MP2975_OUTPUT_VOLTAGE_XP3R3V:
    case PIM6_MP2975_OUTPUT_VOLTAGE_XP3R3V:
    case PIM7_MP2975_OUTPUT_VOLTAGE_XP3R3V:
    case PIM8_MP2975_OUTPUT_VOLTAGE_XP3R3V:
      sprintf(units, "Volts");
      break;
    default:
      return -1;
  }
  return 0;
}

static int
get_psu_sensor_units(uint8_t sensor_num, char *units) {

  switch(sensor_num) {
    case PSU1_SENSOR_IN_VOLT:
    case PSU1_SENSOR_12V_VOLT:
    case PSU1_SENSOR_STBY_VOLT:
    case PSU2_SENSOR_IN_VOLT:
    case PSU2_SENSOR_12V_VOLT:
    case PSU2_SENSOR_STBY_VOLT:
    case PSU3_SENSOR_IN_VOLT:
    case PSU3_SENSOR_12V_VOLT:
    case PSU3_SENSOR_STBY_VOLT:
    case PSU4_SENSOR_IN_VOLT:
    case PSU4_SENSOR_12V_VOLT:
    case PSU4_SENSOR_STBY_VOLT:
      sprintf(units, "Volts");
      break;
    case PSU1_SENSOR_IN_CURR:
    case PSU1_SENSOR_12V_CURR:
    case PSU1_SENSOR_STBY_CURR:
    case PSU2_SENSOR_IN_CURR:
    case PSU2_SENSOR_12V_CURR:
    case PSU2_SENSOR_STBY_CURR:
    case PSU3_SENSOR_IN_CURR:
    case PSU3_SENSOR_12V_CURR:
    case PSU3_SENSOR_STBY_CURR:
    case PSU4_SENSOR_IN_CURR:
    case PSU4_SENSOR_12V_CURR:
    case PSU4_SENSOR_STBY_CURR:
      sprintf(units, "Amps");
      break;
    case PSU1_SENSOR_IN_POWER:
    case PSU1_SENSOR_12V_POWER:
    case PSU1_SENSOR_STBY_POWER:
    case PSU2_SENSOR_IN_POWER:
    case PSU2_SENSOR_12V_POWER:
    case PSU2_SENSOR_STBY_POWER:
    case PSU3_SENSOR_IN_POWER:
    case PSU3_SENSOR_12V_POWER:
    case PSU3_SENSOR_STBY_POWER:
    case PSU4_SENSOR_IN_POWER:
    case PSU4_SENSOR_12V_POWER:
    case PSU4_SENSOR_STBY_POWER:
      sprintf(units, "Watts");
      break;
    case PSU1_SENSOR_FAN_TACH:
    case PSU2_SENSOR_FAN_TACH:
    case PSU3_SENSOR_FAN_TACH:
    case PSU4_SENSOR_FAN_TACH:
      sprintf(units, "RPM");
      break;
    case PSU1_SENSOR_TEMP1:
    case PSU1_SENSOR_TEMP2:
    case PSU1_SENSOR_TEMP3:
    case PSU2_SENSOR_TEMP1:
    case PSU2_SENSOR_TEMP2:
    case PSU2_SENSOR_TEMP3:
    case PSU3_SENSOR_TEMP1:
    case PSU3_SENSOR_TEMP2:
    case PSU3_SENSOR_TEMP3:
    case PSU4_SENSOR_TEMP1:
    case PSU4_SENSOR_TEMP2:
    case PSU4_SENSOR_TEMP3:
      sprintf(units, "C");
      break;
     default:
      return -1;
  }
  return 0;
}

int
pal_get_sensor_units(uint8_t fru, uint8_t sensor_num, char *units) {
  int ret = -1;

  switch(fru) {
    case FRU_SCM:
      ret = get_scm_sensor_units(sensor_num, units);
      break;
    case FRU_SMB:
      ret = get_smb_sensor_units(sensor_num, units);
      break;
    case FRU_PIM1:
    case FRU_PIM2:
    case FRU_PIM3:
    case FRU_PIM4:
    case FRU_PIM5:
    case FRU_PIM6:
    case FRU_PIM7:
    case FRU_PIM8:
      ret = get_pim_sensor_units(sensor_num, units);
      break;
    case FRU_PSU1:
    case FRU_PSU2:
    case FRU_PSU3:
    case FRU_PSU4:
      ret = get_psu_sensor_units(sensor_num, units);
      break;
    default:
      return -1;
  }
  return ret;
}

static void
sensor_thresh_array_init(uint8_t fru) {
  int i = 0, j;
  float fvalue;

  if (init_threshold_done[fru])
    return;

  switch (fru) {
    case FRU_SCM:
      scm_sensor_threshold[SCM_SENSOR_OUTLET_U7_TEMP][UCR_THRESH] = 60;
      scm_sensor_threshold[SCM_SENSOR_BMC_LM75_U9_TEMP][UCR_THRESH] = 80;
      scm_sensor_threshold[SCM_SENSOR_INLET_U8_TEMP][UCR_THRESH] = 50;
      scm_sensor_threshold[SCM_SENSOR_HSC_VOLT][LCR_THRESH] = 7.5;
      scm_sensor_threshold[SCM_SENSOR_POWER_VOLT][LCR_THRESH] = 7.5;
      scm_sensor_threshold[SCM_SENSOR_HSC_CURR][LCR_THRESH] = 0;
      scm_sensor_threshold[SCM_SENSOR_HSC_POWER][LCR_THRESH] = 0;
      for (i = scm_sensor_cnt; i < scm_all_sensor_cnt; i++) {
        for (j = 1; j <= MAX_SENSOR_THRESHOLD; j++) {
          if (!bic_get_sdr_thresh_val(fru ,scm_all_sensor_list[i], j, &fvalue)){
            scm_sensor_threshold[scm_all_sensor_list[i]][j] = fvalue;
          } else {
            /* Error case, if get BIC data retry more than 30 times(30s),
             * it means BIC get wrong, skip init BIC threshold value */
            goto scm_thresh_done;
          }
        }
      }
scm_thresh_done:
      kv_set(SCM_INIT_THRESH_STATUS, "done", 0, 0);
      break;
    case FRU_SMB:
      smb_sensor_threshold[SMB_XP3R3V_BMC][UCR_THRESH] = 3.8;
      smb_sensor_threshold[SMB_XP3R3V_BMC][UNC_THRESH] = 3.465;
      smb_sensor_threshold[SMB_XP3R3V_BMC][LNC_THRESH] = 3.165;
      smb_sensor_threshold[SMB_XP3R3V_BMC][LCR_THRESH] = 2.8;
      smb_sensor_threshold[SMB_XP2R5V_BMC][UCR_THRESH] = 2.875;
      smb_sensor_threshold[SMB_XP2R5V_BMC][UNC_THRESH] = 2.625;
      smb_sensor_threshold[SMB_XP2R5V_BMC][LNC_THRESH] = 2.375;
      smb_sensor_threshold[SMB_XP2R5V_BMC][LCR_THRESH] = 2.125;
      smb_sensor_threshold[SMB_XP1R8V_BMC][UCR_THRESH] = 2.07;
      smb_sensor_threshold[SMB_XP1R8V_BMC][UNC_THRESH] = 1.89;
      smb_sensor_threshold[SMB_XP1R8V_BMC][LNC_THRESH] = 1.71;
      smb_sensor_threshold[SMB_XP1R8V_BMC][LCR_THRESH] = 1.53;
      smb_sensor_threshold[SMB_XP1R2V_BMC][UCR_THRESH] = 1.38;
      smb_sensor_threshold[SMB_XP1R2V_BMC][UNC_THRESH] = 1.26;
      smb_sensor_threshold[SMB_XP1R2V_BMC][LNC_THRESH] = 1.14;
      smb_sensor_threshold[SMB_XP1R2V_BMC][LCR_THRESH] = 1.02;
      smb_sensor_threshold[SMB_XP1R0V_FPGA][UCR_THRESH] = 1.15;
      smb_sensor_threshold[SMB_XP1R0V_FPGA][UNC_THRESH] = 1.03;
      smb_sensor_threshold[SMB_XP1R0V_FPGA][LNC_THRESH] = 0.97;
      smb_sensor_threshold[SMB_XP1R0V_FPGA][LCR_THRESH] = 0.85;
      smb_sensor_threshold[SMB_XP3R3V_USB][UCR_THRESH] = 3.8;
      smb_sensor_threshold[SMB_XP3R3V_USB][UNC_THRESH] = 3.465;
      smb_sensor_threshold[SMB_XP3R3V_USB][LNC_THRESH] = 3.165;
      smb_sensor_threshold[SMB_XP3R3V_USB][LCR_THRESH] = 2.8;
      smb_sensor_threshold[SMB_XP5R0V][UCR_THRESH] = 5.75;
      smb_sensor_threshold[SMB_XP5R0V][UNC_THRESH] = 5.25;
      smb_sensor_threshold[SMB_XP5R0V][LNC_THRESH] = 4.75;
      smb_sensor_threshold[SMB_XP5R0V][LCR_THRESH] = 4.25;
      smb_sensor_threshold[SMB_XP3R3V_EARLY][UCR_THRESH] = 3.8;
      smb_sensor_threshold[SMB_XP3R3V_EARLY][UNC_THRESH] = 3.465;
      smb_sensor_threshold[SMB_XP3R3V_EARLY][LNC_THRESH] = 3.165;
      smb_sensor_threshold[SMB_XP3R3V_EARLY][LCR_THRESH] = 2.8;
      smb_sensor_threshold[SMB_LM57_VTEMP][LNC_THRESH] = 1.038;
      smb_sensor_threshold[SMB_LM57_VTEMP][LCR_THRESH] = 0.99;
      smb_sensor_threshold[SMB_XP1R8][UCR_THRESH] = 2.07;
      smb_sensor_threshold[SMB_XP1R8][UNC_THRESH] = 1.854;
      smb_sensor_threshold[SMB_XP1R8][LNC_THRESH] = 1.746;
      smb_sensor_threshold[SMB_XP1R8][LCR_THRESH] = 1.53;
      smb_sensor_threshold[SMB_XP1R2][UCR_THRESH] = 1.38;
      smb_sensor_threshold[SMB_XP1R2][UNC_THRESH] = 1.23;
      smb_sensor_threshold[SMB_XP1R2][LNC_THRESH] = 1.17;
      smb_sensor_threshold[SMB_XP1R2][LCR_THRESH] = 1.02;
      smb_sensor_threshold[SMB_VDDC_SW][UCR_THRESH] = 1;
      smb_sensor_threshold[SMB_VDDC_SW][UNC_THRESH] = 0.98;
      smb_sensor_threshold[SMB_VDDC_SW][LNC_THRESH] = 0.72;
      smb_sensor_threshold[SMB_VDDC_SW][LCR_THRESH] = 0.69;
      smb_sensor_threshold[SMB_XP3R3V][UCR_THRESH] = 3.8;
      smb_sensor_threshold[SMB_XP3R3V][UNC_THRESH] = 3.465;
      smb_sensor_threshold[SMB_XP3R3V][LNC_THRESH] = 3.165;
      smb_sensor_threshold[SMB_XP3R3V][LCR_THRESH] = 2.8;
      smb_sensor_threshold[SMB_XP1R8V_AVDD][UCR_THRESH] = 2.07;
      smb_sensor_threshold[SMB_XP1R8V_AVDD][UNC_THRESH] = 1.854;
      smb_sensor_threshold[SMB_XP1R8V_AVDD][LNC_THRESH] = 1.746;
      smb_sensor_threshold[SMB_XP1R8V_AVDD][LCR_THRESH] = 1.53;
      smb_sensor_threshold[SMB_XP1R2V_TVDD][UCR_THRESH] = 1.38;
      smb_sensor_threshold[SMB_XP1R2V_TVDD][UNC_THRESH] = 1.236;
      smb_sensor_threshold[SMB_XP1R2V_TVDD][LNC_THRESH] = 1.164;
      smb_sensor_threshold[SMB_XP1R2V_TVDD][LCR_THRESH] = 1.02;
      smb_sensor_threshold[SMB_XP0R75V_1_PVDD][UCR_THRESH] = 0.862;
      smb_sensor_threshold[SMB_XP0R75V_1_PVDD][UNC_THRESH] = 0.81;
      smb_sensor_threshold[SMB_XP0R75V_1_PVDD][LNC_THRESH] = 0.728;
      smb_sensor_threshold[SMB_XP0R75V_1_PVDD][LCR_THRESH] = 0.638;
      smb_sensor_threshold[SMB_XP0R75V_2_PVDD][UCR_THRESH] = 0.862;
      smb_sensor_threshold[SMB_XP0R75V_2_PVDD][UNC_THRESH] = 0.81;
      smb_sensor_threshold[SMB_XP0R75V_2_PVDD][LNC_THRESH] = 0.728;
      smb_sensor_threshold[SMB_XP0R75V_2_PVDD][LCR_THRESH] = 0.638;
      smb_sensor_threshold[SMB_XP0R75V_3_PVDD][UCR_THRESH] = 0.862;
      smb_sensor_threshold[SMB_XP0R75V_3_PVDD][UNC_THRESH] = 0.81;
      smb_sensor_threshold[SMB_XP0R75V_3_PVDD][LNC_THRESH] = 0.728;
      smb_sensor_threshold[SMB_XP0R75V_3_PVDD][LCR_THRESH] = 0.638;
      smb_sensor_threshold[SMB_VDD_PCIE][UCR_THRESH] = 0.966;
      smb_sensor_threshold[SMB_VDD_PCIE][UNC_THRESH] = 0.924;
      smb_sensor_threshold[SMB_VDD_PCIE][LNC_THRESH] = 0.819;
      smb_sensor_threshold[SMB_VDD_PCIE][LCR_THRESH] = 0.714;
      smb_sensor_threshold[SMB_XP0R84V_DCSU][UCR_THRESH] = 0.966;
      smb_sensor_threshold[SMB_XP0R84V_DCSU][UNC_THRESH] = 0.924;
      smb_sensor_threshold[SMB_XP0R84V_DCSU][LNC_THRESH] = 0.819;
      smb_sensor_threshold[SMB_XP0R84V_DCSU][LCR_THRESH] = 0.714;
      smb_sensor_threshold[SMB_XP0R84V_CSU][UCR_THRESH] = 0.966;
      smb_sensor_threshold[SMB_XP0R84V_CSU][UNC_THRESH] = 0.861;
      smb_sensor_threshold[SMB_XP0R84V_CSU][LNC_THRESH] = 0.819;
      smb_sensor_threshold[SMB_XP0R84V_CSU][LCR_THRESH] = 0.714;
      smb_sensor_threshold[SMB_XP1R84V_CSU][UCR_THRESH] = 2.07;
      smb_sensor_threshold[SMB_XP1R84V_CSU][UNC_THRESH] = 1.845;
      smb_sensor_threshold[SMB_XP1R84V_CSU][LNC_THRESH] = 1.755;
      smb_sensor_threshold[SMB_XP1R84V_CSU][LCR_THRESH] = 1.53;
      smb_sensor_threshold[SMB_XP3R3V_TCXO][UCR_THRESH] = 3.8;
      smb_sensor_threshold[SMB_XP3R3V_TCXO][UNC_THRESH] = 3.465;
      smb_sensor_threshold[SMB_XP3R3V_TCXO][LNC_THRESH] = 3.165;
      smb_sensor_threshold[SMB_XP3R3V_TCXO][LCR_THRESH] = 2.8;
      smb_sensor_threshold[SMB_OUTPUT_VOLTAGE_XP0R75V_1][UCR_THRESH] = 0.862;
      smb_sensor_threshold[SMB_OUTPUT_VOLTAGE_XP0R75V_1][UNC_THRESH] = 0.773;
      smb_sensor_threshold[SMB_OUTPUT_VOLTAGE_XP0R75V_1][LNC_THRESH] = 0.728;
      smb_sensor_threshold[SMB_OUTPUT_VOLTAGE_XP0R75V_1][LCR_THRESH] = 0.638;
      smb_sensor_threshold[SMB_SENSOR_FCM_T_HSC_VOLT][LCR_THRESH] = 7.5;
      smb_sensor_threshold[SMB_SENSOR_FCM_B_HSC_VOLT][LCR_THRESH] = 7.5;
      smb_sensor_threshold[SMB_SENSOR_FCM_T_HSC_POWER_VOLT][LCR_THRESH] = 7.5;
      smb_sensor_threshold[SMB_SENSOR_FCM_B_HSC_POWER_VOLT][LCR_THRESH] = 7.5;
      smb_sensor_threshold[SMB_TMP422_U20_1_TEMP][UCR_THRESH] = 80;
      smb_sensor_threshold[SIM_LM75_U1_TEMP][UCR_THRESH] = 50;
      smb_sensor_threshold[SMB_SENSOR_TEMP1][UCR_THRESH] = 85;
      smb_sensor_threshold[SMB_SENSOR_TEMP2][UCR_THRESH] = 80;
      smb_sensor_threshold[SMB_SENSOR_TEMP3][UCR_THRESH] = 85;
      smb_sensor_threshold[SMB_SENSOR_PDB_L_TEMP1][UCR_THRESH] = 55;
      smb_sensor_threshold[SMB_SENSOR_PDB_L_TEMP2][UCR_THRESH] = 55;
      smb_sensor_threshold[SMB_SENSOR_PDB_R_TEMP1][UCR_THRESH] = 55;
      smb_sensor_threshold[SMB_SENSOR_PDB_R_TEMP2][UCR_THRESH] = 55;
      smb_sensor_threshold[SMB_SENSOR_FCM_T_TEMP1][UCR_THRESH] = 60;
      smb_sensor_threshold[SMB_SENSOR_FCM_T_TEMP2][UCR_THRESH] = 60;
      smb_sensor_threshold[SMB_SENSOR_FCM_B_TEMP1][UCR_THRESH] = 60;
      smb_sensor_threshold[SMB_SENSOR_FCM_B_TEMP2][UCR_THRESH] = 60;
      smb_sensor_threshold[SMB_SENSOR_FAN1_FRONT_TACH][UCR_THRESH] = 12000;
      smb_sensor_threshold[SMB_SENSOR_FAN1_FRONT_TACH][LCR_THRESH] = 800;
      smb_sensor_threshold[SMB_SENSOR_FAN1_REAR_TACH][UCR_THRESH] = 12000;
      smb_sensor_threshold[SMB_SENSOR_FAN1_REAR_TACH][LCR_THRESH] = 800;
      smb_sensor_threshold[SMB_SENSOR_FAN2_FRONT_TACH][UCR_THRESH] = 12000;
      smb_sensor_threshold[SMB_SENSOR_FAN2_FRONT_TACH][LCR_THRESH] = 800;
      smb_sensor_threshold[SMB_SENSOR_FAN2_REAR_TACH][UCR_THRESH] = 12000;
      smb_sensor_threshold[SMB_SENSOR_FAN2_REAR_TACH][LCR_THRESH] = 800;
      smb_sensor_threshold[SMB_SENSOR_FAN3_FRONT_TACH][UCR_THRESH] = 12000;
      smb_sensor_threshold[SMB_SENSOR_FAN3_FRONT_TACH][LCR_THRESH] = 800;
      smb_sensor_threshold[SMB_SENSOR_FAN3_REAR_TACH][UCR_THRESH] = 12000;
      smb_sensor_threshold[SMB_SENSOR_FAN3_REAR_TACH][LCR_THRESH] = 800;
      smb_sensor_threshold[SMB_SENSOR_FAN4_FRONT_TACH][UCR_THRESH] = 12000;
      smb_sensor_threshold[SMB_SENSOR_FAN4_FRONT_TACH][LCR_THRESH] = 800;
      smb_sensor_threshold[SMB_SENSOR_FAN4_REAR_TACH][UCR_THRESH] = 12000;
      smb_sensor_threshold[SMB_SENSOR_FAN4_REAR_TACH][LCR_THRESH] = 800;
      smb_sensor_threshold[SMB_SENSOR_FAN5_FRONT_TACH][UCR_THRESH] = 12000;
      smb_sensor_threshold[SMB_SENSOR_FAN5_FRONT_TACH][LCR_THRESH] = 800;
      smb_sensor_threshold[SMB_SENSOR_FAN5_REAR_TACH][UCR_THRESH] = 12000;
      smb_sensor_threshold[SMB_SENSOR_FAN5_REAR_TACH][LCR_THRESH] = 800;
      smb_sensor_threshold[SMB_SENSOR_FAN6_FRONT_TACH][UCR_THRESH] = 12000;
      smb_sensor_threshold[SMB_SENSOR_FAN6_FRONT_TACH][LCR_THRESH] = 800;
      smb_sensor_threshold[SMB_SENSOR_FAN6_REAR_TACH][UCR_THRESH] = 12000;
      smb_sensor_threshold[SMB_SENSOR_FAN6_REAR_TACH][LCR_THRESH] = 800;
      smb_sensor_threshold[SMB_SENSOR_FAN7_FRONT_TACH][UCR_THRESH] = 12000;
      smb_sensor_threshold[SMB_SENSOR_FAN7_FRONT_TACH][LCR_THRESH] = 800;
      smb_sensor_threshold[SMB_SENSOR_FAN7_REAR_TACH][UCR_THRESH] = 12000;
      smb_sensor_threshold[SMB_SENSOR_FAN7_REAR_TACH][LCR_THRESH] = 800;
      smb_sensor_threshold[SMB_SENSOR_FAN8_FRONT_TACH][UCR_THRESH] = 12000;
      smb_sensor_threshold[SMB_SENSOR_FAN8_FRONT_TACH][LCR_THRESH] = 800;
      smb_sensor_threshold[SMB_SENSOR_FAN8_REAR_TACH][UCR_THRESH] = 12000;
      smb_sensor_threshold[SMB_SENSOR_FAN8_REAR_TACH][LCR_THRESH] = 800;
      break;
    case FRU_PSU1:
    case FRU_PSU2:
    case FRU_PSU3:
    case FRU_PSU4:
      i = fru - 11;
      psu_sensor_threshold[PSU1_SENSOR_IN_VOLT+(i*0x0d)][UCR_THRESH] = 310;
      psu_sensor_threshold[PSU1_SENSOR_IN_VOLT+(i*0x0d)][LCR_THRESH] = 92;
      psu_sensor_threshold[PSU1_SENSOR_12V_VOLT+(i*0x0d)][UCR_THRESH] = 13;
      psu_sensor_threshold[PSU1_SENSOR_12V_VOLT+(i*0x0d)][LCR_THRESH] = 11;
      psu_sensor_threshold[PSU1_SENSOR_STBY_VOLT+(i*0x0d)][UCR_THRESH] = 3.6;
      psu_sensor_threshold[PSU1_SENSOR_STBY_VOLT+(i*0x0d)][LCR_THRESH] = 3.0;
      psu_sensor_threshold[PSU1_SENSOR_IN_CURR+(i*0x0d)][UCR_THRESH] = 10;
      psu_sensor_threshold[PSU1_SENSOR_12V_CURR+(i*0x0d)][UCR_THRESH] = 125;
      psu_sensor_threshold[PSU1_SENSOR_STBY_CURR+(i*0x0d)][UCR_THRESH] = 5;
      psu_sensor_threshold[PSU1_SENSOR_IN_POWER+(i*0x0d)][UCR_THRESH] = 1500;
      psu_sensor_threshold[PSU1_SENSOR_12V_POWER+(i*0x0d)][UCR_THRESH] = 1500;
      psu_sensor_threshold[PSU1_SENSOR_STBY_POWER+(i*0x0d)][UCR_THRESH] = 15;
      psu_sensor_threshold[PSU1_SENSOR_FAN_TACH+(i*0x0d)][LCR_THRESH] = 500;
      psu_sensor_threshold[PSU1_SENSOR_TEMP1+(i*0x0d)][UCR_THRESH] = 60;
      psu_sensor_threshold[PSU1_SENSOR_TEMP2+(i*0x0d)][UCR_THRESH] = 80;
      psu_sensor_threshold[PSU1_SENSOR_TEMP3+(i*0x0d)][UCR_THRESH] = 95;
      break;
    case FRU_PIM1:
    case FRU_PIM2:
    case FRU_PIM3:
    case FRU_PIM4:
    case FRU_PIM5:
    case FRU_PIM6:
    case FRU_PIM7:
    case FRU_PIM8:
      pim_thresh_array_init(fru);
      break;
  }
  init_threshold_done[fru] = true;
}

int
pal_get_sensor_threshold(uint8_t fru, uint8_t sensor_num,
                                      uint8_t thresh, void *value) {
  float *val = (float*) value;

  sensor_thresh_array_init(fru);

  switch(fru) {
  case FRU_SCM:
    *val = scm_sensor_threshold[sensor_num][thresh];
    break;
  case FRU_SMB:
    *val = smb_sensor_threshold[sensor_num][thresh];
    break;
  case FRU_PIM1:
  case FRU_PIM2:
  case FRU_PIM3:
  case FRU_PIM4:
  case FRU_PIM5:
  case FRU_PIM6:
  case FRU_PIM7:
  case FRU_PIM8:
    *val = pim_sensor_threshold[sensor_num][thresh];
    break;
  case FRU_PSU1:
  case FRU_PSU2:
  case FRU_PSU3:
  case FRU_PSU4:
    *val = psu_sensor_threshold[sensor_num][thresh];
    break;
  default:
    return -1;
  }
  return 0;
}

static sensor_desc_t *
get_sensor_desc(uint8_t fru, uint8_t snr_num) {

  if (fru < 1 || fru > MAX_NUM_FRUS) {
    syslog(LOG_WARNING, "get_sensor_desc: Wrong FRU ID %d\n", fru);
    return NULL;
  }

  return &m_snr_desc[fru-1][snr_num];
}

void
pal_sensor_assert_handle(uint8_t fru, uint8_t snr_num,
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
      syslog(LOG_WARNING, "pal_sensor_assert_handle: wrong thresh enum value");
      exit(-1);
  }

  switch (fru) {
    case FRU_PIM1:
    case FRU_PIM2:
    case FRU_PIM3:
    case FRU_PIM4:
    case FRU_PIM5:
    case FRU_PIM6:
    case FRU_PIM7:
    case FRU_PIM8:
      switch(snr_num) {
        case PIM1_SENSOR_QSFP_TEMP:
        case PIM2_SENSOR_QSFP_TEMP:
        case PIM3_SENSOR_QSFP_TEMP:
        case PIM4_SENSOR_QSFP_TEMP:
        case PIM5_SENSOR_QSFP_TEMP:
        case PIM6_SENSOR_QSFP_TEMP:
        case PIM7_SENSOR_QSFP_TEMP:
        case PIM8_SENSOR_QSFP_TEMP:
          snr_desc = get_sensor_desc(fru, snr_num);
          snprintf(crisel, sizeof(crisel), "%s %s %.2fV - ASSERT,FRU:%u",
                          snr_desc->name, thresh_name, val, fru);
          break;
        default:
          return;
      }
      break;
    default:
      return;
  }
  pal_add_cri_sel(crisel);
  return;
}

void
pal_sensor_deassert_handle(uint8_t fru, uint8_t snr_num,
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
      syslog(LOG_WARNING,
             "pal_sensor_deassert_handle: wrong thresh enum value");
      return;
  }
  switch (fru) {
    case FRU_PIM1:
    case FRU_PIM2:
    case FRU_PIM3:
    case FRU_PIM4:
    case FRU_PIM5:
    case FRU_PIM6:
    case FRU_PIM7:
    case FRU_PIM8:
      switch(snr_num) {
        case PIM1_SENSOR_QSFP_TEMP:
        case PIM2_SENSOR_QSFP_TEMP:
        case PIM3_SENSOR_QSFP_TEMP:
        case PIM4_SENSOR_QSFP_TEMP:
        case PIM5_SENSOR_QSFP_TEMP:
        case PIM6_SENSOR_QSFP_TEMP:
        case PIM7_SENSOR_QSFP_TEMP:
        case PIM8_SENSOR_QSFP_TEMP:
          snr_desc = get_sensor_desc(fru, snr_num);
          snprintf(crisel, sizeof(crisel), "%s %s %.2fV - ASSERT,FRU:%u",
                          snr_desc->name, thresh_name, val, fru);
          break;
        default:
          return;
      }
      break;
    default:
      return;
  }
  pal_add_cri_sel(crisel);
  return;
}

int
pal_sensor_threshold_flag(uint8_t fru, uint8_t snr_num, uint16_t *flag) {

  switch(fru) {
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

static void
scm_sensor_poll_interval(uint8_t sensor_num, uint32_t *value) {

  switch(sensor_num) {
    case SCM_SENSOR_OUTLET_U7_TEMP:
    case SCM_SENSOR_INLET_U8_TEMP:
    case SCM_SENSOR_BMC_LM75_U9_TEMP:
      *value = 30;
      break;
    case SCM_SENSOR_HSC_VOLT:
    case SCM_SENSOR_POWER_VOLT:
    case SCM_SENSOR_HSC_CURR:
    case SCM_SENSOR_HSC_POWER:
      *value = 30;
      break;
    default:
      *value = 30;
      break;
  }
}

static void
smb_sensor_poll_interval(uint8_t sensor_num, uint32_t *value) {

  switch(sensor_num) {
    case SMB_TMP422_U20_1_TEMP:
    case SMB_TMP422_U20_2_TEMP:
    case SMB_TMP422_U20_3_TEMP:
    case SIM_LM75_U1_TEMP:
    case SMB_SENSOR_TEMP1:
    case SMB_SENSOR_TEMP2:
    case SMB_SENSOR_TEMP3:
    case SMB_VDDC_SW_TEMP:
    case SMB_XP12R0V_VDDC_SW_IN:
    case SMB_VDDC_SW_IN_SENS:
    case SMB_VDDC_SW_POWER_IN:
    case SMB_VDDC_SW_POWER_OUT:
    case SMB_VDDC_SW_CURR_IN:
    case SMB_VDDC_SW_CURR_OUT:
    case SMB_SENSOR_PDB_L_TEMP1:
    case SMB_SENSOR_PDB_L_TEMP2:
    case SMB_SENSOR_PDB_R_TEMP1:
    case SMB_SENSOR_PDB_R_TEMP2:
    case SMB_SENSOR_FCM_T_TEMP1:
    case SMB_SENSOR_FCM_T_TEMP2:
    case SMB_SENSOR_FCM_B_TEMP1:
    case SMB_SENSOR_FCM_B_TEMP2:
      *value = 30;
      break;
    case SMB_XP3R3V_BMC:
    case SMB_XP2R5V_BMC:
    case SMB_XP1R8V_BMC:
    case SMB_XP1R2V_BMC:
    case SMB_XP1R0V_FPGA:
    case SMB_XP3R3V_USB:
    case SMB_XP5R0V:
    case SMB_XP3R3V_EARLY:
    case SMB_LM57_VTEMP:
    case SMB_XP1R8:
    case SMB_XP1R2:
    case SMB_VDDC_SW:
    case SMB_XP3R3V:
    case SMB_XP1R8V_AVDD:
    case SMB_XP1R2V_TVDD:
    case SMB_XP0R75V_1_PVDD:
    case SMB_XP0R75V_2_PVDD:
    case SMB_XP0R75V_3_PVDD:
    case SMB_VDD_PCIE:
    case SMB_XP0R84V_DCSU:
    case SMB_XP0R84V_CSU:
    case SMB_XP3R3V_TCXO:
    case SMB_SENSOR_FCM_T_HSC_VOLT:
    case SMB_SENSOR_FCM_B_HSC_VOLT:
    case SMB_SENSOR_FCM_T_HSC_CURR:
    case SMB_SENSOR_FCM_B_HSC_CURR:
    case SMB_SENSOR_FCM_T_HSC_POWER_VOLT:
    case SMB_SENSOR_FCM_B_HSC_POWER_VOLT:
    case SMB_OUTPUT_VOLTAGE_XP0R75V_1:
    case SMB_OUTPUT_CURRENT_XP0R75V_1:
    case SMB_INPUT_VOLTAGE_1:
    case SMB_OUTPUT_VOLTAGE_XP1R2V:
    case SMB_OUTPUT_CURRENT_XP1R2V:
    case SMB_OUTPUT_VOLTAGE_XP0R75V_2:
    case SMB_OUTPUT_CURRENT_XP0R75V_2:
    case SMB_INPUT_VOLTAGE_2:
    case SMB_SENSOR_TH4_HIGH:
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
    case SMB_SENSOR_FAN5_FRONT_TACH:
    case SMB_SENSOR_FAN5_REAR_TACH:
    case SMB_SENSOR_FAN6_FRONT_TACH:
    case SMB_SENSOR_FAN6_REAR_TACH:
    case SMB_SENSOR_FAN7_FRONT_TACH:
    case SMB_SENSOR_FAN7_REAR_TACH:
    case SMB_SENSOR_FAN8_FRONT_TACH:
    case SMB_SENSOR_FAN8_REAR_TACH:
      *value = 2;
      break;
    default:
      *value = 10;
      break;
  }
}

static void
pim_sensor_poll_interval(uint8_t sensor_num, uint32_t *value) {

  switch(sensor_num) {
    case PIM1_LM75_TEMP_4A:
    case PIM1_LM75_TEMP_4B:
    case PIM1_LM75_TEMP_48:
    case PIM1_SENSOR_HSC_VOLT:
    case PIM1_POWER_VOLTAGE:
    case PIM1_SENSOR_HSC_CURR:
    case PIM1_SENSOR_HSC_POWER:
      *value = 30;
      break;
    case PIM1_UCD90160_VOLT1:
    case PIM1_UCD90160_VOLT2:
    case PIM1_UCD90160_VOLT3:
    case PIM1_UCD90160_VOLT4:
    case PIM1_UCD90160_VOLT5:
    case PIM1_UCD90160_VOLT6:
    case PIM1_UCD90160_VOLT7:
    case PIM1_UCD90160_VOLT8:
    case PIM1_UCD90160_VOLT9:
    case PIM1_UCD90160_VOLT10:
    case PIM1_UCD90160_VOLT11:
    case PIM1_UCD90160_VOLT12:
    case PIM1_UCD90160_VOLT13:
    case PIM1_MP2975_INPUT_VOLTAGE:
    case PIM1_MP2975_OUTPUT_VOLTAGE_XP0R8V:
    case PIM1_MP2975_OUTPUT_VOLTAGE_XP3R3V:
    case PIM1_MP2975_OUTPUT_CURRENT_XP3R3V:
    case PIM1_MP2975_OUTPUT_CURRENT_XP0R8V:
      *value = 60;
      break;
    case PIM2_LM75_TEMP_4A:
    case PIM2_LM75_TEMP_4B:
    case PIM2_LM75_TEMP_48:
    case PIM2_SENSOR_HSC_VOLT:
    case PIM2_POWER_VOLTAGE:
    case PIM2_SENSOR_HSC_CURR:
    case PIM2_SENSOR_HSC_POWER:
      *value = 30;
      break;
    case PIM2_UCD90160_VOLT1:
    case PIM2_UCD90160_VOLT2:
    case PIM2_UCD90160_VOLT3:
    case PIM2_UCD90160_VOLT4:
    case PIM2_UCD90160_VOLT5:
    case PIM2_UCD90160_VOLT6:
    case PIM2_UCD90160_VOLT7:
    case PIM2_UCD90160_VOLT8:
    case PIM2_UCD90160_VOLT9:
    case PIM2_UCD90160_VOLT10:
    case PIM2_UCD90160_VOLT11:
    case PIM2_UCD90160_VOLT12:
    case PIM2_UCD90160_VOLT13:
    case PIM2_MP2975_INPUT_VOLTAGE:
    case PIM2_MP2975_OUTPUT_VOLTAGE_XP0R8V:
    case PIM2_MP2975_OUTPUT_VOLTAGE_XP3R3V:
    case PIM2_MP2975_OUTPUT_CURRENT_XP3R3V:
    case PIM2_MP2975_OUTPUT_CURRENT_XP0R8V:
      *value = 60;
      break;
    case PIM3_LM75_TEMP_4A:
    case PIM3_LM75_TEMP_4B:
    case PIM3_LM75_TEMP_48:
    case PIM3_SENSOR_HSC_VOLT:
    case PIM3_POWER_VOLTAGE:
    case PIM3_SENSOR_HSC_CURR:
    case PIM3_SENSOR_HSC_POWER:
      *value = 30;
      break;
    case PIM3_UCD90160_VOLT1:
    case PIM3_UCD90160_VOLT2:
    case PIM3_UCD90160_VOLT3:
    case PIM3_UCD90160_VOLT4:
    case PIM3_UCD90160_VOLT5:
    case PIM3_UCD90160_VOLT6:
    case PIM3_UCD90160_VOLT7:
    case PIM3_UCD90160_VOLT8:
    case PIM3_UCD90160_VOLT9:
    case PIM3_UCD90160_VOLT10:
    case PIM3_UCD90160_VOLT11:
    case PIM3_UCD90160_VOLT12:
    case PIM3_UCD90160_VOLT13:
    case PIM3_MP2975_OUTPUT_VOLTAGE_XP0R8V:
    case PIM3_MP2975_OUTPUT_VOLTAGE_XP3R3V:
    case PIM3_MP2975_OUTPUT_CURRENT_XP3R3V:
    case PIM3_MP2975_OUTPUT_CURRENT_XP0R8V:
      *value = 60;
      break;
    case PIM4_LM75_TEMP_4A:
    case PIM4_LM75_TEMP_4B:
    case PIM4_LM75_TEMP_48:
    case PIM4_SENSOR_HSC_VOLT:
    case PIM4_POWER_VOLTAGE:
    case PIM4_SENSOR_HSC_CURR:
    case PIM4_SENSOR_HSC_POWER:
      *value = 30;
      break;
    case PIM4_UCD90160_VOLT1:
    case PIM4_UCD90160_VOLT2:
    case PIM4_UCD90160_VOLT3:
    case PIM4_UCD90160_VOLT4:
    case PIM4_UCD90160_VOLT5:
    case PIM4_UCD90160_VOLT6:
    case PIM4_UCD90160_VOLT7:
    case PIM4_UCD90160_VOLT8:
    case PIM4_UCD90160_VOLT9:
    case PIM4_UCD90160_VOLT10:
    case PIM4_UCD90160_VOLT11:
    case PIM4_UCD90160_VOLT12:
    case PIM4_UCD90160_VOLT13:
    case PIM4_MP2975_INPUT_VOLTAGE:
    case PIM4_MP2975_OUTPUT_VOLTAGE_XP0R8V:
    case PIM4_MP2975_OUTPUT_VOLTAGE_XP3R3V:
    case PIM4_MP2975_OUTPUT_CURRENT_XP3R3V:
    case PIM4_MP2975_OUTPUT_CURRENT_XP0R8V:
      *value = 60;
      break;
    case PIM5_LM75_TEMP_4A:
    case PIM5_LM75_TEMP_4B:
    case PIM5_LM75_TEMP_48:
    case PIM5_SENSOR_HSC_VOLT:
    case PIM5_POWER_VOLTAGE:
    case PIM5_SENSOR_HSC_CURR:
    case PIM5_SENSOR_HSC_POWER:
      *value = 30;
      break;
    case PIM5_UCD90160_VOLT1:
    case PIM5_UCD90160_VOLT2:
    case PIM5_UCD90160_VOLT3:
    case PIM5_UCD90160_VOLT4:
    case PIM5_UCD90160_VOLT5:
    case PIM5_UCD90160_VOLT6:
    case PIM5_UCD90160_VOLT7:
    case PIM5_UCD90160_VOLT8:
    case PIM5_UCD90160_VOLT9:
    case PIM5_UCD90160_VOLT10:
    case PIM5_UCD90160_VOLT11:
    case PIM5_UCD90160_VOLT12:
    case PIM5_UCD90160_VOLT13:
    case PIM5_MP2975_INPUT_VOLTAGE:
    case PIM5_MP2975_OUTPUT_VOLTAGE_XP0R8V:
    case PIM5_MP2975_OUTPUT_VOLTAGE_XP3R3V:
    case PIM5_MP2975_OUTPUT_CURRENT_XP3R3V:
    case PIM5_MP2975_OUTPUT_CURRENT_XP0R8V:
      *value = 60;
      break;
    case PIM6_LM75_TEMP_4A:
    case PIM6_LM75_TEMP_4B:
    case PIM6_LM75_TEMP_48:
    case PIM6_SENSOR_HSC_VOLT:
    case PIM6_POWER_VOLTAGE:
    case PIM6_SENSOR_HSC_CURR:
    case PIM6_SENSOR_HSC_POWER:
      *value = 30;
      break;
    case PIM6_UCD90160_VOLT1:
    case PIM6_UCD90160_VOLT2:
    case PIM6_UCD90160_VOLT3:
    case PIM6_UCD90160_VOLT4:
    case PIM6_UCD90160_VOLT5:
    case PIM6_UCD90160_VOLT6:
    case PIM6_UCD90160_VOLT7:
    case PIM6_UCD90160_VOLT8:
    case PIM6_UCD90160_VOLT9:
    case PIM6_UCD90160_VOLT10:
    case PIM6_UCD90160_VOLT11:
    case PIM6_UCD90160_VOLT12:
    case PIM6_UCD90160_VOLT13:
    case PIM6_MP2975_INPUT_VOLTAGE:
    case PIM6_MP2975_OUTPUT_VOLTAGE_XP0R8V:
    case PIM6_MP2975_OUTPUT_VOLTAGE_XP3R3V:
    case PIM6_MP2975_OUTPUT_CURRENT_XP3R3V:
    case PIM6_MP2975_OUTPUT_CURRENT_XP0R8V:
      *value = 60;
      break;
    case PIM7_LM75_TEMP_4A:
    case PIM7_LM75_TEMP_4B:
    case PIM7_LM75_TEMP_48:
    case PIM7_SENSOR_HSC_VOLT:
    case PIM7_POWER_VOLTAGE:
    case PIM7_SENSOR_HSC_CURR:
    case PIM7_SENSOR_HSC_POWER:
      *value = 30;
      break;
    case PIM7_UCD90160_VOLT1:
    case PIM7_UCD90160_VOLT2:
    case PIM7_UCD90160_VOLT3:
    case PIM7_UCD90160_VOLT4:
    case PIM7_UCD90160_VOLT5:
    case PIM7_UCD90160_VOLT6:
    case PIM7_UCD90160_VOLT7:
    case PIM7_UCD90160_VOLT8:
    case PIM7_UCD90160_VOLT9:
    case PIM7_UCD90160_VOLT10:
    case PIM7_UCD90160_VOLT11:
    case PIM7_UCD90160_VOLT12:
    case PIM7_UCD90160_VOLT13:
    case PIM7_MP2975_INPUT_VOLTAGE:
    case PIM7_MP2975_OUTPUT_VOLTAGE_XP0R8V:
    case PIM7_MP2975_OUTPUT_VOLTAGE_XP3R3V:
    case PIM7_MP2975_OUTPUT_CURRENT_XP3R3V:
    case PIM7_MP2975_OUTPUT_CURRENT_XP0R8V:
      *value = 60;
      break;
    case PIM8_LM75_TEMP_4A:
    case PIM8_LM75_TEMP_4B:
    case PIM8_LM75_TEMP_48:
    case PIM8_SENSOR_HSC_VOLT:
    case PIM8_POWER_VOLTAGE:
    case PIM8_SENSOR_HSC_CURR:
    case PIM8_SENSOR_HSC_POWER:
      *value = 30;
      break;
    case PIM8_UCD90160_VOLT1:
    case PIM8_UCD90160_VOLT2:
    case PIM8_UCD90160_VOLT3:
    case PIM8_UCD90160_VOLT4:
    case PIM8_UCD90160_VOLT5:
    case PIM8_UCD90160_VOLT6:
    case PIM8_UCD90160_VOLT7:
    case PIM8_UCD90160_VOLT8:
    case PIM8_UCD90160_VOLT9:
    case PIM8_UCD90160_VOLT10:
    case PIM8_UCD90160_VOLT11:
    case PIM8_UCD90160_VOLT12:
    case PIM8_UCD90160_VOLT13:
    case PIM8_MP2975_INPUT_VOLTAGE:
    case PIM8_MP2975_OUTPUT_VOLTAGE_XP0R8V:
    case PIM8_MP2975_OUTPUT_VOLTAGE_XP3R3V:
    case PIM8_MP2975_OUTPUT_CURRENT_XP3R3V:
    case PIM8_MP2975_OUTPUT_CURRENT_XP0R8V:
      *value = 60;
      break;
    case PIM1_SENSOR_QSFP_TEMP:
    case PIM2_SENSOR_QSFP_TEMP:
    case PIM3_SENSOR_QSFP_TEMP:
    case PIM4_SENSOR_QSFP_TEMP:
    case PIM5_SENSOR_QSFP_TEMP:
    case PIM6_SENSOR_QSFP_TEMP:
    case PIM7_SENSOR_QSFP_TEMP:
    case PIM8_SENSOR_QSFP_TEMP:
      *value = 2;
      break;
    default:
      *value = 10;
      break;
  }
}

static void
psu_sensor_poll_interval(uint8_t sensor_num, uint32_t *value) {

  switch(sensor_num) {
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
    case PSU3_SENSOR_IN_VOLT:
    case PSU3_SENSOR_12V_VOLT:
    case PSU3_SENSOR_STBY_VOLT:
    case PSU3_SENSOR_IN_CURR:
    case PSU3_SENSOR_12V_CURR:
    case PSU3_SENSOR_STBY_CURR:
    case PSU3_SENSOR_IN_POWER:
    case PSU3_SENSOR_12V_POWER:
    case PSU3_SENSOR_STBY_POWER:
    case PSU3_SENSOR_FAN_TACH:
    case PSU3_SENSOR_TEMP1:
    case PSU3_SENSOR_TEMP2:
    case PSU3_SENSOR_TEMP3:
    case PSU4_SENSOR_IN_VOLT:
    case PSU4_SENSOR_12V_VOLT:
    case PSU4_SENSOR_STBY_VOLT:
    case PSU4_SENSOR_IN_CURR:
    case PSU4_SENSOR_12V_CURR:
    case PSU4_SENSOR_STBY_CURR:
    case PSU4_SENSOR_IN_POWER:
    case PSU4_SENSOR_12V_POWER:
    case PSU4_SENSOR_STBY_POWER:
    case PSU4_SENSOR_FAN_TACH:
    case PSU4_SENSOR_TEMP1:
    case PSU4_SENSOR_TEMP2:
    case PSU4_SENSOR_TEMP3:
      *value = 30;
      break;
    default:
      *value = 10;
      break;
  }
}

int
pal_get_sensor_poll_interval(uint8_t fru, uint8_t sensor_num, uint32_t *value) {

  switch(fru) {
    case FRU_SCM:
      scm_sensor_poll_interval(sensor_num, value);
      break;
    case FRU_SMB:
      smb_sensor_poll_interval(sensor_num, value);
      break;
    case FRU_PIM1:
    case FRU_PIM2:
    case FRU_PIM3:
    case FRU_PIM4:
    case FRU_PIM5:
    case FRU_PIM6:
    case FRU_PIM7:
    case FRU_PIM8:
      pim_sensor_poll_interval(sensor_num, value);
      break;
    case FRU_PSU1:
    case FRU_PSU2:
    case FRU_PSU3:
    case FRU_PSU4:
      psu_sensor_poll_interval(sensor_num, value);
      break;
    default:
      *value = 2;
      break;
  }
  return 0;
}

int
pal_init_sensor_check(uint8_t fru, uint8_t snr_num, void *snr) {
  _sensor_thresh_t *psnr = (_sensor_thresh_t *)snr;
  sensor_desc_t *snr_desc;

  snr_desc = get_sensor_desc(fru, snr_num);
  strncpy(snr_desc->name, psnr->name, sizeof(snr_desc->name));
  snr_desc->name[sizeof(snr_desc->name)-1] = 0;

  pal_set_def_key_value();

  switch (fru) {
    case FRU_PSU1:
    case FRU_PSU2:
    case FRU_PSU3:
    case FRU_PSU4:
      psu_init_acok_key(fru);
      break;
  }

  return 0;
}

int
pal_get_fru_health(uint8_t fru, uint8_t *value) {

  char cvalue[MAX_VALUE_LEN] = {0};
  char key[MAX_KEY_LEN] = {0};
  int ret;

  switch(fru) {
    case FRU_SCM:
      sprintf(key, "scm_sensor_health");
      break;
    case FRU_SMB:
      sprintf(key, "smb_sensor_health");
      break;
	case FRU_PIM1:
      sprintf(key, "pim1_sensor_health");
      break;
	case FRU_PIM2:
      sprintf(key, "pim2_sensor_health");
      break;
	case FRU_PIM3:
      sprintf(key, "pim3_sensor_health");
      break;
	case FRU_PIM4:
      sprintf(key, "pim4_sensor_health");
      break;
	case FRU_PIM5:
      sprintf(key, "pim5_sensor_health");
      break;
	case FRU_PIM6:
      sprintf(key, "pim6_sensor_health");
      break;
	case FRU_PIM7:
      sprintf(key, "pim7_sensor_health");
      break;
	case FRU_PIM8:
      sprintf(key, "pim8_sensor_health");
      break;
	case FRU_PSU1:
      sprintf(key, "psu1_sensor_health");
      break;
	case FRU_PSU2:
      sprintf(key, "psu2_sensor_health");
      break;
	case FRU_PSU3:
      sprintf(key, "psu3_sensor_health");
      break;
	case FRU_PSU4:
      sprintf(key, "psu4_sensor_health");
      break;

    default:
      return -1;
  }

  ret = pal_get_key_value(key, cvalue);
  if (ret) {
    return ret;
  }

  *value = atoi(cvalue);

  *value = *value & atoi(cvalue);
  return 0;
}

int
pal_set_sensor_health(uint8_t fru, uint8_t value) {

  char key[MAX_KEY_LEN] = {0};
  char cvalue[MAX_VALUE_LEN] = {0};

  switch(fru) {
    case FRU_SCM:
      sprintf(key, "scm_sensor_health");
      break;
    case FRU_SMB:
      sprintf(key, "smb_sensor_health");
      break;
	case FRU_PIM1:
      sprintf(key, "pim1_sensor_health");
      break;
	case FRU_PIM2:
      sprintf(key, "pim2_sensor_health");
      break;
	case FRU_PIM3:
      sprintf(key, "pim3_sensor_health");
      break;
	case FRU_PIM4:
      sprintf(key, "pim4_sensor_health");
      break;
	case FRU_PIM5:
      sprintf(key, "pim5_sensor_health");
      break;
	case FRU_PIM6:
      sprintf(key, "pim6_sensor_health");
      break;
	case FRU_PIM7:
      sprintf(key, "pim7_sensor_health");
      break;
	case FRU_PIM8:
      sprintf(key, "pim8_sensor_health");
      break;
	case FRU_PSU1:
      sprintf(key, "psu1_sensor_health");
      break;
	case FRU_PSU2:
      sprintf(key, "psu2_sensor_health");
      break;
	case FRU_PSU3:
      sprintf(key, "psu3_sensor_health");
      break;
	case FRU_PSU4:
      sprintf(key, "psu4_sensor_health");
      break;

    default:
      return -1;
  }

  sprintf(cvalue, (value > 0) ? "1": "0");

  return pal_set_key_value(key, cvalue);
}

int
pal_parse_sel(uint8_t fru, uint8_t *sel, char *error_log)
{
  uint8_t snr_num = sel[11];
  uint8_t *event_data = &sel[10];
  uint8_t *ed = &event_data[3];
  char temp_log[512] = {0};
  uint8_t sen_type = event_data[0];
  uint8_t chn_num, dimm_num;
  bool parsed = false;

  switch(snr_num) {
    case BIC_SENSOR_SYSTEM_STATUS:
      strcpy(error_log, "");
      switch (ed[0] & 0x0F) {
        case 0x00:
          strcat(error_log, "SOC_Thermal_Trip");
          break;
        case 0x01:
          strcat(error_log, "SOC_FIVR_Fault");
          break;
        case 0x02:
          strcat(error_log, "SOC_Throttle");
          break;
        case 0x03:
          strcat(error_log, "PCH_HOT");
          break;
      }
      parsed = true;
      break;

    case BIC_SENSOR_CPU_DIMM_HOT:
      strcpy(error_log, "");
      switch (ed[0] & 0x0F) {
        case 0x01:
          strcat(error_log, "SOC_MEMHOT");
          break;
      }
      parsed = true;
      break;

    case MEMORY_ECC_ERR:
    case MEMORY_ERR_LOG_DIS:
      strcpy(error_log, "");
      if (snr_num == MEMORY_ECC_ERR) {
        // SEL from MEMORY_ECC_ERR Sensor
        if ((ed[0] & 0x0F) == 0x0) {
          if (sen_type == 0x0C) {
            strcat(error_log, "Correctable");
            sprintf(temp_log, "DIMM%02X ECC err", ed[2]);
            pal_add_cri_sel(temp_log);
          } else if (sen_type == 0x10)
            strcat(error_log, "Correctable ECC error Logging Disabled");
        } else if ((ed[0] & 0x0F) == 0x1) {
          strcat(error_log, "Uncorrectable");
          sprintf(temp_log, "DIMM%02X UECC err", ed[2]);
          pal_add_cri_sel(temp_log);
        } else if ((ed[0] & 0x0F) == 0x5)
          strcat(error_log, "Correctable ECC error Logging Limit Reached");
        else
          strcat(error_log, "Unknown");
      } else {
        // SEL from MEMORY_ERR_LOG_DIS Sensor
        if ((ed[0] & 0x0F) == 0x0)
          strcat(error_log, "Correctable Memory Error Logging Disabled");
        else
          strcat(error_log, "Unknown");
      }

      // DIMM number (ed[2]):
      // Bit[7:5]: Socket number  (Range: 0-7)
      // Bit[4:3]: Channel number (Range: 0-3)
      // Bit[2:0]: DIMM number    (Range: 0-7)
      if (((ed[1] & 0xC) >> 2) == 0x0) {
        /* All Info Valid */
        chn_num = (ed[2] & 0x18) >> 3;
        dimm_num = ed[2] & 0x7;

        /* If critical SEL logging is available, do it */
        if (sen_type == 0x0C) {
          if ((ed[0] & 0x0F) == 0x0) {
            sprintf(temp_log, "DIMM%c%d ECC err,FRU:%u", 'A'+chn_num,
                    dimm_num, fru);
            pal_add_cri_sel(temp_log);
          } else if ((ed[0] & 0x0F) == 0x1) {
            sprintf(temp_log, "DIMM%c%d UECC err,FRU:%u", 'A'+chn_num,
                    dimm_num, fru);
            pal_add_cri_sel(temp_log);
          }
        }
        /* Then continue parse the error into a string. */
        /* All Info Valid                               */
        sprintf(temp_log, " DIMM %c%d Logical Rank %d (CPU# %d, CHN# %d, DIMM# %d)",
            'A'+chn_num, dimm_num, ed[1] & 0x03, (ed[2] & 0xE0) >> 5, chn_num, dimm_num);
      } else if (((ed[1] & 0xC) >> 2) == 0x1) {
        /* DIMM info not valid */
        sprintf(temp_log, " (CPU# %d, CHN# %d)",
            (ed[2] & 0xE0) >> 5, (ed[2] & 0x18) >> 3);
      } else if (((ed[1] & 0xC) >> 2) == 0x2) {
        /* CHN info not valid */
        sprintf(temp_log, " (CPU# %d, DIMM# %d)",
            (ed[2] & 0xE0) >> 5, ed[2] & 0x7);
      } else if (((ed[1] & 0xC) >> 2) == 0x3) {
        /* CPU info not valid */
        sprintf(temp_log, " (CHN# %d, DIMM# %d)",
            (ed[2] & 0x18) >> 3, ed[2] & 0x7);
      }
      strcat(error_log, temp_log);
      parsed = true;
      break;
  }

  if (parsed == true) {
    if ((event_data[2] & 0x80) == 0) {
      strcat(error_log, " Assertion");
    } else {
      strcat(error_log, " Deassertion");
    }
    return 0;
  }

  pal_parse_sel_helper(fru, sel, error_log);

  return 0;
}

int
fuji_sensor_name(uint8_t fru, uint8_t sensor_num, char *name) {
  switch(fru) {
    case FRU_SCM:
      switch(sensor_num) {
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
      break;
  }
  return 0;
}

int
pal_get_event_sensor_name(uint8_t fru, uint8_t *sel, char *name) {
  uint8_t snr_type = sel[10];
  uint8_t snr_num = sel[11];

  // If SNR_TYPE is OS_BOOT, sensor name is OS
  switch (snr_type) {
    case OS_BOOT:
      // OS_BOOT used by OS
      sprintf(name, "OS");
      return 0;
    default:
      if (fuji_sensor_name(fru, snr_num, name) != 0) {
        break;
      }
      return 0;
  }
  // Otherwise, translate it based on snr_num
  return pal_get_x86_event_sensor_name(fru, snr_num, name);
}

/* Populates all sensor_info_t struct using the path to SDR dump */
static int
sdr_init(char *path, sensor_info_t *sinfo) {
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
    syslog(LOG_ERR, "%s: open failed for %s\n", __func__, path);
    return -1;
  }

  ret = pal_flock_retry(fd);
  if (ret == -1) {
   syslog(LOG_WARNING, "%s: failed to flock on %s", __func__, path);
   close(fd);
   return -1;
  }

  while ((bytes_rd = read(fd, buf, sizeof(sdr_full_t))) > 0) {
    if (bytes_rd != sizeof(sdr_full_t)) {
      syslog(LOG_ERR, "%s: read returns %d bytes\n", __func__, bytes_rd);
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
    syslog(LOG_WARNING, "%s: failed to unflock on %s", __func__, path);
    close(fd);
    return -1;
  }

  close(fd);
  return 0;
}

int pal_set_sdr_update_flag(uint8_t slot, uint8_t update) {
  sdr_fru_update_flag[slot] = update;
  init_threshold_done[slot] = false;
  return 0;
}

int pal_get_sdr_update_flag(uint8_t slot) {
  return sdr_fru_update_flag[slot];
}


int bic_sensor_sdr_init(uint8_t fru, sensor_info_t *sinfo) {
  char path[64];
  int retry = 0;
  if (bic_sensor_sdr_path(fru, path) < 0) {
    syslog(LOG_WARNING,
           "bic_sensor_sdr_path: get_fru_sdr_path failed\n");
    return ERR_NOT_READY;
  }
  while (retry <= 3) {
    if (sdr_init(path, sinfo) < 0) {
      if (retry == 3) { //if the third retry still failed, return -1
        syslog(LOG_ERR,
               "bic_sensor_sdr_init: sdr_init failed for FRU %d", fru);
        return -1;
      }
      retry++;
      sleep(1);
    } else {
      break;
    }
  }
  return 0;
}

int bic_sdr_init(uint8_t fru, bool reinit) {

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
int bic_get_sdr_thresh_val(uint8_t fru, uint8_t snr_num,
                       uint8_t thresh, void *value) {
  int ret, retry = 0;
  int8_t b_exp, r_exp;
  uint8_t x, m_lsb, m_msb, b_lsb, b_msb, thresh_val;
  uint16_t m = 0, b = 0;
  sdr_full_t *sdr;
  char cvalue[MAX_VALUE_LEN] = {0};

  ret = kv_get(SCM_INIT_THRESH_STATUS, cvalue, NULL, 0);
  if (!strncmp(cvalue, "done", sizeof("done"))) {
    ret = bic_sdr_init(fru, false);
  } else {
    while ((ret = bic_sdr_init(fru, false)) == ERR_NOT_READY &&
      retry++ < MAX_SDR_THRESH_RETRY) {
      sleep(1);
    }
  }

  if (ret < 0) {
    syslog(LOG_CRIT, "BIC threshold value can't get");
    return -1;
  }
  sdr = &g_sinfo[fru-1][snr_num].sdr;

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
  * (float *) value = ((m * x) + (b * pow(10, b_exp))) * (pow(10, r_exp));

  return 0;
}

int bic_read_sensor_wrapper(uint8_t slot_id, uint8_t fru, uint8_t sensor_num, bool discrete,
    void *value) {

  int ret, i;
  ipmi_sensor_reading_t sensor;
  sdr_full_t *sdr;

  ret = bic_read_sensor(slot_id, sensor_num, &sensor);
  if (ret) {
    return ret;
  }

  if (sensor.flags & BIC_SENSOR_READ_NA) {
#ifdef DEBUG
    syslog(LOG_ERR, "bic_read_sensor_wrapper: Reading Not Available");
    syslog(LOG_ERR, "bic_read_sensor_wrapper: sensor_num: 0x%X, flag: 0x%X",
        sensor_num, sensor.flags);
#endif
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
    for(i=0;i<sizeof(bic_neg_reading_sensor_support_list)/sizeof(uint8_t);i++) {
      if (sensor_num == bic_neg_reading_sensor_support_list[i]) {
        * (float *) value -= (float) THERMAL_CONSTANT;
      }
    }
  }

  return 0;
}

int bic_sensor_sdr_path(uint8_t fru, char *path) {

  char fru_name[16];

  switch(fru) {
    case FRU_SCM:
      sprintf(fru_name, "%s", "scm");
      break;
    default:
  #ifdef DEBUG
      syslog(LOG_WARNING, "bic_sensor_sdr_path: Wrong Slot ID\n");
  #endif
      return -1;
  }

  sprintf(path, BIC_SDR_PATH, fru_name);

  if (access(path, F_OK) == -1) {
    return -1;
  }

  return 0;
}

int pal_get_sensor_util_timeout(uint8_t fru) {
  uint8_t pim_id = 0, pim_type = 0;
  size_t cnt = 0;
  int brd_type_rev = 0;

  pal_get_board_rev(&brd_type_rev);
  switch(fru) {
    case FRU_SCM:
      cnt = scm_all_sensor_cnt;
      break;
    case FRU_SMB:
      if(brd_type_rev == BOARD_FUJI_EVT1 ||
        brd_type_rev == BOARD_FUJI_EVT2 ||
        brd_type_rev == BOARD_FUJI_EVT3) {
        cnt = smb_sensor_cnt_evt;
      } else if (brd_type_rev == BOARD_FUJI_DVT1) {
        cnt = smb_sensor_cnt_dvt;
      }
      break;
    case FRU_PIM1:
    case FRU_PIM2:
    case FRU_PIM3:
    case FRU_PIM4:
    case FRU_PIM5:
    case FRU_PIM6:
    case FRU_PIM7:
    case FRU_PIM8:
      pim_id = fru - FRU_PIM1;
      pim_type = pal_get_pim_type_from_file(fru);
      if (pim_type == PIM_TYPE_16Q) {
        cnt = sizeof(pim16q_sensor_list[pim_id]) / sizeof(uint8_t);
      } else if (pim_type == PIM_TYPE_16O) {
        cnt = sizeof(pim16o_sensor_list[pim_id]) / sizeof(uint8_t);
      }
      break;
    case FRU_PSU1:
    case FRU_PSU2:
    case FRU_PSU3:
    case FRU_PSU4:
      cnt = psu1_sensor_cnt;
      break;
    default:
      if (fru > MAX_NUM_FRUS)
      cnt = 5;
      break;
  }
  return (READ_UNIT_SENSOR_TIMEOUT * cnt);
}
