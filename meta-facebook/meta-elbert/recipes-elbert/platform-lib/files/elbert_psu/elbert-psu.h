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

#define UNLOCK_UPGRADE 0xf0
#define BOOT_FLAG 0xf1
#define DATA_TO_RAM 0xf2
#define DATA_TO_FLASH 0xf3
#define CRC_CHECK 0xf4

#define DELTA_NUM_HEADER_LINES 4
#define RESERVED_LINE_0 2
#define RESERVED_LINE_1 12

#define DELTA_PSU_BOOT_UNLOCKED_BOOTLOADER_MASK 0xC

/* define for LITEON PSU */
#define LITEON_MODEL "PS-2242-9A"
#define LITEON_ID_LEN 10
#define LITEON_HDR_LENGTH 32

#define LITEON_ISP_KEY 0xd1
#define LITEON_BOOT_KEY "Arst"
#define LITEON_BOOT_KEY_LEN 4
#define LITEON_CLEAR_FAULT 0x03
#define LITEON_ISP_STATUS 0xD2
#define LITEON_RESET_SEQ 0x01
#define LITEON_BOOT_ISP 0x02
#define LITEON_REBOOT_CMD 0x03
#define LITEON_ISP_WRITE 0xd4
#define LITEON_STATUS_CML 0x7E

#define CML_INV_CMD 0x80
#define CML_INV_DATA 0x40
#define CML_PEC_FAIL 0x20

#define LITEON_RESERVED 2

#define LITEON_BOOT_FLAG_STATUS_MASK 0x40
#define LITEON_WRITE_CHECKSUM_MASK 0x01

#define HEX_FILE_PREFIX_LEN 9
#define LITEON_NUM_HEADER_LINES 2

#define FAILURE_RETRIES 3


/* define for common PSU */
#define WRITE_PROTECT 0x10
#define MFR_MODEL 0x9a
#define BYTES_PER_LINE 16

/* Vendor-defined time delays, in ms */
#define UNLOCK_DELAY 5
#define BOOT_FLAG_DELAY 3000
#define HEADER_DELAY 5
#define POST_HEADER_DELAY 5000
#define PRIMARY_DELAY 25
#define SECONDARY_DELAY 5
#define FLASH_DELAY 1000
#define ISP_WRITE_DELAY 1
#define BOOT_DELAY 1000
#define LITEON_BOOT_FLAG_DELAY 50
#define BOOTLOADER_STATUS_DELAY 500
#define LITEON_HDR_TO_DATA_DELAY  1500


/* WRITE_PROTECT values */
#define ENABLE_ALL_CMDS 0x00
#define OPERATION_PAGE_ONLY 0x40

/* HexRecord */
#define HEX_LEN_IDX 1
#define HEX_ADDR_IDX 3
#define HEX_TYPE_IDX 7
#define HEX_DATA_IDX 9
#define HEX_TYPE_DATA 0
#define HEX_TYPE_EOF 1
#define HEX_TYPE_EXT 4

typedef struct _hex_record {
  uint8_t len;
  uint32_t addr;
  uint8_t type;
  uint8_t data[BYTES_PER_LINE];
  uint8_t checksum;
} hex_record;

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

typedef struct _liteon_hdr_t {
  uint16_t tot_blk_num;
  uint16_t sec_data_start;
  uint8_t check_fw_wait;
  uint8_t copy_fw_wait;
  uint8_t fw_id[LITEON_ID_LEN + 1];
  uint8_t update_region;
  uint8_t fw_major;
  uint8_t pri_fw_minor;
  uint8_t sec_fw_minor;
  uint8_t compatibility[3];
  uint16_t blk_size;
  uint16_t write_time;
} liteon_hdr_t;

enum { STOP, START };

enum { NORMAL_MODE, BOOT_MODE };

enum { READ, WRITE };

enum { DELTA_2400, LITEON_2400, UNKNOWN };

enum { PRIMARY, SECONDARY };

int is_psu_prsnt(uint8_t num, uint8_t* status);
int is_psu_power_ok(uint8_t num, uint8_t* status);
int get_mfr_model(uint8_t num, uint8_t* block);
int do_update_psu(uint8_t num, const char* file, const char* vendor);

#ifdef __cplusplus
}
#endif

#endif
