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

#include <openbmc/obmc-pal.h>
#include <openbmc/kv.h>

#ifdef __cplusplus
extern "C" {
#endif

#include <facebook/bic.h>
#include <facebook/minilaketb_common.h>
#include <facebook/minilaketb_fruid.h>
#include <facebook/minilaketb_sensor.h>

#define MAX_NUM_FAN     2

#define MAX_NODES 4

#define MAX_NIC_TEMP_RETRY 3

extern char * key_list[];
extern size_t pal_pwm_cnt;
extern size_t pal_tach_cnt;
extern const char pal_pwm_list[];
extern const char pal_tach_list[];
extern const char pal_fru_list[];
extern const char pal_server_list[];

enum {
  USB_MUX_OFF,
  USB_MUX_ON,
};

enum {
  HAND_SW_SERVER1 = 1,
  HAND_SW_BMC
};

enum {
  SERVER1 = 1,
  BMC
};

enum {
  BIC_MODE_NORMAL = 0x01,
  BIC_MODE_UPDATE = 0x0F,
};

enum {
  FAN_0 = 0,
  FAN_1,
};

int pal_is_test_board(void);
int pal_is_server_12v_on(uint8_t slot_id, uint8_t *status);
int pal_slot_pair_12V_off(uint8_t slot_id);
bool pal_is_hsvc_ongoing(uint8_t slot_id);
int pal_set_hsvc_ongoing(uint8_t slot_id, uint8_t status, uint8_t ident);
int pal_is_debug_card_prsnt(uint8_t *status);
int pal_get_hand_sw_physically(uint8_t *pos);
int pal_get_hand_sw(uint8_t *pos);
int pal_switch_vga_mux(uint8_t slot);
int pal_switch_usb_mux(uint8_t slot);
int pal_switch_uart_mux(uint8_t slot);
int pal_post_enable(uint8_t slot);
int pal_post_disable(uint8_t slot);
int pal_post_get_last(uint8_t slot, uint8_t *post);
int pal_get_pwr_btn(uint8_t *status);
int pal_get_rst_btn(uint8_t *status);
int pal_set_sled_led(uint8_t status);
int pal_set_id_led(uint8_t slot, uint8_t status);
int pal_get_fru_sdr_path(uint8_t fru, char *path);
int pal_sensor_sdr_init(uint8_t fru, sensor_info_t *sinfo);
int pal_get_pair_slot_type(uint8_t fru);
int pal_system_config_check(uint8_t slot_id);
int pal_read_nic_fruid(const char *path, int size);
void pal_post_end_chk(uint8_t *post_end_chk);
uint8_t pal_get_status(void);
int pal_get_fan_latch(uint8_t *status);

#ifdef __cplusplus
} // extern "C"
#endif

#endif /* __PAL_H__ */
