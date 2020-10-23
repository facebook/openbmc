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

#ifndef __PAL_H__
#define __PAL_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <openbmc/obmc-pal.h>
#include <openbmc/kv.h>
#include <facebook/fbgc_common.h>
#include <facebook/fbgc_fruid.h>
#include "pal_sensors.h"

#define MAX_NUM_FRUS (FRU_CNT-1)
#define MAX_NODES    1

#define CUSTOM_FRU_LIST 1

#define MAX_FRU_CMD_STR   16

#define GUID_SIZE 16
#define OFFSET_SYS_GUID 0x17F0
#define OFFSET_DEV_GUID 0x1800

enum {
  I2C_BUS_0 = 0,
  I2C_BUS_1,
  I2C_BUS_2,
  I2C_BUS_3,
  I2C_BUS_4,
  I2C_BUS_5,
  I2C_BUS_6,
  I2C_BUS_7,
  I2C_BUS_8,
  I2C_BUS_9,
  I2C_BUS_10,
  I2C_BUS_11,
  I2C_BUS_12,
  I2C_BUS_13,
  I2C_BUS_14,
  I2C_BUS_15,
};

extern const char pal_fru_list_print[];
extern const char pal_fru_list_rw[];
extern const char pal_fru_list_sensor_history[];
extern const char pal_fru_list[];
extern const char pal_pwm_list[];
extern const char pal_tach_list[];

extern size_t pal_pwm_cnt;
extern size_t pal_tach_cnt;

int pal_get_fru_id(char *str, uint8_t *fru);
int pal_is_fru_ready(uint8_t fru, uint8_t *status);
int pal_is_fru_prsnt(uint8_t fru, uint8_t *status);
int pal_get_fruid_name(uint8_t fru, char *name);
int pal_get_fruid_path(uint8_t fru, char *path);
int pal_get_fruid_eeprom_path(uint8_t fru, char *path);
int pal_get_fru_list(char *list) ;
int pal_get_fru_name(uint8_t fru, char *name);
int pal_get_fan_name(uint8_t fan_id, char *name);
int pal_set_fan_speed(uint8_t fan_id, uint8_t pwm);
int pal_get_pwm_value(uint8_t fan_id, uint8_t *pwm);
int pal_get_fru_sensor_list(uint8_t fru, uint8_t **sensor_list, int *cnt);
int pal_sensor_read_raw(uint8_t fru, uint8_t sensor_num, void *value);
int pal_get_sensor_name(uint8_t fru, uint8_t sensor_num, char *name);
int pal_get_sensor_threshold(uint8_t fru, uint8_t sensor_num, uint8_t thresh, void *value);
int pal_get_sensor_units(uint8_t fru, uint8_t sensor_num, char *units);

#ifdef __cplusplus
} // extern "C"
#endif

#endif /* __PAL_H__ */
