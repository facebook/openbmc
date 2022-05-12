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

#include <syslog.h>
#include <openbmc/obmc-pal.h>
#include <facebook/fby35_common.h>
#include <facebook/fby35_fruid.h>
#include <facebook/bic_ipmi.h>
#include <facebook/bic_xfer.h>
#include <facebook/bic_power.h>
#include "pal_power.h"
#include "pal_sensors.h"
#ifdef __cplusplus
extern "C" {
#endif

#define PWR_OPTION_LIST "status, graceful-shutdown, off, on, reset, cycle, 12V-on, 12V-off, 12V-cycle"

#define CUSTOM_FRU_LIST 1
#define FRU_DEVICE_LIST 1
#define GUID_FRU_LIST 1

#define ERR_NOT_READY  (-2)
#define MAX_READ_RETRY 5

#define NIC_CARD_PERST_CTRL 0x16

#define MAX_ERR_LOG_SIZE 256

#define SET_NIC_PWR_MODE_LOCK "/var/run/set_nic_power.lock"
#define PWR_UTL_LOCK "/var/run/power-util_%d.lock"

#define MAX_SNR_NAME 32
#define STR_VALUE_0  "0"
#define STR_VALUE_1  "1"

extern const char pal_fru_list_print[];
extern const char pal_fru_list_rw[];
extern const char pal_fru_list_sensor_history[];

extern size_t pal_pwm_cnt;
extern size_t pal_tach_cnt;
extern const char pal_pwm_list[];
extern const char pal_fru_list[];
extern const char pal_guid_fru_list[];
extern const char pal_server_list[];
extern const char pal_dev_fru_list[];
extern const char pal_dev_pwr_list[];
extern const char pal_dev_pwr_option_list[];
extern const char pal_fan_opt_list[];

enum {
  LED_LOCATE_MODE = 0x0,
  LED_CRIT_PWR_OFF_MODE,
  LED_CRIT_PWR_ON_MODE,
};

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
  CONFIG_A          = 0x01,
  CONFIG_B          = 0x02,
  CONFIG_C          = 0x03,
  CONFIG_D          = 0x04,
  CONFIG_B_DPV2_X16 = 0x08,
  CONFIG_B_DPV2_X8  = 0x09,
  CONFIG_B_DPV2_X4  = 0x0a,
  CONFIG_C_WF       = 0x0b,
  CONFIG_C_VF       = 0x0c,
  CONFIG_MFG        = 0xfe,
  CONFIG_UNKNOWN    = 0xff,
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

typedef struct {
    uint8_t fru;
    uint8_t component;
} GET_FW_VER_REQ;

enum {
  PCH_BIC_PWR_FAULT_OFFSET = 0x09,
  CPU_PWR_FAULT_OFFSET = 0x0A,
};

enum {
  NIC_PE_RST_LOW   = 0x00,
  NIC_PE_RST_HIGH  = 0x01,
};

enum {
  DEV_FRU_NOT_COMPLETE,
  DEV_FRU_COMPLETE,
  DEV_FRU_IGNORE,
};

typedef struct {
  uint8_t err_id;
  char *err_des;
} err_t;

int pal_get_uart_select_from_cpld(uint8_t *uart_select);
int pal_get_uart_select_from_kv(uint8_t *uart_select);
int pal_get_cpld_ver(uint8_t fru, uint8_t *ver);
int pal_check_sled_mgmt_cbl_id(uint8_t slot_id, uint8_t *cbl_val, uint8_t bmc_location);
int pal_set_nic_perst(uint8_t fru, uint8_t val);
int pal_sb_set_amber_led(uint8_t fru, bool led_on, uint8_t led_mode);
int pal_set_uart_IO_sts(uint8_t slot_id, uint8_t io_sts);
int pal_is_debug_card_prsnt(uint8_t *status);
int pal_get_dev_info(uint8_t slot_id, uint8_t dev_id, uint8_t *nvme_ready, uint8_t *status, uint8_t *type);
int pal_check_slot_cpu_present(uint8_t slot_id);
int pal_gpv3_mux_select(uint8_t slot_id, uint8_t dev_id);
int pal_check_slot_fru(uint8_t slot_id);
int pal_clear_cmos(uint8_t slot_id);
int pal_is_cable_connect_baseborad(uint8_t slot_id, uint16_t curr);

#ifdef __cplusplus
} // extern "C"
#endif

#endif /* __PAL_H__ */
