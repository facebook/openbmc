/*
 *
 * Copyright 2020-present Facebook. All Rights Reserved.
 *
 * This file contains code to support IPMI2.0 Specification available @
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
 
#include <syslog.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>

#include <bic.h>
#include <bic_ipmi.h>
#include <openbmc/ipmb.h>
 
#define MAX_FRU_DATA_READ_COUNT 0x20
#define MAX_FRU_DATA_WRITE_COUNT 0x20


int
bic_read_fruid(uint8_t bic_fru_id, char *path) {
  int ret = 0;
  uint32_t nread = 0;
  uint32_t offset = 0;
  uint8_t count = 0;
  uint8_t rbuf[MAX_IPMB_RES_LEN] = {0};
  uint8_t rlen = 0;
  int fd = 0;
  ssize_t bytes_wr = 0;
  ipmi_fruid_info_t info = {0};

  // Remove the file if exists already
  unlink(path);

  // Open the file exclusively for write
  fd = open(path, O_WRONLY | O_CREAT | O_EXCL, 0666);
  if (fd < 0) {
    syslog(LOG_ERR, "%s open fails for path: %s\n", __func__, path);
    goto error_exit;
  }

  // Read the FRUID information
  ret = bic_get_fruid_info(FRU_MB, bic_fru_id, &info);
  if (ret) {
    syslog(LOG_ERR, "%s bic_read_fruid_info returns %d \n", __func__, ret);
    goto error_exit;
  }

  // Indicates the size of the FRUID
  nread = (info.size_msb << 8) | info.size_lsb;
  if (nread > MAX_FRU_DATA_SIZE) {
    nread = MAX_FRU_DATA_SIZE;
  } else if (nread == 0) {
    goto error_exit;
  }

  // Read chunks of FRUID binary data in a loop
  offset = 0;
  while (nread > 0) {
    if (nread > MAX_FRU_DATA_READ_COUNT) {
      count = MAX_FRU_DATA_READ_COUNT;
    } else {
      count = nread;
    }

    ret = bic_read_fru_data(FRU_MB, bic_fru_id, offset, count, rbuf, &rlen);
    if (ret) {
      syslog(LOG_ERR, "%s _read_fruid failed, ret: %d\n", __func__, ret);
      goto error_exit;
    }

    // Ignore the first byte as it indicates length of response
    bytes_wr = write(fd, &rbuf[1], rlen-1);
    if (bytes_wr != rlen-1) {
      syslog(LOG_ERR, "%s write fruid failed, write bytes: %d\n", __func__, bytes_wr);
      return -1;
    }

    // Update offset
    offset += (rlen-1);
    nread -= (rlen-1);
  }

error_exit:
  if (fd >= 0) {
    close(fd);
  }

  return ret;
}

int
bic_write_fruid(uint8_t bic_fru_id, char *path) {
  int ret = -1;
  uint32_t offset = 0;
  ssize_t count = 0;
  uint8_t buf[64] = {0};
  int fd = 0;

  // Open the file exclusively for read
  fd = open(path, O_RDONLY, 0666);
  if (fd < 0) {
    syslog(LOG_ERR, "%s() Failed to open %s\n", __func__, path);
    goto error_exit;
  }

  // Write chunks of FRUID binary data in a loop
  offset = 0;
  while (1) {
    // Read from file
    count = read(fd, buf, MAX_FRU_DATA_WRITE_COUNT);
    if (count <= 0) {
      break;
    }

    // Write to the FRUID
    ret = bic_write_fru_data(FRU_MB, bic_fru_id, offset, (uint8_t)count, buf);
    if (ret) {
      break;
    }

    // Update counter
    offset += count;
  }

error_exit:
  if (fd >= 0 ) {
    close(fd);
  }

  return ret;
}


int
bic_get_sensor_reading(uint8_t sensor_num, float *value) {
#define BIC_SENSOR_READ_NA 0x20
  int ret = 0;
  ipmi_extend_sensor_reading_t sensor = {0};

  ret = bic_get_accur_sensor(FRU_MB, sensor_num, &sensor);
  if (ret < 0) {
    syslog(LOG_WARNING, "%s() Failed, snr#0x%x", __func__, sensor_num);
    return READING_NA;
  } else {
    if (sensor.flags & BIC_SENSOR_READ_NA) {
      return READING_NA;
    }

    *value = ((int16_t)((sensor.value & 0xFFFF0000) >> 16)) * 0.001 + ((int16_t)(sensor.value & 0x0000FFFF)) ;
  }

  return ret;
}
