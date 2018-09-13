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
#include <time.h>
#include <unistd.h>
#include <syslog.h>
#include <openbmc/obmc-sensor.h>
#include <openbmc/obmc-i2c.h>
#include "lightning_sensor.h"

#define LARGEST_DEVICE_NAME 128

#define I2C_DEV_PEB       "/dev/i2c-4"
#define I2C_DEV_PDPB      "/dev/i2c-6"
#define I2C_DEV_FCB       "/dev/i2c-5"

#define I2C_BUS_PEB_DIR "/sys/class/i2c-adapter/i2c-4/"
#define I2C_BUS_PDPB_DIR "/sys/class/i2c-adapter/i2c-6/"
#define I2C_BUS_FCB_DIR "/sys/class/i2c-adapter/i2c-5/"

#define I2C_ADDR_PEB_HSC 0x11

#define I2C_ADDR_PDPB_ADC  0x48

#define I2C_ADDR_FCB_HSC  0x22
#define I2C_ADDR_NCT7904  0x2d

#define ADC_DIR "/sys/devices/platform/ast_adc.0"
#define ADC_VALUE "adc%d_value"

#define UNIT_DIV 1000

#define TPM75_TEMP_RESOLUTION 0.0625

#define ADS1015_DEFAULT_CONFIG 0xe383

#define ALL_BYTES 0xFF
#define LAST_REC_ID 0xFFFF

#define PEB_TMP75_U136_DEVICE I2C_BUS_PEB_DIR "4-004d"
#define PEB_TMP75_U134_DEVICE I2C_BUS_PEB_DIR "4-004a"

#define PDPB_TMP75_U47_DEVICE I2C_BUS_PDPB_DIR "6-0049/hwmon/hwmon2"
#define PDPB_TMP75_U48_DEVICE I2C_BUS_PDPB_DIR "6-004a/hwmon/hwmon3"
#define PDPB_TMP75_U49_DEVICE I2C_BUS_PDPB_DIR "6-004b/hwmon/hwmon4"
#define PDPB_TMP75_U51_DEVICE I2C_BUS_PDPB_DIR "6-004c/hwmon/hwmon5"

#define FAN_REGISTER 0x80

#define VOLT_SENSOR_READING_OFFSET 0.2

enum temp_sensor_type {
  LOCAL_SENSOR = 0,
  REMOTE_SENSOR,
};
enum tmp75_peb_sensors {
  PEB_TMP75_U136 = 0x4d,
  PEB_TMP75_U134 = 0x4a,
  PEB_TMP421_U15 = 0x4c,
  PEB_MAX6654_U4 = 0x18,
};

enum tmp75_pdpb_sensors {
  PDPB_TMP75_U47 = 0x49,
  PDPB_TMP75_U48 = 0x4a,
  PDPB_TMP75_U49 = 0x4b,
  PDPB_TMP75_U51 = 0x4c,
};

enum nct7904_registers {
  NCT7904_TEMP_CH1 = 0x42,
  NCT7904_TEMP_CH2 = 0x46,
  NCT7904_VSEN6 = 0x4A,
  NCT7904_VSEN7 = 0x4C,
  NCT7904_VSEN9 = 0x50,
  NCT7904_3VDD = 0x5C,
  NCT7904_MONITOR_FLAG = 0xBA,
  NCT7904_BANK_SEL = 0xFF,
};

enum hsc_controllers {
  HSC_ADM1276 = 0,
  HSC_ADM1278,
};

enum hsc_commands {
  HSC_IN_VOLT = 0x88,
  HSC_OUT_CURR = 0x8c,
  HSC_IN_POWER = 0x97,
  HSC_TEMP = 0x8D,
};

enum ads1015_registers {
  ADS1015_CONVERSION = 0,
  ADS1015_CONFIG,
};

enum ads1015_channels {
  ADS1015_CHANNEL0 = 0,
  ADS1015_CHANNEL1,
  ADS1015_CHANNEL2,
  ADS1015_CHANNEL3,
  ADS1015_CHANNEL4,
  ADS1015_CHANNEL5,
  ADS1015_CHANNEL6,
  ADS1015_CHANNEL7,
};

enum adc_pins {
  ADC_PIN0 = 0,
  ADC_PIN1,
  ADC_PIN2,
  ADC_PIN3,
  ADC_PIN4,
  ADC_PIN5,
  ADC_PIN6,
  ADC_PIN7,
};

// NVMe temp status description code
enum nvme_temp_status {
  NVME_NO_TEMP_DATA = 0x80,
  NVME_SENSOR_FAILURE = 0x81,
};

// List of PEB sensors to be monitored (PMC)
const uint8_t peb_sensor_pmc_list[] = {
  PEB_SENSOR_ADC_P12V,
  PEB_SENSOR_ADC_P5V,
  PEB_SENSOR_ADC_P3V3_STBY,
  PEB_SENSOR_ADC_P1V8_STBY,
  PEB_SENSOR_ADC_P1V53,
  PEB_SENSOR_ADC_P0V9,
  PEB_SENSOR_ADC_P0V9_E,
  PEB_SENSOR_ADC_P1V26,
  PEB_SENSOR_HSC_IN_VOLT,
  PEB_SENSOR_HSC_OUT_CURR,
  PEB_SENSOR_HSC_IN_POWER,
  PEB_SENSOR_PCIE_SW_TEMP,
  PEB_SENSOR_SYS_INLET_TEMP,
};


// List of PEB sensors to be monitored (PLX)
const uint8_t peb_sensor_plx_list[] = {
  PEB_SENSOR_ADC_P12V,
  PEB_SENSOR_ADC_P5V,
  PEB_SENSOR_ADC_P3V3_STBY,
  PEB_SENSOR_ADC_P1V8_STBY,
  PEB_SENSOR_ADC_P1V53,
  PEB_SENSOR_ADC_P0V9,
  PEB_SENSOR_ADC_P0V9_E,
  PEB_SENSOR_ADC_P1V26,
  PEB_SENSOR_HSC_IN_VOLT,
  PEB_SENSOR_HSC_OUT_CURR,
  PEB_SENSOR_HSC_IN_POWER,
  PEB_SENSOR_SYS_INLET_TEMP,
};


// List of U.2 SKU PDPB sensors to be monitored
const uint8_t pdpb_u2_sensor_list[] = {
  PDPB_SENSOR_P12V,
  PDPB_SENSOR_P3V3,
  PDPB_SENSOR_LEFT_REAR_TEMP,
  PDPB_SENSOR_LEFT_FRONT_TEMP,
  PDPB_SENSOR_RIGHT_REAR_TEMP,
  PDPB_SENSOR_RIGHT_FRONT_TEMP,
  PDPB_SENSOR_FLASH_TEMP_0,
  PDPB_SENSOR_FLASH_TEMP_1,
  PDPB_SENSOR_FLASH_TEMP_2,
  PDPB_SENSOR_FLASH_TEMP_3,
  PDPB_SENSOR_FLASH_TEMP_4,
  PDPB_SENSOR_FLASH_TEMP_5,
  PDPB_SENSOR_FLASH_TEMP_6,
  PDPB_SENSOR_FLASH_TEMP_7,
  PDPB_SENSOR_FLASH_TEMP_8,
  PDPB_SENSOR_FLASH_TEMP_9,
  PDPB_SENSOR_FLASH_TEMP_10,
  PDPB_SENSOR_FLASH_TEMP_11,
  PDPB_SENSOR_FLASH_TEMP_12,
  PDPB_SENSOR_FLASH_TEMP_13,
  PDPB_SENSOR_FLASH_TEMP_14,
};


// List of M.2 SKU PDPB sensors to be monitored
const uint8_t pdpb_m2_sensor_list[] = {
  PDPB_SENSOR_P12V,
  PDPB_SENSOR_P3V3,
  PDPB_SENSOR_LEFT_REAR_TEMP,
  PDPB_SENSOR_LEFT_FRONT_TEMP,
  PDPB_SENSOR_RIGHT_REAR_TEMP,
  PDPB_SENSOR_RIGHT_FRONT_TEMP,
  PDPB_SENSOR_FLASH_TEMP_0,
  PDPB_SENSOR_FLASH_TEMP_1,
  PDPB_SENSOR_FLASH_TEMP_2,
  PDPB_SENSOR_FLASH_TEMP_3,
  PDPB_SENSOR_FLASH_TEMP_4,
  PDPB_SENSOR_FLASH_TEMP_5,
  PDPB_SENSOR_FLASH_TEMP_6,
  PDPB_SENSOR_FLASH_TEMP_7,
  PDPB_SENSOR_FLASH_TEMP_8,
  PDPB_SENSOR_FLASH_TEMP_9,
  PDPB_SENSOR_FLASH_TEMP_10,
  PDPB_SENSOR_FLASH_TEMP_11,
  PDPB_SENSOR_FLASH_TEMP_12,
  PDPB_SENSOR_FLASH_TEMP_13,
  PDPB_SENSOR_FLASH_TEMP_14,
  PDPB_SENSOR_AMB_TEMP_0,
  PDPB_SENSOR_AMB_TEMP_1,
  PDPB_SENSOR_AMB_TEMP_2,
  PDPB_SENSOR_AMB_TEMP_3,
  PDPB_SENSOR_AMB_TEMP_4,
  PDPB_SENSOR_AMB_TEMP_5,
  PDPB_SENSOR_AMB_TEMP_6,
  PDPB_SENSOR_AMB_TEMP_7,
  PDPB_SENSOR_AMB_TEMP_8,
  PDPB_SENSOR_AMB_TEMP_9,
  PDPB_SENSOR_AMB_TEMP_10,
  PDPB_SENSOR_AMB_TEMP_11,
  PDPB_SENSOR_AMB_TEMP_12,
  PDPB_SENSOR_AMB_TEMP_13,
  PDPB_SENSOR_AMB_TEMP_14,
};

// List of NIC sensors to be monitored
const uint8_t fcb_sensor_list[] = {
  FCB_SENSOR_P12V_AUX,
  FCB_SENSOR_P12VL,
  FCB_SENSOR_P12VU,
  FCB_SENSOR_P3V3,
  FCB_SENSOR_HSC_IN_VOLT,
  FCB_SENSOR_HSC_OUT_CURR,
  FCB_SENSOR_HSC_IN_POWER,
  FCB_SENSOR_BJT_TEMP_1,
  FCB_SENSOR_BJT_TEMP_2,
  FCB_SENSOR_FAN1_FRONT_SPEED,
  FCB_SENSOR_FAN1_REAR_SPEED,
  FCB_SENSOR_FAN2_FRONT_SPEED,
  FCB_SENSOR_FAN2_REAR_SPEED,
  FCB_SENSOR_FAN3_FRONT_SPEED,
  FCB_SENSOR_FAN3_REAR_SPEED,
  FCB_SENSOR_FAN4_FRONT_SPEED,
  FCB_SENSOR_FAN4_REAR_SPEED,
  FCB_SENSOR_FAN5_FRONT_SPEED,
  FCB_SENSOR_FAN5_REAR_SPEED,
  FCB_SENSOR_FAN6_FRONT_SPEED,
  FCB_SENSOR_FAN6_REAR_SPEED,
  FCB_SENSOR_AIRFLOW,
};

float peb_sensor_threshold[MAX_SENSOR_NUM][MAX_SENSOR_THRESHOLD + 1] = {0};
float pdpb_sensor_threshold[MAX_SENSOR_NUM][MAX_SENSOR_THRESHOLD + 1] = {0};
float fcb_sensor_threshold[MAX_SENSOR_NUM][MAX_SENSOR_THRESHOLD + 1] = {0};

/* flags to record which ssd is abnormal */
uint8_t ssd_no_temp[NUM_SSD] = {0};
uint8_t ssd_sensor_fail[NUM_SSD] = {0};

int
lightning_sensor_get_airflow(float *airflow_cfm)
{
  uint8_t ssd_sku = 0;
  float rpm_avg = 0, rpm_sum = 0, value;
  int fan=0;
  int ret,rc;

  if (airflow_cfm == NULL){
    syslog(LOG_ERR, "%s() Invalid memory address", __func__);
    return -1;
  }

  // Calculate average RPM
  for (fan = 0; fan < pal_tach_count; fan++) {
    rc = sensor_cache_read(FRU_FCB, fcb_sensor_fan1_front_speed + fan, &value);
    if(rc == -1) {
      continue;
    }
    rpm_sum+=value;
  }

  rpm_avg = rpm_sum/pal_tach_count;

  ret = lightning_ssd_sku(&ssd_sku);
  if (ret < 0) {
    syslog(LOG_DEBUG, "%s() get SSD SKU failed", __func__);
    return -1;
  }

  if (ssd_sku == U2_SKU) {
     *airflow_cfm = (((-2) * (rpm_avg*rpm_avg) / 10000000) + (0.0208*(rpm_avg)) - 7.8821);
  }
  else if (ssd_sku == M2_SKU) {
     *airflow_cfm = (((-2) * (rpm_avg*rpm_avg) / 10000000) + (0.0211*(rpm_avg)) - 10.585);
  }
  else {
    syslog(LOG_DEBUG, "%s(): Cannot find corresponding SSD SKU", __func__);
    return -1;
  }

  if(*airflow_cfm < 0) {
    *airflow_cfm = 0;
  }

  return 0;
}

static void
assign_sensor_threshold(uint8_t fru, uint8_t snr_num, float ucr, float unc,
    float unr, float lcr, float lnc, float lnr, float pos_hyst, float neg_hyst) {

  switch(fru) {
    case FRU_PEB:
      peb_sensor_threshold[snr_num][UCR_THRESH] = ucr;
      peb_sensor_threshold[snr_num][UNC_THRESH] = unc;
      peb_sensor_threshold[snr_num][UNR_THRESH] = unr;
      peb_sensor_threshold[snr_num][LCR_THRESH] = lcr;
      peb_sensor_threshold[snr_num][LNC_THRESH] = lnc;
      peb_sensor_threshold[snr_num][LNR_THRESH] = lnr;
      peb_sensor_threshold[snr_num][POS_HYST] = pos_hyst;
      peb_sensor_threshold[snr_num][NEG_HYST] = neg_hyst;
      break;
    case FRU_PDPB:
      pdpb_sensor_threshold[snr_num][UCR_THRESH] = ucr;
      pdpb_sensor_threshold[snr_num][UNC_THRESH] = unc;
      pdpb_sensor_threshold[snr_num][UNR_THRESH] = unr;
      pdpb_sensor_threshold[snr_num][LCR_THRESH] = lcr;
      pdpb_sensor_threshold[snr_num][LNC_THRESH] = lnc;
      pdpb_sensor_threshold[snr_num][LNR_THRESH] = lnr;
      pdpb_sensor_threshold[snr_num][POS_HYST] = pos_hyst;
      pdpb_sensor_threshold[snr_num][NEG_HYST] = neg_hyst;
      break;
    case FRU_FCB:
      fcb_sensor_threshold[snr_num][UCR_THRESH] = ucr;
      fcb_sensor_threshold[snr_num][UNC_THRESH] = unc;
      fcb_sensor_threshold[snr_num][UNR_THRESH] = unr;
      fcb_sensor_threshold[snr_num][LCR_THRESH] = lcr;
      fcb_sensor_threshold[snr_num][LNC_THRESH] = lnc;
      fcb_sensor_threshold[snr_num][LNR_THRESH] = lnr;
      fcb_sensor_threshold[snr_num][POS_HYST] = pos_hyst;
      fcb_sensor_threshold[snr_num][NEG_HYST] = neg_hyst;
      break;
  }
}

static void
sensor_thresh_array_init() {

  static bool init_done = false;
  uint8_t ssd_sku = 0;
  uint8_t ssd_vendor = 0;
  int ret;

  if (init_done)
    return;

  int i;

  // PEB HSC SENSORS
  assign_sensor_threshold(FRU_PEB, PEB_SENSOR_HSC_IN_VOLT,
      13.63, 0, 0, 11.25, 0, 0, 0.63, 0.63);
  assign_sensor_threshold(FRU_PEB, PEB_SENSOR_HSC_OUT_CURR,
      8, 0, 0, 0, 0, 0, 0, 0);
  assign_sensor_threshold(FRU_PEB, PEB_SENSOR_HSC_IN_POWER,
      96, 0, 0, 0, 0, 0, 0, 0);

  // PEB TEMP SENSORS
  assign_sensor_threshold(FRU_PEB, PEB_SENSOR_PCIE_SW_TEMP,
      100, 93, 0, 0, 0, 0, 0, 2);
  assign_sensor_threshold(FRU_PEB, PEB_SENSOR_SYS_INLET_TEMP,
      50, 45, 0, 0, 0, 0, 0, 2);

  // PEB VOLT SENSORS
  assign_sensor_threshold(FRU_PEB, PEB_SENSOR_ADC_P12V,
      13.63, 13.38, 0, 11.25, 11.50, 0, 0.63, 0.63);
  assign_sensor_threshold(FRU_PEB, PEB_SENSOR_ADC_P5V,
      5.5, 5.4, 0, 4.5, 4.6, 0, 0.25, 0.25);
  assign_sensor_threshold(FRU_PEB, PEB_SENSOR_ADC_P3V3_STBY,
      3.63, 3.56, 0, 3.02, 3.09, 0, 0.17, 0.17);
  assign_sensor_threshold(FRU_PEB, PEB_SENSOR_ADC_P1V8_STBY,
      1.98, 1.94, 0, 1.62, 1.66, 0, 0.09, 0.09);
  assign_sensor_threshold(FRU_PEB, PEB_SENSOR_ADC_P1V53,
      1.68, 1.65, 0, 1.4, 1.43, 0, 0.08, 0.08);
  assign_sensor_threshold(FRU_PEB, PEB_SENSOR_ADC_P0V9,
      0.99, 0.97, 0, 0.81, 0.83, 0, 0.05, 0.05);
  assign_sensor_threshold(FRU_PEB, PEB_SENSOR_ADC_P0V9_E,
      0.99, 0.97, 0, 0.81, 0.83, 0, 0.05, 0.05);
  assign_sensor_threshold(FRU_PEB, PEB_SENSOR_ADC_P1V26,
      1.39, 1.36, 0, 1.13, 1.16, 0, 0.06, 0.06);

  // PDPB VOLT SENSORS
  assign_sensor_threshold(FRU_PDPB, PDPB_SENSOR_P12V,
      13.63, 13.38, 0, 11.25, 11.5, 0, 0.63, 0.63);
  assign_sensor_threshold(FRU_PDPB, PDPB_SENSOR_P3V3,
      3.63, 3.56, 0, 2.98, 3.05, 0, 0.17, 0.17);

  // PDPB TEMP SENSORS
  assign_sensor_threshold(FRU_PDPB, PDPB_SENSOR_LEFT_REAR_TEMP,
      55, 50, 0, 0, 0, 0, 0, 2);
  assign_sensor_threshold(FRU_PDPB, PDPB_SENSOR_LEFT_FRONT_TEMP,
      55, 50, 0, 0, 0, 0, 0, 2);
  assign_sensor_threshold(FRU_PDPB, PDPB_SENSOR_RIGHT_REAR_TEMP,
      55, 50, 0, 0, 0, 0, 0, 2);
  assign_sensor_threshold(FRU_PDPB, PDPB_SENSOR_RIGHT_FRONT_TEMP,
      55, 50, 0, 0, 0, 0, 0, 2);

  // PDPB SSD and AMBIENT TEMP SENSORS
  ret = lightning_ssd_sku(&ssd_sku);
  if (ret < 0) {
    syslog(LOG_DEBUG, "%s() get SSD SKU failed", __func__);
  }
  ret = lightning_ssd_vendor(&ssd_vendor);
  if (ret < 0) {
    syslog(LOG_DEBUG, "%s() get SSD vendor failed", __func__);
  }

  for (i = 0; i < lightning_flash_cnt; i++) {
    if(ssd_sku == U2_SKU) {
      // Intel U.2 threshold
      if (ssd_vendor == INTEL)
        assign_sensor_threshold(FRU_PDPB, PDPB_SENSOR_FLASH_TEMP_0 + i,
            75, 73, 0, 0, 0, 0, 0, 2);
      // Samsung U.2 threshold
      else if(ssd_vendor == SAMSUNG)
        assign_sensor_threshold(FRU_PDPB, PDPB_SENSOR_FLASH_TEMP_0 + i,
            75, 73, 0, 0, 0, 0, 0, 2);
      // Default threshold
      else
        assign_sensor_threshold(FRU_PDPB, PDPB_SENSOR_FLASH_TEMP_0 + i,
            75, 73, 0, 0, 0, 0, 0, 2);

    } else if (ssd_sku == M2_SKU) {
      // Seagate M.2 threshold
      if(ssd_vendor == SEAGATE)
        assign_sensor_threshold(FRU_PDPB, PDPB_SENSOR_FLASH_TEMP_0 + i,
            75, 73, 0, 0, 0, 0, 0, 2);
      // Samsung M.2 threshold
      else if (ssd_vendor == SAMSUNG)
        assign_sensor_threshold(FRU_PDPB, PDPB_SENSOR_FLASH_TEMP_0 + i,
            75, 73, 0, 0, 0, 0, 0, 2);
      // Default threshold
      else
        assign_sensor_threshold(FRU_PDPB, PDPB_SENSOR_FLASH_TEMP_0 + i,
            75, 73, 0, 0, 0, 0, 0, 2);

      // AMBIENT SENSORS
      assign_sensor_threshold(FRU_PDPB, PDPB_SENSOR_AMB_TEMP_0 + i,
            0, 0, 0, 0, 0, 0, 0, 2);
    } else {
      // Default threshold
      assign_sensor_threshold(FRU_PDPB, PDPB_SENSOR_FLASH_TEMP_0 + i,
            75, 73, 0, 0, 0, 0, 0, 2);
    }

  }

  // FCB Volt Sensors
  assign_sensor_threshold(FRU_FCB, FCB_SENSOR_P12V_AUX,
      13.63, 13.38, 0, 11.25, 11.5, 0, 0.63, 0.63);
  assign_sensor_threshold(FRU_FCB, FCB_SENSOR_P12VL,
      13.63, 13.38, 0, 11.25, 11.5, 0, 0.63, 0.63);
  assign_sensor_threshold(FRU_FCB, FCB_SENSOR_P12VU,
      13.63, 13.38, 0, 11.25, 11.5, 0, 0.63, 0.63);
  assign_sensor_threshold(FRU_FCB, FCB_SENSOR_P3V3,
      3.63, 3.56, 0, 3.1, 3.16, 0, 0.17, 0.17);

  // FCB HSC Sensors
  assign_sensor_threshold(FRU_FCB, FCB_SENSOR_HSC_IN_VOLT,
      13.63, 0, 0, 11.25, 0, 0, 0.63, 0.63);
  assign_sensor_threshold(FRU_FCB, FCB_SENSOR_HSC_OUT_CURR,
      60, 0, 0, 0, 0, 0, 0, 0);
  assign_sensor_threshold(FRU_FCB, FCB_SENSOR_HSC_IN_POWER,
      750, 0, 0, 0, 0, 0, 0, 0);

  // FCB Temp Sensor
  assign_sensor_threshold(FRU_FCB, FCB_SENSOR_BJT_TEMP_1,
      60, 55, 0, 0, 0, 0, 0, 2);
  assign_sensor_threshold(FRU_FCB, FCB_SENSOR_BJT_TEMP_2,
      60, 55, 0, 0, 0, 0, 0, 2);

  //FCB Airflow
  assign_sensor_threshold(FRU_FCB, FCB_SENSOR_AIRFLOW,
      0, 0, 0, 0, 0, 0, 0, 0);

  // FCB Fan Speed
  assign_sensor_threshold(FRU_FCB, FCB_SENSOR_FAN1_FRONT_SPEED,
      0, 0, 0, 400, 0, 0, 50, 0);
  assign_sensor_threshold(FRU_FCB, FCB_SENSOR_FAN1_REAR_SPEED,
      0, 0, 0, 400, 0, 0, 50, 0);
  assign_sensor_threshold(FRU_FCB, FCB_SENSOR_FAN2_FRONT_SPEED,
      0, 0, 0, 400, 0, 0, 50, 0);
  assign_sensor_threshold(FRU_FCB, FCB_SENSOR_FAN2_REAR_SPEED,
      0, 0, 0, 400, 0, 0, 50, 0);
  assign_sensor_threshold(FRU_FCB, FCB_SENSOR_FAN3_FRONT_SPEED,
      0, 0, 0, 400, 0, 0, 50, 0);
  assign_sensor_threshold(FRU_FCB, FCB_SENSOR_FAN3_REAR_SPEED,
      0, 0, 0, 400, 0, 0, 50, 0);
  assign_sensor_threshold(FRU_FCB, FCB_SENSOR_FAN4_FRONT_SPEED,
      0, 0, 0, 400, 0, 0, 50, 0);
  assign_sensor_threshold(FRU_FCB, FCB_SENSOR_FAN4_REAR_SPEED,
      0, 0, 0, 400, 0, 0, 50, 0);
  assign_sensor_threshold(FRU_FCB, FCB_SENSOR_FAN5_FRONT_SPEED,
      0, 0, 0, 400, 0, 0, 50, 0);
  assign_sensor_threshold(FRU_FCB, FCB_SENSOR_FAN5_REAR_SPEED,
      0, 0, 0, 400, 0, 0, 50, 0);
  assign_sensor_threshold(FRU_FCB, FCB_SENSOR_FAN6_FRONT_SPEED,
      0, 0, 0, 400, 0, 0, 50, 0);
  assign_sensor_threshold(FRU_FCB, FCB_SENSOR_FAN6_REAR_SPEED,
      0, 0, 0, 400, 0, 0, 50, 0);

  init_done = true;
}

size_t peb_sensor_pmc_cnt = sizeof(peb_sensor_pmc_list)/sizeof(uint8_t);

size_t peb_sensor_plx_cnt = sizeof(peb_sensor_plx_list)/sizeof(uint8_t);

size_t pdpb_u2_sensor_cnt = sizeof(pdpb_u2_sensor_list)/sizeof(uint8_t);

size_t pdpb_m2_sensor_cnt = sizeof(pdpb_m2_sensor_list)/sizeof(uint8_t);

size_t fcb_sensor_cnt = sizeof(fcb_sensor_list)/sizeof(uint8_t);

// Helper function for msleep
void
msleep(int msec) {
  struct timespec req;

  req.tv_sec = 0;
  req.tv_nsec = msec * 1000 * 1000;

  while(nanosleep(&req, &req) == -1 && errno == EINTR) {
    continue;
  }
}

int32_t signextend32(uint32_t val, int idx) {
    uint8_t shift = 31 - idx;
      return (int32_t)(val << shift) >> shift;
}

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

int
nvme_special_case_handling(uint8_t flash_num, float *value) {
    if ( (uint8_t)(*value) == NVME_NO_TEMP_DATA ) {
      if (!ssd_no_temp[flash_num])
        syslog(LOG_WARNING, "%s(): SSD %d no temperature data or temperature data is more the 5 seconds old", __func__, flash_num);
      ssd_no_temp[flash_num] = 1;
      return -1;
    } else if ( (uint8_t)(*value) == NVME_SENSOR_FAILURE ) {
      if (!ssd_sensor_fail[flash_num])
        syslog(LOG_CRIT, "%s(): SSD %d temperature sensor failure", __func__, flash_num);
      ssd_sensor_fail[flash_num] = 1;
      return -1;
    } else {
      if (ssd_no_temp[flash_num]) {
        syslog(LOG_WARNING, "%s(): SSD %d can get temperature data now, %f C", __func__, flash_num, *value);
        ssd_no_temp[flash_num] = 0;
      } else if (ssd_sensor_fail[flash_num]) {
        syslog(LOG_CRIT, "%s(): SSD %d temperature sensor is back from failure mode", __func__, flash_num);
        ssd_sensor_fail[flash_num] = 0;
      }
      return 0;
    }
}

static int
read_flash_temp(uint8_t flash_num, float *value) {

  uint8_t sku;
  int ret;

  ret = lightning_ssd_sku(&sku);

  if (ret < 0) {
#ifdef DEBUG
    syslog(LOG_DEBUG, "%s(): lightning_ssd_sku failed", __func__);
#endif
    return -1;
  }

  if (sku == U2_SKU) {
    ret = lightning_u2_flash_temp_read(lightning_flash_list[flash_num], value);
    if (ret != 0)
      return ret;

    return nvme_special_case_handling(flash_num, value);
  } else if (sku == M2_SKU) {
    ret = lightning_m2_flash_temp_read(lightning_flash_list[flash_num], value);
    if (ret != 0)
      return ret;

    return nvme_special_case_handling(flash_num, value);
  } else {
#ifdef DEBUG
    syslog(LOG_DEBUG, "%s(): unknown ssd sku", __func__);
#endif
    return -1;
  }
}

static int
read_m2_amb_temp(uint8_t flash_num, float *value) {

  return lightning_m2_amb_temp_read(lightning_flash_list[flash_num], value);
}

static int
read_adc_value(const int pin, const char *device, float *value) {
  char device_name[LARGEST_DEVICE_NAME];
  char full_name[LARGEST_DEVICE_NAME];

  snprintf(device_name, LARGEST_DEVICE_NAME, device, pin);
  snprintf(full_name, LARGEST_DEVICE_NAME, "%s/%s", ADC_DIR, device_name);
  return read_device_float(full_name, value);
}

/* Function to read the temp from the sysfs */
static int
read_tmp75_temp_value(const char *device, float *value) {
  char full_name[LARGEST_DEVICE_NAME + 1];
  int tmp;

  snprintf(full_name, LARGEST_DEVICE_NAME, "%s/temp1_input", device);
  if (read_device(full_name, &tmp)) {
    return -1;
  }

  *value = ((float)tmp)/UNIT_DIV;

  return 0;
}

/* Function to read the temp directly from the i2c dev */
static int
read_temp_value(char *device, uint8_t addr, uint8_t type, float *value) {

  int dev,fan;
  int ret,rc;
  int32_t res;
  float rpm_avg = 0, rpm_sum = 0;
  float rpm_val;

  dev = open(device, O_RDWR);
  if (dev < 0) {
#ifdef DEBUG
    syslog(LOG_ERR, "read_temp_value: open() failed");
#endif
    return -1;
  }

  /* Assign the i2c device address */
  ret = ioctl(dev, I2C_SLAVE, addr);
  if (ret < 0) {
#ifdef DEBUG
    syslog(LOG_ERR, "read_temp_value: ioctl() assigning i2c addr failed");
#endif
    close(dev);
    return -1;
  }

  /* Read the Temperature Register result based on whether it is internal or external sensor */
  res = i2c_smbus_read_word_data(dev, type);
  if (res < 0) {
#ifdef DEBUG
    syslog(LOG_ERR, "read_temp_value: i2c_smbus_read_word_data failed");
#endif
    close(dev);
    return -1;
  }

  close(dev);
  /* Result is read as MSB byte first and LSB byte second.
   * Result is 12bit with res[11:4]  == MSB[7:0] and res[3:0] = LSB */
  res = ((res & 0x0FF) << 4) | ((res & 0xF000) >> 12);

   /* Resolution is 0.0625 deg C/bit */
  if (res <= 0x7FF) {
    /* Temperature is positive  */
    *value = (float) res * TPM75_TEMP_RESOLUTION;
  } else if (res >= 0xC90) {
    /* Temperature is negative */
    *value = (float) (0xFFF - res + 1) * (-1) * TPM75_TEMP_RESOLUTION;
  } else {
    /* Out of range [128C to -55C] */
#ifdef DEBUG
    syslog(LOG_WARNING, "read_temp_value: invalid res value = 0x%X", res);
#endif
    return -1;
  }

  // Correction Factor for Inlet temperature sensor
  if(addr == PEB_TMP421_U15){
    // Calculate average RPM
      for(fan = 0; fan < 12; fan++) {
          rc = sensor_cache_read(FRU_FCB, FCB_SENSOR_FAN1_FRONT_SPEED + fan, &rpm_val);
          if(rc == -1) {
            continue;
           }
       rpm_sum+=rpm_val;
      }
      rpm_avg = rpm_sum/12;

      if(rpm_avg <= 1700){
        *value = (*value) - 4;
      }else if(rpm_avg <= 2200){
        *value = (*value) - 3;
      }else if(rpm_avg <= 2700){
        *value = (*value) - 2;
      }else if(rpm_avg <= 3300){
        *value = (*value) - 1;
      }
  }

  return 0;
}


static int
read_hsc_value(uint8_t reg, char *device, uint8_t addr, uint8_t cntlr, float *value) {

  int dev;
  int ret;
  int32_t res;

  dev = open(device, O_RDWR);
  if (dev < 0) {
#ifdef DEBUG
    syslog(LOG_ERR, "read_hsc_value: open() failed");
#endif
    return -1;
  }

  /* Assign the i2c device address */
  ret = ioctl(dev, I2C_SLAVE, addr);
  if (ret < 0) {
#ifdef DEBUG
    syslog(LOG_ERR, "read_hsc_value: ioctl() assigning i2c addr failed");
#endif
    close(dev);
    return -1;
  }

  /* Read the er result based on whether it is internal or external sensor */
  res = i2c_smbus_read_word_data(dev, reg);
  if (res < 0) {
#ifdef DEBUG
    syslog(LOG_ERR, "read_hsc_value: i2c_smbus_read_word_data failed");
#endif
    close(dev);
    return -1;
  }

  close(dev);


  switch(reg) {
    case HSC_IN_VOLT:
      res &= 0xFFF; // This Parameter uses bits [11:0]
      if (cntlr == HSC_ADM1276 || cntlr == HSC_ADM1278) {
        *value = (1.0/19599) * ((res * 100) - 0);
      }
      break;
    case HSC_OUT_CURR:
      res &= 0xFFF; // This Parameter uses bits [11:0]
      if (cntlr == HSC_ADM1276) {
        *value = ((res * 10) - 20475) / (807 * 0.1667);
      } else if (cntlr == HSC_ADM1278) {
        *value = (1.0/1600) * ((res * 10) - 20475);
      }
      break;
    case HSC_IN_POWER:
      res &= 0xFFFF; // This Parameter uses bits [15:0]
      if (cntlr == HSC_ADM1276) {
        *value = ((res * 100) - 0) / (6043 * 0.1667);
      } else if (cntlr == HSC_ADM1278) {
        *value = (1.0/12246) * ((res * 100) - 0);
      }
        *value *= 0.99; // This is to compensate the controller reading offset value
      break;
    case HSC_TEMP:
      res &= 0xFFF; // This Parameter uses bits [11:0]
      if (cntlr == HSC_ADM1278) {
        *value = (1.0/42) * ((res * 10) - 31880);
      }
        *value *= 0.97; // This is to compensate the controller reading offset value
      break;
    default:
#ifdef DEBUG
      syslog(LOG_ERR, "read_hsc_value: wrong param");
#endif
      return -1;
  }

  return 0;
}

static int
read_nct7904_value(uint8_t reg, char *device, uint8_t addr, float *value) {

  int dev;
  int ret;
  int res_h;
  int res_l;
  int bank;
  int retry;
  uint8_t peer_tray_exist;
  uint8_t location;
  uint8_t monitor_flag;
  uint16_t res;
  float multipler = 0;

  dev = open(device, O_RDWR);
  if (dev < 0) {
#ifdef DEBUG
    syslog(LOG_ERR, "read_nct7904_value: open() failed");
#endif
    return -1;
  }

  /* Assign the i2c device address */
  ret = ioctl(dev, I2C_SLAVE, addr);
  if (ret < 0) {
#ifdef DEBUG
    syslog(LOG_ERR, "read_nct7904_value: ioctl() assigning i2c addr failed");
#endif
    return -1;
  }

  /* Read the Bank Register and set it to 0 */
  bank = i2c_smbus_read_byte_data(dev, NCT7904_BANK_SEL);
  if (bank != 0x0) {
#ifdef DEBUG
    syslog(LOG_INFO, "read_nct7904_value: Bank Register set to %d", bank);
#endif
    if (i2c_smbus_write_byte_data(dev, NCT7904_BANK_SEL, 0) < 0) {
#ifdef DEBUG
      syslog(LOG_ERR, "read_nct7904_value: i2c_smbus_write_byte_data: "
          "selecting Bank 0 failed");
#endif
      close(dev);
      return -1;
    }
  }

  ret = gpio_peer_tray_detection(&peer_tray_exist);
  if (ret < 0) {
#ifdef DEBUG
    syslog(LOG_ERR, "%s: pal_peer_tray_detection() failed.", __func__);
#endif
    return -1;
  }

  if (peer_tray_exist) {
  /* Determine this tray located on upper or lower tray; 0:Upper, 1:Lower*/
    ret = pal_self_tray_location(&location);
    if(ret < 0) {
#ifdef DEBUG
      syslog(LOG_ERR, "read_nct7904_value: pal_self_tray_location failed");
#endif
      return -1;
    }
    retry = 0;
    while(retry < MAX_RETRY_TIMES) {
      monitor_flag = i2c_smbus_read_byte_data(dev, NCT7904_MONITOR_FLAG);

      if (UPPER_TRAY == location) {
        /* BMC can query sensor only when peer BMC's queries are done. */
        if (LOWER_TRAY_DONE == monitor_flag) {
          break;
        } else {
          retry++;
        }
      } else if (LOWER_TRAY == location) {
        if (UPPER_TRAY_DONE == monitor_flag) {
          break;
        } else {
          retry++;
        }
      }
      msleep(100);
    }
  }

  retry = 0;
  while (retry < MAX_RETRY_TIMES) {
    /* Read the MSB byte for the value */
    res_h = i2c_smbus_read_byte_data(dev, reg);

    /* Read the LSB byte for the value */
    res_l = i2c_smbus_read_byte_data(dev, reg + 1);

    /* Read failed */
    if ((res_h == -1) || (res_l == -1))
      retry++;
    else
      break;

    msleep(100);
  }

  if ( (retry >= MAX_RETRY_TIMES) && ((res_h == -1) || (res_l == -1)) ) {
#ifdef DEBUG
    syslog(LOG_DEBUG, "%s() i2c_smbus_read_byte_data failed, high byte: 0x%x, low byte: 0x%x", __func__, res_h, res_l);
#endif
    return -1;
  }


  /* Modify the monitor_flag when the last sensor query finish in this query interval */
  if (FAN_REGISTER+2 == reg) {

    if (location == UPPER_TRAY) {

      if (i2c_smbus_write_byte_data(dev, NCT7904_MONITOR_FLAG, UPPER_TRAY_DONE) < 0) {
#ifdef DEBUG
        syslog(LOG_ERR, "read_nct7904_value: i2c_smbus_write_byte_data: "
            "upper tray monitor failed");
#endif
        close(dev);
        return -1;
      }

    } else {

      if (i2c_smbus_write_byte_data(dev, NCT7904_MONITOR_FLAG, LOWER_TRAY_DONE) < 0) {
#ifdef DEBUG
        syslog(LOG_ERR, "read_nct7904_value: i2c_smbus_write_byte_data: "
            "lower tray monitor failed");
#endif
        close(dev);
        return -1;
      }

    }
  }

  close(dev);

  /*
   * Fan speed reading is 13 bits
   * res[12:5] = res_h[7:0]
   * res[4:0]  = res_l[4:0]
   *
   * Other reading is 11 bits
   * res[10:3] = res_h[7:0]
   * res[2:0] = res_l[2:0]
   */
  if (reg >= FAN_REGISTER && reg <= FAN_REGISTER+22) {
    res = ((res_h & 0xFF) << 5) | (res_l & 0x1F);
  } else {
    res = ((res_h & 0xFF) << 3) | (res_l & 0x7);
  }

  switch(reg) {
    case NCT7904_TEMP_CH1:
      multipler = (0.125 /* NCT7904 Section 6.4 */);
      break;
    case NCT7904_TEMP_CH2:
      multipler = (0.125 /* NCT7904 Section 6.4 */);
      break;
    case NCT7904_VSEN9:
      multipler = (12 /* Voltage Divider*/ * 0.002 /* NCT7904 Section 6.4 */);
      break;
    case NCT7904_VSEN6:
      multipler = (12 /* Voltage Divider*/ * 0.002 /* NCT7904 Section 6.4 */);
      break;
    case NCT7904_VSEN7:
      multipler = (12 /* Voltage Divider*/ * 0.002 /* NCT7904 Section 6.4 */);
      break;
    case NCT7904_3VDD:
      multipler = (0.006 /* NCT7904 Section 6.4 */);
      break;
  }

  if (reg >= FAN_REGISTER && reg <= FAN_REGISTER+22) {
    /* fan speed reading */
    if (res == 0x1fff || res == 0)
      *value = 0;
    else
      *value = (float) (1350000 / res);
  } else {
    /* temp sensor reading */
    if (reg == NCT7904_TEMP_CH1 || reg == NCT7904_TEMP_CH2) {
      *value = signextend32(res, 10) * multipler;
      if (reg == NCT7904_TEMP_CH2)
        *value = *value - 2;
    /* add offset to sensor reading  */
    } else if (reg == NCT7904_VSEN6 || reg == NCT7904_VSEN7 || reg == NCT7904_VSEN9) {
      *value = (float) (res * multipler) + VOLT_SENSOR_READING_OFFSET;
    /* other sensor reading */
    } else
      *value = (float) (res * multipler);
  }


  return 0;
}

static int
read_ads1015_value(uint8_t channel, char *device, uint8_t addr, float *value) {

  int dev;
  int ret;
  int32_t config, config2;
  int32_t res;

  dev = open(device, O_RDWR);
  if (dev < 0) {
#ifdef DEBUG
    syslog(LOG_ERR, "read_ads1015_value: open() failed");
#endif
    return -1;
  }

  /* Assign the i2c device address */
  ret = ioctl(dev, I2C_SLAVE, addr);
  if (ret < 0) {
#ifdef DEBUG
    syslog(LOG_ERR, "read_ads1015_value: ioctl() assigning i2c addr failed");
#endif
    close(dev);
    return -1;
  }

  /*
   * config
   * Byte 0: OS[7]  CHANNEL[6:4] PGA[3:1] MODE [0]
   * Byte 1: Data Rate[7:5] COMPARATOR[4:0]
   */
  config = ADS1015_DEFAULT_CONFIG;
  config |= (channel & 0x7) << 4;

  /* Write the config in the CONFIG register */
  ret = i2c_smbus_write_word_data(dev, ADS1015_CONFIG, config);
  if (ret < 0) {
#ifdef DEBUG
    syslog(LOG_ERR, "read_ads1015_value: i2c_smbus_write_word_data failed");
#endif
    close(dev);
    return -1;
  }
  /* Wait for the conversion to finish */
  msleep(5);

  /* Read the CONFIG register to check if the conversion completed. */
  config = i2c_smbus_read_word_data(dev, ADS1015_CONFIG);
  if (!(config & (1 << 15))) {
    close(dev);
    return -1;
  } else if (config < 0) {
#ifdef DEBUG
    syslog(LOG_ERR, "read_ads1015_value: i2c_smbus_read_word_data failed");
#endif
    close(dev);
    return -1;
  }

  /* Read the CONVERSION result */
  res = i2c_smbus_read_word_data(dev, ADS1015_CONVERSION);
  if (res < 0) {
#ifdef DEBUG
    syslog(LOG_ERR, "read_ads1015_value: i2c_smbus_read_word_data failed");
#endif
    close(dev);
    return -1;
  }

  /* Read the CONFIG register to check if the conversion completed. */
  config2 = i2c_smbus_read_word_data(dev, ADS1015_CONFIG);
  if (config < 0) {
#ifdef DEBUG
    syslog(LOG_ERR, "read_ads1015_value: i2c_smbus_read_word_data failed");
#endif
    close(dev);
    return -1;
  }

  /*
   * If the config register value changed after conversion result read,
   * the result is invalid
   */
  if (config2 != config) {
#ifdef DEBUG
    syslog(LOG_ERR, "read_ads1015_value: config changed while conversion result read");
#endif
    close(dev);
    return -1;
  }

  close(dev);

  /* Result is read as MSB byte first and LSB byte second. */
  res = ((res & 0x0FF) << 8) | ((res & 0xFF00) >> 8);

  /*
   * Based on the config PGA, COMP MODE, DATA RATE value,
   * Voltage in Volts = res [15:4] * Reference volt / 2048
   */
  *value = (float) (res >> 4) * (4.096 / 2048);

  return 0;
}

int
lightning_sensor_sdr_init(uint8_t fru, sensor_info_t *sinfo) {
  return -1;
}

/* Get the units for the sensor */
int
lightning_sensor_units(uint8_t fru, uint8_t sensor_num, char *units) {
  switch(fru) {
    case FRU_PEB:
      switch(sensor_num) {
        case PEB_SENSOR_PCIE_SW_TEMP:
        case PEB_SENSOR_SYS_INLET_TEMP:
          sprintf(units, "C");
          break;
        case PEB_SENSOR_HSC_IN_VOLT:
          sprintf(units, "Volts");
          break;
        case PEB_SENSOR_HSC_OUT_CURR:
          sprintf(units, "Amps");
          break;
        case PEB_SENSOR_HSC_IN_POWER:
          sprintf(units, "Watts");
          break;
        case PEB_SENSOR_ADC_P12V:
        case PEB_SENSOR_ADC_P5V:
        case PEB_SENSOR_ADC_P3V3_STBY:
        case PEB_SENSOR_ADC_P1V8_STBY:
        case PEB_SENSOR_ADC_P1V53:
        case PEB_SENSOR_ADC_P0V9:
        case PEB_SENSOR_ADC_P0V9_E:
        case PEB_SENSOR_ADC_P1V26:
          sprintf(units, "Volts");
          break;
        default:
          sprintf(units, "%s", "");
          break;
      }
      break;

    case FRU_PDPB:

      if (sensor_num >= PDPB_SENSOR_FLASH_TEMP_0 &&
          sensor_num < (PDPB_SENSOR_FLASH_TEMP_0 + lightning_flash_cnt)) {
        sprintf(units, "C");
        break;
      }

      if (sensor_num >= PDPB_SENSOR_AMB_TEMP_0 &&
          sensor_num < (PDPB_SENSOR_AMB_TEMP_0 + lightning_flash_cnt)) {
        sprintf(units, "C");
        break;
      }

      switch(sensor_num) {
        case PDPB_SENSOR_LEFT_REAR_TEMP:
        case PDPB_SENSOR_LEFT_FRONT_TEMP:
        case PDPB_SENSOR_RIGHT_REAR_TEMP:
        case PDPB_SENSOR_RIGHT_FRONT_TEMP:
          sprintf(units, "C");
          break;
        case PDPB_SENSOR_P12V:
        case PDPB_SENSOR_P3V3:
          sprintf(units, "Volts");
          break;
        default:
          sprintf(units, "%s", "");
          break;
      }
      break;

    case FRU_FCB:
      switch(sensor_num) {
        case FCB_SENSOR_P12V_AUX:
        case FCB_SENSOR_P12VL:
        case FCB_SENSOR_P12VU:
        case FCB_SENSOR_P3V3:
          sprintf(units, "Volts");
          break;
        case FCB_SENSOR_HSC_IN_VOLT:
          sprintf(units, "Volts");
          break;
        case FCB_SENSOR_HSC_OUT_CURR:
          sprintf(units, "Amps");
          break;
        case FCB_SENSOR_HSC_IN_POWER:
          sprintf(units, "Watts");
          break;
        case FCB_SENSOR_BJT_TEMP_1:
        case FCB_SENSOR_BJT_TEMP_2:
          sprintf(units, "C");
          break;
        case FCB_SENSOR_FAN1_FRONT_SPEED:
        case FCB_SENSOR_FAN1_REAR_SPEED:
        case FCB_SENSOR_FAN2_FRONT_SPEED:
        case FCB_SENSOR_FAN2_REAR_SPEED:
        case FCB_SENSOR_FAN3_FRONT_SPEED:
        case FCB_SENSOR_FAN3_REAR_SPEED:
        case FCB_SENSOR_FAN4_FRONT_SPEED:
        case FCB_SENSOR_FAN4_REAR_SPEED:
        case FCB_SENSOR_FAN5_FRONT_SPEED:
        case FCB_SENSOR_FAN5_REAR_SPEED:
        case FCB_SENSOR_FAN6_FRONT_SPEED:
        case FCB_SENSOR_FAN6_REAR_SPEED:
          sprintf(units, "RPM");
          break;
        case FCB_SENSOR_AIRFLOW:
          sprintf(units, "CFM");
          break;
        default:
          sprintf(units, "%s", "");
          break;
      }
      break;
  }
  return 0;
}

int
lightning_sensor_threshold(uint8_t fru, uint8_t sensor_num, uint8_t thresh, float *value) {

  sensor_thresh_array_init();

  switch(fru) {
    case FRU_PEB:
      *value = peb_sensor_threshold[sensor_num][thresh];
      break;
    case FRU_PDPB:
      *value = pdpb_sensor_threshold[sensor_num][thresh];
      break;
    case FRU_FCB:
      *value = fcb_sensor_threshold[sensor_num][thresh];
      break;
  }
  return 0;
}

/* Get the poll interval for the sensor */
int
lightning_sensor_poll_interval(uint8_t fru, uint8_t sensor_num, uint32_t* value) {
  *value = (uint32_t)2;
  return 0;
}

/* Get the name for the sensor */
int
lightning_sensor_name(uint8_t fru, uint8_t sensor_num, char *name) {

  switch(fru) {
    case FRU_PEB:
      switch(sensor_num) {
        case PEB_SENSOR_PCIE_SW_TEMP:
          sprintf(name, "PCIE_SW_TEMP");
          break;
        case PEB_SENSOR_SYS_INLET_TEMP:
          sprintf(name, "SYS_INLET_TEMP");
          break;
        case PEB_SENSOR_HSC_IN_VOLT:
          sprintf(name, "PEB_HSC_IN_VOLT");
          break;
        case PEB_SENSOR_HSC_OUT_CURR:
          sprintf(name, "PEB_HSC_OUT_CURR");
          break;
        case PEB_SENSOR_HSC_IN_POWER:
          sprintf(name, "PEB_HSC_IN_POWER");
          break;
        case PEB_SENSOR_ADC_P12V:
          sprintf(name, "PEB_P12V");
          break;
        case PEB_SENSOR_ADC_P5V:
          sprintf(name, "PEB_P5V");
          break;
        case PEB_SENSOR_ADC_P3V3_STBY:
          sprintf(name, "PEB_P3V3");
          break;
        case PEB_SENSOR_ADC_P1V8_STBY:
          sprintf(name, "PEB_P1V8");
          break;
        case PEB_SENSOR_ADC_P1V53:
          sprintf(name, "PEB_P1V53");
          break;
        case PEB_SENSOR_ADC_P0V9:
          sprintf(name, "PEB_P0V9");
          break;
        case PEB_SENSOR_ADC_P0V9_E:
          sprintf(name, "PEB_P0V9_E");
          break;
        case PEB_SENSOR_ADC_P1V26:
          sprintf(name, "PEB_P1V26");
          break;
        default:
          sprintf(name, "%s", "");
          break;
      }
      break;

    case FRU_PDPB:

      if (sensor_num >= PDPB_SENSOR_FLASH_TEMP_0 &&
          sensor_num < (PDPB_SENSOR_FLASH_TEMP_0 + lightning_flash_cnt)) {
        sprintf(name, "SSD_%d_TEMP", sensor_num - PDPB_SENSOR_FLASH_TEMP_0);
        break;
      }

      if (sensor_num >= PDPB_SENSOR_AMB_TEMP_0 &&
          sensor_num < (PDPB_SENSOR_AMB_TEMP_0 + lightning_flash_cnt)) {
        sprintf(name, "M.2_Amb_TEMP_%d", sensor_num - PDPB_SENSOR_AMB_TEMP_0);
        break;
      }

      switch(sensor_num) {
        case PDPB_SENSOR_LEFT_REAR_TEMP:
          sprintf(name, "LEFT_REAR_TEMP");
          break;
        case PDPB_SENSOR_LEFT_FRONT_TEMP:
          sprintf(name, "LEFT_FRONT_TEMP");
          break;
        case PDPB_SENSOR_RIGHT_REAR_TEMP:
          sprintf(name, "RIGHT_REAR_TEMP");
          break;
        case PDPB_SENSOR_RIGHT_FRONT_TEMP:
          sprintf(name, "RIGHT_FRONT_TEMP");
          break;
        case PDPB_SENSOR_P12V:
          sprintf(name, "PDPB_P12V");
          break;
        case PDPB_SENSOR_P3V3:
          sprintf(name, "PDPB_P3V3");
          break;
        default:
          sprintf(name, "%s", "");
          break;
      }
      break;

    case FRU_FCB:
      switch(sensor_num) {
        case FCB_SENSOR_P12V_AUX:
          sprintf(name, "FCB_P12V_AUX");
          break;
        case FCB_SENSOR_P12VL:
          sprintf(name, "FCB_P12VL");
          break;
        case FCB_SENSOR_P12VU:
          sprintf(name, "FCB_P12VU");
          break;
        case FCB_SENSOR_P3V3:
          sprintf(name, "FCB_P3V3");
          break;
        case FCB_SENSOR_HSC_IN_VOLT:
          sprintf(name, "FCB_HSC_IN_VOLT");
          break;
        case FCB_SENSOR_HSC_OUT_CURR:
          sprintf(name, "FCB_HSC_OUT_CURR");
          break;
        case FCB_SENSOR_HSC_IN_POWER:
          sprintf(name, "FCB_HSC_IN_POWER");
          break;
        case FCB_SENSOR_BJT_TEMP_1:
          sprintf(name, "FCB_BJT_TEMP_1");
          break;
        case FCB_SENSOR_BJT_TEMP_2:
          sprintf(name, "FCB_BJT_TEMP_2");
          break;
        case FCB_SENSOR_FAN1_FRONT_SPEED:
          sprintf(name, "FAN1_FRONT_SPEED");
          break;
        case FCB_SENSOR_FAN1_REAR_SPEED:
          sprintf(name, "FAN1_REAR_SPEED");
          break;
        case FCB_SENSOR_FAN2_FRONT_SPEED:
          sprintf(name, "FAN2_FRONT_SPEED");
          break;
        case FCB_SENSOR_FAN2_REAR_SPEED:
          sprintf(name, "FAN2_REAR_SPEED");
          break;
        case FCB_SENSOR_FAN3_FRONT_SPEED:
          sprintf(name, "FAN3_FRONT_SPEED");
          break;
        case FCB_SENSOR_FAN3_REAR_SPEED:
          sprintf(name, "FAN3_REAR_SPEED");
          break;
        case FCB_SENSOR_FAN4_FRONT_SPEED:
          sprintf(name, "FAN4_FRONT_SPEED");
          break;
        case FCB_SENSOR_FAN4_REAR_SPEED:
          sprintf(name, "FAN4_REAR_SPEED");
          break;
        case FCB_SENSOR_FAN5_FRONT_SPEED:
          sprintf(name, "FAN5_FRONT_SPEED");
          break;
        case FCB_SENSOR_FAN5_REAR_SPEED:
          sprintf(name, "FAN5_REAR_SPEED");
          break;
        case FCB_SENSOR_FAN6_FRONT_SPEED:
          sprintf(name, "FAN6_FRONT_SPEED");
          break;
        case FCB_SENSOR_FAN6_REAR_SPEED:
          sprintf(name, "FAN6_REAR_SPEED");
          break;
        case FCB_SENSOR_AIRFLOW:
          sprintf(name, "AIRFLOW");
          break;
        default:
          sprintf(name, "%s", "");
          break;
      }
      break;
  }
  return 0;
}

int
lightning_sensor_read(uint8_t fru, uint8_t sensor_num, void *value) {
  int ret;

  switch (fru) {
    case FRU_PEB:
      switch(sensor_num) {
        // Temperature Sensors
        case PEB_SENSOR_PCIE_SW_TEMP:
          return read_temp_value(I2C_DEV_PEB, PEB_MAX6654_U4, REMOTE_SENSOR, (float*) value);
        case PEB_SENSOR_SYS_INLET_TEMP:
          return read_temp_value(I2C_DEV_PEB, PEB_TMP421_U15, REMOTE_SENSOR, (float*) value);

        // Hot Swap Controller
        case PEB_SENSOR_HSC_IN_VOLT:
          return read_hsc_value(HSC_IN_VOLT, I2C_DEV_PEB, I2C_ADDR_PEB_HSC, HSC_ADM1278, (float*) value);
        case PEB_SENSOR_HSC_OUT_CURR:
          return read_hsc_value(HSC_OUT_CURR, I2C_DEV_PEB, I2C_ADDR_PEB_HSC, HSC_ADM1278, (float*) value);
        case PEB_SENSOR_HSC_IN_POWER:
          return read_hsc_value(HSC_IN_POWER, I2C_DEV_PEB, I2C_ADDR_PEB_HSC, HSC_ADM1278, (float*) value);

        // ADC Voltages
        case PEB_SENSOR_ADC_P12V:
          return read_adc_value(ADC_PIN0, ADC_VALUE, (float*) value);
        case PEB_SENSOR_ADC_P5V:
          return read_adc_value(ADC_PIN1, ADC_VALUE, (float*) value);
        case PEB_SENSOR_ADC_P3V3_STBY:
          return read_adc_value(ADC_PIN2, ADC_VALUE, (float*) value);
        case PEB_SENSOR_ADC_P1V8_STBY:
          return read_adc_value(ADC_PIN3, ADC_VALUE, (float*) value);
        case PEB_SENSOR_ADC_P1V53:
          return read_adc_value(ADC_PIN4, ADC_VALUE, (float*) value);
        case PEB_SENSOR_ADC_P0V9:
          return read_adc_value(ADC_PIN5, ADC_VALUE, (float*) value);
        case PEB_SENSOR_ADC_P0V9_E:
          return read_adc_value(ADC_PIN6, ADC_VALUE, (float*) value);
        case PEB_SENSOR_ADC_P1V26:
          return read_adc_value(ADC_PIN7, ADC_VALUE, (float*) value);
        default:
          return -1;
      }
      break;

    case FRU_PDPB:

        if (sensor_num >= PDPB_SENSOR_FLASH_TEMP_0 &&
            sensor_num < (PDPB_SENSOR_FLASH_TEMP_0 + lightning_flash_cnt)) {
          ret = read_flash_temp(sensor_num - PDPB_SENSOR_FLASH_TEMP_0, (float*) value);

          if (ret < 0) {
            if (gpio_reset_ssd_switch() < 0) {
#ifdef DEBUG
              syslog(LOG_ERR, "lightning_sensor_read: gpio_reset_ssd_switch failed");
#endif
            }
          }

          return ret;
        }

        if (sensor_num >= PDPB_SENSOR_AMB_TEMP_0 &&
            sensor_num < (PDPB_SENSOR_AMB_TEMP_0 + lightning_flash_cnt)) {
          ret = read_m2_amb_temp(sensor_num - PDPB_SENSOR_AMB_TEMP_0, (float*) value);

          if (ret < 0) {
            if (gpio_reset_ssd_switch() < 0) {
#ifdef DEBUG
              syslog(LOG_ERR, "lightning_sensor_read: gpio_reset_ssd_switch failed");
#endif
            }
          }

          return ret;
        }

      switch(sensor_num) {
        // Temp
        case PDPB_SENSOR_LEFT_REAR_TEMP:
          //return read_temp_value(I2C_DEV_PDPB, PDPB_TMP75_U47, LOCAL_SENSOR, (float*) value);
          return read_tmp75_temp_value(PDPB_TMP75_U47_DEVICE, (float*) value);
        case PDPB_SENSOR_LEFT_FRONT_TEMP:
          //return read_temp_value(I2C_DEV_PDPB, PDPB_TMP75_U49, LOCAL_SENSOR, (float*) value);
          return read_tmp75_temp_value(PDPB_TMP75_U49_DEVICE, (float*) value);
        case PDPB_SENSOR_RIGHT_REAR_TEMP:
          //return read_temp_value(I2C_DEV_PDPB, PDPB_TMP75_U48, LOCAL_SENSOR, (float*) value);
          return read_tmp75_temp_value(PDPB_TMP75_U48_DEVICE, (float*) value);
        case PDPB_SENSOR_RIGHT_FRONT_TEMP:
          //return read_temp_value(I2C_DEV_PDPB, PDPB_TMP75_U51, LOCAL_SENSOR, (float*) value);
          return read_tmp75_temp_value(PDPB_TMP75_U51_DEVICE, (float*) value);

        // Voltage
        case PDPB_SENSOR_P12V:
          if (read_ads1015_value(ADS1015_CHANNEL4, I2C_DEV_PDPB, I2C_ADDR_PDPB_ADC, (float*) value) < 0) {
            return -1;
          }
          *((float *) value) = *((float *) value) * ((10 + 2) / 2); // Voltage Divider Circuit
          return 0;

        case PDPB_SENSOR_P3V3:
          if (read_ads1015_value(ADS1015_CHANNEL6, I2C_DEV_PDPB, I2C_ADDR_PDPB_ADC, (float*) value) < 0) {
            return -1;
          }
          // NOTE: DVT system has the Voltage Divider Circuit not populated
          //*((float *) value) = *((float *) value) * ((1 + 1) / 1); // Voltage Divider Circuit
          return 0;

        default:
          return -1;

      }
      break;

    case FRU_FCB:
      switch(sensor_num) {
        // Voltage
        case FCB_SENSOR_P12V_AUX:
          return read_nct7904_value(NCT7904_VSEN9, I2C_DEV_FCB, I2C_ADDR_NCT7904, (float*) value);
        case FCB_SENSOR_P12VL:
          return read_nct7904_value(NCT7904_VSEN6, I2C_DEV_FCB, I2C_ADDR_NCT7904, (float*) value);
        case FCB_SENSOR_P12VU:
          return read_nct7904_value(NCT7904_VSEN7, I2C_DEV_FCB, I2C_ADDR_NCT7904, (float*) value);
        case FCB_SENSOR_P3V3:
          return read_nct7904_value(NCT7904_3VDD, I2C_DEV_FCB, I2C_ADDR_NCT7904, (float*) value);

        // Hot Swap Controller
        case FCB_SENSOR_HSC_IN_VOLT:
          return read_hsc_value(HSC_IN_VOLT, I2C_DEV_FCB, I2C_ADDR_FCB_HSC, HSC_ADM1276, (float*) value);
        case FCB_SENSOR_HSC_OUT_CURR:
          return read_hsc_value(HSC_OUT_CURR, I2C_DEV_FCB, I2C_ADDR_FCB_HSC, HSC_ADM1276, (float*) value);
        case FCB_SENSOR_HSC_IN_POWER:
          return read_hsc_value(HSC_IN_POWER, I2C_DEV_FCB, I2C_ADDR_FCB_HSC, HSC_ADM1276, (float*) value);
        case FCB_SENSOR_BJT_TEMP_1:
          return read_nct7904_value(NCT7904_TEMP_CH1, I2C_DEV_FCB, I2C_ADDR_NCT7904, (float*) value);
        case FCB_SENSOR_BJT_TEMP_2:
          return read_nct7904_value(NCT7904_TEMP_CH2, I2C_DEV_FCB, I2C_ADDR_NCT7904, (float*) value);

        // Fan Speed
        case FCB_SENSOR_FAN6_FRONT_SPEED:
          return read_nct7904_value(FAN_REGISTER, I2C_DEV_FCB, I2C_ADDR_NCT7904, (float*) value);
        case FCB_SENSOR_FAN6_REAR_SPEED:
          return read_nct7904_value(FAN_REGISTER+2, I2C_DEV_FCB, I2C_ADDR_NCT7904, (float*) value);
        case FCB_SENSOR_FAN5_FRONT_SPEED:
          return read_nct7904_value(FAN_REGISTER+4, I2C_DEV_FCB, I2C_ADDR_NCT7904, (float*) value);
        case FCB_SENSOR_FAN5_REAR_SPEED:
          return read_nct7904_value(FAN_REGISTER+6, I2C_DEV_FCB, I2C_ADDR_NCT7904, (float*) value);
        case FCB_SENSOR_FAN4_FRONT_SPEED:
          return read_nct7904_value(FAN_REGISTER+8, I2C_DEV_FCB, I2C_ADDR_NCT7904, (float*) value);
        case FCB_SENSOR_FAN4_REAR_SPEED:
          return read_nct7904_value(FAN_REGISTER+10, I2C_DEV_FCB, I2C_ADDR_NCT7904, (float*) value);
        case FCB_SENSOR_FAN3_FRONT_SPEED:
          return read_nct7904_value(FAN_REGISTER+12, I2C_DEV_FCB, I2C_ADDR_NCT7904, (float*) value);
        case FCB_SENSOR_FAN3_REAR_SPEED:
          return read_nct7904_value(FAN_REGISTER+14, I2C_DEV_FCB, I2C_ADDR_NCT7904, (float*) value);
        case FCB_SENSOR_FAN2_FRONT_SPEED:
          return read_nct7904_value(FAN_REGISTER+16, I2C_DEV_FCB, I2C_ADDR_NCT7904, (float*) value);
        case FCB_SENSOR_FAN2_REAR_SPEED:
          return read_nct7904_value(FAN_REGISTER+18, I2C_DEV_FCB, I2C_ADDR_NCT7904, (float*) value);
        case FCB_SENSOR_FAN1_FRONT_SPEED:
          return read_nct7904_value(FAN_REGISTER+20, I2C_DEV_FCB, I2C_ADDR_NCT7904, (float*) value);
        case FCB_SENSOR_FAN1_REAR_SPEED:
          return read_nct7904_value(FAN_REGISTER+22, I2C_DEV_FCB, I2C_ADDR_NCT7904, (float*) value);

        // Airflow
        case FCB_SENSOR_AIRFLOW:
          return lightning_sensor_get_airflow((float*) value);

        default:
          return -1;
      }
      break;
  }
  return -1;
}
