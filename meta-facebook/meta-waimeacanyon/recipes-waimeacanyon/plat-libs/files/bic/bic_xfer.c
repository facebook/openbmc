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


#define RETRY_TIME 3

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
  // uint8_t status_12v = 0;

  // ret = fbgc_common_server_stby_pwr_sts(&status_12v);
  // if (ret < 0) {
  //   return BIC_STATUS_FAILURE;
  // }
  // 
  // if (status_12v == STAT_12V_OFF) {
  //   return BIC_STATUS_12V_OFF;
  // }

  bus_id = I2C_BUS_MB_BIC;

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
  if ((txbuf != NULL) && (txlen != 0)) {
    memcpy(req->data, txbuf, txlen);
  }

  tlen = IPMB_HDR_SIZE + IPMI_REQ_HDR_SIZE + txlen;

  while (retry < RETRY_TIME) {
    // Invoke IPMB library handler
    ret = lib_ipmb_handle(bus_id, tbuf, tlen, rbuf, &rlen);

    if (ret < 0 || rlen == 0) {
      retry++;
      usleep(IPMB_RETRY_DELAY_TIME);
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
bic_me_xmit(uint8_t slot_id, uint8_t *txbuf, uint8_t txlen, uint8_t *rxbuf, uint8_t *rxlen) {
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
