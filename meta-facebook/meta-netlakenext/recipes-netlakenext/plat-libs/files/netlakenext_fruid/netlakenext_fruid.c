/* Copyright 2020-present Facebook. All Rights Reserved.
 *
 * This file contains code to support IPMI2.0 Specificaton available @
 * http://www.intel.com/content/www/us/en/servers/ipmi/ipmi-specifications.html
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

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <syslog.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <facebook/netlakenext_common.h>
#include "netlakenext_fruid.h"

/**
*  Cloned from the lib/crc-ccitt.c. Used for CRC variants with polynomial 0x1021 
*  and no bit reflection (MSB first), such as CRC-CCITT-FALSE and CRC-CCITT-AUG.
**/
static const uint16_t crc_ccitt_aug_table[256] = {
    0x0000, 0x1021, 0x2042, 0x3063, 0x4084, 0x50A5, 0x60C6, 0x70E7,
    0x8108, 0x9129, 0xA14A, 0xB16B, 0xC18C, 0xD1AD, 0xE1CE, 0xF1EF,
    0x1231, 0x0210, 0x3273, 0x2252, 0x52B5, 0x4294, 0x72F7, 0x62D6,
    0x9339, 0x8318, 0xB37B, 0xA35A, 0xD3BD, 0xC39C, 0xF3FF, 0xE3DE,
    0x2462, 0x3443, 0x0420, 0x1401, 0x64E6, 0x74C7, 0x44A4, 0x5485,
    0xA56A, 0xB54B, 0x8528, 0x9509, 0xE5EE, 0xF5CF, 0xC5AC, 0xD58D,
    0x3653, 0x2672, 0x1611, 0x0630, 0x76D7, 0x66F6, 0x5695, 0x46B4,
    0xB75B, 0xA77A, 0x9719, 0x8738, 0xF7DF, 0xE7FE, 0xD79D, 0xC7BC,
    0x48C4, 0x58E5, 0x6886, 0x78A7, 0x0840, 0x1861, 0x2802, 0x3823,
    0xC9CC, 0xD9ED, 0xE98E, 0xF9AF, 0x8948, 0x9969, 0xA90A, 0xB92B,
    0x5AF5, 0x4AD4, 0x7AB7, 0x6A96, 0x1A71, 0x0A50, 0x3A33, 0x2A12,
    0xDBFD, 0xCBDC, 0xFBBF, 0xEB9E, 0x9B79, 0x8B58, 0xBB3B, 0xAB1A,
    0x6CA6, 0x7C87, 0x4CE4, 0x5CC5, 0x2C22, 0x3C03, 0x0C60, 0x1C41,
    0xEDAE, 0xFD8F, 0xCDEC, 0xDDCD, 0xAD2A, 0xBD0B, 0x8D68, 0x9D49,
    0x7E97, 0x6EB6, 0x5ED5, 0x4EF4, 0x3E13, 0x2E32, 0x1E51, 0x0E70,
    0xFF9F, 0xEFBE, 0xDFDD, 0xCFFC, 0xBF1B, 0xAF3A, 0x9F59, 0x8F78,
    0x9188, 0x81A9, 0xB1CA, 0xA1EB, 0xD10C, 0xC12D, 0xF14E, 0xE16F,
    0x1080, 0x00A1, 0x30C2, 0x20E3, 0x5004, 0x4025, 0x7046, 0x6067,
    0x83B9, 0x9398, 0xA3FB, 0xB3DA, 0xC33D, 0xD31C, 0xE37F, 0xF35E,
    0x02B1, 0x1290, 0x22F3, 0x32D2, 0x4235, 0x5214, 0x6277, 0x7256,
    0xB5EA, 0xA5CB, 0x95A8, 0x8589, 0xF56E, 0xE54F, 0xD52C, 0xC50D,
    0x34E2, 0x24C3, 0x14A0, 0x0481, 0x7466, 0x6447, 0x5424, 0x4405,
    0xA7DB, 0xB7FA, 0x8799, 0x97B8, 0xE75F, 0xF77E, 0xC71D, 0xD73C,
    0x26D3, 0x36F2, 0x0691, 0x16B0, 0x6657, 0x7676, 0x4615, 0x5634,
    0xD94C, 0xC96D, 0xF90E, 0xE92F, 0x99C8, 0x89E9, 0xB98A, 0xA9AB,
    0x5844, 0x4865, 0x7806, 0x6827, 0x18C0, 0x08E1, 0x3882, 0x28A3,
    0xCB7D, 0xDB5C, 0xEB3F, 0xFB1E, 0x8BF9, 0x9BD8, 0xABBB, 0xBB9A,
    0x4A75, 0x5A54, 0x6A37, 0x7A16, 0x0AF1, 0x1AD0, 0x2AB3, 0x3A92,
    0xFD2E, 0xED0F, 0xDD6C, 0xCD4D, 0xBDAA, 0xAD8B, 0x9DE8, 0x8DC9,
    0x7C26, 0x6C07, 0x5C64, 0x4C45, 0x3CA2, 0x2C83, 0x1CE0, 0x0CC1,
    0xEF1F, 0xFF3E, 0xCF5D, 0xDF7C, 0xAF9B, 0xBFBA, 0x8FD9, 0x9FF8,
    0x6E17, 0x7E36, 0x4E55, 0x5E74, 0x2E93, 0x3EB2, 0x0ED1, 0x1EF0
};

/**
*  @brief Function of getting fru ID name by FRU ID
*
*  @param fru: FRU ID
*  @param *name: return variable of FRU ID name
*
*  @return Status of getting FRU ID name
*  0: Success
*  -1: wrong FRU ID
**/
int
netlakenext_get_fruid_name(uint8_t fru, char *name) {
  if (name == NULL) {
    syslog(LOG_ERR, "%s: Invalid parameter: name is NULL ", __func__);
    return 0;
  }

  switch(fru) {
    case FRU_BMC:
      snprintf(name, MAX_FRU_NAME_STR, "MTP");
      break;
    case FRU_PDB:
      snprintf(name, MAX_FRU_NAME_STR, "Power Distribution Board");
      break;
    case FRU_FIO:
      snprintf(name, MAX_FRU_NAME_STR, "Front IO Board");
      break;
    case FRU_NIC:
      snprintf(name, MAX_FRU_NAME_STR, "NIC Board");
      break;
    default:
      syslog(LOG_WARNING, "%s: wrong fruid", __func__);
      return -1;
  }

  return 0;
}

/**
*  @brief Function of getting FRU temp binary path by FRU ID
*
*  @param fru: FRU ID
*  @param *path: return variable of FRU temp binary path
*
*  @return Status of getting FRU temp binary path
*  0: Success
*  -1: wrong FRU ID
**/
int
netlakenext_get_fruid_path(uint8_t fru, char *path) {
  char fname[MAX_FILE_PATH] = {0};

  if (path == NULL) {
    syslog(LOG_ERR, "%s: Invalid parameter: path is NULL ", __func__);
    return 0;
  }

  switch(fru) {
    case FRU_SERVER:
      snprintf(fname, sizeof(fname), "fboss"); // MB FRU uses Meta FBOSS EEPROM format
      break;
    case FRU_BMC:
      snprintf(fname, sizeof(fname), "bmc");
      break;
    case FRU_PDB:
      snprintf(fname, sizeof(fname), "pdb");
      break;
    case FRU_FIO:
      snprintf(fname, sizeof(fname), "fio");
      break;
    case FRU_NIC:
      snprintf(fname, sizeof(fname), "nic");
      break;
    default:
      syslog(LOG_WARNING, "%s: wrong fruid", __func__);
      return -1;
  }
  snprintf(path, MAX_BIN_FILE_STR, COMMON_FRU_PATH, fname);

  return 0;
}

/**
*  @brief Function of getting FRU EEPROM path by FRU ID
*
*  @param fru: FRU ID
*  @param *path: return variable of FRU EEPROM binary path
*
*  @return Status of getting FRU EEPROM path
*  0: Success
*  -1: wrong FRU ID
**/
int
netlakenext_get_fruid_eeprom_path(uint8_t fru, char *path) {
  if (path == NULL) {
    syslog(LOG_ERR, "%s: Invalid parameter: path is NULL ", __func__);
    return 0;
  }

  switch(fru) {
    case FRU_SERVER:
      snprintf(path, MAX_FILE_PATH, EEPROM_PATH, I2C_SERVER_BUS, SERVER_FRU_ADDR); // MB FRU uses Meta FBOSS EEPROM format
      break;
    case FRU_BMC:
      snprintf(path, MAX_FILE_PATH, EEPROM_PATH, I2C_BMC_BUS, BMC_FRU_ADDR);
      break;
    case FRU_PDB:
    case FRU_FIO:
    case FRU_NIC:
      return -1;
    default:
      syslog(LOG_WARNING, "%s: wrong fruid", __func__);
      return -1;
  }

  return 0;
}

/**
*  @brief Update CRC-CCITT-AUG value with next data byte
*
*  @param crc: previous CRC value
*  @param c: input data byte
*
*  @return Updated CRC16-CCITT-AUG value
**/
static inline uint16_t
netlakenext_crc_ccitt_aug_byte(uint16_t crc, const uint8_t c) {
  return (crc << 8) ^ crc_ccitt_aug_table[(crc >> 8) ^ c];
};

/**
*  @brief Function of computing CRC-CCITT-AUG over the data buffer
*
*  @param *buf: data buffer
*  @param len: number of bytes in the buffer
*
*  @return Computed CRC16-CCITT-AUG value
**/
uint16_t
netlakenext_crc16_ccitt_aug(const uint8_t *buf, size_t len) {
  uint16_t crc = 0x1D0F; // initial value
  while (len--) {
    crc = netlakenext_crc_ccitt_aug_byte(crc, *buf++);
  }
  return crc;
}

/**
*  @brief Function of finding the offset of CRC field in FBOSS EEPROM
*
*  @param *buf: data buffer
*  @param *crc_offset: return variable of offset of CRC's value field
*
*  @return Status of finding CRC field offset
*   0: Success
*  -1: Failed to find CRC field
**/
int
netlakenext_find_crc_offset(const uint8_t *buf, int *crc_offset) {
  int cursor = 4;

  while (cursor + 1 < FBOSS_FRUID_SIZE) {
    // parse TLV to find CRC field
    uint8_t type = buf[cursor];
    uint8_t length = buf[cursor + 1];

    if (type == 0x0 || type == 0xFF) break; // end of FRU data
    if (type == FBOSS_BODY_CRC16_TYPE) {
      if (cursor + 3 < FBOSS_FRUID_SIZE) {
        *crc_offset = cursor + 2;
        return 0;
      } else {
        return -1; 
      }
    }

    cursor += 2 + length; // to next TLV entry
  }

  return -1;
}

/**
*  @brief Function of validating the IPMI FRU ID header
*
*  @param *buf: header buffer
*  @param *bin_file: binary path
*
*  @return Status of IPMI FRUID header validation
*  0: Valid
*  -1: Invalid FRU
**/
int
netlakenext_check_ipmi_fru_is_valid(const char *bin_file) {
  int i = 0, bin = 0;
  uint8_t cal_chksum = 0, header_chksum = 0;
  uint8_t head_buf[FRUID_HEADER_SIZE] = {0};
  bool all_zero_flag = true;
  ssize_t bytes_rd = 0;

  bin = open(bin_file, O_RDONLY);
  if (bin < 0) {
    syslog(LOG_ERR, "%s: Falied to check FRU is valid or not because unable to open the %s file, %s", __func__, bin_file, strerror(errno));
    return -1;
  }

  bytes_rd = read(bin, head_buf, sizeof(head_buf));
  close(bin);

  if (bytes_rd < 0) {
    syslog(LOG_ERR, "%s: Falied to check FRU is valid or not because read FRU failed: %s", __func__, strerror(errno));
    return -1;
  } else if (bytes_rd != FRUID_HEADER_SIZE) {
    syslog(LOG_ERR, "%s: Falied to check FRU is valid or not because the size of header is wrong: %zd, expected: %d", __func__, bytes_rd, FRUID_HEADER_SIZE);
    return -1;
  }

  // Zero checksum calculation
  for (i = 0; i < FRUID_HEADER_SIZE - 1; i++) {
    cal_chksum += head_buf[i];
    if ((all_zero_flag == true) && (head_buf[i] > 0)) {
      all_zero_flag = false;
    }
  }
  cal_chksum = ~(cal_chksum) + 1;
  header_chksum = head_buf[i];

  if ((all_zero_flag == true) && (header_chksum == 0x00)) {
    // The header bytes are all zero.
    syslog(LOG_CRIT, "FRU header %s is empty", bin_file);
    return -1;
  } else if (cal_chksum != header_chksum) {
    if ((header_chksum == 0xff) && (cal_chksum == FRUID_HEADER_EMPTY)) {
      // The header bytes are all 0xff.
      syslog(LOG_CRIT, "FRU header %s is empty", bin_file);
    } else {
      // The checksum is wrong
      syslog(LOG_CRIT, "New FRU data %s checksum is invalid", bin_file);
    }
    return -1;
  }

  return 0;
}

/**
*  @brief Function of validating the Meta FBOSS v6 FRU ID
* +----------------------------------------------------------------------+
* |                     FBOSS V6 FRUID header format                     |
* +----------------------------------------------------------------------+
* | Offset (byte) | Length (bytes) | Value  | Description                |
* | ------------- | -------------- | ------ | -------------------------- |
* |       0       |       2        | 0xFBFB | Magic word                 |
* |       2       |       1        | 0x6    | Meta EEPROM Format Version |
* |       3       |       1        | 0XFF   | Reserved for future use    |
* +----------------------------------------------------------------------+
*
*  @param *bin_file: binary path
*
*  @return Status of FBOSS FRUID header validation
*   0: Valid
*  -1: Failed to check / Invalid FRU / Not supported
**/
int
netlakenext_check_fboss_fru_is_valid(const char *bin_file) {
  int i = 0, bin = 0;
  bool all_zero_flag = true;
  bool all_empty_flag = true;
  uint8_t buf[FBOSS_FRUID_SIZE] = {0};
  ssize_t bytes_rd = 0;
  
  bin = open(bin_file, O_RDONLY);
  if (bin < 0) {
    syslog(LOG_ERR, "%s: Falied to check FRU is valid or not because unable to open the %s file, %s", __func__, bin_file, strerror(errno));
    return -1;
  }

  bytes_rd = read(bin, buf, FBOSS_FRUID_SIZE);
  close(bin);

  if (bytes_rd < 0) {
    syslog(LOG_ERR, "%s: Falied to check FRU is valid or not because read FRU %s failed: %s", __func__, bin_file, strerror(errno));
    return -1;
  } else if (bytes_rd != FBOSS_FRUID_SIZE) {
    syslog(LOG_ERR, "%s: Falied to check FRU is valid or not because read FRU %s incomplete (read %zd of %d bytes)", __func__, bin_file, bytes_rd, FBOSS_FRUID_SIZE);
    return -1;
  }

  // check header
  if ((buf[0] != FBOSS_EEPROM_MAGIC) || (buf[1] != FBOSS_EEPROM_MAGIC)) {
    for (i = 0; i < FBOSS_FRUID_HEADER_SIZE; i++) {
      if (buf[i] != 0xff) all_empty_flag = false;
      if (buf[i] != 0x0) all_zero_flag = false;
      if (!all_empty_flag && !all_zero_flag) break;
    }
    // empty FRU ID
    if (all_zero_flag || all_empty_flag) {
      syslog(LOG_CRIT, "FRU header %s is empty", bin_file);
      return -1;
    }
    // non-empty and magic word invalid
    syslog(LOG_CRIT, "New FRU data %s header is invalid: 0x%02X%02X, expected: 0xFBFB", bin_file, buf[0], buf[1]);
    return -1;
  }

  if (buf[2] < 3 || buf[2] > 6) {
    syslog(LOG_CRIT, "FRU data %s FBOSS version not supported: %d, support version: 3-6", bin_file, buf[2]);
    return -1;
  }

  // check CRC value
  int crc_offset = 0;
  if (netlakenext_find_crc_offset(buf, &crc_offset) < 0) {
    syslog(LOG_ERR, "%s: Failed to find CRC field in FRU data %s", __func__, bin_file);
    return -1;
  }
  uint16_t calculated_crc = netlakenext_crc16_ccitt_aug(buf, crc_offset - 2);
  uint16_t programmed_crc = (buf[crc_offset] << 8 | buf[crc_offset + 1]); // big-endian
  if (calculated_crc != programmed_crc) {
    syslog(LOG_CRIT, "FRU data %s checksum mismatch: 0x%04X, expected: 0x%04X", bin_file, programmed_crc, calculated_crc);
    return -1;
  }

  return 0;
}

/**
*  @brief Function of checking whether FRU EEPROM header is valid by binary path
*
*  @param * bin_file: binary path
*
*  @return Status of getting FRU EEPROM path
*   0: Valid
*  -1: Failed to check/Invalid FRU
**/
int
netlakenext_check_fru_is_valid(const char *bin_file) {
  if (bin_file == NULL) {
    syslog(LOG_ERR, "%s: Falied to check FRU is valid or not due to NULL parameter ", __func__);
    return -1;
  }

  if (strcmp(bin_file, FRU_SERVER_BIN) == 0) {
    return netlakenext_check_fboss_fru_is_valid(bin_file);
  } else {
    return netlakenext_check_ipmi_fru_is_valid(bin_file);
  }
}

