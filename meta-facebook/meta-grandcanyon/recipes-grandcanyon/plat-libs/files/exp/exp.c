/*
 *
 * Copyright 2020-present Facebook. All Rights Reserved.
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
#include <string.h>
#include <time.h>
#include <facebook/fbgc_common.h>
#include "exp.h"

int
expander_ipmb_wrapper(uint8_t netfn, uint8_t cmd, uint8_t *txbuf, uint8_t txlen, uint8_t *rxbuf, uint8_t *rxlen) {
  ipmb_req_t *req;
  ipmb_res_t *res;
  uint8_t rbuf[MAX_IPMB_RES_LEN] = {0};
  uint8_t tbuf[MAX_IPMB_RES_LEN] = {0};
  uint8_t tlen = 0;
  uint8_t rlen = 0;
  int retry = 0;
  int ret = 0;

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
    ret = lib_ipmb_handle(EXPANDER_IPMB_BUS_NUM, tbuf, tlen, rbuf, &rlen);
    if (ret < 0) {
      syslog(LOG_ERR, "%s: Failed to send ipmb request, ret:%d netfn:%u cmd:%u\n", __func__, ret, netfn, cmd);
    }
    if (rlen == 0) {
      retry++;
      msleep(20);
    }
    else {
      break;
    }
  }

  if (rlen == 0) {
    syslog(LOG_DEBUG, "%s: Zero bytes received, netfn:%u cmd:%u retry:%d\n", __func__, netfn, cmd, retry);
    return -1;
  }

  // Handle IPMB response
  res = (ipmb_res_t*) rbuf;

  if (res->cc) {
    syslog(LOG_ERR, "%s: netfn:%u cmd:%u Completion Code: 0x%X\n", __func__, netfn, cmd, res->cc);
    return -1;
  }

  // copy the received data back to caller
  *rxlen = rlen - IPMB_HDR_SIZE - IPMI_RESP_HDR_SIZE;
  memcpy(rxbuf, res->data, *rxlen);

  return 0;
}


