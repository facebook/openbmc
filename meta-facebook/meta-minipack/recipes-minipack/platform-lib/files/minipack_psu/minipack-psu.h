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
#include <openbmc/pal.h>
#include <openbmc/fruid.h>

#define ERR_PRINT(fmt, args...) \
        fprintf(stderr, fmt ": %s\n", ##args, strerror(errno));

#define msleep(n) usleep(n*1000)
#define POW2(x) (1 << (x))

#define PSU1_EEPROM  "/sys/class/i2c-adapter/i2c-49/49-0051/eeprom"
#define PSU2_EEPROM  "/sys/class/i2c-adapter/i2c-48/48-0050/eeprom"
#define PSU3_EEPROM  "/sys/class/i2c-adapter/i2c-57/57-0051/eeprom"
#define PSU4_EEPROM  "/sys/class/i2c-adapter/i2c-56/56-0050/eeprom"

/* define for DELTA PSU */
#define DELTA_MODEL         "ECD55020006"
#define DELTA_HDR_LENGTH    32
#define UNLOCK_UPGRADE      0xf0
#define BOOT_FLAG           0xf1
#define DATA_TO_RAM         0xf2
#define DATA_TO_FLASH       0xf3
#define CRC_CHECK           0xf4

#define NORMAL_MODE         0x00
#define BOOT_MODE           0x01

#define DELTA_PRI_NUM_OF_BLOCK    32
#define DELTA_PRI_NUM_OF_PAGE     44
#define DELTA_PRI_PAGE_START      16
#define DELTA_PRI_PAGE_END        59
#define DELTA_SEC_NUM_OF_BLOCK    16
#define DELTA_SEC_NUM_OF_PAGE     232
#define DELTA_SEC_PAGE_START      48
#define DELTA_SEC_PAGE_END        279

/* define for BELPOWER PSU */
#define BELPOWER_MODEL      "PFE1500-12-054NACS457"

/* define for MURATA PSU */
#define MURATA_MODEL        "D1U54P-W-1500-12-HC4TC-AF"

typedef struct _i2c_info_t {
  int fd;
  uint8_t bus;
  uint8_t eeprom_addr;
  uint8_t pmbus_addr;
  const char *eeprom_file;
} i2c_info_t;

typedef struct _pmbus_info_t {
  const char *item;
  uint8_t reg;
} pmbus_info_t;

typedef struct _time_info_t {
  uint32_t day;
  uint8_t hour;
  uint8_t min;
  uint8_t sec;
} time_info_t;

typedef struct _blackbox_info_t {
  uint8_t len;
  uint8_t page;
  uint8_t v1_status[2];
  uint8_t v1_status_vout;
  uint8_t v1_status_iout;
  uint8_t vsb_status[2];
  uint8_t vsb_status_vout;
  uint8_t vsb_status_iout;
  uint8_t input_status;
  uint8_t temp_status;
  uint8_t cml_status;
  uint8_t fan_status;
  uint8_t vin[2];
  uint8_t iin[2];
  uint8_t vout[2];
  uint8_t iout[2];
  uint8_t temp1[2];
  uint8_t temp2[2];
  uint8_t temp3[2];
  uint8_t fan_speed[2];
  uint8_t pri_code_ver[2];
  uint8_t sec_code_ver[2];
  uint8_t optn_time_total[4];
  uint8_t optn_time_present[4];
} blackbox_info_t;

typedef struct _delta_hdr_t {
  uint8_t crc[2];
  uint16_t page_start;
  uint16_t page_end;
  uint16_t byte_per_blk;
  uint16_t blk_per_page;
  uint8_t uc;
  uint8_t app_fw_major;
  uint8_t app_fw_minor;
  uint8_t bl_fw_major;
  uint8_t bl_fw_minor;
  uint8_t fw_id_len;
  uint8_t fw_id[16];
  uint8_t compatibility;
} delta_hdr_t;

typedef struct _murata_hdr_t {
  uint8_t boot_addr;
  uint8_t uc;
  uint8_t unlock[4];
} murata_hdr_t;

enum {
  STOP,
  START
};

enum {
  DELTA_1500,
  BELPOWER_1500_NAC,
  MURATA_1500,
  UNKNOWN
};

enum {
  MFR_ID = 0,
  MFR_MODEL = 1,
  MFR_REVISION = 2,
  MFR_DATE = 3,
  MFR_SERIAL = 4,
  PRI_FW_VER = 5,
  SEC_FW_VER = 6,
  STATUS_WORD = 7,
  STATUS_VOUT = 8,
  STATUS_IOUT = 9,
  STATUS_INPUT = 10,
  STATUS_TEMP = 11,
  STATUS_CML = 12,
  STATUS_FAN = 13,
  STATUS_STBY_WORD = 14,
  STATUS_VSTBY = 15,
  STATUS_ISTBY = 16,
  OPTN_TIME_TOTAL = 17,
  OPTN_TIME_PRESENT = 18
};

int is_psu_prsnt(uint8_t num, uint8_t *status);
int get_mfr_model(uint8_t num, uint8_t *block);
int do_update_psu(uint8_t num, const char *file, const char *vendor);
int get_eeprom_info(uint8_t mum, const char *tpye);
int get_psu_info(uint8_t num);
int get_blackbox_info(uint8_t num, const char *option);

#ifdef __cplusplus
}
#endif

#endif
