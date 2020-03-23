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
    int bus_value;
    int dev_value;
    char *silk_screen;
    char *location;
} MAPTOSTRING;

int pal_is_fru_prsnt(uint8_t fru, uint8_t *status);
int pal_get_uart_select_from_cpld(uint8_t *uart_select);
int pal_handle_dcmi(uint8_t fru, uint8_t *tbuf, uint8_t tlen, uint8_t *rbuf, uint8_t *rlen);

#ifdef __cplusplus
} // extern "C"
#endif

#endif /* __PAL_H__ */
