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
#include <facebook/netlakemtp_common.h>
#include "netlakemtp_fruid.h"

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
netlakemtp_get_fruid_name(uint8_t fru, char *name) {
  if (name == NULL) {
    syslog(LOG_ERR, "%s: Invalid parameter: name is NULL ", __func__);
    return 0;
  }

  switch(fru) {
    case FRU_SERVER:
      snprintf(name, MAX_FRU_NAME_STR, "Netlake");
      break;
    case FRU_BMC:
      snprintf(name, MAX_FRU_NAME_STR, "MTP");
      break;
    case FRU_PDB:
      snprintf(name, MAX_FRU_NAME_STR, "Power Distribution Board");
      break;
    case FRU_FIO:
      snprintf(name, MAX_FRU_NAME_STR, "Front IO Board");
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
netlakemtp_get_fruid_path(uint8_t fru, char *path) {
  char fname[MAX_FILE_PATH] = {0};

  if (path == NULL) {
    syslog(LOG_ERR, "%s: Invalid parameter: path is NULL ", __func__);
    return 0;
  }

  switch(fru) {
    case FRU_SERVER:
      snprintf(fname, sizeof(fname), "server");
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
*  @param *name: return variable of FRU EEPROM binary path
*
*  @return Status of getting FRU EEPROM path
*  0: Success
*  -1: wrong FRU ID
**/
int
netlakemtp_get_fruid_eeprom_path(uint8_t fru, char *path) {
  if (path == NULL) {
    syslog(LOG_ERR, "%s: Invalid parameter: path is NULL ", __func__);
    return 0;
  }

  switch(fru) {
    case FRU_SERVER:
      snprintf(path, MAX_FILE_PATH, EEPROM_PATH, I2C_SERVER_BUS, SERVER_FRU_ADDR);
      break;
    case FRU_BMC:
      snprintf(path, MAX_FILE_PATH, EEPROM_PATH, I2C_BMC_BUS, BMC_FRU_ADDR);
      break;
    case FRU_PDB:
    case FRU_FIO:
      return -1;
    default:
      syslog(LOG_WARNING, "%s: wrong fruid", __func__);
      return -1;
  }

  return 0;
}

/**
*  @brief Function of checking FRU EEPROM path by FRU ID
*
*  @param * bin_file: binary path
*
*  @return Status of getting FRU EEPROM path
*  0: Success
*  -1: Failed to check/Invalid FRU
**/
int
netlakemtp_check_fru_is_valid(const char * bin_file)
{
  int i = 0, bin = 0;
  uint8_t cal_chksum = 0, header_chksum = 0;
  uint8_t head_buf[FRUID_HEADER_SIZE] = {0};
  bool all_zero_flag = true;
  ssize_t bytes_rd = 0;

  if (bin_file == NULL) {
    syslog(LOG_ERR, "%s: Falied to check FRU is valid or not due to NULL parameter ", __func__);
    return -1;
  }

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
    syslog(LOG_ERR, "%s: Falied to check FRU is valid or not because the size of header is wrong: %d, expected: %d", __func__, bytes_rd, FRUID_HEADER_SIZE);
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

