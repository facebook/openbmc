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
#include <unistd.h>
#include <syslog.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <openbmc/obmc-i2c.h>
#include "lightning_flash.h"

#define I2C_DEV_FLASH1 "/dev/i2c-7"
#define I2C_DEV_FLASH2 "/dev/i2c-8"
#define I2C_FLASH_ADDR 0x1b
#define NVME_STATUS_CMD 0x5
#define I2C_M2CARD_AMB_ADDR 0x4c
#define I2C_M2_MUX_ADDR 0x73
#define I2C_NVME_INTF_ADDR 0x6a

#define M2CARD_AMB_TEMP_REG 0x00
#define NVME_TEMP_REG 0x03

#define SKIP_READ_SSD_TEMP "/tmp/skipSSD"

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

  int dev = 0;
  int ret = 0;
  int32_t res = 0;
  uint8_t mux = i2c_map / 10;
  uint8_t chan = i2c_map % 10;
  char bus[32] = {0};

  //if enclosure-util read SSD, skip SSD monitor
  if (access(SKIP_READ_SSD_TEMP, F_OK) == 0)
    return 1;

  ret = lightning_flash_mux_sel_chan(mux, chan);
  if(ret < 0) {
#ifdef DEBUG
    syslog(LOG_DEBUG, "%s(): lightning_flash_mux_sel_chan on Mux %d failed", __func__, mux);
#endif
    return -1;
  }

  if (mux == I2C_MUX_FLASH1)
    sprintf(bus, "%s", I2C_DEV_FLASH1);
  else if (mux == I2C_MUX_FLASH2)
    sprintf(bus, "%s", I2C_DEV_FLASH2);
  else {
#ifdef DEBUG
    syslog(LOG_DEBUG, "%s(): unknown mux", __func__);
#endif
    return -1;
  }

  //if enclosure-util read SSD, skip SSD monitor
  if (access(SKIP_READ_SSD_TEMP, F_OK) == 0)
    return 1;

  dev = open(bus, O_RDWR);
  if (dev < 0) {
#ifdef DEBUG
    syslog(LOG_DEBUG, "%s(): open() failed", __func__);
#endif
    return -1;
  }

  /* Assign the i2c device address */
  ret = ioctl(dev, I2C_SLAVE, I2C_FLASH_ADDR);
  if (ret < 0) {
#ifdef DEBUG
    syslog(LOG_DEBUG, "%s(): ioctl() assigning i2c addr failed", __func__);
#endif
    close(dev);
    return -1;
  }

  /* Read the Status from the NVMe device */
  res = i2c_smbus_read_word_data(dev, NVME_STATUS_CMD);
  if (res < 0) {
#ifdef DEBUG
    syslog(LOG_DEBUG, "%s(): i2c_smbus_read_block_data failed", __func__);
#endif
    close(dev);
    return -1;
  }

  // Return only the word
  *status = (res & 0xFF00) >> 8 | (res & 0xFF) << 8;

  close(dev);

  return ret;
}

/* To read NVMe Management Spec Based device Temperature */
int
lightning_flash_temp_read(uint8_t i2c_map, float *temp) {

  int ret = 0;
  uint16_t status = 0;

  ret = lightning_flash_status_read(i2c_map, &status);
  if(ret < 0) {
#ifdef DEBUG
    syslog(LOG_DEBUG, "%s(): lightning_flash_status_read failed", __func__);
#endif
    return -1;
  } else if(ret == 1) {
    return ret;
  }

  *temp = (status & 0x0FF0) >> 4;

  // Check is the temperature is negative
  if (status & (1 << 12))
    *temp = (~((uint8_t) *temp) + 1) * -1; // Neg Temp is stored in Two's compliment form

  // Decimal value of temperature
  *temp += (status & 0x000F) * 0.125;

  if (*temp == 0xFF) {
#ifdef DEBUG
    syslog(LOG_DEBUG, "%s(): flash not reachable or not present", __func__);
#endif
    return -1;
  }

  return ret;
}

int
lightning_u2_flash_temp_read(uint8_t i2c_map, float *temp) {

  int ret = 0;
  uint8_t mux = i2c_map / 10;
  uint8_t chan = i2c_map % 10;
  uint8_t vendor = 0;

  //if enclosure-util read SSD, skip SSD monitor
  if (access(SKIP_READ_SSD_TEMP, F_OK) == 0)
    return 1;

  /* Set 1-level mux */
  ret = lightning_flash_mux_sel_chan(mux, chan);
  if(ret < 0) {
#ifdef DEBUG
    syslog(LOG_DEBUG, "%s(): lightning_flash_mux_sel_chan on Mux %d failed", __func__, mux);
#endif
    return -1;
  }

  // Get temp reading via NVME
  ret = lightning_nvme_temp_read(mux, temp);

  if(ret < 0) {
#ifdef DEBUG
    syslog(LOG_DEBUG, "%s(): lightning_nvme_temp_read failed, try old method", __func__);
#endif
    // If it does not support NVME, try old method
    ret = lightning_flash_temp_read(i2c_map, temp);

    if(ret < 0) {
#ifdef DEBUG
      syslog(LOG_DEBUG, "%s(): lightning_flash_temp_read failed", __func__);
#endif
      return -1;
    } else if(ret == 1) {
      return ret;
    } else {
      return 0;
    }

  } else if(ret == 1) {
    return ret;
  }

  // Error handling for flash seems support NVME but always reture same data, try old method to get temp
  ret = lightning_ssd_vendor(&vendor);

  if((ret < 0) && (*temp == 0xD5))
    ret = lightning_flash_temp_read(i2c_map, temp);

  if(ret < 0) {
#ifdef DEBUG
    syslog(LOG_DEBUG, "%s(): lightning_flash_temp_read failed", __func__);
#endif
    return -1;
  }

  return ret;
}

/* Enable the mux to select a particular channel */
int
lightning_flash_mux_sel_chan(uint8_t mux, uint8_t channel) {

  int dev = 0;
  int ret = 0;
  uint8_t addr = 0;
  uint8_t chan_en = 0;
  char bus[32] = {0};

  if (mux == I2C_MUX_FLASH1)
    sprintf(bus, "%s", I2C_DEV_FLASH1);
  else if (mux == I2C_MUX_FLASH2)
    sprintf(bus, "%s", I2C_DEV_FLASH2);
  else {
#ifdef DEBUG
    syslog(LOG_DEBUG, "%s(): unknown mux", __func__);
#endif
    return -1;
  }

  dev = open(bus, O_RDWR);
  if (dev < 0) {
#ifdef DEBUG
    syslog(LOG_DEBUG, "%s(): open() failed", __func__);
#endif
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
#ifdef DEBUG
    syslog(LOG_DEBUG, "%s(): ioctl() assigning i2c addr failed", __func__);
#endif
    close(dev);
    return -1;
  }

  /* Write the channel number to enable it */
  ret = i2c_smbus_write_byte(dev, chan_en);
  if (ret < 0) {
#ifdef DEBUG
    syslog(LOG_DEBUG, "%s(): i2c_smbus_write_byte failed", __func__);
#endif
    close(dev);
    return -1;
  }

  close(dev);

  return 0;
}

int
lightning_flash_sec_mux_sel_chan(uint8_t mux, uint8_t channel) {

  int dev = 0;
  int ret = 0;
  char bus[32] = {0};

  if (mux == I2C_MUX_FLASH1)
    sprintf(bus, "%s", I2C_DEV_FLASH1);
  else if (mux == I2C_MUX_FLASH2)
    sprintf(bus, "%s", I2C_DEV_FLASH2);
  else {
#ifdef DEBUG
    syslog(LOG_DEBUG, "%s(): unknown mux", __func__);
#endif
    return -1;
  }

  dev = open(bus, O_RDWR);
  if (dev < 0) {
#ifdef DEBUG
    syslog(LOG_DEBUG, "%s(): open() failed", __func__);
#endif
    return -1;
  }

  /* Assign the 2-level mux address */
  ret = ioctl(dev, I2C_SLAVE, I2C_M2_MUX_ADDR);
  if (ret < 0) {
#ifdef DEBUG
    syslog(LOG_DEBUG, "%s(): ioctl() assigning i2c addr failed", __func__);
#endif
    close(dev);
    return -1;
  }


  /* Write the channel number to enable it */
  ret = i2c_smbus_write_byte(dev, channel);
  if (ret < 0) {
#ifdef DEBUG
    syslog(LOG_DEBUG, "%s(): i2c_smbus_write_byte failed", __func__);
#endif
    close(dev);
    return -1;
  }

  close(dev);

  return 0;
}

int
lightning_m2_amb_temp_read(uint8_t i2c_map, float *temp) {

  int dev = 0;
  int ret = 0;
  int32_t res = 0;
  uint8_t mux = i2c_map / 10;
  uint8_t chan = i2c_map % 10;
  char bus[32] = {0};

  //if enclosure-util read SSD, skip SSD monitor
  if (access(SKIP_READ_SSD_TEMP, F_OK) == 0)
    return 1;

  ret = lightning_flash_mux_sel_chan(mux, chan);
  if(ret < 0) {
#ifdef DEBUG
    syslog(LOG_DEBUG, "%s(): lightning_flash_mux_sel_chan on Mux %d failed", __func__, mux);
#endif
    return -1;
  }

  if (mux == I2C_MUX_FLASH1)
    sprintf(bus, "%s", I2C_DEV_FLASH1);
  else if (mux == I2C_MUX_FLASH2)
    sprintf(bus, "%s", I2C_DEV_FLASH2);
  else {
#ifdef DEBUG
    syslog(LOG_DEBUG, "%s(): unknown mux", __func__);
#endif
    return -1;
  }

  //if enclosure-util read SSD, skip SSD monitor
  if (access(SKIP_READ_SSD_TEMP, F_OK) == 0)
    return 1;

  dev = open(bus, O_RDWR);
  if (dev < 0) {
#ifdef DEBUG
    syslog(LOG_DEBUG, "%s(): open() failed", __func__);
#endif
    return -1;
  }

  /* Assign the i2c device address */
  ret = ioctl(dev, I2C_SLAVE, I2C_M2CARD_AMB_ADDR);
  if (ret < 0) {
#ifdef DEBUG
    syslog(LOG_DEBUG, "%s(): ioctl() assigning i2c addr failed", __func__);
#endif
    close(dev);
    return -1;
  }

  /* Read the ambient temp */
  res = i2c_smbus_read_word_data(dev, M2CARD_AMB_TEMP_REG);
  if (res < 0) {
#ifdef DEBUG
    syslog(LOG_DEBUG, "%s(): i2c_smbus_read_block_data failed", __func__);
#endif
    close(dev);
    return -1;
  }

  close(dev);

  /* Result is read as MSB byte first and LSB byte second.
   * Result is 12bit with res[11:4]  == MSB[7:0] and res[3:0] = LSB */
  res = ((res & 0x0FF) << 4) | ((res & 0xF000) >> 12);

   /* Resolution is 0.0625 deg C/bit */
  if (res <= 0x7FF) {
    /* Temperature is positive  */
    *temp = (float) res * 0.0625;
  } else if (res >= 0xC90) {
    /* Temperature is negative */
    *temp = (float) (0xFFF - res + 1) * (-1) * 0.0625;
  } else {
    /* Out of range [128C to -55C] */
#ifdef DEBUG
    syslog(LOG_DEBUG, "%s(): invalid res value = 0x%X", __func__, res);
#endif
    return -1;
  }

  return ret;
}

int
lightning_m2_flash_temp_read(uint8_t i2c_map, float *temp) {

  float temp1 = 0.0;
  float temp2 = 0.0;
  int ret = 0;

  /* read the M.2 temp on channel 0 */
  ret = lightning_m2_temp_read(i2c_map, M2_MUX_CHANNEL_0, &temp1);
  if(ret < 0) {
#ifdef DEBUG
    syslog(LOG_DEBUG, "%s(): lightning_m2_temp_read on channel 0 failed", __func__);
#endif
    return -1;
  } else if(ret == 1) {
    return 1;
  }

  /* read the M.2 temp on channel 1 */
  ret = lightning_m2_temp_read(i2c_map, M2_MUX_CHANNEL_1, &temp2);
  if(ret < 0) {
#ifdef DEBUG
    syslog(LOG_DEBUG, "%s(): lightning_m2_temp_read on channel 1 failed", __func__);
#endif
    return -1;
  } else if(ret == 1) {
    return 1;
  }

  /*  return the bigger temp reading */
  *temp = (temp1 > temp2)? temp1:temp2;

  return ret;
}

int
lightning_m2_temp_read(uint8_t i2c_map, uint8_t m2_mux_chan, float *temp) {

  int ret = 0;
  uint8_t mux = i2c_map / 10;
  uint8_t chan = i2c_map % 10;

  //if enclosure-util read SSD, skip SSD monitor
  if (access(SKIP_READ_SSD_TEMP, F_OK) == 0)
    return 1;

  /* Set 1-level mux */
  ret = lightning_flash_mux_sel_chan(mux, chan);
  if(ret < 0) {
#ifdef DEBUG
    syslog(LOG_DEBUG, "%s(): lightning_flash_mux_sel_chan on Mux %d failed", __func__, mux);
#endif
    return -1;
  }

  //if enclosure-util read SSD, skip SSD monitor
  if (access(SKIP_READ_SSD_TEMP, F_OK) == 0)
    return 1;

  /* Set 2-level mux */
  ret = lightning_flash_sec_mux_sel_chan(mux, m2_mux_chan);
  if(ret < 0) {
#ifdef DEBUG
    syslog(LOG_DEBUG, "%s(): lightning_flash_sec_mux_sel_chan on Mux %d failed", __func__, mux);
#endif
    return -1;
  }

  ret = lightning_nvme_temp_read(mux, temp);

  if(ret < 0) {
#ifdef DEBUG
    syslog(LOG_DEBUG, "%s(): lightning_nvme_temp_read failed", __func__);
#endif
    return -1;
  }

  return ret;
}

int
lightning_nvme_temp_read(uint8_t mux, float *temp) {
  int dev = 0;
  int ret = 0;
  int32_t res = 0;
  char bus[32] = {0};

  if (mux == I2C_MUX_FLASH1)
    sprintf(bus, "%s", I2C_DEV_FLASH1);
  else if (mux == I2C_MUX_FLASH2)
    sprintf(bus, "%s", I2C_DEV_FLASH2);
  else {
#ifdef DEBUG
    syslog(LOG_DEBUG, "%s(): unknown mux", __func__);
#endif
    return -1;
  }

  //if enclosure-util read SSD, skip SSD monitor
  if (access(SKIP_READ_SSD_TEMP, F_OK) == 0)
    return 1;

  dev = open(bus, O_RDWR);
  if (dev < 0) {
#ifdef DEBUG
    syslog(LOG_DEBUG, "%s(): open() failed", __func__);
#endif
    return -1;
  }

  /* Assign the i2c device address */
  ret = ioctl(dev, I2C_SLAVE, I2C_NVME_INTF_ADDR);
  if (ret < 0) {
#ifdef DEBUG
    syslog(LOG_DEBUG, "%s(): ioctl() assigning i2c addr failed", __func__);
#endif
    close(dev);
    return -1;
  }

  res = i2c_smbus_read_byte_data(dev, NVME_TEMP_REG);
  if (res < 0) {
#ifdef DEBUG
    syslog(LOG_DEBUG, "%s(): i2c_smbus_read_byte_data failed", __func__);
#endif
    close(dev);
    return -1;
  }

  *temp = (float) res;

  close(dev);

  return ret;
}
