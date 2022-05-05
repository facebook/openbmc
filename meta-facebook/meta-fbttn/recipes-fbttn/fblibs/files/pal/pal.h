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
#include <facebook/exp.h>
#include <facebook/mctp.h>
#include <facebook/fbttn_common.h>
#include <facebook/fbttn_fruid.h>
#include <facebook/fbttn_sensor.h>
#include <openbmc/nvme-mi.h>

#define MAX_NUM_FAN     2

#define BIC_READY 0           // BIC ready: 0; BIC NOT ready: 1
#define BIC_NOT_READY 1

#define SERVER_PWR_ON_LOCK "/var/run/server%d_power_on.lock"

#define MAX_NODES 4

#define MAX_ERROR_CODES 256

//Expander
#define SCC_FIRST_SENSOR_NUM 96 //Expander_TEMP 0x60
#define DPB_FIRST_SENSOR_NUM 24 //P3V3_SENSE  0x18
#define MAX_EXP_IPMB_SENSOR_COUNT 40

#define ERROR_CODE_NUM 32

#define HB_INTERVAL 50

#define NIC_TEMP_RETRY 3

// For I2C bus crash error code
#define I2C_BUS_MAX_NUMBER 14
#define ERR_CODE_I2C_CRASH_BASE 0xE9

// For BMC health error code
#define ERR_CODE_CPU 0xE0
#define ERR_CODE_MEM 0xE1
#define ERR_CODE_ECC_RECOVERABLE 0xE2
#define ERR_CODE_ECC_UNRECOVERABLE 0xE3

typedef struct {
  uint8_t code;
  bool status;
} error_code;

extern char * key_list[];
extern size_t pal_pwm_cnt;
extern size_t pal_tach_cnt;
extern const char pal_pwm_list[];
extern const char pal_tach_list[];
extern const char pal_fru_list[];
extern const char pal_server_list[];

#define CUSTOM_FRU_LIST 1
extern const char pal_fru_list_print[];
extern const char pal_fru_list_rw[];
extern const char pal_fru_list_sensor_history[];

extern unsigned char g_err_code[ERROR_CODE_NUM];

enum {
  USB_MUX_OFF,
  USB_MUX_ON,
};

enum {
  TARGET_BIOS_VER = 0,
  TARGET_CPLD_VER,
  TARGET_BIC_VER,
  TARGET_ME_VER,
  TARGET_VR_PVCCIN_VER,
  TARGET_VR_PVDDR_VER,
  TARGET_VR_P1V05_VER,
  TARGET_VR_PVCCGBE_VER,
  TARGET_VR_PVCCSCUS_VER,
};

enum {
  UART_SEL_SERVER = 0,
  UART_SEL_BMC = 1,
};

// Distinguish between FRU ID, it means the debug card only for front-paneld controlling
enum {
  HAND_SW_SERVER = 10,
  HAND_SW_BMC = 11,
};

//Event/Reading Type Code Ranges
enum {
  GENERIC = 0x5,
  SENSOR_SPECIFIC = 0x6F,
};

//Generic Event/Reading Type Codes
enum {
  DIGITAL_DISCRETE = 0x5,
};


enum {
  BIC_MODE_NORMAL = 0x01,
  BIC_MODE_UPDATE = 0x0F,
};

enum {
  FAN_0 = 0,
  FAN_1,
};

enum {
  IOM_SIDEA = 1,
  IOM_SIDEB = 2,
};

enum {
  PICE_CONFIG_TYPE5 = 0x6,
  PICE_CONFIG_TYPE7 = 0x8,
};

enum {
  PLAT_INFO_SKUID_TYPE5A = 2,
  PLAT_INFO_SKUID_TYPE5B = 3,
  PLAT_INFO_SKUID_TYPE7SS = 4,
};

enum SAS_EXT_PORT {
  SAS_EXT_PORT_1 = 0,
  SAS_EXT_PORT_2,
};

enum {
  IOM_LED_OFF,
  IOM_LED_YELLOW,
  IOM_LED_BLUE,
};

enum {
  BOARD_EVT = 0,
  BOARD_DVT,
  BOARD_MP,
};

//fw-util FW Updating Flag File
#define FW_UPDATE_FLAG "/tmp/fw_update_flag"

#define POST_CODE_FILE "/tmp/post_code_buffer.bin"
#define LAST_POST_CODE_FILE "/tmp/last_post_code_buffer.bin"

int pal_is_debug_card_prsnt(uint8_t *status);
int pal_get_uart_sel_pos(uint8_t *pos);
int pal_switch_usb_mux(uint8_t slot);
int pal_post_enable(uint8_t slot);
int pal_post_disable(uint8_t slot);
int pal_post_get_last(uint8_t slot, uint8_t *post);
int pal_get_dbg_pwr_btn(uint8_t *status);
int pal_get_dbg_rst_btn(uint8_t *status);
int pal_get_pwr_btn(uint8_t *status);
int pal_get_rst_btn(uint8_t *status);
int pal_set_id_led(uint8_t slot, uint8_t status);
int pal_get_fru_sdr_path(uint8_t fru, char *path);
int pal_sensor_sdr_init(uint8_t fru, sensor_info_t *sinfo);
int pal_get_platform_id(uint8_t *id);
int pal_get_board_rev_id(uint8_t *id);
int pal_get_mb_slot_id(uint8_t *id);
int pal_get_slot_cfg_id(uint8_t *id);
int pal_get_sku(void);
int pal_get_iom_location(void);
int pal_get_iom_type(void);
int pal_is_scc_stb_pwrgood(void);
int pal_is_scc_full_pwrgood(void);
int pal_is_iom_full_pwrgood(void);
int pal_en_scc_stb_pwr(void);
int pal_en_scc_full_pwr(void);
int pal_en_iom_full_pwr(void);
int pal_fault_led_mode(uint8_t state, uint8_t mode);
int pal_fault_led_behavior(uint8_t state);
int pal_minisas_led(uint8_t port, uint8_t operation);
int pal_minisas_led(uint8_t port, uint8_t state);
int pal_expander_sensor_check(uint8_t fru, uint8_t sensor_num);
int pal_exp_scc_read_sensor_wrapper(uint8_t fru, uint8_t *sensor_list, int sensor_cnt, uint8_t sensor_num);
int pal_exp_dpb_read_sensor_wrapper(uint8_t fru, uint8_t *sensor_list, int sensor_cnt, uint8_t sensor_num, int second_transaction);
int pal_get_bmc_rmt_hb(void);
int pal_get_scc_loc_hb(void);
int pal_get_scc_rmt_hb(void);
void pal_err_code_enable(unsigned char num);
void pal_err_code_disable(unsigned char num);
uint8_t pal_read_error_code_file(uint8_t *error_code_arrray);
uint8_t pal_write_error_code_file(error_code *update);
unsigned char pal_sum_error_code(void);
int pal_get_error_code(uint8_t data[MAX_ERROR_CODES], uint8_t* error_count);
int pal_post_get_buffer(uint8_t *buffer, uint8_t *buf_len);
int pal_nic_otp(int fru, int snr_num, float thresh_val);
void pal_get_chassis_status(uint8_t slot, uint8_t *req_data, uint8_t *res_data, uint8_t *res_len);
int set_gpio_value(int gpio_num, uint8_t value);
int get_gpio_value(int gpio_num, uint8_t *value);
uint8_t pal_iom_led_control(uint8_t color);
int server_power_reset(uint8_t slot_id);
int pal_get_iom_board_id (void);
int pal_get_edb_value(char *key, char *value);
int pal_set_edb_value(char *key, char *value);
int pal_powering_on_flag(uint8_t slot_id);
void pal_rm_powering_on_flag(uint8_t slot_id);
int pal_is_bic_ready(uint8_t slot_id, uint8_t *status);
int pal_get_iom_ioc_ver(uint8_t *ver);
void pal_power_policy_control(uint8_t fru, char *last_ps);


#ifdef __cplusplus
} // extern "C"
#endif

#endif /* __PAL_H__ */
