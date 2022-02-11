/*
 *
 * Copyright 2020-present Facebook. All Rights Reserved.
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

#include <openbmc/misc-utils.h>
#include "pal-led.h"
#include "pal.h"

int init_led() {
  // TODO
  return 0;
}

int pal_light_scm_led(int color) {
  set_sled(LED_SCM, color);
  return 0;
}

int set_sled(int led, int color) {
  char led_path[LARGEST_DEVICE_NAME];
  char led_str_name[16];
  char color_value[16];
  char brightness_value[16];

  // the color_value ordering by RED BLUE GREEN
  switch(color) {
    case LED_COLOR_OFF:
      strcpy(color_value, "0 0 0");
      strcpy(brightness_value, "0");
      break;
    case LED_COLOR_BLUE:
      strcpy(color_value, "0 255 0");
      strcpy(brightness_value, "255");
      break;
    case LED_COLOR_GREEN:
      strcpy(color_value, "0 0 255");
      strcpy(brightness_value, "255");
      break;
    case LED_COLOR_AMBER:
      strcpy(color_value, "255 0 45");
      strcpy(brightness_value, "255");
      break;
    case LED_COLOR_RED:
      strcpy(color_value, "255 0 0");
      strcpy(brightness_value, "255");
      break;
    default:
      return -1;
  }

  switch(led) {
    case LED_SYS:
      strcpy(led_str_name, "sys");
      break;
    case LED_FAN:
      strcpy(led_str_name, "fan");
      break;
    case LED_PSU:
      strcpy(led_str_name, "psu");
      break;
    case LED_SCM:
      strcpy(led_str_name, "scm");
      break;
    default:
      return -1;
  }

  sprintf(led_path, "/sys/class/leds/%s/multi_intensity", led_str_name);
  device_write_buff(led_path, color_value);
  sprintf(led_path, "/sys/class/leds/%s/brightness", led_str_name);
  device_write_buff(led_path, brightness_value);

  return 0;
}

/*
 * @brief  checking PSU_AC_OK,PSU_DC_OK and PSU presence
 * @note   will be use for psu_led controling
 * @param  none:
 * @retval 0 is normal , 1 is fail
 */
int psu_check() {
  uint8_t status=0;
  int ret=0;
  char path[LARGEST_DEVICE_NAME + 1];
  char *sysfs[] = {"psu1_dc_power_ok", "psu1_ac_input_ok",
                   "psu2_dc_power_ok", "psu2_ac_input_ok"};

  // check psu presence
  for (int psu=1; psu<=2; psu++) {
    if(pal_is_fru_prsnt(FRU_PSU1+psu-1, &status)) {
      OBMC_CRIT("cannot get FRU_PSU%d presence status ", psu);
      ret=1;
      continue;
    }
    if (status == 0) { // status 0 is absent
      OBMC_WARN("FRU_PSU%d is not presence", psu);
      ret=1;
    }
  }

  // check PSU_AC_OK,PSU_DC_OK
  for (int i=0; i<sizeof(sysfs)/sizeof(sysfs[0]) ; i++) {
    snprintf(path, LARGEST_DEVICE_NAME, SMB_SYSFS, sysfs[i]);
    if (device_read(path, (int*)&status)) {
      OBMC_CRIT("cannot access %s", sysfs[i]);
      ret=1;
      continue;
    }
    if (status == 0) { // status 0 is fail
      OBMC_WARN("%s is not ok", sysfs[i]);
      ret=1;
    }
  }

  return ret;
}

/*
 * @brief  checking fan presence and fan speed
 * @note   will be use for fan_led controling
 * @param  none:
 * @retval 0 is normal , 1 is fail
 */
int fan_check() {
  uint8_t status=0;
  int ret=0;
  float value=0, ucr=0, lcr=0;
  char sensor_name[32];
  int sensor_num[] = {
      SMB_SENSOR_FAN1_FRONT_TACH,       SMB_SENSOR_FAN1_REAR_TACH,
      SMB_SENSOR_FAN2_FRONT_TACH,       SMB_SENSOR_FAN2_REAR_TACH,
      SMB_SENSOR_FAN3_FRONT_TACH,       SMB_SENSOR_FAN3_REAR_TACH,
      SMB_SENSOR_FAN4_FRONT_TACH,       SMB_SENSOR_FAN4_REAR_TACH
  };

  // check fan presence
  for (int fan=FRU_FAN1; fan<=FRU_FAN4; fan++) {
    if (pal_is_fru_prsnt(fan, &status)) {
      OBMC_CRIT("cannot get FRU presence status FRU_FAN%d", (fan-FRU_FAN1+1));
      ret=1;
      continue;
    }
    if(status == 0) { // status 0 is absent
      OBMC_WARN("FRU_FAN%d is not presence", (fan-FRU_FAN1+1));
      ret=1;
    }
  }

  // check fan speed
  for (int i=0; i<sizeof(sensor_num)/sizeof(sensor_num[0]); i++) {
    pal_get_sensor_name(FRU_SMB, sensor_num[i], sensor_name);
    if (sensor_cache_read(FRU_SMB,sensor_num[i], &value)) {
      OBMC_INFO("can't read %s from cache read raw instead", sensor_name);
      if (pal_sensor_read_raw(FRU_SMB,sensor_num[i], &value)) {
        OBMC_CRIT("can't read sensor %s", sensor_name);
        ret=1;
        continue;
      } else {
        // write cahed after read success
        if (value == 0) {
          sensor_cache_write(FRU_SMB, sensor_num[i], false, 0.0);
        } else {
          sensor_cache_write(FRU_SMB, sensor_num[i], true, value);
        }
      }
    }

    pal_get_sensor_threshold(FRU_SMB, sensor_num[i], UCR_THRESH, &ucr);
    pal_get_sensor_threshold(FRU_SMB, sensor_num[i], LCR_THRESH, &lcr);

    if(value == 0) {
      OBMC_WARN("sensor %s value is 0", sensor_name);
      ret=1;
    }else if(value > ucr) {
      OBMC_WARN("sensor %s value is %.2f over than UCR %.2f ", sensor_name, value, ucr);
      ret=1;
    }else if(value < lcr) {
      OBMC_WARN("sensor %s value is %.2f less than LCR %.2f ", sensor_name, value, lcr);
      ret=1;
    }
  }

  return ret;
}

/*
 * @brief  checking sensor threshold on SMB/BMC
 * @note   will be used for sys_led
 * @param  none:
 * @retval 0 is normal , 1 is fail
 */
int smb_check() {
  int ret=0;
  float value=0, ucr=0, lcr=0;
  char sensor_name[32];
  int sensor_num[] = {
    SMB_SENSOR_1220_VMON1,               SMB_SENSOR_1220_VMON2,
    SMB_SENSOR_1220_VMON3,               SMB_SENSOR_1220_VMON4,
    SMB_SENSOR_1220_VMON5,               SMB_SENSOR_1220_VMON6,
    SMB_SENSOR_1220_VMON7,               SMB_SENSOR_1220_VMON8,
    SMB_SENSOR_1220_VMON9,               SMB_SENSOR_1220_VMON10,
    SMB_SENSOR_1220_VMON11,              SMB_SENSOR_1220_VMON12,
    SMB_SENSOR_1220_VCCA,                SMB_SENSOR_1220_VCCINP,

    SMB_SENSOR_VDDA_IN_VOLT,             SMB_SENSOR_VDDA_IN_CURR,
    SMB_SENSOR_VDDA_IN_POWER,            SMB_SENSOR_VDDA_OUT_VOLT,
    SMB_SENSOR_VDDA_OUT_CURR,            SMB_SENSOR_VDDA_OUT_POWER,
    SMB_SENSOR_VDDA_TEMP1,

    SMB_SENSOR_PCIE_IN_VOLT,             SMB_SENSOR_PCIE_IN_CURR,
    SMB_SENSOR_PCIE_IN_POWER,            SMB_SENSOR_PCIE_OUT_VOLT,
    SMB_SENSOR_PCIE_OUT_CURR,            SMB_SENSOR_PCIE_OUT_POWER,
    SMB_SENSOR_PCIE_TEMP1,

    SMB_SENSOR_IR3R3V_LEFT_IN_VOLT,      SMB_SENSOR_IR3R3V_LEFT_IN_CURR,
    SMB_SENSOR_IR3R3V_LEFT_IN_POWER,     SMB_SENSOR_IR3R3V_LEFT_OUT_VOLT,
    SMB_SENSOR_IR3R3V_LEFT_OUT_CURR,     SMB_SENSOR_IR3R3V_LEFT_OUT_POWER,
    SMB_SENSOR_IR3R3V_LEFT_TEMP,

    SMB_SENSOR_IR3R3V_RIGHT_IN_VOLT,     SMB_SENSOR_IR3R3V_RIGHT_IN_CURR,
    SMB_SENSOR_IR3R3V_RIGHT_IN_POWER,    SMB_SENSOR_IR3R3V_RIGHT_OUT_VOLT,
    SMB_SENSOR_IR3R3V_RIGHT_OUT_CURR,    SMB_SENSOR_IR3R3V_RIGHT_OUT_POWER,
    SMB_SENSOR_IR3R3V_RIGHT_TEMP,

    SMB_SENSOR_SW_CORE_IN_VOLT,          SMB_SENSOR_SW_CORE_IN_CURR,
    SMB_SENSOR_SW_CORE_IN_POWER,         SMB_SENSOR_SW_CORE_OUT_VOLT,
    SMB_SENSOR_SW_CORE_OUT_CURR,         SMB_SENSOR_SW_CORE_OUT_POWER,
    SMB_SENSOR_SW_CORE_TEMP1,

    SMB_SENSOR_XPDE_HBM_IN_VOLT,         SMB_SENSOR_XPDE_HBM_IN_CURR,
    SMB_SENSOR_XPDE_HBM_IN_POWER,        SMB_SENSOR_XPDE_HBM_OUT_VOLT,
    SMB_SENSOR_XPDE_HBM_OUT_CURR,        SMB_SENSOR_XPDE_HBM_OUT_POWER,
    SMB_SENSOR_XPDE_HBM_TEMP1,

    SMB_SENSOR_LM75B_U28_TEMP,           SMB_SENSOR_LM75B_U25_TEMP,
    SMB_SENSOR_LM75B_U56_TEMP,           SMB_SENSOR_LM75B_U55_TEMP,
    SMB_SENSOR_LM75B_U2_TEMP,            SMB_SENSOR_LM75B_U13_TEMP,
    SMB_SENSOR_TMP421_U62_TEMP,          SMB_SENSOR_TMP421_U63_TEMP,
    SMB_SENSOR_BMC_LM75B_TEMP,

    SMB_BMC_ADC0_VSEN,                   SMB_BMC_ADC1_VSEN,
    SMB_BMC_ADC2_VSEN,                   SMB_BMC_ADC3_VSEN,
    SMB_BMC_ADC4_VSEN,                   SMB_BMC_ADC5_VSEN,
    SMB_BMC_ADC6_VSEN,                   SMB_BMC_ADC7_VSEN,
    SMB_BMC_ADC8_VSEN,                   SMB_BMC_ADC9_VSEN,
    SMB_BMC_ADC10_VSEN,                  SMB_BMC_ADC11_VSEN,

    SMB_SENSOR_VDDCK_0_IN_VOLT,          SMB_SENSOR_VDDCK_0_IN_CURR,
    SMB_SENSOR_VDDCK_0_IN_POWER,         SMB_SENSOR_VDDCK_0_OUT_VOLT,
    SMB_SENSOR_VDDCK_0_OUT_CURR,         SMB_SENSOR_VDDCK_0_OUT_POWER,
    SMB_SENSOR_VDDCK_0_TEMP,

    SMB_SENSOR_VDDCK_1_IN_VOLT,          SMB_SENSOR_VDDCK_1_IN_CURR,
    SMB_SENSOR_VDDCK_1_IN_POWER,         SMB_SENSOR_VDDCK_1_OUT_VOLT,
    SMB_SENSOR_VDDCK_1_OUT_CURR,         SMB_SENSOR_VDDCK_1_OUT_POWER,
    SMB_SENSOR_VDDCK_1_TEMP,

    SMB_SENSOR_VDDCK_2_IN_VOLT,          SMB_SENSOR_VDDCK_2_IN_CURR,
    SMB_SENSOR_VDDCK_2_IN_POWER,         SMB_SENSOR_VDDCK_2_OUT_VOLT,
    SMB_SENSOR_VDDCK_2_OUT_CURR,         SMB_SENSOR_VDDCK_2_OUT_POWER,
    SMB_SENSOR_VDDCK_2_TEMP,

    SMB_SENSOR_XDPE_LEFT_1_IN_VOLT,      SMB_SENSOR_XDPE_LEFT_1_IN_CURR,
    SMB_SENSOR_XDPE_LEFT_1_IN_POWER,     SMB_SENSOR_XDPE_LEFT_1_OUT_VOLT,
    SMB_SENSOR_XDPE_LEFT_1_OUT_CURR,     SMB_SENSOR_XDPE_LEFT_1_OUT_POWER,
    SMB_SENSOR_XDPE_LEFT_1_TEMP,

    SMB_SENSOR_XDPE_LEFT_2_IN_VOLT,      SMB_SENSOR_XDPE_LEFT_2_IN_CURR,
    SMB_SENSOR_XDPE_LEFT_2_IN_POWER,     SMB_SENSOR_XDPE_LEFT_2_OUT_VOLT,
    SMB_SENSOR_XDPE_LEFT_2_OUT_CURR,     SMB_SENSOR_XDPE_LEFT_2_OUT_POWER,
    SMB_SENSOR_XDPE_LEFT_2_TEMP,

    SMB_SENSOR_XDPE_RIGHT_1_IN_VOLT,     SMB_SENSOR_XDPE_RIGHT_1_IN_CURR,
    SMB_SENSOR_XDPE_RIGHT_1_IN_POWER,    SMB_SENSOR_XDPE_RIGHT_1_OUT_VOLT,
    SMB_SENSOR_XDPE_RIGHT_1_OUT_CURR,    SMB_SENSOR_XDPE_RIGHT_1_OUT_POWER,
    SMB_SENSOR_XDPE_RIGHT_1_TEMP,

    SMB_SENSOR_XDPE_RIGHT_2_IN_VOLT,     SMB_SENSOR_XDPE_RIGHT_2_IN_CURR,
    SMB_SENSOR_XDPE_RIGHT_2_IN_POWER,    SMB_SENSOR_XDPE_RIGHT_2_OUT_VOLT,
    SMB_SENSOR_XDPE_RIGHT_2_OUT_CURR,    SMB_SENSOR_XDPE_RIGHT_2_OUT_POWER,
    SMB_SENSOR_XDPE_RIGHT_2_TEMP,

    SMB_DOM1_MAX_TEMP,                   SMB_DOM2_MAX_TEMP,

    /* GB switch internal sensors */
    SMB_SENSOR_GB_TEMP1,                 SMB_SENSOR_GB_TEMP2,
    SMB_SENSOR_GB_TEMP3,                 SMB_SENSOR_GB_TEMP4,
    SMB_SENSOR_GB_TEMP5,                 SMB_SENSOR_GB_TEMP6,
    SMB_SENSOR_GB_TEMP7,                 SMB_SENSOR_GB_TEMP8,
    SMB_SENSOR_GB_TEMP9,                 SMB_SENSOR_GB_TEMP10
  };

  for (int i=0; i<sizeof(sensor_num)/sizeof(sensor_num[0]); i++) {
    pal_get_sensor_name(FRU_SMB, sensor_num[i], sensor_name);
    if (sensor_cache_read(FRU_SMB, sensor_num[i], &value)) {
      OBMC_INFO("can't read %s from cache read raw instead", sensor_name);
      if (pal_sensor_read_raw(FRU_SMB, sensor_num[i], &value)) {
        OBMC_CRIT("can't read sensor %s", sensor_name);
        ret=1;
        continue;
      } else {
        // write cahed after read success
        sensor_cache_write(FRU_SMB, sensor_num[i], true, value);
      }
    }

    pal_get_sensor_threshold(FRU_SMB, sensor_num[i], UCR_THRESH, &ucr);
    pal_get_sensor_threshold(FRU_SMB, sensor_num[i], LCR_THRESH, &lcr);

    if (ucr != 0 && value > ucr) {
      OBMC_WARN("sensor %s value is %.2f over than UCR %.2f ", sensor_name, value, ucr);
      ret=1;
    } else if (lcr != 0 && value < lcr) {
      OBMC_WARN("sensor %s value is %.2f less than LCR %.2f ", sensor_name, value, lcr);
      ret=1;
    }
  }

  return ret;
}

/*
 * @brief  checking scm presence and sensor
 * @note   will be use for scm_led controling
 * @param  none:
 * @retval 0 is normal , 1 is fail
 */
int scm_check() {
  uint8_t status=0;
  int ret=0;
  char fru_name[8];
  uint8_t fru_list[] = { FRU_SCM };
  float value=0, ucr=0, lcr=0;
  char sensor_name[32];
  int sensor_num[] = {
    SCM_SENSOR_OUTLET_TEMP,            SCM_SENSOR_INLET_TEMP,
    SCM_SENSOR_HSC_IN_VOLT,            SCM_SENSOR_HSC_OUT_VOLT,
    SCM_SENSOR_HSC_OUT_CURR,

    /* Threshold Sensors on COM-e */
    BIC_SENSOR_MB_OUTLET_TEMP,         BIC_SENSOR_MB_INLET_TEMP,
    BIC_SENSOR_PCH_TEMP,               BIC_SENSOR_VCCIN_VR_TEMP,
    BIC_SENSOR_1V05COMB_VR_TEMP,       BIC_SENSOR_SOC_TEMP,
    BIC_SENSOR_SOC_THERM_MARGIN,       BIC_SENSOR_VDDR_VR_TEMP,
    BIC_SENSOR_SOC_DIMMA_TEMP,         BIC_SENSOR_SOC_DIMMB_TEMP,
    BIC_SENSOR_SOC_PACKAGE_PWR,        BIC_SENSOR_VCCIN_VR_POUT,
    BIC_SENSOR_VDDR_VR_POUT,           BIC_SENSOR_SOC_TJMAX,
    BIC_SENSOR_P3V3_MB,                BIC_SENSOR_P12V_MB,
    BIC_SENSOR_P1V05_PCH,              BIC_SENSOR_P3V3_STBY_MB,
    BIC_SENSOR_P5V_STBY_MB,            BIC_SENSOR_PV_BAT,
    BIC_SENSOR_PVDDR,                  BIC_SENSOR_P1V05_COMB,
    BIC_SENSOR_1V05COMB_VR_CURR,       BIC_SENSOR_VDDR_VR_CURR,
    BIC_SENSOR_VCCIN_VR_CURR,          BIC_SENSOR_VCCIN_VR_VOL,
    BIC_SENSOR_VDDR_VR_VOL,            BIC_SENSOR_P1V05COMB_VR_VOL,
    BIC_SENSOR_P1V05COMB_VR_POUT,      BIC_SENSOR_INA230_POWER,
    BIC_SENSOR_INA230_VOL
  };

  // check scm fru presence
  for (int fru = 0; fru < sizeof(fru_list) / sizeof(fru_list[0]); fru++) {
    pal_get_fru_name(fru_list[fru], fru_name);
    if(pal_is_fru_prsnt(fru_list[fru], &status)) {
      OBMC_CRIT("cannot get fru %s presence status", fru_name);
      continue;
    }
    if(status == 0) {
      OBMC_WARN("fru %s is not presence", fru_name);
      ret=1;
    }
  }

  // check threshold sensors
  for (int i=0; i<sizeof(sensor_num)/sizeof(sensor_num[0]); i++) {
    pal_get_sensor_name(FRU_SCM, sensor_num[i], sensor_name);
    if (sensor_cache_read(FRU_SCM, sensor_num[i], &value)) {
      OBMC_INFO("can't read %s from cache read raw instead", sensor_name);
      if (pal_sensor_read_raw(FRU_SCM, sensor_num[i], &value)) {
        OBMC_CRIT("can't read sensor %s", sensor_name);
        ret=1;
        continue;
      } else {
        // write cahed after read success
        sensor_cache_write(FRU_SCM, sensor_num[i], true, value);
      }
    }

    pal_get_sensor_threshold(FRU_SCM, sensor_num[i], UCR_THRESH, &ucr);
    pal_get_sensor_threshold(FRU_SCM, sensor_num[i], LCR_THRESH, &lcr);

    if (ucr != 0 && value > ucr) {
      OBMC_WARN("sensor %s value is %.2f over than UCR %.2f ", sensor_name, value, ucr);
      ret=1;
    } else if (lcr != 0 && value < lcr) {
      OBMC_WARN("sensor %s value is %.2f less than LCR %.2f ", sensor_name, value, lcr);
      ret=1;
    }
  }

  return ret;
}
