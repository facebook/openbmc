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
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <fcntl.h>
#include <syslog.h>
#include <errno.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <openbmc/obmc-i2c.h>
#include <openbmc/libgpio.h>
#include "bic_xfer.h"
#include <facebook/fbgc_gpio.h>


#define RETRY_TIME 3

#define I2C_IO_MSG_CNT 2

const uint32_t IANA_ID = 0x009C9C;


int
i2c_open(uint8_t bus_id, uint8_t addr_7bit) {
  int fd = -1;

  fd = i2c_cdev_slave_open(bus_id, addr_7bit, I2C_SLAVE_FORCE_CLAIM);
  if ( fd < 0 ) {
    syslog(LOG_ERR, "Failed to open I2C bus %d\n", bus_id);
    return BIC_STATUS_FAILURE;
  }

  return fd;
}

int i2c_io(int fd, uint8_t *tbuf, uint8_t tcount, uint8_t *rbuf, uint8_t rcount) {
  struct i2c_rdwr_ioctl_data data;
  struct i2c_msg msg[I2C_IO_MSG_CNT];
  int n_msg = 0;
  int rc = 0;

  memset(&msg, 0, sizeof(msg));

  if (tcount != 0) {
    msg[n_msg].addr = BRIDGE_SLAVE_ADDR;
    msg[n_msg].flags = 0;
    msg[n_msg].len = tcount;
    msg[n_msg].buf = tbuf;
    n_msg++;
  }

  if (rcount != 0) {
    msg[n_msg].addr = BRIDGE_SLAVE_ADDR;
    msg[n_msg].flags = I2C_M_RD;
    msg[n_msg].len = rcount;
    msg[n_msg].buf = rbuf;
    n_msg++;
  }

  data.msgs = msg;
  data.nmsgs = n_msg;

  rc = ioctl(fd, I2C_RDWR, &data);
  if (rc < 0) {
    syslog(LOG_ERR, "Failed to do raw io");
    return BIC_STATUS_FAILURE;
  }

  return BIC_STATUS_SUCCESS;
}

int
is_bic_ready() {
  int ret = GPIO_VALUE_INVALID;

  ret = gpio_get_value_by_shadow(fbgc_get_gpio_name(GPIO_BIC_READY_IN));
  if (ret != GPIO_VALUE_HIGH) {
    return STATUS_BIC_NOT_READY;
  } else {
    return STATUS_BIC_READY;
  }  
}

int
bic_ipmb_wrapper(uint8_t netfn, uint8_t cmd,
                     uint8_t *txbuf, uint16_t txlen, uint8_t *rxbuf, uint8_t *rxlen) {
  ipmb_req_t *req = NULL;
  ipmb_res_t *res = NULL;
  uint8_t rbuf[MAX_IPMB_RES_LEN] = {0};
  uint8_t tbuf[MAX_IPMB_REQ_LEN] = {0};
  uint16_t tlen = 0;
  uint8_t rlen = 0;
  int i = 0;
  int ret = 0;
  uint8_t bus_id = 0;
  uint8_t dataCksum = 0;
  int retry = 0;
  uint8_t status_12v = 0;

  ret = fbgc_common_server_stby_pwr_sts(&status_12v);
  if (ret < 0) {
    return BIC_STATUS_FAILURE;
  }

  if (status_12v == STAT_12V_OFF) {
    return BIC_STATUS_12V_OFF;
  }

  bus_id = I2C_BIC_BUS;

  req = (ipmb_req_t*)tbuf;

  req->res_slave_addr = BRIDGE_SLAVE_ADDR << 1;
  req->netfn_lun = netfn << LUN_OFFSET;
  req->hdr_cksum = req->res_slave_addr +
                  req->netfn_lun;
  req->hdr_cksum = ZERO_CKSUM_CONST - req->hdr_cksum;

  req->req_slave_addr = BMC_SLAVE_ADDR << 1;
  req->seq_lun = 0x00;
  req->cmd = cmd;

  //copy the data to be send
  if (txlen != 0) {
    memcpy(req->data, txbuf, txlen);
  }

  tlen = IPMB_HDR_SIZE + IPMI_REQ_HDR_SIZE + txlen;

  while (retry < RETRY_TIME) {
    // Invoke IPMB library handler
    ret = lib_ipmb_handle(bus_id, tbuf, tlen, rbuf, &rlen);

    if (ret < 0 || rlen == 0) {
      retry++;
      msleep(IPMB_RETRY_DELAY_TIME);
    }
    else {
      break;
    }
  }

  if (rlen == 0) {
    syslog(LOG_ERR, "bic_ipmb_wrapper: Zero bytes received, retry:%d, cmd:%x\n", retry, cmd);
    return BIC_STATUS_FAILURE;
  }

  // Handle IPMB response
  res = (ipmb_res_t*) rbuf;

  if (res->cc) {
    syslog(LOG_ERR, "bic_ipmb_wrapper: Completion Code: 0x%X\n", res->cc);
    return BIC_STATUS_FAILURE;
  }

  // copy the received data back to caller
  *rxlen = rlen - IPMB_HDR_SIZE - IPMI_RESP_HDR_SIZE;
  memcpy(rxbuf, res->data, *rxlen);

  // Calculate dataCksum
  // Note: dataCkSum byte is last byte
  dataCksum = 0;
  for (i = IPMB_DATA_OFFSET; i < (rlen - 1); i++) {
    dataCksum += rbuf[i];
  }
  dataCksum = ZERO_CKSUM_CONST - dataCksum;

  if (dataCksum != rbuf[rlen - 1]) {
    syslog(LOG_ERR, "%s: Receive Data cksum does not match (expectative 0x%x, actual 0x%x)", __func__, dataCksum, rbuf[rlen - 1]);
    return BIC_STATUS_FAILURE;
  }

  return BIC_STATUS_SUCCESS;
}

int
open_and_get_size(char *path, int *file_size) {
  struct stat finfo;
  int fd = 0;

  fd = open(path, O_RDONLY, 0666);
  if (fd < 0) {
    return -1;
  }
  fstat(fd, &finfo);
  *file_size = finfo.st_size;

  return fd;
}

int
send_image_data_via_bic(uint8_t comp, uint32_t offset, uint16_t len, uint32_t image_len, uint8_t *buf) {
  uint8_t tbuf[MAX_IPMB_BUFFER] = {0};
  uint8_t rbuf[MAX_IPMB_BUFFER] = {0};
  uint8_t tlen = 0;
  uint8_t rlen = 0;
  int ret = -1;
  int retries = MAX_RETRY;

  memcpy(tbuf, (uint8_t *)&IANA_ID, 3);
  tbuf[3] = comp;
  tbuf[4] = (offset) & 0xFF;
  tbuf[5] = (offset >>  8) & 0xFF;
  tbuf[6] = (offset >> 16) & 0xFF;
  tbuf[7] = (offset >> 24) & 0xFF;
  tbuf[8] = len & 0xFF;
  tbuf[9] = (len >> 8) & 0xFF;
  if (image_len > 0) {
    tbuf[10] = (image_len) & 0xFF;
    tbuf[11] = (image_len >> 8) & 0xFF;
    tbuf[12] = (image_len >> 16) & 0xFF;
    tbuf[13] = (image_len >> 24) & 0xFF;
    memcpy(&tbuf[14], buf, len);
    tlen = len + 14;
  } else {
    memcpy(&tbuf[10], buf, len);
    tlen = len + 10;
  }

  do {
    ret = bic_ipmb_wrapper(NETFN_OEM_1S_REQ, CMD_OEM_1S_UPDATE_FW, tbuf, tlen, rbuf, &rlen);
    if (ret != BIC_STATUS_SUCCESS) {
      if (ret == BIC_STATUS_NOT_SUPP_IN_CURR_STATE) {
        return ret;
      }
      printf("%s() target: %d, offset: %d, len: %d retrying..\n", __func__, comp, offset, len);
    }
  } while((ret < 0) && (retries--));

  return ret;
}

int
_set_fw_update_ongoing(uint16_t tmout) {
  char *key = "fwupd";
  char value[MAX_VALUE_LEN] = {0};
  struct timespec ts;

  clock_gettime(CLOCK_MONOTONIC, &ts);
  ts.tv_sec += tmout;
  snprintf(value, sizeof(value), "%ld", ts.tv_sec);

  if (kv_set(key, value, 0, 0) < 0) {
     return -1;
  }

  return 0;
}

int
bic_me_xmit(uint8_t *txbuf, uint8_t txlen, uint8_t *rxbuf, uint8_t *rxlen) {
  uint8_t tbuf[MAX_IPMB_REQ_LEN] = {0x00};
  uint8_t rbuf[MAX_IPMB_RES_LEN] = {0x00};
  uint8_t rlen = 0;
  uint8_t tlen = 0;
  int ret = 0;

  // Fill the IANA ID
  memcpy(tbuf, (uint8_t *)&IANA_ID, 3);

  // Fill the interface number as ME
  tbuf[3] = BIC_INTF_ME;

  // Fill the data to be sent
  memcpy(&tbuf[4], txbuf, txlen);

  // Send data length includes IANA ID and interface number
  tlen = txlen + 4;

  ret = bic_ipmb_wrapper(NETFN_OEM_1S_REQ, CMD_OEM_1S_MSG_OUT, tbuf, tlen, rbuf, &rlen);
  if (ret != 0) {
    return BIC_STATUS_FAILURE;
  }

  // Make sure the received interface number is same
  if (rbuf[3] != tbuf[3]) {
    return BIC_STATUS_FAILURE;
  }

  // Copy the received data to caller skipping header
  memcpy(rxbuf, &rbuf[6], rlen - 6);

  *rxlen = rlen - 6;

  return BIC_STATUS_SUCCESS;
}
