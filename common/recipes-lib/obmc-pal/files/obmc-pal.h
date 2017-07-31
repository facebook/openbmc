/*
 * Copyright 2015-present Facebook. All Rights Reserved.
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
#ifndef __OBMC_PAL_H__
#define __OBMC_PAL_H__

#include <stdint.h>
#include <stdbool.h>
#include <errno.h>
#include <sys/file.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

enum {
  PAL_EOK = 0,
  PAL_ENOTSUP = -ENOTSUP,
  PAL_ENOTREADY = -EAGAIN,
  /* non system errors start from -256 downwards */
};

//for power restore policy
enum {
  POWER_CFG_OFF = 0,
  POWER_CFG_LPS,
  POWER_CFG_ON,
  POWER_CFG_UKNOWN,
};

enum LED_LOW_ACTIVE{
  LED_N_ON = 0,
  LED_N_OFF,
};

enum LED_HIGH_ACTIVE{
  LED_OFF = 0,
  LED_ON,
};

enum GPIO_VALUE{
  GPIO_LOW = 0,
  GPIO_HIGH,
};

enum CTRL_HIGH_ACTIVE{
  CTRL_ENABLE  = 1,
  CTRL_DISABLE = 0,
};

enum CTRL_LOW_ACTIVE{
  CTRL_N_ENABLE  = 0,
  CTRL_N_DISABLE = 1,
};

/* Enum for type of Upper and Lower threshold values */
enum {
  SENSOR_VALID = 0x0,
  UCR_THRESH = 0x01,
  UNC_THRESH,
  UNR_THRESH,
  LCR_THRESH,
  LNC_THRESH,
  LNR_THRESH,
  POS_HYST,
  NEG_HYST,
};

// Function Declarations
int pal_is_crashdump_ongoing(uint8_t fru);
int pal_init_sensor_check(uint8_t fru, uint8_t snr_num, void *snr);
int pal_set_last_boot_time(uint8_t slot, uint8_t *last_boot_time);
int pal_get_last_boot_time(uint8_t slot, uint8_t *last_boot_time);
void pal_get_chassis_status(uint8_t slot, uint8_t *req_data, uint8_t *res_data, uint8_t *res_len);
int pal_get_80port_record(uint8_t slot, uint8_t *req_data, uint8_t req_len, uint8_t *res_data, uint8_t *res_len);
int pal_set_boot_order(uint8_t slot, uint8_t *boot, uint8_t *res_data, uint8_t *res_len);
int pal_get_boot_order(uint8_t slot, uint8_t *req_data, uint8_t *boot, uint8_t *res_len);
void pal_set_post_start(uint8_t slot, uint8_t *req_data, uint8_t *res_data, uint8_t *res_len);
void pal_set_post_end(uint8_t slot, uint8_t *req_data, uint8_t *res_data, uint8_t *res_len);
int pal_get_board_id(uint8_t slot, uint8_t *req_data, uint8_t req_len, uint8_t *res_data, uint8_t *res_len);
int pal_parse_oem_sel(uint8_t fru, uint8_t *sel, char *error_log);
int pal_set_ppin_info(uint8_t slot, uint8_t *req_data, uint8_t req_len, uint8_t *res_data, uint8_t *res_len);
int pal_bmc_err_enable(const char *error_item);
int pal_bmc_err_disable(const char *error_item);
int pal_set_bios_current_boot_list(uint8_t slot, uint8_t *boot_list, uint8_t list_length, uint8_t *cc);
int pal_get_bios_current_boot_list(uint8_t slot, uint8_t *boot_list, uint8_t *list_length);
int pal_set_bios_fixed_boot_device(uint8_t slot, uint8_t *fixed_boot_device);
int pal_get_bios_fixed_boot_device(uint8_t slot, uint8_t *fixed_boot_device);
int pal_set_bios_restores_default_setting(uint8_t slot, uint8_t *default_setting);
int pal_get_bios_restores_default_setting(uint8_t slot, uint8_t *default_setting);
uint8_t pal_set_power_restore_policy(uint8_t slot, uint8_t *pwr_policy, uint8_t *res_data);
void pal_set_boot_option(unsigned char para,unsigned char* pbuff);
int pal_get_boot_option(unsigned char para,unsigned char* pbuff);
int pal_get_poss_pcie_config(uint8_t slot, uint8_t *req_data, uint8_t req_len, uint8_t *res_data, uint8_t *res_len);
int pal_get_pcie_port_config(uint8_t slot, uint8_t *req_data, uint8_t req_len, uint8_t *res_data, uint8_t *res_len);
int pal_set_pcie_port_config(uint8_t slot, uint8_t *req_data, uint8_t req_len, uint8_t *res_data, uint8_t *res_len);
int pal_slot_ac_cycle(uint8_t slot, uint8_t *req_data, uint8_t req_len, uint8_t *res_data, uint8_t *res_len);
int pal_set_cpu_mem_threshold(const char* threshold_path);
int pal_open_fw_update_flag(void);
int pal_remove_fw_update_flag(void);
int pal_get_fw_update_flag(void);
int pal_get_platform_name(char *name);
int pal_get_num_slots(uint8_t *num);
int pal_is_fru_prsnt(uint8_t fru, uint8_t *status);
int pal_get_server_power(uint8_t slot_id, uint8_t *status);
int pal_set_server_power(uint8_t slot_id, uint8_t cmd);
int pal_sled_cycle(void);
int pal_post_handle(uint8_t slot, uint8_t status);
int pal_set_rst_btn(uint8_t slot, uint8_t status);
int pal_set_led(uint8_t led, uint8_t status);
int pal_set_hb_led(uint8_t status);
int pal_get_fru_list(char *list);
int pal_get_fru_id(char *fru_str, uint8_t *fru);
int pal_get_fru_name(uint8_t fru, char *name);
int pal_get_fruid_path(uint8_t fru, char *path);
int pal_get_fruid_eeprom_path(uint8_t fru, char *path);
int pal_get_fruid_name(uint8_t fru, char *name);
int pal_get_fru_sensor_list(uint8_t fru, uint8_t **sensor_list, int *cnt);
int pal_get_fru_discrete_list(uint8_t fru, uint8_t **sensor_list, int *cnt);
int pal_fruid_write(uint8_t slot, char *path);
int pal_get_fru_devtty(uint8_t fru, char *devtty);
int pal_sensor_read(uint8_t fru, uint8_t sensor_num, void *value);
int pal_sensor_read_raw(uint8_t fru, uint8_t sensor_num, void *value);
int pal_sensor_threshold_flag(uint8_t fru, uint8_t snr_num, uint16_t *flag);
int pal_get_sensor_name(uint8_t fru, uint8_t sensor_num, char *name);
int pal_get_sensor_units(uint8_t fru, uint8_t sensor_num, char *units);
int pal_get_sensor_threshold(uint8_t fru, uint8_t sensor_num, uint8_t thresh, void *value);
int pal_get_key_value(char *key, char *value);
int pal_set_key_value(char *key, char *value);
int pal_set_def_key_value(void);
void pal_dump_key_value(void);
int pal_get_last_pwr_state(uint8_t fru, char *state);
int pal_set_last_pwr_state(uint8_t fru, char *state);
int pal_get_sys_guid(uint8_t slot, char *guid);
int pal_get_sysfw_ver(uint8_t slot, uint8_t *ver);
int pal_set_sysfw_ver(uint8_t slot, uint8_t *ver);
int pal_sensor_discrete_check(uint8_t fru, uint8_t snr_num, char *snr_name, uint8_t o_val, uint8_t n_val);
int pal_get_event_sensor_name(uint8_t fru, uint8_t *sel, char *name);
int pal_sel_handler(uint8_t fru, uint8_t snr_num, uint8_t *event_data);
int pal_parse_sel(uint8_t fru, uint8_t *sel, char *error_log);
int pal_set_sensor_health(uint8_t fru, uint8_t value);
int pal_get_fru_health(uint8_t fru, uint8_t *value);
int pal_get_fan_name(uint8_t num, char *name);
int pal_get_fan_speed(uint8_t fan, int *rpm);
int pal_set_fan_speed(uint8_t fan, uint8_t pwm);
int pal_get_pwm_value(uint8_t fan_num, uint8_t *value);
int pal_fan_dead_handle(int fan_num);
int pal_fan_recovered_handle(int fan_num);
void pal_inform_bic_mode(uint8_t fru, uint8_t mode);
void pal_update_ts_sled(void);
int pal_handle_dcmi(uint8_t fru, uint8_t *tbuf, uint8_t tlen, uint8_t *rbuf, uint8_t *rlen);
int pal_is_fru_ready(uint8_t fru, uint8_t *status);
int pal_is_slot_server(uint8_t fru);
int pal_self_tray_location(uint8_t *value);
void pal_log_clear(char *fru);
int pal_get_dev_guid(uint8_t fru, char *guid);
int pal_get_plat_sku_id(void);
void pal_sensor_assert_handle(uint8_t snr_num, float val, uint8_t thresh);
void pal_sensor_deassert_handle(uint8_t snr_num, float val, uint8_t thresh);
int pal_get_fw_info(unsigned char target, unsigned char* res, unsigned char* res_len);
void pal_i2c_crash_assert_handle(int i2c_bus_num);
void pal_i2c_crash_deassert_handle(int i2c_bus_num);
int pal_set_machine_configuration(uint8_t slot, uint8_t *req_data, uint8_t req_len, uint8_t *res_data, uint8_t *res_len);
int pal_handle_string_sel(char *log, uint8_t log_len);
int pal_set_adr_trigger(uint8_t slot, bool trigger);
int pal_flock_retry(int fd);
int pal_devnum_to_fruid(int devnum);
int pal_channel_to_bus(int channel);
int pal_set_fw_update_ongoing(uint8_t fruid, uint16_t tmout);
bool pal_is_fw_update_ongoing(uint8_t fruid);
bool pal_is_fw_update_ongoing_system(void);
int run_command(const char* cmd);
int pal_get_restart_cause(uint8_t slot, uint8_t *restart_cause);
int pal_set_restart_cause(uint8_t slot, uint8_t restart_cause);
int pal_get_nm_selftest_result(uint8_t fruid, uint8_t *data);
int pal_handle_oem_1s_intr(uint8_t slot, uint8_t *data);
int pal_set_gpio_value(int gpio_num, uint8_t value);
int pal_get_gpio_value(int gpio_num, uint8_t *value);

#ifdef __cplusplus
}
#endif

#endif
