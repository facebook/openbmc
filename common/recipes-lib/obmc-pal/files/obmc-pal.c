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
