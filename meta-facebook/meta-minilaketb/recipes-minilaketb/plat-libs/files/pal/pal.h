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

#ifdef __cplusplus
extern "C" {
#endif

#include <facebook/bic.h>
#include <facebook/minilaketb_common.h>
#include <facebook/minilaketb_fruid.h>
#include <facebook/minilaketb_sensor.h>
#include <openbmc/kv.h>

#define MAX_KEY_LEN     64
#define MAX_NUM_FAN     2

#define FRU_STATUS_GOOD   1
#define FRU_STATUS_BAD    0

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

int pal_get_platform_name(char *name);
int pal_get_num_slots(uint8_t *num);
int pal_is_test_board(void);
int pal_is_fru_prsnt(uint8_t fru, uint8_t *status);
int pal_is_fru_ready(uint8_t fru, uint8_t *status);
int pal_is_slot_server(uint8_t fru);
int pal_get_server_power(uint8_t slot_id, uint8_t *status);
int pal_set_server_power(uint8_t slot_id, uint8_t cmd);
int pal_is_server_12v_on(uint8_t slot_id, uint8_t *status);
int pal_slot_pair_12V_off(uint8_t slot_id);
bool pal_is_hsvc_ongoing(uint8_t slot_id);
int pal_set_hsvc_ongoing(uint8_t slot_id, uint8_t status, uint8_t ident);
int pal_sled_cycle(void);
int pal_is_debug_card_prsnt(uint8_t *status);
int pal_get_hand_sw_physically(uint8_t *pos);
int pal_get_hand_sw(uint8_t *pos);
int pal_switch_vga_mux(uint8_t slot);
int pal_switch_usb_mux(uint8_t slot);
int pal_switch_uart_mux(uint8_t slot);
int pal_post_enable(uint8_t slot);
int pal_post_disable(uint8_t slot);
int pal_post_get_last(uint8_t slot, uint8_t *post);
int pal_post_handle(uint8_t slot, uint8_t status);
int pal_get_pwr_btn(uint8_t *status);
int pal_get_rst_btn(uint8_t *status);
int pal_set_rst_btn(uint8_t slot, uint8_t status);
int pal_set_sled_led(uint8_t status);
int pal_set_led(uint8_t slot, uint8_t status);
int pal_set_hb_led(uint8_t status);
int pal_set_id_led(uint8_t slot, uint8_t status);
int pal_get_fru_list(char *list);
int pal_get_fru_id(char *fru_str, uint8_t *fru);
int pal_get_fru_name(uint8_t fru, char *name);
int pal_get_fruid_path(uint8_t fru, char *path);
int pal_get_fruid_eeprom_path(uint8_t fru, char *path);
int pal_get_fruid_name(uint8_t fru, char *name);
int pal_get_fru_sdr_path(uint8_t fru, char *path);
int pal_get_sensor_units(uint8_t fru, uint8_t sensor_num, char *units);
int pal_get_fru_sensor_list(uint8_t fru, uint8_t **sensor_list, int *cnt);
int pal_get_fru_discrete_list(uint8_t fru, uint8_t **sensor_list, int *cnt);
int pal_sensor_sdr_init(uint8_t fru, sensor_info_t *sinfo);
int pal_sensor_read_raw(uint8_t fru, uint8_t sensor_num, void *value);
int pal_sensor_threshold_flag(uint8_t fru, uint8_t snr_num, uint16_t *flag);
int pal_get_sensor_name(uint8_t fru, uint8_t sensor_num, char *name);
int pal_get_sensor_threshold(uint8_t fru, uint8_t sensor_num, uint8_t thresh, void *value);
int pal_get_key_value(char *key, char *value);
int pal_set_key_value(char *key, char *value);
int pal_set_def_key_value();
void pal_dump_key_value(void);
int pal_get_pair_slot_type(uint8_t fru);
int pal_system_config_check(uint8_t slot_id);
int pal_get_fru_devtty(uint8_t fru, char *devtty);
int pal_get_last_pwr_state(uint8_t fru, char *state);
int pal_set_last_pwr_state(uint8_t fru, char *state);
int pal_get_dev_guid(uint8_t slot, char *guid);
int pal_set_dev_guid(uint8_t slot, char *str);
int pal_get_sys_guid(uint8_t slot, char *guid);
int pal_set_sys_guid(uint8_t slot, char *str);
int pal_set_sysfw_ver(uint8_t slot, uint8_t *ver);
int pal_get_sysfw_ver(uint8_t slot, uint8_t *ver);
int pal_read_nic_fruid(const char *path, int size);
int pal_fruid_write(uint8_t slot, char *path);
int pal_is_bmc_por(void);
int pal_sensor_discrete_check(uint8_t fru, uint8_t snr_num, char *snr_name, uint8_t o_val, uint8_t n_val);
int pal_get_event_sensor_name(uint8_t fru, uint8_t *sel, char *name);
int pal_parse_sel(uint8_t fru, uint8_t *sel, char *error_log);
int pal_parse_oem_sel(uint8_t fru, uint8_t *sel, char *error_log);
int pal_sel_handler(uint8_t fru, uint8_t snr_num, uint8_t *event_data);
void msleep(int msec);
int pal_set_sensor_health(uint8_t fru, uint8_t value);
int pal_get_fru_health(uint8_t fru, uint8_t *value);
int pal_set_fan_speed(uint8_t fan, uint8_t pwm);
int pal_get_fan_speed(uint8_t fan, int *rpm);
int pal_get_fan_name(uint8_t num, char *name);
void pal_inform_bic_mode(uint8_t fru, uint8_t mode);
void pal_update_ts_sled();
int pal_handle_dcmi(uint8_t fru, uint8_t *tbuf, uint8_t tlen, uint8_t *rbuf, uint8_t *rlen);
void pal_log_clear(char *fru);
int pal_get_pwm_value(uint8_t fan_num, uint8_t *value);
int pal_fan_dead_handle(int fan_num);
int pal_fan_recovered_handle(int fan_num);
void pal_sensor_assert_handle(uint8_t fru, uint8_t snr_num, float val, uint8_t thresh);
void pal_sensor_deassert_handle(uint8_t fru, uint8_t snr_num, float val, uint8_t thresh);
void pal_post_end_chk(uint8_t *post_end_chk);
int pal_get_fw_info(uint8_t fru, unsigned char target, unsigned char* res, unsigned char* res_len);
void pal_add_cri_sel(char *str);
uint8_t pal_get_status(void);
int pal_bypass_cmd(uint8_t slot, uint8_t *req_data, uint8_t req_len, uint8_t *res_data, uint8_t *res_len);
int pal_get_fan_latch(uint8_t *status);
int pal_ipmb_processing(int bus, void *buf, uint16_t size);
bool pal_is_mcu_working(void);
#ifdef __cplusplus
} // extern "C"
#endif

#endif /* __PAL_H__ */
