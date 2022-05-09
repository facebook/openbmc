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
#include <openbmc/fruid.h>
#include <facebook/fbgc_common.h>
#include "fbgc_fruid.h"
#include <facebook/bic.h>

int
fbgc_get_fruid_name(uint8_t fru, char *name) {
  switch(fru) {
    case FRU_SERVER:
      snprintf(name, MAX_FRU_NAME_STR, "Barton Springs");
      break;
    case FRU_BMC:
      snprintf(name, MAX_FRU_NAME_STR, "BMC");
      break;
    case FRU_UIC:
      snprintf(name, MAX_FRU_NAME_STR, "User Interface Card");
      break;
    case FRU_DPB:
      snprintf(name, MAX_FRU_NAME_STR, "Drive Plane Board");
      break;
    case FRU_SCC:
      snprintf(name, MAX_FRU_NAME_STR, "Storage Controller Card");
      break;
    case FRU_NIC:
      snprintf(name, MAX_FRU_NAME_STR, "NIC");
      break;
    case FRU_E1S_IOCM:
      snprintf(name, MAX_FRU_NAME_STR, "IOC Module");
      break;
    case FRU_FAN0:
      snprintf(name, MAX_FRU_NAME_STR, "FAN0");
      break;
    case FRU_FAN1:
      snprintf(name, MAX_FRU_NAME_STR, "FAN1");
      break;
    case FRU_FAN2:
      snprintf(name, MAX_FRU_NAME_STR, "FAN2");
      break;
    case FRU_FAN3:
      snprintf(name, MAX_FRU_NAME_STR, "FAN3");
      break;
    default:
      syslog(LOG_WARNING, "%s: wrong fruid", __func__);
      return -1;
  }

  return 0;
}

int
fbgc_get_fruid_path(uint8_t fru, char *path) {
  char fname[MAX_FILE_PATH] = {0};
  uint8_t type = 0;

  switch(fru) {
    case FRU_SERVER:
      snprintf(fname, sizeof(fname), "server");
      break;
    case FRU_BMC:
      snprintf(fname, sizeof(fname), "bmc");
      break;
    case FRU_UIC:
      snprintf(fname, sizeof(fname), "uic");
      break;
    case FRU_DPB:
      snprintf(fname, sizeof(fname), "dpb");
      break;
    case FRU_SCC:
      snprintf(fname, sizeof(fname), "scc");
      break;
    case FRU_NIC:
      snprintf(fname, sizeof(fname), "nic");
      break;
    case FRU_E1S_IOCM:      
      snprintf(fname, sizeof(fname), "iocm");
      fbgc_common_get_chassis_type(&type);
      if (type == CHASSIS_TYPE5) {
        syslog(LOG_WARNING, "%s: FRU iocm not supported by type 5 system", __func__);
      }
      break;
    case FRU_FAN0:
      snprintf(fname, sizeof(fname), "fan0");
      break;
    case FRU_FAN1:
      snprintf(fname, sizeof(fname), "fan1");
      break;
    case FRU_FAN2:
      snprintf(fname, sizeof(fname), "fan2");
      break;
    case FRU_FAN3:
      snprintf(fname, sizeof(fname), "fan3");
      break;
    default:
      syslog(LOG_WARNING, "%s: wrong fruid", __func__);
      return -1;
  }

  if ((fru == FRU_FAN0) || (fru == FRU_FAN1) || (fru == FRU_FAN2) || (fru == FRU_FAN3)) {
    snprintf(path, MAX_BIN_FILE_STR, COMMON_FAN_FRU_PATH, fname);
  } else {
    snprintf(path, MAX_BIN_FILE_STR, COMMON_FRU_PATH, fname);
  }

  return 0;
}

int
fbgc_get_fruid_eeprom_path(uint8_t fru, char *path) {
  uint8_t type = 0;
  switch(fru) {
    case FRU_SERVER:
    case FRU_DPB:
    case FRU_SCC:
    case FRU_FAN0:
    case FRU_FAN1:
    case FRU_FAN2:
    case FRU_FAN3:
      return -1;
    case FRU_E1S_IOCM:
      if (fbgc_common_get_chassis_type(&type) < 0) {
        syslog(LOG_WARNING, "%s() Failed to get chassis type\n", __func__);
        return -1;
      }
      if (type == CHASSIS_TYPE5) {
        syslog(LOG_WARNING, "%s: FRU iocm not supported by type 5 system", __func__);
        return -1;
      }
      snprintf(path, MAX_FILE_PATH, EEPROM_PATH, I2C_T5E1S1_T7IOC_BUS, IOCM_FRU_ADDR);
      break;
    case FRU_BMC:
      snprintf(path, MAX_FILE_PATH, EEPROM_PATH, I2C_BSM_BUS, BMC_FRU_ADDR);
      break;
    case FRU_UIC:
      snprintf(path, MAX_FILE_PATH, EEPROM_PATH, I2C_UIC_BUS, UIC_FRU_ADDR);
      break;
    case FRU_NIC:
      snprintf(path, MAX_FILE_PATH, EEPROM_PATH, I2C_NIC_BUS, NIC_FRU_ADDR);
      break;
    default:
      syslog(LOG_WARNING, "%s: wrong fruid", __func__);
      return -1;
  }

  return 0;
}

int
fbgc_fruid_write(uint8_t fru, char *path) {
  return bic_write_fruid(fru, path);
}

int
fbgc_check_fru_is_valid(const char * bin_file, int log_level)
{
  int i = 0, bin = 0;
  uint8_t cal_chksum = 0, header_chksum = 0;
  uint8_t head_buf[FRUID_HEADER_SIZE] = {0};
  bool all_zero_flag = true;
  ssize_t bytes_rd = 0;
  fruid_info_t fruid = {0};

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
    syslog(log_level, "FRU header %s is empty", bin_file);
    return -1; 
  } else if (cal_chksum != header_chksum) {
    if ((header_chksum == 0xff) && (cal_chksum == FRUID_HEADER_EMPTY)) {
      // The header bytes are all 0xff.
      syslog(log_level, "FRU header %s is empty", bin_file);
    } else {
      // The checksum is wrong
      syslog(log_level, "New FRU data %s checksum is invalid", bin_file);
    }
    return -1;
  } else if (fruid_parse(bin_file, &fruid) != 0) {
    // Check zero checksum of all area.
    syslog(log_level, "FRU data %s is wrong", bin_file);
    return -1;
  } else {
    free_fruid_info(&fruid);
  }

  return 0;
}

