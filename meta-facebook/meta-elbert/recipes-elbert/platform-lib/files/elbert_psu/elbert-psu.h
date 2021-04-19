/*
 * Copyright 2021-present Facebook. All Rights Reserved.
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

#ifndef __ELBERT_PSU_H__
#define __ELBERT_PSU_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <errno.h>
#include <fcntl.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <syslog.h>
#include <unistd.h>

#define ERR_PRINT(fmt, args...) \
  fprintf(stderr, fmt ": %s\n", ##args, strerror(errno));

#define msleep(n) usleep(n * 1000)

#define UPDATE_SKIP 10

/* define for DELTA PSU */
#define DELTA_MODEL "ECD15020056"
#define DELTA_ID_LEN 11
#define DELTA_HDR_LENGTH 64

#define WRITE_PROTECT 0x10
#define MFR_MODEL 0x9a
#define UNLOCK_UPGRADE 0xf0
#define BOOT_FLAG 0xf1
#define DATA_TO_RAM 0xf2
#define DATA_TO_FLASH 0xf3
#define CRC_CHECK 0xf4

#define NUM_HEADER_LINES 4
#define BYTES_PER_LINE 16
#define RESERVED_LINE_0 2
#define RESERVED_LINE_1 12

/* Vendor-defined time delays, in ms */
#define UNLOCK_DELAY 5
#define BOOT_FLAG_DELAY 3000
#define HEADER_DELAY 5
#define POST_HEADER_DELAY 5000
#define PRIMARY_DELAY 25
#define SECONDARY_DELAY 5
#define FLASH_DELAY 1000

#define ENABLE_ALL_CMDS 0x00

typedef struct _i2c_info_t {
  int fd;
  uint8_t bus;
  uint8_t pmbus_addr;
} i2c_info_t;

typedef struct _delta_hdr_t {
  uint8_t compatibility;
  uint16_t sec_data_start;
  uint8_t pri_fw_major;
  uint8_t pri_fw_minor;
  uint8_t pri_crc[2];
  uint8_t sec_fw_major;
  uint8_t sec_fw_minor;
  uint8_t sec_crc[2];
  uint8_t fw_id[DELTA_ID_LEN + 1];
} delta_hdr_t;

enum { STOP, START };

enum { NORMAL_MODE, BOOT_MODE };

enum { READ, WRITE };

enum { DELTA_2400, UNKNOWN };

enum { PRIMARY, SECONDARY };

int is_psu_prsnt(uint8_t num, uint8_t* status);
int is_psu_power_ok(uint8_t num, uint8_t* status);
int get_mfr_model(uint8_t num, uint8_t* block);
int do_update_psu(uint8_t num, const char* file, const char* vendor);

#ifdef __cplusplus
}
#endif

#endif
