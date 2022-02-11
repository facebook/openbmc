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
#include <fcntl.h>
#include <syslog.h>
#include <errno.h>
#include <sys/stat.h>
#include "exp.h"

static void
msleep(int msec) {
  struct timespec req;

  req.tv_sec = 0;
  req.tv_nsec = msec * 1000 * 1000;

  while(nanosleep(&req, &req) == -1 && errno == EINTR) {
    continue;
  }
}

int
expander_ipmb_wrapper(uint8_t netfn, uint8_t cmd, uint8_t *txbuf, uint8_t txlen, uint8_t *rxbuf, uint8_t *rxlen) {
  ipmb_req_t *req;
  ipmb_res_t *res;
  uint8_t rbuf[MAX_IPMB_RES_LEN] = {0};
  uint8_t tbuf[MAX_IPMB_RES_LEN] = {0};
  uint8_t tlen = 0;
  uint8_t rlen = 0;
  int count = 0;
  int i = 0;
  int ret;
  int retry = 0;

  req = (ipmb_req_t*)tbuf;

  req->res_slave_addr = EXPANDER_SLAVE_ADDR << 1;
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

  while(retry < 5) {
    // Invoke IPMB library handler
    lib_ipmb_handle(EXPANDER_IPMB_BUS_NUM, tbuf, tlen, &rbuf, &rlen);

    if (rlen == 0) {
      retry++;
      msleep(20);
    }
    else {
      break;
    }
  }

  if (rlen == 0) {
#ifdef DEBUG
    syslog(LOG_DEBUG, "%s: Zero bytes received, retry:%d\n", __func__, retry);
#endif
    return -1;
  }

  // Handle IPMB response
  res  = (ipmb_res_t*) rbuf;

  if (res->cc) {
    syslog(LOG_ERR, "%s: Completion Code: 0x%X\n", __func__, res->cc);
    return -1;
  }

  // copy the received data back to caller
  *rxlen = rlen - IPMB_HDR_SIZE - IPMI_RESP_HDR_SIZE;
  memcpy(rxbuf, res->data, *rxlen);

  return 0;
}

// Read Firwmare Versions of Expander via IPMB, and save to cache
int
exp_get_fw_ver(uint8_t *ver) {
  uint8_t tbuf[256] = {0x00};
  uint8_t rbuf[256] = {0x00};
  uint8_t rlen = 0;
  uint8_t tlen = 0;
  int fw_select_shift = 0;
  int ret;

  ret = expander_ipmb_wrapper(NETFN_OEM_REQ, CMD_GET_EXP_VERSION, tbuf, tlen, rbuf, &rlen);
  if (ret) {
    syslog(LOG_ERR, "%s: expander_ipmb_wrapper failed...\n", __func__);
    return -1;
  }

  if(!rbuf[5]) //the 5th byte is FW 1 selecte byte, the 10th byte is backup FW select byte
    fw_select_shift = 5;

  memcpy(ver, &rbuf[6 + fw_select_shift], 4);

  return 0;
}

// Read Firwmare Versions of IOC via IPMB, and save to cache
int
exp_get_ioc_fw_ver(uint8_t *ver) {
  uint8_t tbuf[256] = {0x00};
  uint8_t rbuf[256] = {0x00};
  uint8_t rlen = 0;
  uint8_t tlen = 0;
  int ret;

  ret = expander_ipmb_wrapper(NETFN_OEM_REQ, CMD_GET_IOC_VERSION, tbuf, tlen, rbuf, &rlen);
  if (ret) {
    syslog(LOG_ERR, "%s: expander_ipmb_wrapper failed...\n", __func__);
    return -1;
  }

  memcpy(ver, rbuf, 4);

  return 0;
}

int
exp_read_fruid(const char *path, unsigned char FRUID) {
  int fd;
  uint8_t tbuf[256] = {0x00};
  uint8_t rbuf[256] = {0x00};
  uint8_t r1_buf[256] = {0x00};
  uint8_t abuf[512] = {0x0};
  uint8_t rlen = 0;
  uint8_t tlen = 0;
  uint8_t l_rlen = 0;
  int ret;
  int i;

  tbuf[0] = FRUID; //FRU Device ID
  tbuf[1] = 0; //FRU Inventory Offset to read, LS Byte
  tbuf[2] = 0; //FRU Inventory Offset to read, MS Byte
  tbuf[3] = EXPANDER_FRUID_SIZE + 2; //Count to read --- count is 1 based
  tlen = 4;
  ret = expander_ipmb_wrapper(NETFN_STORAGE_REQ, CMD_GET_EXP_FRUID, tbuf, tlen, rbuf, &rlen);
  if (ret) {
    syslog(LOG_ERR, "%s: first half failed for fru:%d\n", __func__, FRUID);
    return -1;
  }

  memcpy( abuf, rbuf + 1, rlen-1);
  l_rlen = rlen - 1;
  memset(rbuf,0,512);

  tbuf[1] = EXPANDER_FRUID_SIZE + 2; //FRU Inventory Offset to read, LS Byte
  tbuf[2] = 0; //FRU Inventory Offset to read, MS Byte
  ret = expander_ipmb_wrapper(NETFN_STORAGE_REQ, CMD_GET_EXP_FRUID, tbuf, tlen, r1_buf, &rlen);
  if (ret) {
    syslog(LOG_ERR, "%s: second half failed for fru:%d\n", __func__, FRUID);
    return -1;
  }

  memcpy( abuf + l_rlen, r1_buf + 1, rlen - 1);

  // Remove the file if exists already
  unlink(path);

  // Open the file exclusively for write
  fd = open(path, O_WRONLY | O_CREAT | O_EXCL, 0666);
  if (fd < 0) {
#ifdef DEBUG
    syslog(LOG_ERR, "%s: open failed for path: %s\n", __func__, path);
#endif
    return -1;
  }
  // Ignore the first byte as it indicates length of response
  write(fd, abuf, l_rlen + rlen - 2);

  close(fd);
  return 0;
}

