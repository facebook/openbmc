/* Copyright 2017-present Facebook. All Rights Reserved.
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
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <facebook/i2c-dev.h>
#include "nvme-mi.h"

#define I2C_NVME_INTF_ADDR 0x6A
#define NVME_SFLGS_REG 0x01
#define NVME_WARN_REG 0x02
#define NVME_TEMP_REG 0x03
#define NVME_PDLU_REG 0x04
#define NVME_SERIAL_NUM_REG 0x0B
#define SERIAL_NUM_SIZE 20

/* Read a byte from NVMe-MI 0x6A. Need to give a bus and a byte address for reading. */
int
nvme_read_byte(const char *device, uint8_t item, uint8_t *value) {
  int dev;
  int ret;
  int32_t res;
  int retry = 0;

  dev = open(device, O_RDWR);
  if (dev < 0) {
    syslog(LOG_DEBUG, "%s(): open() failed", __func__);
    return -1;
  }

  ret = ioctl(dev, I2C_SLAVE, I2C_NVME_INTF_ADDR);
  if (ret < 0) {
    syslog(LOG_DEBUG, "%s(): ioctl() assigning i2c addr failed", __func__);
    close(dev);
    return -1;
  }

  retry = 0;
  while (retry < 5) {
    res = i2c_smbus_read_byte_data(dev, item);
    if (res < 0)
      retry++;
    else
      break;

    msleep(100);
  }

  if (res < 0) {
    syslog(LOG_DEBUG, "%s(): i2c_smbus_read_byte_data failed", __func__);
    close(dev);
    return -1;
  }

  *value = (uint8_t) res;

  close(dev);

  return 0;
}

/* Read NVMe-MI SFLGS. Need to give a bus for reading. */
int
nvme_sflgs_read(const char *device, uint8_t *value) {
  int ret;

  ret = nvme_read_byte(device, NVME_SFLGS_REG, value);

  if(ret < 0) {
    syslog(LOG_DEBUG, "%s(): nvme_read_byte failed", __func__);
    return -1;
  }

  return 0;
}

/* Read NVMe-MI SMART Warnings. Need to give a bus for reading. */
int
nvme_smart_warning_read(const char *device, uint8_t *value) {
  int ret;

  ret = nvme_read_byte(device, NVME_WARN_REG, value);

  if(ret < 0) {
    syslog(LOG_DEBUG, "%s(): nvme_read_byte failed", __func__);
    return -1;
  }

  return 0;
}

/* Read NVMe-MI Composite Temperature. Need to give a bus for reading. */
int
nvme_temp_read(const char *device, uint8_t *value) {
  int ret;

  ret = nvme_read_byte(device, NVME_TEMP_REG, value);

  if(ret < 0) {
    syslog(LOG_DEBUG, "%s(): nvme_read_byte failed", __func__);
    return -1;
  }

  return 0;
}

/* Read NVMe-MI PDLU. Need to give a bus for reading. */
int
nvme_pdlu_read(const char *device, uint8_t *value) {
  int ret;

  ret = nvme_read_byte(device, NVME_PDLU_REG, value);

  if(ret < 0) {
    syslog(LOG_DEBUG, "%s(): nvme_read_byte failed", __func__);
    return -1;
  }

  return 0;
}

/* Read NVMe-MI Serial Number. Need to give a bus for reading. */
int
nvme_serial_num_read(const char *device, uint8_t *value, int size) {
  int ret;
  uint8_t reg = NVME_SERIAL_NUM_REG;
  int count;

  if(size != SERIAL_NUM_SIZE) {
    syslog(LOG_DEBUG, "%s(): the array size is wrong", __func__);
    return -1;
  }

  for(count = 0; count < SERIAL_NUM_SIZE; count++) {
    ret = nvme_read_byte(device, reg + count, value + count);
    if(ret < 0) {
      syslog(LOG_DEBUG, "%s(): nvme_read_byte failed", __func__);
      return -1;
    }
  }
  return 0;
}
