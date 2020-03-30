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
#include <stdlib.h>
#include <stdint.h>
#include <errno.h>
#include <string.h>
#include <syslog.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <pthread.h>
#include <stdarg.h>
#include <unistd.h>
#include <openbmc/ipc.h>
#include "ipmb.h"
#include <openbmc/ipmi.h>

static pthread_key_t rxkey, txkey;
static pthread_once_t key_once = PTHREAD_ONCE_INIT;

static void
destructor(void *buf)
{
  if (buf)
    free(buf);
}

static void
make_key()
{
  (void) pthread_key_create(&rxkey, destructor);
  (void) pthread_key_create(&txkey, destructor);
}

/*
 *  Return thread specific ipmb rx buffer
 */
ipmb_res_t*
ipmb_rxb()
{
  ipmb_res_t *buf = NULL;

  pthread_once(&key_once, make_key);
  if ((buf = pthread_getspecific(rxkey)) == NULL) {
    buf = malloc(MAX_IPMB_RES_LEN);
    pthread_setspecific(rxkey, buf);
  }

  return (ipmb_res_t*)buf;
}

/*
 *  Return thread specific ipmb tx buffer
 */
ipmb_req_t*
ipmb_txb()
{
  void *buf = NULL;

  pthread_once(&key_once, make_key);
  if ((buf = pthread_getspecific(txkey)) == NULL) {
    buf = malloc(MAX_IPMB_RES_LEN);
    pthread_setspecific(txkey, buf);
  }

  return (ipmb_req_t*)buf;
}

/*
 * Function to handle IPMB messages
 */
int
lib_ipmb_handle(unsigned char bus_id,
            unsigned char *request, unsigned short req_len,
            unsigned char *response, unsigned char *res_len) {

  size_t resp_len = MAX_IPMB_RES_LEN;
  char sock_path[64];

  sprintf(sock_path, "%s_%d", SOCK_PATH_IPMB, bus_id);

  if (ipc_send_req(sock_path, request, (size_t)req_len, response,
                   &resp_len, TIMEOUT_IPMB) != 0) {
    return -1;
  }

  *res_len = (unsigned char)resp_len;
  return 0;
}

int
ipmb_send_buf (unsigned char bus_id, unsigned char tlen)
{
  unsigned char rlen = 0;

  lib_ipmb_handle(bus_id,
    (unsigned char *)ipmb_txb(), tlen,
    (unsigned char *)ipmb_rxb(), &rlen);

  if (rlen >= MIN_IPMB_RES_LEN)
    return (int)rlen;
  else
    return -1;
}

int
ipmb_send_internal (unsigned char bus_id,
  unsigned char addr,
  unsigned char netfn,
  unsigned char cmd, ...)
{
  unsigned char *tbuf;
  unsigned char *rbuf;
  unsigned char tlen = 0;
  unsigned char rlen = 0;
  int data_len;
  ipmb_req_t *req;
  int data;
  va_list dl; // data list

  // Get thread specific buffers
  tbuf = (unsigned char *)ipmb_txb();
  rbuf = (unsigned char *)ipmb_rxb();
  if (rbuf == NULL || tbuf == NULL)
    return -1;

  req = (ipmb_req_t*)tbuf;

  req->res_slave_addr = addr;
  req->netfn_lun = netfn;
  req->cmd = cmd;

  tlen = 0;
  va_start(dl, cmd);
  while ((data = va_arg(dl, int)) != OBMC_API_IPMB_CMD_END) {
    if (tlen == (MAX_IPMB_RES_LEN - MIN_IPMB_REQ_LEN)) {
      va_end(dl);
      return -1;
    }
    req->data[tlen++] = (uint8_t)data;
  }
  va_end(dl);
  tlen += MIN_IPMB_REQ_LEN;

  rlen = 0;
  lib_ipmb_handle(bus_id, tbuf, tlen, rbuf, &rlen);

  if (rlen >= MIN_IPMB_RES_LEN) {
    data_len = rlen - MIN_IPMB_RES_LEN;
  } else {
    data_len = -1;
  }

  return data_len;
}

int
lib_ipmb_send_request(uint8_t ipmi_cmd, uint8_t netfn,
              uint8_t *txbuf, uint8_t txlen, 
              uint8_t *rxbuf, uint8_t *rxlen,
              uint8_t bus_num, uint8_t dev_addr, uint8_t bmc_addr) {
  uint8_t rdata[MAX_IPMB_RES_LEN] = {0x00};
  uint8_t wdata[MAX_IPMB_RES_LEN] = {0x00};
  uint8_t tlen = 0;
  uint8_t rlen = 0;
  ipmb_req_t *req;
  ipmb_res_t *res;
  int ret=0;

  req = (ipmb_req_t*) wdata;

  req->res_slave_addr = dev_addr;
  req->netfn_lun = netfn << LUN_OFFSET;
  req->cmd = ipmi_cmd;
  req->seq_lun = 0x00;
  req->hdr_cksum = req->res_slave_addr + req->netfn_lun;
  req->hdr_cksum = ZERO_CKSUM_CONST - req->hdr_cksum;
  
  req->req_slave_addr = bmc_addr << 1;

  if (txlen) {
    memcpy(req->data, txbuf, txlen);
  }
  tlen = MIN_IPMB_REQ_LEN + txlen;

  // Invoke IPMB library handler
  lib_ipmb_handle(bus_num, wdata, tlen, rdata, &rlen);
  res = (ipmb_res_t*) rdata;

  if (rlen == 0) {
    syslog(LOG_DEBUG, "%s: Zero bytes received cc=0x%x\n", __func__, res->cc);
    return CC_CAN_NOT_RESPOND;
  }

  // Handle IPMB response
  if (res->cc) {
    syslog(LOG_ERR, "%s: Completion Code: 0x%X\n", __func__, res->cc);
    return res->cc;
  }

  // copy the received data back to caller
  *rxlen = rlen - IPMB_HDR_SIZE - IPMI_RESP_HDR_SIZE;
  memcpy(rxbuf, res->data, *rxlen);

#ifdef DEBUG
  {
    for(int i=0; i< *rxlen; i++)
    syslog(LOG_WARNING, "rbuf[%d]=%x\n", i, rxbuf[i]);
  }
#endif

  return CC_SUCCESS;
}
