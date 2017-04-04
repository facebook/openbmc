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

#include <facebook/lightning_common.h>
#include <facebook/lightning_fruid.h>
#include <facebook/lightning_sensor.h>
#include <facebook/lightning_flash.h>
#include <openbmc/kv.h>
#include <openbmc/nvme-mi.h>

#define MAX_KEY_LEN     64
#define MAX_VALUE_LEN   128
#define MAX_RETRY       5

#define FRU_STATUS_GOOD   1
#define FRU_STATUS_BAD    0

#define KV_STORE "/mnt/data/kv_store/%s"
#define KV_STORE_PATH "/mnt/data/kv_store"

#define SETBIT(x, y)        (x | (1 << y))
#define GETBIT(x, y)        ((x & (1 << y)) > y)
#define CLEARBIT(x, y)      (x & (~(1 << y)))
#define GETMASK(y)          (1 << y)

#define MAX_NODES 1
#define MAX_FAN_LED_NUM 6

#define MAX_SENSOR_NUM 256
#define ERROR_CODE_NUM 13
#define ERR_CODE_FILE "/tmp/error_code"
#define SENSOR_STATUS_FILE "/tmp/share_sensor_status"
#define SKIP_READ_SSD_TEMP "/tmp/skipSSD"

#define CMD_DRIVE_STATUS 0
#define CMD_DRIVE_HEALTH 1

#define ERR_CODE_I2C_CRASH_BASE 0x02
#define ERR_CODE_SELF_TRAY_PULL_OUT 0x5D
#define ERR_CODE_PEER_TRAY_PULL_OUT 0x5E
#define ERR_CODE_BMC_ERR 0x60

#define HB_INTERVAL 500

#define ERR_ASSERT 1
#define ERR_DEASSERT 0

#define I2C_BUS_MAX_NUMBER 14

#define BMC_HEALTH 4      // for bmc_health key 

extern char * key_list[];
extern size_t pal_pwm_cnt;
extern size_t pal_tach_cnt;
extern const char pal_pwm_list[];
extern const char pal_tach_list[];
extern const char pal_fru_list[];
extern const char pal_server_list[];

enum {
  LED_STATE_OFF,
  LED_STATE_ON,
};

enum {
  USB_MUX_OFF,
  USB_MUX_ON,
};

enum {
  SERVER_POWER_OFF,
  SERVER_POWER_ON,
  SERVER_POWER_CYCLE,
  SERVER_GRACEFUL_SHUTDOWN,
  SERVER_12V_OFF,
  SERVER_12V_ON,
  SERVER_12V_CYCLE,
};

enum {
  UART_POS_BMC = 0,
  UART_POS_PCIE_SW,
};

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
  PROCHOT_EXT = 0X51,
  PWR_ERR = 0X56,
  CATERR= 0xEB,
};

enum {
  FAN_1_FRONT = 0,
  FAN_1_REAR,
  FAN_2_FRONT,
  FAN_2_REAR,
  FAN_3_FRONT,
  FAN_3_REAR,
  FAN_4_FRONT,
  FAN_4_REAR,
  FAN_5_FRONT,
  FAN_5_REAR,
  FAN_6_FRONT,
  FAN_6_REAR,
};

enum {
  LED_ENCLOSURE = 127, // GPIOP7
  LED_HB = 115, //GPIOO3
  LED_DR_LED1 = 106, // GPION2
  LED_BMC_ID = 57, //GPIOH1
};

// FAN LED operation
enum {
  FAN_LED_ON = 0x00,
  FAN_LED_OFF,
  FAN_LED_BLINK_PWM0_RATE,
  FAN_LED_BLINK_PWM1_RATE,
};

//FAN LED control regsiter
enum {
  REG_INPUT = 0x00,
  REG_PSC0,
  REG_PWM0,
  REG_OSC1,
  REG_PWM1,
  REG_LS0,
  REG_LS1,
};

//: fixme fixme fixme fixme
//
//  Temp remap of  SERVER_POWER_RESET  to  power cycle for ipmid watchdog
//  Need to remove once pal.c adds  SERVER_POWER_RESET support
#define SERVER_POWER_RESET  SERVER_POWER_CYCLE

int pal_get_platform_name(char *name);
int pal_get_num_slots(uint8_t *num);
int pal_is_fru_prsnt(uint8_t fru, uint8_t *status);
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
int pal_set_led(uint8_t led, uint8_t status);
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
int pal_sensor_read(uint8_t fru, uint8_t sensor_num, void *value);
int pal_sensor_read_raw(uint8_t fru, uint8_t sensor_num, void *value);
int pal_sensor_threshold_flag(uint8_t fru, uint8_t snr_num, uint16_t *flag);
int pal_get_sensor_name(uint8_t fru, uint8_t sensor_num, char *name);
int pal_get_sensor_threshold(uint8_t fru, uint8_t sensor_num, uint8_t thresh,
    void *value);
int pal_get_key_value(char *key, char *value);
int pal_set_key_value(char *key, char *value);
int pal_set_def_key_value();
void pal_dump_key_value(void);
int pal_get_fru_devtty(uint8_t fru, char *devtty);
int pal_get_last_pwr_state(uint8_t fru, char *state);
int pal_set_last_pwr_state(uint8_t fru, char *state);
int pal_get_sys_guid(uint8_t slot, char *guid);
int pal_set_sysfw_ver(uint8_t slot, uint8_t *ver);
int pal_get_sysfw_ver(uint8_t slot, uint8_t *ver);
int pal_fruid_write(uint8_t slot, char *path);
int pal_is_bmc_por(void);
int pal_sensor_discrete_check(uint8_t fru, uint8_t snr_num, char *snr_name,
    uint8_t o_val, uint8_t n_val);
int pal_get_event_sensor_name(uint8_t fru, uint8_t *sel, char *name);
int pal_parse_sel(uint8_t fru, uint8_t *sel,
    char *error_log);
int pal_sel_handler(uint8_t fru, uint8_t snr_num, uint8_t *event_data);
void msleep(int msec);
int pal_set_sensor_health(uint8_t fru, uint8_t value);
int pal_get_fru_health(uint8_t fru, uint8_t *value);
int pal_set_fan_speed(uint8_t fan, uint8_t pwm);
int pal_get_fan_speed(uint8_t fan, int *rpm);
int pal_get_fan_name(uint8_t num, char *name);
void pal_inform_bic_mode(uint8_t fru, uint8_t mode);
int pal_get_uart_chan_btn(uint8_t *status);
int pal_get_uart_chan(uint8_t *status);
int pal_set_uart_chan(uint8_t status);
void pal_update_ts_sled();
int pal_handle_dcmi(uint8_t fru, uint8_t *tbuf, uint8_t tlen, uint8_t *rbuf, uint8_t *rlen);
int pal_is_fru_ready(uint8_t fru, uint8_t *status);
int pal_reset_pcie_switch();
int pal_get_pwm_value(uint8_t fan_num, uint8_t *value);
int pal_set_fan_led(uint8_t num, uint8_t operation);
int pal_fan_dead_handle(int fan_num);
int pal_fan_recovered_handle(int fan_num);
int pal_peer_tray_detection(uint8_t *value);
int pal_self_tray_location(uint8_t *value);
int pal_peer_tray_location(uint8_t *value);
int pal_is_crashdump_ongoing(uint8_t slot);
int pal_reset_ssd_switch();
void pal_log_clear(char *fru);
int pal_get_airflow(float *airflow_cfm);

// dummy definition
int pal_get_platform_id(uint8_t *id);
int pal_get_board_rev_id(uint8_t *id);
int pal_get_mb_slot_id(uint8_t *id);
int pal_get_slot_cfg_id(uint8_t *id);
int pal_set_boot_order(uint8_t fru, uint8_t *boot);
int pal_get_boot_order(uint8_t fru, uint8_t *boot);
int pal_get_dev_guid(uint8_t fru, char *guid);
int pal_get_poss_pcie_config(uint8_t *pcie_config);
int pal_get_plat_sku_id(void);
void pal_sensor_assert_handle(uint8_t snr_num, float val, uint8_t thresh);
void pal_sensor_deassert_handle(uint8_t snr_num, float val, uint8_t thresh);
void pal_set_post_end(void);
void pal_post_end_chk(uint8_t *post_end_chk);
int pal_get_fw_info(unsigned char target, unsigned char* res, unsigned char* res_len);
void pal_err_code_enable(const uint8_t error_num);
void pal_err_code_disable(const uint8_t error_num);
int pal_read_error_code_file(uint8_t *error_code_arrray);
int pal_write_error_code_file(const uint8_t error_num, const bool status);
int pal_drive_status(const char* dev);
int pal_read_nvme_data(uint8_t slot_num, uint8_t cmd);
int pal_u2_flash_read_nvme_data(uint8_t slot_num, uint8_t cmd);
int pal_m2_flash_read_nvme_data(uint8_t slot_num, uint8_t cmd);
int pal_m2_read_nvme_data(uint8_t i2c_map, uint8_t m2_mux_chan, uint8_t cmd);
int pal_drive_health(const char* dev);
void pal_add_cri_sel(char *str);
void pal_i2c_crash_assert_handle(int i2c_bus_num);
void pal_i2c_crash_deassert_handle(int i2c_bus_num);
int pal_bmc_err_enable(void);
int pal_bmc_err_disable(void);

#ifdef __cplusplus
} // extern "C"
#endif

#endif /* __PAL_H__ */

