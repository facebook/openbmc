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
#include "obmc-pal.h"
#include <syslog.h>
#include <string.h>
#include <unistd.h>

int __attribute__((weak))
pal_is_fw_update_ongoing(uint8_t fru)
{
  return 0;                     /* false */
}

void __attribute__((weak))
set_fw_update_ongoing(uint8_t fru, uint16_t tmout)
{
  return;
}

int __attribute__((weak))
pal_init_sensor_check(uint8_t fru, uint8_t snr_num, void *snr)
{
  return PAL_EOK;
}

int __attribute__((weak))
pal_set_last_boot_time(uint8_t slot, uint8_t *last_boot_time)
{
  return PAL_ENOTSUP;
}

int __attribute__((weak))
pal_get_last_boot_time(uint8_t slot, uint8_t *last_boot_time)
{
  return PAL_ENOTSUP;
}

void __attribute__((weak))
pal_get_chassis_status(uint8_t slot, uint8_t *req_data, uint8_t *res_data, uint8_t *res_len)
{
  *res_len = 0;
  return;
}

int __attribute__((weak))
pal_get_80port_record(uint8_t slot, uint8_t *req_data, uint8_t req_len, uint8_t *res_data, uint8_t *res_len)
{
  return PAL_ENOTSUP;
}

int __attribute__((weak))
pal_set_boot_order(uint8_t slot, uint8_t *boot, uint8_t *res_data, uint8_t *res_len)
{
  return PAL_ENOTSUP;
}

int __attribute__((weak))
pal_get_boot_order(uint8_t slot, uint8_t *req_data, uint8_t *boot, uint8_t *res_len)
{
  return PAL_ENOTSUP;
}

void __attribute__((weak))
pal_set_boot_option(unsigned char para,unsigned char* pbuff)
{
  return;
}

int __attribute__((weak))
pal_get_boot_option(unsigned char para,unsigned char* pbuff)
{
  return PAL_ENOTSUP;
}

int __attribute__((weak))
pal_set_bios_current_boot_list(uint8_t slot, uint8_t *boot_list, uint8_t list_length, uint8_t *cc) {
  return 0;
}

int __attribute__((weak))
pal_get_bios_current_boot_list(uint8_t slot, uint8_t *boot_list, uint8_t *list_length) {
  return 0;
}

int __attribute__((weak))
pal_set_bios_fixed_boot_device(uint8_t slot, uint8_t *fixed_boot_device) {
  return 0;
}

int __attribute__((weak))
pal_get_bios_fixed_boot_device(uint8_t slot, uint8_t *fixed_boot_device) {
  return 0;
}

int __attribute__((weak))
pal_set_bios_restores_default_setting(uint8_t slot, uint8_t *default_setting) {
  return 0;
}

int __attribute__((weak))
pal_get_bios_restores_default_setting(uint8_t slot, uint8_t *default_setting) {
  return 0;
}

uint8_t __attribute__((weak))
pal_set_power_restore_policy(uint8_t slot, uint8_t *pwr_policy, uint8_t *res_data) {
  return 0;
}

int __attribute__((weak))
pal_bmc_err_disable(void) {
  // dummy function
  return 0;
}
int __attribute__((weak))
pal_bmc_err_enable() {
  // dummy function
  return 0;
}

void __attribute__((weak))
pal_set_post_start(uint8_t slot, uint8_t *req_data, uint8_t *res_data, uint8_t *res_len)
{
// TODO: For now logging the event, need to find usage for this info
  syslog (LOG_INFO, "POST Start Event for Payload#%d\n", slot);
  *res_len = 0;
  return;
}

void __attribute__((weak))
pal_set_post_end(uint8_t slot, uint8_t *req_data, uint8_t *res_data, uint8_t *res_len)
{
// TODO: For now logging the event, need to find usage for this info
  syslog (LOG_INFO, "POST End Event for Payload#%d\n", slot);
  *res_len = 0;
}

int __attribute__((weak))
pal_get_board_id(uint8_t slot, uint8_t *req_data, uint8_t req_len, uint8_t *res_data, uint8_t *res_len)
{
  return PAL_EOK;
}

int __attribute__((weak))
pal_parse_oem_sel(uint8_t fru, uint8_t *sel, char *error_log)
{
  error_log[0] = '\0';
  return 0;
}

int __attribute__((weak))
pal_set_ppin_info(uint8_t slot, uint8_t *req_data, uint8_t req_len, uint8_t *res_data, uint8_t *res_len)
{
	return PAL_EOK;
}

int __attribute__((weak))
pal_get_poss_pcie_config(uint8_t slot, uint8_t *req_data, uint8_t req_len, uint8_t *res_data, uint8_t *res_len)
{
  return PAL_EOK;
}

int __attribute__((weak))
pal_set_cpu_mem_threshold(const char* threshold_path)
{
  return PAL_EOK;
}

int __attribute__((weak))
pal_get_platform_name(char *name)
{
  return PAL_EOK;
}

int __attribute__((weak))
pal_get_num_slots(uint8_t *num)
{
  return PAL_EOK;
}

int __attribute__((weak))
pal_is_fru_prsnt(uint8_t fru, uint8_t *status)
{
  return PAL_EOK;
}

int __attribute__((weak))
pal_get_server_power(uint8_t slot_id, uint8_t *status)
{
  return PAL_EOK;
}

int __attribute__((weak))
pal_set_server_power(uint8_t slot_id, uint8_t cmd)
{
  return PAL_EOK;
}

int __attribute__((weak))
pal_sled_cycle(void)
{
  return PAL_EOK;
}

int __attribute__((weak))
pal_post_handle(uint8_t slot, uint8_t status)
{
  return PAL_EOK;
}

int __attribute__((weak))
pal_set_rst_btn(uint8_t slot, uint8_t status)
{
  return PAL_EOK;
}

int __attribute__((weak))
pal_set_led(uint8_t led, uint8_t status)
{
  return PAL_EOK;
}

int __attribute__((weak))
pal_set_hb_led(uint8_t status)
{
  return PAL_EOK;
}

int __attribute__((weak))
pal_get_fru_list(char *list)
{
  return PAL_EOK;
}

int __attribute__((weak))
pal_get_fru_id(char *str, uint8_t *fru)
{
  return PAL_EOK;
}

int __attribute__((weak))
pal_get_fru_name(uint8_t fru, char *name)
{
  return PAL_EOK;
}

int __attribute__((weak))
pal_get_fruid_path(uint8_t fru, char *path)
{
  return PAL_EOK;
}

int __attribute__((weak))
pal_get_fruid_eeprom_path(uint8_t fru, char *path)
{
  return PAL_EOK;
}

int __attribute__((weak))
pal_get_fruid_name(uint8_t fru, char *name)
{
  return PAL_EOK;
}

int __attribute__((weak))
pal_get_fru_sensor_list(uint8_t fru, uint8_t **sensor_list, int *cnt)
{
  return PAL_EOK;
}

int __attribute__((weak))
pal_get_fru_discrete_list(uint8_t fru, uint8_t **sensor_list, int *cnt)
{
  return PAL_EOK;
}

int __attribute__((weak))
pal_fruid_write(uint8_t slot, char *path)
{
  return PAL_EOK;
}

int __attribute__((weak))
pal_get_fru_devtty(uint8_t fru, char *devtty)
{
  return PAL_EOK;
}

int __attribute__((weak))
pal_sensor_read(uint8_t fru, uint8_t sensor_num, void *value)
{
  return PAL_EOK;
}

int __attribute__((weak))
pal_sensor_read_raw(uint8_t fru, uint8_t sensor_num, void *value)
{
  return PAL_EOK;
}

int __attribute__((weak))
pal_sensor_threshold_flag(uint8_t fru, uint8_t snr_num, uint16_t *flag)
{
  return PAL_EOK;
}

int __attribute__((weak))
pal_get_sensor_name(uint8_t fru, uint8_t sensor_num, char *name)
{
  return PAL_EOK;
}

int __attribute__((weak))
pal_get_sensor_units(uint8_t fru, uint8_t sensor_num, char *units)
{
  return PAL_EOK;
}

int __attribute__((weak))
pal_get_sensor_threshold(uint8_t fru, uint8_t sensor_num, uint8_t thresh, void *value)
{
  return PAL_EOK;
}

int __attribute__((weak))
pal_get_key_value(char *key, char *value)
{
  return PAL_EOK;
}

int __attribute__((weak))
pal_set_key_value(char *key, char *value)
{
  return PAL_EOK;
}

int __attribute__((weak))
pal_set_def_key_value(void)
{
  return PAL_EOK;
}

void __attribute__((weak))
pal_dump_key_value(void)
{
  return;
}

int __attribute__((weak))
pal_get_last_pwr_state(uint8_t fru, char *state)
{
  return PAL_EOK;
}

int __attribute__((weak))
pal_set_last_pwr_state(uint8_t fru, char *state)
{
  return PAL_EOK;
}

int __attribute__((weak))
pal_get_sys_guid(uint8_t slot, char *guid)
{
  return PAL_EOK;
}

int __attribute__((weak))
pal_get_sysfw_ver(uint8_t slot, uint8_t *ver)
{
  return PAL_EOK;
}

int __attribute__((weak))
pal_set_sysfw_ver(uint8_t slot, uint8_t *ver)
{
  return PAL_EOK;
}

int __attribute__((weak))
pal_sensor_discrete_check(uint8_t fru, uint8_t snr_num, char *snr_name,
    uint8_t o_val, uint8_t n_val)
{
  return PAL_EOK;
}

int __attribute__((weak))
pal_get_event_sensor_name(uint8_t fru, uint8_t *sel, char *name)
{
  return PAL_EOK;
}

int __attribute__((weak))
pal_sel_handler(uint8_t fru, uint8_t snr_num, uint8_t *event_data)
{
  return PAL_EOK;
}

int __attribute__((weak))
pal_parse_sel(uint8_t fru, uint8_t *sel, char *error_log)
{
  return PAL_EOK;
}

int __attribute__((weak))
pal_set_sensor_health(uint8_t fru, uint8_t value)
{
  return PAL_EOK;
}

int __attribute__((weak))
pal_get_fru_health(uint8_t fru, uint8_t *value)
{
  return PAL_EOK;
}

int __attribute__((weak))
pal_get_fan_name(uint8_t num, char *name)
{
  return PAL_EOK;
}

int __attribute__((weak))
pal_get_fan_speed(uint8_t fan, int *rpm)
{
  return PAL_EOK;
}

int __attribute__((weak))
pal_set_fan_speed(uint8_t fan, uint8_t pwm)
{
  return PAL_EOK;
}

int __attribute__((weak))
pal_get_pwm_value(uint8_t fan_num, uint8_t *value)
{
  return PAL_EOK;
}

int __attribute__((weak))
pal_fan_dead_handle(int fan_num)
{
  return PAL_EOK;
}

int __attribute__((weak))
pal_fan_recovered_handle(int fan_num)
{
  return PAL_EOK;
}

void __attribute__((weak))
pal_inform_bic_mode(uint8_t fru, uint8_t mode)
{
  return;
}

void __attribute__((weak))
pal_update_ts_sled(void)
{
  return;
}

int __attribute__((weak))
pal_handle_dcmi(uint8_t fru, uint8_t *tbuf, uint8_t tlen, uint8_t *rbuf, uint8_t *rlen)
{
  return PAL_EOK;
}

int __attribute__((weak))
pal_is_fru_ready(uint8_t fru, uint8_t *status)
{
  return PAL_EOK;
}

int __attribute__((weak))
pal_is_slot_server(uint8_t fru)
{
  return PAL_EOK;
}

int __attribute__((weak))
pal_self_tray_location(uint8_t *value)
{
  return PAL_EOK;
}

void __attribute__((weak))
pal_log_clear(char *fru)
{
  return;
}

int __attribute__((weak))
pal_get_dev_guid(uint8_t fru, char *guid)
{
  return PAL_EOK;
}

int __attribute__((weak))
pal_get_plat_sku_id(void)
{
  return PAL_EOK;
}

void __attribute__((weak))
pal_sensor_assert_handle(uint8_t snr_num, float val, uint8_t thresh)
{
  return;
}

void __attribute__((weak))
pal_sensor_deassert_handle(uint8_t snr_num, float val, uint8_t thresh)
{
  return;
}

int __attribute__((weak))
pal_get_fw_info(unsigned char target, unsigned char* res, unsigned char* res_len)
{
  return PAL_EOK;
}

void __attribute__((weak))
pal_i2c_crash_assert_handle(int i2c_bus_num)
{
  return;
}

void __attribute__((weak))
pal_i2c_crash_deassert_handle(int i2c_bus_num)
{
  return;
}

int __attribute__((weak))
pal_is_crashdump_ongoing(uint8_t fru)
{
  char fname[128];
  snprintf(fname, 128, "/var/run/autodump%d.pid", fru);
  if (access(fname, F_OK) == 0) {
    return 1;
  }
  return 0;                     /* false */
}

int __attribute__((weak))
pal_open_fw_update_flag(void) {
  return -1;
}

int __attribute__((weak))
pal_remove_fw_update_flag(void) {
  return -1;
}

int __attribute__((weak))
pal_get_fw_update_flag(void)
{
  return 0;
}

int __attribute__((weak))
pal_set_machine_configuration(uint8_t slot, uint8_t *req_data, uint8_t req_len, uint8_t *res_data, uint8_t *res_len)
{
  return PAL_EOK;
}
