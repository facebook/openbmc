/*
 *
 * Copyright 2015-present Facebook. All Rights Reserved.
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
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <math.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <syslog.h>
#include <facebook/i2c.h>
#include "fbttn_sensor.h"
//For Kernel 2.6 -> 4.1
#define MEZZ_TEMP_DEVICE "/sys/devices/platform/ast-i2c.12/i2c-12/12-001f/hwmon/hwmon*"

#define LARGEST_DEVICE_NAME 120

#define GPIO_VAL "/sys/class/gpio/gpio%d/value"
#define PCA9555_VAL "/sys/devices/platform/ast-i2c.5/i2c-5/5-0024/gpio/gpio%d/value"

#define I2C_BUS_0_DIR "/sys/devices/platform/ast-i2c.0/i2c-0/"
#define I2C_BUS_5_DIR "/sys/devices/platform/ast-i2c.5/i2c-5/"
#define I2C_BUS_6_DIR "/sys/devices/platform/ast-i2c.6/i2c-6/"
#define I2C_BUS_9_DIR "/sys/devices/platform/ast-i2c.9/i2c-9/"
#define I2C_BUS_10_DIR "/sys/devices/platform/ast-i2c.10/i2c-10/"

#define TACH_DIR "/sys/devices/platform/ast_pwm_tacho.0"
#define ADC_DIR "/sys/devices/platform/ast_adc.0"

#define IOM_MEZZ_TEMP_DEVICE "/sys/devices/platform/ast-i2c.0/i2c-0/0-004a/hwmon/hwmon*"
#define IOM_M2_AMBIENT_TEMP1_DEVICE "/sys/devices/platform/ast-i2c.0/i2c-0/0-0048/hwmon/hwmon*"
#define IOM_M2_AMBIENT_TEMP2_DEVICE "/sys/devices/platform/ast-i2c.0/i2c-0/0-004c/hwmon/hwmon*"
#define HSC_DEVICE_IOM "/sys/devices/platform/ast-i2c.0/i2c-0/0-0010/hwmon/hwmon*"

#define I2C_M2_1 "/dev/i2c-7"
#define I2C_M2_2 "/dev/i2c-8"
#define I2C_NVME_INTF_ADDR 0x6a
#define M2_TEMP_REG 0x03
#define M2_1_GPIO_PRESENT "/sys/class/gpio/gpio32/value"
#define M2_2_GPIO_PRESENT "/sys/class/gpio/gpio33/value"
#define IOM_M2_1_TEMP_DEVICE 1
#define IOM_M2_2_TEMP_DEVICE 2

#define HSC_DEVICE_DPB "/sys/devices/platform/ast-i2c.6/i2c-6/6-0010/hwmon/hwmon*"
#define HSC_DEVICE_ML  "/sys/devices/platform/ast-i2c.5/i2c-5/5-0010/hwmon/hwmon*"

#define FAN_TACH_RPM "tacho%d_rpm"
#define ADC_VALUE "adc%d_value"
#define HSC_IN_VOLT "in1_input"
#define HSC_OUT_CURR "curr1_input"
#define HSC_TEMP "temp1_input"
#define HSC_IN_POWER "power1_input"

#define UNIT_DIV 1000

#define I2C_DEV_NIC "/dev/i2c-11"
#define I2C_NIC_ADDR 0x1f
#define I2C_NIC_SENSOR_TEMP_REG 0x01

#define BIC_SENSOR_READ_NA 0x20

#define MAX_SENSOR_NUM 0xFF
#define ALL_BYTES 0xFF
#define LAST_REC_ID 0xFFFF

#define FBTTN_SDR_PATH "/tmp/sdr_%s.bin"

#define IOM_ADM1278_R_SENSE  2 /* R_sense resistor value in microohms   */
#define ML_ADM1278_R_SENSE  1
#define EXP_R_SENSE  1

static int iom_hsc_r_sense = IOM_ADM1278_R_SENSE;
static int ml_hsc_r_sense = ML_ADM1278_R_SENSE;

// List of BIC sensors to be monitored
const uint8_t bic_sensor_list[] = {
  /* Threshold sensors */
  BIC_SENSOR_MB_OUTLET_TEMP,
  BIC_SENSOR_VCCIN_VR_TEMP,
  BIC_SENSOR_VCC_GBE_VR_TEMP,
  BIC_SENSOR_1V05PCH_VR_TEMP,
  BIC_SENSOR_SOC_TEMP,
  BIC_SENSOR_MB_INLET_TEMP,
  BIC_SENSOR_PCH_TEMP,
  BIC_SENSOR_SOC_THERM_MARGIN,
  BIC_SENSOR_VDDR_VR_TEMP,
  BIC_SENSOR_VCC_GBE_VR_CURR,
  BIC_SENSOR_1V05_PCH_VR_CURR,
  BIC_SENSOR_VCCIN_VR_POUT,
  BIC_SENSOR_VCCIN_VR_CURR,
  BIC_SENSOR_VCCIN_VR_VOL,
  BIC_SENSOR_INA230_POWER,
  BIC_SENSOR_INA230_VOL,
  BIC_SENSOR_SOC_PACKAGE_PWR,
  BIC_SENSOR_SOC_TJMAX,
  BIC_SENSOR_VDDR_VR_POUT,
  BIC_SENSOR_VDDR_VR_CURR,
  BIC_SENSOR_VDDR_VR_VOL,
  BIC_SENSOR_VCC_SCSUS_VR_CURR,
  BIC_SENSOR_VCC_SCSUS_VR_VOL,
  BIC_SENSOR_VCC_SCSUS_VR_TEMP,
  BIC_SENSOR_VCC_SCSUS_VR_POUT,
  BIC_SENSOR_VCC_GBE_VR_POUT,
  BIC_SENSOR_VCC_GBE_VR_VOL,
  BIC_SENSOR_1V05_PCH_VR_POUT,
  BIC_SENSOR_1V05_PCH_VR_VOL,
  BIC_SENSOR_SOC_DIMMA0_TEMP,
  BIC_SENSOR_SOC_DIMMA1_TEMP,
  BIC_SENSOR_SOC_DIMMB0_TEMP,
  BIC_SENSOR_SOC_DIMMB1_TEMP,
  BIC_SENSOR_P3V3_MB,
  BIC_SENSOR_P12V_MB,
  BIC_SENSOR_P1V05_PCH,
  BIC_SENSOR_P3V3_STBY_MB,
  BIC_SENSOR_P5V_STBY_MB,
  BIC_SENSOR_PV_BAT,
  BIC_SENSOR_PVDDR,
  BIC_SENSOR_PVCC_GBE,
};

const uint8_t bic_discrete_list[] = {
  /* Discrete sensors */
  BIC_SENSOR_SYSTEM_STATUS,
  BIC_SENSOR_VR_HOT,
  BIC_SENSOR_CPU_DIMM_HOT,
};

// List of IOM Type VII sensors to be monitored
const uint8_t iom_sensor_list_type5[] = {
  /* Threshold sensors */
  ML_SENSOR_HSC_VOLT,
  ML_SENSOR_HSC_CURR,
  ML_SENSOR_HSC_PWR,
  IOM_SENSOR_MEZZ_TEMP,
  IOM_SENSOR_HSC_POWER,
  IOM_SENSOR_HSC_VOLT,
  IOM_SENSOR_HSC_CURR,
  IOM_SENSOR_ADC_12V,
  IOM_SENSOR_ADC_P5V_STBY,
  IOM_SENSOR_ADC_P3V3_STBY,
  IOM_SENSOR_ADC_P1V8_STBY,
  IOM_SENSOR_ADC_P2V5_STBY,
  IOM_SENSOR_ADC_P1V2_STBY,
  IOM_SENSOR_ADC_P1V15_STBY,
  IOM_SENSOR_ADC_P3V3,
  IOM_SENSOR_ADC_P1V8,
  IOM_SENSOR_ADC_P3V3_M2,
  IOM_SENSOR_M2_AMBIENT_TEMP1,
  IOM_SENSOR_M2_AMBIENT_TEMP2,
  IOM_SENSOR_M2_SMART_TEMP1,
  IOM_SENSOR_M2_SMART_TEMP2,
};

// List of IOM Type VII sensors to be monitored
const uint8_t iom_sensor_list_type7[] = {
  /* Threshold sensors */
  ML_SENSOR_HSC_VOLT,
  ML_SENSOR_HSC_CURR,
  ML_SENSOR_HSC_PWR,
  IOM_SENSOR_MEZZ_TEMP,
  IOM_SENSOR_HSC_POWER,
  IOM_SENSOR_HSC_VOLT,
  IOM_SENSOR_HSC_CURR,
  IOM_SENSOR_ADC_12V,
  IOM_SENSOR_ADC_P5V_STBY,
  IOM_SENSOR_ADC_P3V3_STBY,
  IOM_SENSOR_ADC_P1V8_STBY,
  IOM_SENSOR_ADC_P2V5_STBY,
  IOM_SENSOR_ADC_P1V2_STBY,
  IOM_SENSOR_ADC_P1V15_STBY,
  IOM_SENSOR_ADC_P3V3,
  IOM_SENSOR_ADC_P1V8,
  IOM_SENSOR_ADC_P1V5,
  IOM_SENSOR_ADC_P0V975,
  IOM_IOC_TEMP,
};

// List of DPB sensors to be monitored
const uint8_t dpb_sensor_list[] = {
  /* Threshold sensors */
  P3V3_SENSE,
  P5V_A_1_SENSE,
  P5V_A_2_SENSE,
  P5V_A_3_SENSE,
  P5V_A_4_SENSE,
  DPB_SENSOR_12V_POWER_CLIP,
  DPB_SENSOR_P12V_CLIP,
  DPB_SENSOR_12V_CURR_CLIP,
  DPB_SENSOR_A_TEMP,
  DPB_SENSOR_B_TEMP,
  DPB_SENSOR_FAN0_FRONT,
  DPB_SENSOR_FAN0_REAR,
  DPB_SENSOR_FAN1_FRONT,
  DPB_SENSOR_FAN1_REAR,
  DPB_SENSOR_FAN2_FRONT,
  DPB_SENSOR_FAN2_REAR,
  DPB_SENSOR_FAN3_FRONT,
  DPB_SENSOR_FAN3_REAR,
  DPB_SENSOR_HSC_POWER,
  DPB_SENSOR_HSC_VOLT,
  DPB_SENSOR_HSC_CURR,
  DPB_SENSOR_HDD_0,
  DPB_SENSOR_HDD_1,
  DPB_SENSOR_HDD_2,
  DPB_SENSOR_HDD_3,
  DPB_SENSOR_HDD_4,
  DPB_SENSOR_HDD_5,
  DPB_SENSOR_HDD_6,
  DPB_SENSOR_HDD_7,
  DPB_SENSOR_HDD_8,
  DPB_SENSOR_HDD_9,
  DPB_SENSOR_HDD_10,
  DPB_SENSOR_HDD_11,
  DPB_SENSOR_HDD_12,
  DPB_SENSOR_HDD_13,
  DPB_SENSOR_HDD_14,
  DPB_SENSOR_HDD_15,
  DPB_SENSOR_HDD_16,
  DPB_SENSOR_HDD_17,
  DPB_SENSOR_HDD_18,
  DPB_SENSOR_HDD_19,
  DPB_SENSOR_HDD_20,
  DPB_SENSOR_HDD_21,
  DPB_SENSOR_HDD_22,
  DPB_SENSOR_HDD_23,
  DPB_SENSOR_HDD_24,
  DPB_SENSOR_HDD_25,
  DPB_SENSOR_HDD_26,
  DPB_SENSOR_HDD_27,
  DPB_SENSOR_HDD_28,
  DPB_SENSOR_HDD_29,
  DPB_SENSOR_HDD_30,
  DPB_SENSOR_HDD_31,
  DPB_SENSOR_HDD_32,
  DPB_SENSOR_HDD_33,
  DPB_SENSOR_HDD_34,
  DPB_SENSOR_HDD_35,
};

const uint8_t dpb_discrete_list[] = {
  /* Discrete sensors */
};

// List of SCC sensors to be monitored
const uint8_t scc_sensor_list[] = {
  /* Threshold sensors */
  SCC_SENSOR_EXPANDER_TEMP,
  SCC_SENSOR_IOC_TEMP,
  SCC_SENSOR_HSC_POWER,
  SCC_SENSOR_HSC_CURR,
  SCC_SENSOR_HSC_VOLT,
  SCC_SENSOR_P3V3_SENSE,
  SCC_SENSOR_P1V8_E_SENSE,
  SCC_SENSOR_P1V5_E_SENSE,
  SCC_SENSOR_P0V9_SENSE,
  SCC_SENSOR_P1V8_C_SENSE,
  SCC_SENSOR_P1V5_C_SENSE,
  SCC_SENSOR_P0V975_SENSE,
};

// List of NIC sensors to be monitored
const uint8_t nic_sensor_list[] = {
  MEZZ_SENSOR_TEMP,
};

float iom_sensor_threshold[MAX_SENSOR_NUM][MAX_SENSOR_THRESHOLD + 1] = {0};
float dpb_sensor_threshold[MAX_SENSOR_NUM][MAX_SENSOR_THRESHOLD + 1] = {0};
float scc_sensor_threshold[MAX_SENSOR_NUM][MAX_SENSOR_THRESHOLD + 1] = {0};
float nic_sensor_threshold[MAX_SENSOR_NUM][MAX_SENSOR_THRESHOLD + 1] = {0};

static void
sensor_thresh_array_init() {
  static bool init_done = false;

  if (init_done)
    return;

  // IOM
  iom_sensor_threshold[ML_SENSOR_HSC_VOLT][UCR_THRESH] = 13.75;
  iom_sensor_threshold[ML_SENSOR_HSC_VOLT][UNC_THRESH] = 13.38;
  iom_sensor_threshold[ML_SENSOR_HSC_VOLT][LCR_THRESH] = 11.25;
  iom_sensor_threshold[ML_SENSOR_HSC_VOLT][LNC_THRESH] = 11.63;
  iom_sensor_threshold[ML_SENSOR_HSC_CURR][UCR_THRESH] = 12;
  iom_sensor_threshold[ML_SENSOR_HSC_CURR][UNC_THRESH] = 10.4;
  iom_sensor_threshold[ML_SENSOR_HSC_PWR][UCR_THRESH] = 150;
  iom_sensor_threshold[ML_SENSOR_HSC_PWR][UNC_THRESH] = 130;
  iom_sensor_threshold[IOM_SENSOR_MEZZ_TEMP][UCR_THRESH] = 85;
  iom_sensor_threshold[IOM_SENSOR_HSC_VOLT][UCR_THRESH] = 13.75;
  iom_sensor_threshold[IOM_SENSOR_HSC_VOLT][UNC_THRESH] = 13.38;
  iom_sensor_threshold[IOM_SENSOR_HSC_VOLT][LCR_THRESH] = 11.25;
  iom_sensor_threshold[IOM_SENSOR_HSC_VOLT][LNC_THRESH] = 11.63;
  iom_sensor_threshold[IOM_SENSOR_HSC_CURR][UCR_THRESH] = 4;
  iom_sensor_threshold[IOM_SENSOR_HSC_CURR][UNC_THRESH] = 3;
  iom_sensor_threshold[IOM_SENSOR_HSC_POWER][UCR_THRESH] = 50;
  iom_sensor_threshold[IOM_SENSOR_HSC_POWER][UNC_THRESH] = 37.5;
  iom_sensor_threshold[IOM_SENSOR_ADC_12V][UCR_THRESH] = 13.75;
  iom_sensor_threshold[IOM_SENSOR_ADC_12V][UNC_THRESH] = 13.38;
  iom_sensor_threshold[IOM_SENSOR_ADC_12V][LCR_THRESH] = 11.25;
  iom_sensor_threshold[IOM_SENSOR_ADC_12V][LNC_THRESH] = 11.63;
  iom_sensor_threshold[IOM_SENSOR_ADC_P5V_STBY][UCR_THRESH] = 5.5;
  iom_sensor_threshold[IOM_SENSOR_ADC_P5V_STBY][UNC_THRESH] = 5.35;
  iom_sensor_threshold[IOM_SENSOR_ADC_P5V_STBY][LCR_THRESH] = 4.5;
  iom_sensor_threshold[IOM_SENSOR_ADC_P5V_STBY][LNC_THRESH] = 4.65;
  iom_sensor_threshold[IOM_SENSOR_ADC_P3V3_STBY][UCR_THRESH] = 3.63;
  iom_sensor_threshold[IOM_SENSOR_ADC_P3V3_STBY][UNC_THRESH] = 3.53;
  iom_sensor_threshold[IOM_SENSOR_ADC_P3V3_STBY][LCR_THRESH] = 2.97;
  iom_sensor_threshold[IOM_SENSOR_ADC_P3V3_STBY][LNC_THRESH] = 3.07;
  iom_sensor_threshold[IOM_SENSOR_ADC_P1V8_STBY][UCR_THRESH] = 1.98;
  iom_sensor_threshold[IOM_SENSOR_ADC_P1V8_STBY][UNC_THRESH] = 1.93;
  iom_sensor_threshold[IOM_SENSOR_ADC_P1V8_STBY][LCR_THRESH] = 1.62;
  iom_sensor_threshold[IOM_SENSOR_ADC_P1V8_STBY][LNC_THRESH] = 1.67;
  iom_sensor_threshold[IOM_SENSOR_ADC_P2V5_STBY][UCR_THRESH] = 2.75;
  iom_sensor_threshold[IOM_SENSOR_ADC_P2V5_STBY][UNC_THRESH] = 2.68;
  iom_sensor_threshold[IOM_SENSOR_ADC_P2V5_STBY][LCR_THRESH] = 2.25;
  iom_sensor_threshold[IOM_SENSOR_ADC_P2V5_STBY][LNC_THRESH] = 2.33;
  iom_sensor_threshold[IOM_SENSOR_ADC_P1V2_STBY][UCR_THRESH] = 1.32;
  iom_sensor_threshold[IOM_SENSOR_ADC_P1V2_STBY][UNC_THRESH] = 1.28;
  iom_sensor_threshold[IOM_SENSOR_ADC_P1V2_STBY][LCR_THRESH] = 1.08;
  iom_sensor_threshold[IOM_SENSOR_ADC_P1V2_STBY][LNC_THRESH] = 1.12;
  iom_sensor_threshold[IOM_SENSOR_ADC_P1V15_STBY][UCR_THRESH] = 1.27;
  iom_sensor_threshold[IOM_SENSOR_ADC_P1V15_STBY][UNC_THRESH] = 1.23;
  iom_sensor_threshold[IOM_SENSOR_ADC_P1V15_STBY][LCR_THRESH] = 1.04;
  iom_sensor_threshold[IOM_SENSOR_ADC_P1V15_STBY][LNC_THRESH] = 1.07;
  iom_sensor_threshold[IOM_SENSOR_ADC_P3V3][UCR_THRESH] = 3.63;
  iom_sensor_threshold[IOM_SENSOR_ADC_P3V3][UNC_THRESH] = 3.53;
  iom_sensor_threshold[IOM_SENSOR_ADC_P3V3][LCR_THRESH] = 2.97;
  iom_sensor_threshold[IOM_SENSOR_ADC_P3V3][LNC_THRESH] = 3.07;
  iom_sensor_threshold[IOM_SENSOR_ADC_P1V8][UCR_THRESH] = 1.98;
  iom_sensor_threshold[IOM_SENSOR_ADC_P1V8][UNC_THRESH] = 1.93;
  iom_sensor_threshold[IOM_SENSOR_ADC_P1V8][LCR_THRESH] = 1.62;
  iom_sensor_threshold[IOM_SENSOR_ADC_P1V8][LNC_THRESH] = 1.67;
  iom_sensor_threshold[IOM_SENSOR_ADC_P1V5][UCR_THRESH] = 1.65; // for Type VII
  iom_sensor_threshold[IOM_SENSOR_ADC_P1V5][UNC_THRESH] = 1.61; // for Type VII
  iom_sensor_threshold[IOM_SENSOR_ADC_P1V5][LCR_THRESH] = 1.35; // for Type VII
  iom_sensor_threshold[IOM_SENSOR_ADC_P1V5][LNC_THRESH] = 1.4; // for Type VII
  iom_sensor_threshold[IOM_SENSOR_ADC_P0V975][UCR_THRESH] = 1.07; // for Type VII
  iom_sensor_threshold[IOM_SENSOR_ADC_P0V975][UNC_THRESH] = 1.04; // for Type VII
  iom_sensor_threshold[IOM_SENSOR_ADC_P0V975][LCR_THRESH] = 0.81; // for Type VII
  iom_sensor_threshold[IOM_SENSOR_ADC_P0V975][LNC_THRESH] = 0.84; // for Type VII
  iom_sensor_threshold[IOM_SENSOR_ADC_P3V3_M2][UCR_THRESH] = 3.63; // for Type V
  iom_sensor_threshold[IOM_SENSOR_ADC_P3V3_M2][UNC_THRESH] = 3.53; // for Type V
  iom_sensor_threshold[IOM_SENSOR_ADC_P3V3_M2][LCR_THRESH] = 2.97; // for Type V
  iom_sensor_threshold[IOM_SENSOR_ADC_P3V3_M2][LNC_THRESH] = 3.07; // for Type V
  iom_sensor_threshold[IOM_SENSOR_M2_AMBIENT_TEMP1][UCR_THRESH] = 70; // for Type V
  iom_sensor_threshold[IOM_SENSOR_M2_AMBIENT_TEMP2][UCR_THRESH] = 70; // for Type V
  iom_sensor_threshold[IOM_SENSOR_M2_SMART_TEMP1][UCR_THRESH] = 82; // for Type V
  iom_sensor_threshold[IOM_SENSOR_M2_SMART_TEMP2][UCR_THRESH] = 82; // for Type V
  iom_sensor_threshold[IOM_IOC_TEMP][UCR_THRESH] = 100; // for Type VII

  // DPB
  dpb_sensor_threshold[DPB_SENSOR_12V_POWER_CLIP][UCR_THRESH] = 1875;
  dpb_sensor_threshold[DPB_SENSOR_P12V_CLIP][UCR_THRESH] = 13.73;
  dpb_sensor_threshold[DPB_SENSOR_P12V_CLIP][LCR_THRESH] = 11;
  dpb_sensor_threshold[DPB_SENSOR_12V_CURR_CLIP][UCR_THRESH] = 150;
  dpb_sensor_threshold[DPB_SENSOR_FAN0_FRONT][LCR_THRESH] = 400;
  dpb_sensor_threshold[DPB_SENSOR_FAN0_REAR][LCR_THRESH] = 400;
  dpb_sensor_threshold[DPB_SENSOR_FAN1_FRONT][LCR_THRESH] = 400;
  dpb_sensor_threshold[DPB_SENSOR_FAN1_REAR][LCR_THRESH] = 400;
  dpb_sensor_threshold[DPB_SENSOR_FAN2_FRONT][LCR_THRESH] = 400;
  dpb_sensor_threshold[DPB_SENSOR_FAN2_REAR][LCR_THRESH] = 400;
  dpb_sensor_threshold[DPB_SENSOR_FAN3_FRONT][LCR_THRESH] = 400;
  dpb_sensor_threshold[DPB_SENSOR_FAN3_REAR][LCR_THRESH] = 400;
  dpb_sensor_threshold[DPB_SENSOR_HSC_POWER][UCR_THRESH] = 885;
  dpb_sensor_threshold[DPB_SENSOR_HSC_VOLT][UCR_THRESH] = 13.73;
  dpb_sensor_threshold[DPB_SENSOR_HSC_VOLT][LCR_THRESH] = 11;
  dpb_sensor_threshold[DPB_SENSOR_HSC_CURR][UCR_THRESH] = 70.8;
  dpb_sensor_threshold[DPB_SENSOR_A_TEMP][UCR_THRESH] = 60;
  dpb_sensor_threshold[DPB_SENSOR_B_TEMP][UCR_THRESH] = 60;
  dpb_sensor_threshold[DPB_SENSOR_HDD_0][UCR_THRESH] = 65;
  dpb_sensor_threshold[DPB_SENSOR_HDD_1][UCR_THRESH] = 65;
  dpb_sensor_threshold[DPB_SENSOR_HDD_2][UCR_THRESH] = 65;
  dpb_sensor_threshold[DPB_SENSOR_HDD_3][UCR_THRESH] = 65;
  dpb_sensor_threshold[DPB_SENSOR_HDD_4][UCR_THRESH] = 65;
  dpb_sensor_threshold[DPB_SENSOR_HDD_5][UCR_THRESH] = 65;
  dpb_sensor_threshold[DPB_SENSOR_HDD_6][UCR_THRESH] = 65;
  dpb_sensor_threshold[DPB_SENSOR_HDD_7][UCR_THRESH] = 65;
  dpb_sensor_threshold[DPB_SENSOR_HDD_8][UCR_THRESH] = 65;
  dpb_sensor_threshold[DPB_SENSOR_HDD_9][UCR_THRESH] = 65;
  dpb_sensor_threshold[DPB_SENSOR_HDD_10][UCR_THRESH] = 65;
  dpb_sensor_threshold[DPB_SENSOR_HDD_11][UCR_THRESH] = 65;
  dpb_sensor_threshold[DPB_SENSOR_HDD_12][UCR_THRESH] = 65;
  dpb_sensor_threshold[DPB_SENSOR_HDD_13][UCR_THRESH] = 65;
  dpb_sensor_threshold[DPB_SENSOR_HDD_14][UCR_THRESH] = 65;
  dpb_sensor_threshold[DPB_SENSOR_HDD_15][UCR_THRESH] = 65;
  dpb_sensor_threshold[DPB_SENSOR_HDD_16][UCR_THRESH] = 65;
  dpb_sensor_threshold[DPB_SENSOR_HDD_17][UCR_THRESH] = 65;
  dpb_sensor_threshold[DPB_SENSOR_HDD_18][UCR_THRESH] = 65;
  dpb_sensor_threshold[DPB_SENSOR_HDD_19][UCR_THRESH] = 65;
  dpb_sensor_threshold[DPB_SENSOR_HDD_20][UCR_THRESH] = 65;
  dpb_sensor_threshold[DPB_SENSOR_HDD_21][UCR_THRESH] = 65;
  dpb_sensor_threshold[DPB_SENSOR_HDD_22][UCR_THRESH] = 65;
  dpb_sensor_threshold[DPB_SENSOR_HDD_23][UCR_THRESH] = 65;
  dpb_sensor_threshold[DPB_SENSOR_HDD_24][UCR_THRESH] = 65;
  dpb_sensor_threshold[DPB_SENSOR_HDD_25][UCR_THRESH] = 65;
  dpb_sensor_threshold[DPB_SENSOR_HDD_26][UCR_THRESH] = 65;
  dpb_sensor_threshold[DPB_SENSOR_HDD_27][UCR_THRESH] = 65;
  dpb_sensor_threshold[DPB_SENSOR_HDD_28][UCR_THRESH] = 65;
  dpb_sensor_threshold[DPB_SENSOR_HDD_29][UCR_THRESH] = 65;
  dpb_sensor_threshold[DPB_SENSOR_HDD_30][UCR_THRESH] = 65;
  dpb_sensor_threshold[DPB_SENSOR_HDD_31][UCR_THRESH] = 65;
  dpb_sensor_threshold[DPB_SENSOR_HDD_32][UCR_THRESH] = 65;
  dpb_sensor_threshold[DPB_SENSOR_HDD_33][UCR_THRESH] = 65;
  dpb_sensor_threshold[DPB_SENSOR_HDD_34][UCR_THRESH] = 65;
  dpb_sensor_threshold[DPB_SENSOR_HDD_35][UCR_THRESH] = 65;

  // SCC
  scc_sensor_threshold[SCC_SENSOR_P3V3_SENSE][UCR_THRESH] = 3.64;
  scc_sensor_threshold[SCC_SENSOR_P3V3_SENSE][LCR_THRESH] = 2.97;
  scc_sensor_threshold[SCC_SENSOR_P1V8_E_SENSE][UCR_THRESH] = 1.98;
  scc_sensor_threshold[SCC_SENSOR_P1V8_E_SENSE][LCR_THRESH] = 1.62;
  scc_sensor_threshold[SCC_SENSOR_P1V5_E_SENSE][UCR_THRESH] = 1.65;
  scc_sensor_threshold[SCC_SENSOR_P1V5_E_SENSE][LCR_THRESH] = 1.35;
  scc_sensor_threshold[SCC_SENSOR_P0V9_SENSE][UCR_THRESH] = 0.99;
  scc_sensor_threshold[SCC_SENSOR_P0V9_SENSE][LCR_THRESH] = 0.81;
  scc_sensor_threshold[SCC_SENSOR_P1V8_C_SENSE][UCR_THRESH] = 1.98;
  scc_sensor_threshold[SCC_SENSOR_P1V8_C_SENSE][LCR_THRESH] = 1.62;
  scc_sensor_threshold[SCC_SENSOR_P1V5_C_SENSE][UCR_THRESH] = 1.65;
  scc_sensor_threshold[SCC_SENSOR_P1V5_C_SENSE][LCR_THRESH] = 1.35;
  scc_sensor_threshold[SCC_SENSOR_P0V975_SENSE][UCR_THRESH] = 1.07;
  scc_sensor_threshold[SCC_SENSOR_P0V975_SENSE][LCR_THRESH] = 0.88;
  scc_sensor_threshold[SCC_SENSOR_EXPANDER_TEMP][UCR_THRESH] = 100;
  scc_sensor_threshold[SCC_SENSOR_IOC_TEMP][UCR_THRESH] = 100;
  scc_sensor_threshold[SCC_SENSOR_HSC_POWER][UCR_THRESH] = 78.12;
  scc_sensor_threshold[SCC_SENSOR_HSC_CURR][UCR_THRESH] = 6.51;
  scc_sensor_threshold[SCC_SENSOR_HSC_VOLT][UCR_THRESH] = 12.84;
  scc_sensor_threshold[SCC_SENSOR_HSC_VOLT][LCR_THRESH] = 11.16;

  // MEZZ
  nic_sensor_threshold[MEZZ_SENSOR_TEMP][UCR_THRESH] = 100;

  init_done = true;
}

size_t bic_sensor_cnt = sizeof(bic_sensor_list)/sizeof(uint8_t);

size_t bic_discrete_cnt = sizeof(bic_discrete_list)/sizeof(uint8_t);

size_t iom_sensor_cnt_type5 = sizeof(iom_sensor_list_type5 )/sizeof(uint8_t);
size_t iom_sensor_cnt_type7 = sizeof(iom_sensor_list_type7 )/sizeof(uint8_t);


size_t dpb_sensor_cnt = sizeof(dpb_sensor_list)/sizeof(uint8_t);

size_t dpb_discrete_cnt = sizeof(dpb_discrete_list)/sizeof(uint8_t);

size_t scc_sensor_cnt = sizeof(scc_sensor_list)/sizeof(uint8_t);

size_t nic_sensor_cnt = sizeof(nic_sensor_list)/sizeof(uint8_t);

enum {
  FAN0 = 0,
  FAN1,
};

enum {
  ADC_PIN0 = 0,
  ADC_PIN1,
  ADC_PIN2,
  ADC_PIN3,
  ADC_PIN4,
  ADC_PIN5,
  ADC_PIN6,
  ADC_PIN7,
  ADC_PIN8,
  ADC_PIN9,
  ADC_PIN10,
  ADC_PIN11,
};

static sensor_info_t g_sinfo[MAX_NUM_FRUS][MAX_SENSOR_NUM] = {0};

static int
read_device(const char *device, int *value) {
  FILE *fp;
  int rc;

  fp = fopen(device, "r");
  if (!fp) {
    int err = errno;

#ifdef DEBUG
    syslog(LOG_INFO, "failed to open device %s", device);
#endif
    return err;
  }

  rc = fscanf(fp, "%d", value);
  fclose(fp);

  if (rc != 1) {
#ifdef DEBUG
    syslog(LOG_INFO, "failed to read device %s", device);
#endif
    return ENOENT;
  } else {
    return 0;
  }
}

static int
read_device_float(const char *device, float *value) {
  FILE *fp;
  int rc;
  char tmp[10];

  fp = fopen(device, "r");
  if (!fp) {
    int err = errno;
#ifdef DEBUG
    syslog(LOG_INFO, "failed to open device %s", device);
#endif
    return err;
  }

  rc = fscanf(fp, "%s", tmp);
  fclose(fp);

  if (rc != 1) {
#ifdef DEBUG
    syslog(LOG_INFO, "failed to read device %s", device);
#endif
    return ENOENT;
  }

  *value = atof(tmp);

  return 0;
}

static int
read_temp_attr(const char *device, const char *attr, float *value) {
  char full_name[LARGEST_DEVICE_NAME + 1];
  char dir_name[LARGEST_DEVICE_NAME + 1];
  int tmp;
  FILE *fp;
  int size;

  // Get current working directory
  snprintf(
      full_name, LARGEST_DEVICE_NAME, "cd %s;pwd", device);

  fp = popen(full_name, "r");
  fgets(dir_name, LARGEST_DEVICE_NAME, fp);
  pclose(fp);

  // Remove the newline character at the end
  size = strlen(dir_name);
  dir_name[size-1] = '\0';

  snprintf(
      full_name, LARGEST_DEVICE_NAME, "%s/%s", dir_name, attr);

  if (read_device(full_name, &tmp)) {
    return -1;
  }

  *value = ((float)tmp)/UNIT_DIV;

  return 0;
}

static int
read_temp(const char *device, float *value) {
  return read_temp_attr(device, "temp1_input", value);
}


static int
read_M2_temp(int device, float *value) {
  int ret;
  int dev;
  int32_t res;
  char bus[32];
  int gpio_value;
  char full_name[LARGEST_DEVICE_NAME + 1];

  if (device == 1) {
    snprintf(full_name, LARGEST_DEVICE_NAME, M2_1_GPIO_PRESENT);
    if (read_device(full_name, &gpio_value)) {
    return -1;
    }
    if (gpio_value) {
    return -1;
    }
    sprintf(bus, "%s", I2C_M2_1);
  }
  else if (device == 2) {
    snprintf(full_name, LARGEST_DEVICE_NAME, M2_2_GPIO_PRESENT);
    if (read_device(full_name, &gpio_value)) {
    return -1;
    }
    if (gpio_value) {
    return -1;
    }

    sprintf(bus, "%s", I2C_M2_2);
  }
  else {
    syslog(LOG_DEBUG, "%s(): unknown dev", __func__);
    return -1;
  }

  dev = open(bus, O_RDWR);
  if (dev < 0) {
    syslog(LOG_DEBUG, "%s(): open() failed", __func__);
    return -1;
  }

  /* Assign the i2c device address */
  ret = ioctl(dev, I2C_SLAVE, I2C_NVME_INTF_ADDR);
  if (ret < 0) {
    syslog(LOG_DEBUG, "%s(): ioctl() assigning i2c addr failed", __func__);
    close(dev);
    return -1;
  }

  res = i2c_smbus_read_byte_data(dev, M2_TEMP_REG);
  if (res < 0) {
    syslog(LOG_DEBUG, "%s(): i2c_smbus_read_byte_data failed", __func__);
    close(dev);
    return -1;
  }
  *value = (float) res;

  close(dev);

  return 0;
}

static int
read_fan_value(const int fan, const char *device, float *value) {
  char device_name[LARGEST_DEVICE_NAME];
  char full_name[LARGEST_DEVICE_NAME];

  snprintf(device_name, LARGEST_DEVICE_NAME, device, fan);
  snprintf(full_name, LARGEST_DEVICE_NAME, "%s/%s", TACH_DIR, device_name);
  return read_device_float(full_name, value);
}

static int
read_adc_value(const int pin, const char *device, float *value) {
  char device_name[LARGEST_DEVICE_NAME];
  char full_name[LARGEST_DEVICE_NAME];

  snprintf(device_name, LARGEST_DEVICE_NAME, device, pin);
  snprintf(full_name, LARGEST_DEVICE_NAME, "%s/%s", ADC_DIR, device_name);
  return read_device_float(full_name, value);
}

static int
read_hsc_value(const char* att, const char *device, int r_sense, float *value) {
  char full_name[LARGEST_DEVICE_NAME];
  char dir_name[LARGEST_DEVICE_NAME + 1];
  int tmp;
  FILE *fp;
  int size;

  // Get current working directory
  snprintf(
      full_name, LARGEST_DEVICE_NAME, "cd %s;pwd", device);

  fp = popen(full_name, "r");
  fgets(dir_name, LARGEST_DEVICE_NAME, fp);
  pclose(fp);

  // Remove the newline character at the end
  size = strlen(dir_name);
  dir_name[size-1] = '\0';

  snprintf(full_name, LARGEST_DEVICE_NAME, "%s/%s", dir_name, att);
  if(read_device(full_name, &tmp)) {
    return -1;
  }

  if ((strcmp(att, HSC_OUT_CURR) == 0) || (strcmp(att, HSC_IN_POWER) == 0)) {
    *value = ((float) tmp)/r_sense/UNIT_DIV;
  }
  else {
    *value = ((float) tmp)/UNIT_DIV;
  }

  return 0;
}

static int
read_nic_temp(const char *device, float *value) {
  char full_name[LARGEST_DEVICE_NAME + 1];
  char dir_name[LARGEST_DEVICE_NAME + 1];
  int tmp;
  FILE *fp;
  int size;

  // Get current working directory
  snprintf(
      full_name, LARGEST_DEVICE_NAME, "cd %s;pwd", device);

  fp = popen(full_name, "r");
  fgets(dir_name, LARGEST_DEVICE_NAME, fp);
  pclose(fp);

  // Remove the newline character at the end
  size = strlen(dir_name);
  dir_name[size-1] = '\0';

  snprintf(
      full_name, LARGEST_DEVICE_NAME, "%s/temp2_input", dir_name);

  if (read_device(full_name, &tmp)) {
    return -1;
  }

  *value = ((float)tmp)/UNIT_DIV;

  return 0;
}

static int
bic_read_sensor_wrapper(uint8_t fru, uint8_t sensor_num, bool discrete,
    void *value) {

  int ret;
  sdr_full_t *sdr;
  ipmi_sensor_reading_t sensor;

  ret = bic_read_sensor(fru, sensor_num, &sensor);
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
  uint8_t m_lsb, m_msb, m;
  uint8_t b_lsb, b_msb, b;
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

  //printf("m:%d, x:%d, b:%d, b_exp:%d, r_exp:%d\n", m, x, b, b_exp, r_exp);

  * (float *) value = ((m * x) + (b * pow(10, b_exp))) * (pow(10, r_exp));

  if ((sensor_num == BIC_SENSOR_SOC_THERM_MARGIN) && (* (float *) value > 0)) {
   * (float *) value -= (float) THERMAL_CONSTANT;
  }

  return 0;
}

int
fbttn_sensor_sdr_path(uint8_t fru, char *path) {

char fru_name[16] = {0};

switch(fru) {
  case FRU_SLOT1:
    sprintf(fru_name, "%s", "slot1");
    break;
  case FRU_IOM:
    sprintf(fru_name, "%s", "iom");
    break;
  case FRU_DPB:
    sprintf(fru_name, "%s", "dpb");
    break;
  case FRU_SCC:
    sprintf(fru_name, "%s", "scc");
    break;
  case FRU_NIC:
    sprintf(fru_name, "%s", "nic");
    break;
  default:
#ifdef DEBUG
    syslog(LOG_WARNING, "fbttn_sensor_sdr_path: Wrong Slot ID\n");
#endif
    return -1;
}

sprintf(path, FBTTN_SDR_PATH, fru_name);

if (access(path, F_OK) == -1) {
  return -1;
}

return 0;
}

/* Populates all sensor_info_t struct using the path to SDR dump */
int
sdr_init(char *path, sensor_info_t *sinfo) {
int fd;
uint8_t buf[MAX_SDR_LEN] = {0};
uint8_t bytes_rd = 0;
uint8_t snr_num = 0;
sdr_full_t *sdr;

while (access(path, F_OK) == -1) {
  sleep(5);
}

fd = open(path, O_RDONLY);
if (fd < 0) {
  syslog(LOG_ERR, "sdr_init: open failed for %s\n", path);
  return -1;
}

while ((bytes_rd = read(fd, buf, sizeof(sdr_full_t))) > 0) {
  if (bytes_rd != sizeof(sdr_full_t)) {
    syslog(LOG_ERR, "sdr_init: read returns %d bytes\n", bytes_rd);
    return -1;
  }

  sdr = (sdr_full_t *) buf;
  snr_num = sdr->sensor_num;
  sinfo[snr_num].valid = true;
  memcpy(&sinfo[snr_num].sdr, sdr, sizeof(sdr_full_t));
}

return 0;
}

int
fbttn_sensor_sdr_init(uint8_t fru, sensor_info_t *sinfo) {
  int fd;
  uint8_t buf[MAX_SDR_LEN] = {0};
  uint8_t bytes_rd = 0;
  uint8_t sn = 0;
  char path[64] = {0};

  switch(fru) {
    case FRU_SLOT1:
      if (fbttn_sensor_sdr_path(fru, path) < 0) {
#ifdef DEBUG
        syslog(LOG_WARNING, "fbttn_sensor_sdr_init: get_fru_sdr_path failed\n");
#endif
        return ERR_NOT_READY;
      }

      if (sdr_init(path, sinfo) < 0) {
#ifdef DEBUG
        syslog(LOG_ERR, "fbttn_sensor_sdr_init: sdr_init failed for FRU %d", fru);
#endif
      }
      break;

    case FRU_IOM:
    case FRU_DPB:
    case FRU_SCC:
    case FRU_NIC:
      return -1;
      break;
  }

  return 0;
}

static int
fbttn_sdr_init(uint8_t fru) {

  static bool init_done[MAX_NUM_FRUS] = {false};

  if (!init_done[fru - 1]) {

    sensor_info_t *sinfo = g_sinfo[fru-1];

    if (fbttn_sensor_sdr_init(fru, sinfo) < 0)
      return ERR_NOT_READY;

    init_done[fru - 1] = true;
  }

  return 0;
}

bool
is_server_prsnt(uint8_t fru) {
  int val;
  char path[64] = {0};

  sprintf(path, PCA9555_VAL, SLOT_INS);

  if (read_device(path, &val)) {
    return false;
  }

  if (val == 0) {   // present
    return true;
  } else {
    return false;
  }
}

bool
is_scc_prsnt(uint8_t scc_port) {
  int val;
  char path[64] = {0};

  sprintf(path, PCA9555_VAL, scc_port);

  if (read_device(path, &val)) {
    return false;
  }

  if (val == 0) {   // present
    return true;
  } else {
    return false;
  }
}

bool
is_fan_prsnt(uint8_t fan_port) {
  int val;
  char path[64] = {0};

  sprintf(path, PCA9555_VAL, fan_port);

  if (read_device(path, &val)) {
    return false;
  }

  if (val == 0) {   // present
    return true;
  } else {
    return false;
  }
}

/* Get the units for the sensor */
int
fbttn_sensor_units(uint8_t fru, uint8_t sensor_num, char *units) {
  uint8_t op, modifier;
  sensor_info_t *sinfo;

  switch(fru) {
    case FRU_SLOT1:
      if (is_server_prsnt(fru) && (fbttn_sdr_init(fru) != 0)) {
        return -1;
      }
      sprintf(units, "");
      break;

    case FRU_IOM:
      switch(sensor_num) {
    		case ML_SENSOR_HSC_PWR:
              sprintf(units, "Watts");
              break;
            case ML_SENSOR_HSC_VOLT:
              sprintf(units, "Volts");
              break;
            case ML_SENSOR_HSC_CURR:
              sprintf(units, "Amps");
              break;
            case IOM_SENSOR_MEZZ_TEMP:
              sprintf(units, "C");
              break;
            case IOM_SENSOR_HSC_POWER:
              sprintf(units, "Watts");
              break;
            case IOM_SENSOR_HSC_VOLT:
              sprintf(units, "Volts");
              break;
            case IOM_SENSOR_HSC_CURR:
              sprintf(units, "Amps");
              break;
            case IOM_SENSOR_ADC_12V:
              sprintf(units, "Volts");
              break;
            case IOM_SENSOR_ADC_P5V_STBY:
              sprintf(units, "Volts");
              break;
            case IOM_SENSOR_ADC_P3V3_STBY:
              sprintf(units, "Volts");
              break;
            case IOM_SENSOR_ADC_P1V8_STBY:
              sprintf(units, "Volts");
              break;
            case IOM_SENSOR_ADC_P2V5_STBY:
              sprintf(units, "Volts");
              break;
            case IOM_SENSOR_ADC_P1V2_STBY:
              sprintf(units, "Volts");
              break;
            case IOM_SENSOR_ADC_P1V15_STBY:
              sprintf(units, "Volts");
              break;
            case IOM_SENSOR_ADC_P3V3:
              sprintf(units, "Volts");
              break;
            case IOM_SENSOR_ADC_P1V8:
              sprintf(units, "Volts");
              break;
            case IOM_SENSOR_ADC_P1V5:
              sprintf(units, "Volts");
              break;
            case IOM_SENSOR_ADC_P0V975:
              sprintf(units, "Volts");
              break;
            case IOM_SENSOR_ADC_P3V3_M2:
              sprintf(units, "Volts");
              break;
            case IOM_SENSOR_M2_AMBIENT_TEMP1:
              sprintf(units, "C");
              break;
            case IOM_SENSOR_M2_AMBIENT_TEMP2:
              sprintf(units, "C");
              break;
            case IOM_SENSOR_M2_SMART_TEMP1:
              sprintf(units, "C");
              break;
            case IOM_SENSOR_M2_SMART_TEMP2:
              sprintf(units, "C");
              break;
            case IOM_IOC_TEMP:
              sprintf(units, "C");
              break;
      }
      break;

    case FRU_DPB:

      if(sensor_num >= DPB_SENSOR_HDD_0 &&
		 sensor_num <= DPB_SENSOR_HDD_35) {
         sprintf(units, "C");
        break;
      }

      switch(sensor_num) {
        case DPB_SENSOR_FAN0_FRONT:
          sprintf(units, "RPM");
          break;
        case DPB_SENSOR_FAN1_FRONT:
          sprintf(units, "RPM");
          break;
        case DPB_SENSOR_FAN2_FRONT:
          sprintf(units, "RPM");
          break;
        case DPB_SENSOR_FAN3_FRONT:
          sprintf(units, "RPM");
          break;
        case DPB_SENSOR_FAN0_REAR:
          sprintf(units, "RPM");
          break;
        case DPB_SENSOR_FAN1_REAR:
          sprintf(units, "RPM");
          break;
        case DPB_SENSOR_FAN2_REAR:
          sprintf(units, "RPM");
          break;
        case DPB_SENSOR_FAN3_REAR:
          sprintf(units, "RPM");
          break;
        case DPB_SENSOR_HSC_POWER:
          sprintf(units, "Watts");
          break;
        case DPB_SENSOR_HSC_VOLT:
          sprintf(units, "Volts");
          break;
        case DPB_SENSOR_HSC_CURR:
          sprintf(units, "Amps");
          break;
        case P3V3_SENSE:
          sprintf(units, "Volts");
          break;
        case P5V_A_1_SENSE:
          sprintf(units, "Volts");
          break;
        case P5V_A_2_SENSE:
          sprintf(units, "Volts");
          break;
        case P5V_A_3_SENSE:
          sprintf(units, "Volts");
          break;
        case P5V_A_4_SENSE:
          sprintf(units, "Volts");
          break;
        case DPB_SENSOR_12V_POWER_CLIP:
          sprintf(units, "Watts");
          break;
        case DPB_SENSOR_P12V_CLIP:
          sprintf(units, "Volts");
          break;
        case DPB_SENSOR_12V_CURR_CLIP:
          sprintf(units, "Amps");
          break;
        case DPB_SENSOR_A_TEMP:
          sprintf(units, "C");
          break;
        case DPB_SENSOR_B_TEMP:
          sprintf(units, "C");
          break;
      }
      break;

    case FRU_SCC:
      switch(sensor_num) {
        case SCC_SENSOR_EXPANDER_TEMP:
          sprintf(units, "C");
          break;
        case SCC_SENSOR_IOC_TEMP:
          sprintf(units, "C");
          break;
        case SCC_SENSOR_HSC_POWER:
          sprintf(units, "Watts");
          break;
        case SCC_SENSOR_HSC_CURR:
          sprintf(units, "Amps");
          break;
        case SCC_SENSOR_HSC_VOLT:
          sprintf(units, "Volts");
          break;
        case SCC_SENSOR_P3V3_SENSE:
          sprintf(units, "Volts");
          break;
        case SCC_SENSOR_P1V8_E_SENSE:
          sprintf(units, "Volts");
          break;
        case SCC_SENSOR_P1V5_E_SENSE:
          sprintf(units, "Volts");
          break;
        case SCC_SENSOR_P0V9_SENSE:
          sprintf(units, "Volts");
          break;
        case SCC_SENSOR_P1V8_C_SENSE:
          sprintf(units, "Volts");
          break;
        case SCC_SENSOR_P1V5_C_SENSE:
          sprintf(units, "Volts");
          break;
        case SCC_SENSOR_P0V975_SENSE:
          sprintf(units, "Volts");
          break;
      }
      break;

    case FRU_NIC:
      switch(sensor_num) {
        case MEZZ_SENSOR_TEMP:
          sprintf(units, "C");
          break;
      }
      break;
  }
  return 0;
}

int
fbttn_sensor_threshold(uint8_t fru, uint8_t sensor_num, uint8_t thresh, float *value) {

  sensor_thresh_array_init();

  switch(fru) {
    case FRU_SLOT1:
      break;
    case FRU_IOM:
      *value = iom_sensor_threshold[sensor_num][thresh];
      break;
    case FRU_DPB:
      *value = dpb_sensor_threshold[sensor_num][thresh];
      break;
    case FRU_SCC:
      *value = scc_sensor_threshold[sensor_num][thresh];
      break;
    case FRU_NIC:
      *value = nic_sensor_threshold[sensor_num][thresh];
      break;
  }
  return 0;
}

/* Get the name for the sensor */
int
fbttn_sensor_name(uint8_t fru, uint8_t sensor_num, char *name) {

  switch(fru) {
    case FRU_SLOT1:
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
          sprintf(name, "");
          break;
      }
      break;

    case FRU_IOM:
      switch(sensor_num) {
		  case ML_SENSOR_HSC_VOLT:
          sprintf(name, "ML_SENSOR_HSC_VOLT");
          break;
          case ML_SENSOR_HSC_CURR:
          sprintf(name, "ML_SENSOR_HSC_CURR");
          break;
          case ML_SENSOR_HSC_PWR:
          sprintf(name, "ML_SENSOR_HSC_PWR");
          break;
        case IOM_SENSOR_MEZZ_TEMP:
          sprintf(name, "IOM_NIC_Ambient_Temp");
          break;
        case IOM_SENSOR_HSC_POWER:
          sprintf(name, "IOM_HSC_POWER");
          break;
        case IOM_SENSOR_HSC_VOLT:
          sprintf(name, "IOM_HSC_VOLT");
          break;
        case IOM_SENSOR_HSC_CURR:
          sprintf(name, "IOM_HSC_CURR");
          break;
        case IOM_SENSOR_ADC_12V:
          sprintf(name, "ADC_12V");
          break;
        case IOM_SENSOR_ADC_P5V_STBY:
          sprintf(name, "ADC_P5V_STBY");
          break;
        case IOM_SENSOR_ADC_P3V3_STBY:
          sprintf(name, "ADC_P3V3_STBY");
          break;
        case IOM_SENSOR_ADC_P1V8_STBY:
          sprintf(name, "ADC_P1V8_STBY");
          break;
        case IOM_SENSOR_ADC_P2V5_STBY:
          sprintf(name, "ADC_P2V5_STBY");
          break;
        case IOM_SENSOR_ADC_P1V2_STBY:
          sprintf(name, "ADC_P1V2_STBY");
          break;
        case IOM_SENSOR_ADC_P1V15_STBY:
          sprintf(name, "ADC_P1V15_STBY");
          break;
        case IOM_SENSOR_ADC_P3V3:
          sprintf(name, "ADC_P3V3");
          break;
        case IOM_SENSOR_ADC_P1V8:
          sprintf(name, "ADC_P1V8");
          break;
        case IOM_SENSOR_ADC_P1V5:
          sprintf(name, "ADC_P1V5");
          break;
        case IOM_SENSOR_ADC_P0V975:
          sprintf(name, "ADC_P0V975");
          break;
        case IOM_SENSOR_ADC_P3V3_M2:
          sprintf(name, "ADC_P3V3_M2");
          break;
        case IOM_SENSOR_M2_AMBIENT_TEMP1:
          sprintf(name, "M.2_Ambient_Temp_1");
          break;
        case IOM_SENSOR_M2_AMBIENT_TEMP2:
          sprintf(name, "M.2_Ambient_Temp_2");
          break;
        case IOM_SENSOR_M2_SMART_TEMP1:
          sprintf(name, "M.2_SMART_Temp_1");
          break;
        case IOM_SENSOR_M2_SMART_TEMP2:
          sprintf(name, "M.2_SMART_Temp_2");
          break;
        case IOM_IOC_TEMP:
          sprintf(name, "IOM_IOC_TEMP");
          break;
      }
      break;

    case FRU_DPB:

      if(sensor_num >= DPB_SENSOR_HDD_0 &&
		 sensor_num <= DPB_SENSOR_HDD_35) {
        sprintf(name, "HDD_%d_TEMP", sensor_num - DPB_SENSOR_HDD_0);
        break;
      }

      switch(sensor_num) {
        case DPB_SENSOR_FAN0_FRONT:
          sprintf(name, "FAN0_FRONT");
          break;
        case DPB_SENSOR_FAN1_FRONT:
          sprintf(name, "FAN1_FRONT");
          break;
        case DPB_SENSOR_FAN2_FRONT:
          sprintf(name, "FAN2_FRONT");
          break;
        case DPB_SENSOR_FAN3_FRONT:
          sprintf(name, "FAN3_FRONT");
          break;
        case DPB_SENSOR_FAN0_REAR:
          sprintf(name, "FAN0_REAR");
          break;
        case DPB_SENSOR_FAN1_REAR:
          sprintf(name, "FAN1_REAR");
          break;
        case DPB_SENSOR_FAN2_REAR:
          sprintf(name, "FAN2_REAR");
          break;
        case DPB_SENSOR_FAN3_REAR:
          sprintf(name, "FAN3_REAR");
          break;
        case DPB_SENSOR_HSC_POWER:
          sprintf(name, "DPB_HSC_POWER");
          break;
        case DPB_SENSOR_HSC_VOLT:
          sprintf(name, "DPB_HSC_VOLT");
          break;
        case DPB_SENSOR_HSC_CURR:
          sprintf(name, "DPB_HSC_CURR");
          break;
        case P3V3_SENSE:
          sprintf(name, "P3V3_SENSE");
          break;
        case P5V_A_1_SENSE:
          sprintf(name, "P5V_A_1_SENSE");
          break;
        case P5V_A_2_SENSE:
          sprintf(name, "P5V_A_2_SENSE");
          break;
        case P5V_A_3_SENSE:
          sprintf(name, "P5V_A_3_SENSE");
          break;
        case P5V_A_4_SENSE:
          sprintf(name, "P5V_A_4_SENSE");
          break;
        case DPB_SENSOR_12V_POWER_CLIP:
          sprintf(name, "DPB_12V_POWER_CLIP");
          break;
        case DPB_SENSOR_P12V_CLIP:
          sprintf(name, "DPB_P12V_CLIP");
          break;
        case DPB_SENSOR_12V_CURR_CLIP:
          sprintf(name, "DPB_12V_CURR_CLIP");
          break;
        case DPB_SENSOR_A_TEMP:
          sprintf(name, "DPB_Inlet_Temp_1");
          break;
        case DPB_SENSOR_B_TEMP:
          sprintf(name, "DPB_Inlet_Temp_2");
          break;
      }
      break;

    case FRU_SCC:
      switch(sensor_num) {
        case SCC_SENSOR_EXPANDER_TEMP:
          sprintf(name, "EXPANDER_TEMP");
          break;
        case SCC_SENSOR_IOC_TEMP:
          sprintf(name, "IOC_TEMP");
          break;
        case SCC_SENSOR_HSC_POWER:
          sprintf(name, "SCC_HSC_POWER");
          break;
        case SCC_SENSOR_HSC_CURR:
          sprintf(name, "SCC_HSC_CURR");
          break;
        case SCC_SENSOR_HSC_VOLT:
          sprintf(name, "SCC_HSC_VOLT");
          break;
        case SCC_SENSOR_P3V3_SENSE:
          sprintf(name, "P3V3_SENSE");
          break;
        case SCC_SENSOR_P1V8_E_SENSE:
          sprintf(name, "P1V8_E_SENSE");
          break;
        case SCC_SENSOR_P1V5_E_SENSE:
          sprintf(name, "P1V5_E_SENSE");
          break;
        case SCC_SENSOR_P0V9_SENSE:
          sprintf(name, "P0V9_SENSE");
          break;
        case SCC_SENSOR_P1V8_C_SENSE:
          sprintf(name, "P1V8_C_SENSE");
          break;
        case SCC_SENSOR_P1V5_C_SENSE:
          sprintf(name, "P1V5_C_SENSE");
          break;
        case SCC_SENSOR_P0V975_SENSE:
          sprintf(name, "P0V975_SENSE");
          break;
      }
      break;

    case FRU_NIC:
      switch(sensor_num) {
        case MEZZ_SENSOR_TEMP:
          sprintf(name, "MEZZ_SENSOR_TEMP");
          break;
      }
      break;
  }
  return 0;
}


int
fbttn_sensor_read(uint8_t fru, uint8_t sensor_num, void *value) {

  float volt;
  float curr;
  int ret;
  bool discrete;
  int i;

  switch (fru) {
    case FRU_SLOT1:

      if (!(is_server_prsnt(fru))) {
        return -1;
      }

      ret = fbttn_sdr_init(fru);
      if (ret < 0) {
        return ret;
      }

      discrete = false;

      i = 0;
      while (i < bic_discrete_cnt) {
        if (sensor_num == bic_discrete_list[i++]) {
          discrete = true;
          break;
        }
      }

      return bic_read_sensor_wrapper(fru, sensor_num, discrete, value);

    case FRU_IOM:
      switch(sensor_num) {
		//ML HSC
		case ML_SENSOR_HSC_PWR:
          return read_hsc_value(HSC_IN_POWER, HSC_DEVICE_ML, ml_hsc_r_sense, (float *) value);
        case ML_SENSOR_HSC_VOLT:
          return read_hsc_value(HSC_IN_VOLT, HSC_DEVICE_ML, ml_hsc_r_sense, (float *) value);
        case ML_SENSOR_HSC_CURR:
          return read_hsc_value(HSC_OUT_CURR, HSC_DEVICE_ML, ml_hsc_r_sense, (float *) value);
        // Behind NIC Card Temp
        case IOM_SENSOR_MEZZ_TEMP:
          return read_temp(IOM_MEZZ_TEMP_DEVICE, (float *) value);
        // Hot Swap Controller
        case IOM_SENSOR_HSC_POWER:
          return read_hsc_value(HSC_IN_POWER, HSC_DEVICE_IOM, iom_hsc_r_sense, (float *) value);
        case IOM_SENSOR_HSC_VOLT:
          return read_hsc_value(HSC_IN_VOLT, HSC_DEVICE_IOM, iom_hsc_r_sense, (float *) value);
        case IOM_SENSOR_HSC_CURR:
          return read_hsc_value(HSC_OUT_CURR, HSC_DEVICE_IOM, iom_hsc_r_sense, (float *) value);
        // Various Voltages
        case IOM_SENSOR_ADC_12V:
          return read_adc_value(ADC_PIN0, ADC_VALUE, (float *) value);
        case IOM_SENSOR_ADC_P5V_STBY:
          return read_adc_value(ADC_PIN1, ADC_VALUE, (float *) value);
        case IOM_SENSOR_ADC_P3V3_STBY:
          return read_adc_value(ADC_PIN2, ADC_VALUE, (float *) value);
        case IOM_SENSOR_ADC_P1V8_STBY:
          return read_adc_value(ADC_PIN3, ADC_VALUE, (float *) value);
        case IOM_SENSOR_ADC_P2V5_STBY:
          return read_adc_value(ADC_PIN4, ADC_VALUE, (float *) value);
        case IOM_SENSOR_ADC_P1V2_STBY:
          return read_adc_value(ADC_PIN5, ADC_VALUE, (float *) value);
        case IOM_SENSOR_ADC_P1V15_STBY:
          return read_adc_value(ADC_PIN6, ADC_VALUE, (float *) value);
        case IOM_SENSOR_ADC_P3V3:
          return read_adc_value(ADC_PIN7, ADC_VALUE, (float *) value);
        case IOM_SENSOR_ADC_P1V8:
          return read_adc_value(ADC_PIN8, ADC_VALUE, (float *) value);
        case IOM_SENSOR_ADC_P1V5:
          return read_adc_value(ADC_PIN9, ADC_VALUE, (float *) value);
        case IOM_SENSOR_ADC_P0V975:
          return read_adc_value(ADC_PIN10, ADC_VALUE, (float *) value);
        case IOM_SENSOR_ADC_P3V3_M2:
          return read_adc_value(ADC_PIN11, ADC_VALUE, (float *) value);
        // M.2 Ambient Temp
        case IOM_SENSOR_M2_AMBIENT_TEMP1:
          return read_temp(IOM_M2_AMBIENT_TEMP1_DEVICE, (float *) value);
        case IOM_SENSOR_M2_AMBIENT_TEMP2:
          return read_temp(IOM_M2_AMBIENT_TEMP2_DEVICE, (float *) value);
        // M.2 SMART Temp
        case IOM_SENSOR_M2_SMART_TEMP1:
          return read_M2_temp(IOM_M2_1_TEMP_DEVICE, (float *) value);
        case IOM_SENSOR_M2_SMART_TEMP2:
          return read_M2_temp(IOM_M2_2_TEMP_DEVICE, (float *) value);
        case IOM_IOC_TEMP:
          return mctp_get_iom_ioc_temp((float *) value);
      }
      break;

    // TODO: Add read functions for every DPB sensors.
    case FRU_DPB:
      switch(sensor_num) {
        case DPB_SENSOR_FAN0_FRONT:
        case DPB_SENSOR_FAN1_FRONT:
        case DPB_SENSOR_FAN2_FRONT:
        case DPB_SENSOR_FAN3_FRONT:
        case DPB_SENSOR_FAN0_REAR:
        case DPB_SENSOR_FAN1_REAR:
        case DPB_SENSOR_FAN2_REAR:
        case DPB_SENSOR_FAN3_REAR:
          *(float *) value = 0;
        return 0;
        case DPB_SENSOR_HSC_POWER:
            return read_hsc_value(HSC_IN_POWER, HSC_DEVICE_DPB, EXP_R_SENSE, (float *) value);
        case DPB_SENSOR_HSC_VOLT:
            return read_hsc_value(HSC_IN_VOLT, HSC_DEVICE_DPB, EXP_R_SENSE, (float *) value);
        case DPB_SENSOR_HSC_CURR:
            return read_hsc_value(HSC_OUT_CURR, HSC_DEVICE_DPB, EXP_R_SENSE, (float *) value);

      }
      break;

    case FRU_SCC:
      switch(sensor_num) {
        case SCC_SENSOR_EXPANDER_TEMP:
        case SCC_SENSOR_IOC_TEMP:
        case SCC_SENSOR_HSC_POWER:
        case SCC_SENSOR_HSC_CURR:
        case SCC_SENSOR_HSC_VOLT:
        case SCC_SENSOR_P3V3_SENSE:
        case SCC_SENSOR_P1V8_E_SENSE:
        case SCC_SENSOR_P1V5_E_SENSE:
        case SCC_SENSOR_P0V9_SENSE:
        case SCC_SENSOR_P1V8_C_SENSE:
        case SCC_SENSOR_P1V5_C_SENSE:
        case SCC_SENSOR_P0V975_SENSE:
          *(float *) value = 0;
          return 0;
      }
      break;

    case FRU_NIC:
      switch(sensor_num) {
      // Mezz Temp
        case MEZZ_SENSOR_TEMP:
          return read_nic_temp(MEZZ_TEMP_DEVICE, (float*) value);
      }
      break;
  }
}

