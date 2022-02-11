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
#include <unistd.h>
#include "exp.h"

#define CMD_BUF_SIZE           4
#define FRUID_DATA_SIZE        512
#define MAX_FRUID_QUERY_SIZE   32

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
    ret = lib_ipmb_handle(I2C_EXP_BUS, tbuf, tlen, rbuf, &rlen);
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

// Read Firwmare Versions of Expander via IPMB, and save to cache
int
expander_get_fw_ver(uint8_t *ver, uint8_t ver_len) {
  uint8_t tbuf[MAX_IPMB_RES_LEN] = {0x00};
  uint8_t rbuf[MAX_IPMB_RES_LEN] = {0x00};
  uint8_t rlen = 0;
  uint8_t tlen = 0;
  int ret = 0;
  exp_ver expander_ver;

  if (ver == NULL) {
    syslog(LOG_ERR, "%s: Firmware version pointer is null...\n", __func__);
    return -1;
  }

  memset(&expander_ver, 0, sizeof(expander_ver));

  ret = expander_ipmb_wrapper(NETFN_OEM_REQ, CMD_GET_EXP_VERSION, tbuf, tlen, rbuf, &rlen);
  if ((ret != 0) || (rlen != EXP_VERSION_RES_LEN)) {
    syslog(LOG_ERR, "%s: expander_ipmb_wrapper failed...\n", __func__);
    return -1;
  }

  memcpy(&expander_ver, rbuf, sizeof(expander_ver));

  // Use the region of expander firmware version if the status is active.
  if (expander_ver.firmware_region1.status == EXP_FW_VERSION_ACTIVATE) {
    memcpy(ver, &expander_ver.firmware_region1.major_ver, ver_len);
  } else {
    memcpy(ver, &expander_ver.firmware_region2.major_ver, ver_len);
  }

  return 0;
}

int
exp_read_fruid(const char *path, uint8_t fru_id) {
  uint8_t tbuf[CMD_BUF_SIZE] = {0x00};
  uint8_t rbuf[FRUID_DATA_SIZE] = {0x00};
  uint8_t fruid_buf[FRUID_DATA_SIZE] = {0x00};
  uint8_t rlen = 0;
  int fd = 0;
  int ret = 0;
  int remain = FRUID_DATA_SIZE;
  int index = 0;
  ssize_t write_length = 0;
  ExpanderGetFruidCommand *cmd = NULL;

  cmd = (ExpanderGetFruidCommand*)tbuf;
  cmd->fruid = fru_id;
  cmd->query_size = MAX_FRUID_QUERY_SIZE;

  while (remain > 0) {
    cmd->offset_low = (index & 0xFF);
    cmd->offset_high = (index >> 8) & 0xFF;
    ret = expander_ipmb_wrapper(NETFN_STORAGE_REQ, CMD_GET_EXP_FRUID, tbuf, sizeof(tbuf), rbuf, &rlen);
    if (ret < 0) {
      syslog(LOG_WARNING, "%s() expander_ipmb_wrapper failed. ret: %d\n", __func__, ret);
      goto end;
    }
    if (rlen != MAX_FRUID_QUERY_SIZE + 1) {
      syslog(LOG_WARNING, "%s() expander_ipmb_wrapper incorrect rlen: %d\n", __func__, rlen);
      ret = -1;
      goto end;
    }
    memcpy(fruid_buf + index, rbuf + 1, MAX_FRUID_QUERY_SIZE);
    index += MAX_FRUID_QUERY_SIZE;
    remain-= MAX_FRUID_QUERY_SIZE;
  }

  // Remove the file if exists already
  unlink(path);

  // Open the file exclusively for write
  fd = open(path, O_WRONLY | O_CREAT | O_EXCL, 0666);
  if (fd < 0) {
    syslog(LOG_ERR, "%s: open: %s\n", __func__, strerror(errno));
    return -1;
  }
  write_length = write(fd, fruid_buf, FRUID_DATA_SIZE);
  if (write_length != FRUID_DATA_SIZE) {
    syslog(LOG_ERR, "%s: write fruid failed, len: %d, error: %s\n", __func__, write_length, strerror(errno));
    ret = -1;
  }

  close(fd);
end:

  return ret;
}
