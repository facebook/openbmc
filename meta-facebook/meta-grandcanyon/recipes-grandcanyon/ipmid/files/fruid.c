/*
 *
 * Copyright 2020-present Facebook. All Rights Reserved.
 *
 * This file provides platform specific implementation of FRUID information
 *
 * FRUID specification can be found at
 * www.intel.com/content/dam/www/public/us/en/documents/product-briefs/platform-management-fru-document-rev-1-2-feb-2013.pdf
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

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <syslog.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <pthread.h>
#include <sys/stat.h>
#include <openbmc/pal.h>
#include <openbmc/fruid.h>
#include <facebook/fbgc_common.h>

#define FRU_ID_SERVER 0
#define FRU_ID_BMC    1
#define FRU_ID_UIC    2
#define FRU_ID_NIC    3
#define FRU_ID_IOCM   4


int
plat_fruid_init() {
  char path[128] = {0};
  int path_len = sizeof(path);

  //create the fru binary in /tmp/
  //fruid_bmc.bin
  snprintf(path, path_len, EEPROM_PATH, I2C_BSM_BUS, BMC_FRU_ADDR);
  if (pal_copy_eeprom_to_bin(path, FRU_BMC_BIN) < 0) {
    syslog(LOG_WARNING, "%s() Failed to copy %s to %s", __func__, path, FRU_BMC_BIN);
  }

  //fruid_uic.bin
  snprintf(path, path_len, EEPROM_PATH, I2C_UIC_BUS, UIC_FRU_ADDR);
  if (pal_copy_eeprom_to_bin(path, FRU_UIC_BIN) < 0) {
    syslog(LOG_WARNING, "%s() Failed to copy %s to %s", __func__, path, FRU_UIC_BIN);
  }

  //fruid_nic.bin
  snprintf(path, path_len, EEPROM_PATH, I2C_NIC_BUS, NIC_FRU_ADDR);
  if (pal_copy_eeprom_to_bin(path, FRU_NIC_BIN) < 0) {
    syslog(LOG_WARNING, "%s() Failed to copy %s to %s", __func__, path, FRU_NIC_BIN);
  }

  //fruid_iocm.bin
  snprintf(path, path_len, EEPROM_PATH, I2C_T5E1S1_T7IOC_BUS, IOCM_FRU_ADDR);
  if (pal_copy_eeprom_to_bin(path, FRU_IOCM_BIN) < 0) {
    syslog(LOG_WARNING, "%s() Failed to copy %s to %s", __func__, path, FRU_IOCM_BIN);
  }

  return 0;
}

int
plat_fruid_data(unsigned char payload_id, int fru_id, int offset, int count, unsigned char *data) {
  char fpath[64] = {0};
  int fd = 0;
  int ret = 0;

  // Fill the file path for a given FRU
  if (fru_id == FRU_ID_BMC) {    
    snprintf(fpath, sizeof(fpath), FRU_BMC_BIN);
  } else if (fru_id == FRU_ID_UIC) {
    snprintf(fpath, sizeof(fpath), FRU_UIC_BIN);
  } else if (fru_id == FRU_ID_NIC) {
    snprintf(fpath, sizeof(fpath), FRU_NIC_BIN);
  } else if (fru_id == FRU_ID_IOCM) {
    snprintf(fpath, sizeof(fpath), FRU_IOCM_BIN);
  } else {
    return -1;
  }

  // open file for read purpose
  fd = open(fpath, O_RDONLY);
  if (fd < 0) {
    syslog(LOG_ERR, "%s: unable to open the %s file: %s",
    __func__, fpath, strerror(errno));
    return -1;
  }

  // seek position based on given offset
  ret = lseek(fd, offset, SEEK_SET);
  if (ret < 0) {
    close(fd);
    return -1;
  }

  // read the file content
  ret = read(fd, data, count);
  if (ret != count) {
    close(fd);
    return -1;
  }
  close(fd);

  return 0;
}

int
plat_fruid_size(unsigned char payload_id) {
  struct stat buf;
  int ret = 0;
  char fpath[64] = {0};

  if (payload_id == FRU_ID_BMC) {    
    snprintf(fpath, sizeof(fpath), FRU_BMC_BIN);
  } else if (payload_id == FRU_ID_UIC) {
    snprintf(fpath, sizeof(fpath), FRU_UIC_BIN);
  } else if (payload_id == FRU_ID_NIC) {
    snprintf(fpath, sizeof(fpath), FRU_NIC_BIN);
  } else if (payload_id == FRU_ID_IOCM) {
    snprintf(fpath, sizeof(fpath), FRU_IOCM_BIN);
  } else {
    return -1;
  }

  // check the size of the file and return size
  ret = stat(fpath, &buf);
  if (ret) {
    return 0;
  }

  return buf.st_size;
}
