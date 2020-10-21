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
#include <facebook/fby3_common.h>
#include <facebook/bic.h>
#include <facebook/fby3_fruid.h>
#include "pal_power.h"
#include "pal_sensors.h"
#ifdef __cplusplus
extern "C" {
#endif

#define PWR_OPTION_LIST "status, graceful-shutdown, off, on, reset, cycle, 12V-on, 12V-off, 12V-cycle"

#define LARGEST_DEVICE_NAME (120)
#define UNIT_DIV            (1000)
#define ERR_NOT_READY       (-2)

#define CUSTOM_FRU_LIST 1
#define FRU_DEVICE_LIST 1
#define GUID_FRU_LIST 1

#define MAX_READ_RETRY 5
#define CPLD_INTENT_CTRL_ADDR 0x70
#define NIC_CPLD_BUS 9
#define BB_CPLD_BUS 12
#define SLOT_BUS_BASE 3

#define NIC_CARD_PERST_CTRL 0x16

// Baseboard PFR
#define CPLD_UPDATE_ADDR (0x40)
#define UFM_PROVISIONED_MSK 0x20
#define INTENT_UPDATE_AT_RESET_MASK 0x80

#define PFR_I2C_FILTER_OFFSET 0x10
#define DISABLE_PFR_I2C_FILTER 0
#define ENABLE_PFR_I2C_FILTER 1

#define MAX_ERR_LOG_SIZE 256
#define BIOS_CAP_VER_OFFSET 0x80C
#define BIOS_CAP_VER_LEN 16
#define CPLD_CAP_VER_OFFSET 0x404
#define CPLD_CAP_VER_LEN 4

#define SET_NIC_PWR_MODE_LOCK "/var/run/set_nic_power.lock"

#define MAX_SNR_NAME 32

extern const char pal_fru_list_print[];
extern const char pal_fru_list_rw[];
extern const char pal_fru_list_sensor_history[];

extern size_t pal_pwm_cnt;
extern size_t pal_tach_cnt;
extern const char pal_pwm_list[];
extern const char pal_tach_list[];
extern const char pal_fru_list[];
extern const char pal_guid_fru_list[];
extern const char pal_server_list[];
extern const char pal_dev_list[];
extern const char pal_dev_pwr_option_list[];
extern const char pal_fan_opt_list[];

enum {
  FAN_0 = 0,
  FAN_1,
  FAN_2,
  FAN_3,
  FAN_4,
  FAN_5,
  FAN_6,
  FAN_7,
};

enum {
  PWM_0 = 0,
  PWM_1,
  PWM_2,
  PWM_3,
};

enum {
  CONFIG_A = 0x01,
  CONFIG_B = 0x02,
  CONFIG_C = 0x03,
  CONFIG_D = 0x04,
  CONFIG_B_E1S = 0x05,
  CONFIG_UNKNOWN = 0xff,
};

enum {
  AC_OFF = 0x00,
  AC_ON  = 0x01,
};

enum {
  MANUAL_MODE  = 0x00,
  AUTO_MODE    = 0x01,
  GET_FAN_MODE = 0x02,
};

#define MAX_NODES     (4)
#define READING_SKIP  (1)
#define READING_NA    (-2)

enum {
  BOOT_DEVICE_IPV4     = 0x1,
  BOOT_DEVICE_HDD      = 0x2,
  BOOT_DEVICE_CDROM    = 0x3,
  BOOT_DEVICE_OTHERS   = 0x4,
  BOOT_DEVICE_IPV6     = 0x9,
  BOOT_DEVICE_RESERVED = 0xff,
};

typedef struct {
    uint8_t bus_value;
    uint8_t dev_value;
    uint8_t root_port;
    char *silk_screen;
    char *location;
} MAPTOSTRING;

typedef struct {
    int err_id;;
    char *err_descr;
} PCIE_ERR_DECODE;

enum {
  PLATFORM_STATE_OFFSET = 0x03,
  RCVY_CNT_OFFSET       = 0x04,
  LAST_RCVY_OFFSET      = 0x05,
  PANIC_CNT_OFFSET      = 0x06,
  LAST_PANIC_OFFSET     = 0x07,
  MAJOR_ERR_OFFSET      = 0x08,
  MINOR_ERR_OFFSET      = 0x09,
  UFM_STATUS_OFFSET     = 0x0A,
};

enum {
  MAJOR_ERROR_BMC_AUTH_FAILED=0x01,
  MAJOR_ERROR_PCH_AUTH_FAILED=0x02,
  MAJOR_ERROR_UPDATE_FROM_PCH_FAILED=0x03,
  MAJOR_ERROR_UPDATE_FROM_BMC_FAILED=0x04,
};

enum {
  BIOS_CAP_STAG_MAILBOX = 0xE0,
  BIOS_CAP_RCVY_MAILBOX = 0xD0,
  CPLD_CAP_STAG_MAILBOX = 0x64,
  CPLD_CAP_RCVY_MAILBOX = 0x60,
};

enum {
  NIC_PE_RST_LOW   = 0x00,
  NIC_PE_RST_HIGH  = 0x01,
};

typedef struct {
  uint8_t err_id;
  char *err_des;
} err_t;

extern size_t minor_auth_size;
extern size_t minor_update_size;
extern err_t minor_auth_error[];
extern err_t minor_update_error[];

int pal_is_fru_prsnt(uint8_t fru, uint8_t *status);
int pal_get_uart_select_from_cpld(uint8_t *uart_select);
int pal_handle_dcmi(uint8_t fru, uint8_t *tbuf, uint8_t tlen, uint8_t *rbuf, uint8_t *rlen);
int pal_bypass_cmd(uint8_t slot, uint8_t *req_data, uint8_t req_len, uint8_t *res_data, uint8_t *res_len);
int pal_check_pfr_mailbox(uint8_t fru);
int set_pfr_i2c_filter(uint8_t slot_id, uint8_t value);
int pal_check_sled_mgmt_cbl_id(uint8_t slot_id, uint8_t *cbl_val, bool log_evnt, uint8_t bmc_location);
int pal_parse_oem_sel(uint8_t fru, uint8_t *sel, char *error_log);
int pal_get_fw_info(uint8_t fru, unsigned char target, unsigned char* res, unsigned char* res_len);
int pal_set_nic_perst(uint8_t fru, uint8_t val);
int pal_is_slot_pfr_active(uint8_t fru);
int pal_sb_set_amber_led(uint8_t fru, bool led_on);
int pal_set_uart_IO_sts(uint8_t slot_id, uint8_t io_sts);
int pal_is_debug_card_prsnt(uint8_t *status);
#ifdef __cplusplus
} // extern "C"
#endif

#endif /* __PAL_H__ */
