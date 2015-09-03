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

#include <facebook/bic.h>
#include <facebook/yosemite_common.h>
#include <facebook/yosemite_fruid.h>
#include <facebook/yosemite_sensor.h>

#define MAX_NUM_FRUS 6
#define MAX_KEY_LEN     64
#define MAX_VALUE_LEN   64

#define KV_STORE "/mnt/data/kv_store/%s"
#define KV_STORE_PATH "/mnt/data/kv_store"

extern char * key_list[];
extern const char pal_fru_list[];
extern const char pal_server_list[];

enum {
  LED_STATE_OFF,
  LED_STATE_ON,
};

enum {
  SERVER_POWER_OFF,
  SERVER_POWER_ON,
  SERVER_POWER_CYCLE,
  SERVER_GRACEFUL_SHUTDOWN,
};

enum {
  HAND_SW_SERVER1 = 1,
  HAND_SW_SERVER2,
  HAND_SW_SERVER3,
  HAND_SW_SERVER4,
  HAND_SW_BMC
};

int pal_get_platform_name(char *name);
int pal_get_num_slots(uint8_t *num);
int pal_is_server_prsnt(uint8_t slot_id, uint8_t *status);
int pal_get_server_power(uint8_t slot_id, uint8_t *status);
int pal_set_server_power(uint8_t slot_id, uint8_t cmd);
int pal_sled_cycle(void);
int pal_is_debug_card_prsnt(uint8_t *status);
int pal_get_hand_sw(uint8_t *pos);
int pal_switch_usb_mux(uint8_t slot);
int pal_switch_uart_mux(uint8_t slot);
int pal_post_enable(uint8_t slot);
int pal_post_disable(uint8_t slot);
int pal_post_get_last(uint8_t slot, uint8_t *post);
int pal_post_handle(uint8_t slot, uint8_t status);
int pal_get_pwr_btn(uint8_t *status);
int pal_get_rst_btn(uint8_t *status);
int pal_set_rst_btn(uint8_t slot, uint8_t status);
int pal_set_led(uint8_t slot, uint8_t status);
int pal_get_fru_id(char *fru_str, uint8_t *fru);
int pal_get_fruid_path(uint8_t fru, char *path);
int pal_get_fruid_name(uint8_t fru, char *name);
int pal_get_sensor_units(uint8_t fru, uint8_t sensor_num, char *units);
int pal_get_fru_sensor_list(uint8_t fru, uint8_t **sensor_list, int *cnt);
int pal_sensor_read(uint8_t fru, uint8_t sensor_num, void *value);
int pal_get_sensor_name(uint8_t fru, uint8_t sensor_num, char *name);
int pal_get_key_value(char *key, char *value);
int pal_set_key_value(char *key, char *value);
void pal_dump_key_value(void);
int pal_get_fru_devtty(uint8_t fru, char *devtty);

#ifdef __cplusplus
} // extern "C"
#endif

#endif /* __PAL_H__ */
