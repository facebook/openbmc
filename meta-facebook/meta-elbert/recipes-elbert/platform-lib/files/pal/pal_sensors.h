/*
 * Copyright 2021-present Facebook. All Rights Reserved.
 *
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

#ifndef __PAL_SENSORS_H__
#define __PAL_SENSORS_H__

#include <openbmc/obmc-pal.h>
#include <openbmc/kv.h>
#include <openbmc/obmc_pal_sensors.h>

#ifdef __cplusplus
extern "C" {
#endif

#define I2C_DEV_HWMON_DIR(dev) I2C_SYSFS_DEV_ENTRY(dev, hwmon/hwmon*)

// SCM Sensors
#define SCM_ECB_DEVICE      I2C_DEV_HWMON_DIR(11-0040)
#define SCM_MAX6658_DEVICE  I2C_DEV_HWMON_DIR(11-004c)
#define SCM_LM73_DEVICE     I2C_DEV_HWMON_DIR(15-004a)

// SMB Sensors
#define SMB_UCD90160_DEVICE    I2C_DEV_HWMON_DIR(3-004e)
#define SMB_RAA228228_DEVICE   I2C_DEV_HWMON_DIR(3-0060)
#define SMB_ISL68226_DEVICE    I2C_DEV_HWMON_DIR(3-0062)
#define SMB_MAX6581_DEVICE     I2C_DEV_HWMON_DIR(4-004d)
#define SMB_NET_BRCM_DEVICE    I2C_DEV_HWMON_DIR(4-0044)

// PIM Sensors
#define PIM_TPS546D24_UPPER_DEVICE_ADDR 0x16
#define PIM_TPS546D24_LOWER_DEVICE_ADDR 0x18
#define PIM_ECB_DEVICE_ADDR 0x40
#define PIM_LM73_DEVICE_ADDR 0x4a
#define PIM_UCD9090_DEVICE_ADDR 0x4e
#define PIM_ISL68224_DEVICE_ADDR 0x54

// PSU Sensors
#define PSU_DEVICE_ADDR 0x58

// FAN Sensors
#define FAN_CPLD_DEVICE    I2C_SYSFS_DEV_DIR(6-0060)
#define FAN_MAX6658_DEVICE I2C_DEV_HWMON_DIR(6-004c)

#define MAX_SENSOR_NUM 0xFF
#define MAX_SENSOR_THRESHOLD 8

#define TEMP(x)  "temp"#x"_input"
#define VOLT(x)  "in"#x"_input"
#define VOLT_SET(x)  "vo"#x"_input"
#define CURR(x)  "curr"#x"_input"
#define POWER(x) "power"#x"_input"
#define FAN(x) "fan"#x"_input"

#define TEMP_UNIT "C"
#define VOLT_UNIT "Volts"
#define CURR_UNIT "Amps"
#define POWER_UNIT "Watts"
#define FAN_UNIT "RPM"
#define CFM_UNIT "CFM"

#define UNIT_DIV 1000
#define READING_NA -2

// Used for system airflow calculation
#define PWM_MAX 255
#define CFM_STEP 10
#define KV_SYS_AIRFLOW_CFG_KEY "sys_airflow_cfg"
#define BASE_TEN 10

// Sensors on SCM
enum {
  SCM_ECB_VIN = 1,
  SCM_ECB_VOUT,
  SCM_ECB_CURR,
  SCM_ECB_POWER,
  SCM_ECB_TEMP,
  SCM_BOARD_TEMP,
  SCM_INLET_TEMP,
  SCM_BMC_TEMP,
};

// Sensors on SMB
enum {
  SMB_POS_0V75_CORE = 1,
  SMB_POS_0V75_ANLG,
  SMB_POS_1V2,
  SMB_POS_1V2_ANLG_L,
  SMB_POS_1V2_ANLG_R,
  SMB_POS_1V8,
  SMB_POS_1V8_ANLG,
  SMB_POS_3V3,
  SMB_POS_3V3_DPM,
  SMB_POS_5V0,
  SMB_POS_12V_B,
  SMB_POS_12V_A,
  SMB_POS_1V2B_E,
  SMB_POS_2V5B_E,
  SMB_POS_3V3B_E,
  TH4_VRD1_VIN,
  TH4_VRD1_VOUT_LOOP0,
  TH4_VRD1_TEMP_LOOP0,
  TH4_VRD1_PIN,
  TH4_VRD1_POUT_LOOP0,
  TH4_VRD1_IIN,
  TH4_VRD1_IOUT_LOOP0,
  TH4_VRD2_VIN,
  TH4_VRD2_VOUT_LOOP0,
  TH4_VRD2_VOUT_LOOP1,
  TH4_VRD2_VOUT_LOOP2,
  TH4_VRD2_TEMP_LOOP0,
  TH4_VRD2_TEMP_LOOP1,
  TH4_VRD2_TEMP_LOOP2,
  TH4_VRD2_TEMP_INTERNAL,
  TH4_VRD2_PIN_LOOP0,
  TH4_VRD2_PIN_LOOP1,
  TH4_VRD2_PIN_LOOP2,
  TH4_VRD2_POUT_LOOP0,
  TH4_VRD2_POUT_LOOP1,
  TH4_VRD2_POUT_LOOP2,
  TH4_VRD2_IIN_LOOP0,
  TH4_VRD2_IIN_LOOP1,
  TH4_VRD2_IIN_LOOP2,
  TH4_VRD2_IOUT_LOOP0,
  TH4_VRD2_IOUT_LOOP1,
  TH4_VRD2_IOUT_LOOP2,
  SMB_T,
  TH4_DIE_TEMP_0,
  TH4_DIE_TEMP_1,
  SMB_R,
  SMB_U,
  SMB_L,
  BLACKHAWK_CORE_0_5_TEMP,
  BLACKHAWK_CORE_6_12_TEMP,
  BLACKHAWK_CORE_13_18_TEMP,
  BLACKHAWK_CORE_19_25_TEMP,
  BLACKHAWK_CORE_26_31_TEMP,
  BLACKHAWK_CORE_32_37_TEMP,
  BLACKHAWK_CORE_38_44_TEMP,
  BLACKHAWK_CORE_45_50_TEMP,
  BLACKHAWK_CORE_51_57_TEMP,
  BLACKHAWK_CORE_58_63_TEMP,
  CORE_0_TEMP,
  CORE_1_TEMP,
  CORE_2_TEMP,
  CORE_3_TEMP,
  PIM_QSFP200,
  PIM_QSFP400,
  PIM_F104,
};

// Sensors on PIM
enum {
  PIM_POS_3V3_U_VOUT = 1,
  PIM_POS_3V3_U_TEMP,
  PIM_POS_3V3_U_CURR,
  PIM_POS_3V3_L_VOUT,
  PIM_POS_3V3_L_TEMP,
  PIM_POS_3V3_L_CURR,
  PIM16Q2_SENSOR_COUNT = PIM_POS_3V3_L_CURR,
  PIM_LM73_TEMP,
  PIM_POS_12V,
  PIM_POS_3V3_E,
  PIM_POS_1V2_E,
  PIM_POS_3V3_U,
  PIM_POS_3V3_L,
  PIM_POS_3V8_LEDS,
  PIM_DPM_TEMP,
  PIM16Q_SENSOR_COUNT = PIM_DPM_TEMP,
  PIM_POS_5V0,
  PIM_POS_0V9,
  PIM_POS_1V5,
  PIM_POS_1V8,
  PIM_ISL_VIN,
  PIM_ISL_VOUT_LOOP0,
  PIM_ISL_VOUT_LOOP1,
  PIM_ISL_VOUT_LOOP2,
  PIM_ISL_TEMP_LOOP0,
  PIM_ISL_TEMP_LOOP1,
  PIM_ISL_TEMP_LOOP2,
  PIM_ISL_TEMP_INTERNAL,
  PIM_ISL_PIN_LOOP0,
  PIM_ISL_PIN_LOOP1,
  PIM_ISL_PIN_LOOP2,
  PIM_ISL_POUT_LOOP0,
  PIM_ISL_POUT_LOOP1,
  PIM_ISL_POUT_LOOP2,
  PIM_ISL_IIN_LOOP0,
  PIM_ISL_IIN_LOOP1,
  PIM_ISL_IIN_LOOP2,
  PIM_ISL_IOUT_LOOP0,
  PIM_ISL_IOUT_LOOP1,
  PIM_ISL_IOUT_LOOP2,
  PIM_ECB_VIN,
  PIM_ECB_VOUT,
  PIM_ECB_TEMP,
  PIM_ECB_POWER,
  PIM_ECB_CURR,
  // -1 because 3V8_LEDS sensor is not on PIM8DDM
  PIM8DDM_SENSOR_COUNT = PIM_ECB_CURR - 1,
};

// Sensors on PSUs
enum {
  // Sensors on PSU1
  PSU1_VIN = 1,
  PSU1_VOUT,
  PSU1_FAN,
  PSU1_TEMP1,
  PSU1_TEMP2,
  PSU1_TEMP3,
  PSU1_PIN,
  PSU1_POUT,
  PSU1_IIN,
  PSU1_IOUT,
  PSU_SENSOR_COUNT = PSU1_IOUT,
  PSU2_VIN,
  PSU2_VOUT,
  PSU2_FAN,
  PSU2_TEMP1,
  PSU2_TEMP2,
  PSU2_TEMP3,
  PSU2_PIN,
  PSU2_POUT,
  PSU2_IIN,
  PSU2_IOUT,
  PSU3_VIN,
  PSU3_VOUT,
  PSU3_FAN,
  PSU3_TEMP1,
  PSU3_TEMP2,
  PSU3_TEMP3,
  PSU3_PIN,
  PSU3_POUT,
  PSU3_IIN,
  PSU3_IOUT,
  PSU4_VIN,
  PSU4_VOUT,
  PSU4_FAN,
  PSU4_TEMP1,
  PSU4_TEMP2,
  PSU4_TEMP3,
  PSU4_PIN,
  PSU4_POUT,
  PSU4_IIN,
  PSU4_IOUT,
};

// Sensors on Fan Card
enum {
  FAN1_RPM = 1,
  FAN2_RPM,
  FAN3_RPM,
  FAN4_RPM,
  FAN5_RPM,
  FAN_CARD_BOARD_TEMP,
  FAN_CARD_OUTLET_TEMP,
  SYSTEM_AIRFLOW,
};

enum
{
  CONFIG_8PIM16Q_0PIM8DDM_2PSU,
  CONFIG_5PIM16Q_3PIM8DDM_2PSU,
  CONFIG_2PIM16Q_6PIM8DDM_4PSU,
  CONFIG_0PIM16Q_8PIM8DDM_4PSU,
  CONFIG_UNKNOWN,
};

int pal_get_fru_sensor_list(uint8_t fru, uint8_t **sensor_list, int *cnt);
int pal_sensor_read_raw(uint8_t fru, uint8_t sensor_num, void *value);
int pal_get_sensor_name(uint8_t fru, uint8_t sensor_num, char *name);
int pal_get_sensor_units(uint8_t fru, uint8_t sensor_num, char *units);
int pal_get_sensor_threshold(uint8_t fru, uint8_t sensor_num, uint8_t thresh, void *value);
int pal_sensor_threshold_flag(uint8_t fru, uint8_t snr_num, uint16_t *flag);
int pal_set_pim_thresh(uint8_t fru);
int pal_clear_thresh_value(uint8_t fru);
int pal_init_sensor_check(uint8_t fru, uint8_t snr_num, void *snr);
int pal_set_sdr_update_flag(uint8_t slot, uint8_t update);
int pal_get_sdr_update_flag(uint8_t slot);
int pal_get_sensor_util_timeout(uint8_t fru);

#ifdef __cplusplus
} // extern "C"
#endif

#endif /* __PAL_SENSORS_H__ */
