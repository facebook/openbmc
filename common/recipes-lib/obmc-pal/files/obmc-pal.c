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
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <fcntl.h>
#include <errno.h>
#include <syslog.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <sys/wait.h>
#include <openbmc/edb.h>
#include <openbmc/kv.h>

#define GPIO_VAL "/sys/class/gpio/gpio%d/value"

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
pal_bmc_err_disable(const char *error_item) {
  return 0;
}

int __attribute__((weak))
pal_bmc_err_enable(const char *error_item) {
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
pal_slot_ac_cycle(uint8_t slot, uint8_t *req_data, uint8_t req_len, uint8_t *res_data, uint8_t *res_len)
{
  return PAL_EOK;
}

int __attribute__((weak))
pal_get_poss_pcie_config(uint8_t slot, uint8_t *req_data, uint8_t req_len, uint8_t *res_data, uint8_t *res_len)
{
  return PAL_EOK;
}

int __attribute__((weak))
pal_get_pcie_port_config(uint8_t slot, uint8_t *req_data, uint8_t req_len, uint8_t *res_data, uint8_t *res_len)
{
  return PAL_ENOTSUP;
}

int __attribute__((weak))
pal_set_pcie_port_config(uint8_t slot, uint8_t *req_data, uint8_t req_len, uint8_t *res_data, uint8_t *res_len)
{
  return PAL_ENOTSUP;
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
  sprintf(name, "fru%u", fru);
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

int __attribute__((weak))
pal_handle_string_sel(char *log, uint8_t log_len)
{
  return PAL_EOK;
}

int __attribute__((weak))
pal_set_adr_trigger(uint8_t slot, bool trigger)
{
  return PAL_ENOTSUP;
}

int __attribute__((weak))
pal_flock_retry(int fd)
{
  int ret = 0;
  int retry_count = 0;

  ret = flock(fd, LOCK_EX | LOCK_NB);
  while (ret && (retry_count < 3)) {
    retry_count++;
    msleep(100);
    ret = flock(fd, LOCK_EX | LOCK_NB);
  }
  if (ret) {
    return -1;
  }

  return 0;
}

int __attribute__((weak))
pal_slotid_to_fruid(int slotid)
{
  // This function is for mapping to fruid from slotid
  // If the platform's slotid is different with fruid, need to rewrite
  // this function in project layer.
  return slotid;
}

int __attribute__((weak))
pal_devnum_to_fruid(int devnum)
{
  // This function is for mapping to fruid from devnum
  // If the platform's devnum is different with fruid, need to rewrite
  // this function in project layer.
  return devnum;
}

int __attribute__((weak))
pal_channel_to_bus(int channel)
{
  // This function is for mapping to bus from channel
  // If the platform's channel is different with bus, need to rewrite
  // this function in project layer.
  return channel;
}

int __attribute__((weak))
pal_set_fw_update_ongoing(uint8_t fruid, uint16_t tmout) {
  char key[64] = {0};
  char value[64] = {0};
  struct timespec ts;

  sprintf(key, "fru%d_fwupd", fruid);

  clock_gettime(CLOCK_MONOTONIC, &ts);
  ts.tv_sec += tmout;
  sprintf(value, "%d", ts.tv_sec);

  if (edb_cache_set(key, value) < 0) {
     return -1;
  }

  return 0;
}

bool __attribute__((weak))
pal_is_fw_update_ongoing(uint8_t fruid) {
  char key[MAX_KEY_LEN];
  char value[MAX_VALUE_LEN] = {0};
  int ret;
  struct timespec ts;

  ret = system("ps | grep -v 'grep' | grep 'flashcp' &> /dev/null");
  if (WIFEXITED(ret) && WEXITSTATUS(ret) == 0)
    return true;

  sprintf(key, "fru%d_fwupd", fruid);
  ret = edb_cache_get(key, value);
  if (ret < 0) {
     return false;
  }

  clock_gettime(CLOCK_MONOTONIC, &ts);
  if (strtoul(value, NULL, 10) > ts.tv_sec)
     return true;

  return false;
}

bool __attribute__((weak))
pal_is_fw_update_ongoing_system(void) {
  //Base on fru number to sum up if fw update is onging.
  uint8_t max_slot_num = 0;

  pal_get_num_slots(&max_slot_num);

  for(int i = 0; i <= max_slot_num; i++) { // 0 is reserved for BMC update
    int fruid = pal_slotid_to_fruid(i);
    if (pal_is_fw_update_ongoing(fruid) == true) //if any slot is true, then we can return true
      return true;
  }

  return false;
}

int __attribute__((weak))
run_command(const char* cmd) {
  int status = system(cmd);
  if (status == -1) { // system error or environment error
    return 127;
  }

  // WIFEXITED(status): cmd is executed complete or not. return true for success.
  // WEXITSTATUS(status): cmd exit code
  if (WIFEXITED(status) && (WEXITSTATUS(status) == 0))
    return 0;
  else
    return -1;
}

int __attribute__((weak))
pal_get_restart_cause(uint8_t slot, uint8_t *restart_cause) {
  char key[MAX_KEY_LEN];
  char value[MAX_VALUE_LEN] = {0};
  unsigned int cause;

  sprintf(key, "fru%d_restart_cause", slot);

  if (kv_get(key, value)) {
    return -1;
  }
  if(sscanf(value, "%u", &cause) != 1) {
    return -1;
  }
  *restart_cause = cause;
  return 0;
}

int __attribute__((weak))
pal_set_restart_cause(uint8_t slot, uint8_t restart_cause) {
  char key[MAX_KEY_LEN];
  char value[MAX_VALUE_LEN] = {0};

  sprintf(key, "fru%d_restart_cause", slot);
  sprintf(value, "%d", restart_cause);

  if (kv_set(key, value)) {
    return -1;
  }
  return 0;
}

int __attribute__((weak))
pal_get_nm_selftest_result(uint8_t fruid, uint8_t *data)
{
  return PAL_ENOTSUP;
}


int __attribute__((weak))
pal_handle_oem_1s_intr(uint8_t slot, uint8_t *data)
{
  return 0;
}

int __attribute__((weak))
pal_set_gpio_value(int gpio_num, uint8_t value) {
  char vpath[64] = {0};
  char *val;
  FILE *fp = NULL;
  int rc = 0;
  int ret = 0;
  int retry_cnt = 5;
  int i = 0;

  sprintf(vpath, GPIO_VAL, gpio_num);
  val = (value == 0) ? "0": "1";

  for (i = 0; i < retry_cnt; i++) {
    fp = fopen(vpath, "w");
    if (fp == NULL) {
      syslog(LOG_ERR, "%s(): failed to open device %s (%s)",
                       __func__, vpath, strerror(errno));
      if (i == (retry_cnt - 1)) {
        return errno;
      }
    } else {
      break;
    }
    msleep(100);
  }
  for (i = 0; i < retry_cnt; i++) {
    ret = 0;
    rc = fputs(val, fp);
    if (rc < 0) {
      syslog(LOG_ERR, "failed to write device %s (%s)", vpath, strerror(errno));
      if (i == (retry_cnt - 1)) {
        ret = errno;
      }
    } else {
      break;
    }
    msleep(100);
  }

  fclose(fp);

  return ret;
}

int __attribute__((weak))
pal_get_gpio_value(int gpio_num, uint8_t *value) {
  char vpath[64] = {0};
  FILE *fp = NULL;
  int rc = 0;
  int ret = 0;
  int retry_cnt = 5;
  int i = 0;

  sprintf(vpath, GPIO_VAL, gpio_num);

  for (i = 0; i < retry_cnt; i++) {
    fp = fopen(vpath, "r");
    if (fp == NULL) {
      syslog(LOG_ERR, "%s(): failed to open device %s (%s)",
                       __func__, vpath, strerror(errno));
      if (i == (retry_cnt - 1)) {
        return errno;
      }
    } else {
      break;
    }
    msleep(100);
  }
  for (i = 0; i < retry_cnt; i++) {
    ret = 0;
    rc = fscanf(fp, "%d", value);
    if (rc != 1) {
      syslog(LOG_ERR, "failed to read device %s (%s)", vpath, strerror(errno));
      if (i == (retry_cnt - 1)) {
        ret = errno;
      }
    } else {
      break;
    }
    msleep(100);
  }

  fclose(fp);

  return ret;
}
