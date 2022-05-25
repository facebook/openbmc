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
#include <errno.h>
#include <sys/stat.h>
#include <openbmc/pal.h>
#include <openbmc/fruid.h>
#include <facebook/netlakemtp_common.h>

static int
get_bmc_fruid(uint8_t ipmi_fruid, uint8_t *fruid) {

  if (fruid == NULL) {
    syslog(LOG_ERR, "%s: invalid parameter: fruid is NULL", __func__);
    return -1;
  }

  if (ipmi_fruid >= (FRU_CNT - 1)) {
    syslog(LOG_WARNING, "%s() Invalid fruid: %u\n", __func__, ipmi_fruid);
    return -1;
  }

  // ipmi command use 0-base fruid
  // bmc use 1-base fruid
  *fruid = ipmi_fruid + 1;

  return 0;
}

int
plat_fruid_init() {
  char path[MAX_PATH_LEN] = {0};
  int path_len = sizeof(path);

  //create the fru binary in /tmp/
  //fruid_server.bin
  snprintf(path, path_len, EEPROM_PATH, I2C_SERVER_BUS, SERVER_FRU_ADDR);
  if (pal_copy_eeprom_to_bin(path, FRU_SERVER_BIN) < 0) {
    syslog(LOG_WARNING, "%s() Failed to copy %s to %s", __func__, path, FRU_SERVER_BIN);
  }
  if (pal_check_fru_is_valid(FRU_SERVER_BIN) < 0) {
    syslog(LOG_WARNING, "%s() The FRU %s is wrong.", __func__, FRU_SERVER_BIN);
  }
  //fruid_bmc.bin
  snprintf(path, path_len, EEPROM_PATH, I2C_BMC_BUS, BMC_FRU_ADDR);
  if (pal_copy_eeprom_to_bin(path, FRU_BMC_BIN) < 0) {
    syslog(LOG_WARNING, "%s() Failed to copy %s to %s", __func__, path, FRU_BMC_BIN);
  }
  if (pal_check_fru_is_valid(FRU_BMC_BIN) < 0) {
    syslog(LOG_WARNING, "%s() The FRU %s is wrong.", __func__, FRU_BMC_BIN);
  }


  return 0;
}

int
plat_fruid_data(unsigned char payload_id, int fru_id, int offset, int count, unsigned char *data) {
  int fd = 0;
  int ret = 0;
  uint8_t bmc_fruid = 0;
  char fru_path[MAX_PATH_LEN] = {0};
  char fru_name[MAX_FRU_CMD_STR] = {0};

  if (data == NULL) {
    syslog(LOG_ERR, "%s: invalid parameter: data is NULL", __func__);
    return -1;
  }

  if (get_bmc_fruid((uint8_t)fru_id, &bmc_fruid) < 0) {
    return -1;
  }

  if (pal_get_fru_name(bmc_fruid, fru_name) < 0) {
    syslog(LOG_WARNING, "%s() Fail to get fru%u name\n", __func__, bmc_fruid);
    return -1;
  }

  snprintf(fru_path, sizeof(fru_path), COMMON_FRU_PATH, fru_name);

  // open file for read purpose
  fd = open(fru_path, O_RDONLY);
  if (fd < 0) {
    syslog(LOG_ERR, "%s: unable to open the %s file: %s",
    __func__, fru_path, strerror(errno));
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
  struct stat buf = {0};
  int ret = 0;
  char fru_path[MAX_PATH_LEN] = {0};
  char fru_name[MAX_FRU_CMD_STR] = {0};

  if (payload_id != FRU_SERVER) {
    syslog(LOG_WARNING, "%s() Payload id: %u not support, only support get fruid size from server\n", __func__, (uint8_t)payload_id);
    return 0;
  }

  if (pal_get_fru_name((uint8_t)payload_id, fru_name) < 0) {
    syslog(LOG_WARNING, "%s() Fail to get fru%u name\n", __func__, (uint8_t)payload_id);
    return 0;
  }

  snprintf(fru_path, sizeof(fru_path), COMMON_FRU_PATH, fru_name);

  // check the size of the file and return size
  ret = stat(fru_path, &buf);
  if (ret != 0) {
    return 0;
  }

  return buf.st_size;
}
