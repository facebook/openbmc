/*
 *
 * Copyright 2017-present Facebook. All Rights Reserved.
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

#include <openbmc/kv.h>
#include <openbmc/ipmi.h>
#include <stdbool.h>

#define BIT(value, index) ((value >> index) & 1)

#define MAX_NODES     2
#define MAX_NUM_FRUS  2
#define LAST_KEY "last_key"

#define WEDGE100_SYSCPLD_RESTART_CAUSE "/sys/bus/i2c/drivers/syscpld/12-0031/reset_reason"
#define FRU_EEPROM "/sys/class/i2c-adapter/i2c-6/6-0051/eeprom"
#define PAGE_SIZE  0x1000

#define W100_PLATFORM_NAME "wedge100"
#define LAST_KEY "last_key"
#define W100_MAX_NUM_SLOTS 1

enum {
  SERVER_POWER_OFF,
  SERVER_POWER_ON,
  SERVER_POWER_CYCLE,
  SERVER_POWER_RESET,
  SERVER_GRACEFUL_SHUTDOWN,
  SERVER_12V_OFF,
  SERVER_12V_ON,
  SERVER_12V_CYCLE,
};

enum {
  FRU_ALL   = 0,
  FRU_MB = 1,
};

typedef struct _sensor_info_t {
  bool valid;
  sdr_full_t sdr;
} sensor_info_t;

#ifdef __cplusplus
} // extern "C"
#endif

#endif /* __PAL_H__ */
