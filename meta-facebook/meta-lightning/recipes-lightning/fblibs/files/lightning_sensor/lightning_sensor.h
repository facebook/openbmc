/*
 *
 * Copyright 2015-present Facebook. All Rights Reserved.
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

#ifndef __LIGHTNING_SENSOR_H__
#define __LIGHTNING_SENSOR_H__

#include <stdbool.h>
#include <openbmc/ipmi.h>
#include <openbmc/obmc-pal.h>
#include <facebook/lightning_common.h>
#include <facebook/lightning_flash.h>
#include <facebook/lightning_gpio.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_SDR_LEN           64
#define MAX_SENSOR_NUM        256
#define MAX_SENSOR_THRESHOLD  8
#define MAX_RETRIES_SDR_INIT  30
#define THERMAL_CONSTANT      255
#define ERR_NOT_READY         -2

#define UPPER_TRAY 0x00
#define LOWER_TRAY 0x01

#define LOWER_TRAY_DONE 0x00
#define UPPER_TRAY_DONE 0x01

#define MAX_RETRY_TIMES 3

#define NUM_SSD 15

// Sensors under PEB
enum {
  PEB_SENSOR_ADC_P12V = 0x42,
  PEB_SENSOR_ADC_P5V = 0x43,
  PEB_SENSOR_ADC_P3V3_STBY = 0x44,
  PEB_SENSOR_ADC_P1V8_STBY = 0x45,
  PEB_SENSOR_ADC_P1V53 = 0x46,
  PEB_SENSOR_ADC_P0V9 = 0x47,
  PEB_SENSOR_ADC_P0V9_E = 0x48,
  PEB_SENSOR_ADC_P1V26 = 0xF0,
  PEB_SENSOR_HSC_IN_VOLT = 0x49,
  PEB_SENSOR_HSC_OUT_CURR = 0x4A,
  PEB_SENSOR_HSC_IN_POWER = 0x4D,
  PEB_SENSOR_PCIE_SW_TEMP = 0x53,
  PEB_SENSOR_PCIE_SW_FRONT_TEMP = 0x54,
  PEB_SENSOR_SYS_INLET_TEMP = 0x56,
};

// Sensors under PDPB
enum {
  PDPB_SENSOR_P12V = 0x4B,
  PDPB_SENSOR_P3V3 = 0x4C,
  PDPB_SENSOR_LEFT_REAR_TEMP = 0x4F,
  PDPB_SENSOR_LEFT_FRONT_TEMP = 0x50,
  PDPB_SENSOR_RIGHT_REAR_TEMP = 0x51,
  PDPB_SENSOR_RIGHT_FRONT_TEMP = 0x52,
  PDPB_SENSOR_FLASH_TEMP_0 = 0x63,
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
  PDPB_SENSOR_FLASH_TEMP_14 = 0x71,
  PDPB_SENSOR_AMB_TEMP_0 = 0x73,
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
  PDPB_SENSOR_AMB_TEMP_14 = 0x81,
};

// Sensors under FCB
enum {
  FCB_SENSOR_FAN1_FRONT_SPEED = 0x20,
  FCB_SENSOR_FAN1_REAR_SPEED = 0x21,
  FCB_SENSOR_FAN2_FRONT_SPEED = 0x22,
  FCB_SENSOR_FAN2_REAR_SPEED = 0x23,
  FCB_SENSOR_FAN3_FRONT_SPEED = 0x24,
  FCB_SENSOR_FAN3_REAR_SPEED = 0x25,
  FCB_SENSOR_FAN4_FRONT_SPEED = 0x26,
  FCB_SENSOR_FAN4_REAR_SPEED = 0x27,
  FCB_SENSOR_FAN5_FRONT_SPEED = 0x28,
  FCB_SENSOR_FAN5_REAR_SPEED = 0x29,
  FCB_SENSOR_FAN6_FRONT_SPEED = 0x2A,
  FCB_SENSOR_FAN6_REAR_SPEED = 0x2B,
  FCB_SENSOR_P12V_AUX = 0x34,
  FCB_SENSOR_P12VL = 0x35,
  FCB_SENSOR_P12VU = 0x36,
  FCB_SENSOR_P3V3 = 0x37,
  FCB_SENSOR_HSC_IN_VOLT = 0x38,
  FCB_SENSOR_HSC_OUT_CURR = 0x39,
  FCB_SENSOR_HSC_IN_POWER = 0x3A,
  FCB_SENSOR_BJT_TEMP_1 = 0x60,
  FCB_SENSOR_BJT_TEMP_2 = 0x61,
  FCB_SENSOR_AIRFLOW = 0x2C,
};

extern const uint8_t peb_sensor_pmc_list[];

extern const uint8_t peb_sensor_plx_list[];

extern const uint8_t pdpb_u2_sensor_list[];

extern const uint8_t pdpb_m2_sensor_list[];

extern const uint8_t fcb_sensor_list[];

extern const int peb_sensor_pmc_time_period[];

extern const int peb_sensor_plx_time_period[];

extern const int pdpb_u2_sensor_time_period[];

extern const int pdpb_m2_sensor_time_period[];

extern const int fcb_sensor_time_period[];

extern size_t peb_sensor_pmc_cnt;

extern size_t peb_sensor_plx_cnt;

extern size_t pdpb_u2_sensor_cnt;

extern size_t pdpb_m2_sensor_cnt;

extern size_t fcb_sensor_cnt;

int lightning_sensor_name(uint8_t fru, uint8_t sensor_num, char *name);
int lightning_sensor_units(uint8_t fru, uint8_t sensor_num, char *units);
int lightning_sensor_threshold(uint8_t fru, uint8_t sensor_num, uint8_t thresh, float *value);
int lightning_sensor_sdr_init(uint8_t fru, sensor_info_t *sinfo);
int lightning_sensor_read(uint8_t fru, uint8_t sensor_num, void *value);
int lightning_sensor_poll_interval(uint8_t fru, uint8_t sensor_num, uint32_t* value);
int lightning_sensor_get_airflow(float *airflow_cfm);

#ifdef __cplusplus
} // extern "C"
#endif

#endif /* __LIGHTNING_SENSOR_H__ */
