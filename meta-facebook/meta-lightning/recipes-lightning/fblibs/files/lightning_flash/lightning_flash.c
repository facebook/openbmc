/* Copyright 2016-present Facebook. All Rights Reserved.
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
#include "lightning_flash.h"

#define I2C_DEV_FLASH1 "/dev/i2c-7"
#define I2C_DEV_FLASH2 "/dev/i2c-8"
#define I2C_FLASH_ADDR 0x1b
#define NVME_STATUS_CMD 0x5

/* List of information of I2C mapping for mux and channel */
const uint8_t lightning_flash_list[] = {
  I2C_MAP_FLASH0,
  I2C_MAP_FLASH1,
  I2C_MAP_FLASH2,
  I2C_MAP_FLASH3,
  I2C_MAP_FLASH4,
  I2C_MAP_FLASH5,
  I2C_MAP_FLASH6,
  I2C_MAP_FLASH7,
  I2C_MAP_FLASH8,
  I2C_MAP_FLASH9,
  I2C_MAP_FLASH10,
  I2C_MAP_FLASH11,
  I2C_MAP_FLASH12,
  I2C_MAP_FLASH13,
  I2C_MAP_FLASH14,
};

size_t lightning_flash_cnt = sizeof(lightning_flash_list) / sizeof(uint8_t);

// TODO: Need to change the lightning_flash_status_read to read SSD status
// data and not just Temperature Data.

/* To read NVMe Management Spec Based Status information */
int
lightning_flash_status_read(uint8_t i2c_map, uint16_t *status) {

  int dev;
  int ret;
  int32_t res;
  uint8_t mux;
  uint8_t chan;
  char bus[32];

  mux = i2c_map / 10;
  chan = i2c_map % 10;

  ret = lightning_flash_mux_sel_chan(mux, chan);
  if(ret < 0) {
    syslog(LOG_ERR, "lightning_flash_status_read: lightning_flash_mux_sel_chan on Mux %d failed", mux);
    return -1;
  }

  if (mux == I2C_MUX_FLASH1)
    sprintf(bus, "%s", I2C_DEV_FLASH1);
  else if (mux == I2C_MUX_FLASH2)
    sprintf(bus, "%s", I2C_DEV_FLASH2);

  dev = open(bus, O_RDWR);
  if (dev < 0) {
    syslog(LOG_ERR, "lightning_flash_status_read: open() failed");
    return -1;
  }

  /* Assign the i2c device address */
  ret = ioctl(dev, I2C_SLAVE, I2C_FLASH_ADDR);
  if (ret < 0) {
    syslog(LOG_ERR, "lightning_flash_status_read: ioctl() assigning i2c addr failed");
    close(dev);
    return -1;
  }

  /* Read the Status from the NVMe device */
  res = i2c_smbus_read_word_data(dev, NVME_STATUS_CMD);
  if (res < 0) {
    close(dev);
    syslog(LOG_ERR, "lightning_flash_status_read: i2c_smbus_read_block_data failed");
    return -1;
  }

  // Return only the word
  *status = (res & 0xFF00) >> 8 | (res & 0xFF) << 8;

  close(dev);

  return 0;
}

/* To read NVMe Management Spec Based device Temperature */
int
lightning_flash_temp_read(uint8_t i2c_map, float *temp) {

  int ret;
  uint16_t status;

  ret = lightning_flash_status_read(i2c_map, &status);
  if(ret < 0) {
    syslog(LOG_ERR, "lightning_flash_temp_read: lightning_flash_status_read failed");
    return -1;
  }

  *temp = (status & 0x0FF0) >> 4;

  // Check is the temperature is negative
  if (status & (1 << 12))
    *temp *= -1;

  // Decimal value of temperature
  *temp += (status & 0x000F) * 0.125;

  if (*temp == 0xFF) {
    syslog(LOG_INFO, "lightning_flash_temp_read: flash not reachable or not present");
    return -1;
  }

  return 0;
}

/* Enable the mux to select a particular channel */
int
lightning_flash_mux_sel_chan(uint8_t mux, uint8_t channel) {

  int dev;
  int ret;
  uint8_t addr;
  uint8_t chan_en;
  char bus[32];

  if (mux == I2C_MUX_FLASH1)
    sprintf(bus, "%s", I2C_DEV_FLASH1);
  else if (mux == I2C_MUX_FLASH2)
    sprintf(bus, "%s", I2C_DEV_FLASH2);

  dev = open(bus, O_RDWR);
  if (dev < 0) {
    syslog(LOG_ERR, "lightning_flash_mux_sel_chan: open() failed");
    return -1;
  }

  switch(mux) {
    case I2C_MUX_FLASH1:
      addr = I2C_MUX_FLASH1_ADDR,
      chan_en = (1 << 3) | channel;
      break;

    case I2C_MUX_FLASH2:
      addr = I2C_MUX_FLASH2_ADDR,
      chan_en = (1 << 3) | channel;
      break;
  }

  /* Assign the i2c device address */
  ret = ioctl(dev, I2C_SLAVE, addr);
  if (ret < 0) {
    syslog(LOG_ERR, "lightning_flash_mux_sel_chan: ioctl() assigning i2c addr failed");
    close(dev);
    return -1;
  }

  /* Write the channel number to enable it */
  ret = i2c_smbus_write_byte(dev, chan_en);
  if (ret < 0) {
    syslog(LOG_ERR, "lightning_flash_mux_sel_chan: i2c_smbus_write_byte failed");
    close(dev);
    return -1;
  }

  close(dev);

  return 0;
}
