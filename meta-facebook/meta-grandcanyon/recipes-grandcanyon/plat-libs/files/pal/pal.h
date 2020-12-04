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

typedef enum {
  STATUS_LED_OFF,
  STATUS_LED_YELLOW,
  STATUS_LED_BLUE,
} status_led_color;

extern const char pal_fru_list_print[];
extern const char pal_fru_list_rw[];
extern const char pal_fru_list_sensor_history[];
extern const char pal_fru_list[];
extern const char pal_pwm_list[];
extern const char pal_tach_list[];

extern size_t pal_pwm_cnt;
extern size_t pal_tach_cnt;

enum {
  BOOT_DEVICE_IPV4     = 0x1,
  BOOT_DEVICE_HDD      = 0x2,
  BOOT_DEVICE_CDROM    = 0x3,
  BOOT_DEVICE_OTHERS   = 0x4,
  BOOT_DEVICE_IPV6     = 0x9,
  BOOT_DEVICE_RESERVED = 0xff,
};

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
int pal_set_key_value(char *key, char *value);
int pal_get_key_value(char *key, char *value);
int pal_set_id_led(uint8_t slot, enum LED_HIGH_ACTIVE status);
int pal_set_hb_led(uint8_t status);
int pal_set_status_led(uint8_t fru, status_led_color color);
int pal_sensor_sdr_init(uint8_t fru, sensor_info_t *sinfo);
int pal_get_nic_fru_id(void);
bool pal_sensor_is_cached(uint8_t fru, uint8_t sensor_num);

#ifdef __cplusplus
} // extern "C"
#endif

#endif /* __PAL_H__ */
