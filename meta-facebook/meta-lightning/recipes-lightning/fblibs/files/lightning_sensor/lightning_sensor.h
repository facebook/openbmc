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
#include <facebook/lightning_common.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_SDR_LEN           64
#define MAX_SENSOR_NUM        0xFF
#define MAX_SENSOR_THRESHOLD  8
#define MAX_RETRIES_SDR_INIT  30
#define THERMAL_CONSTANT      255
#define ERR_NOT_READY         -2

typedef struct _sensor_info_t {
  bool valid;
  sdr_full_t sdr;
} sensor_info_t;

/* Enum for type of Upper and Lower threshold values */
enum {
  UCR_THRESH = 0x01,
  UNC_THRESH,
  UNR_THRESH,
  LCR_THRESH,
  LNC_THRESH,
  LNR_THRESH,
  POS_HYST,
  NEG_HYST,
};

// Sensors under PEB
enum {
  PEB_SENSOR_TEMP_1 = 0x37,
  PEB_SENSOR_TEMP_2 = 0x38,
  PEB_SENSOR_HSC_IN_VOLT = 0xE1,
  PEB_SENSOR_HSC_OUT_CURR = 0xE2,
  PEB_SENSOR_HSC_TEMP = 0xE3,
  PEB_SENSOR_HSC_IN_POWER = 0xE4,
};

// Sensors under PDPB
enum {
  PDPB_SENSOR_TEMP_1 = 0x31,
  PDPB_SENSOR_TEMP_2 = 0x32,
  PDPB_SENSOR_TEMP_3 = 0x33,
};

// Sensors under FCB
enum {
  FCB_SENSOR_HSC_IN_VOLT = 0xE5,
  FCB_SENSOR_HSC_OUT_CURR = 0xE6,
  FCB_SENSOR_HSC_IN_POWER = 0xE7,
};

extern const uint8_t peb_sensor_list[];

extern const uint8_t pdpb_sensor_list[];

extern const uint8_t fcb_sensor_list[];

extern size_t peb_sensor_cnt;

extern size_t pdpb_sensor_cnt;

extern size_t fcb_sensor_cnt;

int lightning_sensor_name(uint8_t fru, uint8_t sensor_num, char *name);
int lightning_sensor_units(uint8_t fru, uint8_t sensor_num, char *units);
int lightning_sensor_sdr_path(uint8_t fru, char *path);
int lightning_sensor_threshold(uint8_t fru, uint8_t sensor_num, uint8_t thresh, float *value);
int lightning_sensor_sdr_init(uint8_t fru, sensor_info_t *sinfo);


#ifdef __cplusplus
} // extern "C"
#endif

#endif /* __LIGHTNING_SENSOR_H__ */
