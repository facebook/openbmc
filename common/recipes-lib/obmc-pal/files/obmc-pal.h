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

#ifndef BIT
#define BIT(value, index) ((value >> index) & 1)
#endif // BIT

#define SETBIT(x, y)        (x | (1ULL << y))
#define GETBIT(x, y)        ((x & (1ULL << y)) > y)
#define CLEARBIT(x, y)      (x & (~(1ULL << y)))
#define GETMASK(y)          (1ULL << y)
#define SETMASK(y)          (1ULL << y)

// for threshold-util
#define THRESHOLD_PATH     "/tmp/thresh-cache"
#define INIT_THRESHOLD_BIN "/tmp/thresh-cache/%s_init_thresh-val.bin"
#define THRESHOLD_BIN      "/tmp/thresh-cache/%s_thresh-val.bin"
#define THRESHOLD_RE_FLAG  "/tmp/thresh-cache/%s_thresh-reinit"
#define MAX_SENSOR_NUMBER  0xFF
#define MAX_THERSH_LEN     256

// for fru device
#define DEV_ALL         0x0
#define DEV_NONE        0xff

/* To hold the sensor info and calculated threshold values from the SDR */
/* To hold the sensor info and calculated threshold values from the SDR */
typedef struct {
  uint16_t flag;
  float ucr_thresh;
  float unc_thresh;
  float unr_thresh;
  float lcr_thresh;
  float lnc_thresh;
  float lnr_thresh;
  float pos_hyst;
  float neg_hyst;
  int curr_state;
  char name[32];
  char units[64];
  uint32_t poll_interval; // poll interval for each fru's sensor

} thresh_sensor_t;

enum {
  SENSORD_MODE_TESTING = 0x01,
  SENSORD_MODE_NORMAL  = 0x0F,
};

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

//For power control/status
enum {
  SERVER_POWER_OFF,
  SERVER_POWER_ON,
  SERVER_POWER_CYCLE,
  SERVER_POWER_RESET,
  SERVER_GRACEFUL_SHUTDOWN,
  SERVER_12V_OFF,
  SERVER_12V_ON,
  SERVER_12V_CYCLE,
  SERVER_GLOBAL_RESET,
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

enum {
  OS_BOOT = 0x1F,
};

enum {
  UNIFIED_PCIE_ERR = 0x0,
};

// Enum for get the event sensor name in processing SEL
enum {
  SYSTEM_EVENT = 0xE9,
  THERM_THRESH_EVT = 0x7D,
  BUTTON = 0xAA,
  POWER_STATE = 0xAB,
  CRITICAL_IRQ = 0xEA,
  POST_ERROR = 0x2B,
  MACHINE_CHK_ERR = 0x40,
  PCIE_ERR = 0x41,
  IIO_ERR = 0x43,
  MEMORY_ECC_ERR = 0X63,
  MEMORY_ERR_LOG_DIS = 0X87,
  PROCHOT_EXT = 0X51,
  PWR_ERR = 0X56,
  CATERR_A = 0xE6, // W100, FBTP
  CATERR_B = 0xEB, // FBY2, FBTTN, LIGHTNING ,YOSEMITE
  CPU_DIMM_HOT = 0xB3,
  SOFTWARE_NMI = 0x90,
  CPU0_THERM_STATUS = 0x1C,
  CPU1_THERM_STATUS = 0x1D,
  ME_POWER_STATE = 0x16,
  SPS_FW_HEALTH = 0x17,
  NM_EXCEPTION_A = 0x18,   // W100, FBTP, FBY2, YOSEMITE, FBTTN
  PCH_THERM_THRESHOLD = 0x8,
  NM_HEALTH = 0x19,
  NM_CAPABILITIES = 0x1A,
  NM_THRESHOLD = 0x1B,
  PWR_THRESH_EVT = 0x3B,
  MSMI = 0xE7,
  HPR_WARNING = 0xC5,
};

// Helper function needed by some of pal functions
void msleep(int msec);

// Function Declarations
int pal_is_bmc_por(void);
int pal_is_crashdump_ongoing(uint8_t fru);
int pal_is_cplddump_ongoing(uint8_t fru);
int pal_init_sensor_check(uint8_t fru, uint8_t snr_num, void *snr);
int pal_set_last_boot_time(uint8_t slot, uint8_t *last_boot_time);
int pal_get_last_boot_time(uint8_t slot, uint8_t *last_boot_time);
void pal_get_chassis_status(uint8_t slot, uint8_t *req_data, uint8_t *res_data, uint8_t *res_len);
int pal_chassis_control(uint8_t slot, uint8_t *req_data, uint8_t req_len);
void pal_get_sys_intf_caps(uint8_t slot, uint8_t *req_data, uint8_t *res_data, uint8_t *res_len);
int pal_get_80port_record(uint8_t slot, uint8_t *req_data, uint8_t req_len, uint8_t *res_data, uint8_t *res_len);
int pal_set_boot_order(uint8_t slot, uint8_t *boot, uint8_t *res_data, uint8_t *res_len);
int pal_get_boot_order(uint8_t slot, uint8_t *req_data, uint8_t *boot, uint8_t *res_len);
void pal_set_post_start(uint8_t slot, uint8_t *req_data, uint8_t *res_data, uint8_t *res_len);
void pal_set_post_end(uint8_t slot, uint8_t *req_data, uint8_t *res_data, uint8_t *res_len);
int pal_get_board_id(uint8_t slot, uint8_t *req_data, uint8_t req_len, uint8_t *res_data, uint8_t *res_len);
int pal_parse_oem_unified_sel(uint8_t fru, uint8_t *sel, char *error_log);
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
int pal_set_slot_led(uint8_t fru, uint8_t *req_data, uint8_t req_len, uint8_t *res_data, uint8_t *res_len);
int pal_get_poss_pcie_config(uint8_t slot, uint8_t *req_data, uint8_t req_len, uint8_t *res_data, uint8_t *res_len);
int pal_get_pcie_port_config(uint8_t slot, uint8_t *req_data, uint8_t req_len, uint8_t *res_data, uint8_t *res_len);
int pal_set_pcie_port_config(uint8_t slot, uint8_t *req_data, uint8_t req_len, uint8_t *res_data, uint8_t *res_len);
int pal_set_imc_version(uint8_t slot, uint8_t *req_data, uint8_t req_len, uint8_t *res_data, uint8_t *res_len);
int pal_sled_ac_cycle(uint8_t slot, uint8_t *req_data, uint8_t req_len, uint8_t *res_data, uint8_t *res_len);
int pal_set_cpu_mem_threshold(const char* threshold_path);
int pal_open_fw_update_flag(void);
int pal_remove_fw_update_flag(void);
int pal_get_fw_update_flag(void);
int pal_get_platform_name(char *name);
int pal_get_num_slots(uint8_t *num);
int pal_get_num_devs(uint8_t slot, uint8_t *num);
int pal_is_fru_prsnt(uint8_t fru, uint8_t *status);
int pal_get_server_power(uint8_t slot_id, uint8_t *status);
int pal_set_server_power(uint8_t slot_id, uint8_t cmd);
int pal_get_device_power(uint8_t slot_id, uint8_t dev_id, uint8_t *status, uint8_t *type);
int pal_set_device_power(uint8_t slot_id, uint8_t dev_id, uint8_t cmd);
int pal_sled_cycle(void);
int pal_post_handle(uint8_t slot, uint8_t status);
int pal_set_rst_btn(uint8_t slot, uint8_t status);
int pal_set_led(uint8_t led, uint8_t status);
int pal_set_hb_led(uint8_t status);
int pal_get_fru_list(char *list);
int pal_get_fru_id(char *fru_str, uint8_t *fru);
int pal_get_dev_id(char *fru_str, uint8_t *fru);
int pal_get_fru_name(uint8_t fru, char *name);
int pal_get_dev_name(uint8_t fru, uint8_t dev, char *name);
int pal_get_fruid_path(uint8_t fru, char *path);
int pal_get_dev_fruid_path(uint8_t fru, uint8_t dev_id, char *path);
int pal_get_fruid_eeprom_path(uint8_t fru, char *path);
int pal_get_fruid_name(uint8_t fru, char *name);
int pal_slotid_to_fruid(int slotid);
int pal_get_fru_sensor_list(uint8_t fru, uint8_t **sensor_list, int *cnt);
int pal_get_sensor_poll_interval(uint8_t fru, uint8_t sensor_num, uint32_t *value);
int pal_get_fru_discrete_list(uint8_t fru, uint8_t **sensor_list, int *cnt);
int pal_fruid_write(uint8_t slot, char *path);
int pal_dev_fruid_write(uint8_t fru, uint8_t dev_id, char *path);
int pal_get_fru_devtty(uint8_t fru, char *devtty);
bool pal_sensor_is_cached(uint8_t fru, uint8_t sensor_num);
int pal_sensor_read_raw(uint8_t fru, uint8_t sensor_num, void *value);
int pal_sensor_threshold_flag(uint8_t fru, uint8_t snr_num, uint16_t *flag);
int pal_alter_sensor_thresh_flag(uint8_t fru, uint8_t snr_num, uint16_t *flag);
int pal_get_sensor_name(uint8_t fru, uint8_t sensor_num, char *name);
int pal_get_sensor_units(uint8_t fru, uint8_t sensor_num, char *units);
int pal_get_sensor_threshold(uint8_t fru, uint8_t sensor_num, uint8_t thresh, void *value);
int pal_cfg_key_check(char *key);
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
bool pal_is_fru_x86(uint8_t fru);
int pal_get_x86_event_sensor_name(uint8_t fru, uint8_t snr_num, char *name);
int pal_get_event_sensor_name(uint8_t fru, uint8_t *sel, char *name);
int pal_sel_handler(uint8_t fru, uint8_t snr_num, uint8_t *event_data);
bool pal_parse_sel_helper(uint8_t fru, uint8_t *sel, char *error_log);
int pal_parse_sel(uint8_t fru, uint8_t *sel, char *error_log);
void pal_add_cri_sel(char *str);
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
int pal_is_slot_support_update(uint8_t fru);
int pal_self_tray_location(uint8_t *value);
void pal_log_clear(char *fru);
int pal_get_dev_guid(uint8_t fru, char *guid);
int pal_get_plat_sku_id(void);
int pal_is_test_board(void);
void pal_sensor_assert_handle(uint8_t fru, uint8_t snr_num, float val, uint8_t thresh);
void pal_sensor_deassert_handle(uint8_t fru, uint8_t snr_num, float val, uint8_t thresh);
int pal_get_fw_info(uint8_t fru, unsigned char target, unsigned char* res, unsigned char* res_len);
void pal_i2c_crash_assert_handle(int i2c_bus_num);
void pal_i2c_crash_deassert_handle(int i2c_bus_num);
int pal_set_machine_configuration(uint8_t slot, uint8_t *req_data, uint8_t req_len, uint8_t *res_data, uint8_t *res_len);
int pal_handle_string_sel(char *log, uint8_t log_len);
int pal_set_adr_trigger(uint8_t slot, bool trigger);
int pal_flock_retry(int fd);
int pal_unflock_retry(int fd);
int pal_devnum_to_fruid(int devnum);
int pal_channel_to_bus(int channel);
int pal_set_fw_update_ongoing(uint8_t fruid, uint16_t tmout);
bool pal_is_fw_update_ongoing(uint8_t fruid);
bool pal_is_fw_update_ongoing_system(void);
bool pal_is_crashdump_ongoing_system(void);
bool pal_is_cplddump_ongoing_system(void);
int pal_set_fw_update_state(uint8_t slot, uint8_t *req_data, uint8_t req_len, uint8_t *res_data, uint8_t *res_len);
int run_command(const char* cmd);
int pal_get_restart_cause(uint8_t slot, uint8_t *restart_cause);
int pal_set_restart_cause(uint8_t slot, uint8_t restart_cause);
int pal_get_nm_selftest_result(uint8_t fruid, uint8_t *data);
int pal_handle_oem_1s_intr(uint8_t slot, uint8_t *data);
int pal_handle_oem_1s_asd_msg_in(uint8_t slot, uint8_t *data, uint8_t data_len);
int pal_handle_oem_1s_ras_dump_in(uint8_t slot, uint8_t *data, uint8_t data_len);
int pal_set_gpio_value(int gpio_num, uint8_t value);
int pal_get_gpio_value(int gpio_num, uint8_t *value);
int pal_ipmb_processing(int bus, void *buf, uint16_t size);
int pal_ipmb_finished(int bus, void *buf, uint16_t size);
int pal_bypass_cmd(uint8_t slot, uint8_t *req_data, uint8_t req_len, uint8_t *res_data, uint8_t *res_len);
void pal_set_def_restart_cause(uint8_t slot);
int pal_compare_fru_data(char *fru_out, char *fru_in, int cmp_size);
int pal_sensor_thresh_modify(uint8_t fru,  uint8_t sensor_num, uint8_t thresh_type, float value);
int pal_get_all_thresh_from_file(uint8_t fru, thresh_sensor_t *sinfo, int mode);
int pal_copy_all_thresh_to_file(uint8_t fru, thresh_sensor_t *sinfo);
int pal_get_thresh_from_file(uint8_t fru, uint8_t snr_num, thresh_sensor_t *sinfo);
int pal_copy_thresh_to_file(uint8_t fru, uint8_t snr_num, thresh_sensor_t *sinfo);
bool pal_is_sensor_existing(uint8_t fru, uint8_t snr_num);
void pal_get_me_name(uint8_t fru, char *target_name);
uint8_t pal_parse_ras_sel(uint8_t slot, uint8_t *sel, char *error_log);
int pal_ignore_thresh(uint8_t fru, uint8_t snr_num, uint8_t thresh);
int pal_set_fru_post(uint8_t fru, uint8_t value);
uint8_t pal_add_imc_log(uint8_t slot, uint8_t *req_data, uint8_t req_len, uint8_t *res_data, uint8_t *res_len);
uint8_t pal_add_cper_log(uint8_t slot, uint8_t *req_data, uint8_t req_len, uint8_t *res_data, uint8_t *res_len);
int pal_set_tpm_physical_presence(uint8_t slot, uint8_t presence);
int pal_get_tpm_physical_presence(uint8_t slot);
int pal_create_TPMTimer(int fru);
int pal_force_update_bic_fw(uint8_t slot_id, uint8_t comp, char *path);
void pal_specific_plat_fan_check(bool status);
int pal_get_sensor_util_timeout(uint8_t fru);
#ifdef __cplusplus
}
#endif

#endif
