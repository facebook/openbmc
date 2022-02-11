/*
 *
 * Copyright 2015-present Facebook. All Rights Reserved.
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
#include <stdint.h>
#include <stdbool.h>
#include <fcntl.h>
#include <errno.h>
#include <syslog.h>
#include <string.h>
#include <sys/ioctl.h>
#include <openbmc/obmc-i2c.h>
#include "mctp.h"

#define MAX_MCTP_RETRY_CNT  3

int
mctp_i2c_open(uint8_t bus_num) {
  int fd;
  char fn[32];
  int rc;

  snprintf(fn, sizeof(fn), "/dev/i2c-%d", bus_num);
  fd = open(fn, O_RDWR);
  if (fd == -1) {
    syslog(LOG_WARNING, "Failed to open i2c device %s", fn);
    return -1;
  }

  rc = ioctl(fd, I2C_SLAVE, IOM_IOC_SLAVE_ADDRESS);
  if (rc < 0) {
    syslog(LOG_WARNING, "Failed to open slave @ address 0x%x", IOM_IOC_SLAVE_ADDRESS);
    close(fd);
    return -1;
  }

  return fd;
}

int
mctp_i2c_write(uint8_t dev_number, uint8_t slave_addr,uint8_t *buf, uint8_t len) {
  struct i2c_rdwr_ioctl_data data;
  struct i2c_msg msg;
  int rc;
  int i;
  struct timespec req;
  struct timespec rem;
  char bus[32];
  int dev;

  dev = mctp_i2c_open(dev_number);
  if (dev < 0) {
    syslog(LOG_WARNING, "mctp_i2c_write mctp_i2c_open failed");
    return -1;
  }

  memset(&msg, 0, sizeof(msg));

  msg.addr = slave_addr;
  msg.flags = 0;
  msg.len = len;
  msg.buf = buf;

  data.msgs = &msg;
  data.nmsgs = 1;

  // Setup wait time
  req.tv_sec = 0;
  req.tv_nsec = 20000000;//20mSec

  for (i = 0; i < I2C_RETRIES_MAX; i++) {
    rc = ioctl(dev, I2C_RDWR, &data);
    if (rc < 0) {
      nanosleep(&req, &rem);
      continue;
    } else {
      break;
    }
  }
  close(dev);
  if (rc < 0) {
    syslog(LOG_WARNING, "mctp_i2c_write failed to do raw io");
    return -1;
  }

  return 0;
}

int
mctp_i2c_read(uint8_t dev_number, uint8_t slave_addr,uint8_t *buf, uint8_t len) {
  struct i2c_rdwr_ioctl_data data;
  struct i2c_msg msg;
  int rc;
  int i;
  struct timespec req;
  struct timespec rem;
  char bus[32];
  int dev;

  dev = mctp_i2c_open(dev_number);
  if (dev < 0) {
    syslog(LOG_WARNING, "mctp_i2c_read mctp_i2c_open failed");
    return -1;
  }

  memset(&msg, 0, sizeof(msg));

  msg.addr = slave_addr;
  msg.flags = I2C_M_RD;
  msg.len = len;
  msg.buf = buf;

  data.msgs = &msg;
  data.nmsgs = 1;

  // Setup wait time
  req.tv_sec = 0;
  req.tv_nsec = 20000000;//20mSec

  for (i = 0; i < I2C_RETRIES_MAX; i++) {
    rc = ioctl(dev, I2C_RDWR, &data);
    if (rc < 0) {
      nanosleep(&req, &rem);
      continue;
    } else {
      break;
    }
  }
  close(dev);
  if (rc < 0) {
    syslog(LOG_WARNING, "mctp_i2c_read failed to do raw io");
    return -1;
  }

  return 0;
}

//Type7 IOM IOC initialization
int mctp_ioc_init(uint8_t tag) {
  uint8_t pwrite_buf[72];
  uint8_t readd[80];
  /* IOC_Tempeature initialization */
  uint8_t slave_addr = 0x0a;  /* slave address */
  int reading = -1;
  int fd, ret = 0;

  memset(pwrite_buf, 0, 72);
  memset(readd, 0, 80);

  pwrite_buf[0] = 0xf;
  pwrite_buf[1] = 0x11;
  pwrite_buf[2] = 0x1;
  pwrite_buf[3] = 0x01;
  pwrite_buf[4] = 0x00;
  pwrite_buf[5] = 0x0;
  pwrite_buf[6] = 0xc8;
  pwrite_buf[7] = 0x7e; /* Message type */
  pwrite_buf[8] = 0x10;
  pwrite_buf[9] = 0x00;
  pwrite_buf[10] = 0x20;  /* Payload ID LSI define command or respomse */
  pwrite_buf[11] = 0x0;
  pwrite_buf[12] = 0x0;
  pwrite_buf[13] = tag;   /* App Msg tag */
  pwrite_buf[14] = 0x01;  /* Command initail MCTP interface can't open */
  pwrite_buf[15] = 0x00;  /* Payload length */
  pwrite_buf[16] = 0x00;
  pwrite_buf[17] = 0x00;
  pwrite_buf[18] = 0x00;

  ret = mctp_i2c_write(IOM_IOC_BUS_NUM, IOM_IOC_SLAVE_ADDRESS, pwrite_buf, 19);
  if (ret < 0) {
    return -1;
  }
  ret = mctp_i2c_read(IOM_IOC_BUS_NUM, IOM_IOC_SLAVE_ADDRESS, readd, 72);
  if (ret < 0) {
    return -1;
  }

  return 0;
}

int mctp_get_iom_ioc_ver(uint8_t *value) {
  static uint8_t tag_num = 0;
  uint8_t pwrite_buf[72];
  uint8_t readd[80];
  int reading = -1;
  int i = 0, ret = 0, retry = 0;

  memset(pwrite_buf, 0, 72);

  while(retry <= MAX_MCTP_RETRY_CNT) {
    pwrite_buf[0] = 0xf;
    pwrite_buf[1] = 0x1D;
    pwrite_buf[2] = 0x01;
    pwrite_buf[3] = 0x01;
    pwrite_buf[4] = 0x00;
    pwrite_buf[5] = 0x00;
    pwrite_buf[6] = 0xc8;
    pwrite_buf[7] = 0x7e;
    pwrite_buf[8] = 0x10;
    pwrite_buf[9] = 0x00;
    pwrite_buf[10] = 0x20; /* Payload ID */
    pwrite_buf[11] = 0x00;
    pwrite_buf[12] = 0x0;
    pwrite_buf[13] = tag_num; /* App Msg tag */
    pwrite_buf[14] = 0x05; /* Command         Issue an MPI request */
    pwrite_buf[15] = 0x00; /* Payload length */
    pwrite_buf[16] = 0x0C;
    pwrite_buf[17] = 0x00; /*Response length */
    pwrite_buf[18] = 0x3C;
    pwrite_buf[19] = 0x00;
    pwrite_buf[20] = 0x00;
    pwrite_buf[21] = 0x00; /*ChainOffset*/
    pwrite_buf[22] = 0x03; /* Function code */
    pwrite_buf[23] = 0x00;
    pwrite_buf[24] = 0x00;
    pwrite_buf[25] = 0x00;
    pwrite_buf[26] = 0x00; /* MsgFlags */
    pwrite_buf[27] = 0x00; /* VP_ID */
    pwrite_buf[28] = 0x00; /* VF_ID */
    pwrite_buf[29] = 0x00;
    pwrite_buf[30] = 0x00;

    ret = mctp_i2c_write(IOM_IOC_BUS_NUM, IOM_IOC_SLAVE_ADDRESS, pwrite_buf, 31);
    if (ret < 0) {
      retry++;
      continue;
    }

    memset(readd, 0, 80);

    ret = mctp_i2c_read(IOM_IOC_BUS_NUM, IOM_IOC_SLAVE_ADDRESS, readd, 72);
    // If any of the version register byte is 0xff, init and retry
    if (readd[51] == 0xff || readd[52] == 0xff || readd[53] == 0xff || readd[54] == 0xff || ret != 0) {

      retry++;

      if (retry <= MAX_MCTP_RETRY_CNT) {
        tag_num = tag_num + 1;
        if (tag_num == 0)
        tag_num = 1;
        
        ret = mctp_ioc_init(tag_num);
        if (ret != 0) {
          #ifdef DEBUG
            syslog(LOG_WARNING, "IOC init failed for Get IOC version.");
          #endif
        }
      }

      #ifdef DEBUG
        syslog(LOG_WARNING, "GET IOC Version Failed. Retry: %d\n", retry);
      #endif
      continue;
    }

    break;
  }

  if (retry <= MAX_MCTP_RETRY_CNT) {
    memcpy(value, readd + 51, 4);
    return 0;
  } else
      return -1;
}

int mctp_get_iom_ioc_temp(float *value) {
  static uint8_t tag_num = 0;
  uint8_t pwrite_buf[72];
  uint8_t readd[80];
  int i = 0, ret = 0, retry = 0;

  memset(pwrite_buf, 0, 72);

  while(retry <= MAX_MCTP_RETRY_CNT) {
    pwrite_buf[0] = 0xf;
    pwrite_buf[1] = 0x21;
    pwrite_buf[2] = 0x1;
    pwrite_buf[3] = 0x01;
    pwrite_buf[4] = 0x00;
    pwrite_buf[5] = 0x00;
    pwrite_buf[6] = 0xc8;
    pwrite_buf[7] = 0x7e;
    pwrite_buf[8] = 0x10;
    pwrite_buf[9] = 0x00;
    pwrite_buf[10] = 0x20;  /*  Payload ID */
    pwrite_buf[11] = 0x00;
    pwrite_buf[12] = 0x0;
    pwrite_buf[13] = tag_num;  /* App Msg tag */
    pwrite_buf[14] = 0x05;  /* Command  Issue an MPI request */
    pwrite_buf[15] = 0x0;   /* Payload length */
    pwrite_buf[16] = 0x14;
    pwrite_buf[17] = 0x0;
    pwrite_buf[18] = 0x3C;
    pwrite_buf[19] = 0x01;
    pwrite_buf[20] = 0x0;
    pwrite_buf[21] = 0x0;
    pwrite_buf[22] = 0x04;
    pwrite_buf[23] = 0x0;   /* ExtpageLength */
    pwrite_buf[24] = 0x0;
    pwrite_buf[25] = 0x0;
    pwrite_buf[26] = 0x0;
    pwrite_buf[27] = 0x02;  /* Page Version */
    pwrite_buf[28] = 0x0a;  /* Page Length */
    pwrite_buf[29] = 0x07;  /* Page Number */
    pwrite_buf[30] = 0x00;  /* Page Type */
    pwrite_buf[31] = 0x0;
    pwrite_buf[32] = 0x0;
    pwrite_buf[33] = 0x0;
    pwrite_buf[34] = 0x0;

    ret = mctp_i2c_write(IOM_IOC_BUS_NUM, IOM_IOC_SLAVE_ADDRESS, pwrite_buf, 35);
    if (ret < 0) {
      retry++;
      continue;
    }

    memset(readd, 0, 80);

    ret = mctp_i2c_read(IOM_IOC_BUS_NUM, IOM_IOC_SLAVE_ADDRESS, readd, 72);
    // When any of the temp register byte is 0xff, init and retry
    if (readd[55] == 0xff || readd[56] == 0xff  || ret != 0) {

      retry++;

      if (retry <= MAX_MCTP_RETRY_CNT) {
        tag_num = tag_num + 1;
        if (tag_num == 0)
        tag_num = 1;

        ret = mctp_ioc_init(tag_num);
        if ( ret != 0) {
          #ifdef DEBUG
            syslog(LOG_WARNING, "IOC init failed for Get IOC version.");
          #endif
        }
      }

      #ifdef DEBUG
        syslog(LOG_WARNING, "GET IOC Temp Failed. Retry: %d\n", retry);
      #endif
      continue;
    }

    break;
  }

  if (retry <= MAX_MCTP_RETRY_CNT) {
    *value = readd[55] + (readd[56] * 256);
    return 0;
  } else
    return -1;
}
