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

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(a)   (sizeof(a) / sizeof((a)[0]))
#endif

const uint32_t IANA_ID = 0x009C9C;

void msleep(int msec) {
  struct timespec req;

  req.tv_sec = 0;
  req.tv_nsec = msec * 1000 * 1000;

  while(nanosleep(&req, &req) == -1 && errno == EINTR) {
    continue;
  }
}

int i2c_open(uint8_t bus_id) {
  int fd;
  char fn[32];
  int rc;

  snprintf(fn, sizeof(fn), "/dev/i2c-%d", bus_id);
  fd = open(fn, O_RDWR);
  if (fd == -1) {
    syslog(LOG_ERR, "Failed to open i2c device %s", fn);
    return BIC_STATUS_FAILURE;
  }

  rc = ioctl(fd, I2C_SLAVE, BRIDGE_SLAVE_ADDR);
  if (rc < 0) {
    syslog(LOG_ERR, "Failed to open slave @ address 0x%x", BRIDGE_SLAVE_ADDR);
    close(fd);
  }

  return fd;
}

int i2c_io(int fd, uint8_t *tbuf, uint8_t tcount, uint8_t *rbuf, uint8_t rcount) {
  struct i2c_rdwr_ioctl_data data;
  struct i2c_msg msg[2];
  int n_msg = 0;
  int rc;

  memset(&msg, 0, sizeof(msg));

  if (tcount) {
    msg[n_msg].addr = BRIDGE_SLAVE_ADDR;
    msg[n_msg].flags = 0;
    msg[n_msg].len = tcount;
    msg[n_msg].buf = tbuf;
    n_msg++;
  }

  if (rcount) {
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

int is_bic_ready(uint8_t slot_id, uint8_t intf) {
  //TODO: 1. check the ready pin of server bic by reading GPIO
  //send a command to BIC2 to see if it can be reached
  return BIC_STATUS_SUCCESS;
}

// Repack data according to the interface
int
bic_ipmb_send(uint8_t slot_id, uint8_t netfn, uint8_t cmd, uint8_t *tbuf, uint8_t tlen, uint8_t *rbuf, uint8_t *rlen, uint8_t intf) {
  int ret = 0;
  uint8_t tmp_buf[MAX_IPMB_RES_LEN] = {0x0};
  uint8_t rsp_buf[MAX_IPMB_RES_LEN] = {0x0};
  uint8_t tmp_len = 0;
  uint8_t rsp_len = 0;

  switch(intf) {
    case NONE_INTF:
      ret = bic_ipmb_wrapper(slot_id, netfn, cmd, tbuf, tlen, rbuf, rlen);
      break;
    case FEXP_BIC_INTF:
    case BB_BIC_INTF:
    case REXP_BIC_INTF:
      if ( tlen + MIN_IPMB_REQ_LEN + MIN_IPMB_BYPASS_LEN > MAX_IPMB_RES_LEN ) {
        syslog(LOG_WARNING, "%s() xfer length is too long. len=%d, max=%d", __func__, (tlen + MIN_IPMB_REQ_LEN + MIN_IPMB_BYPASS_LEN), MAX_IPMB_RES_LEN);
        return BIC_STATUS_FAILURE;
      }

      tmp_len = 3;
      memcpy(tmp_buf, (uint8_t *)&IANA_ID, tmp_len);
      tmp_buf[tmp_len++] = intf;
      tmp_buf[tmp_len++] = netfn << 2;
      tmp_buf[tmp_len++] = cmd;
      memcpy(&tmp_buf[tmp_len], tbuf, tlen);
      tmp_len += tlen;

      ret = bic_ipmb_wrapper(slot_id, NETFN_OEM_1S_REQ, CMD_OEM_1S_MSG_OUT, tmp_buf, tmp_len, rsp_buf, &rsp_len);
      //rsp_buf[6] is the completion code
      if ( (ret < 0) || (ret == BIC_STATUS_SUCCESS && rsp_buf[6] != CC_SUCCESS) ) {
        syslog(LOG_WARNING, "%s() The 2nd BIC cannot be reached. CC: 0x%02X, intf: 0x%x, ret = %d\n", __func__, rsp_buf[6], intf, ret);
        switch(rsp_buf[6]) {
        case CC_NOT_SUPP_IN_CURR_STATE:
          ret = BIC_STATUS_NOT_SUPP_IN_CURR_STATE;
          break;
        default:
          ret = BIC_STATUS_FAILURE;
          break;
        }
      } else { 
        //catch the data and ignore the packet of the bypass command.
        *rlen = rsp_len - 7;
        memmove(rbuf, &rsp_buf[7], *rlen);
      }

      break;
  }
  return ret;
}

int bic_ipmb_wrapper(uint8_t slot_id, uint8_t netfn, uint8_t cmd,
                     uint8_t *txbuf, uint16_t txlen, uint8_t *rxbuf, uint8_t *rxlen) {
  ipmb_req_t *req;
  ipmb_res_t *res;
  uint8_t rbuf[MAX_IPMB_RES_LEN] = {0};
  uint8_t tbuf[MAX_IPMB_RES_LEN] = {0};
  uint16_t tlen = 0;
  uint8_t rlen = 0;
  int i = 0;
  int ret;
  uint8_t bus_id;
  uint8_t dataCksum;
  int retry = 0;
  uint8_t status_12v = 0;

#if 0
  //TODO: implement the is_bic_ready funcitons
  if (!is_bic_ready(slot_id)) {
    return -1;
  }
#endif

  // avoid waiting 5 seconds to do the retry
  ret = fby3_common_server_stby_pwr_sts(slot_id, &status_12v);
  if ( ret < 0 || status_12v == 0) {
    return -1;
  }

  ret = fby3_common_get_bus_id(slot_id);
  if (ret < 0) {
    syslog(LOG_ERR, "%s: Wrong Slot ID %d\n", __func__, slot_id);
    return ret;
  }

  bus_id = (uint8_t) ret;

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
  if (txlen) {
    memcpy(req->data, txbuf, txlen);
  }

  tlen = IPMB_HDR_SIZE + IPMI_REQ_HDR_SIZE + txlen;

  while(retry < RETRY_TIME) {
    // Invoke IPMB library handler
    lib_ipmb_handle(bus_id, tbuf, tlen, rbuf, &rlen);

    if (rlen == 0) {
#if 0
      //TODO: implement the is_bic_ready funcitons
      if (!is_bic_ready(slot_id)) {
        break;
      }
#endif
      retry++;
      msleep(IPMB_RETRY_DELAY_TIME);
    }
    else
      break;
  }

  if (rlen == 0) {
    syslog(LOG_ERR, "bic_ipmb_wrapper: Zero bytes received, retry:%d, cmd:%x\n", retry, cmd);
    return BIC_STATUS_FAILURE;
  }

  // Handle IPMB response
  res  = (ipmb_res_t*) rbuf;

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

int bic_me_xmit(uint8_t slot_id, uint8_t *txbuf, uint8_t txlen, uint8_t *rxbuf, uint8_t *rxlen) {
  uint8_t tbuf[256] = {0x00};
  uint8_t rbuf[256] = {0x00};
  uint8_t rlen = 0;
  uint8_t tlen = 0;
  int ret;

  // Fill the IANA ID
  memcpy(tbuf, (uint8_t *)&IANA_ID, 3);

  // Fill the interface number as ME
  tbuf[3] = BIC_INTF_ME;

  // Fill the data to be sent
  memcpy(&tbuf[4], txbuf, txlen);

  // Send data length includes IANA ID and interface number
  tlen = txlen + 4;

  ret = bic_ipmb_wrapper(slot_id, NETFN_OEM_1S_REQ, CMD_OEM_1S_MSG_OUT, tbuf, tlen, rbuf, &rlen);
  if (ret ) {
    return BIC_STATUS_FAILURE;
  }

  // Make sure the received interface number is same
  if (rbuf[3] != tbuf[3]) {
    return BIC_STATUS_FAILURE;
  }

  // Copy the received data to caller skipping header
  memcpy(rxbuf, &rbuf[6], rlen-6);

  *rxlen = rlen-6;

  return BIC_STATUS_SUCCESS;
}
