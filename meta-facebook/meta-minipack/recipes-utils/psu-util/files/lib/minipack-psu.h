/*
 * Copyright 2018-present Facebook. All Rights Reserved.
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

#ifndef __MINIPACK_PSU_H__
#define __MINIPACK_PSU_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <openbmc/obmc-i2c.h>
#include <openbmc/obmc-pal.h>
#include <openbmc/fruid.h>

#define ERR_PRINT(fmt, args...) \
        fprintf(stderr, fmt ": %s\n", ##args, strerror(errno));

#define DRIVER_ADD   "echo %s %d > /sys/class/i2c-dev/i2c-%d/device/new_device"
#define DRIVER_DEL   "echo %d > /sys/class/i2c-dev/i2c-%d/device/delete_device"
#define PSU1_EEPROM  "/sys/class/i2c-adapter/i2c-49/49-0051/eeprom"
#define PSU2_EEPROM  "/sys/class/i2c-adapter/i2c-48/48-0050/eeprom"
#define PSU3_EEPROM  "/sys/class/i2c-adapter/i2c-57/57-0051/eeprom"
#define PSU4_EEPROM  "/sys/class/i2c-adapter/i2c-56/56-0050/eeprom"

typedef struct _i2c_info_t {
  uint8_t bus;
  uint8_t eeprom_addr;
  uint8_t pmbus_addr;
  const char *eeprom_file;
} i2c_info_t;

typedef struct _pmbus_info_t {
  const char *item;
  uint8_t reg;
} pmbus_info_t;

int get_mfr_model(int fd, u_int8_t reg, u_int8_t *block);

#ifdef __cplusplus
}
#endif

#endif
