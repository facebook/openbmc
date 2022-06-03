/*
 *
 * Copyright 2019-present Facebook. All Rights Reserved.
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

// #define DEBUG
#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>
#include <pthread.h>
#include <math.h>
#include <openbmc/log.h>
#include <openbmc/libgpio.h>
#include <openbmc/kv.h>
#include <openbmc/obmc-i2c.h>
#include <openbmc/sensor-correction.h>
#include <openbmc/misc-utils.h>
#include <facebook/bic.h>
#include <facebook/wedge_eeprom.h>
#include "pal_sensors.h"
#include "pal.h"

#define OFFSET_DEV_GUID 0x1800

#define GPIO_VAL "/sys/class/gpio/gpio%d/value"
#define SCM_BRD_ID "6-0021"
#define SENSOR_NAME_ERR "---- It should not be show ----"
#define LED_INTERVAL 5
uint8_t g_dev_guid[GUID_SIZE] = {0};

typedef struct {
  char name[32];
} sensor_desc_t;

struct threadinfo {
  uint8_t is_running;
  uint8_t fru;
  pthread_t pt;
};

static sensor_desc_t m_snr_desc[MAX_NUM_FRUS][MAX_SENSOR_NUM + 1] = {0};
static struct threadinfo t_dump[MAX_NUM_FRUS] = {0, };

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
/* for EVT DVT MP Hardware revision */
const uint8_t scm_evt_dvt_mp_sensor_list[] = {
  SCM_SENSOR_OUTLET_TEMP,
  SCM_SENSOR_INLET_TEMP,
  SCM_SENSOR_HSC_VOLT,
  SCM_SENSOR_HSC_CURR,
};

/* for MP respin or newer Hardware revision */
const uint8_t scm_mp_respin_sensor_list[] = {
  SCM_SENSOR_OUTLET_TEMP,
  SCM_SENSOR_HSC_VOLT,
  SCM_SENSOR_HSC_CURR,
};

/* List of SCM and BIC sensors to be monitored */
/* for EVT DVT MP Hardware revision */
const uint8_t scm_evt_dvt_mp_all_sensor_list[] = {
  SCM_SENSOR_OUTLET_TEMP,
  SCM_SENSOR_INLET_TEMP,
  SCM_SENSOR_HSC_VOLT,
  SCM_SENSOR_HSC_CURR,
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

/* for MP respin or newer Hardware revision */
const uint8_t scm_mp_respin_all_sensor_list[] = {
  SCM_SENSOR_OUTLET_TEMP,
  SCM_SENSOR_HSC_VOLT,
  SCM_SENSOR_HSC_CURR,
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
const uint8_t w400_smb_sensor_list[] = {
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
  SMB_SENSOR_SW_SERDES_PVDD_IN_VOLT,
  SMB_SENSOR_SW_SERDES_PVDD_IN_CURR,
  SMB_SENSOR_SW_SERDES_PVDD_IN_POWER,
  SMB_SENSOR_SW_SERDES_PVDD_OUT_VOLT,
  SMB_SENSOR_SW_SERDES_PVDD_OUT_CURR,
  SMB_SENSOR_SW_SERDES_PVDD_OUT_POWER,
  SMB_SENSOR_SW_SERDES_PVDD_TEMP1,
  SMB_SENSOR_SW_SERDES_TRVDD_IN_VOLT,
  SMB_SENSOR_SW_SERDES_TRVDD_IN_CURR,
  SMB_SENSOR_SW_SERDES_TRVDD_IN_POWER,
  SMB_SENSOR_SW_SERDES_TRVDD_OUT_VOLT,
  SMB_SENSOR_SW_SERDES_TRVDD_OUT_CURR,
  SMB_SENSOR_SW_SERDES_TRVDD_OUT_POWER,
  SMB_SENSOR_SW_SERDES_TRVDD_TEMP1,
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
  SMB_SENSOR_SW_CORE_VOLT,
  SMB_SENSOR_SW_CORE_CURR,
  SMB_SENSOR_SW_CORE_POWER,
  SMB_SENSOR_SW_CORE_TEMP1,
  SMB_SENSOR_LM75B_U28_TEMP,
  SMB_SENSOR_LM75B_U25_TEMP,
  SMB_SENSOR_LM75B_U56_TEMP,
  SMB_SENSOR_LM75B_U55_TEMP,
  SMB_SENSOR_TMP421_U62_TEMP,
  SMB_SENSOR_TMP421_Q9_TEMP,
  SMB_SENSOR_TMP421_U63_TEMP,
  SMB_SENSOR_TMP421_Q10_TEMP,
  SMB_SENSOR_TMP422_U20_TEMP,
  SMB_SENSOR_SW_DIE_TEMP1,
  SMB_SENSOR_SW_DIE_TEMP2,
  SMB_DOM1_MAX_TEMP,
  SMB_DOM2_MAX_TEMP,
  /* Sensors on FCM */
  SMB_SENSOR_FCM_LM75B_U1_TEMP,
  SMB_SENSOR_FCM_LM75B_U2_TEMP,
  SMB_SENSOR_FCM_HSC_VOLT,
  SMB_SENSOR_FCM_HSC_CURR,
  /* BMC ADC Sensors  */
  SMB_BMC_ADC0_VSEN,
  SMB_BMC_ADC1_VSEN,
  SMB_BMC_ADC2_VSEN,
  SMB_BMC_ADC3_VSEN,
  SMB_BMC_ADC4_VSEN,

};

const uint8_t w400_mp_respin_smb_sensor_list[] = {
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
  SMB_SENSOR_1220_VCCA,
  SMB_SENSOR_1220_VCCINP,
  SMB_SENSOR_SW_SERDES_PVDD_IN_VOLT,
  SMB_SENSOR_SW_SERDES_PVDD_IN_CURR,
  SMB_SENSOR_SW_SERDES_PVDD_IN_POWER,
  SMB_SENSOR_SW_SERDES_PVDD_OUT_VOLT,
  SMB_SENSOR_SW_SERDES_PVDD_OUT_CURR,
  SMB_SENSOR_SW_SERDES_PVDD_OUT_POWER,
  SMB_SENSOR_SW_SERDES_PVDD_TEMP1,
  SMB_SENSOR_SW_SERDES_TRVDD_IN_VOLT,
  SMB_SENSOR_SW_SERDES_TRVDD_IN_CURR,
  SMB_SENSOR_SW_SERDES_TRVDD_IN_POWER,
  SMB_SENSOR_SW_SERDES_TRVDD_OUT_VOLT,
  SMB_SENSOR_SW_SERDES_TRVDD_OUT_CURR,
  SMB_SENSOR_SW_SERDES_TRVDD_OUT_POWER,
  SMB_SENSOR_SW_SERDES_TRVDD_TEMP1,
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
  SMB_SENSOR_SW_CORE_VOLT,
  SMB_SENSOR_SW_CORE_CURR,
  SMB_SENSOR_SW_CORE_POWER,
  SMB_SENSOR_SW_CORE_TEMP1,
  SMB_SENSOR_LM75B_U28_TEMP,
  SMB_SENSOR_LM75B_U55_TEMP,
  SMB_SENSOR_TMP421_U62_TEMP,
  SMB_SENSOR_TMP421_Q9_TEMP,
  SMB_SENSOR_TMP421_U63_TEMP,
  SMB_SENSOR_TMP421_Q10_TEMP,
  SMB_SENSOR_SW_DIE_TEMP1,
  SMB_SENSOR_SW_DIE_TEMP2,
  SMB_DOM1_MAX_TEMP,
  SMB_DOM2_MAX_TEMP,
  /* Sensors on FCM */
  SMB_SENSOR_FCM_LM75B_U1_TEMP,
  SMB_SENSOR_FCM_LM75B_U2_TEMP,
  SMB_SENSOR_FCM_HSC_VOLT,
  SMB_SENSOR_FCM_HSC_CURR,
  /* BMC ADC Sensors  */
  SMB_BMC_ADC0_VSEN,
  SMB_BMC_ADC1_VSEN,
  SMB_BMC_ADC2_VSEN,
  SMB_BMC_ADC3_VSEN,
  SMB_BMC_ADC4_VSEN,
};

const uint8_t w400c_evt1_smb_sensor_list[] = {
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
  SMB_SENSOR_SW_SERDES_PVDD_IN_VOLT,
  SMB_SENSOR_SW_SERDES_PVDD_IN_CURR,
  SMB_SENSOR_SW_SERDES_PVDD_IN_POWER,
  SMB_SENSOR_SW_SERDES_PVDD_OUT_VOLT,
  SMB_SENSOR_SW_SERDES_PVDD_OUT_CURR,
  SMB_SENSOR_SW_SERDES_PVDD_OUT_POWER,
  SMB_SENSOR_SW_SERDES_PVDD_TEMP1,
  SMB_SENSOR_SW_SERDES_TRVDD_IN_VOLT,
  SMB_SENSOR_SW_SERDES_TRVDD_IN_CURR,
  SMB_SENSOR_SW_SERDES_TRVDD_IN_POWER,
  SMB_SENSOR_SW_SERDES_TRVDD_OUT_VOLT,
  SMB_SENSOR_SW_SERDES_TRVDD_OUT_CURR,
  SMB_SENSOR_SW_SERDES_TRVDD_OUT_POWER,
  SMB_SENSOR_SW_SERDES_TRVDD_TEMP1,
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
  SMB_SENSOR_SW_CORE_VOLT,
  SMB_SENSOR_SW_CORE_CURR,
  SMB_SENSOR_SW_CORE_POWER,
  SMB_SENSOR_LM75B_U28_TEMP,
  SMB_SENSOR_LM75B_U25_TEMP,
  SMB_SENSOR_LM75B_U56_TEMP,
  SMB_SENSOR_LM75B_U55_TEMP,
  SMB_SENSOR_TMP421_U62_TEMP,
  SMB_SENSOR_TMP421_Q9_TEMP,
  SMB_SENSOR_TMP421_U63_TEMP,
  SMB_SENSOR_TMP421_Q10_TEMP,
  SMB_SENSOR_SW_DIE_TEMP1,
  SMB_SENSOR_SW_DIE_TEMP2,
  SMB_DOM1_MAX_TEMP,
  SMB_DOM2_MAX_TEMP,
  /* Sensors on FCM */
  SMB_SENSOR_FCM_LM75B_U1_TEMP,
  SMB_SENSOR_FCM_LM75B_U2_TEMP,
  SMB_SENSOR_FCM_HSC_VOLT,
  SMB_SENSOR_FCM_HSC_CURR,
  /* BMC ADC Sensors  */
  SMB_BMC_ADC0_VSEN,
  SMB_BMC_ADC1_VSEN,
  SMB_BMC_ADC2_VSEN,
  SMB_BMC_ADC3_VSEN,
  SMB_BMC_ADC4_VSEN,
  /* IR35215 1-43 only on Wedge400C-EVT*/
  SMB_SENSOR_HBM_IN_VOLT,
  SMB_SENSOR_HBM_OUT_VOLT,
  SMB_SENSOR_HBM_IN_CURR,
  SMB_SENSOR_HBM_OUT_CURR,
  SMB_SENSOR_HBM_IN_POWER,
  SMB_SENSOR_HBM_OUT_POWER,
  SMB_SENSOR_HBM_TEMP,
  SMB_SENSOR_VDDCK_0_IN_VOLT,
  SMB_SENSOR_VDDCK_0_OUT_VOLT,
  SMB_SENSOR_VDDCK_0_IN_CURR,
  SMB_SENSOR_VDDCK_0_OUT_CURR,
  SMB_SENSOR_VDDCK_0_IN_POWER,
  SMB_SENSOR_VDDCK_0_OUT_POWER,
  SMB_SENSOR_VDDCK_0_TEMP,
  /* GB switch internal sensors */
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
};

const uint8_t w400c_evt2_smb_sensor_list[] = {
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
  SMB_SENSOR_SW_SERDES_PVDD_IN_VOLT,
  SMB_SENSOR_SW_SERDES_PVDD_IN_CURR,
  SMB_SENSOR_SW_SERDES_PVDD_IN_POWER,
  SMB_SENSOR_SW_SERDES_PVDD_OUT_VOLT,
  SMB_SENSOR_SW_SERDES_PVDD_OUT_CURR,
  SMB_SENSOR_SW_SERDES_PVDD_OUT_POWER,
  SMB_SENSOR_SW_SERDES_PVDD_TEMP1,
  SMB_SENSOR_SW_SERDES_TRVDD_IN_VOLT,
  SMB_SENSOR_SW_SERDES_TRVDD_IN_CURR,
  SMB_SENSOR_SW_SERDES_TRVDD_IN_POWER,
  SMB_SENSOR_SW_SERDES_TRVDD_OUT_VOLT,
  SMB_SENSOR_SW_SERDES_TRVDD_OUT_CURR,
  SMB_SENSOR_SW_SERDES_TRVDD_OUT_POWER,
  SMB_SENSOR_SW_SERDES_TRVDD_TEMP1,
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
  SMB_SENSOR_SW_CORE_VOLT,
  SMB_SENSOR_SW_CORE_CURR,
  SMB_SENSOR_SW_CORE_POWER,
  SMB_SENSOR_LM75B_U28_TEMP,
  SMB_SENSOR_LM75B_U25_TEMP,
  SMB_SENSOR_LM75B_U56_TEMP,
  SMB_SENSOR_LM75B_U55_TEMP,
  SMB_SENSOR_TMP421_U62_TEMP,
  SMB_SENSOR_TMP421_Q9_TEMP,
  SMB_SENSOR_TMP421_U63_TEMP,
  SMB_SENSOR_TMP421_Q10_TEMP,
  SMB_SENSOR_SW_DIE_TEMP1,
  SMB_SENSOR_SW_DIE_TEMP2,
  SMB_DOM1_MAX_TEMP,
  SMB_DOM2_MAX_TEMP,
  /* Sensors on FCM */
  SMB_SENSOR_FCM_LM75B_U1_TEMP,
  SMB_SENSOR_FCM_LM75B_U2_TEMP,
  SMB_SENSOR_FCM_HSC_VOLT,
  SMB_SENSOR_FCM_HSC_CURR,
  /* BMC ADC Sensors  */
  SMB_BMC_ADC0_VSEN,
  SMB_BMC_ADC1_VSEN,
  SMB_BMC_ADC2_VSEN,
  SMB_BMC_ADC3_VSEN,
  SMB_BMC_ADC4_VSEN,
  /* PXE1211C on Wedge400C-EVT2 or later */
  SMB_SENSOR_HBM_IN_VOLT,
  SMB_SENSOR_HBM_OUT_VOLT,
  SMB_SENSOR_HBM_OUT_CURR,
  SMB_SENSOR_HBM_OUT_POWER,
  SMB_SENSOR_HBM_TEMP,
  SMB_SENSOR_VDDCK_0_IN_VOLT,
  SMB_SENSOR_VDDCK_0_OUT_VOLT,
  SMB_SENSOR_VDDCK_0_OUT_CURR,
  SMB_SENSOR_VDDCK_0_OUT_POWER,
  SMB_SENSOR_VDDCK_0_TEMP,
  SMB_SENSOR_VDDCK_1_IN_VOLT,
  SMB_SENSOR_VDDCK_1_OUT_VOLT,
  SMB_SENSOR_VDDCK_1_OUT_CURR,
  SMB_SENSOR_VDDCK_1_OUT_POWER,
  SMB_SENSOR_VDDCK_1_TEMP,
  /* GB switch internal sensors */
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
};

const uint8_t w400c_respin_smb_sensor_list[] = {
  SMB_SENSOR_1220_VMON1,
  SMB_SENSOR_1220_VMON2,
  SMB_SENSOR_1220_VMON3,
  SMB_SENSOR_1220_VMON4,
  SMB_SENSOR_1220_VMON6,
  SMB_SENSOR_1220_VMON7,
  SMB_SENSOR_1220_VMON9,
  SMB_SENSOR_1220_VMON10,
  SMB_SENSOR_1220_VMON11,
  SMB_SENSOR_1220_VMON12,
  SMB_SENSOR_1220_VCCA,
  SMB_SENSOR_1220_VCCINP,
  SMB_SENSOR_SW_SERDES_PVDD_IN_VOLT,
  SMB_SENSOR_SW_SERDES_PVDD_IN_CURR,
  SMB_SENSOR_SW_SERDES_PVDD_IN_POWER,
  SMB_SENSOR_SW_SERDES_PVDD_OUT_VOLT,
  SMB_SENSOR_SW_SERDES_PVDD_OUT_CURR,
  SMB_SENSOR_SW_SERDES_PVDD_OUT_POWER,
  SMB_SENSOR_SW_SERDES_PVDD_TEMP1,
  SMB_SENSOR_SW_SERDES_TRVDD_IN_VOLT,
  SMB_SENSOR_SW_SERDES_TRVDD_IN_CURR,
  SMB_SENSOR_SW_SERDES_TRVDD_IN_POWER,
  SMB_SENSOR_SW_SERDES_TRVDD_OUT_VOLT,
  SMB_SENSOR_SW_SERDES_TRVDD_OUT_CURR,
  SMB_SENSOR_SW_SERDES_TRVDD_OUT_POWER,
  SMB_SENSOR_SW_SERDES_TRVDD_TEMP1,
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
  SMB_SENSOR_SW_CORE_VOLT,
  SMB_SENSOR_SW_CORE_CURR,
  SMB_SENSOR_SW_CORE_POWER,
  SMB_SENSOR_LM75B_U28_TEMP,
  SMB_SENSOR_LM75B_U25_TEMP,
  SMB_SENSOR_LM75B_U56_TEMP,
  SMB_SENSOR_LM75B_U55_TEMP,
  SMB_SENSOR_SW_DIE_TEMP1,
  SMB_SENSOR_SW_DIE_TEMP2,
  SMB_DOM1_MAX_TEMP,
  SMB_DOM2_MAX_TEMP,
  /* Sensors on FCM */
  SMB_SENSOR_FCM_LM75B_U1_TEMP,
  SMB_SENSOR_FCM_LM75B_U2_TEMP,
  SMB_SENSOR_FCM_HSC_VOLT,
  SMB_SENSOR_FCM_HSC_CURR,
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
  /* PXE1211C on Wedge400C-EVT2 or later */
  SMB_SENSOR_HBM_IN_VOLT,
  SMB_SENSOR_HBM_OUT_VOLT,
  SMB_SENSOR_HBM_OUT_CURR,
  SMB_SENSOR_HBM_OUT_POWER,
  SMB_SENSOR_HBM_TEMP,
  SMB_SENSOR_VDDCK_0_IN_VOLT,
  SMB_SENSOR_VDDCK_0_OUT_VOLT,
  SMB_SENSOR_VDDCK_0_OUT_CURR,
  SMB_SENSOR_VDDCK_0_OUT_POWER,
  SMB_SENSOR_VDDCK_0_TEMP,
  SMB_SENSOR_VDDCK_1_IN_VOLT,
  SMB_SENSOR_VDDCK_1_OUT_VOLT,
  SMB_SENSOR_VDDCK_1_OUT_CURR,
  SMB_SENSOR_VDDCK_1_OUT_POWER,
  SMB_SENSOR_VDDCK_1_TEMP,
  /* GB switch internal sensors */
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
};

const uint8_t fan1_sensor_list[] = {
  /* Sensors FAN1 Speed */
  FAN_SENSOR_FAN1_FRONT_TACH,
  FAN_SENSOR_FAN1_REAR_TACH,
};

const uint8_t fan2_sensor_list[] = {
  /* Sensors FAN2 Speed */
  FAN_SENSOR_FAN2_FRONT_TACH,
  FAN_SENSOR_FAN2_REAR_TACH,
};

const uint8_t fan3_sensor_list[] = {
  /* Sensors FAN3 Speed */
  FAN_SENSOR_FAN3_FRONT_TACH,
  FAN_SENSOR_FAN3_REAR_TACH,
};

const uint8_t fan4_sensor_list[] = {
  /* Sensors FAN4 Speed */
  FAN_SENSOR_FAN4_FRONT_TACH,
  FAN_SENSOR_FAN4_REAR_TACH,
};

const uint8_t pem1_sensor_list[] = {
  PEM1_SENSOR_IN_VOLT,
  PEM1_SENSOR_OUT_VOLT,
  PEM1_SENSOR_FET_BAD,
  PEM1_SENSOR_FET_SHORT,
  PEM1_SENSOR_CURR,
  PEM1_SENSOR_POWER,
  PEM1_SENSOR_FAN1_TACH,
  PEM1_SENSOR_FAN2_TACH,
  PEM1_SENSOR_TEMP1,
  PEM1_SENSOR_TEMP2,
  PEM1_SENSOR_TEMP3,
};

const uint8_t pem1_discrete_list[] = {
  /* Discrete fault sensors on PEM1 */
  PEM1_SENSOR_FAULT_OV,
  PEM1_SENSOR_FAULT_UV,
  PEM1_SENSOR_FAULT_OC,
  PEM1_SENSOR_FAULT_POWER,
  PEM1_SENSOR_ON_FAULT,
  PEM1_SENSOR_FAULT_FET_SHORT,
  PEM1_SENSOR_FAULT_FET_BAD,
  PEM1_SENSOR_EEPROM_DONE,
  /* Discrete ADC alert sensors on PEM2 */
  PEM1_SENSOR_POWER_ALARM_HIGH,
  PEM1_SENSOR_POWER_ALARM_LOW,
  PEM1_SENSOR_VSENSE_ALARM_HIGH,
  PEM1_SENSOR_VSENSE_ALARM_LOW,
  PEM1_SENSOR_VSOURCE_ALARM_HIGH,
  PEM1_SENSOR_VSOURCE_ALARM_LOW,
  PEM1_SENSOR_GPIO_ALARM_HIGH,
  PEM1_SENSOR_GPIO_ALARM_LOW,
  /* Discrete status sensors on PEM1 */
  PEM1_SENSOR_ON_STATUS,
  PEM1_SENSOR_STATUS_FET_BAD,
  PEM1_SENSOR_STATUS_FET_SHORT,
  PEM1_SENSOR_STATUS_ON_PIN,
  PEM1_SENSOR_STATUS_POWER_GOOD,
  PEM1_SENSOR_STATUS_OC,
  PEM1_SENSOR_STATUS_UV,
  PEM1_SENSOR_STATUS_OV,
  PEM1_SENSOR_STATUS_GPIO3,
  PEM1_SENSOR_STATUS_GPIO2,
  PEM1_SENSOR_STATUS_GPIO1,
  PEM1_SENSOR_STATUS_ALERT,
  PEM1_SENSOR_STATUS_EEPROM_BUSY,
  PEM1_SENSOR_STATUS_ADC_IDLE,
  PEM1_SENSOR_STATUS_TICKER_OVERFLOW,
  PEM1_SENSOR_STATUS_METER_OVERFLOW,
};

const uint8_t pem2_sensor_list[] = {
  PEM2_SENSOR_IN_VOLT,
  PEM2_SENSOR_OUT_VOLT,
  PEM2_SENSOR_FET_BAD,
  PEM2_SENSOR_FET_SHORT,
  PEM2_SENSOR_CURR,
  PEM2_SENSOR_POWER,
  PEM2_SENSOR_FAN1_TACH,
  PEM2_SENSOR_FAN2_TACH,
  PEM2_SENSOR_TEMP1,
  PEM2_SENSOR_TEMP2,
  PEM2_SENSOR_TEMP3,
};

const uint8_t pem2_discrete_list[] = {
  /* Discrete fault sensors on PEM2 */
  PEM2_SENSOR_FAULT_OV,
  PEM2_SENSOR_FAULT_UV,
  PEM2_SENSOR_FAULT_OC,
  PEM2_SENSOR_FAULT_POWER,
  PEM2_SENSOR_ON_FAULT,
  PEM2_SENSOR_FAULT_FET_SHORT,
  PEM2_SENSOR_FAULT_FET_BAD,
  PEM2_SENSOR_EEPROM_DONE,
  /* Discrete ADC alert sensors on PEM2 */
  PEM2_SENSOR_POWER_ALARM_HIGH,
  PEM2_SENSOR_POWER_ALARM_LOW,
  PEM2_SENSOR_VSENSE_ALARM_HIGH,
  PEM2_SENSOR_VSENSE_ALARM_LOW,
  PEM2_SENSOR_VSOURCE_ALARM_HIGH,
  PEM2_SENSOR_VSOURCE_ALARM_LOW,
  PEM2_SENSOR_GPIO_ALARM_HIGH,
  PEM2_SENSOR_GPIO_ALARM_LOW,
  /* Discrete status sensors on PEM2 */
  PEM2_SENSOR_ON_STATUS,
  PEM2_SENSOR_STATUS_FET_BAD,
  PEM2_SENSOR_STATUS_FET_SHORT,
  PEM2_SENSOR_STATUS_ON_PIN,
  PEM2_SENSOR_STATUS_POWER_GOOD,
  PEM2_SENSOR_STATUS_OC,
  PEM2_SENSOR_STATUS_UV,
  PEM2_SENSOR_STATUS_OV,
  PEM2_SENSOR_STATUS_GPIO3,
  PEM2_SENSOR_STATUS_GPIO2,
  PEM2_SENSOR_STATUS_GPIO1,
  PEM2_SENSOR_STATUS_ALERT,
  PEM2_SENSOR_STATUS_EEPROM_BUSY,
  PEM2_SENSOR_STATUS_ADC_IDLE,
  PEM2_SENSOR_STATUS_TICKER_OVERFLOW,
  PEM2_SENSOR_STATUS_METER_OVERFLOW,
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
  PSU1_SENSOR_FAN2_TACH,
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
  PSU2_SENSOR_FAN2_TACH,
};


float scm_sensor_threshold[MAX_SENSOR_NUM + 1][MAX_SENSOR_THRESHOLD + 1] = {0};
float smb_sensor_threshold[MAX_SENSOR_NUM + 1][MAX_SENSOR_THRESHOLD + 1] = {0};
float fan_sensor_threshold[MAX_SENSOR_NUM + 1][MAX_SENSOR_THRESHOLD + 1] = {0};
float pem_sensor_threshold[MAX_SENSOR_NUM + 1][MAX_SENSOR_THRESHOLD + 1] = {0};
float psu_sensor_threshold[MAX_SENSOR_NUM + 1][MAX_SENSOR_THRESHOLD + 1] = {0};

size_t bic_discrete_cnt = sizeof(bic_discrete_list)/sizeof(uint8_t);
size_t scm_evt_dvt_mp_sensor_cnt = sizeof(scm_evt_dvt_mp_sensor_list)/sizeof(uint8_t);
size_t scm_mp_respin_sensor_cnt = sizeof(scm_mp_respin_sensor_list)/sizeof(uint8_t);
size_t scm_evt_dvt_mp_all_sensor_cnt = sizeof(scm_evt_dvt_mp_all_sensor_list)/sizeof(uint8_t);
size_t scm_mp_respin_all_sensor_cnt = sizeof(scm_mp_respin_all_sensor_list)/sizeof(uint8_t);
size_t w400_smb_sensor_cnt = sizeof(w400_smb_sensor_list)/sizeof(uint8_t);
size_t w400_mp_respin_smb_sensor_cnt = sizeof(w400_mp_respin_smb_sensor_list)/sizeof(uint8_t);
size_t w400c_evt1_smb_sensor_cnt = sizeof(w400c_evt1_smb_sensor_list)/sizeof(uint8_t);
size_t w400c_evt2_smb_sensor_cnt = sizeof(w400c_evt2_smb_sensor_list)/sizeof(uint8_t);
size_t w400c_respin_smb_sensor_cnt = sizeof(w400c_respin_smb_sensor_list)/sizeof(uint8_t);
size_t fan_sensor_cnt = sizeof(fan1_sensor_list)/sizeof(uint8_t);
size_t pem1_sensor_cnt = sizeof(pem1_sensor_list)/sizeof(uint8_t);
size_t pem1_discrete_cnt = sizeof(pem1_discrete_list)/sizeof(uint8_t);
size_t pem2_sensor_cnt = sizeof(pem2_sensor_list)/sizeof(uint8_t);
size_t pem2_discrete_cnt = sizeof(pem2_discrete_list)/sizeof(uint8_t);
size_t psu1_sensor_cnt = sizeof(psu1_sensor_list)/sizeof(uint8_t);
size_t psu2_sensor_cnt = sizeof(psu2_sensor_list)/sizeof(uint8_t);

static sensor_info_t g_sinfo[MAX_NUM_FRUS][MAX_SENSOR_NUM + 1] = {0};

static float hsc_rsense[MAX_NUM_FRUS] = {0};

const char pal_fru_list[] = "all, scm, smb, pem1, pem2, \
psu1, psu2, fan1, fan2, fan3, fan4 ";

char * key_list[] = {
  "pwr_server_last_state",
  "sysfw_ver_server",
  "timestamp_sled",
  "server_por_cfg",
  "server_sel_error",
  "scm_sensor_health",
  "smb_sensor_health",
  "pem1_sensor_health",
  "pem2_sensor_health",
  "psu1_sensor_health",
  "psu2_sensor_health",
  "fan1_sensor_health",
  "fan2_sensor_health",
  "fan3_sensor_health",
  "fan4_sensor_health",
  "server_boot_order",
  /* Add more Keys here */
  LAST_KEY /* This is the last key of the list */
};

char * def_val_list[] = {
  "on", /* pwr_server_last_state */
  "0", /* sysfw_ver_server */
  "0", /* timestamp_sled */
  "lps", /* server_por_cfg */
  "1", /* server_sel_error */
  "1", /* scm_sensor_health */
  "1", /* smb_sensor_health */
  "1", /* pem1_sensor_health */
  "1", /* pem2_sensor_health */
  "1", /* psu1_sensor_health */
  "1", /* psu2_sensor_health */
  "1", /* fan1_sensor_health */
  "1", /* fan2_sensor_health */
  "1", /* fan3_sensor_health */
  "1", /* fan4_sensor_health */
  "0000000",/* server_boot_order */
  /* Add more def values for the correspoding keys*/
  LAST_KEY /* Same as last entry of the key_list */
};

void
pal_inform_bic_mode(uint8_t fru, uint8_t mode) {
  switch(mode) {
  case BIC_MODE_NORMAL:
    // Bridge IC entered normal mode
    // Inform BIOS that BMC is ready
    bic_set_gpio(fru, BMC_READY_N, 0);
    break;
  case BIC_MODE_UPDATE:
    // Bridge IC entered update mode
    // TODO: Might need to handle in future
    break;
  default:
    break;
  }
}

bool is_psu48(void);

//For OEM command "CMD_OEM_GET_PLAT_INFO" 0x7e
int
pal_get_plat_sku_id(void){
  return 0x06; // Wedge400/Wedge400-C
}


//Use part of the function for OEM Command "CMD_OEM_GET_POSS_PCIE_CONFIG" 0xF4
int
pal_get_poss_pcie_config(uint8_t slot, uint8_t *req_data, uint8_t req_len,
                         uint8_t *res_data, uint8_t *res_len) {

  uint8_t completion_code = CC_SUCCESS;
  uint8_t pcie_conf = 0x02; // Wedge400/wedge400-C
  uint8_t *data = res_data;
  *data++ = pcie_conf;
  *res_len = data - res_data;
  return completion_code;
}

static int
pal_key_check(char *key) {
  int i;

  i = 0;
  while(strcmp(key_list[i], LAST_KEY)) {
    // If Key is valid, return success
    if (!strcmp(key, key_list[i]))
      return 0;

    i++;
  }

#ifdef DEBUG
  OBMC_WARN("pal_key_check: invalid key - %s", key);
#endif
  return -1;
}

int
pal_get_key_value(char *key, char *value) {
  int ret;
  // Check is key is defined and valid
  if (pal_key_check(key))
    return -1;

  ret = kv_get(key, value, NULL, KV_FPERSIST);
  return ret;
}

int
pal_set_key_value(char *key, char *value) {

  // Check is key is defined and valid
  if (pal_key_check(key))
    return -1;

  return kv_set(key, value, 0, KV_FPERSIST);
}

int
pal_set_sysfw_ver(uint8_t slot, uint8_t *ver) {
  int i;
  char key[MAX_KEY_LEN];
  char str[MAX_VALUE_LEN] = {0};
  char tstr[10];
  sprintf(key, "%s", "sysfw_ver_server");

  for (i = 0; i < SIZE_SYSFW_VER; i++) {
    sprintf(tstr, "%02x", ver[i]);
    strcat(str, tstr);
  }

  return pal_set_key_value(key, str);
}

int
pal_get_sysfw_ver(uint8_t slot, uint8_t *ver) {
  int i;
  int j = 0;
  int ret;
  int msb, lsb;
  char key[MAX_KEY_LEN];
  char str[MAX_VALUE_LEN] = {0};
  char tstr[4];

  sprintf(key, "%s", "sysfw_ver_server");

  ret = pal_get_key_value(key, str);
  if (ret) {
    return ret;
  }

  for (i = 0; i < 2*SIZE_SYSFW_VER; i += 2) {
    sprintf(tstr, "%c\n", str[i]);
    msb = strtol(tstr, NULL, 16);

    sprintf(tstr, "%c\n", str[i+1]);
    lsb = strtol(tstr, NULL, 16);
    ver[j++] = (msb << 4) | lsb;
  }

  return 0;
}

int
pal_get_fru_list(char *list) {
  strcpy(list, pal_fru_list);
  return 0;
}

int
pal_get_fru_capability(uint8_t fru, unsigned int *caps)
{
  if (fru == FRU_BMC) {
    *caps = FRU_CAPABILITY_FRUID_ALL | FRU_CAPABILITY_SENSOR_ALL;
  } else if (fru > FRU_ALL && fru <= FRU_FPGA) {
    *caps = FRU_CAPABILITY_SENSOR_ALL;
  } else {
    return -1;
  }
  return 0;
}

int
pal_get_fru_count() {
  return MAX_NUM_FRUS;
}

int
pal_get_fru_id(char *str, uint8_t *fru) {
  if (!strcmp(str, "all")) {
    *fru = FRU_ALL;
  } else if (!strcmp(str, "smb")) {
    *fru = FRU_SMB;
  } else if (!strcmp(str, "scm")) {
    *fru = FRU_SCM;
  } else if (!strcmp(str, "pem1")) {
    *fru = FRU_PEM1;
  } else if (!strcmp(str, "pem2")) {
    *fru = FRU_PEM2;
  } else if (!strcmp(str, "psu1")) {
    *fru = FRU_PSU1;
  } else if (!strcmp(str, "psu2")) {
    *fru = FRU_PSU2;
  } else if (!strcmp(str, "fan1")) {
    *fru = FRU_FAN1;
  } else if (!strcmp(str, "fan2")) {
    *fru = FRU_FAN2;
  } else if (!strcmp(str, "fan3")) {
    *fru = FRU_FAN3;
  } else if (!strcmp(str, "fan4")) {
    *fru = FRU_FAN4;
  } else if (!strcmp(str, "bmc")) {
    *fru = FRU_BMC;
  } else if (!strcmp(str, "cpld")) {
    *fru = FRU_CPLD;
  } else if (!strcmp(str, "fpga")) {
    *fru = FRU_FPGA;
  } else {
    OBMC_WARN("pal_get_fru_id: Wrong fru#%s", str);
    return -1;
  }

  return 0;
}

int
pal_get_fru_name(uint8_t fru, char *name) {
  switch(fru) {
    case FRU_SMB:
      strcpy(name, "smb");
      break;
    case FRU_SCM:
      strcpy(name, "scm");
      break;
    case FRU_PEM1:
      strcpy(name, "pem1");
      break;
    case FRU_PEM2:
      strcpy(name, "pem2");
      break;
    case FRU_PSU1:
      strcpy(name, "psu1");
      break;
    case FRU_PSU2:
      strcpy(name, "psu2");
      break;
    case FRU_FAN1:
      strcpy(name, "fan1");
      break;
    case FRU_FAN2:
      strcpy(name, "fan2");
      break;
    case FRU_FAN3:
      strcpy(name, "fan3");
      break;
    case FRU_FAN4:
      strcpy(name, "fan4");
      break;
    default:
      if (fru > MAX_NUM_FRUS)
        return -1;
      sprintf(name, "fru%d", fru);
      break;
  }
  return 0;
}

// Platform Abstraction Layer (PAL) Functions
int
pal_get_platform_name(char *name) {
  strcpy(name, PLATFORM_NAME);

  return 0;
}

int
pal_is_fru_prsnt(uint8_t fru, uint8_t *status) {
  int val,ext_prsnt;
  char tmp[LARGEST_DEVICE_NAME];
  char path[LARGEST_DEVICE_NAME + 1];
  *status = 0;

  switch (fru) {
    case FRU_SMB:
      *status = 1;
      return 0;
    case FRU_SCM:
      snprintf(path, LARGEST_DEVICE_NAME, SMB_SYSFS, SCM_PRSNT_STATUS);
      break;
    case FRU_PEM1:
      snprintf(tmp, LARGEST_DEVICE_NAME, SMB_SYSFS, PEM_PRSNT_STATUS);
      snprintf(path, LARGEST_DEVICE_NAME, tmp, 1);
      break;
    case FRU_PEM2:
      snprintf(tmp, LARGEST_DEVICE_NAME, SMB_SYSFS, PEM_PRSNT_STATUS);
      snprintf(path, LARGEST_DEVICE_NAME, tmp, 2);
      break;
    case FRU_PSU1:
      snprintf(tmp, LARGEST_DEVICE_NAME, SMB_SYSFS, PSU_PRSNT_STATUS);
      snprintf(path, LARGEST_DEVICE_NAME, tmp, 1);
      break;
    case FRU_PSU2:
      snprintf(tmp, LARGEST_DEVICE_NAME, SMB_SYSFS, PSU_PRSNT_STATUS);
      snprintf(path, LARGEST_DEVICE_NAME, tmp, 2);
      break;
    case FRU_FAN1:
    case FRU_FAN2:
    case FRU_FAN3:
    case FRU_FAN4:
      snprintf(tmp, LARGEST_DEVICE_NAME, FCM_SYSFS, FAN_PRSNT_STATUS);
      snprintf(path, LARGEST_DEVICE_NAME, tmp, fru - FRU_FAN1 + 1);
      break;
    default:
      printf("unsupported fru id %d\n", fru);
      return -1;
    }

    if (device_read(path, &val)) {
      return -1;
    }

    if (val == 0x0) {
      *status = 1;
    } else {
      *status = 0;
      return 0;
    }

    if ( fru == FRU_PEM1 || fru == FRU_PSU1 ){
      ext_prsnt = i2c_detect_device(24,0x18); // 0 present -1 absent
      if( fru == FRU_PEM1 && ext_prsnt == 0 ){ // for PEM 0x18 should present
        *status = 1;
      } else if ( fru == FRU_PSU1 && ext_prsnt < 0 ){ // for PSU 0x18 should absent
        *status = 1;
      } else {
        *status = 0;
      }
    }
    else if ( fru == FRU_PEM2 || fru == FRU_PSU2 ){
      ext_prsnt = i2c_detect_device(25,0x18); // 0 present -1 absent
      if( fru == FRU_PEM2 && ext_prsnt == 0 ){ // for PEM 0x18 should present
        *status = 1;
      } else if ( fru == FRU_PSU2 && ext_prsnt < 0 ){ // for PSU 0x18 should absent
        *status = 1;
      } else {
        *status = 0;
      }
    }
    return 0;
}

static int check_dir_exist(const char *device);

int
pal_is_fru_ready(uint8_t fru, uint8_t *status) {
  int ret = 0;

  switch(fru) {
    case FRU_PEM1:
      if(!check_dir_exist(PEM1_LTC4282_DIR) && !check_dir_exist(PEM1_MAX6615_DIR)) {
        *status = 1;
      }
      break;
    case FRU_PEM2:
      if(!check_dir_exist(PEM2_LTC4282_DIR) && !check_dir_exist(PEM2_MAX6615_DIR)) {
        *status = 1;
      }
      break;
    case FRU_PSU1:
      if(!check_dir_exist(PSU1_DEVICE)) {
        *status = 1;
      }
      break;
    case FRU_PSU2:
      if(!check_dir_exist(PSU2_DEVICE)) {
        *status = 1;
      }
      break;
    default:
      *status = 1;

      break;
  }

  return ret;
}

int
pal_get_sensor_util_timeout(uint8_t fru) {
  uint8_t brd_type;
  uint8_t brd_type_rev;
  size_t cnt = 0;
  pal_get_board_type(&brd_type);
  pal_get_board_type_rev(&brd_type_rev);
  switch(fru) {
    case FRU_SCM:
      if (( brd_type == BRD_TYPE_WEDGE400 && brd_type_rev >= BOARD_WEDGE400_MP_RESPIN ) ||
          ( brd_type == BRD_TYPE_WEDGE400C && brd_type_rev >= BOARD_WEDGE400C_MP_RESPIN ))
      {
        cnt = scm_mp_respin_all_sensor_cnt;
      } else {
        cnt = scm_evt_dvt_mp_all_sensor_cnt;
      }
      break;
    case FRU_SMB:
      if(brd_type == BRD_TYPE_WEDGE400){
        if(brd_type_rev >= BOARD_WEDGE400_MP_RESPIN){
          cnt = w400_mp_respin_smb_sensor_cnt;
        }else{
          cnt = w400_smb_sensor_cnt;
        }
      }else if(brd_type == BRD_TYPE_WEDGE400C){
        if(brd_type_rev == BOARD_WEDGE400C_EVT){
          cnt = w400c_evt1_smb_sensor_cnt;
        }else if(brd_type_rev == BOARD_WEDGE400C_EVT2 ||
                 brd_type_rev == BOARD_WEDGE400C_DVT ||
                 brd_type_rev == BOARD_WEDGE400C_DVT2 ){
          cnt = w400c_evt2_smb_sensor_cnt;
        }else if(brd_type_rev == BOARD_WEDGE400C_MP_RESPIN ){
          cnt = w400c_respin_smb_sensor_cnt;
        }
      }
      break;
    case FRU_FAN1:
    case FRU_FAN2:
    case FRU_FAN3:
    case FRU_FAN4:
      cnt = fan_sensor_cnt;
      break;
    case FRU_PEM1:
      cnt = pem1_sensor_cnt;
      break;
    case FRU_PEM2:
      cnt = pem2_sensor_cnt;
      break;
    case FRU_PSU1:
      if (is_psu48()) {
        cnt = psu1_sensor_cnt;
      } else {
        cnt = psu1_sensor_cnt - 1;
      }
      break;
    case FRU_PSU2:
      if (is_psu48()) {
        cnt = psu2_sensor_cnt;
      } else {
        cnt = psu2_sensor_cnt - 1;
      }
      break;
    default:
      if (fru > MAX_NUM_FRUS)
      cnt = 5;
      break;
  }
  return (READ_UNIT_SENSOR_TIMEOUT * cnt);
}

int
pal_get_fru_sensor_list(uint8_t fru, uint8_t **sensor_list, int *cnt) {
  uint8_t brd_type;
  uint8_t brd_type_rev;
  pal_get_board_type(&brd_type);
  pal_get_board_type_rev(&brd_type_rev);
  switch(fru) {
  case FRU_SCM:
    if (( brd_type == BRD_TYPE_WEDGE400 && brd_type_rev >= BOARD_WEDGE400_MP_RESPIN ) ||
        ( brd_type == BRD_TYPE_WEDGE400C && brd_type_rev >= BOARD_WEDGE400C_MP_RESPIN ))
    {
      *sensor_list = (uint8_t *) scm_mp_respin_all_sensor_list;
      *cnt = scm_mp_respin_all_sensor_cnt;
    } else {
      *sensor_list = (uint8_t *) scm_evt_dvt_mp_all_sensor_list;
      *cnt = scm_evt_dvt_mp_all_sensor_cnt;
    }
    break;
  case FRU_SMB:
    if(brd_type == BRD_TYPE_WEDGE400){
      if(brd_type_rev >= BOARD_WEDGE400_MP_RESPIN){
        *sensor_list = (uint8_t *) w400_mp_respin_smb_sensor_list;
        *cnt = w400_mp_respin_smb_sensor_cnt;
      }else{
        *sensor_list = (uint8_t *) w400_smb_sensor_list;
        *cnt = w400_smb_sensor_cnt;
      }
    }else if(brd_type == BRD_TYPE_WEDGE400C){
      if(brd_type_rev == BOARD_WEDGE400C_EVT){
        *sensor_list = (uint8_t *) w400c_evt1_smb_sensor_list;
        *cnt = w400c_evt1_smb_sensor_cnt;
      }else if(brd_type_rev == BOARD_WEDGE400C_EVT2 ||
               brd_type_rev == BOARD_WEDGE400C_DVT ||
               brd_type_rev == BOARD_WEDGE400C_DVT2 ){
        *sensor_list = (uint8_t *) w400c_evt2_smb_sensor_list;
        *cnt = w400c_evt2_smb_sensor_cnt;
      }else if(brd_type_rev == BOARD_WEDGE400C_MP_RESPIN ){
        *sensor_list = (uint8_t *) w400c_respin_smb_sensor_list;
        *cnt = w400c_respin_smb_sensor_cnt;
      }
    }
    break;
  case FRU_FAN1:
    *sensor_list = (uint8_t *) fan1_sensor_list;
    *cnt = fan_sensor_cnt;
    break;
  case FRU_FAN2:
    *sensor_list = (uint8_t *) fan2_sensor_list;
    *cnt = fan_sensor_cnt;
    break;
  case FRU_FAN3:
    *sensor_list = (uint8_t *) fan3_sensor_list;
    *cnt = fan_sensor_cnt;
    break;
  case FRU_FAN4:
    *sensor_list = (uint8_t *) fan4_sensor_list;
    *cnt = fan_sensor_cnt;
    break;
  case FRU_PEM1:
    *sensor_list = (uint8_t *) pem1_sensor_list;
    *cnt = pem1_sensor_cnt;
    break;
  case FRU_PEM2:
    *sensor_list = (uint8_t *) pem2_sensor_list;
    *cnt = pem2_sensor_cnt;
    break;
  case FRU_PSU1:
    *sensor_list = (uint8_t *) psu1_sensor_list;
    if (is_psu48()) {
      *cnt = psu1_sensor_cnt;
    } else {
      *cnt = psu1_sensor_cnt - 1;
    }
    break;
  case FRU_PSU2:
    *sensor_list = (uint8_t *) psu2_sensor_list;
    if (is_psu48()) {
      *cnt = psu2_sensor_cnt;
    } else {
      *cnt = psu2_sensor_cnt - 1;
    }
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

void
pal_update_ts_sled()
{
  char key[MAX_KEY_LEN];
  char tstr[MAX_VALUE_LEN] = {0};
  struct timespec ts;

  clock_gettime(CLOCK_REALTIME, &ts);
  sprintf(tstr, "%ld", ts.tv_sec);

  sprintf(key, "timestamp_sled");

  pal_set_key_value(key, tstr);
}

int
pal_get_fru_discrete_list(uint8_t fru, uint8_t **sensor_list, int *cnt) {
  switch(fru) {
  case FRU_SCM:
    *sensor_list = (uint8_t *) bic_discrete_list;
    *cnt = bic_discrete_cnt;
    break;
  case FRU_PEM1:
    *sensor_list = (uint8_t *) pem1_discrete_list;
    *cnt = pem1_discrete_cnt;
    break;
  case FRU_PEM2:
    *sensor_list = (uint8_t *) pem2_discrete_list;
    *cnt = pem2_discrete_cnt;
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
    OBMC_CRIT("ASSERT: %s discrete - raised - FRU: %d, num: 0x%X,"
        " snr: %-16s val: %d", event, fru, snr_num, snr_name, val);
  } else {
    OBMC_CRIT("DEASSERT: %s discrete - settled - FRU: %d, num: 0x%X,"
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
    switch(snr_num) {
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
      _print_sensor_discrete_log(fru, snr_num, snr_name,
                                      GETBIT(n_val, 0), name);
      valid = false;
    }
  }

  if (GETBIT(diff, 1)) {
    switch(snr_num) {
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
      _print_sensor_discrete_log(fru, snr_num, snr_name,
                                      GETBIT(n_val, 1), name);
      valid = false;
    }
  }

  if (GETBIT(diff, 2)) {
    switch(snr_num) {
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
      _print_sensor_discrete_log(fru, snr_num, snr_name,
                                      GETBIT(n_val, 2), name);
      valid = false;
    }
  }

  if (GETBIT(diff, 4)) {
    if (snr_num == BIC_SENSOR_PROC_FAIL) {
        _print_sensor_discrete_log(fru, snr_num, snr_name,
                                        GETBIT(n_val, 4), "FRB3");
    }
  }

  return 0;
}

int
pal_is_debug_card_prsnt(uint8_t *status) {
  int val;
  char path[LARGEST_DEVICE_NAME + 1];

  snprintf(path, LARGEST_DEVICE_NAME, GPIO_DEBUG_PRSNT_N, "value");

  if (device_read(path, &val)) {
    return -1;
  }

  if (val) {
    *status = 1;
  } else {
    *status = 0;
  }

  return 0;
}

// Enable POST buffer for the server in given slot
int
pal_post_enable(uint8_t slot) {
  int ret;

  bic_config_t config = {0};
  bic_config_u *t = (bic_config_u *) &config;

  ret = bic_get_config(IPMB_BUS, &config);
  if (ret) {
#ifdef DEBUG
    OBMC_WARN("post_enable: bic_get_config failed for fru: %d\n", slot);
#endif
    return ret;
  }

  if (0 == t->bits.post) {
    t->bits.post = 1;
    ret = bic_set_config(IPMB_BUS, &config);
    if (ret) {
#ifdef DEBUG
      OBMC_WARN("post_enable: bic_set_config failed\n");
#endif
      return ret;
    }
  }

  return 0;
}

// Get the last post code of the given slot
int
pal_post_get_last(uint8_t slot, uint8_t *status) {
  int ret;
  uint8_t buf[MAX_IPMB_RES_LEN] = {0x0};
  uint8_t len = 0 ;

  ret = bic_get_post_buf(IPMB_BUS, buf, &len);
  if (ret) {
    return ret;
  }

  *status = buf[0];

  return 0;
}

int
pal_post_get_last_and_len(uint8_t slot, uint8_t *data, uint8_t *len) {
  int ret;

  ret = bic_get_post_buf(IPMB_BUS, data, len);
  if (ret) {
    return ret;
  }

  return 0;
}

int
pal_get_80port_record(uint8_t slot, uint8_t *res_data, size_t max_len, size_t *res_len)
{
  int ret;
  uint8_t len;

  ret = bic_get_post_buf(IPMB_BUS, res_data, &len);
  if (ret) {
    return ret;
  } else {
    *res_len = len;
  }

  return 0;
}

static int
pal_set_post_gpio_out(void) {
  char path[LARGEST_DEVICE_NAME + 1];
  int ret;
  char *val = "out";

  snprintf(path, LARGEST_DEVICE_NAME, GPIO_POSTCODE_0, "direction");
  ret = device_write_buff(path, val);
  if (ret) {
    goto post_exit;
  }

  snprintf(path, LARGEST_DEVICE_NAME, GPIO_POSTCODE_1, "direction");
  ret = device_write_buff(path, val);
  if (ret) {
    goto post_exit;
  }

  snprintf(path, LARGEST_DEVICE_NAME, GPIO_POSTCODE_2, "direction");
  ret = device_write_buff(path, val);
  if (ret) {
    goto post_exit;
  }

  snprintf(path, LARGEST_DEVICE_NAME, GPIO_POSTCODE_3, "direction");
  ret = device_write_buff(path, val);
  if (ret) {
    goto post_exit;
  }

  snprintf(path, LARGEST_DEVICE_NAME, GPIO_POSTCODE_4, "direction");
  ret = device_write_buff(path, val);
  if (ret) {
    goto post_exit;
  }

  snprintf(path, LARGEST_DEVICE_NAME, GPIO_POSTCODE_5, "direction");
  ret = device_write_buff(path, val);
  if (ret) {
    goto post_exit;
  }

  snprintf(path, LARGEST_DEVICE_NAME, GPIO_POSTCODE_6, "direction");
  ret = device_write_buff(path, val);
  if (ret) {
    goto post_exit;
  }

  snprintf(path, LARGEST_DEVICE_NAME, GPIO_POSTCODE_7, "direction");
  ret = device_write_buff(path, val);
  if (ret) {
    goto post_exit;
  }

  post_exit:
  if (ret) {
#ifdef DEBUG
    OBMC_WARN("device_write_buff failed for %s\n", path);
#endif
    return -1;
  } else {
    return 0;
  }
}

// Display the given POST code using GPIO port
static int
pal_post_display(uint8_t status) {
  char path[LARGEST_DEVICE_NAME + 1];
  int ret;
  char *val;

#ifdef DEBUG
  OBMC_WARN("pal_post_display: status is %d\n", status);
#endif

  ret = pal_set_post_gpio_out();
  if (ret) {
    return -1;
  }

  snprintf(path, LARGEST_DEVICE_NAME, GPIO_POSTCODE_0, "value");
  if (BIT(status, 0)) {
    val = "1";
  } else {
    val = "0";
  }

  ret = device_write_buff(path, val);
  if (ret) {
    goto post_exit;
  }

  snprintf(path, LARGEST_DEVICE_NAME, GPIO_POSTCODE_1, "value");
  if (BIT(status, 1)) {
    val = "1";
  } else {
    val = "0";
  }

  ret = device_write_buff(path, val);
  if (ret) {
    goto post_exit;
  }

  snprintf(path, LARGEST_DEVICE_NAME, GPIO_POSTCODE_2, "value");
  if (BIT(status, 2)) {
    val = "1";
  } else {
    val = "0";
  }

  ret = device_write_buff(path, val);
  if (ret) {
    goto post_exit;
  }

  snprintf(path, LARGEST_DEVICE_NAME, GPIO_POSTCODE_3, "value");
  if (BIT(status, 3)) {
    val = "1";
  } else {
    val = "0";
  }

  ret = device_write_buff(path, val);
  if (ret) {
    goto post_exit;
  }

  snprintf(path, LARGEST_DEVICE_NAME, GPIO_POSTCODE_4, "value");
  if (BIT(status, 4)) {
    val = "1";
  } else {
    val = "0";
  }

  ret = device_write_buff(path, val);
  if (ret) {
    goto post_exit;
  }

  snprintf(path, LARGEST_DEVICE_NAME, GPIO_POSTCODE_5, "value");
  if (BIT(status, 5)) {
    val = "1";
  } else {
    val = "0";
  }

  ret = device_write_buff(path, val);
  if (ret) {
    goto post_exit;
  }

  snprintf(path, LARGEST_DEVICE_NAME, GPIO_POSTCODE_6, "value");
  if (BIT(status, 6)) {
    val = "1";
  } else {
    val = "0";
  }

  ret = device_write_buff(path, val);
  if (ret) {
    goto post_exit;
  }

  snprintf(path, LARGEST_DEVICE_NAME, GPIO_POSTCODE_7, "value");
  if (BIT(status, 7)) {
    val = "1";
  } else {
    val = "0";
  }

  ret = device_write_buff(path, val);
  if (ret) {
    goto post_exit;
  }

post_exit:
  if (ret) {
#ifdef DEBUG
    OBMC_WARN("device_write_buff failed for %s\n", path);
#endif
    return -1;
  } else {
    return 0;
  }
}

// Handle the received post code, for now display it on debug card
int
pal_post_handle(uint8_t slot, uint8_t status) {
  uint8_t prsnt;
  int ret;

  // Check for debug card presence
  ret = pal_is_debug_card_prsnt(&prsnt);
  if (ret) {
    return ret;
  }

  // No debug card present, return
  if (!prsnt) {
    return 0;
  }

  // Display the post code in the debug card
  ret = pal_post_display(status);
  if (ret) {
    return ret;
  }

  return 0;
}

// Return the Front panel Power Button
int
pal_get_board_rev(int *rev) {
  char path[LARGEST_DEVICE_NAME + 1];
  int val_id_0, val_id_1, val_id_2;

  snprintf(path, LARGEST_DEVICE_NAME, GPIO_SMB_REV_ID_0, "value");
  if (device_read(path, &val_id_0)) {
    return -1;
  }

  snprintf(path, LARGEST_DEVICE_NAME, GPIO_SMB_REV_ID_1, "value");
  if (device_read(path, &val_id_1)) {
    return -1;
  }

  snprintf(path, LARGEST_DEVICE_NAME, GPIO_SMB_REV_ID_2, "value");
  if (device_read(path, &val_id_2)) {
    return -1;
  }

  *rev = val_id_0 | (val_id_1 << 1) | (val_id_2 << 2);

  return 0;
}

int
pal_get_board_type(uint8_t *brd_type){
  char path[LARGEST_DEVICE_NAME + 1];
  int val;

  snprintf(path, LARGEST_DEVICE_NAME, GPIO_BMC_BRD_TYPE_0, "value");
  if (device_read(path, &val)) {
    return CC_UNSPECIFIED_ERROR;
  }

  if(val == 0x1){
    *brd_type = BRD_TYPE_WEDGE400;
    return CC_SUCCESS;
  }else if(val == 0x0){
    *brd_type = BRD_TYPE_WEDGE400C;
    return CC_SUCCESS;
  }else{
    return CC_UNSPECIFIED_ERROR;
  }
}

int
pal_get_full_board_type(uint8_t *full_brd_type) {
  char path[LARGEST_DEVICE_NAME + 1];
  int val_id_0, val_id_1, val_id_2;

  snprintf(path, LARGEST_DEVICE_NAME, GPIO_BMC_BRD_TYPE_0, "value");
  if (device_read(path, &val_id_0)) {
    return -1;
  }

  snprintf(path, LARGEST_DEVICE_NAME, GPIO_BMC_BRD_TYPE_1, "value");
  if (device_read(path, &val_id_1)) {
    return -1;
  }

  snprintf(path, LARGEST_DEVICE_NAME, GPIO_BMC_BRD_TYPE_2, "value");
  if (device_read(path, &val_id_2)) {
    return -1;
  }

  *full_brd_type = val_id_0 | (val_id_1 << 1) | (val_id_2 << 2);

  return 0;
}

int
pal_get_board_type_rev(uint8_t *brd_type_rev){
  int brd_rev;
  uint8_t brd_type;
  uint8_t full_brd_type;
  if( pal_get_board_rev(&brd_rev) != 0 ||
      pal_get_board_type(&brd_type) != 0 ||
      pal_get_full_board_type(&full_brd_type) != 0 ){
        return -1;
  } else if ( brd_type == BRD_TYPE_WEDGE400 ){
    if ( full_brd_type & 0x02 ) {  // Indicates MP RESPIN type.
      switch ( brd_rev ) {
        case 0x06: *brd_type_rev = BOARD_WEDGE400_MP_RESPIN; break;
        default:
          *brd_type_rev = BOARD_UNDEFINED;
          return -1;
      }
    } else {
      switch ( brd_rev ) {
        case 0x00: *brd_type_rev = BOARD_WEDGE400_EVT_EVT3; break;
        case 0x02: *brd_type_rev = BOARD_WEDGE400_DVT; break;
        case 0x03: *brd_type_rev = BOARD_WEDGE400_DVT2_PVT_PVT2; break;
        case 0x04: *brd_type_rev = BOARD_WEDGE400_PVT3; break;
        case 0x05: *brd_type_rev = BOARD_WEDGE400_MP; break;
        default:
          *brd_type_rev = BOARD_UNDEFINED;
          return -1;
      }
    }
  } else if ( brd_type == BRD_TYPE_WEDGE400C ){
    if ( full_brd_type & 0x02 ) { // Indicates MP RESPIN type.
      switch ( brd_rev ) {
        case 0x06: *brd_type_rev = BOARD_WEDGE400C_MP_RESPIN; break;
        default:
          *brd_type_rev = BOARD_UNDEFINED;
          return -1;
      }
    } else {
      switch ( brd_rev ) {
        case 0x00: *brd_type_rev = BOARD_WEDGE400C_EVT; break;
        case 0x01: *brd_type_rev = BOARD_WEDGE400C_EVT2; break;
        case 0x02: *brd_type_rev = BOARD_WEDGE400C_DVT; break;
        case 0x03: *brd_type_rev = BOARD_WEDGE400C_DVT2; break;
        default:
          *brd_type_rev = BOARD_UNDEFINED;
          return -1;
      }
    }
  } else {
    *brd_type_rev = BOARD_UNDEFINED;
    return -1;
  }
  return 0;
}

int
pal_get_board_id(uint8_t slot, uint8_t *req_data, uint8_t req_len, uint8_t *res_data, uint8_t *res_len)
{
	int board_id = 0, board_rev ;
	unsigned char *data = res_data;
	int completion_code = CC_UNSPECIFIED_ERROR;


  int i = 0, num_chips, num_pins;
  char device[64], path[32];
	gpiochip_desc_t *chips[GPIO_CHIP_MAX];
  gpiochip_desc_t *gcdesc;
  gpio_desc_t *gdesc;
  gpio_value_t value;

	num_chips = gpiochip_list(chips, ARRAY_SIZE(chips));
  if(num_chips < 0){
    *res_len = 0;
		return completion_code;
  }

  gcdesc = gpiochip_lookup(SCM_BRD_ID);
  if (gcdesc == NULL) {
    *res_len = 0;
		return completion_code;
  }

  num_pins = gpiochip_get_ngpio(gcdesc);
  gpiochip_get_device(gcdesc, device, sizeof(device));

  for(i = 0; i < num_pins; i++){
    sprintf(path, "%s%d", "BRD_ID_",i);
    gpio_export_by_offset(device,i,path);
    gdesc = gpio_open_by_shadow(path);
    if (gpio_get_value(gdesc, &value) == 0) {
      board_id  |= (((int)value)<<i);
    }
    gpio_unexport(path);
  }

	if(pal_get_board_rev(&board_rev) == -1){
    *res_len = 0;
		return completion_code;
  }

	*data++ = board_id;
	*data++ = board_rev;
	*data++ = slot;
	*data++ = 0x00; // 1S Server.
	*res_len = data - res_data;
	completion_code = CC_SUCCESS;

	return completion_code;
}

int
pal_get_cpld_board_rev(int *rev, const char *device) {
  char full_name[LARGEST_DEVICE_NAME + 1];

  snprintf(full_name, LARGEST_DEVICE_NAME, device, "board_ver");
  if (device_read(full_name, rev)) {
    return -1;
  }

  return 0;
}

int
pal_get_cpld_fpga_fw_ver(uint8_t fru, const char *device, uint8_t* ver) {
  int val = -1;
  char ver_path[PATH_MAX];
  char sub_ver_path[PATH_MAX];

  switch(fru) {
    case FRU_CPLD:
      if (!(strncmp(device, SCM_CPLD, strlen(SCM_CPLD)))) {
        snprintf(ver_path, sizeof(ver_path), SCM_SYSFS, "cpld_ver");
        snprintf(sub_ver_path, sizeof(sub_ver_path),
                 SCM_SYSFS, "cpld_sub_ver");
      } else if (!(strncmp(device, SMB_CPLD, strlen(SMB_CPLD)))) {
        snprintf(ver_path, sizeof(ver_path), SMB_SYSFS, "cpld_ver");
        snprintf(sub_ver_path, sizeof(sub_ver_path),
                 SMB_SYSFS, "cpld_sub_ver");
      } else if (!(strncmp(device, PWR_CPLD, strlen(PWR_CPLD)))) {
        snprintf(ver_path, sizeof(ver_path), PWR_SYSFS, "cpld_ver");
        snprintf(sub_ver_path, sizeof(sub_ver_path),
                 PWR_SYSFS, "cpld_sub_ver");
      } else if (!(strncmp(device, FCM_CPLD, strlen(FCM_CPLD)))) {
        snprintf(ver_path, sizeof(ver_path), FCM_SYSFS, "cpld_ver");
        snprintf(sub_ver_path, sizeof(sub_ver_path),
                 FCM_SYSFS, "cpld_sub_ver");
      } else {
        return -1;
      }
      break;
    case FRU_FPGA:
      if (!(strncmp(device, DOM_FPGA1, strlen(DOM_FPGA1)))) {
        snprintf(ver_path, sizeof(ver_path), DOMFPGA1_SYSFS, "fpga_ver");
        snprintf(sub_ver_path, sizeof(sub_ver_path),
                 DOMFPGA1_SYSFS, "fpga_sub_ver");
      } else if (!(strncmp(device, DOM_FPGA2, strlen(DOM_FPGA2)))) {
        snprintf(ver_path, sizeof(ver_path), DOMFPGA2_SYSFS, "fpga_ver");
        snprintf(sub_ver_path, sizeof(sub_ver_path),
                 DOMFPGA2_SYSFS, "fpga_sub_ver");
      } else {
        return -1;
      }
      break;
    default:
      return -1;
  }

  if (!device_read(ver_path, &val)) {
    ver[0] = (uint8_t)val;
  } else {
    return -1;
  }

  if (!device_read(sub_ver_path, &val)) {
    ver[1] = (uint8_t)val;
  } else {
    printf("[debug][ver:%s]\n", ver_path);
    printf("[debug][sub_ver:%s]\n", sub_ver_path);
    OBMC_INFO("[debug][ver:%s]\n", ver_path);
    OBMC_INFO("[debug][sub_ver:%s]\n", sub_ver_path);
    return -1;
  }

  return 0;
}

int
pal_set_last_pwr_state(uint8_t fru, char *state) {

  int ret;
  char key[MAX_KEY_LEN];

  sprintf(key, "%s", "pwr_server_last_state");

  ret = pal_set_key_value(key, state);
  if (ret < 0) {
#ifdef DEBUG
    OBMC_WARN("pal_set_last_pwr_state: pal_set_key_value failed for "
        "fru %u", fru);
#endif
  }
  return ret;
}

int
pal_get_last_pwr_state(uint8_t fru, char *state) {
  int ret;
  char key[MAX_KEY_LEN];

  sprintf(key, "%s", "pwr_server_last_state");

  ret = pal_get_key_value(key, state);
  if (ret < 0) {
#ifdef DEBUG
    OBMC_WARN("pal_get_last_pwr_state: pal_get_key_value failed for "
      "fru %u", fru);
#endif
  }
  return ret;
}

int
pal_set_com_pwr_btn_n(char *status) {
  char path[64];
  int ret;
  sprintf(path, SCM_SYSFS, COM_PWR_BTN_N);

  ret = device_write_buff(path, status);
  if (ret) {
#ifdef DEBUG
  OBMC_WARN("device_write_buff failed for %s\n", path);
#endif
    return -1;
  }

  return 0;
}

int
pal_get_num_slots(uint8_t *num)
{
  *num = MAX_NUM_SCM;
  return PAL_EOK;
}

// Power On the COM-E
static int
scm_power_on(uint8_t slot_id) {
  int ret;

  ret = run_command("/usr/local/bin/wedge_power.sh on");
  if (ret) {
    OBMC_ERROR(ret, "%s failed", __func__);
    return -1;
  }
  return 0;
}

// Power Off the COM-E
static int
scm_power_off(uint8_t slot_id) {
  int ret;

  ret = run_command("/usr/local/bin/wedge_power.sh off");
  if (ret) {
    OBMC_ERROR(ret, "%s failed", __func__);
    return -1;
  }
  return 0;
}

// Power Button trigger the server in given slot
static int
cpu_power_btn(uint8_t slot_id) {
  int ret;

  ret = pal_set_com_pwr_btn_n("0");
  if (ret)
    return -1;
  sleep(DELAY_POWER_BTN);
  ret = pal_set_com_pwr_btn_n("1");
  if (ret)
    return -1;

  return 0;
}

// set CPU power off with power button
static int
cpu_power_off(uint8_t slot_id) {
  int ret = pal_set_com_pwr_btn_n("0");//set COM_PWR_BTN_N to low
  if(ret){
    OBMC_ERROR(ret, "%s set COM-e power button low failed.\n", __func__);
    return -1;
  }
  sleep(6);
  ret = pal_set_com_pwr_btn_n("1");//set COM_PWR_BTN_N to high
  if(ret){
    OBMC_ERROR(ret, "%s set COM-e power button high failed.\n", __func__);
    return -1;
  }
  return 0;
}

int
pal_get_server_power(uint8_t slot_id, uint8_t *status) {

  int ret;
  char value[MAX_VALUE_LEN];
  bic_gpio_t gpio;
  uint8_t retry = MAX_READ_RETRY;

  /* check if the CPU is turned on or not */
  while (retry) {
    ret = bic_get_gpio(IPMB_BUS, &gpio);
    if (!ret)
      break;
    msleep(50);
    retry--;
  }
  if (ret) {
    // Check for if the BIC is irresponsive due to 12V_OFF or 12V_CYCLE
    OBMC_INFO("pal_get_server_power: bic_get_gpio returned error hence"
        " reading the kv_store for last power state  for slot %d", slot_id);
    pal_get_last_pwr_state(slot_id, value);
    if (!(strncmp(value, "off", strlen("off")))) {
      *status = SERVER_POWER_OFF;
    } else if (!(strncmp(value, "on", strlen("on")))) {
      *status = SERVER_POWER_ON;
    } else {
      return ret;
    }
    return 0;
  }

  if (gpio.pwrgood_cpu) {
    *status = SERVER_POWER_ON;
  } else {
    *status = SERVER_POWER_OFF;
  }

  return 0;
}

// Power Off, Power On, or Power Reset the server in given slot
int
pal_set_server_power(uint8_t slot_id, uint8_t cmd) {
  int ret = 0;
  uint8_t status;

  if (pal_get_server_power(slot_id, &status) < 0) {
    return -1;
  }

  switch(cmd) {
    case SERVER_POWER_ON:
      if (status == SERVER_POWER_ON)
        return 1;
      else
        return scm_power_on(slot_id);
      break;

    case SERVER_POWER_OFF:
      if (status == SERVER_POWER_OFF)
        return 1;
      else
        return scm_power_off(slot_id);
      break;

    case SERVER_POWER_CYCLE:
      if (status == SERVER_POWER_ON) {
        if (scm_power_off(slot_id))
          return -1;

        sleep(DELAY_POWER_CYCLE);

        return scm_power_on(slot_id);

      } else if (status == SERVER_POWER_OFF) {

        return (scm_power_on(slot_id));
      }
      break;

    case SERVER_POWER_RESET:
    if (status == SERVER_POWER_ON) {
        ret = pal_set_rst_btn(slot_id, 0);
        if (ret < 0) {
          OBMC_CRIT("Micro-server can't power reset");
          return ret;
        }
        msleep(100); //some server miss to detect a quick pulse, so delay 100ms between low high
        ret = pal_set_rst_btn(slot_id, 1);
        if (ret < 0) {
          OBMC_CRIT("Micro-server in reset state, can't go back to normal state");
          return ret;
        }
      } else if (status == SERVER_POWER_OFF) {
          OBMC_CRIT("Micro-server power status is off, ignore power reset action");
        return -2;
      }
      break;

    case SERVER_GRACEFUL_SHUTDOWN:
      ret = cpu_power_off(slot_id);
      break;

    case SERVER_GRACEFUL_POWER_ON:
      ret = cpu_power_btn(slot_id);
      break;

    default:
      ret = -1;
      break;
  }

  return ret;
}

static bool
is_server_on(void) {
  int ret;
  uint8_t status;
  ret = pal_get_server_power(FRU_SCM, &status);
  if (ret) {
    return false;
  }

  if (status == SERVER_POWER_ON) {
    return true;
  } else {
    return false;
  }
}

int
pal_set_th3_power(int option) {
  char path[256];
  int ret;
  uint8_t brd_type;
  char sysfs[128];
  if(pal_get_board_type(&brd_type)){
    return -1;
  }

  if(brd_type == BRD_TYPE_WEDGE400){
    sprintf(sysfs,TH3_POWER);
  }else if(brd_type == BRD_TYPE_WEDGE400C){
    sprintf(sysfs,GB_POWER);
  }else{
    return -1;
  }

  switch(option) {
    case TH3_POWER_ON:
      sprintf(path, SMB_SYSFS, sysfs);
      ret = device_write_buff(path, "1");
      break;
    case TH3_POWER_OFF:
      sprintf(path, SMB_SYSFS, sysfs);
      ret = device_write_buff(path, "0");
      break;
    case TH3_RESET:
      sprintf(path, SMB_SYSFS, sysfs);
      ret = device_write_buff(path, "0");
      sleep(1);
      ret = device_write_buff(path, "1");
      break;
    default:
      ret = -1;
  }
  if (ret)
    return -1;
  return 0;
}

static int check_dir_exist(const char *device) {
  char cmd[LARGEST_DEVICE_NAME + 1];
  FILE *fp;
  char cmd_ret;
  int ret=-1;

  // Check if device exists
  snprintf(cmd, LARGEST_DEVICE_NAME, "[ -e %s ];echo $?", device);
  fp = popen(cmd, "r");
  if(NULL == fp)
     return -1;

  cmd_ret = fgetc(fp);
  ret = pclose(fp);
  if(-1 == ret)
    return -1;
  if('0' != cmd_ret) {
    return -1;
  }

  return 0;
}

static int
get_current_dir(const char *device, char *dir_name) {
  char cmd[LARGEST_DEVICE_NAME + 1];
  FILE *fp;
  int ret=-1;
  int size;

  // Get current working directory
  snprintf(
      cmd, LARGEST_DEVICE_NAME, "cd %s;pwd", device);

  fp = popen(cmd, "r");
  if(NULL == fp)
     return -1;
  if (fgets(dir_name, LARGEST_DEVICE_NAME, fp) == NULL) {
    pclose(fp);
    return -1;
  }

  ret = pclose(fp);
  if(-1 == ret)
     OBMC_ERROR(-1, "%s pclose() fail ", __func__);

  // Remove the newline character at the end
  size = strlen(dir_name);
  dir_name[size-1] = '\0';

  return 0;
}

static int
read_attr_integer(const char *device, const char *attr, int *value) {
  char full_name[LARGEST_DEVICE_NAME * 2];
  char dir_name[LARGEST_DEVICE_NAME + 1];

  // Get current working directory
  if (get_current_dir(device, dir_name)) {
    return -1;
  }

  snprintf(
      full_name, sizeof(full_name), "%s/%s", dir_name, attr);

  if (device_read(full_name, value)) {
    return -1;
  }

  return 0;
}

static int
read_attr(const char *device, const char *attr, float *value) {
  char full_name[LARGEST_DEVICE_NAME * 2];
  char dir_name[LARGEST_DEVICE_NAME + 1];
  int tmp;

  // Get current working directory
  if (get_current_dir(device, dir_name)) {
    return -1;
  }

  snprintf(
      full_name, sizeof(full_name), "%s/%s", dir_name, attr);

  if (device_read(full_name, &tmp)) {
    return -1;
  }

  *value = ((float)tmp)/UNIT_DIV;

  return 0;
}

static int
read_hsc_attr(const char *device,
              const char* attr, float r_sense, float *value) {
  char full_dir_name[LARGEST_DEVICE_NAME * 2];
  char dir_name[LARGEST_DEVICE_NAME + 1];
  int tmp;

  // Get current working directory
  if (get_current_dir(device, dir_name))
  {
    return -1;
  }
  snprintf(
      full_dir_name, sizeof(full_dir_name), "%s/%s", dir_name, attr);

  if (device_read(full_dir_name, &tmp)) {
    return -1;
  }

  *value = ((float)tmp)/r_sense/UNIT_DIV;

  return 0;
}

static int
read_hsc_volt(const char *device, float r_sense, float *value) {
  return read_hsc_attr(device, VOLT(1), r_sense, value);
}

static int
read_hsc_curr(const char *device, float r_sense, float *value) {
  return read_hsc_attr(device, CURR(1), r_sense, value);
}

static int
read_fan_rpm_f(const char *device, uint8_t fan, float *value) {
  char full_name[LARGEST_DEVICE_NAME * 2];
  char dir_name[LARGEST_DEVICE_NAME + 1];
  char device_name[11];
  int tmp;

  /* Get current working directory */
  if (get_current_dir(device, dir_name)) {
    return -1;
  }

  snprintf(device_name, 11, "fan%d_input", fan);
  snprintf(full_name, sizeof(full_name), "%s/%s", dir_name, device_name);
  if (device_read(full_name, &tmp)) {
    return -1;
  }

  *value = (float)tmp;

  return 0;
}

static int
read_fan_rpm(const char *device, uint8_t fan, int *value) {
  char full_name[LARGEST_DEVICE_NAME * 2];
  char dir_name[LARGEST_DEVICE_NAME + 1];
  char device_name[11];
  int tmp;

  /* Get current working directory */
  if (get_current_dir(device, dir_name)) {
    return -1;
  }

  snprintf(device_name, 11, "fan%d_input", fan);
  snprintf(full_name, sizeof(full_name), "%s/%s", dir_name, device_name);
  if (device_read(full_name, &tmp)) {
    return -1;
  }

  *value = tmp;

  return 0;
}

int
pal_get_fan_speed(uint8_t fan, int *rpm) {
  if (fan >= MAX_NUM_FAN * 2) {
    OBMC_INFO("get_fan_speed: invalid fan#:%d", fan);
    return -1;
  }

  return read_fan_rpm(SMB_FCM_TACH_DEVICE, (fan + 1), rpm);
}

static int
bic_sensor_sdr_path(uint8_t fru, char *path) {

  char fru_name[16];

  switch(fru) {
    case FRU_SCM:
      sprintf(fru_name, "%s", "scm");
      break;
    default:
  #ifdef DEBUG
      OBMC_WARN("bic_sensor_sdr_path: Wrong Slot ID\n");
  #endif
      return -1;
  }

  sprintf(path, WEDGE400_SDR_PATH, fru_name);

  if (access(path, F_OK) == -1) {
    return -1;
  }

  return 0;
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
    OBMC_ERROR(fd, "%s: open failed for %s\n", __func__, path);
    return -1;
  }

  ret = pal_flock_flag_retry(fd, LOCK_SH | LOCK_NB);
  if (ret == -1) {
   OBMC_WARN("%s: failed to flock flag: %d on %s", __func__,
              LOCK_SH | LOCK_NB, path);
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

static int
bic_sensor_sdr_init(uint8_t fru, sensor_info_t *sinfo) {
  char path[64];
  int retry = 0;

  switch(fru) {
    case FRU_SCM:
      if (bic_sensor_sdr_path(fru, path) < 0) {
#ifdef DEBUG
        OBMC_WARN(
               "bic_sensor_sdr_path: get_fru_sdr_path failed\n");
#endif
        return ERR_NOT_READY;
      }
      while (retry <= 3) {
        if (sdr_init(path, sinfo) < 0) {
          if (retry == 3) { //if the third retry still failed, return -1
#ifdef DEBUG
            OBMC_ERROR(-1, "bic_sensor_sdr_init: sdr_init failed for FRU %d", fru);
#endif
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

static int
bic_sdr_init(uint8_t fru, bool reinit) {

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
static int
bic_get_sdr_thresh_val(uint8_t fru, uint8_t snr_num,
                       uint8_t thresh, void *value) {
  int ret, retry = 0;
  int8_t b_exp, r_exp;
  uint8_t x, m_lsb, m_msb, b_lsb, b_msb, thresh_val;
  uint16_t m = 0, b = 0;
  sensor_info_t sinfo[MAX_SENSOR_NUM + 1] = {0};
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
#ifdef DEBUG
      OBMC_ERROR(-1, "bic_get_sdr_thresh_val: reading unknown threshold val");
#endif
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

static int
bic_read_sensor_wrapper(uint8_t fru, uint8_t sensor_num, bool discrete,
    void *value) {

  int ret, i;
  ipmi_sensor_reading_t sensor;
  sdr_full_t *sdr;

  ret = bic_read_sensor(IPMB_BUS, sensor_num, &sensor);
  if (ret) {
    return ret;
  }

  if (sensor.flags & BIC_SENSOR_READ_NA) {
#ifdef DEBUG
    OBMC_ERROR(-1, "bic_read_sensor_wrapper: Reading Not Available");
    OBMC_ERROR(-1, "bic_read_sensor_wrapper: sensor_num: 0x%X, flag: 0x%X",
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

static void
hsc_rsense_init(uint8_t hsc_id, const char* device) {
  static bool rsense_inited[MAX_NUM_FRUS] = {false};

  if (!rsense_inited[hsc_id]) {
    int brd_rev = -1;

    pal_get_cpld_board_rev(&brd_rev, device);
    /* R0D or R0E FCM */
    if (brd_rev == 0x4 || brd_rev == 0x5) {
      hsc_rsense[hsc_id] = 1;
    } else {
      hsc_rsense[hsc_id] = 1;
    }

    rsense_inited[hsc_id] = true;
  }
}

static int
scm_sensor_read(uint8_t sensor_num, float *value) {

  int ret = -1;
  int i = 0;
  int j = 0;
  bool discrete = false;
  bool scm_sensor = false;
  int scm_sensor_cnt = 0;
  uint8_t *scm_sensor_list;
  uint8_t brd_type;
  uint8_t brd_type_rev;
  if(pal_get_board_type(&brd_type)){
    return -1;
  }
  if(pal_get_board_type_rev(&brd_type_rev)){
    return -1;
  }

  if ( brd_type == BRD_TYPE_WEDGE400 &&
           brd_type_rev >= BOARD_WEDGE400_MP_RESPIN ) {
    scm_sensor_cnt = scm_mp_respin_sensor_cnt;
    scm_sensor_list = (uint8_t *) scm_mp_respin_sensor_list;
  } else {
    scm_sensor_cnt = scm_evt_dvt_mp_sensor_cnt;
    scm_sensor_list = (uint8_t *) scm_evt_dvt_mp_sensor_list;
  }

  while (i < scm_sensor_cnt) {
    if (sensor_num == scm_sensor_list[i++]) {
      scm_sensor = true;
      break;
    }
  }
  if (scm_sensor) {
    switch(sensor_num) {
      case SCM_SENSOR_OUTLET_TEMP:
        ret = read_attr(SCM_OUTLET_TEMP_DEVICE, TEMP(1), value);
        break;
      case SCM_SENSOR_INLET_TEMP:
        ret = read_attr(SCM_INLET_TEMP_DEVICE, TEMP(1), value);
        break;
      case SCM_SENSOR_HSC_VOLT:
        if (!check_dir_exist(SCM_HSC_ADM1278_DIR)) {
          ret = read_hsc_volt(SCM_HSC_ADM1278_DEVICE, 1, value);
        } else if (!check_dir_exist(SCM_HSC_LM25066_DIR)) {
          ret = read_hsc_volt(SCM_HSC_LM25066_DEVICE, 1, value);
        } else {
          ret = READING_NA;
          break;
        }
        break;
      case SCM_SENSOR_HSC_CURR:
        if (!check_dir_exist(SCM_HSC_ADM1278_DIR)) {
          ret = read_hsc_curr(SCM_HSC_ADM1278_DEVICE, SCM_RSENSE, value);
        } else if (!check_dir_exist(SCM_HSC_LM25066_DIR)) {
          ret = read_hsc_curr(SCM_HSC_LM25066_DEVICE, SCM_RSENSE, value);
        } else {
          ret = READING_NA;
          break;
        }
        *value = *value * 1.0036 - 0.1189;
        if (*value < 0)
          *value = 0;
        break;
      default:
        ret = READING_NA;
        break;
    }
  } else if (!scm_sensor && is_server_on()) {
    ret = bic_sdr_init(FRU_SCM, false);
    if (ret < 0) {
#ifdef DEBUG
    OBMC_INFO("bic_sdr_init fail\n");
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

    ret = bic_read_sensor_wrapper(FRU_SCM, sensor_num, discrete, value);
  } else {
    ret = READING_NA;
  }
  return ret;
}

static int
cor_th3_volt(uint8_t board_type) {
  /*
   * Currently, skip to set vdd core with sysfs nodes because sysfs node exposed
   * has no write permission. Laterly we will fix this.
   */
  return 0;
  int tmp_volt, i;
  int val_volt = 0;
  char str[32];
  char tmp[LARGEST_DEVICE_NAME + 1];
  char path[LARGEST_DEVICE_NAME + 1];
  int rov_cnt = (board_type == BRD_TYPE_WEDGE400) ? SMB_CPLD_TH3_ROV_NUM : SMB_CPLD_GB_ROV_NUM;
  snprintf(tmp, LARGEST_DEVICE_NAME, SMB_SYSFS,
           (board_type == BRD_TYPE_WEDGE400) ? SMB_CPLD_TH3_ROV : SMB_CPLD_GB_ROV);

  for(i = rov_cnt - 1; i >= 0; i--) {
    memset(path, 0, sizeof(path));
    snprintf(path, LARGEST_DEVICE_NAME, tmp, i);
    if(device_read(path, &tmp_volt)) {
      OBMC_ERROR(-1, "%s, Cannot read %s of th3/gb voltage from smbcpld\n", __func__, path);
      return -1;
    }
    val_volt += tmp_volt;
    if(i)
      val_volt = (val_volt << 1);
  }
  val_volt = (int)(round((1.6 - (((double)val_volt - 3) * 0.00625)) * 1000));

  if(val_volt > TH3_VOL_MAX || val_volt < TH3_VOL_MIN)
    return -1;

  snprintf(str, sizeof(str), "%d", val_volt);
  snprintf(path, LARGEST_DEVICE_NAME, SMB_ISL_DEVICE"/%s", VOLT_SET(3));
  if(device_write_buff(path, str)) {
    OBMC_ERROR(-1, "%s, Cannot write th3/gb voltage into ISL68127\n", __func__);
    return -1;
  }

  return 0;
}

static int
fan_sensor_read(uint8_t sensor_num, float *value) {
  int ret = -1;

  switch(sensor_num) {
    case FAN_SENSOR_FAN1_FRONT_TACH:
      ret = read_fan_rpm_f(SMB_FCM_TACH_DEVICE, 1, value);
      break;
    case FAN_SENSOR_FAN1_REAR_TACH:
      ret = read_fan_rpm_f(SMB_FCM_TACH_DEVICE, 2, value);
      break;
    case FAN_SENSOR_FAN2_FRONT_TACH:
      ret = read_fan_rpm_f(SMB_FCM_TACH_DEVICE, 3, value);
      break;
    case FAN_SENSOR_FAN2_REAR_TACH:
      ret = read_fan_rpm_f(SMB_FCM_TACH_DEVICE, 4, value);
      break;
    case FAN_SENSOR_FAN3_FRONT_TACH:
      ret = read_fan_rpm_f(SMB_FCM_TACH_DEVICE, 5, value);
      break;
    case FAN_SENSOR_FAN3_REAR_TACH:
      ret = read_fan_rpm_f(SMB_FCM_TACH_DEVICE, 6, value);
      break;
    case FAN_SENSOR_FAN4_FRONT_TACH:
      ret = read_fan_rpm_f(SMB_FCM_TACH_DEVICE, 7, value);
      break;
    case FAN_SENSOR_FAN4_REAR_TACH:
      ret = read_fan_rpm_f(SMB_FCM_TACH_DEVICE, 8, value);
      break;
    default:
      ret = -1;
      break;
  }

  return ret;
}

static int
smb_sensor_read(uint8_t sensor_num, float *value) {

  int ret = -1, th3_ret = -1;
  static uint8_t bootup_check = 0;
  uint8_t brd_type;
  uint8_t brd_type_rev;
  pal_get_board_type(&brd_type);
  pal_get_board_type_rev(&brd_type_rev);

  switch(sensor_num) {
    case SMB_SENSOR_SW_SERDES_PVDD_TEMP1:
      ret = read_attr(SMB_SW_SERDES_PVDD_DEVICE, TEMP(2), value);
      break;
    case SMB_SENSOR_SW_SERDES_TRVDD_TEMP1:
      ret = read_attr(SMB_SW_SERDES_TRVDD_DEVICE, TEMP(2), value);
      break;
    case SMB_SENSOR_IR3R3V_RIGHT_TEMP:
      ret = read_attr(SMB_SW_SERDES_TRVDD_DEVICE, TEMP(1), value);
      break;
    case SMB_SENSOR_IR3R3V_LEFT_TEMP:
      ret = read_attr(SMB_SW_SERDES_PVDD_DEVICE, TEMP(1), value);
      break;
    case SMB_SENSOR_SW_CORE_TEMP1:
      ret = read_attr(SMB_ISL_DEVICE, TEMP(2), value);
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
    case SMB_SENSOR_TMP421_U62_TEMP:
      if ( brd_type == BRD_TYPE_WEDGE400 &&
           brd_type_rev >= BOARD_WEDGE400_MP_RESPIN ) {
        if ( !check_dir_exist(SMB_TMP421_1_DIR) ) {
          ret = read_attr(SMB_TMP421_1_DEVICE, TEMP(1), value);
        } else if( !check_dir_exist(SMB_ADM1032_1_DIR) ) {
          ret = read_attr(SMB_ADM1032_1_DEVICE, TEMP(1), value);
        } else {
          ret = READING_NA;
        }
      } else {
        ret = read_attr(SMB_TMP421_U62_DEVICE, TEMP(1), value);
      }
      break;
    case SMB_SENSOR_TMP421_Q9_TEMP:
      if ( brd_type == BRD_TYPE_WEDGE400 &&
           brd_type_rev >= BOARD_WEDGE400_MP_RESPIN ) {
        if ( !check_dir_exist(SMB_TMP421_1_DIR) ) {
          ret = read_attr(SMB_TMP421_1_DEVICE, TEMP(2), value);
        } else if( !check_dir_exist(SMB_ADM1032_1_DIR) ) {
          ret = read_attr(SMB_ADM1032_1_DEVICE, TEMP(2), value);
        } else {
          ret = READING_NA;
        }
      } else {
        ret = read_attr(SMB_TMP421_U62_DEVICE, TEMP(2), value);
      }
      break;
    case SMB_SENSOR_TMP421_U63_TEMP:
      if ( brd_type == BRD_TYPE_WEDGE400 &&
           brd_type_rev >= BOARD_WEDGE400_MP_RESPIN ) {
        if ( !check_dir_exist(SMB_TMP421_2_DIR) ) {
          ret = read_attr(SMB_TMP421_2_DEVICE, TEMP(1), value);
        } else if( !check_dir_exist(SMB_ADM1032_2_DIR) ) {
          ret = read_attr(SMB_ADM1032_2_DEVICE, TEMP(1), value);
        } else {
          ret = READING_NA;
        }
      } else {
        ret = read_attr(SMB_TMP421_U63_DEVICE, TEMP(1), value);
      }
      break;
    case SMB_SENSOR_TMP421_Q10_TEMP:
      if ( brd_type == BRD_TYPE_WEDGE400 &&
           brd_type_rev >= BOARD_WEDGE400_MP_RESPIN ) {
        if( !check_dir_exist(SMB_TMP421_2_DIR) ) {
          ret = read_attr(SMB_TMP421_2_DEVICE, TEMP(2), value);
        } else if( !check_dir_exist(SMB_ADM1032_2_DIR) ) {
          ret = read_attr(SMB_ADM1032_2_DEVICE, TEMP(2), value);
        } else {
          ret = READING_NA;
        }
      } else {
        ret = read_attr(SMB_TMP421_U63_DEVICE, TEMP(2), value);
      }
      break;
    case SMB_SENSOR_TMP422_U20_TEMP:
      if( brd_type == BRD_TYPE_WEDGE400 ){
        ret = read_attr(SMB_SW_TEMP_DEVICE, TEMP(1), value);
      }else{
        ret = READING_NA;
      }
      break;
    case SMB_SENSOR_SW_DIE_TEMP1:
      if( brd_type == BRD_TYPE_WEDGE400 ){
        if ( brd_type_rev >= BOARD_WEDGE400_MP_RESPIN ) {
          if( !check_dir_exist(SMB_TMP421_3_DIR) ) {
            ret = read_attr(SMB_TMP421_3_DEVICE, TEMP(2), value);
          } else if( !check_dir_exist(SMB_ADM1032_3_DIR) ) {
            ret = read_attr(SMB_ADM1032_3_DEVICE, TEMP(2), value);
          } else {
            ret = READING_NA;
          }
        } else {
          ret = read_attr(SMB_SW_TEMP_DEVICE, TEMP(2), value);
        }
      }
      if( brd_type == BRD_TYPE_WEDGE400C ){ // Get Highest
        char path[32];
        int num = 0;
        float read_val = 0;
        for ( int id=1;id<=10;id++ ){
          sprintf(path,"temp%d_input",id);
          ret = read_attr(SMB_GB_TEMP_DEVICE, path, &read_val);
          if(ret){
            continue;
          }else{
            num++;
          }
          if(read_val > *value){
            *value = read_val;
          }
        }
        if(num){
          ret = 0;
        }else{
          ret = READING_NA;
        }
      }
      break;
    case SMB_SENSOR_SW_DIE_TEMP2:
      if( brd_type == BRD_TYPE_WEDGE400 ){
        if ( brd_type_rev >= BOARD_WEDGE400_MP_RESPIN ) {
          if( !check_dir_exist(SMB_TMP421_4_DIR) ) {
            ret = read_attr(SMB_TMP421_4_DEVICE, TEMP(2), value);
          } else if( !check_dir_exist(SMB_ADM1032_4_DIR) ) {
            ret = read_attr(SMB_ADM1032_4_DEVICE, TEMP(2), value);
          } else {
            ret = READING_NA;
          }
        } else {
          ret = read_attr(SMB_SW_TEMP_DEVICE, TEMP(3), value);
        }
      }
      if( brd_type == BRD_TYPE_WEDGE400C ){ // Average
        int num = 0;
        char path[32];
        float total = 0;
        float read_val = 0;
        for ( int id=1;id<=10;id++ ){
          sprintf(path,"temp%d_input",id);
          ret = read_attr(SMB_GB_TEMP_DEVICE, path, &read_val);
          if(ret){
            continue;
          }else{
            num++;
          }
          total += read_val;
        }
        if(num){
          *value = total/num;
          ret = 0;
        }else{
          ret = READING_NA;
        }
      }
      break;
    case SMB_SENSOR_GB_TEMP1:
      if( brd_type == BRD_TYPE_WEDGE400C ){
        ret = read_attr(SMB_GB_TEMP_DEVICE, TEMP(1), value);
      }
      break;
    case SMB_SENSOR_GB_TEMP2:
      if( brd_type == BRD_TYPE_WEDGE400C ){
        ret = read_attr(SMB_GB_TEMP_DEVICE, TEMP(2), value);
      }
      break;
    case SMB_SENSOR_GB_TEMP3:
      if( brd_type == BRD_TYPE_WEDGE400C ){
        ret = read_attr(SMB_GB_TEMP_DEVICE, TEMP(3), value);
      }
      break;
    case SMB_SENSOR_GB_TEMP4:
      if( brd_type == BRD_TYPE_WEDGE400C ){
        ret = read_attr(SMB_GB_TEMP_DEVICE, TEMP(4), value);
      }
      break;
    case SMB_SENSOR_GB_TEMP5:
      if( brd_type == BRD_TYPE_WEDGE400C ){
        ret = read_attr(SMB_GB_TEMP_DEVICE, TEMP(5), value);
      }
      break;
    case SMB_SENSOR_GB_TEMP6:
      if( brd_type == BRD_TYPE_WEDGE400C ){
        ret = read_attr(SMB_GB_TEMP_DEVICE, TEMP(6), value);
      }
      break;
    case SMB_SENSOR_GB_TEMP7:
      if( brd_type == BRD_TYPE_WEDGE400C ){
        ret = read_attr(SMB_GB_TEMP_DEVICE, TEMP(7), value);
      }
      break;
    case SMB_SENSOR_GB_TEMP8:
      if( brd_type == BRD_TYPE_WEDGE400C ){
        ret = read_attr(SMB_GB_TEMP_DEVICE, TEMP(8), value);
      }
      break;
    case SMB_SENSOR_GB_TEMP9:
      if( brd_type == BRD_TYPE_WEDGE400C ){
        ret = read_attr(SMB_GB_TEMP_DEVICE, TEMP(9), value);
      }
      break;
    case SMB_SENSOR_GB_TEMP10:
      if( brd_type == BRD_TYPE_WEDGE400C ){
        ret = read_attr(SMB_GB_TEMP_DEVICE, TEMP(10), value);
      }
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
    case SMB_SENSOR_SW_SERDES_PVDD_IN_VOLT:
      ret = read_attr(SMB_SW_SERDES_PVDD_DEVICE, VOLT(2), value);
      break;
    case SMB_SENSOR_SW_SERDES_TRVDD_IN_VOLT:
      ret = read_attr(SMB_SW_SERDES_TRVDD_DEVICE, VOLT(2), value);
      break;
    case SMB_SENSOR_SW_SERDES_PVDD_OUT_VOLT:
      ret = read_attr(SMB_SW_SERDES_PVDD_DEVICE, VOLT(4), value);
      break;
    case SMB_SENSOR_SW_SERDES_TRVDD_OUT_VOLT:
      ret = read_attr(SMB_SW_SERDES_TRVDD_DEVICE, VOLT(4), value);
      break;
    case SMB_SENSOR_IR3R3V_RIGHT_IN_VOLT:
      ret = read_attr(SMB_SW_SERDES_TRVDD_DEVICE, VOLT(1), value);
      break;
    case SMB_SENSOR_IR3R3V_LEFT_IN_VOLT:
      ret = read_attr(SMB_SW_SERDES_PVDD_DEVICE, VOLT(1), value);
      break;
    case SMB_SENSOR_IR3R3V_RIGHT_OUT_VOLT:
      ret = read_attr(SMB_SW_SERDES_TRVDD_DEVICE, VOLT(3), value);
      *value *= 2;
      break;
    case SMB_SENSOR_IR3R3V_LEFT_OUT_VOLT:
      ret = read_attr(SMB_SW_SERDES_PVDD_DEVICE, VOLT(3), value);
      *value *= 2;
      break;
    case SMB_SENSOR_SW_CORE_VOLT:
      if( brd_type == BRD_TYPE_WEDGE400 ){
        ret = read_attr(SMB_ISL_DEVICE, VOLT(3), value);
        int board_rev = -1;
        if((pal_get_board_rev(&board_rev) != -1)) {
          if (bootup_check == 0) {
            th3_ret = cor_th3_volt(brd_type);
            if (!th3_ret)
              bootup_check = 1;
          }
        }
      }
      else if( brd_type == BRD_TYPE_WEDGE400C ){
        ret = read_attr(SMB_XPDE_DEVICE, VOLT(3), value);
      }
      break;
    case SMB_SENSOR_FCM_HSC_VOLT:
      if (!check_dir_exist(SMB_FCM_HSC_ADM1278_DIR)) {
        ret = read_hsc_volt(SMB_FCM_HSC_ADM1278_DEVICE, 1, value);
      } else if (!check_dir_exist(SMB_FCM_HSC_LM25066_DIR)) {
        ret = read_hsc_volt(SMB_FCM_HSC_LM25066_DEVICE, 1, value);
      } else {
        ret = READING_NA;
      }
      break;
    case SMB_SENSOR_SW_SERDES_PVDD_IN_CURR:
      ret = read_attr(SMB_SW_SERDES_PVDD_DEVICE, CURR(2), value);
      break;
    case SMB_SENSOR_SW_SERDES_TRVDD_IN_CURR:
      ret = read_attr(SMB_SW_SERDES_TRVDD_DEVICE, CURR(2), value);
      break;
    case SMB_SENSOR_SW_SERDES_PVDD_OUT_CURR:
      ret = read_attr(SMB_SW_SERDES_PVDD_DEVICE, CURR(4), value);
      break;
    case SMB_SENSOR_SW_SERDES_TRVDD_OUT_CURR:
      ret = read_attr(SMB_SW_SERDES_TRVDD_DEVICE, CURR(4), value);
      break;
    case SMB_SENSOR_IR3R3V_RIGHT_IN_CURR:
      ret = read_attr(SMB_SW_SERDES_TRVDD_DEVICE, CURR(1), value);
      break;
    case SMB_SENSOR_IR3R3V_LEFT_IN_CURR:
      ret = read_attr(SMB_SW_SERDES_PVDD_DEVICE, CURR(1), value);
      break;
    case SMB_SENSOR_IR3R3V_RIGHT_OUT_CURR:
      ret = read_attr(SMB_SW_SERDES_TRVDD_DEVICE, CURR(3), value);
      break;
    case SMB_SENSOR_IR3R3V_LEFT_OUT_CURR:
      ret = read_attr(SMB_SW_SERDES_PVDD_DEVICE, CURR(3), value);
      break;
    case SMB_SENSOR_SW_CORE_CURR:
      if( brd_type == BRD_TYPE_WEDGE400 ){
        ret = read_attr(SMB_ISL_DEVICE,  CURR(3), value);
      }
      else if( brd_type == BRD_TYPE_WEDGE400C ){
        ret = read_attr(SMB_XPDE_DEVICE, CURR(3), value);
      }
      break;
    case SMB_SENSOR_FCM_HSC_CURR:
      hsc_rsense_init(HSC_FCM, FCM_SYSFS);
      if (!check_dir_exist(SMB_FCM_HSC_ADM1278_DIR)) {
        ret = read_hsc_curr(SMB_FCM_HSC_ADM1278_DEVICE, hsc_rsense[HSC_FCM], value);
      } else if (!check_dir_exist(SMB_FCM_HSC_LM25066_DIR)) {
        ret = read_hsc_curr(SMB_FCM_HSC_LM25066_DEVICE, hsc_rsense[HSC_FCM], value);
      } else {
        ret = READING_NA;
      }
      *value = *value * 4.4254 - 0.2048;
      if (*value < 0)
        *value = 0;
      break;
    case SMB_SENSOR_SW_SERDES_PVDD_IN_POWER:
      ret = read_attr(SMB_SW_SERDES_PVDD_DEVICE,  POWER(2), value);
      *value /= 1000;
      break;
    case SMB_SENSOR_SW_SERDES_TRVDD_IN_POWER:
      ret = read_attr(SMB_SW_SERDES_TRVDD_DEVICE,  POWER(2), value);
      *value /= 1000;
      break;
    case SMB_SENSOR_SW_SERDES_PVDD_OUT_POWER:
      ret = read_attr(SMB_SW_SERDES_PVDD_DEVICE,  POWER(4), value);
      *value /= 1000;
      break;
    case SMB_SENSOR_SW_SERDES_TRVDD_OUT_POWER:
      ret = read_attr(SMB_SW_SERDES_TRVDD_DEVICE,  POWER(4), value);
      *value /= 1000;
      break;
    case SMB_SENSOR_IR3R3V_RIGHT_IN_POWER:
      ret = read_attr(SMB_SW_SERDES_TRVDD_DEVICE,  POWER(1), value);
      *value = *value / 1000;
      break;
    case SMB_SENSOR_IR3R3V_LEFT_IN_POWER:
      ret = read_attr(SMB_SW_SERDES_PVDD_DEVICE,  POWER(1), value);
      *value = *value / 1000;
      break;
    case SMB_SENSOR_IR3R3V_RIGHT_OUT_POWER:
      ret = read_attr(SMB_SW_SERDES_TRVDD_DEVICE,  POWER(3), value);
      *value = *value / 1000 * 2;
      break;
    case SMB_SENSOR_IR3R3V_LEFT_OUT_POWER:
      ret = read_attr(SMB_SW_SERDES_PVDD_DEVICE,  POWER(3), value);
      *value = *value / 1000 * 2;
      break;
    case SMB_SENSOR_SW_CORE_POWER:
      if( brd_type == BRD_TYPE_WEDGE400 ){
        ret = read_attr(SMB_ISL_DEVICE,  POWER(3), value);
      }
      else if( brd_type == BRD_TYPE_WEDGE400C ){
        ret = read_attr(SMB_XPDE_DEVICE, POWER(3), value);
      }
      *value /= 1000;
      break;
    case SMB_BMC_ADC0_VSEN:
      ret = read_attr(AST_ADC_DEVICE, VOLT(1), value);
      break;
    case SMB_BMC_ADC1_VSEN:
      ret = read_attr(AST_ADC_DEVICE, VOLT(2), value);
      /* Formulas of this sensor comes from resister values
       * R1 = 3.3k//52k unit: ohm (parallel connection)
       * R2 = 3.3k unit: ohm
       * in2: 2.06=(R1+R2)/R1 
       */
      *value = *value * 2.06;
      break;
    case SMB_BMC_ADC2_VSEN:
      ret = read_attr(AST_ADC_DEVICE, VOLT(3), value);
      if( brd_type == BRD_TYPE_WEDGE400 ){
        *value = *value * 3.2;
      }
      break;
    case SMB_BMC_ADC3_VSEN:
      ret = read_attr(AST_ADC_DEVICE, VOLT(4), value);
      *value = *value * 3.2;
      break;
    case SMB_BMC_ADC4_VSEN:
      ret = read_attr(AST_ADC_DEVICE, VOLT(5), value);
      *value = *value * 3.2;
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
      break;
    case SMB_BMC_ADC8_VSEN:
      ret = read_attr(AST_ADC_DEVICE, VOLT(9), value);
      break;
    case SMB_SENSOR_HBM_IN_VOLT:
      if( brd_type_rev == BOARD_WEDGE400C_EVT ){
        ret = read_attr(SMB_IR_HMB_DEVICE, VOLT(1), value);
      }else if( brd_type_rev == BOARD_WEDGE400C_EVT2 ||
                brd_type_rev == BOARD_WEDGE400C_DVT ||
                brd_type_rev == BOARD_WEDGE400C_DVT2 ||
                brd_type_rev == BOARD_WEDGE400C_MP_RESPIN ){
        ret = read_attr(SMB_PXE1211_DEVICE, VOLT(1), value);
      }
      break;
    case SMB_SENSOR_HBM_OUT_VOLT:
      if( brd_type_rev == BOARD_WEDGE400C_EVT ){
        ret = read_attr(SMB_IR_HMB_DEVICE, VOLT(3), value);
      }else if( brd_type_rev == BOARD_WEDGE400C_EVT2 ||
                brd_type_rev == BOARD_WEDGE400C_DVT ||
                brd_type_rev == BOARD_WEDGE400C_DVT2 ||
                brd_type_rev == BOARD_WEDGE400C_MP_RESPIN ){
        ret = read_attr(SMB_PXE1211_DEVICE, VOLT(4), value);
      }
      break;
    case SMB_SENSOR_HBM_IN_CURR:
      if( brd_type_rev == BOARD_WEDGE400C_EVT ){
        ret = read_attr(SMB_IR_HMB_DEVICE, CURR(1), value);
      }
      break;
    case SMB_SENSOR_HBM_OUT_CURR:
      if( brd_type_rev == BOARD_WEDGE400C_EVT ){
        ret = read_attr(SMB_IR_HMB_DEVICE, CURR(3), value);
      }else if( brd_type_rev == BOARD_WEDGE400C_EVT2 ||
                brd_type_rev == BOARD_WEDGE400C_DVT ||
                brd_type_rev == BOARD_WEDGE400C_DVT2 ||
                brd_type_rev == BOARD_WEDGE400C_MP_RESPIN ){
        ret = read_attr(SMB_PXE1211_DEVICE, CURR(4), value);
      }
      break;
    case SMB_SENSOR_HBM_IN_POWER:
      if( brd_type_rev == BOARD_WEDGE400C_EVT ){
        ret = read_attr(SMB_IR_HMB_DEVICE, POWER(1), value);
      }
      *value /= 1000;
      break;
    case SMB_SENSOR_HBM_OUT_POWER:
      if( brd_type_rev == BOARD_WEDGE400C_EVT ){
        ret = read_attr(SMB_IR_HMB_DEVICE, POWER(3), value);
      }else if( brd_type_rev == BOARD_WEDGE400C_EVT2 ||
                brd_type_rev == BOARD_WEDGE400C_DVT ||
                brd_type_rev == BOARD_WEDGE400C_DVT2 ||
                brd_type_rev == BOARD_WEDGE400C_MP_RESPIN ){
        ret = read_attr(SMB_PXE1211_DEVICE, POWER(4), value);
      }
      *value /= 1000;
      break;
    case SMB_SENSOR_HBM_TEMP:
      if( brd_type_rev == BOARD_WEDGE400C_EVT ){
        ret = read_attr(SMB_IR_HMB_DEVICE, TEMP(1), value);
      }else if( brd_type_rev == BOARD_WEDGE400C_EVT2 ||
                brd_type_rev == BOARD_WEDGE400C_DVT ||
                brd_type_rev == BOARD_WEDGE400C_DVT2 ||
                brd_type_rev == BOARD_WEDGE400C_MP_RESPIN ){
        ret = read_attr(SMB_PXE1211_DEVICE, TEMP(1), value);
      }
      break;
    case SMB_SENSOR_VDDCK_0_IN_VOLT:
      if( brd_type_rev == BOARD_WEDGE400C_EVT ){
        ret = read_attr(SMB_IR_HMB_DEVICE, VOLT(2), value);
      }else if( brd_type_rev == BOARD_WEDGE400C_EVT2 ||
                brd_type_rev == BOARD_WEDGE400C_DVT ||
                brd_type_rev == BOARD_WEDGE400C_DVT2 ||
                brd_type_rev == BOARD_WEDGE400C_MP_RESPIN ){
        ret = read_attr(SMB_PXE1211_DEVICE, VOLT(2), value);
      }
      break;
    case SMB_SENSOR_VDDCK_0_OUT_VOLT:
      if( brd_type_rev == BOARD_WEDGE400C_EVT ){
        ret = read_attr(SMB_IR_HMB_DEVICE, VOLT(4), value);
      }else if( brd_type_rev == BOARD_WEDGE400C_EVT2 ||
                brd_type_rev == BOARD_WEDGE400C_DVT ||
                brd_type_rev == BOARD_WEDGE400C_DVT2 ||
                brd_type_rev == BOARD_WEDGE400C_MP_RESPIN ){
        ret = read_attr(SMB_PXE1211_DEVICE, VOLT(5), value);
      }
      break;
    case SMB_SENSOR_VDDCK_0_IN_CURR:
      if( brd_type_rev == BOARD_WEDGE400C_EVT ){
        ret = read_attr(SMB_IR_HMB_DEVICE, CURR(2), value);
      }
      break;
    case SMB_SENSOR_VDDCK_0_OUT_CURR:
      if( brd_type_rev == BOARD_WEDGE400C_EVT ){
        ret = read_attr(SMB_IR_HMB_DEVICE, CURR(4), value);
      }else if( brd_type_rev == BOARD_WEDGE400C_EVT2 ||
                brd_type_rev == BOARD_WEDGE400C_DVT ||
                brd_type_rev == BOARD_WEDGE400C_DVT2 ||
                brd_type_rev == BOARD_WEDGE400C_MP_RESPIN ){
        ret = read_attr(SMB_PXE1211_DEVICE, CURR(5), value);
      }
      break;
    case SMB_SENSOR_VDDCK_0_IN_POWER:
      if( brd_type_rev == BOARD_WEDGE400C_EVT ){
        ret = read_attr(SMB_IR_HMB_DEVICE, POWER(2), value);
      }else if( brd_type_rev == BOARD_WEDGE400C_EVT2 ||
                brd_type_rev == BOARD_WEDGE400C_DVT ||
                brd_type_rev == BOARD_WEDGE400C_DVT2 ||
                brd_type_rev == BOARD_WEDGE400C_MP_RESPIN ){
        ret = read_attr(SMB_PXE1211_DEVICE, POWER(2), value);
      }
      *value /= 1000;
      break;
    case SMB_SENSOR_VDDCK_0_OUT_POWER:
      if( brd_type_rev == BOARD_WEDGE400C_EVT ){
        ret = read_attr(SMB_IR_HMB_DEVICE, POWER(4), value);
      }else if( brd_type_rev == BOARD_WEDGE400C_EVT2 ||
                brd_type_rev == BOARD_WEDGE400C_DVT ||
                brd_type_rev == BOARD_WEDGE400C_DVT2 ||
                brd_type_rev == BOARD_WEDGE400C_MP_RESPIN ){
        ret = read_attr(SMB_PXE1211_DEVICE, POWER(5), value);
      }
      *value /= 1000;
      break;
    case SMB_SENSOR_VDDCK_0_TEMP:
      if( brd_type_rev == BOARD_WEDGE400C_EVT ){
        ret = read_attr(SMB_IR_HMB_DEVICE, TEMP(2), value);
      }else if( brd_type_rev == BOARD_WEDGE400C_EVT2 ||
                brd_type_rev == BOARD_WEDGE400C_DVT ||
                brd_type_rev == BOARD_WEDGE400C_DVT2 ||
                brd_type_rev == BOARD_WEDGE400C_MP_RESPIN ){
        ret = read_attr(SMB_PXE1211_DEVICE, TEMP(2),value);
      }
      break;
    case SMB_SENSOR_VDDCK_1_IN_VOLT:
      if( brd_type_rev == BOARD_WEDGE400C_EVT2 ||
          brd_type_rev == BOARD_WEDGE400C_DVT ||
          brd_type_rev == BOARD_WEDGE400C_DVT2 ||
          brd_type_rev == BOARD_WEDGE400C_MP_RESPIN ){
        ret = read_attr(SMB_PXE1211_DEVICE, VOLT(3), value);
      }
      break;
    case SMB_SENSOR_VDDCK_1_OUT_VOLT:
      if( brd_type_rev == BOARD_WEDGE400C_EVT2 ||
          brd_type_rev == BOARD_WEDGE400C_DVT ||
          brd_type_rev == BOARD_WEDGE400C_DVT2 ||
          brd_type_rev == BOARD_WEDGE400C_MP_RESPIN ){
        ret = read_attr(SMB_PXE1211_DEVICE, VOLT(6), value);
      }
      break;
    case SMB_SENSOR_VDDCK_1_OUT_CURR:
      if( brd_type_rev == BOARD_WEDGE400C_EVT2 ||
          brd_type_rev == BOARD_WEDGE400C_DVT ||
          brd_type_rev == BOARD_WEDGE400C_DVT2 ||
          brd_type_rev == BOARD_WEDGE400C_MP_RESPIN ){
        ret = read_attr(SMB_PXE1211_DEVICE, CURR(6),value);
      }
      break;
    case SMB_SENSOR_VDDCK_1_OUT_POWER:
      if( brd_type_rev == BOARD_WEDGE400C_EVT2 ||
          brd_type_rev == BOARD_WEDGE400C_DVT ||
          brd_type_rev == BOARD_WEDGE400C_DVT2 ||
          brd_type_rev == BOARD_WEDGE400C_MP_RESPIN ){
        ret = read_attr(SMB_PXE1211_DEVICE, POWER(6), value);
      }
      *value /= 1000;
      break;
    case SMB_SENSOR_VDDCK_1_TEMP:
      if( brd_type_rev == BOARD_WEDGE400C_EVT2 ||
          brd_type_rev == BOARD_WEDGE400C_DVT ||
          brd_type_rev == BOARD_WEDGE400C_DVT2 ||
          brd_type_rev == BOARD_WEDGE400C_MP_RESPIN ){
        ret = read_attr(SMB_PXE1211_DEVICE, TEMP(3), value);
      }
      break;
    default:
      ret = READING_NA;
      break;
  }
  return ret;
}

static int
pem_sensor_read(uint8_t sensor_num, void *value) {

  int ret = -1;

  switch(sensor_num) {
    case PEM1_SENSOR_IN_VOLT:
      ret = read_attr(PEM1_DEVICE, VOLT(1), value);
      break;
    case PEM1_SENSOR_OUT_VOLT:
      ret = read_attr(PEM1_DEVICE, VOLT(2), value);
      break;
    case PEM1_SENSOR_FET_BAD:
      ret = read_attr(PEM1_DEVICE, VOLT(3), value);
      break;
    case PEM1_SENSOR_FET_SHORT:
      ret = read_attr(PEM1_DEVICE, VOLT(4), value);
      break;
    case PEM1_SENSOR_CURR:
      ret = read_attr(PEM1_DEVICE, CURR(1), value);
      break;
    case PEM1_SENSOR_POWER:
      ret = read_attr(PEM1_DEVICE, POWER(1), value);
      *(float *)value /= 1000;
      break;
    case PEM1_SENSOR_TEMP1:
      ret = read_attr(PEM1_DEVICE, TEMP(1), value);
      break;
    case PEM1_SENSOR_FAN1_TACH:
      ret = read_fan_rpm_f(PEM1_DEVICE_EXT, 1, value);
      break;
    case PEM1_SENSOR_FAN2_TACH:
      ret = read_fan_rpm_f(PEM1_DEVICE_EXT, 2, value);
      break;
    case PEM1_SENSOR_TEMP2:
      ret = read_attr(PEM1_DEVICE_EXT, TEMP(1), value);
      break;
    case PEM1_SENSOR_TEMP3:
      ret = read_attr(PEM1_DEVICE_EXT, TEMP(2), value);
      break;
    case PEM1_SENSOR_FAULT_OV:
      ret = read_attr_integer(PEM1_DEVICE, "fault_ov", value);
      break;
    case PEM1_SENSOR_FAULT_UV:
      ret = read_attr_integer(PEM1_DEVICE, "fault_uv", value);
      break;
    case PEM1_SENSOR_FAULT_OC:
      ret = read_attr_integer(PEM1_DEVICE, "fault_oc", value);
      break;
    case PEM1_SENSOR_FAULT_POWER:
      ret = read_attr_integer(PEM1_DEVICE, "fault_power", value);
      break;
    case PEM1_SENSOR_ON_FAULT:
      ret = read_attr_integer(PEM1_DEVICE, "on_fault", value);
      break;
    case PEM1_SENSOR_FAULT_FET_SHORT:
      ret = read_attr_integer(PEM1_DEVICE, "fault_fet_short", value);
      break;
    case PEM1_SENSOR_FAULT_FET_BAD:
      ret = read_attr_integer(PEM1_DEVICE, "fault_fet_bad", value);
      break;
    case PEM1_SENSOR_EEPROM_DONE:
      ret = read_attr_integer(PEM1_DEVICE, "eeprom_done", value);
      break;
    case PEM1_SENSOR_POWER_ALARM_HIGH:
      ret = read_attr_integer(PEM1_DEVICE, "power_alarm_high", value);
      break;
    case PEM1_SENSOR_POWER_ALARM_LOW:
      ret = read_attr_integer(PEM1_DEVICE, "power_alarm_low", value);
      break;
    case PEM1_SENSOR_VSENSE_ALARM_HIGH:
      ret = read_attr_integer(PEM1_DEVICE, "vsense_alarm_high", value);
      break;
    case PEM1_SENSOR_VSENSE_ALARM_LOW:
      ret = read_attr_integer(PEM1_DEVICE, "vsense_alarm_low", value);
      break;
    case PEM1_SENSOR_VSOURCE_ALARM_HIGH:
      ret = read_attr_integer(PEM1_DEVICE, "vsource_alarm_high", value);
      break;
    case PEM1_SENSOR_VSOURCE_ALARM_LOW:
      ret = read_attr_integer(PEM1_DEVICE, "vsource_alarm_low", value);
      break;
    case PEM1_SENSOR_GPIO_ALARM_HIGH:
      ret = read_attr_integer(PEM1_DEVICE, "gpio_alarm_high", value);
      break;
    case PEM1_SENSOR_GPIO_ALARM_LOW:
      ret = read_attr_integer(PEM1_DEVICE, "gpio_alarm_low", value);
      break;
    case PEM1_SENSOR_ON_STATUS:
      ret = read_attr_integer(PEM1_DEVICE, "on_status", value);
      break;
    case PEM1_SENSOR_STATUS_FET_BAD:
      ret = read_attr_integer(PEM1_DEVICE, "fet_bad", value);
      break;
    case PEM1_SENSOR_STATUS_FET_SHORT:
      ret = read_attr_integer(PEM1_DEVICE, "fet_short", value);
      break;
    case PEM1_SENSOR_STATUS_ON_PIN:
      ret = read_attr_integer(PEM1_DEVICE, "on_pin_status", value);
      break;
    case PEM1_SENSOR_STATUS_POWER_GOOD:
      ret = read_attr_integer(PEM1_DEVICE, "power_good", value);
      break;
    case PEM1_SENSOR_STATUS_OC:
      ret = read_attr_integer(PEM1_DEVICE, "oc_status", value);
      break;
    case PEM1_SENSOR_STATUS_UV:
      ret = read_attr_integer(PEM1_DEVICE, "uv_status", value);
      break;
    case PEM1_SENSOR_STATUS_OV:
      ret = read_attr_integer(PEM1_DEVICE, "ov_status", value);
      break;
    case PEM1_SENSOR_STATUS_GPIO3:
      ret = read_attr_integer(PEM1_DEVICE, "gpio3_status", value);
      break;
    case PEM1_SENSOR_STATUS_GPIO2:
      ret = read_attr_integer(PEM1_DEVICE, "gpio2_status", value);
      break;
    case PEM1_SENSOR_STATUS_GPIO1:
      ret = read_attr_integer(PEM1_DEVICE, "gpio1_status", value);
      break;
    case PEM1_SENSOR_STATUS_ALERT:
      ret = read_attr_integer(PEM1_DEVICE, "alert_status", value);
      break;
    case PEM1_SENSOR_STATUS_EEPROM_BUSY:
      ret = read_attr_integer(PEM1_DEVICE, "eeprom_busy", value);
      break;
    case PEM1_SENSOR_STATUS_ADC_IDLE:
      ret = read_attr_integer(PEM1_DEVICE, "adc_idle", value);
      break;
    case PEM1_SENSOR_STATUS_TICKER_OVERFLOW:
      ret = read_attr_integer(PEM1_DEVICE, "ticker_overflow", value);
      break;
    case PEM1_SENSOR_STATUS_METER_OVERFLOW:
      ret = read_attr_integer(PEM1_DEVICE, "meter_overflow", value);
      break;

    case PEM2_SENSOR_IN_VOLT:
      ret = read_attr(PEM2_DEVICE, VOLT(1), value);
      break;
    case PEM2_SENSOR_OUT_VOLT:
      ret = read_attr(PEM2_DEVICE, VOLT(2), value);
      break;
    case PEM2_SENSOR_FET_BAD:
      ret = read_attr(PEM2_DEVICE, VOLT(3), value);
      break;
    case PEM2_SENSOR_FET_SHORT:
      ret = read_attr(PEM2_DEVICE, VOLT(4), value);
      break;
    case PEM2_SENSOR_CURR:
      ret = read_attr(PEM2_DEVICE, CURR(1), value);
      break;
    case PEM2_SENSOR_POWER:
      ret = read_attr(PEM2_DEVICE, POWER(1), value);
      *(float *)value /= 1000;
      break;
    case PEM2_SENSOR_TEMP1:
      ret = read_attr(PEM2_DEVICE, TEMP(1), value);
      break;
    case PEM2_SENSOR_FAN1_TACH:
      ret = read_fan_rpm_f(PEM2_DEVICE_EXT, 1, value);
      break;
    case PEM2_SENSOR_FAN2_TACH:
      ret = read_fan_rpm_f(PEM2_DEVICE_EXT, 2, value);
      break;
    case PEM2_SENSOR_TEMP2:
      ret = read_attr(PEM2_DEVICE_EXT, TEMP(1), value);
      break;
    case PEM2_SENSOR_TEMP3:
      ret = read_attr(PEM2_DEVICE_EXT, TEMP(2), value);
      break;
    case PEM2_SENSOR_FAULT_OV:
      ret = read_attr_integer(PEM2_DEVICE, "fault_ov", value);
      break;
    case PEM2_SENSOR_FAULT_UV:
      ret = read_attr_integer(PEM2_DEVICE, "fault_uv", value);
      break;
    case PEM2_SENSOR_FAULT_OC:
      ret = read_attr_integer(PEM2_DEVICE, "fault_oc", value);
      break;
    case PEM2_SENSOR_FAULT_POWER:
      ret = read_attr_integer(PEM2_DEVICE, "fault_power", value);
      break;
    case PEM2_SENSOR_ON_FAULT:
      ret = read_attr_integer(PEM2_DEVICE, "on_fault", value);
      break;
    case PEM2_SENSOR_FAULT_FET_SHORT:
      ret = read_attr_integer(PEM2_DEVICE, "fault_fet_short", value);
      break;
    case PEM2_SENSOR_FAULT_FET_BAD:
      ret = read_attr_integer(PEM2_DEVICE, "fault_fet_bad", value);
      break;
    case PEM2_SENSOR_EEPROM_DONE:
      ret = read_attr_integer(PEM2_DEVICE, "eeprom_done", value);
      break;
    case PEM2_SENSOR_POWER_ALARM_HIGH:
      ret = read_attr_integer(PEM2_DEVICE, "power_alrm_high", value);
      break;
    case PEM2_SENSOR_POWER_ALARM_LOW:
      ret = read_attr_integer(PEM2_DEVICE, "power_alrm_low", value);
      break;
    case PEM2_SENSOR_VSENSE_ALARM_HIGH:
      ret = read_attr_integer(PEM2_DEVICE, "vsense_alrm_high", value);
      break;
    case PEM2_SENSOR_VSENSE_ALARM_LOW:
      ret = read_attr_integer(PEM2_DEVICE, "vsense_alrm_low", value);
      break;
    case PEM2_SENSOR_VSOURCE_ALARM_HIGH:
      ret = read_attr_integer(PEM2_DEVICE, "vsource_alrm_high", value);
      break;
    case PEM2_SENSOR_VSOURCE_ALARM_LOW:
      ret = read_attr_integer(PEM2_DEVICE, "vsource_alrm_low", value);
      break;
    case PEM2_SENSOR_GPIO_ALARM_HIGH:
      ret = read_attr_integer(PEM2_DEVICE, "gpio_alrm_high", value);
      break;
    case PEM2_SENSOR_GPIO_ALARM_LOW:
      ret = read_attr_integer(PEM2_DEVICE, "gpio_alrm_low", value);
      break;
    case PEM2_SENSOR_ON_STATUS:
      ret = read_attr_integer(PEM2_DEVICE, "on_status", value);
      break;
    case PEM2_SENSOR_STATUS_FET_BAD:
      ret = read_attr_integer(PEM2_DEVICE, "fet_bad", value);
      break;
    case PEM2_SENSOR_STATUS_FET_SHORT:
      ret = read_attr_integer(PEM2_DEVICE, "fet_short", value);
      break;
    case PEM2_SENSOR_STATUS_ON_PIN:
      ret = read_attr_integer(PEM2_DEVICE, "on_pin_status", value);
      break;
    case PEM2_SENSOR_STATUS_POWER_GOOD:
      ret = read_attr_integer(PEM2_DEVICE, "power_good", value);
      break;
    case PEM2_SENSOR_STATUS_OC:
      ret = read_attr_integer(PEM2_DEVICE, "oc_status", value);
      break;
    case PEM2_SENSOR_STATUS_UV:
      ret = read_attr_integer(PEM2_DEVICE, "uv_status", value);
      break;
    case PEM2_SENSOR_STATUS_OV:
      ret = read_attr_integer(PEM2_DEVICE, "ov_status", value);
      break;
    case PEM2_SENSOR_STATUS_GPIO3:
      ret = read_attr_integer(PEM2_DEVICE, "gpio3_status", value);
      break;
    case PEM2_SENSOR_STATUS_GPIO2:
      ret = read_attr_integer(PEM2_DEVICE, "gpio2_status", value);
      break;
    case PEM2_SENSOR_STATUS_GPIO1:
      ret = read_attr_integer(PEM2_DEVICE, "gpio1_status", value);
      break;
    case PEM2_SENSOR_STATUS_ALERT:
      ret = read_attr_integer(PEM2_DEVICE, "alert_status", value);
      break;
    case PEM2_SENSOR_STATUS_EEPROM_BUSY:
      ret = read_attr_integer(PEM2_DEVICE, "eeprom_busy", value);
      break;
    case PEM2_SENSOR_STATUS_ADC_IDLE:
      ret = read_attr_integer(PEM2_DEVICE, "adc_idle", value);
      break;
    case PEM2_SENSOR_STATUS_TICKER_OVERFLOW:
      ret = read_attr_integer(PEM2_DEVICE, "ticker_overflow", value);
      break;
    case PEM2_SENSOR_STATUS_METER_OVERFLOW:
      ret = read_attr_integer(PEM2_DEVICE, "meter_overflow", value);
      break;

    default:
      ret = READING_NA;
      break;
  }
  return ret;
}

static int
psu_sensor_read(uint8_t sensor_num, float *value) {

  int ret = -1;

  switch(sensor_num) {
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
      break;
    case PSU1_SENSOR_12V_POWER:
      ret = read_attr(PSU1_DEVICE, POWER(2), value);
      break;
    case PSU1_SENSOR_STBY_POWER:
      ret = read_attr(PSU1_DEVICE, POWER(3), value);
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
    case PSU1_SENSOR_FAN2_TACH:
      ret = read_fan_rpm_f(PSU1_DEVICE, 2, value);
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
      break;
    case PSU2_SENSOR_12V_POWER:
      ret = read_attr(PSU2_DEVICE, POWER(2), value);
      break;
    case PSU2_SENSOR_STBY_POWER:
      ret = read_attr(PSU2_DEVICE, POWER(3), value);
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
    case PSU2_SENSOR_FAN2_TACH:
      ret = read_fan_rpm_f(PSU2_DEVICE, 2, value);
      break;
    default:
      ret = READING_NA;
      break;
  }
  return ret;
}

int
pal_sensor_read_raw(uint8_t fru, uint8_t sensor_num, void *value) {

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
#ifdef DEBUG
  OBMC_INFO("pal_sensor_read_raw(): %s is not present\n", fru_name);
#endif
    return -1;
  }

  ret = pal_is_fru_ready(fru, &status);
  if (ret) {
    return ret;
  }
  if (!status) {
#ifdef DEBUG
  OBMC_INFO("pal_sensor_read_raw(): %s is not ready\n", fru_name);
#endif
    return -1;
  }

  sprintf(key, "%s_sensor%d", fru_name, sensor_num);
  switch(fru) {
    case FRU_SCM:
      ret = scm_sensor_read(sensor_num, value);
      if (sensor_num == SCM_SENSOR_INLET_TEMP) {
        delay = 100;
      }
      break;
    case FRU_FAN1:
    case FRU_FAN2:
    case FRU_FAN3:
    case FRU_FAN4:
      ret = fan_sensor_read(sensor_num, value);
      delay=100;
      break;
    case FRU_SMB:
      ret = smb_sensor_read(sensor_num, value);
      if (sensor_num == SMB_SENSOR_SW_DIE_TEMP1 ||
          sensor_num == SMB_SENSOR_SW_DIE_TEMP2 ||
          (sensor_num >= FAN_SENSOR_FAN1_FRONT_TACH &&
           sensor_num <= FAN_SENSOR_FAN4_REAR_TACH)) {
        delay = 100;
      }
      break;
    case FRU_PEM1:
    case FRU_PEM2:
      ret = pem_sensor_read(sensor_num, value);
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

int
pal_sensor_discrete_read_raw(uint8_t fru, uint8_t sensor_num, void *value) {

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
#ifdef DEBUG
  OBMC_INFO("pal_sensor_discrete_read_raw(): %s is not present\n", fru_name);
#endif
    return -1;
  }

  ret = pal_is_fru_ready(fru, &status);
  if (ret) {
    return ret;
  }
  if (!status) {
#ifdef DEBUG
  OBMC_INFO("pal_sensor_discrete_read_raw(): %s is not ready\n", fru_name);
#endif
    return -1;
  }

  sprintf(key, "%s_sensor%d", fru_name, sensor_num);
  switch(fru) {
    case FRU_PEM1:
    case FRU_PEM2:
      ret = pem_sensor_read(sensor_num, value);
      break;

    default:
      return -1;
  }

  if (ret == READING_NA || ret == -1) {
    return READING_NA;
  }
  msleep(delay);

  return 0;
};

static int
get_scm_sensor_name(uint8_t sensor_num, char *name) {

  switch(sensor_num) {
    case SCM_SENSOR_OUTLET_TEMP:
      sprintf(name, "SCM_OUTLET_TEMP");
      break;
    case SCM_SENSOR_INLET_TEMP:
      sprintf(name, "SCM_INLET_TEMP");
      break;
    case SCM_SENSOR_HSC_VOLT:
      sprintf(name, "SCM_HSC_VOLT");
      break;
    case SCM_SENSOR_HSC_CURR:
      sprintf(name, "SCM_HSC_CURR");
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

static int
get_fan_sensor_name(uint8_t sensor_num, char *name) {

  switch(sensor_num) {
    case FAN_SENSOR_FAN1_FRONT_TACH:
      sprintf(name, "FAN1_FRONT_SPEED");
      break;
    case FAN_SENSOR_FAN1_REAR_TACH:
      sprintf(name, "FAN1_REAR_SPEED");
      break;
    case FAN_SENSOR_FAN2_FRONT_TACH:
      sprintf(name, "FAN2_FRONT_SPEED");
      break;
    case FAN_SENSOR_FAN2_REAR_TACH:
      sprintf(name, "FAN2_REAR_SPEED");
      break;
    case FAN_SENSOR_FAN3_FRONT_TACH:
      sprintf(name, "FAN3_FRONT_SPEED");
      break;
    case FAN_SENSOR_FAN3_REAR_TACH:
      sprintf(name, "FAN3_REAR_SPEED");
      break;
    case FAN_SENSOR_FAN4_FRONT_TACH:
      sprintf(name, "FAN4_FRONT_SPEED");
      break;
    case FAN_SENSOR_FAN4_REAR_TACH:
      sprintf(name, "FAN4_REAR_SPEED");
      break;
    default:
      return -1;
  }

  return 0;
}

static int
get_smb_sensor_name(uint8_t sensor_num, char *name) {
  uint8_t brd_type;
  uint8_t brd_type_rev;
  if(pal_get_board_type(&brd_type)){
    return -1;
  }
  if(pal_get_board_type_rev(&brd_type_rev)){
    return -1;
  }
  sprintf(name,SENSOR_NAME_ERR);
  switch(sensor_num) {
    case SMB_SENSOR_SW_SERDES_PVDD_TEMP1:
      if(brd_type == BRD_TYPE_WEDGE400){
        sprintf(name, "TH3_SERDES_TEMP1(PVDD)");
      }else if(brd_type == BRD_TYPE_WEDGE400C){
        sprintf(name, "XP0R94V_VDDA_TEMP");
      }
      break;
    case SMB_SENSOR_SW_SERDES_TRVDD_TEMP1:
      if(brd_type == BRD_TYPE_WEDGE400){
        sprintf(name, "TH3_SERDES_TEMP1(TRVDD)");
      }else if(brd_type == BRD_TYPE_WEDGE400C){
        sprintf(name, "XP0R75V_PCIE_TEMP");
      }
      break;
    case SMB_SENSOR_IR3R3V_LEFT_TEMP:
      sprintf(name, "XP3R3V_LEFT_TEMP");
      break;
    case SMB_SENSOR_IR3R3V_RIGHT_TEMP:
      sprintf(name, "XP3R3V_RIGHT_TEMP");
      break;
    case SMB_SENSOR_SW_CORE_TEMP1:
      if(brd_type == BRD_TYPE_WEDGE400){
        sprintf(name, "TH3_CORE_TEMP1");
      }else if(brd_type == BRD_TYPE_WEDGE400C){
        sprintf(name, SENSOR_NAME_ERR);
      }
      break;
    case SMB_SENSOR_LM75B_U28_TEMP:
      sprintf(name, "SMB_LM75B_U28_TEMP");
      break;
    case SMB_SENSOR_LM75B_U25_TEMP:
      sprintf(name, "SMB_LM75B_U25_TEMP");
      break;
    case SMB_SENSOR_LM75B_U56_TEMP:
      sprintf(name, "SMB_LM75B_U56_TEMP");
      break;
    case SMB_SENSOR_LM75B_U55_TEMP:
      sprintf(name, "SMB_LM75B_U55_TEMP");
      break;
    case SMB_SENSOR_TMP421_U62_TEMP:
      sprintf(name, "SMB_TMP421_U62_TEMP");
      break;
    case SMB_SENSOR_TMP421_Q9_TEMP:
      sprintf(name, "SMB_TMP421_Q9_TEMP");
      break;
    case SMB_SENSOR_TMP421_U63_TEMP:
      sprintf(name, "SMB_TMP421_U63_TEMP");
      break;
    case SMB_SENSOR_TMP421_Q10_TEMP:
      sprintf(name, "SMB_TMP421_Q10_TEMP");
      break;
    case SMB_SENSOR_TMP422_U20_TEMP:
      sprintf(name, "SMB_TMP422_U20_TEMP");
      break;
    case SMB_SENSOR_SW_DIE_TEMP1:
      if(brd_type == BRD_TYPE_WEDGE400){
        sprintf(name, "TH3_DIE_TEMP1");
      }else if(brd_type == BRD_TYPE_WEDGE400C){
        sprintf(name, "GB_DIE_TEMP_HIGH");
      }
      break;
    case SMB_SENSOR_SW_DIE_TEMP2:
      if(brd_type == BRD_TYPE_WEDGE400){
        sprintf(name, "TH3_DIE_TEMP2");
      }else if(brd_type == BRD_TYPE_WEDGE400C){
        sprintf(name, "GB_DIE_TEMP_MEAN");
      }
      break;
    case SMB_SENSOR_GB_TEMP1:
      sprintf(name, "GB_TEMP1");
      break;
    case SMB_SENSOR_GB_TEMP2:
      sprintf(name, "GB_TEMP2");
      break;
    case SMB_SENSOR_GB_TEMP3:
      sprintf(name, "GB_TEMP3");
      break;
    case SMB_SENSOR_GB_TEMP4:
      sprintf(name, "GB_TEMP4");
      break;
    case SMB_SENSOR_GB_TEMP5:
      sprintf(name, "GB_TEMP5");
      break;
    case SMB_SENSOR_GB_TEMP6:
      sprintf(name, "GB_TEMP6");
      break;
    case SMB_SENSOR_GB_TEMP7:
      sprintf(name, "GB_TEMP7");
      break;
    case SMB_SENSOR_GB_TEMP8:
      sprintf(name, "GB_TEMP8");
      break;
    case SMB_SENSOR_GB_TEMP9:
      sprintf(name, "GB_TEMP9");
      break;
    case SMB_SENSOR_GB_TEMP10:
      sprintf(name, "GB_TEMP10");
      break;
    case SMB_DOM1_MAX_TEMP:
      sprintf(name, "DOM_FPGA1_MAX_TEMP");
      break;
    case SMB_DOM2_MAX_TEMP:
      sprintf(name, "DOM_FPGA2_MAX_TEMP");
      break;
    case SMB_SENSOR_FCM_LM75B_U1_TEMP:
      sprintf(name, "FCM_LM75B_U1_TEMP");
      break;
    case SMB_SENSOR_FCM_LM75B_U2_TEMP:
      sprintf(name, "FCM_LM75B_U2_TEMP");
      break;
    case SMB_SENSOR_1220_VMON1:
      sprintf(name, "XP12R0V(12V)");
      break;
    case SMB_SENSOR_1220_VMON2:
      sprintf(name, "XP5R0V(5V)");
      break;
    case SMB_SENSOR_1220_VMON3:
      sprintf(name, "XP3R3V_BMC(3.3V)");
      break;
    case SMB_SENSOR_1220_VMON4:
      if(brd_type == BRD_TYPE_WEDGE400){
        sprintf(name, "XP2R5V_BMC(2.5V)");
      }else if(brd_type == BRD_TYPE_WEDGE400C){
        sprintf(name, "XP3R3V_FPGA(3.3V)");
      }
      break;
    case SMB_SENSOR_1220_VMON5:
      sprintf(name, "XP1R2V_BMC(1.2V)");
      break;
    case SMB_SENSOR_1220_VMON6:
      if(brd_type == BRD_TYPE_WEDGE400){
        sprintf(name, "XP1R15V_BMC(1.15V)");
      }else if(brd_type == BRD_TYPE_WEDGE400C){
        sprintf(name, "XP1R8V_FPGA(1.8V)");
      }
      break;
    case SMB_SENSOR_1220_VMON7:
      if(brd_type == BRD_TYPE_WEDGE400){
        sprintf(name, "XP1R2V_TH3(1.2V)");
      }else if(brd_type == BRD_TYPE_WEDGE400C){
        sprintf(name, "XP1R8V_IO(1.8V)");
      }
      break;
    case SMB_SENSOR_1220_VMON8:
      if(brd_type == BRD_TYPE_WEDGE400){
        sprintf(name, "PVDD0P8_TH3(0.8V)");
      }else if(brd_type == BRD_TYPE_WEDGE400C){
        sprintf(name, "XP2R5V_HBM(2.5V)");
      }
      break;
    case SMB_SENSOR_1220_VMON9:
      if(brd_type == BRD_TYPE_WEDGE400){
        sprintf(name, "XP3R3V_TH3(3.3V)");
      }else if(brd_type == BRD_TYPE_WEDGE400C){
        sprintf(name, "XP0R94V_VDDA(0.94V)");
      }
      break;
    case SMB_SENSOR_1220_VMON10:
      if(brd_type == BRD_TYPE_WEDGE400){
        sprintf(name, "VDD_CORE_TH3(0.75~0.9V)");
      }else if(brd_type == BRD_TYPE_WEDGE400C){
        if(brd_type_rev == BOARD_WEDGE400C_EVT)
          sprintf(name, "VDD_CORE_GB(0.85V)");
        else if(brd_type_rev == BOARD_WEDGE400C_EVT2 ||
                brd_type_rev == BOARD_WEDGE400C_DVT ||
                brd_type_rev == BOARD_WEDGE400C_DVT2 ||
                brd_type_rev == BOARD_WEDGE400C_MP_RESPIN)
          sprintf(name, "VDD_CORE_GB(0.75/0.825V)");
      }
      break;
    case SMB_SENSOR_1220_VMON11:
      if(brd_type == BRD_TYPE_WEDGE400){
        sprintf(name, "TRVDD0P8_TH3(0.8V)");
      }else if(brd_type == BRD_TYPE_WEDGE400C){
        sprintf(name, "XP0R75V_PCIE(0.75V)");
      }
      break;
    case SMB_SENSOR_1220_VMON12:
      if(brd_type == BRD_TYPE_WEDGE400){
        sprintf(name, "XP1R8V_TH3(1.8V)");
      }else if(brd_type == BRD_TYPE_WEDGE400C){
        if(brd_type_rev == BOARD_WEDGE400C_EVT){
          sprintf(name, "XP1R15V_VDDCK(1.15V)");
        }else if(brd_type_rev == BOARD_WEDGE400C_EVT2 ||
                 brd_type_rev == BOARD_WEDGE400C_DVT ||
                 brd_type_rev == BOARD_WEDGE400C_DVT2 ||
                 brd_type_rev == BOARD_WEDGE400C_MP_RESPIN ){
          sprintf(name, "XP1R15V_VDDCK_2(1.15V)");
        }
      }
      break;
    case SMB_SENSOR_1220_VCCA:
      sprintf(name, "POWR1220_VCCA(3.3V)");
      break;
    case SMB_SENSOR_1220_VCCINP:
      sprintf(name, "POWR1220_VCCINP(3.3V)");
      break;
    case SMB_SENSOR_SW_SERDES_PVDD_IN_VOLT:
      if(brd_type == BRD_TYPE_WEDGE400){
        sprintf(name, "TH3_SERDES_IN_VOLT(PVDD)");
      }else if(brd_type == BRD_TYPE_WEDGE400C){
        sprintf(name, "XP0R94V_VDDA_IN_VOLT");
      }
      break;
    case SMB_SENSOR_SW_SERDES_PVDD_OUT_VOLT:
      if(brd_type == BRD_TYPE_WEDGE400){
        sprintf(name, "TH3_SERDES_OUT_VOLT(PVDD)");
      }else if(brd_type == BRD_TYPE_WEDGE400C){
        sprintf(name, "XP0R94V_VDDA_OUT_VOLT");
      }
      break;
    case SMB_SENSOR_SW_SERDES_TRVDD_IN_VOLT:
      if(brd_type == BRD_TYPE_WEDGE400){
        sprintf(name, "TH3_SERDES_IN_VOLT(TRVDD)");
      }else if(brd_type == BRD_TYPE_WEDGE400C){
        sprintf(name, "XP0R75V_PCIE_IN_VOLT");
      }
      break;
    case SMB_SENSOR_SW_SERDES_TRVDD_OUT_VOLT:
      if(brd_type == BRD_TYPE_WEDGE400){
        sprintf(name, "TH3_SERDES_OUT_VOLT(TRVDD)");
      }else if(brd_type == BRD_TYPE_WEDGE400C){
        sprintf(name, "XP0R75V_PCIE_OUT_VOLT");
      }
      break;
    case SMB_SENSOR_IR3R3V_LEFT_IN_VOLT:
      sprintf(name, "XP3R3V_LEFT_IN_VOLT");
      break;
    case SMB_SENSOR_IR3R3V_LEFT_OUT_VOLT:
      sprintf(name, "XP3R3V_LEFT_OUT_VOLT");
      break;
    case SMB_SENSOR_IR3R3V_RIGHT_IN_VOLT:
      sprintf(name, "XP3R3V_RIGHT_IN_VOLT");
      break;
    case SMB_SENSOR_IR3R3V_RIGHT_OUT_VOLT:
      sprintf(name, "XP3R3V_RIGHT_OUT_VOLT");
      break;
    case SMB_SENSOR_SW_CORE_VOLT:
      if(brd_type == BRD_TYPE_WEDGE400){
        sprintf(name, "TH3_CORE_VOLT");
      }else if(brd_type == BRD_TYPE_WEDGE400C){
        sprintf(name, "VDD_CORE_VOLT");
      }
      break;
    case SMB_SENSOR_FCM_HSC_VOLT:
      sprintf(name, "FCM_HSC_VOLT");
      break;
    case SMB_SENSOR_SW_SERDES_PVDD_IN_CURR:
      if(brd_type == BRD_TYPE_WEDGE400){
        sprintf(name, "TH3_SERDES_IN_CURR(PVDD)");
      }else if(brd_type == BRD_TYPE_WEDGE400C){
        sprintf(name, "XP0R94V_VDDA_IN_CURR");
      }
      break;
    case SMB_SENSOR_SW_SERDES_PVDD_OUT_CURR:
      if(brd_type == BRD_TYPE_WEDGE400){
        sprintf(name, "TH3_SERDES_OUT_CURR(PVDD)");
      }else if(brd_type == BRD_TYPE_WEDGE400C){
        sprintf(name, "XP0R94V_VDDA_OUT_CURR");
      }
      break;
    case SMB_SENSOR_SW_SERDES_TRVDD_IN_CURR:
      if(brd_type == BRD_TYPE_WEDGE400){
        sprintf(name, "TH3_SERDES_IN_CURR(TRVDD)");
      }else if(brd_type == BRD_TYPE_WEDGE400C){
        sprintf(name, "XP0R75V_PCIE_IN_CURR");
      }
      break;
    case SMB_SENSOR_SW_SERDES_TRVDD_OUT_CURR:
      if(brd_type == BRD_TYPE_WEDGE400){
        sprintf(name, "TH3_SERDES_OUT_CURR(TRVDD)");
      }else if(brd_type == BRD_TYPE_WEDGE400C){
        sprintf(name, "XP0R75V_PCIE_OUT_CURR");
      }
      break;
    case SMB_SENSOR_IR3R3V_LEFT_IN_CURR:
      sprintf(name, "XP3R3V_LEFT_IN_CURR");
      break;
    case SMB_SENSOR_IR3R3V_LEFT_OUT_CURR:
      sprintf(name, "XP3R3V_LEFT_OUT_CURR");
      break;
    case SMB_SENSOR_IR3R3V_RIGHT_IN_CURR:
      sprintf(name, "XP3R3V_RIGHT_IN_CURR");
      break;
    case SMB_SENSOR_IR3R3V_RIGHT_OUT_CURR:
      sprintf(name, "XP3R3V_RIGHT_OUT_CURR");
      break;
    case SMB_SENSOR_SW_CORE_CURR:
      if(brd_type == BRD_TYPE_WEDGE400){
        sprintf(name, "TH3_CORE_CURR");
      }else if(brd_type == BRD_TYPE_WEDGE400C){
        sprintf(name, "VDD_CORE_CURR");
      }
      break;
    case SMB_SENSOR_FCM_HSC_CURR:
      sprintf(name, "FCM_HSC_CURR");
      break;
    case SMB_SENSOR_SW_SERDES_PVDD_IN_POWER:
      if(brd_type == BRD_TYPE_WEDGE400){
        sprintf(name, "TH3_SERDES_IN_POWER(PVDD)");
      }else if(brd_type == BRD_TYPE_WEDGE400C){
        sprintf(name, "XP0R94V_VDDA_IN_POWER");
      }
      break;
    case SMB_SENSOR_SW_SERDES_PVDD_OUT_POWER:
      if(brd_type == BRD_TYPE_WEDGE400){
        sprintf(name, "TH3_SERDES_OUT_POWER(PVDD)");
      }else if(brd_type == BRD_TYPE_WEDGE400C){
        sprintf(name, "XP0R94V_VDDA_OUT_POWER");
      }
      break;
    case SMB_SENSOR_SW_SERDES_TRVDD_IN_POWER:
      if(brd_type == BRD_TYPE_WEDGE400){
        sprintf(name, "TH3_SERDES_IN_POWER(TRVDD)");
      }else if(brd_type == BRD_TYPE_WEDGE400C){
        sprintf(name, "XP0R75V_PCIE_IN_POWER");
      }
      break;
    case SMB_SENSOR_SW_SERDES_TRVDD_OUT_POWER:
      if(brd_type == BRD_TYPE_WEDGE400){
        sprintf(name, "TH3_SERDES_OUT_POWER(TRVDD)");
      }else if(brd_type == BRD_TYPE_WEDGE400C){
        sprintf(name, "XP0R75V_PCIE_OUT_POWER");
      }
      break;
    case SMB_SENSOR_IR3R3V_LEFT_IN_POWER:
      sprintf(name, "XP3R3V_LEFT_IN_POWER");
      break;
    case SMB_SENSOR_IR3R3V_LEFT_OUT_POWER:
      sprintf(name, "XP3R3V_LEFT_OUT_POWER");
      break;
    case SMB_SENSOR_IR3R3V_RIGHT_IN_POWER:
      sprintf(name, "XP3R3V_RIGHT_IN_POWER");
      break;
    case SMB_SENSOR_IR3R3V_RIGHT_OUT_POWER:
      sprintf(name, "XP3R3V_RIGHT_OUT_POWER");
      break;
    case SMB_SENSOR_SW_CORE_POWER:
      if(brd_type == BRD_TYPE_WEDGE400){
        sprintf(name, "TH3_CORE_POWER");
      }else if(brd_type == BRD_TYPE_WEDGE400C){
        sprintf(name, "VDD_CORE_POWER");
      }
      break;
    case SMB_BMC_ADC0_VSEN:
      sprintf(name, "XP1R0V_FPGA");
      break;
    case SMB_BMC_ADC1_VSEN:
      if(brd_type == BRD_TYPE_WEDGE400){
        sprintf(name, "XP1R8V_FPGA");
      }else if(brd_type == BRD_TYPE_WEDGE400C){
        sprintf(name, "XP2R5V_BMC");
      }
      break;
    case SMB_BMC_ADC2_VSEN:
      if(brd_type == BRD_TYPE_WEDGE400){
        sprintf(name, "XP3R3V_FPGA");
      }else if(brd_type == BRD_TYPE_WEDGE400C){
        sprintf(name, "XP1R15V_BMC");
      }
      break;
    case SMB_BMC_ADC3_VSEN:
      sprintf(name, "XP3R3V_RIGHT");
      break;
    case SMB_BMC_ADC4_VSEN:
      sprintf(name, "XP3R3V_LEFT");
      break;
    case SMB_BMC_ADC5_VSEN:
      sprintf(name, "XP1R8V_ALG");
      break;
    case SMB_BMC_ADC6_VSEN:
      sprintf(name, "XP1R2V_VDDH");
      break;
    case SMB_BMC_ADC7_VSEN:
      if ( brd_type_rev == BOARD_WEDGE400C_MP_RESPIN ){
        sprintf(name, "XP1R1V_OOB_6321");
      } else {
        sprintf(name, "XP1R2V_HBM");
      }
      break;
    case SMB_BMC_ADC8_VSEN:
      sprintf(name, "XP1R5V_OOB_6321");
      break;
    case SMB_SENSOR_HBM_IN_VOLT:
      if ( brd_type_rev == BOARD_WEDGE400C_MP_RESPIN ){
        sprintf(name, "VDDCK_1P15V_2_IN_VOLT(12V)");
      } else {
        sprintf(name, "XP1R2V_HBM_IN_VOLT(12V)");
      }
      break;
    case SMB_SENSOR_HBM_OUT_VOLT:
      if ( brd_type_rev == BOARD_WEDGE400C_MP_RESPIN ){
        sprintf(name, "VDDCK_1P15V_2_OUT_VOLT(1.15V)");
      } else {
        sprintf(name, "XP1R2V_HBM_OUT_VOLT(1.2V)");
      }
      break;
    case SMB_SENSOR_HBM_IN_CURR:
      sprintf(name, "XP1R2V_HBM_IN_CURR");
      break;
    case SMB_SENSOR_HBM_OUT_CURR:
      if ( brd_type_rev == BOARD_WEDGE400C_MP_RESPIN ){
        sprintf(name, "VDDCK_1P15V_2_OUT_CURR");
      } else {
        sprintf(name, "XP1R2V_HBM_OUT_CURR");
      }
      break;
    case SMB_SENSOR_HBM_IN_POWER:
      sprintf(name, "XP1R2V_HBM_IN_POWER");
      break;
    case SMB_SENSOR_HBM_OUT_POWER:
      if ( brd_type_rev == BOARD_WEDGE400C_MP_RESPIN ){
        sprintf(name, "VDDCK_1P15V_2_OUT_POWER");
      } else {
        sprintf(name, "XP1R2V_HBM_OUT_POWER");
      }
      break;
    case SMB_SENSOR_HBM_TEMP:
      if ( brd_type_rev == BOARD_WEDGE400C_MP_RESPIN ){
        sprintf(name, "VDDCK_1P15V_2_TEMP");
      } else {
        sprintf(name, "XP1R2V_HBM_TEMP");
      }
      break;
    case SMB_SENSOR_VDDCK_0_IN_VOLT:
      if( brd_type_rev == BOARD_WEDGE400C_EVT ){
        sprintf(name, "VDDCK_1P15V_IN_VOLT(12V)");
      }else if( brd_type_rev == BOARD_WEDGE400C_EVT2 ||
                brd_type_rev == BOARD_WEDGE400C_DVT ||
                brd_type_rev == BOARD_WEDGE400C_DVT2 ||
                brd_type_rev == BOARD_WEDGE400C_MP_RESPIN ){
        sprintf(name, "VDDCK_1P15V_0_IN_VOLT(12V)");
      }
      break;
    case SMB_SENSOR_VDDCK_0_OUT_VOLT:
      if( brd_type_rev == BOARD_WEDGE400C_EVT ){
        sprintf(name, "VDDCK_1P15V_OUT_VOLT(1.15V)");
      }else if( brd_type_rev == BOARD_WEDGE400C_EVT2 ||
                brd_type_rev == BOARD_WEDGE400C_DVT ||
                brd_type_rev == BOARD_WEDGE400C_DVT2 ||
                brd_type_rev == BOARD_WEDGE400C_MP_RESPIN ){
        sprintf(name, "VDDCK_1P15V_0_OUT_VOLT(1.15V)");
      }
      break;
    case SMB_SENSOR_VDDCK_0_IN_CURR:
      if( brd_type_rev == BOARD_WEDGE400C_EVT ){
        sprintf(name, "VDDCK_1P15V_IN_CURR");
      }
      break;
    case SMB_SENSOR_VDDCK_0_OUT_CURR:
      if( brd_type_rev == BOARD_WEDGE400C_EVT ){
        sprintf(name, "VDDCK_1P15V_OUT_CURR");
      }else if( brd_type_rev == BOARD_WEDGE400C_EVT2 ||
                brd_type_rev == BOARD_WEDGE400C_DVT ||
                brd_type_rev == BOARD_WEDGE400C_DVT2 ||
                brd_type_rev == BOARD_WEDGE400C_MP_RESPIN ){
        sprintf(name, "VDDCK_1P15V_0_OUT_CURR");
      }
      break;
    case SMB_SENSOR_VDDCK_0_IN_POWER:
      if( brd_type_rev == BOARD_WEDGE400C_EVT ){
        sprintf(name, "VDDCK_1P15V_IN_POWER");
      }
      break;
    case SMB_SENSOR_VDDCK_0_OUT_POWER:
      if( brd_type_rev == BOARD_WEDGE400C_EVT ){
        sprintf(name, "VDDCK_1P15V_OUT_POWER");
      }else if( brd_type_rev == BOARD_WEDGE400C_EVT2 ||
                brd_type_rev == BOARD_WEDGE400C_DVT ||
                brd_type_rev == BOARD_WEDGE400C_DVT2 ||
                brd_type_rev == BOARD_WEDGE400C_MP_RESPIN ){
        sprintf(name, "VDDCK_1P15V_0_OUT_POWER");
      }
      break;
    case SMB_SENSOR_VDDCK_0_TEMP:
      if( brd_type_rev == BOARD_WEDGE400C_EVT ){
        sprintf(name, "XP0R75V_PCIE_TEMP");
      }else if( brd_type_rev == BOARD_WEDGE400C_EVT2 ||
                brd_type_rev == BOARD_WEDGE400C_DVT ||
                brd_type_rev == BOARD_WEDGE400C_DVT2 ||
                brd_type_rev == BOARD_WEDGE400C_MP_RESPIN ){
        sprintf(name, "VDDCK_1P15V_0_TEMP");
      }
      break;
    case SMB_SENSOR_VDDCK_1_IN_VOLT:
      if( brd_type_rev == BOARD_WEDGE400C_EVT2 ||
          brd_type_rev == BOARD_WEDGE400C_DVT ||
          brd_type_rev == BOARD_WEDGE400C_DVT2 ||
          brd_type_rev == BOARD_WEDGE400C_MP_RESPIN ){
        sprintf(name, "VDDCK_1P15V_1_IN_VOLT(12V)");
      }
      break;
    case SMB_SENSOR_VDDCK_1_OUT_VOLT:
      if( brd_type_rev == BOARD_WEDGE400C_EVT2 ||
          brd_type_rev == BOARD_WEDGE400C_DVT ||
          brd_type_rev == BOARD_WEDGE400C_DVT2 ||
          brd_type_rev == BOARD_WEDGE400C_MP_RESPIN ){
        sprintf(name, "VDDCK_1P15V_1_OUT_VOLT(1.15V)");
      }
      break;
    case SMB_SENSOR_VDDCK_1_OUT_CURR:
      if( brd_type_rev == BOARD_WEDGE400C_EVT2 ||
          brd_type_rev == BOARD_WEDGE400C_DVT ||
          brd_type_rev == BOARD_WEDGE400C_DVT2 ||
          brd_type_rev == BOARD_WEDGE400C_MP_RESPIN ){
        sprintf(name, "VDDCK_1P15V_1_OUT_CURR");
      }
      break;
    case SMB_SENSOR_VDDCK_1_OUT_POWER:
      if( brd_type_rev == BOARD_WEDGE400C_EVT2 ||
          brd_type_rev == BOARD_WEDGE400C_DVT ||
          brd_type_rev == BOARD_WEDGE400C_DVT2 ||
          brd_type_rev == BOARD_WEDGE400C_MP_RESPIN ){
        sprintf(name, "VDDCK_1P15V_1_OUT_POWER");
      }
      break;
    case SMB_SENSOR_VDDCK_1_TEMP:
      if( brd_type_rev == BOARD_WEDGE400C_EVT2 ||
          brd_type_rev == BOARD_WEDGE400C_DVT ||
          brd_type_rev == BOARD_WEDGE400C_DVT2 ||
          brd_type_rev == BOARD_WEDGE400C_MP_RESPIN ){
        sprintf(name, "VDDCK_1P15V_1_TEMP");
      }
      break;
    default:
      return -1;
  }
  return 0;
}

static int
get_pem_sensor_name(uint8_t sensor_num, char *name) {

  switch(sensor_num) {
    case PEM1_SENSOR_IN_VOLT:
      sprintf(name, "PEM1_IN_VOLT");
      break;
    case PEM1_SENSOR_OUT_VOLT:
      sprintf(name, "PEM1_OUT_VOLT");
      break;
    case PEM1_SENSOR_FET_BAD:
      sprintf(name, "PEM1_FET_BAD");
      break;
    case PEM1_SENSOR_FET_SHORT:
      sprintf(name, "PEM1_FET_SHORT");
      break;
    case PEM1_SENSOR_CURR:
      sprintf(name, "PEM1_CURR");
      break;
    case PEM1_SENSOR_POWER:
      sprintf(name, "PEM1_POWER");
      break;
    case PEM1_SENSOR_FAN1_TACH:
      sprintf(name, "PEM1_FAN1_SPEED");
      break;
    case PEM1_SENSOR_FAN2_TACH:
      sprintf(name, "PEM1_FAN2_SPEED");
      break;
    case PEM1_SENSOR_TEMP1:
      sprintf(name, "PEM1_HOT_SWAP_TEMP");
      break;
    case PEM1_SENSOR_TEMP2:
      sprintf(name, "PEM1_AIR_INLET_TEMP");
      break;
    case PEM1_SENSOR_TEMP3:
      sprintf(name, "PEM1_AIR_OUTLET_TEMP");
      break;

    case PEM1_SENSOR_FAULT_OV:
      sprintf(name, "PEM1_FAULT_OV");
      break;
    case PEM1_SENSOR_FAULT_UV:
      sprintf(name, "PEM1_FAULT_UV");
      break;
    case PEM1_SENSOR_FAULT_OC:
      sprintf(name, "PEM1_FAULT_OC");
      break;
    case PEM1_SENSOR_FAULT_POWER:
      sprintf(name, "PEM1_FAULT_POWER");
      break;
    case PEM1_SENSOR_ON_FAULT:
      sprintf(name, "PEM1_FAULT_ON_PIN");
      break;
    case PEM1_SENSOR_FAULT_FET_SHORT:
      sprintf(name, "PEM1_FAULT_FET_SHORT");
      break;
    case PEM1_SENSOR_FAULT_FET_BAD:
      sprintf(name, "PEM1_FAULT_FET_BAD");
      break;
    case PEM1_SENSOR_EEPROM_DONE:
      sprintf(name, "PEM1_EEPROM_DONE");
      break;

    case PEM1_SENSOR_POWER_ALARM_HIGH:
      sprintf(name, "PEM1_POWER_ALARM_HIGH");
      break;
    case PEM1_SENSOR_POWER_ALARM_LOW:
      sprintf(name, "PEM1_POWER_ALARM_LOW");
      break;
    case PEM1_SENSOR_VSENSE_ALARM_HIGH:
      sprintf(name, "PEM1_VSENSE_ALARM_HIGH");
      break;
    case PEM1_SENSOR_VSENSE_ALARM_LOW:
      sprintf(name, "PEM1_VSENSE_ALARM_LOW");
      break;
    case PEM1_SENSOR_VSOURCE_ALARM_HIGH:
      sprintf(name, "PEM1_VSOURCE_ALARM_HIGH");
      break;
    case PEM1_SENSOR_VSOURCE_ALARM_LOW:
      sprintf(name, "PEM1_VSOURCE_ALARM_LOW");
      break;
    case PEM1_SENSOR_GPIO_ALARM_HIGH:
      sprintf(name, "PEM1_GPIO_ALARM_HIGH");
      break;
    case PEM1_SENSOR_GPIO_ALARM_LOW:
      sprintf(name, "PEM1_GPIO_ALARM_LOW");
      break;

    case PEM1_SENSOR_ON_STATUS:
      sprintf(name, "PEM1_ON_STATUS");
      break;
    case PEM1_SENSOR_STATUS_FET_BAD:
      sprintf(name, "PEM1_STATUS_FET_BAD");
      break;
    case PEM1_SENSOR_STATUS_FET_SHORT:
      sprintf(name, "PEM1_STATUS_FET_SHORT");
      break;
    case PEM1_SENSOR_STATUS_ON_PIN:
      sprintf(name, "PEM1_STATUS_ON_PIN");
      break;
    case PEM1_SENSOR_STATUS_POWER_GOOD:
      sprintf(name, "PEM1_STATUS_POWER_GOOD");
      break;
    case PEM1_SENSOR_STATUS_OC:
      sprintf(name, "PEM1_STATUS_OC");
      break;
    case PEM1_SENSOR_STATUS_UV:
      sprintf(name, "PEM1_STATUS_UV");
      break;
    case PEM1_SENSOR_STATUS_OV:
      sprintf(name, "PEM1_STATUS_OV");
      break;
    case PEM1_SENSOR_STATUS_GPIO3:
      sprintf(name, "PEM1_STATUS_GPIO3");
      break;
    case PEM1_SENSOR_STATUS_GPIO2:
      sprintf(name, "PEM1_STATUS_GPIO2");
      break;
    case PEM1_SENSOR_STATUS_GPIO1:
      sprintf(name, "PEM1_STATUS_GPIO1");
      break;
    case PEM1_SENSOR_STATUS_ALERT:
      sprintf(name, "PEM1_STATUS_ALERT");
      break;
    case PEM1_SENSOR_STATUS_EEPROM_BUSY:
      sprintf(name, "PEM1_STATUS_EEPROM_BUSY");
      break;
    case PEM1_SENSOR_STATUS_ADC_IDLE:
      sprintf(name, "PEM1_STATUS_ADC_IDLE");
      break;
    case PEM1_SENSOR_STATUS_TICKER_OVERFLOW:
      sprintf(name, "PEM1_STATUS_TICKER_OVERFLOW");
      break;
    case PEM1_SENSOR_STATUS_METER_OVERFLOW:
      sprintf(name, "PEM1_STATUS_METER_OVERFLOW");
      break;

    case PEM2_SENSOR_IN_VOLT:
      sprintf(name, "PEM2_IN_VOLT");
      break;
    case PEM2_SENSOR_OUT_VOLT:
      sprintf(name, "PEM2_OUT_VOLT");
      break;
    case PEM2_SENSOR_FET_BAD:
      sprintf(name, "PEM2_FET_BAD");
      break;
    case PEM2_SENSOR_FET_SHORT:
      sprintf(name, "PEM2_FET_SHORT");
      break;
    case PEM2_SENSOR_CURR:
      sprintf(name, "PEM2_CURR");
      break;
    case PEM2_SENSOR_POWER:
      sprintf(name, "PEM2_POWER");
      break;
    case PEM2_SENSOR_FAN1_TACH:
      sprintf(name, "PEM2_FAN1_SPEED");
      break;
    case PEM2_SENSOR_FAN2_TACH:
      sprintf(name, "PEM2_FAN2_SPEED");
      break;
    case PEM2_SENSOR_TEMP1:
      sprintf(name, "PEM2_HOT_SWAP_TEMP");
      break;
    case PEM2_SENSOR_TEMP2:
      sprintf(name, "PEM2_AIR_INLET_TEMP");
      break;
    case PEM2_SENSOR_TEMP3:
      sprintf(name, "PEM2_AIR_OUTLET_TEMP");
      break;

    case PEM2_SENSOR_FAULT_OV:
      sprintf(name, "PEM2_FAULT_OV");
      break;
    case PEM2_SENSOR_FAULT_UV:
      sprintf(name, "PEM2_FAULT_UV");
      break;
    case PEM2_SENSOR_FAULT_OC:
      sprintf(name, "PEM2_FAULT_OC");
      break;
    case PEM2_SENSOR_FAULT_POWER:
      sprintf(name, "PEM2_FAULT_POWER");
      break;
    case PEM2_SENSOR_ON_FAULT:
      sprintf(name, "PEM2_FAULT_ON_PIN");
      break;
    case PEM2_SENSOR_FAULT_FET_SHORT:
      sprintf(name, "PEM2_FAULT_FET_SHOET");
      break;
    case PEM2_SENSOR_FAULT_FET_BAD:
      sprintf(name, "PEM2_FAULT_FET_BAD");
      break;
    case PEM2_SENSOR_EEPROM_DONE:
      sprintf(name, "PEM2_EEPROM_DONE");
      break;

    case PEM2_SENSOR_POWER_ALARM_HIGH:
      sprintf(name, "PEM2_POWER_ALARM_HIGH");
      break;
    case PEM2_SENSOR_POWER_ALARM_LOW:
      sprintf(name, "PEM2_POWER_ALARM_LOW");
      break;
    case PEM2_SENSOR_VSENSE_ALARM_HIGH:
      sprintf(name, "PEM2_VSENSE_ALARM_HIGH");
      break;
    case PEM2_SENSOR_VSENSE_ALARM_LOW:
      sprintf(name, "PEM2_VSENSE_ALARM_LOW");
      break;
    case PEM2_SENSOR_VSOURCE_ALARM_HIGH:
      sprintf(name, "PEM2_VSOURCE_ALARM_HIGH");
      break;
    case PEM2_SENSOR_VSOURCE_ALARM_LOW:
      sprintf(name, "PEM2_VSOURCE_ALARM_LOW");
      break;
    case PEM2_SENSOR_GPIO_ALARM_HIGH:
      sprintf(name, "PEM2_GPIO_ALARM_HIGH");
      break;
    case PEM2_SENSOR_GPIO_ALARM_LOW:
      sprintf(name, "PEM2_GPIO_ALARM_LOW");
      break;

    case PEM2_SENSOR_ON_STATUS:
      sprintf(name, "PEM2_ON_STATUS");
      break;
    case PEM2_SENSOR_STATUS_FET_BAD:
      sprintf(name, "PEM2_STATUS_FET_BAD");
      break;
    case PEM2_SENSOR_STATUS_FET_SHORT:
      sprintf(name, "PEM2_STATUS_FET_SHORT");
      break;
    case PEM2_SENSOR_STATUS_ON_PIN:
      sprintf(name, "PEM2_STATUS_ON_PIN");
      break;
    case PEM2_SENSOR_STATUS_POWER_GOOD:
      sprintf(name, "PEM2_STATUS_POWER_GOOD");
      break;
    case PEM2_SENSOR_STATUS_OC:
      sprintf(name, "PEM2_STATUS_OC");
      break;
    case PEM2_SENSOR_STATUS_UV:
      sprintf(name, "PEM2_STATUS_UV");
      break;
    case PEM2_SENSOR_STATUS_OV:
      sprintf(name, "PEM2_STATUS_OV");
      break;
    case PEM2_SENSOR_STATUS_GPIO3:
      sprintf(name, "PEM2_STATUS_GPIO3");
      break;
    case PEM2_SENSOR_STATUS_GPIO2:
      sprintf(name, "PEM2_STATUS_GPIO2");
      break;
    case PEM2_SENSOR_STATUS_GPIO1:
      sprintf(name, "PEM2_STATUS_GPIO1");
      break;
    case PEM2_SENSOR_STATUS_ALERT:
      sprintf(name, "PEM2_STATUS_ALERT");
      break;
    case PEM2_SENSOR_STATUS_EEPROM_BUSY:
      sprintf(name, "PEM2_STATUS_EEPROM_BUSY");
      break;
    case PEM2_SENSOR_STATUS_ADC_IDLE:
      sprintf(name, "PEM2_STATUS_ADC_IDLE");
      break;
    case PEM2_SENSOR_STATUS_TICKER_OVERFLOW:
      sprintf(name, "PEM2_STATUS_TICKER_OVERFLOW");
      break;
    case PEM2_SENSOR_STATUS_METER_OVERFLOW:
      sprintf(name, "PEM2_STATUS_METER_OVERFLOW");
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
    case PSU1_SENSOR_FAN2_TACH:
      if (is_psu48())
      {
        sprintf(name, "PSU1_FAN2_SPEED");
      }
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
    case PSU2_SENSOR_FAN2_TACH:
      if (is_psu48())
      {
        sprintf(name, "PSU2_FAN2_SPEED");
      }
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
    case FRU_FAN1:
    case FRU_FAN2:
    case FRU_FAN3:
    case FRU_FAN4:
      ret = get_fan_sensor_name(sensor_num, name);
      break;
    case FRU_SMB:
      ret = get_smb_sensor_name(sensor_num, name);
      break;
    case FRU_PEM1:
    case FRU_PEM2:
      ret = get_pem_sensor_name(sensor_num, name);
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

static int
get_scm_sensor_units(uint8_t sensor_num, char *units) {

  switch(sensor_num) {
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
    case SCM_SENSOR_HSC_VOLT:
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
    case SCM_SENSOR_HSC_CURR:
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

static int
get_fan_sensor_units(uint8_t sensor_num, char *units) {

  switch(sensor_num) {
    case FAN_SENSOR_FAN1_FRONT_TACH:
    case FAN_SENSOR_FAN1_REAR_TACH:
    case FAN_SENSOR_FAN2_FRONT_TACH:
    case FAN_SENSOR_FAN2_REAR_TACH:
    case FAN_SENSOR_FAN3_FRONT_TACH:
    case FAN_SENSOR_FAN3_REAR_TACH:
    case FAN_SENSOR_FAN4_FRONT_TACH:
    case FAN_SENSOR_FAN4_REAR_TACH:
      sprintf(units, "RPM");
      break;
    default:
      return -1;
  }
  return 0;
}

static int
get_smb_sensor_units(uint8_t sensor_num, char *units) {

  switch(sensor_num) {
    case SMB_SENSOR_SW_SERDES_PVDD_TEMP1:
    case SMB_SENSOR_SW_SERDES_TRVDD_TEMP1:
    case SMB_SENSOR_IR3R3V_LEFT_TEMP:
    case SMB_SENSOR_IR3R3V_RIGHT_TEMP:
    case SMB_SENSOR_SW_CORE_TEMP1:
    case SMB_SENSOR_LM75B_U28_TEMP:
    case SMB_SENSOR_LM75B_U25_TEMP:
    case SMB_SENSOR_LM75B_U56_TEMP:
    case SMB_SENSOR_LM75B_U55_TEMP:
    case SMB_SENSOR_TMP421_U62_TEMP:
    case SMB_SENSOR_TMP421_Q9_TEMP:
    case SMB_SENSOR_TMP421_U63_TEMP:
    case SMB_SENSOR_TMP421_Q10_TEMP:
    case SMB_SENSOR_TMP422_U20_TEMP:
    case SMB_SENSOR_SW_DIE_TEMP1:
    case SMB_SENSOR_SW_DIE_TEMP2:
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
    case SMB_SENSOR_FCM_LM75B_U1_TEMP:
    case SMB_SENSOR_FCM_LM75B_U2_TEMP:
    case SMB_DOM1_MAX_TEMP:
    case SMB_DOM2_MAX_TEMP:
    case SMB_SENSOR_HBM_TEMP:
    case SMB_SENSOR_VDDCK_0_TEMP:
    case SMB_SENSOR_VDDCK_1_TEMP:
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
    case SMB_SENSOR_SW_SERDES_PVDD_IN_VOLT:
    case SMB_SENSOR_SW_SERDES_PVDD_OUT_VOLT:
    case SMB_SENSOR_SW_SERDES_TRVDD_IN_VOLT:
    case SMB_SENSOR_SW_SERDES_TRVDD_OUT_VOLT:
    case SMB_SENSOR_IR3R3V_LEFT_IN_VOLT:
    case SMB_SENSOR_IR3R3V_LEFT_OUT_VOLT:
    case SMB_SENSOR_IR3R3V_RIGHT_IN_VOLT:
    case SMB_SENSOR_IR3R3V_RIGHT_OUT_VOLT:
    case SMB_SENSOR_SW_CORE_VOLT:
    case SMB_SENSOR_FCM_HSC_VOLT:
    case SMB_BMC_ADC0_VSEN:
    case SMB_BMC_ADC1_VSEN:
    case SMB_BMC_ADC2_VSEN:
    case SMB_BMC_ADC3_VSEN:
    case SMB_BMC_ADC4_VSEN:
    case SMB_BMC_ADC5_VSEN:
    case SMB_BMC_ADC6_VSEN:
    case SMB_BMC_ADC7_VSEN:
    case SMB_BMC_ADC8_VSEN:
    case SMB_SENSOR_HBM_IN_VOLT:
    case SMB_SENSOR_HBM_OUT_VOLT:
    case SMB_SENSOR_VDDCK_0_IN_VOLT:
    case SMB_SENSOR_VDDCK_0_OUT_VOLT:
    case SMB_SENSOR_VDDCK_1_IN_VOLT:
    case SMB_SENSOR_VDDCK_1_OUT_VOLT:
      sprintf(units, "Volts");
      break;
    case SMB_SENSOR_SW_SERDES_PVDD_IN_CURR:
    case SMB_SENSOR_SW_SERDES_PVDD_OUT_CURR:
    case SMB_SENSOR_SW_SERDES_TRVDD_IN_CURR:
    case SMB_SENSOR_SW_SERDES_TRVDD_OUT_CURR:
    case SMB_SENSOR_IR3R3V_LEFT_IN_CURR:
    case SMB_SENSOR_IR3R3V_LEFT_OUT_CURR:
    case SMB_SENSOR_IR3R3V_RIGHT_IN_CURR:
    case SMB_SENSOR_IR3R3V_RIGHT_OUT_CURR:
    case SMB_SENSOR_SW_CORE_CURR:
    case SMB_SENSOR_FCM_HSC_CURR:
    case SMB_SENSOR_HBM_IN_CURR:
    case SMB_SENSOR_HBM_OUT_CURR:
    case SMB_SENSOR_VDDCK_0_IN_CURR:
    case SMB_SENSOR_VDDCK_0_OUT_CURR:
    case SMB_SENSOR_VDDCK_1_OUT_CURR:
      sprintf(units, "Amps");
      break;
    case SMB_SENSOR_SW_SERDES_PVDD_IN_POWER:
    case SMB_SENSOR_SW_SERDES_PVDD_OUT_POWER:
    case SMB_SENSOR_SW_SERDES_TRVDD_IN_POWER:
    case SMB_SENSOR_SW_SERDES_TRVDD_OUT_POWER:
    case SMB_SENSOR_IR3R3V_LEFT_IN_POWER:
    case SMB_SENSOR_IR3R3V_LEFT_OUT_POWER:
    case SMB_SENSOR_IR3R3V_RIGHT_IN_POWER:
    case SMB_SENSOR_IR3R3V_RIGHT_OUT_POWER:
    case SMB_SENSOR_SW_CORE_POWER:
    case SMB_SENSOR_HBM_IN_POWER:
    case SMB_SENSOR_HBM_OUT_POWER:
    case SMB_SENSOR_VDDCK_0_IN_POWER:
    case SMB_SENSOR_VDDCK_0_OUT_POWER:
    case SMB_SENSOR_VDDCK_1_OUT_POWER:
      sprintf(units, "Watts");
      break;
    default:
      return -1;
  }
  return 0;
}

static int
get_pem_sensor_units(uint8_t sensor_num, char *units) {

  switch(sensor_num) {
    case PEM1_SENSOR_IN_VOLT:
    case PEM1_SENSOR_OUT_VOLT:
    case PEM1_SENSOR_FET_BAD:
    case PEM1_SENSOR_FET_SHORT:
    case PEM2_SENSOR_IN_VOLT:
    case PEM2_SENSOR_OUT_VOLT:
    case PEM2_SENSOR_FET_BAD:
    case PEM2_SENSOR_FET_SHORT:
      sprintf(units, "Volts");
      break;
    case PEM1_SENSOR_CURR:
    case PEM2_SENSOR_CURR:
      sprintf(units, "Amps");
      break;
    case PEM1_SENSOR_POWER:
    case PEM2_SENSOR_POWER:
      sprintf(units, "Watts");
      break;
    case PEM1_SENSOR_FAN1_TACH:
    case PEM1_SENSOR_FAN2_TACH:
    case PEM2_SENSOR_FAN1_TACH:
    case PEM2_SENSOR_FAN2_TACH:
      sprintf(units, "RPM");
      break;
    case PEM1_SENSOR_TEMP1:
    case PEM1_SENSOR_TEMP2:
    case PEM1_SENSOR_TEMP3:
    case PEM2_SENSOR_TEMP1:
    case PEM2_SENSOR_TEMP2:
    case PEM2_SENSOR_TEMP3:
      sprintf(units, "C");
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
    case PSU1_SENSOR_FAN2_TACH:
    case PSU2_SENSOR_FAN_TACH:
    case PSU2_SENSOR_FAN2_TACH:
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

int
pal_get_sensor_units(uint8_t fru, uint8_t sensor_num, char *units) {
  int ret = -1;

  switch(fru) {
    case FRU_SCM:
      ret = get_scm_sensor_units(sensor_num, units);
      break;
    case FRU_FAN1:
    case FRU_FAN2:
    case FRU_FAN3:
    case FRU_FAN4:
      ret = get_fan_sensor_units(sensor_num, units);
      break;
    case FRU_SMB:
      ret = get_smb_sensor_units(sensor_num, units);
      break;
    case FRU_PEM1:
    case FRU_PEM2:
      ret = get_pem_sensor_units(sensor_num, units);
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

static void
sensor_thresh_array_init(uint8_t fru) {
  static bool init_done[MAX_NUM_FRUS] = {false};
  int fru_offset;
  float fvalue;
  uint8_t brd_type;
  uint8_t brd_type_rev;
  int scm_sensor_cnt;
  int scm_all_sensor_cnt;
  uint8_t *scm_sensor_list;
  if(pal_get_board_type(&brd_type)){
    return;
  }
  if(pal_get_board_type_rev(&brd_type_rev)){
    return;
  }
  if (init_done[fru])
    return;

  switch (fru) {
    case FRU_SCM:
      scm_sensor_threshold[SCM_SENSOR_OUTLET_TEMP][UCR_THRESH] = 80;
      scm_sensor_threshold[SCM_SENSOR_INLET_TEMP][UCR_THRESH] = 80;
      scm_sensor_threshold[SCM_SENSOR_HSC_VOLT][UCR_THRESH] = 14.13;
      scm_sensor_threshold[SCM_SENSOR_HSC_VOLT][LCR_THRESH] = 7.5;
      scm_sensor_threshold[SCM_SENSOR_HSC_CURR][UCR_THRESH] = 24.7;
      
      if (( brd_type == BRD_TYPE_WEDGE400 && brd_type_rev >= BOARD_WEDGE400_MP_RESPIN ) ||
          ( brd_type == BRD_TYPE_WEDGE400C && brd_type_rev >= BOARD_WEDGE400C_MP_RESPIN ))
      {
        scm_sensor_cnt = scm_mp_respin_sensor_cnt;
        scm_all_sensor_cnt = scm_mp_respin_all_sensor_cnt;
        scm_sensor_list = (uint8_t *) scm_mp_respin_all_sensor_list;
      } else {
        scm_sensor_cnt = scm_evt_dvt_mp_sensor_cnt;
        scm_all_sensor_cnt= scm_evt_dvt_mp_all_sensor_cnt;
        scm_sensor_list = (uint8_t *) scm_evt_dvt_mp_all_sensor_list;
      }

      for (int sensor_index = scm_sensor_cnt; sensor_index < scm_all_sensor_cnt; sensor_index++) {
        for (int threshold_type = 1; threshold_type <= MAX_SENSOR_THRESHOLD; threshold_type++) {
            if (!bic_get_sdr_thresh_val(fru, scm_sensor_list[sensor_index], threshold_type, &fvalue)) {
              scm_sensor_threshold[scm_sensor_list[sensor_index]][threshold_type] = fvalue;
            }        
        }
      }
      break;
    case FRU_FAN1:
    case FRU_FAN2:
    case FRU_FAN3:
    case FRU_FAN4:
      fan_sensor_threshold[FAN_SENSOR_FAN1_FRONT_TACH][UCR_THRESH] = 15000;
      fan_sensor_threshold[FAN_SENSOR_FAN1_FRONT_TACH][LCR_THRESH] = 1000;
      fan_sensor_threshold[FAN_SENSOR_FAN1_REAR_TACH][UCR_THRESH] = 15000;
      fan_sensor_threshold[FAN_SENSOR_FAN1_REAR_TACH][LCR_THRESH] = 1000;
      fan_sensor_threshold[FAN_SENSOR_FAN2_FRONT_TACH][UCR_THRESH] = 15000;
      fan_sensor_threshold[FAN_SENSOR_FAN2_FRONT_TACH][LCR_THRESH] = 1000;
      fan_sensor_threshold[FAN_SENSOR_FAN2_REAR_TACH][UCR_THRESH] = 15000;
      fan_sensor_threshold[FAN_SENSOR_FAN2_REAR_TACH][LCR_THRESH] = 1000;
      fan_sensor_threshold[FAN_SENSOR_FAN3_FRONT_TACH][UCR_THRESH] = 15000;
      fan_sensor_threshold[FAN_SENSOR_FAN3_FRONT_TACH][LCR_THRESH] = 1000;
      fan_sensor_threshold[FAN_SENSOR_FAN3_REAR_TACH][UCR_THRESH] = 15000;
      fan_sensor_threshold[FAN_SENSOR_FAN3_REAR_TACH][LCR_THRESH] = 1000;
      fan_sensor_threshold[FAN_SENSOR_FAN4_FRONT_TACH][UCR_THRESH] = 15000;
      fan_sensor_threshold[FAN_SENSOR_FAN4_FRONT_TACH][LCR_THRESH] = 1000;
      fan_sensor_threshold[FAN_SENSOR_FAN4_REAR_TACH][UCR_THRESH] = 15000;
      fan_sensor_threshold[FAN_SENSOR_FAN4_REAR_TACH][LCR_THRESH] = 1000;
      break;
    case FRU_SMB:
      smb_sensor_threshold[SMB_SENSOR_1220_VMON1][UCR_THRESH] = 13.2;
      smb_sensor_threshold[SMB_SENSOR_1220_VMON1][LCR_THRESH] = 10.8;
      smb_sensor_threshold[SMB_SENSOR_1220_VMON2][UCR_THRESH] = 5.5;
      smb_sensor_threshold[SMB_SENSOR_1220_VMON2][LCR_THRESH] = 4.5;
      smb_sensor_threshold[SMB_SENSOR_1220_VMON3][UCR_THRESH] = 3.564;
      smb_sensor_threshold[SMB_SENSOR_1220_VMON3][LCR_THRESH] = 3.036;
      smb_sensor_threshold[SMB_SENSOR_1220_VMON5][UCR_THRESH] = 1.296;
      smb_sensor_threshold[SMB_SENSOR_1220_VMON5][LCR_THRESH] = 1.104;
      smb_sensor_threshold[SMB_SENSOR_1220_VCCA][UCR_THRESH] = 3.564;
      smb_sensor_threshold[SMB_SENSOR_1220_VCCA][LCR_THRESH] = 3.036;
      smb_sensor_threshold[SMB_SENSOR_1220_VCCINP][UCR_THRESH] = 3.564;
      smb_sensor_threshold[SMB_SENSOR_1220_VCCINP][LCR_THRESH] = 3.036;
      if(brd_type == BRD_TYPE_WEDGE400){
        smb_sensor_threshold[SMB_SENSOR_1220_VMON4][UCR_THRESH] = 2.7;
        smb_sensor_threshold[SMB_SENSOR_1220_VMON4][LCR_THRESH] = 2.3;
        smb_sensor_threshold[SMB_SENSOR_1220_VMON6][UCR_THRESH] = 1.242;
        smb_sensor_threshold[SMB_SENSOR_1220_VMON6][LCR_THRESH] = 1.058;
        smb_sensor_threshold[SMB_SENSOR_1220_VMON7][UCR_THRESH] = 1.296;
        smb_sensor_threshold[SMB_SENSOR_1220_VMON7][LCR_THRESH] = 1.104;
        smb_sensor_threshold[SMB_SENSOR_1220_VMON8][UCR_THRESH] = 0.88;
        smb_sensor_threshold[SMB_SENSOR_1220_VMON8][LCR_THRESH] = 0.72;
        smb_sensor_threshold[SMB_SENSOR_1220_VMON9][UCR_THRESH] = 3.564;
        smb_sensor_threshold[SMB_SENSOR_1220_VMON9][LCR_THRESH] = 3.036;
        smb_sensor_threshold[SMB_SENSOR_1220_VMON10][UCR_THRESH] = 0.99;
        smb_sensor_threshold[SMB_SENSOR_1220_VMON10][LCR_THRESH] = 0.675;
        smb_sensor_threshold[SMB_SENSOR_1220_VMON11][UCR_THRESH] = 0.864;
        smb_sensor_threshold[SMB_SENSOR_1220_VMON11][LCR_THRESH] = 0.736;
        smb_sensor_threshold[SMB_SENSOR_1220_VMON12][UCR_THRESH] = 1.944;
        smb_sensor_threshold[SMB_SENSOR_1220_VMON12][LCR_THRESH] = 1.656;
      } else if (brd_type == BRD_TYPE_WEDGE400C){
        smb_sensor_threshold[SMB_SENSOR_1220_VMON4][UCR_THRESH] = 3.564;
        smb_sensor_threshold[SMB_SENSOR_1220_VMON4][LCR_THRESH] = 3.036;
        smb_sensor_threshold[SMB_SENSOR_1220_VMON6][UCR_THRESH] = 1.944;
        smb_sensor_threshold[SMB_SENSOR_1220_VMON6][LCR_THRESH] = 1.656;
        smb_sensor_threshold[SMB_SENSOR_1220_VMON7][UCR_THRESH] = 1.98;
        smb_sensor_threshold[SMB_SENSOR_1220_VMON7][LCR_THRESH] = 1.62;
        smb_sensor_threshold[SMB_SENSOR_1220_VMON8][UCR_THRESH] = 2.75;
        smb_sensor_threshold[SMB_SENSOR_1220_VMON8][LCR_THRESH] = 2.25;
        smb_sensor_threshold[SMB_SENSOR_1220_VMON9][UCR_THRESH] = 1.034;
        smb_sensor_threshold[SMB_SENSOR_1220_VMON9][LCR_THRESH] = 0.825;
        if(brd_type_rev == BOARD_WEDGE400C_EVT){
          smb_sensor_threshold[SMB_SENSOR_1220_VMON10][UCR_THRESH] = 0.935;
          smb_sensor_threshold[SMB_SENSOR_1220_VMON10][LCR_THRESH] = 0.765;
        }
        else if(brd_type_rev == BOARD_WEDGE400C_EVT2 ||
                brd_type_rev == BOARD_WEDGE400C_DVT ||
                brd_type_rev == BOARD_WEDGE400C_DVT2 ||
                brd_type_rev == BOARD_WEDGE400C_MP_RESPIN ){
          smb_sensor_threshold[SMB_SENSOR_1220_VMON10][UCR_THRESH] = 0.94;
          smb_sensor_threshold[SMB_SENSOR_1220_VMON10][LCR_THRESH] = 0.675;
        }
        smb_sensor_threshold[SMB_SENSOR_1220_VMON11][UCR_THRESH] = 0.825;
        smb_sensor_threshold[SMB_SENSOR_1220_VMON11][LCR_THRESH] = 0.675;
        smb_sensor_threshold[SMB_SENSOR_1220_VMON12][UCR_THRESH] = 1.265;
        smb_sensor_threshold[SMB_SENSOR_1220_VMON12][LCR_THRESH] = 1.035;
      }

      /* BMC ADC Sensors */
      smb_sensor_threshold[SMB_BMC_ADC0_VSEN][UCR_THRESH] = 1.14;
      smb_sensor_threshold[SMB_BMC_ADC0_VSEN][LCR_THRESH] = 0.94;
      smb_sensor_threshold[SMB_BMC_ADC3_VSEN][UCR_THRESH] = 3.564;
      smb_sensor_threshold[SMB_BMC_ADC3_VSEN][LCR_THRESH] = 3.036;
      smb_sensor_threshold[SMB_BMC_ADC4_VSEN][UCR_THRESH] = 3.564;
      smb_sensor_threshold[SMB_BMC_ADC4_VSEN][LCR_THRESH] = 3.036;

      if (brd_type == BRD_TYPE_WEDGE400) {
        smb_sensor_threshold[SMB_BMC_ADC1_VSEN][UCR_THRESH] = 1.944;
        smb_sensor_threshold[SMB_BMC_ADC1_VSEN][LCR_THRESH] = 1.656;
        smb_sensor_threshold[SMB_BMC_ADC2_VSEN][UCR_THRESH] = 3.564;
        smb_sensor_threshold[SMB_BMC_ADC2_VSEN][LCR_THRESH] = 3.036;
      } else if (brd_type == BRD_TYPE_WEDGE400C){
        /* BMC ADC Sensors */
        smb_sensor_threshold[SMB_BMC_ADC1_VSEN][UCR_THRESH] = 2.7;
        smb_sensor_threshold[SMB_BMC_ADC1_VSEN][LCR_THRESH] = 2.3;
        smb_sensor_threshold[SMB_BMC_ADC2_VSEN][UCR_THRESH] = 1.242;
        smb_sensor_threshold[SMB_BMC_ADC2_VSEN][LCR_THRESH] = 1.058;
        if (brd_type_rev == BOARD_WEDGE400C_MP_RESPIN) {
          smb_sensor_threshold[SMB_BMC_ADC5_VSEN][UCR_THRESH] = 1.98;
          smb_sensor_threshold[SMB_BMC_ADC5_VSEN][LCR_THRESH] = 1.62;
          smb_sensor_threshold[SMB_BMC_ADC6_VSEN][UCR_THRESH] = 1.32;
          smb_sensor_threshold[SMB_BMC_ADC6_VSEN][LCR_THRESH] = 1.08;
          smb_sensor_threshold[SMB_BMC_ADC7_VSEN][UCR_THRESH] = 1.21;
          smb_sensor_threshold[SMB_BMC_ADC7_VSEN][LCR_THRESH] = 1.0;
          smb_sensor_threshold[SMB_BMC_ADC8_VSEN][UCR_THRESH] = 1.62;
          smb_sensor_threshold[SMB_BMC_ADC8_VSEN][LCR_THRESH] = 1.38;
        }
      }

      /* IR35215 */
      if (brd_type == BRD_TYPE_WEDGE400){
        /* PVDD */
        smb_sensor_threshold[SMB_SENSOR_SW_SERDES_PVDD_IN_VOLT][UCR_THRESH] = 13.2;
        smb_sensor_threshold[SMB_SENSOR_SW_SERDES_PVDD_IN_VOLT][LCR_THRESH] = 10.8;
        smb_sensor_threshold[SMB_SENSOR_SW_SERDES_PVDD_IN_CURR][UCR_THRESH] = 42.9;
        smb_sensor_threshold[SMB_SENSOR_SW_SERDES_PVDD_OUT_VOLT][UCR_THRESH] = 0.88;
        smb_sensor_threshold[SMB_SENSOR_SW_SERDES_PVDD_OUT_VOLT][LCR_THRESH] = 0.72;
        smb_sensor_threshold[SMB_SENSOR_SW_SERDES_PVDD_OUT_CURR][UCR_THRESH] = 90;
        smb_sensor_threshold[SMB_SENSOR_SW_SERDES_PVDD_OUT_POWER][UCR_THRESH] = 80;
        smb_sensor_threshold[SMB_SENSOR_SW_SERDES_PVDD_TEMP1][UCR_THRESH] = 125;
        smb_sensor_threshold[SMB_SENSOR_SW_SERDES_PVDD_TEMP1][LCR_THRESH] = -40;
        /* TRVDD */
        smb_sensor_threshold[SMB_SENSOR_SW_SERDES_TRVDD_IN_VOLT][UCR_THRESH] = 13.2;
        smb_sensor_threshold[SMB_SENSOR_SW_SERDES_TRVDD_IN_VOLT][LCR_THRESH] = 10.8;
        smb_sensor_threshold[SMB_SENSOR_SW_SERDES_TRVDD_IN_CURR][UCR_THRESH] = 49.5;
        smb_sensor_threshold[SMB_SENSOR_SW_SERDES_TRVDD_OUT_VOLT][UCR_THRESH] = 0.88;
        smb_sensor_threshold[SMB_SENSOR_SW_SERDES_TRVDD_OUT_VOLT][LCR_THRESH] = 0.72;
        smb_sensor_threshold[SMB_SENSOR_SW_SERDES_TRVDD_OUT_CURR][UCR_THRESH] = 90;
        smb_sensor_threshold[SMB_SENSOR_SW_SERDES_TRVDD_OUT_POWER][UCR_THRESH] = 80;
        smb_sensor_threshold[SMB_SENSOR_SW_SERDES_TRVDD_TEMP1][UCR_THRESH] = 125;
        smb_sensor_threshold[SMB_SENSOR_SW_SERDES_TRVDD_TEMP1][LCR_THRESH] = -40;
      } else if (brd_type == BRD_TYPE_WEDGE400C){
        /* XP0R94V_VDDA */
        smb_sensor_threshold[SMB_SENSOR_SW_SERDES_PVDD_IN_VOLT][UCR_THRESH] = 13.2;
        smb_sensor_threshold[SMB_SENSOR_SW_SERDES_PVDD_IN_VOLT][LCR_THRESH] = 10.8;
        smb_sensor_threshold[SMB_SENSOR_SW_SERDES_PVDD_OUT_VOLT][UCR_THRESH] = 1.056;
        smb_sensor_threshold[SMB_SENSOR_SW_SERDES_PVDD_OUT_VOLT][LCR_THRESH] = 0.833;
        smb_sensor_threshold[SMB_SENSOR_SW_SERDES_PVDD_IN_CURR][UCR_THRESH] = 5.3;
        smb_sensor_threshold[SMB_SENSOR_SW_SERDES_PVDD_OUT_CURR][UCR_THRESH] = 47;
        smb_sensor_threshold[SMB_SENSOR_SW_SERDES_PVDD_IN_POWER][UCR_THRESH] = 63;
        smb_sensor_threshold[SMB_SENSOR_SW_SERDES_PVDD_OUT_POWER][UCR_THRESH] = 44;
        smb_sensor_threshold[SMB_SENSOR_SW_SERDES_PVDD_TEMP1][UCR_THRESH] = 85;
        smb_sensor_threshold[SMB_SENSOR_SW_SERDES_PVDD_TEMP1][LCR_THRESH] = -40;
        /* XP0R75V_PCIE */
        smb_sensor_threshold[SMB_SENSOR_SW_SERDES_TRVDD_IN_VOLT][UCR_THRESH] = 13.2;
        smb_sensor_threshold[SMB_SENSOR_SW_SERDES_TRVDD_IN_VOLT][LCR_THRESH] = 10.8;
        smb_sensor_threshold[SMB_SENSOR_SW_SERDES_TRVDD_OUT_VOLT][UCR_THRESH] = 0.869;
        smb_sensor_threshold[SMB_SENSOR_SW_SERDES_TRVDD_OUT_VOLT][LCR_THRESH] = 0.641;
        smb_sensor_threshold[SMB_SENSOR_SW_SERDES_TRVDD_IN_CURR][UCR_THRESH] = 4;
        smb_sensor_threshold[SMB_SENSOR_SW_SERDES_TRVDD_OUT_CURR][UCR_THRESH] = 45;
        smb_sensor_threshold[SMB_SENSOR_SW_SERDES_TRVDD_IN_POWER][UCR_THRESH] = 49;
        smb_sensor_threshold[SMB_SENSOR_SW_SERDES_TRVDD_OUT_POWER][UCR_THRESH] = 34;
        smb_sensor_threshold[SMB_SENSOR_SW_SERDES_TRVDD_TEMP1][UCR_THRESH] = 85;
        smb_sensor_threshold[SMB_SENSOR_SW_SERDES_TRVDD_TEMP1][LCR_THRESH] = -40;
      }

      /* IR3R3V LEFT-RIGHT*/
      smb_sensor_threshold[SMB_SENSOR_IR3R3V_LEFT_IN_VOLT][UCR_THRESH] = 13.2;
      smb_sensor_threshold[SMB_SENSOR_IR3R3V_LEFT_IN_VOLT][LCR_THRESH] = 10.8;
      smb_sensor_threshold[SMB_SENSOR_IR3R3V_LEFT_IN_CURR][UCR_THRESH] = 15;
      smb_sensor_threshold[SMB_SENSOR_IR3R3V_LEFT_IN_POWER][UCR_THRESH] = 180;
      smb_sensor_threshold[SMB_SENSOR_IR3R3V_LEFT_OUT_VOLT][UCR_THRESH] = 3.564;
      smb_sensor_threshold[SMB_SENSOR_IR3R3V_LEFT_OUT_VOLT][LCR_THRESH] = 3.036;
      smb_sensor_threshold[SMB_SENSOR_IR3R3V_LEFT_OUT_CURR][UCR_THRESH] = 49;
      smb_sensor_threshold[SMB_SENSOR_IR3R3V_LEFT_OUT_POWER][UCR_THRESH] = 169;
      smb_sensor_threshold[SMB_SENSOR_IR3R3V_LEFT_TEMP][UCR_THRESH] = 85;
      smb_sensor_threshold[SMB_SENSOR_IR3R3V_LEFT_TEMP][LCR_THRESH] = -40;
      smb_sensor_threshold[SMB_SENSOR_IR3R3V_RIGHT_IN_VOLT][UCR_THRESH] = 13.2;
      smb_sensor_threshold[SMB_SENSOR_IR3R3V_RIGHT_IN_VOLT][LCR_THRESH] = 10.8;
      smb_sensor_threshold[SMB_SENSOR_IR3R3V_RIGHT_IN_CURR][UCR_THRESH] = 15;
      smb_sensor_threshold[SMB_SENSOR_IR3R3V_RIGHT_IN_POWER][UCR_THRESH] = 180;
      smb_sensor_threshold[SMB_SENSOR_IR3R3V_RIGHT_OUT_VOLT][UCR_THRESH] = 3.564;
      smb_sensor_threshold[SMB_SENSOR_IR3R3V_RIGHT_OUT_VOLT][LCR_THRESH] = 3.036;
      smb_sensor_threshold[SMB_SENSOR_IR3R3V_RIGHT_OUT_CURR][UCR_THRESH] = 49;
      smb_sensor_threshold[SMB_SENSOR_IR3R3V_RIGHT_OUT_POWER][UCR_THRESH] = 169;
      smb_sensor_threshold[SMB_SENSOR_IR3R3V_RIGHT_TEMP][UCR_THRESH] = 85;
      smb_sensor_threshold[SMB_SENSOR_IR3R3V_RIGHT_TEMP][LCR_THRESH] = -40;

      /* SW CORE*/
      if (brd_type == BRD_TYPE_WEDGE400){ //TH_CORE 0.75-0.9
        smb_sensor_threshold[SMB_SENSOR_SW_CORE_VOLT][UCR_THRESH] = 0.99;
        smb_sensor_threshold[SMB_SENSOR_SW_CORE_VOLT][LCR_THRESH] = 0.675;
      } else if (brd_type == BRD_TYPE_WEDGE400C) { //VDD_CORE 0.82
          smb_sensor_threshold[SMB_SENSOR_SW_CORE_VOLT][UCR_THRESH] = 0.94;
          smb_sensor_threshold[SMB_SENSOR_SW_CORE_VOLT][LCR_THRESH] = 0.675;
      }
      if ( brd_type == BRD_TYPE_WEDGE400 ){
        smb_sensor_threshold[SMB_SENSOR_SW_CORE_CURR][UCR_THRESH] = 450;
        smb_sensor_threshold[SMB_SENSOR_SW_CORE_POWER][UCR_THRESH] = 300;
      }
      else if ( brd_type == BRD_TYPE_WEDGE400C ){
        smb_sensor_threshold[SMB_SENSOR_SW_CORE_CURR][UCR_THRESH] = 370;
        smb_sensor_threshold[SMB_SENSOR_SW_CORE_POWER][UCR_THRESH] = 278;
      }
      smb_sensor_threshold[SMB_SENSOR_SW_CORE_TEMP1][UCR_THRESH] = 125;

      /* HBM for Wedge400C only*/
      if (brd_type == BRD_TYPE_WEDGE400C) {
        if (brd_type_rev == BOARD_WEDGE400C_EVT){
          smb_sensor_threshold[SMB_SENSOR_HBM_IN_VOLT][UCR_THRESH] = 15;
          smb_sensor_threshold[SMB_SENSOR_HBM_IN_VOLT][LCR_THRESH] = 4.5;
          smb_sensor_threshold[SMB_SENSOR_HBM_OUT_VOLT][UCR_THRESH] = 1.353;
          smb_sensor_threshold[SMB_SENSOR_HBM_OUT_VOLT][LCR_THRESH] = 1.053;
          smb_sensor_threshold[SMB_SENSOR_HBM_IN_CURR][UCR_THRESH] = 3;
          smb_sensor_threshold[SMB_SENSOR_HBM_OUT_CURR][UCR_THRESH] = 27;
          smb_sensor_threshold[SMB_SENSOR_HBM_IN_POWER][UCR_THRESH] = 40.15;
          smb_sensor_threshold[SMB_SENSOR_HBM_OUT_POWER][UCR_THRESH] = 36.5;
          smb_sensor_threshold[SMB_SENSOR_HBM_TEMP][UCR_THRESH] = 105;
          smb_sensor_threshold[SMB_SENSOR_VDDCK_0_IN_VOLT][UCR_THRESH] = 15;
          smb_sensor_threshold[SMB_SENSOR_VDDCK_0_IN_VOLT][LCR_THRESH] = 4.5;
          smb_sensor_threshold[SMB_SENSOR_VDDCK_0_OUT_VOLT][UCR_THRESH] = 1.298;
          smb_sensor_threshold[SMB_SENSOR_VDDCK_0_OUT_VOLT][LCR_THRESH] = 1.008;
          smb_sensor_threshold[SMB_SENSOR_VDDCK_0_IN_CURR][UCR_THRESH] = 23.1;
          smb_sensor_threshold[SMB_SENSOR_VDDCK_0_OUT_CURR][UCR_THRESH] = 21;
          smb_sensor_threshold[SMB_SENSOR_VDDCK_0_IN_POWER][UCR_THRESH] = 30;
          smb_sensor_threshold[SMB_SENSOR_VDDCK_0_OUT_POWER][UCR_THRESH] = 27.26;
          smb_sensor_threshold[SMB_SENSOR_VDDCK_0_TEMP][UCR_THRESH] = 105;
        } else if (brd_type_rev == BOARD_WEDGE400C_EVT2 ||
                   brd_type_rev == BOARD_WEDGE400C_DVT ||
                   brd_type_rev == BOARD_WEDGE400C_DVT2 ||
                brd_type_rev == BOARD_WEDGE400C_MP_RESPIN ){
          smb_sensor_threshold[SMB_SENSOR_VDDCK_0_IN_VOLT][UCR_THRESH] = 14.5;
          smb_sensor_threshold[SMB_SENSOR_VDDCK_0_IN_VOLT][LCR_THRESH] = 10.5;
          smb_sensor_threshold[SMB_SENSOR_VDDCK_0_OUT_VOLT][UCR_THRESH] = 1.298;
          smb_sensor_threshold[SMB_SENSOR_VDDCK_0_OUT_VOLT][LCR_THRESH] = 1.008;
          smb_sensor_threshold[SMB_SENSOR_VDDCK_0_OUT_CURR][UCR_THRESH] = 8.5;
          smb_sensor_threshold[SMB_SENSOR_VDDCK_0_OUT_CURR][LCR_THRESH] = 0;
          smb_sensor_threshold[SMB_SENSOR_VDDCK_0_OUT_POWER][UCR_THRESH] = 10;
          smb_sensor_threshold[SMB_SENSOR_VDDCK_0_OUT_POWER][LCR_THRESH] = 0;
          smb_sensor_threshold[SMB_SENSOR_VDDCK_0_TEMP][UCR_THRESH] = 85;
          smb_sensor_threshold[SMB_SENSOR_VDDCK_0_TEMP][LCR_THRESH] = -5;
          smb_sensor_threshold[SMB_SENSOR_VDDCK_1_IN_VOLT][UCR_THRESH] = 14.5;
          smb_sensor_threshold[SMB_SENSOR_VDDCK_1_IN_VOLT][LCR_THRESH] = 10.5;
          smb_sensor_threshold[SMB_SENSOR_VDDCK_1_OUT_VOLT][UCR_THRESH] = 1.298;
          smb_sensor_threshold[SMB_SENSOR_VDDCK_1_OUT_VOLT][LCR_THRESH] = 1.008;
          smb_sensor_threshold[SMB_SENSOR_VDDCK_1_OUT_CURR][UCR_THRESH] = 8.5;
          smb_sensor_threshold[SMB_SENSOR_VDDCK_1_OUT_CURR][LCR_THRESH] = 0;
          smb_sensor_threshold[SMB_SENSOR_VDDCK_1_OUT_POWER][UCR_THRESH] = 10;
          smb_sensor_threshold[SMB_SENSOR_VDDCK_1_OUT_POWER][LCR_THRESH] = 0;
          smb_sensor_threshold[SMB_SENSOR_VDDCK_1_TEMP][UCR_THRESH] = 85;
          smb_sensor_threshold[SMB_SENSOR_VDDCK_1_TEMP][LCR_THRESH] = -5;
        }
      }
      /* SMB TEMP Sensors */
      smb_sensor_threshold[SMB_SENSOR_LM75B_U28_TEMP][UCR_THRESH] = 80;
      smb_sensor_threshold[SMB_SENSOR_LM75B_U25_TEMP][UCR_THRESH] = 80;
      smb_sensor_threshold[SMB_SENSOR_LM75B_U56_TEMP][UCR_THRESH] = 80;
      smb_sensor_threshold[SMB_SENSOR_LM75B_U55_TEMP][UCR_THRESH] = 80;
      smb_sensor_threshold[SMB_SENSOR_TMP421_U62_TEMP][UCR_THRESH] = 80;
      smb_sensor_threshold[SMB_SENSOR_TMP421_Q9_TEMP][UCR_THRESH] = 80;
      smb_sensor_threshold[SMB_SENSOR_TMP421_U63_TEMP][UCR_THRESH] = 80;
      smb_sensor_threshold[SMB_SENSOR_TMP421_Q10_TEMP][UCR_THRESH] = 80;
      smb_sensor_threshold[SMB_SENSOR_TMP422_U20_TEMP][UCR_THRESH] = 80;
      if(brd_type == BRD_TYPE_WEDGE400){
        if (brd_type_rev >= BOARD_WEDGE400_MP_RESPIN) {
          smb_sensor_threshold[SMB_SENSOR_SW_DIE_TEMP1][UCR_THRESH] = 125;
          smb_sensor_threshold[SMB_SENSOR_SW_DIE_TEMP2][UCR_THRESH] = 125;
        } else {
          smb_sensor_threshold[SMB_SENSOR_SW_DIE_TEMP1][UNR_THRESH] = 110;
          smb_sensor_threshold[SMB_SENSOR_SW_DIE_TEMP1][UCR_THRESH] = 103;
          smb_sensor_threshold[SMB_SENSOR_SW_DIE_TEMP1][UNC_THRESH] = 100;
          smb_sensor_threshold[SMB_SENSOR_SW_DIE_TEMP2][UNR_THRESH] = 110;
          smb_sensor_threshold[SMB_SENSOR_SW_DIE_TEMP2][UCR_THRESH] = 103;
          smb_sensor_threshold[SMB_SENSOR_SW_DIE_TEMP2][UNC_THRESH] = 100;
        }
      }else if(brd_type == BRD_TYPE_WEDGE400C){
        smb_sensor_threshold[SMB_SENSOR_SW_DIE_TEMP1][UCR_THRESH] = 105;
        smb_sensor_threshold[SMB_SENSOR_SW_DIE_TEMP2][UCR_THRESH] = 105;
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
      }
      smb_sensor_threshold[SMB_DOM1_MAX_TEMP][UCR_THRESH] = 65;
      smb_sensor_threshold[SMB_DOM2_MAX_TEMP][UCR_THRESH] = 65;

      /* FCM BOARD */
      smb_sensor_threshold[SMB_SENSOR_FCM_LM75B_U1_TEMP][UCR_THRESH] = 80;
      smb_sensor_threshold[SMB_SENSOR_FCM_LM75B_U2_TEMP][UCR_THRESH] = 80;
      smb_sensor_threshold[SMB_SENSOR_FCM_HSC_VOLT][UCR_THRESH] = 14.13;
      smb_sensor_threshold[SMB_SENSOR_FCM_HSC_VOLT][LCR_THRESH] = 7.5;
      smb_sensor_threshold[SMB_SENSOR_FCM_HSC_CURR][UCR_THRESH] = 40;
    case FRU_PEM1:
    case FRU_PEM2:
      fru_offset = fru - FRU_PEM1;
      pem_sensor_threshold[PEM1_SENSOR_IN_VOLT + (fru_offset * PEM1_SENSOR_CNT)][UCR_THRESH] = 13.75;
      pem_sensor_threshold[PEM1_SENSOR_IN_VOLT + (fru_offset * PEM1_SENSOR_CNT)][LCR_THRESH] = 9;
      pem_sensor_threshold[PEM1_SENSOR_OUT_VOLT + (fru_offset * PEM1_SENSOR_CNT)][UCR_THRESH] = 13.2;
      pem_sensor_threshold[PEM1_SENSOR_OUT_VOLT + (fru_offset * PEM1_SENSOR_CNT)][LCR_THRESH] = 10.8;
      pem_sensor_threshold[PEM1_SENSOR_CURR + (fru_offset * PEM1_SENSOR_CNT)][UCR_THRESH] = 83.2;
      pem_sensor_threshold[PEM1_SENSOR_POWER + (fru_offset * PEM1_SENSOR_CNT)][UCR_THRESH] = 960;
      pem_sensor_threshold[PEM1_SENSOR_FAN1_TACH + (fru_offset * PEM1_SENSOR_CNT)][UCR_THRESH] = 23000;
      pem_sensor_threshold[PEM1_SENSOR_FAN1_TACH + (fru_offset * PEM1_SENSOR_CNT)][LCR_THRESH] = 1000;
      pem_sensor_threshold[PEM1_SENSOR_FAN2_TACH + (fru_offset * PEM1_SENSOR_CNT)][UCR_THRESH] = 23000;
      pem_sensor_threshold[PEM1_SENSOR_FAN2_TACH + (fru_offset * PEM1_SENSOR_CNT)][LCR_THRESH] = 1000;
      pem_sensor_threshold[PEM1_SENSOR_TEMP1 + (fru_offset * PEM1_SENSOR_CNT)][UCR_THRESH] = 95;
      pem_sensor_threshold[PEM1_SENSOR_TEMP1 + (fru_offset * PEM1_SENSOR_CNT)][UNC_THRESH] = 85;
      pem_sensor_threshold[PEM1_SENSOR_TEMP2 + (fru_offset * PEM1_SENSOR_CNT)][UCR_THRESH] = 65;
      pem_sensor_threshold[PEM1_SENSOR_TEMP3 + (fru_offset * PEM1_SENSOR_CNT)][UCR_THRESH] = 65;
      break;
    case FRU_PSU1:
    case FRU_PSU2:
      fru_offset = fru - FRU_PSU1;
      if (is_psu48())
      {
        psu_sensor_threshold[PSU1_SENSOR_IN_VOLT + (fru_offset * PSU1_SENSOR_CNT)][UCR_THRESH] = 59;
        psu_sensor_threshold[PSU1_SENSOR_IN_VOLT + (fru_offset * PSU1_SENSOR_CNT)][LCR_THRESH] = 40;
        psu_sensor_threshold[PSU1_SENSOR_12V_VOLT + (fru_offset * PSU1_SENSOR_CNT)][UCR_THRESH] = 12.6;
        psu_sensor_threshold[PSU1_SENSOR_12V_VOLT + (fru_offset * PSU1_SENSOR_CNT)][LCR_THRESH] = 11.4;
        psu_sensor_threshold[PSU1_SENSOR_STBY_VOLT + (fru_offset * PSU1_SENSOR_CNT)][UCR_THRESH] = 3.45;
        psu_sensor_threshold[PSU1_SENSOR_STBY_VOLT + (fru_offset * PSU1_SENSOR_CNT)][LCR_THRESH] = 3.15;
        psu_sensor_threshold[PSU1_SENSOR_IN_CURR + (fru_offset * PSU1_SENSOR_CNT)][UCR_THRESH] = 40;
        psu_sensor_threshold[PSU1_SENSOR_12V_CURR + (fru_offset * PSU1_SENSOR_CNT)][UCR_THRESH] = 140;
        psu_sensor_threshold[PSU1_SENSOR_STBY_CURR + (fru_offset * PSU1_SENSOR_CNT)][UCR_THRESH] = 3;
        psu_sensor_threshold[PSU1_SENSOR_IN_POWER + (fru_offset * PSU1_SENSOR_CNT)][UCR_THRESH] = 1850;
        psu_sensor_threshold[PSU1_SENSOR_12V_POWER + (fru_offset * PSU1_SENSOR_CNT)][UCR_THRESH] = 1680;
        psu_sensor_threshold[PSU1_SENSOR_STBY_POWER + (fru_offset * PSU1_SENSOR_CNT)][UCR_THRESH] = 9.9;
        psu_sensor_threshold[PSU1_SENSOR_FAN_TACH + (fru_offset * PSU1_SENSOR_CNT)][UCR_THRESH] = 32450;
        psu_sensor_threshold[PSU1_SENSOR_FAN_TACH + (fru_offset * PSU1_SENSOR_CNT)][LCR_THRESH] = 1000;
        psu_sensor_threshold[PSU1_SENSOR_FAN2_TACH + (fru_offset * PSU1_SENSOR_CNT)][UCR_THRESH] = 21950;
        psu_sensor_threshold[PSU1_SENSOR_FAN2_TACH + (fru_offset * PSU1_SENSOR_CNT)][LCR_THRESH] = 1000;
      } else {
        psu_sensor_threshold[PSU1_SENSOR_IN_VOLT + (fru_offset * PSU1_SENSOR_CNT)][UCR_THRESH] = 305;
        psu_sensor_threshold[PSU1_SENSOR_IN_VOLT + (fru_offset * PSU1_SENSOR_CNT)][LCR_THRESH] = 90;
        psu_sensor_threshold[PSU1_SENSOR_12V_VOLT + (fru_offset * PSU1_SENSOR_CNT)][UCR_THRESH] = 14.8;
        psu_sensor_threshold[PSU1_SENSOR_12V_VOLT + (fru_offset * PSU1_SENSOR_CNT)][LCR_THRESH] = 0;
        psu_sensor_threshold[PSU1_SENSOR_STBY_VOLT + (fru_offset * PSU1_SENSOR_CNT)][UCR_THRESH] = 4.2;
        psu_sensor_threshold[PSU1_SENSOR_STBY_VOLT + (fru_offset * PSU1_SENSOR_CNT)][LCR_THRESH] = 0;
        psu_sensor_threshold[PSU1_SENSOR_IN_CURR + (fru_offset * PSU1_SENSOR_CNT)][UCR_THRESH] = 9;
        psu_sensor_threshold[PSU1_SENSOR_12V_CURR + (fru_offset * PSU1_SENSOR_CNT)][UCR_THRESH] = 125;
        psu_sensor_threshold[PSU1_SENSOR_STBY_CURR + (fru_offset * PSU1_SENSOR_CNT)][UCR_THRESH] = 5;
        psu_sensor_threshold[PSU1_SENSOR_IN_POWER + (fru_offset * PSU1_SENSOR_CNT)][UCR_THRESH] = 1500;
        psu_sensor_threshold[PSU1_SENSOR_12V_POWER + (fru_offset * PSU1_SENSOR_CNT)][UCR_THRESH] = 1500;
        psu_sensor_threshold[PSU1_SENSOR_STBY_POWER + (fru_offset * PSU1_SENSOR_CNT)][UCR_THRESH] = 16.5;
        psu_sensor_threshold[PSU1_SENSOR_FAN_TACH + (fru_offset * PSU1_SENSOR_CNT)][UCR_THRESH] = 29500;
        psu_sensor_threshold[PSU1_SENSOR_FAN_TACH + (fru_offset * PSU1_SENSOR_CNT)][LCR_THRESH] = 1000;
        psu_sensor_threshold[PSU1_SENSOR_TEMP1 + (fru_offset * PSU1_SENSOR_CNT)][UCR_THRESH] = 65;
        psu_sensor_threshold[PSU1_SENSOR_TEMP2 + (fru_offset * PSU1_SENSOR_CNT)][UCR_THRESH] = 100;
        psu_sensor_threshold[PSU1_SENSOR_TEMP3 + (fru_offset * PSU1_SENSOR_CNT)][UCR_THRESH] = 125;
      }
      break;
  }
  init_done[fru] = true;
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
  case FRU_FAN1:
  case FRU_FAN2:
  case FRU_FAN3:
  case FRU_FAN4:
    *val = fan_sensor_threshold[sensor_num][thresh];
    break;
  case FRU_SMB:
    *val = smb_sensor_threshold[sensor_num][thresh];
    break;
  case FRU_PEM1:
  case FRU_PEM2:
    *val = pem_sensor_threshold[sensor_num][thresh];
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

int
pal_get_fw_info(uint8_t fru, unsigned char target,
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
      OBMC_WARN("pal_sensor_assert_handle: wrong thresh enum value");
      exit(-1);
  }

  switch(snr_num) {
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
    case SCM_SENSOR_OUTLET_TEMP:
    case SCM_SENSOR_INLET_TEMP:
      *value = 30;
      break;
    case SCM_SENSOR_HSC_VOLT:
    case SCM_SENSOR_HSC_CURR:
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

static void
fan_sensor_poll_interval(uint8_t sensor_num, uint32_t *value) {

  switch(sensor_num) {
    case FAN_SENSOR_FAN1_FRONT_TACH:
    case FAN_SENSOR_FAN1_REAR_TACH:
    case FAN_SENSOR_FAN2_FRONT_TACH:
    case FAN_SENSOR_FAN2_REAR_TACH:
    case FAN_SENSOR_FAN3_FRONT_TACH:
    case FAN_SENSOR_FAN3_REAR_TACH:
    case FAN_SENSOR_FAN4_FRONT_TACH:
    case FAN_SENSOR_FAN4_REAR_TACH:
      *value = 2;
      break;
    default:
      *value = 10;
      break;
  }
}


static void
smb_sensor_poll_interval(uint8_t sensor_num, uint32_t *value) {

  switch(sensor_num) {
    case SMB_SENSOR_SW_CORE_TEMP1:
    case SMB_SENSOR_SW_SERDES_PVDD_TEMP1:
    case SMB_SENSOR_SW_SERDES_TRVDD_TEMP1:
    case SMB_SENSOR_IR3R3V_LEFT_TEMP:
    case SMB_SENSOR_IR3R3V_RIGHT_TEMP:
    case SMB_SENSOR_LM75B_U28_TEMP:
    case SMB_SENSOR_LM75B_U25_TEMP:
    case SMB_SENSOR_LM75B_U56_TEMP:
    case SMB_SENSOR_LM75B_U55_TEMP:
    case SMB_SENSOR_TMP421_U62_TEMP:
    case SMB_SENSOR_TMP421_Q9_TEMP:
    case SMB_SENSOR_TMP421_U63_TEMP:
    case SMB_SENSOR_TMP421_Q10_TEMP:
    case SMB_SENSOR_TMP422_U20_TEMP:
    case SMB_SENSOR_FCM_LM75B_U1_TEMP:
    case SMB_SENSOR_FCM_LM75B_U2_TEMP:
    case SMB_DOM1_MAX_TEMP:
    case SMB_DOM2_MAX_TEMP:
    case SMB_SENSOR_HBM_TEMP:
    case SMB_SENSOR_VDDCK_0_TEMP:
    case SMB_SENSOR_VDDCK_1_TEMP:
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
      *value = 30;
      break;
    case SMB_SENSOR_SW_DIE_TEMP1:
    case SMB_SENSOR_SW_DIE_TEMP2:
      *value = 2;
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
    case SMB_SENSOR_SW_SERDES_PVDD_IN_VOLT:
    case SMB_SENSOR_SW_SERDES_PVDD_IN_CURR:
    case SMB_SENSOR_SW_SERDES_PVDD_OUT_VOLT:
    case SMB_SENSOR_SW_SERDES_PVDD_OUT_CURR:
    case SMB_SENSOR_SW_SERDES_TRVDD_IN_VOLT:
    case SMB_SENSOR_SW_SERDES_TRVDD_IN_CURR:
    case SMB_SENSOR_SW_SERDES_TRVDD_OUT_VOLT:
    case SMB_SENSOR_SW_SERDES_TRVDD_OUT_CURR:
    case SMB_SENSOR_IR3R3V_LEFT_IN_VOLT:
    case SMB_SENSOR_IR3R3V_LEFT_IN_CURR:
    case SMB_SENSOR_IR3R3V_LEFT_OUT_VOLT:
    case SMB_SENSOR_IR3R3V_LEFT_OUT_CURR:
    case SMB_SENSOR_IR3R3V_RIGHT_IN_VOLT:
    case SMB_SENSOR_IR3R3V_RIGHT_IN_CURR:
    case SMB_SENSOR_IR3R3V_RIGHT_OUT_VOLT:
    case SMB_SENSOR_IR3R3V_RIGHT_OUT_CURR:
    case SMB_SENSOR_SW_CORE_VOLT:
    case SMB_SENSOR_FCM_HSC_VOLT:
    case SMB_SENSOR_SW_CORE_CURR:
    case SMB_SENSOR_FCM_HSC_CURR:
    case SMB_BMC_ADC0_VSEN:
    case SMB_BMC_ADC1_VSEN:
    case SMB_BMC_ADC2_VSEN:
    case SMB_BMC_ADC3_VSEN:
    case SMB_BMC_ADC4_VSEN:
    case SMB_BMC_ADC5_VSEN:
    case SMB_BMC_ADC6_VSEN:
    case SMB_BMC_ADC7_VSEN:
    case SMB_BMC_ADC8_VSEN:
    case SMB_SENSOR_HBM_IN_VOLT:
    case SMB_SENSOR_HBM_OUT_VOLT:
    case SMB_SENSOR_HBM_IN_CURR:
    case SMB_SENSOR_HBM_OUT_CURR:
    case SMB_SENSOR_VDDCK_0_IN_VOLT:
    case SMB_SENSOR_VDDCK_0_OUT_VOLT:
    case SMB_SENSOR_VDDCK_0_IN_CURR:
    case SMB_SENSOR_VDDCK_0_OUT_CURR:
    case SMB_SENSOR_VDDCK_1_IN_VOLT:
    case SMB_SENSOR_VDDCK_1_OUT_VOLT:
    case SMB_SENSOR_VDDCK_1_OUT_CURR:
      *value = 30;
      break;
    default:
      *value = 10;
      break;
  }
}

static void
pem_sensor_poll_interval(uint8_t sensor_num, uint32_t *value) {

  switch(sensor_num) {
    case PEM1_SENSOR_IN_VOLT:
    case PEM1_SENSOR_OUT_VOLT:
    case PEM1_SENSOR_FET_BAD:
    case PEM1_SENSOR_FET_SHORT:
    case PEM1_SENSOR_CURR:
    case PEM1_SENSOR_POWER:
    case PEM1_SENSOR_FAN1_TACH:
    case PEM1_SENSOR_FAN2_TACH:
    case PEM1_SENSOR_TEMP1:
    case PEM1_SENSOR_TEMP2:
    case PEM1_SENSOR_TEMP3:

    case PEM2_SENSOR_IN_VOLT:
    case PEM2_SENSOR_OUT_VOLT:
    case PEM2_SENSOR_FET_BAD:
    case PEM2_SENSOR_FET_SHORT:
    case PEM2_SENSOR_CURR:
    case PEM2_SENSOR_POWER:
    case PEM2_SENSOR_FAN1_TACH:
    case PEM2_SENSOR_FAN2_TACH:
    case PEM2_SENSOR_TEMP1:
    case PEM2_SENSOR_TEMP2:
    case PEM2_SENSOR_TEMP3:
      *value = 30;
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
    case PSU1_SENSOR_FAN2_TACH:
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
    case PSU2_SENSOR_FAN2_TACH:
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

int
pal_get_sensor_poll_interval(uint8_t fru, uint8_t sensor_num, uint32_t *value) {

  switch(fru) {
    case FRU_SCM:
      scm_sensor_poll_interval(sensor_num, value);
      break;
    case FRU_FAN1:
    case FRU_FAN2:
    case FRU_FAN3:
    case FRU_FAN4:
      fan_sensor_poll_interval(sensor_num, value);
      break;
    case FRU_SMB:
      smb_sensor_poll_interval(sensor_num, value);
      break;
    case FRU_PEM1:
    case FRU_PEM2:
      pem_sensor_poll_interval(sensor_num, value);
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

void
pal_get_chassis_status(uint8_t slot, uint8_t *req_data, uint8_t *res_data, uint8_t *res_len) {

  char key[MAX_KEY_LEN];
  char buff[MAX_VALUE_LEN];
  int policy = 3;
  uint8_t status, ret;
  unsigned char *data = res_data;

  /* Platform Power Policy */
  sprintf(key, "%s", "server_por_cfg");
  if (pal_get_key_value(key, buff) == 0)
  {
    if (!memcmp(buff, "off", strlen("off")))
      policy = 0;
    else if (!memcmp(buff, "lps", strlen("lps")))
      policy = 1;
    else if (!memcmp(buff, "on", strlen("on")))
      policy = 2;
    else
      policy = 3;
  }

  /* Current Power State */
  ret = pal_get_server_power(FRU_SCM, &status);
  if (ret >= 0) {
    *data++ = status | (policy << 5);
  } else {
    /* load default */
    OBMC_WARN("ipmid: pal_get_server_power failed for server\n");
    *data++ = 0x00 | (policy << 5);
  }
  *data++ = 0x00;   /* Last Power Event */
  *data++ = 0x40;   /* Misc. Chassis Status */
  *data++ = 0x00;   /* Front Panel Button Disable */
  *res_len = data - res_data;
}

uint8_t
pal_set_power_restore_policy(uint8_t slot, uint8_t *pwr_policy, uint8_t *res_data) {

    uint8_t completion_code;
    char key[MAX_KEY_LEN];
    unsigned char policy = *pwr_policy & 0x07;  /* Power restore policy */

  completion_code = CC_SUCCESS;   /* Fill response with default values */
    sprintf(key, "%s", "server_por_cfg");
    switch (policy)
    {
      case 0:
        if (pal_set_key_value(key, "off") != 0)
          completion_code = CC_UNSPECIFIED_ERROR;
        break;
      case 1:
        if (pal_set_key_value(key, "lps") != 0)
          completion_code = CC_UNSPECIFIED_ERROR;
        break;
      case 2:
        if (pal_set_key_value(key, "on") != 0)
          completion_code = CC_UNSPECIFIED_ERROR;
        break;
      case 3:
        /* no change (just get present policy support) */
        break;
      default:
          completion_code = CC_PARAM_OUT_OF_RANGE;
        break;
    }
    return completion_code;
}

void *
generate_dump(void *arg) {

  uint8_t fru = *(uint8_t *) arg;
  char cmd[256];
  char fname[128];
  char fruname[16];

  // Usually the pthread cancel state are enable by default but
  // here we explicitly would like to enable them
  pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
  pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);

  pal_get_fru_name(fru, fruname);//scm

  memset(fname, 0, sizeof(fname));
  snprintf(fname, 128, "/var/run/autodump%d.pid", fru);
  if (access(fname, F_OK) == 0) {
    memset(cmd, 0, sizeof(cmd));
    snprintf(cmd,sizeof(cmd),"rm %s",fname);
    if (system(cmd)) {
      OBMC_CRIT("Removing old crashdump: %s failed!\n", fname);
    }
  }

  // Execute automatic crashdump
  memset(cmd, 0, 128);
  sprintf(cmd, "%s %s", CRASHDUMP_BIN, fruname);
  if (system(cmd)) {
    OBMC_CRIT("Crashdump for FRU: %d failed.", fru);
  } else {
    OBMC_CRIT("Crashdump for FRU: %d is generated.", fru);
  }

  t_dump[fru-1].is_running = 0;
  return 0;
}

static int
pal_store_crashdump(uint8_t fru) {

  int ret;
  char cmd[100];

  // Check if the crashdump script exist
  if (access(CRASHDUMP_BIN, F_OK) == -1) {
    OBMC_CRIT("Crashdump for FRU: %d failed : "
        "auto crashdump binary is not preset", fru);
    return 0;
  }

  // Check if a crashdump for that fru is already running.
  // If yes, kill that thread and start a new one.
  if (t_dump[fru-1].is_running) {
    ret = pthread_cancel(t_dump[fru-1].pt);
    if (ret == ESRCH) {
      OBMC_INFO("pal_store_crashdump: No Crashdump pthread exists");
    } else {
      pthread_join(t_dump[fru-1].pt, NULL);
      sprintf(cmd,
              "ps | grep '{dump.sh}' | grep 'scm' "
              "| awk '{print $1}'| xargs kill");
      if (system(cmd)) {
        OBMC_INFO("Detection of existing crashdump failed!\n");
      }
      sprintf(cmd,
              "ps | grep 'bic-util' | grep 'scm' "
              "| awk '{print $1}'| xargs kill");
      if (system(cmd)) {
        OBMC_INFO("Detection of existing bic-util scm failed!\n");
      }
#ifdef DEBUG
      OBMC_INFO("pal_store_crashdump:"
                       " Previous crashdump thread is cancelled");
#endif
    }
  }

  // Start a thread to generate the crashdump
  t_dump[fru-1].fru = fru;
  if (pthread_create(&(t_dump[fru-1].pt), NULL, generate_dump,
      (void*) &t_dump[fru-1].fru) < 0) {
    OBMC_WARN("pal_store_crashdump: pthread_create for"
        " FRU %d failed\n", fru);
    return -1;
  }

  t_dump[fru-1].is_running = 1;

  OBMC_INFO("Crashdump for FRU: %d is being generated.", fru);

  return 0;
}

int
pal_sel_handler(uint8_t fru, uint8_t snr_num, uint8_t *event_data) {

  char key[MAX_KEY_LEN] = {0};
  char cvalue[MAX_VALUE_LEN] = {0};
  static int assert_cnt[WEDGE400_MAX_NUM_SLOTS] = {0};

  switch(fru) {
    case FRU_SCM:
      switch(snr_num) {
        case CATERR_B:
          pal_store_crashdump(fru);
          break;

        case 0x00:  // don't care sensor number 00h
          return 0;
      }
      sprintf(key, "server_sel_error");

      if ((event_data[2] & 0x80) == 0) {  // 0: Assertion,  1: Deassertion
         assert_cnt[fru-1]++;
      } else {
        if (--assert_cnt[fru-1] < 0)
           assert_cnt[fru-1] = 0;
      }
      sprintf(cvalue, "%s", (assert_cnt[fru-1] > 0) ? "0" : "1");
      break;

    default:
      return -1;
  }

  /* Write the value "0" which means FRU_STATUS_BAD */
  return pal_set_key_value(key, cvalue);

}

void
init_led(void)
{
  int dev, ret;
  dev = open(LED_DEV, O_RDWR);
  if(dev < 0) {
    OBMC_ERROR(-1, "%s: open() failed\n", __func__);
    return;
  }

  ret = ioctl(dev, I2C_SLAVE, I2C_ADDR_SIM_LED);
  if(ret < 0) {
    OBMC_ERROR(-1, "%s: ioctl() assigned i2c addr failed\n", __func__);
    close(dev);
    return;
  }

  i2c_smbus_write_byte_data(dev, 0x06, 0x00);
  i2c_smbus_write_byte_data(dev, 0x07, 0x00);
  close(dev);

  return;
}

int
set_sled(int brd_rev, uint8_t color, int led_name)
{
  int dev, ret;
  uint8_t io0_reg = 0x02, io1_reg = 0x03;
  uint8_t clr_val, val_io0, val_io1;
  int led_id = -1;
  uint8_t brd_type;
  uint8_t brd_type_rev;
  pal_get_board_type(&brd_type);
  pal_get_board_type_rev(&brd_type_rev);

  // in Wedge400 MP Respin LED position are swaped
  if(brd_type == BRD_TYPE_WEDGE400 && brd_type_rev == BOARD_WEDGE400_MP_RESPIN){
    switch (led_name) {
      case SLED_SYS: led_id = SLED_2; break;
      case SLED_FAN: led_id = SLED_4; break;
      case SLED_PSU: led_id = SLED_3; break;
      case SLED_SMB: led_id = SLED_1; break;
    }
  } else {
    switch (led_name) {
      case SLED_SYS: led_id = SLED_1; break;
      case SLED_FAN: led_id = SLED_2; break;
      case SLED_PSU: led_id = SLED_3; break;
      case SLED_SMB: led_id = SLED_4; break;
    }
  }

  dev = open(LED_DEV, O_RDWR);
  if(dev < 0) {
    OBMC_ERROR(-1, "%s: open() failed\n", __func__);
    return -1;
  }

  ret = ioctl(dev, I2C_SLAVE, I2C_ADDR_SIM_LED);
  if(ret < 0) {
    OBMC_ERROR(-1, "%s: ioctl() assigned i2c addr 0x%x failed\n", __func__, I2C_ADDR_SIM_LED);
    close(dev);
    return -1;
  }
  val_io0 = i2c_smbus_read_byte_data(dev, 0x02);
  if(val_io0 < 0) {
    close(dev);
    OBMC_ERROR(-1, "%s: i2c_smbus_read_byte_data failed\n", __func__);
    return -1;
  }

  val_io1 = i2c_smbus_read_byte_data(dev, 0x03);
  if(val_io1 < 0) {
    close(dev);
    OBMC_ERROR(-1, "%s: i2c_smbus_read_byte_data failed\n", __func__);
    return -1;
  }

  clr_val = color;

  if(led_id == SLED_2 || led_id == SLED_4) {
    clr_val = clr_val << 3;
    val_io0 = (val_io0 & 0x7) | clr_val;
    val_io1 = (val_io1 & 0x7) | clr_val;
  }
  else if(led_id == SLED_1 || led_id == SLED_3) {
    val_io0 = (val_io0 & 0x38) | clr_val;
    val_io1 = (val_io1 & 0x38) | clr_val;
  }
  else {
    OBMC_WARN("%s: unknown led name\n", __func__);
  }

  if(led_id == SLED_1 || led_id == SLED_2) {
    i2c_smbus_write_byte_data(dev, io0_reg, val_io0);
  } else {
    i2c_smbus_write_byte_data(dev, io1_reg, val_io1);
  }

  close(dev);
  return 0;
}

#define MATCH_UPGRADE_CMD(_cmd_line, _patterns, _flag)   \
  if ((_flag) == 0) {                                    \
    int __i;                                             \
    for (__i = 0; (_patterns)[__i] != NULL; __i++) {     \
      if (strstr(_cmd_line, (_patterns)[__i]) != NULL) { \
        (_flag) = 1;                                     \
        goto next_cmd;                                   \
      }                                                  \
    }                                                    \
  }

/*
 * XXX listing running processes and looking for a certain pattern is
 * not an effective way to determine if scm/fan/psu/smb upgrade is in
 * progress, and it's not reliable, either (for example, when extra
 * spaces are inserted between commands and arguments).
 * TODO: we could create a kv pair in scm/fan/psu/smb upgrade commands,
 * and then all we need is checking if the specific kv pair exists.
 */
static int
pal_mon_fw_upgrade(int brd_rev, uint8_t *sys_ug, uint8_t *fan_ug,
                   uint8_t *psu_ug, uint8_t *smb_ug)
{
  FILE *fp;
  char cmd[32];
  char line[256];
  const char *sys_ug_cmds[] = {
    "write spi2",
    "write spi1 BACKUP_BIOS",
    "scmcpld_update",
    "fw-util scm --update",
    NULL,
  };
  const char* fan_ug_cmds[] = {
    "fcmcpld_update",
    NULL,
  };
  const char* psu_ug_cmds[] = {
    "psu-util psu1 --update",
    "psu-util psu2 --update",
    NULL,
  };
  const char* smb_ug_cmds[] = {
    "smbcpld_update",
    "pwrcpld_update",
    "flashcp",
    "write spi1 DOM_FPGA_FLASH",
    "write spi1 TH3_PCIE_FLASH",
    "write spi1 GB_PCIE_FLASH",
    "write spi1 BCM5389_EE",
    NULL,
  };

  snprintf(cmd, sizeof(cmd), "ps w");
  fp = popen(cmd, "r");
  if(NULL == fp)
     return -1;

  while (fgets(line, sizeof(line), fp) != NULL) {
    MATCH_UPGRADE_CMD(line, sys_ug_cmds, *sys_ug);
    MATCH_UPGRADE_CMD(line, fan_ug_cmds, *fan_ug);
    MATCH_UPGRADE_CMD(line, psu_ug_cmds, *psu_ug);
    MATCH_UPGRADE_CMD(line, smb_ug_cmds, *smb_ug);

next_cmd:
    if ((*sys_ug + *fan_ug + *psu_ug + *smb_ug) >= 4) {
      break;
    }
  } /* while (fgets...) */

  if (pclose(fp) != 0) {
     OBMC_ERROR(errno, "%s pclose() fail", __func__);
     return -1;
  }

  return 0;
}

void set_sys_led(int brd_rev)
{
  uint8_t ret = 0;
  uint8_t prsnt = 0;
  uint8_t sys_ug = 0, fan_ug = 0, psu_ug = 0, smb_ug = 0;
  static uint8_t alter_sys = 0;
  ret = pal_is_fru_prsnt(FRU_SCM, &prsnt);
  if (ret) {
    set_sled(brd_rev, SLED_CLR_YELLOW, SLED_SYS);
    OBMC_WARN("%s: can't get SCM status\n",__func__);
    return;
  }
  if (!prsnt) {
    set_sled(brd_rev, SLED_CLR_YELLOW, SLED_SYS);
    OBMC_WARN("%s: SCM isn't presence\n",__func__);
    return;
  }

  pal_mon_fw_upgrade(brd_rev, &sys_ug, &fan_ug, &psu_ug, &smb_ug);
  if( sys_ug || fan_ug || psu_ug || smb_ug ){
    if(alter_sys==0){
      alter_sys = 1;
      set_sled(brd_rev, SLED_CLR_YELLOW, SLED_SYS);
    }else{
      alter_sys = 0;
      set_sled(brd_rev, SLED_CLR_BLUE, SLED_SYS);
    }
  } else {
    set_sled(brd_rev, SLED_CLR_BLUE, SLED_SYS);
  }
}

void set_fan_led(int brd_rev)
{
  int i;
  float value, unc, lcr;
  uint8_t prsnt;
  uint8_t fan_num = MAX_NUM_FAN * 2;//rear & front: MAX_NUM_FAN * 2
  char path[LARGEST_DEVICE_NAME + 1];
  char sensor_name[LARGEST_DEVICE_NAME];
  int sensor_num[] = { FAN_SENSOR_FAN1_FRONT_TACH,
                       FAN_SENSOR_FAN1_REAR_TACH ,
                       FAN_SENSOR_FAN2_FRONT_TACH,
                       FAN_SENSOR_FAN2_REAR_TACH ,
                       FAN_SENSOR_FAN3_FRONT_TACH,
                       FAN_SENSOR_FAN3_REAR_TACH ,
                       FAN_SENSOR_FAN4_FRONT_TACH,
                       FAN_SENSOR_FAN4_REAR_TACH };

  for(i = FRU_FAN1; i <= FRU_FAN4; i++) {
    pal_is_fru_prsnt(i, &prsnt);
#ifdef DEBUG
    OBMC_INFO("fan %d : %d\n",i,prsnt);
#endif
    if(!prsnt) {
      OBMC_WARN("%s: fan %d isn't presence\n",__func__,i-FRU_FAN1+1);
      goto cleanup;
    }
  }
  for(i = 0; i < fan_num; i++) {
    if(fan_sensor_read(sensor_num[i],&value)) {
      OBMC_WARN("%s: can't access %s\n",__func__,path);
      goto cleanup;
    }

    pal_get_sensor_threshold(FRU_FAN1, sensor_num[i], UCR_THRESH, &unc);
    pal_get_sensor_threshold(FRU_FAN1, sensor_num[i], LCR_THRESH, &lcr);
#ifdef DEBUG
    OBMC_INFO("fan %d\n",i);
    OBMC_INFO(" val %d\n",value);
    OBMC_INFO(" ucr %f\n",smb_sensor_threshold[sensor_num[i]][UCR_THRESH]);
    OBMC_INFO(" lcr %f\n",smb_sensor_threshold[sensor_num[i]][LCR_THRESH]);
#endif
    if(value == 0) {
      OBMC_WARN("%s: can't access %s\n",__func__,path);
      goto cleanup;
    }
    else if(value > unc){
      pal_get_sensor_name(FRU_FAN1,sensor_num[i],sensor_name);
      OBMC_WARN("%s: %s value is over than UNC ( %.2f > %.2f )\n",
      __func__,sensor_name,value,unc);
      goto cleanup;
    }
    else if(value < lcr){
      pal_get_sensor_name(FRU_FAN1,sensor_num[i],sensor_name);
      OBMC_WARN("%s: %s value is under than LCR ( %.2f < %.2f )\n",
      __func__,sensor_name,value,lcr);
      goto cleanup;
    }
  }
  set_sled(brd_rev, SLED_CLR_BLUE, SLED_FAN);
  sleep(LED_INTERVAL);
  return;
cleanup:
  set_sled(brd_rev, SLED_CLR_YELLOW, SLED_FAN);
  sleep(LED_INTERVAL);
  return;
}

void set_psu_led(int brd_rev)
{
  int i;
  float value,ucr,lcr;
  uint8_t prsnt,pem_prsnt,fru,ready[4] = { 0 };
  char path[LARGEST_DEVICE_NAME + 1];
  int psu1_sensor_num[] = { PSU1_SENSOR_IN_VOLT,
                            PSU1_SENSOR_12V_VOLT,
                            PSU1_SENSOR_STBY_VOLT};
  int psu2_sensor_num[] = { PSU2_SENSOR_IN_VOLT,
                            PSU2_SENSOR_12V_VOLT,
                            PSU2_SENSOR_STBY_VOLT};
  int pem1_sensor_num[] = { PEM1_SENSOR_IN_VOLT,
                            PEM1_SENSOR_OUT_VOLT};
  int pem2_sensor_num[] = { PEM2_SENSOR_IN_VOLT,
                            PEM2_SENSOR_OUT_VOLT};
  char sensor_name[LARGEST_DEVICE_NAME];
  int *sensor_num,sensor_cnt;
  char pem1_sensor_cnt,pem2_sensor_cnt;
  char psu1_sensor_cnt,psu2_sensor_cnt;
  pem1_sensor_cnt = sizeof(pem1_sensor_num)/sizeof(pem1_sensor_num[0]);
  pem2_sensor_cnt = sizeof(pem2_sensor_num)/sizeof(pem2_sensor_num[0]);
  psu1_sensor_cnt = sizeof(psu1_sensor_num)/sizeof(psu1_sensor_num[0]);
  psu2_sensor_cnt = sizeof(psu2_sensor_num)/sizeof(psu2_sensor_num[0]);

  pem_prsnt = 0;
  for( fru = FRU_PEM1 ; fru <= FRU_PSU2 ; fru++){
    switch(fru){
      case FRU_PEM1:
        sensor_cnt = pem1_sensor_cnt;
        sensor_num = pem1_sensor_num;  break;
      case FRU_PEM2:
        sensor_cnt = pem2_sensor_cnt;
        sensor_num = pem2_sensor_num;  break;
      case FRU_PSU1:
        sensor_cnt = psu1_sensor_cnt;
        sensor_num = psu1_sensor_num;  break;
      case FRU_PSU2:
        sensor_cnt = psu2_sensor_cnt;
        sensor_num = psu2_sensor_num;  break;
      default :
        OBMC_WARN("%s: skip with invalid fru %d\n",__func__,fru);
        continue;
    }

    pal_is_fru_prsnt(fru,&prsnt);
    if(fru == FRU_PEM1 || fru == FRU_PEM2){    // check PEM present.
      if(prsnt){
        pem_prsnt = prsnt;                     // if PEM present,no need to check PSU.
      }else{
        pem_prsnt = 0;
        continue;                              // if PEM not present, continue to check
      }
    }else if(fru == FRU_PSU1 || fru == FRU_PSU2){
      if(!prsnt){            // PSU not present then check PEM present.
        if(!pem_prsnt)       // PEM not present.
        {
          OBMC_WARN("%s: PSU%d isn't presence\n",__func__,fru-FRU_PSU1+1);
          goto cleanup;
        }
        else{               // PEM present check another PSU or PEM.
          continue;
        }
      }
    }
    pal_is_fru_ready(fru,&ready[fru-FRU_PEM1]);
    if(!ready[fru-FRU_PEM1]){
      if(fru >= FRU_PSU1 && !ready[fru-2]){     // if PSU and PEM aren't ready both
        OBMC_WARN("%s: PSU%d and PEM%d can't access\n",
        __func__,fru-FRU_PSU1+1,fru-FRU_PSU1+1);
        goto cleanup;
      }
      continue;
    }

    for(i = 0; i < sensor_cnt ; i++) {
      pal_get_sensor_threshold(fru, sensor_num[i], UCR_THRESH, &ucr);
      pal_get_sensor_threshold(fru, sensor_num[i], LCR_THRESH, &lcr);
      pal_get_sensor_name(fru, sensor_num[i],sensor_name);

      if(fru >= FRU_PEM1 && fru <= FRU_PEM2){
        if(pem_sensor_read(sensor_num[i], &value)) {
          OBMC_WARN("%s: fru %d sensor id %d (%s) can't access %s\n",
          __func__,fru,sensor_num[i],sensor_name,path);
          goto cleanup;
        }
      }else if(fru >= FRU_PSU1 && fru <= FRU_PSU2){
        if(psu_sensor_read(sensor_num[i], &value)) {
          OBMC_WARN("%s: fru %d sensor id %d (%s) can't access %s\n",
          __func__,fru,sensor_num[i],sensor_name,path);
          goto cleanup;
        }
      }

      if(value > ucr) {
        OBMC_WARN("%s: %s value is over than UCR ( %.2f > %.2f )\n",
        __func__,sensor_name,value,ucr);
        goto cleanup;
      }else if(value < lcr){
        OBMC_WARN("%s: %s value is under than LCR ( %.2f < %.2f )\n",
        __func__,sensor_name,value,lcr);
        goto cleanup;
      }
    }
  }
  set_sled(brd_rev, SLED_CLR_BLUE, SLED_PSU);
  sleep(LED_INTERVAL);
  return;

cleanup:
  set_sled(brd_rev, SLED_CLR_YELLOW, SLED_PSU);
  sleep(LED_INTERVAL);
  return;
}

void set_scm_led(int brd_rev)
{
  static int prev_userver_state = USERVER_STATE_NONE;
  char path[LARGEST_DEVICE_NAME];
  char cmd[64];
  char buffer[256];
  FILE *fp;
  int power;
  int ret;
  const char *scm_ip_usb = "fe80::ff:fe00:2%usb0";

  snprintf(path, sizeof(path), GPIO_COME_PWRGD, "value");
  if (device_read(path, &power)) {
    OBMC_WARN("%s: can't get GPIO value '%s'",__func__,path);
    return;
  }

  if (!power) {
    if (prev_userver_state != USERVER_STATE_POWER_OFF) {
      OBMC_WARN("%s: micro server is power off\n",__func__);
      prev_userver_state = USERVER_STATE_POWER_OFF;
    }
    set_sled(brd_rev, SLED_CLR_RED, SLED_SMB);
    return;
  }

  // -c count = 1 times
  // -W timeout = 1 secound
  snprintf(cmd, sizeof(cmd), "ping -c 1 -W 1 %s", scm_ip_usb);
  fp = popen(cmd,"r");
  if (!fp) {
    goto error;
  }

  while (fgets(buffer, sizeof(buffer), fp) != NULL) {}

  ret = pclose(fp);
  if(ret == 0) { // PING OK
    if (prev_userver_state != USERVER_STATE_NORMAL) {
      OBMC_WARN("%s: micro server in normal state",__func__);
      prev_userver_state = USERVER_STATE_NORMAL;
    }
    set_sled(brd_rev,SLED_CLR_GREEN,SLED_SMB);
    return;
  }

error:
  if (prev_userver_state != USERVER_STATE_PING_DOWN) {
    OBMC_WARN("%s: can't ping to %s",__func__,scm_ip_usb);
    prev_userver_state = USERVER_STATE_PING_DOWN;
  }
  set_sled(brd_rev,SLED_CLR_YELLOW,SLED_SMB);
  sleep(LED_INTERVAL);
}

int
pal_light_scm_led(uint8_t led_color)
{
  char path[64];
  int ret;
  char *val;
  sprintf(path, SCM_SYSFS, SYS_LED_COLOR);
  if(led_color == SCM_LED_BLUE)
    val = "0";
  else
    val = "1";
  ret = device_write_buff(path, val);
  if (ret) {
#ifdef DEBUG
  OBMC_WARN("device_write_buff failed for %s\n", path);
#endif
    return -1;
  }

  return 0;
}

int
pal_set_def_key_value(void) {

  int i, ret;
  char path[LARGEST_DEVICE_NAME + 1];

  for (i = 0; strcmp(key_list[i], LAST_KEY) != 0; i++) {
    snprintf(path, LARGEST_DEVICE_NAME, KV_PATH, key_list[i]);
    if ((ret = kv_set(key_list[i], def_val_list[i],
                    0, KV_FPERSIST | KV_FCREATE)) < 0) {
#ifdef DEBUG
      OBMC_WARN("pal_set_def_key_value: kv_set failed. %d", ret);
#endif
    }
  }
  return 0;
 }

int
pal_init_sensor_check(uint8_t fru, uint8_t snr_num, void *snr) {
  pal_set_def_key_value();
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

    case FRU_PEM1:
        sprintf(key, "pem1_sensor_health");
        break;
    case FRU_PEM2:
        sprintf(key, "pem2_sensor_health");
        break;

    case FRU_PSU1:
        sprintf(key, "psu1_sensor_health");
        break;
    case FRU_PSU2:
        sprintf(key, "psu2_sensor_health");
        break;

    case FRU_FAN1:
        sprintf(key, "fan1_sensor_health");
        break;
    case FRU_FAN2:
        sprintf(key, "fan2_sensor_health");
        break;
    case FRU_FAN3:
        sprintf(key, "fan3_sensor_health");
        break;
    case FRU_FAN4:
        sprintf(key, "fan4_sensor_health");
        break;

    default:
      return ERR_SENSOR_NA;
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
    case FRU_PEM1:
      sprintf(key, "pem1_sensor_health");
      break;
    case FRU_PEM2:
      sprintf(key, "pem2_sensor_health");
      break;
    case FRU_PSU1:
      sprintf(key, "psu1_sensor_health");
      break;
    case FRU_PSU2:
      sprintf(key, "psu2_sensor_health");
      break;
    case FRU_FAN1:
      sprintf(key, "fan1_sensor_health");
      break;
    case FRU_FAN2:
      sprintf(key, "fan2_sensor_health");
      break;
    case FRU_FAN3:
      sprintf(key, "fan3_sensor_health");
      break;
    case FRU_FAN4:
      sprintf(key, "fan4_sensor_health");
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
wedge400_sensor_name(uint8_t fru, uint8_t sensor_num, char *name) {

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
      if (wedge400_sensor_name(fru, snr_num, name) != 0) {
        break;
      }
      return 0;
  }
  // Otherwise, translate it based on snr_num
  return pal_get_x86_event_sensor_name(fru, snr_num, name);
}

// Write GUID into EEPROM
static int
pal_set_guid(uint16_t offset, char *guid) {
  int fd = 0;
  ssize_t bytes_wr;
  char eeprom_path[FBW_EEPROM_PATH_SIZE];
  errno = 0;

  wedge_eeprom_path(eeprom_path);

  // Check for file presence
  if (access(eeprom_path, F_OK) == -1) {
      OBMC_ERROR(-1, "pal_set_guid: unable to access the %s file: %s",
          eeprom_path, strerror(errno));
      return errno;
  }

  // Open file
  fd = open(eeprom_path, O_WRONLY);
  if (fd == -1) {
    OBMC_ERROR(-1, "pal_set_guid: unable to open the %s file: %s",
        eeprom_path, strerror(errno));
    return errno;
  }

  // Seek the offset
  lseek(fd, offset, SEEK_SET);

  // Write GUID data
  bytes_wr = write(fd, guid, GUID_SIZE);
  if (bytes_wr != GUID_SIZE) {
    OBMC_ERROR(-1, "pal_set_guid: write to %s file failed: %s",
        eeprom_path, strerror(errno));
    goto err_exit;
  }

err_exit:
  close(fd);
  return errno;
}

// Read GUID from EEPROM
static int
pal_get_guid(uint16_t offset, char *guid) {
  int fd = 0;
  ssize_t bytes_rd;
  char eeprom_path[FBW_EEPROM_PATH_SIZE];
  errno = 0;

  wedge_eeprom_path(eeprom_path);

  // Check if file is present
  if (access(eeprom_path, F_OK) == -1) {
      OBMC_ERROR(-1, "pal_get_guid: unable to access the %s file: %s",
          eeprom_path, strerror(errno));
      return errno;
  }

  // Open the file
  fd = open(eeprom_path, O_RDONLY);
  if (fd == -1) {
    OBMC_ERROR(-1, "pal_get_guid: unable to open the %s file: %s",
        eeprom_path, strerror(errno));
    return errno;
  }

  // seek to the offset
  lseek(fd, offset, SEEK_SET);

  // Read bytes from location
  bytes_rd = read(fd, guid, GUID_SIZE);
  if (bytes_rd != GUID_SIZE) {
    OBMC_ERROR(-1, "pal_get_guid: read to %s file failed: %s",
        eeprom_path, strerror(errno));
    goto err_exit;
  }

err_exit:
  close(fd);
  return errno;
}

int
pal_get_dev_guid(uint8_t fru, char *guid) {

  pal_get_guid(OFFSET_DEV_GUID, (char *)g_dev_guid);
  memcpy(guid, g_dev_guid, GUID_SIZE);

  return 0;
}

int
pal_set_dev_guid(uint8_t slot, char *guid) {

  pal_populate_guid(g_dev_guid, guid);

  return pal_set_guid(OFFSET_DEV_GUID, (char *)g_dev_guid);
}

int
pal_get_boot_order(uint8_t slot, uint8_t *req_data,
                   uint8_t *boot, uint8_t *res_len) {
  int ret, msb, lsb, i, j = 0;
  char str[MAX_VALUE_LEN] = {0};
  char tstr[4] = {0};

  ret = pal_get_key_value("server_boot_order", str);
  if (ret) {
    *res_len = 0;
    return ret;
  }

  for (i = 0; i < 2*SIZE_BOOT_ORDER; i += 2) {
    snprintf(tstr, sizeof(tstr), "%c\n", str[i]);
    msb = strtol(tstr, NULL, 16);

    snprintf(tstr, sizeof(tstr), "%c\n", str[i+1]);
    lsb = strtol(tstr, NULL, 16);
    boot[j++] = (msb << 4) | lsb;
  }
  *res_len = SIZE_BOOT_ORDER;

  return 0;
}

int
pal_set_boot_order(uint8_t slot, uint8_t *boot,
                   uint8_t *res_data, uint8_t *res_len) {
  int i, j, offset, network_dev = 0;
  char str[MAX_VALUE_LEN] = {0};
  enum {
    BOOT_DEVICE_IPV4 = 0x1,
    BOOT_DEVICE_IPV6 = 0x9,
  };

  *res_len = 0;

  for (i = offset = 0; i < SIZE_BOOT_ORDER && offset < sizeof(str); i++) {
    if (i > 0) {  // byte[0] is boot mode, byte[1:5] are boot order
      for (j = i+1; j < SIZE_BOOT_ORDER; j++) {
        if (boot[i] == boot[j])
          return CC_INVALID_PARAM;
      }

      // If bit[2:0] is 001b (Network), bit[3] is IPv4/IPv6 order
      // bit[3]=0b: IPv4 first
      // bit[3]=1b: IPv6 first
      if ((boot[i] == BOOT_DEVICE_IPV4) || (boot[i] == BOOT_DEVICE_IPV6))
        network_dev++;
    }

    offset += snprintf(str + offset, sizeof(str) - offset, "%02x", boot[i]);
  }

  // not allow having more than 1 network boot device in the boot order
  if (network_dev > 1)
    return CC_INVALID_PARAM;

  return pal_set_key_value("server_boot_order", str);
}

int pal_get_bmc_ipmb_slave_addr(uint16_t *slave_addr, uint8_t bus_id)
{
  *slave_addr = 0x10;
  return 0;
}

int
pal_ipmb_processing(int bus, void *buf, uint16_t size) {
  char key[MAX_KEY_LEN];
  char value[MAX_VALUE_LEN];
  struct timespec ts;
  static time_t last_time = 0;
  if ((bus == 4) && (((uint8_t *)buf)[0] == 0x20)) {  // OCP LCD debug card
    clock_gettime(CLOCK_MONOTONIC, &ts);
    if (ts.tv_sec >= (last_time + 5)) {
      last_time = ts.tv_sec;
      ts.tv_sec += 30;

      snprintf(key, sizeof(key), "ocpdbg_lcd");
      snprintf(value, sizeof(value), "%ld", ts.tv_sec);
      if (kv_set(key, value, 0, 0) < 0) {
        return -1;
      }
    }
  }
  return 0;
}

bool
pal_is_mcu_working(void) {
  char key[MAX_KEY_LEN] = {0};
  char value[MAX_VALUE_LEN] = {0};
  struct timespec ts;

  snprintf(key, sizeof(key), "ocpdbg_lcd");
  if (kv_get(key, value, NULL, 0)) {
     return false;
  }

  clock_gettime(CLOCK_MONOTONIC, &ts);
  if (strtoul(value, NULL, 10) > ts.tv_sec) {
     return true;
  }

  return false;
}

/* Add button function for Debug Card */
/* Read the Front Panel Hand Switch and return the position */
int
pal_get_hand_sw_physically(uint8_t *pos) {
  char path[LARGEST_DEVICE_NAME + 1];
  int loc;

  snprintf(path, LARGEST_DEVICE_NAME, BMC_UART_SEL);
  if (device_read(path, &loc)) {
    return -1;
  }
  *pos = loc;

  return 0;
}

int
pal_get_hand_sw(uint8_t *pos) {
  char value[MAX_VALUE_LEN];
  uint8_t loc;
  int ret;

  ret = kv_get("scm_hand_sw", value, NULL, 0);
  if (!ret) {
    loc = atoi(value);
    if (loc > HAND_SW_BMC) {
      return pal_get_hand_sw_physically(pos);
    }
    *pos = loc;

    return 0;
  }

  return pal_get_hand_sw_physically(pos);
}

/* Return the Front Panel Power Button */
int
pal_get_dbg_pwr_btn(uint8_t *status) {
   char cmd[MAX_KEY_LEN + 32] = {0};
  char value[MAX_VALUE_LEN];
  char *p;
  FILE *fp;
  int val = 0;

  sprintf(cmd, "/usr/sbin/i2cget -f -y 4 0x27 1");
  fp = popen(cmd, "r");
  if (!fp) {
    return -1;
  }

  if (fgets(value, MAX_VALUE_LEN, fp) == NULL) {
    pclose(fp);
    return -1;
  }

  for (p = value; *p != '\0'; p++) {
    if (*p == '\n' || *p == '\r') {
      *p = '\0';
      break;
    }
  }

  sscanf(value, "%x", &val);
  if ((!(val & 0x2)) && (val & 0x1)) {
    *status = 1;      /* PWR BTN status pressed */
    syslog(LOG_WARNING, "%s PWR pressed  0x%x\n", __FUNCTION__, val);
  } else {
    *status = 0;      /* PWR BTN status cleared */
  }
  pclose(fp);
  return 0;
}

/* Return the Debug Card Reset Button status */
int
pal_get_dbg_rst_btn(uint8_t *status) {
  char cmd[MAX_KEY_LEN + 32] = {0};
  char value[MAX_VALUE_LEN];
  char *p;
  FILE *fp;
  int val = 0;

  sprintf(cmd, "/usr/sbin/i2cget -f -y 4 0x27 1");
  fp = popen(cmd, "r");
  if (!fp) {
    return -1;
  }

  if (fgets(value, MAX_VALUE_LEN, fp) == NULL) {
    pclose(fp);
    return -1;
  }

  for (p = value; *p != '\0'; p++) {
    if (*p == '\n' || *p == '\r') {
      *p = '\0';
      break;
    }
  }

  sscanf(value, "%x", &val);
  if ((!(val & 0x1)) && (val & 0x2)) {
    *status = 1;      /* RST BTN status pressed */
  } else {
    *status = 0;      /* RST BTN status cleared */
  }
  pclose(fp);
  return 0;
}

/* Update the Reset button input to the server at given slot */
int
pal_set_rst_btn(uint8_t slot, uint8_t status) {
  char *val;
  char path[64];
  int ret;
  if (slot != FRU_SCM) {
    return -1;
  }

  if (status) {
    val = "1";
  } else {
    val = "0";
  }

  sprintf(path, SCM_SYSFS, SCM_COM_RST_BTN);

  ret = device_write_buff(path, val);
  if (ret) {
    return -1;
  }
  return 0;
}

/* Return the Debug Card UART Sel Button status */
int
pal_get_dbg_uart_btn(uint8_t *status) {
  char cmd[MAX_KEY_LEN + 32] = {0};
  char value[MAX_VALUE_LEN];
  char *p;
  FILE *fp;
  int val = 0;

  sprintf(cmd, "/usr/sbin/i2cget -f -y 4 0x27 3");
  fp = popen(cmd, "r");
  if (!fp) {
    return -1;
  }

  if (fgets(value, MAX_VALUE_LEN, fp) == NULL) {
    pclose(fp);
    return -1;
  }
  pclose(fp);
  for (p = value; *p != '\0'; p++) {
    if (*p == '\n' || *p == '\r') {
      *p = '\0';
      break;
    }
  }
  sscanf(value, "%x", &val);
  if (!(val & 0x80)) {
    *status = 1;      /* UART BTN status pressed */
  } else {
    *status = 0;      /* UART BTN status cleared */
  }
  return 0;
}

/* Clear Debug Card UART Sel Button status */
int
pal_clr_dbg_uart_btn() {
  int ret;
  ret = run_command("/usr/sbin/i2cset -f -y 4 0x27 3 0xff");
  if (ret)
    return -1;
  return 0;
}

/* Switch the UART mux to userver or BMC */
int
pal_switch_uart_mux() {
  char path[LARGEST_DEVICE_NAME + 1];
  char *val;
  int loc;
  /* Refer the UART select status */
  snprintf(path, LARGEST_DEVICE_NAME, BMC_UART_SEL);
  if (device_read(path, &loc)) {
    return -1;
  }

  if (loc == 3) {
    val = "2";
  } else {
    val = "3";
  }

  if (device_write_buff(path, val)) {
    return -1;
  }
  return 0;
}

bool is_psu48(void)
{
  char buffer[6];
  FILE *fp;
  fp = popen("source /usr/local/bin/openbmc-utils.sh;wedge_power_supply_type", "r");

  if(NULL == fp)
     return false;

  if (fgets(buffer, sizeof(buffer), fp) == NULL) {
    pclose(fp);
    return false;
  }

  pclose(fp);
  if (!strcmp(buffer, "PSU48"))
  {
      return true;
  }
  else
  {
    return false;
  }
}
