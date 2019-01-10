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

#include <facebook/lightning_common.h>
#include <facebook/lightning_fruid.h>
#include <facebook/lightning_sensor.h>
#include <facebook/lightning_flash.h>
#include <facebook/lightning_gpio.h>
#include <openbmc/kv.h>
#include <openbmc/nvme-mi.h>

#define MAX_RETRY       5

#define FRU_STATUS_GOOD   1
#define FRU_STATUS_BAD    0

#define MAX_NODES 1
#define MAX_FAN_LED_NUM 6
#define FAN_INDEX_BASE 1

#define ERROR_CODE_NUM 13
#define ERR_CODE_FILE "/tmp/error_code"
#define SENSOR_STATUS_FILE "/tmp/share_sensor_status"
#define SKIP_READ_SSD_TEMP "/tmp/skipSSD"

#define CMD_DRIVE_STATUS 0
#define CMD_DRIVE_HEALTH 1

#define ERR_CODE_I2C_CRASH_BASE 0x02
#define ERR_CODE_SELF_TRAY_PULL_OUT 0x5D
#define ERR_CODE_PEER_TRAY_PULL_OUT 0x5E
#define ERR_CODE_CPU 0x5F
#define ERR_CODE_MEM 0x60

#define HB_INTERVAL 500

#define ERR_ASSERT 1
#define ERR_DEASSERT 0

#define I2C_BUS_MAX_NUMBER 14

#define BMC_HEALTH 4      // for bmc_health key

#define CPU_THRESHOLD 80
#define MEM_THRESHOLD 70

#define MAX_NUM_SLOTS 0

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
  UART_POS_BMC = 0,
  UART_POS_PCIE_SW,
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
int pal_post_get_last(uint8_t slot, uint8_t *post);
int pal_post_handle(uint8_t slot, uint8_t status);
int pal_get_rst_btn(uint8_t *status);
int pal_set_led(uint8_t led, uint8_t status);
int pal_set_hb_led(uint8_t status);
int pal_get_fru_list(char *list);
int pal_get_fru_id(char *fru_str, uint8_t *fru);
int pal_get_fru_name(uint8_t fru, char *name);
int pal_get_fruid_path(uint8_t fru, char *path);
int pal_get_fruid_eeprom_path(uint8_t fru, char *path);
int pal_get_fruid_name(uint8_t fru, char *name);
int pal_get_sensor_units(uint8_t fru, uint8_t sensor_num, char *units);
int pal_get_fru_sensor_list(uint8_t fru, uint8_t **sensor_list, int *cnt);
int pal_sensor_sdr_init(uint8_t fru, sensor_info_t *sinfo);
int pal_sensor_read_raw(uint8_t fru, uint8_t sensor_num, void *value);
int pal_get_sensor_name(uint8_t fru, uint8_t sensor_num, char *name);
int pal_get_sensor_threshold(uint8_t fru, uint8_t sensor_num, uint8_t thresh,
    void *value);
int pal_get_key_value(char *key, char *value);
int pal_set_key_value(char *key, char *value);
int pal_set_def_key_value();
void pal_dump_key_value(void);
void msleep(int msec);
int pal_set_sensor_health(uint8_t fru, uint8_t value);
int pal_get_fru_health(uint8_t fru, uint8_t *value);
int pal_set_fan_speed(uint8_t fan, uint8_t pwm);
int pal_get_fan_speed(uint8_t fan, int *rpm);
int pal_get_fan_name(uint8_t num, char *name);
int pal_get_uart_chan_btn(uint8_t *status);
int pal_get_uart_chan(uint8_t *status);
int pal_set_uart_chan(uint8_t status);
int pal_is_fru_ready(uint8_t fru, uint8_t *status);
int pal_reset_pcie_switch();
int pal_get_pwm_value(uint8_t fan_num, uint8_t *value);
int pal_set_fan_led(uint8_t num, uint8_t operation);
int pal_fan_dead_handle(int fan_num);
int pal_fan_recovered_handle(int fan_num);
int pal_self_tray_location(uint8_t *value);
int pal_self_tray_insertion(uint8_t *value);
int pal_peer_tray_insertion(uint8_t *value);
int pal_get_tray_location(char *self_name, uint8_t self_len, char *peer_name, uint8_t peer_len, uint8_t *peer_tray_pwr);
void pal_log_clear(char *fru);
void pal_sensor_assert_handle(uint8_t fru, uint8_t snr_num, float val, uint8_t thresh);
void pal_sensor_deassert_handle(uint8_t fru, uint8_t snr_num, float val, uint8_t thresh);
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
void pal_i2c_crash_assert_handle(int i2c_bus_num);
void pal_i2c_crash_deassert_handle(int i2c_bus_num);
uint8_t pal_get_status(void);
int pal_bmc_err_enable(const char *error_item);
int pal_bmc_err_disable(const char *error_item);
int pal_set_debug_card_led(const int display_num);

#ifdef __cplusplus
} // extern "C"
#endif

#endif /* __PAL_H__ */
