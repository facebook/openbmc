/*
 *
 * Copyright 2018-present Facebook. All Rights Reserved.
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
#include <string.h>
#include <syslog.h>
#include <unistd.h>
#include <fcntl.h>
#include <openbmc/ipmb.h>
#include <openbmc/ipmi.h>
#include "fbal_fruid.h"

#define PDB_BUS  8
#define PDB_ADDR 0x68
#define FRUID_READ_COUNT_MAX 0x20
#define FRUID_WRITE_COUNT_MAX 0x20

static int
pdb_ipmb_wrapper(uint8_t netfn, uint8_t cmd, uint8_t *txbuf, uint16_t txlen, uint8_t *rxbuf, uint8_t *rxlen) {
  ipmb_req_t *req;
  ipmb_res_t *res;
  uint8_t rbuf[MAX_IPMB_RES_LEN] = {0};
  uint8_t tbuf[MAX_IPMB_RES_LEN] = {0};
  uint16_t tlen = 0;
  uint8_t rlen = 0;

  req = (ipmb_req_t *)tbuf;
  req->res_slave_addr = PDB_ADDR;
  req->netfn_lun = netfn << LUN_OFFSET;
  req->hdr_cksum = req->res_slave_addr + req->netfn_lun;
  req->hdr_cksum = ZERO_CKSUM_CONST - req->hdr_cksum;

  req->req_slave_addr = BMC_SLAVE_ADDR << 1;
  req->seq_lun = 0x00;
  req->cmd = cmd;

  // Copy the data to be sent
  if (txlen) {
    memcpy(req->data, txbuf, txlen);
  }

  tlen = IPMB_HDR_SIZE + IPMI_REQ_HDR_SIZE + txlen;

  // Invoke IPMB library handler
  lib_ipmb_handle(PDB_BUS, tbuf, tlen, rbuf, &rlen);
  if (rlen < (IPMB_HDR_SIZE + IPMI_RESP_HDR_SIZE)) {
    syslog(LOG_ERR, "%s: insufficient bytes received", __func__);
    return -1;
  }

  // Handle IPMB response
  res = (ipmb_res_t *)rbuf;
  if (res->cc) {
    syslog(LOG_ERR, "%s: Completion Code: 0x%02X", __func__, res->cc);
    return -1;
  }

  // Copy the received data back to caller
  if ((rlen -= (IPMB_HDR_SIZE + IPMI_RESP_HDR_SIZE)) > *rxlen) {
    return -1;
  }
  *rxlen = rlen;
  memcpy(rxbuf, res->data, *rxlen);

  return 0;
}

static int
_get_pdb_fruid_info(uint8_t fru_id, ipmi_fruid_info_t *info) {
  int ret;
  uint8_t rlen = sizeof(ipmi_fruid_info_t);

  ret = pdb_ipmb_wrapper(NETFN_STORAGE_REQ, CMD_STORAGE_GET_FRUID_INFO, &fru_id, 1, (uint8_t *)info, &rlen);
  return ret;
}

static int
_read_pdb_fruid(uint8_t fru_id, uint16_t offset, uint8_t count, uint8_t *rbuf, uint8_t *rlen) {
  int ret;
  uint8_t tbuf[8];

  tbuf[0] = fru_id;
  tbuf[1] = offset & 0xFF;
  tbuf[2] = (offset >> 8) & 0xFF;
  tbuf[3] = count;

  ret = pdb_ipmb_wrapper(NETFN_STORAGE_REQ, CMD_STORAGE_READ_FRUID_DATA, tbuf, 4, rbuf, rlen);
  return ret;
}

static int
_write_pdb_fruid(uint8_t fru_id, uint16_t offset, uint8_t count, uint8_t *buf) {
  int ret;
  uint8_t tbuf[64];
  uint8_t rbuf[4];
  uint8_t tlen;
  uint8_t rlen = sizeof(rbuf);

  tbuf[0] = fru_id;
  tbuf[1] = offset & 0xFF;
  tbuf[2] = (offset >> 8) & 0xFF;

  memcpy(&tbuf[3], buf, count);
  tlen = count + 3;

  ret = pdb_ipmb_wrapper(NETFN_STORAGE_REQ, CMD_STORAGE_WRITE_FRUID_DATA, tbuf, tlen, rbuf, &rlen);
  if (ret) {
    return ret;
  }

  if (rbuf[0] != count) {
    return -1;
  }

  return ret;
}

int
fbal_read_pdb_fruid(uint8_t fru_id, const char *path, int fru_size) {
  int ret, fd;
  uint16_t nread, offset;
  uint8_t count;
  uint8_t rbuf[64] = {0};
  uint8_t rlen = 0;
  ipmi_fruid_info_t info;

  // Remove the file if exists already
  unlink(path);

  // Open the file exclusively for write
  fd = open(path, O_WRONLY | O_CREAT | O_EXCL, 0666);
  if (fd < 0) {
    syslog(LOG_ERR, "%s: open fails for path: %s", __func__, path);
    return -1;
  }

  do {
    // Read the FRUID information
    ret = _get_pdb_fruid_info(fru_id, &info);
    if (ret) {
      syslog(LOG_ERR, "%s: _get_pdb_fruid_info returns %d", __func__, ret);
      break;
    }

    // Indicates the size of the FRUID
    nread = (info.size_msb << 8) | info.size_lsb;
    if (nread > fru_size) {
      nread = fru_size;
    }

    // Read chunks of FRUID binary data in a loop
    offset = 0;
    while (nread > 0) {
      count = (nread > FRUID_READ_COUNT_MAX) ? FRUID_READ_COUNT_MAX : nread;
      rlen = sizeof(rbuf);
      ret = _read_pdb_fruid(fru_id, offset, count, rbuf, &rlen);
      if (ret || (rlen < 1)) {
        break;
      }

      // Ignore the first byte as it indicates length of response
      rlen -= 1;
      if (write(fd, &rbuf[1], rlen) != rlen) {
        ret = -1;
        break;
      }

      // Update offset
      offset += rlen;
      nread -= rlen;
    }
    if (nread > 0) {
      ret = -1;
    }
  } while (0);

  close(fd);
  return ret;
}

int
fbal_write_pdb_fruid(uint8_t fru_id, const char *path) {
  int fd, ret = -1;
  uint16_t offset;
  uint8_t count;
  uint8_t buf[64] = {0};

  fd = open(path, O_RDONLY, 0666);
  if (fd < 0) {
    syslog(LOG_ERR, "%s: open fails for path: %s", __func__, path);
    return -1;
  }

  // Write chunks of FRUID binary data in a loop
  offset = 0;
  while (1) {
    // Read from file
    count = read(fd, buf, FRUID_WRITE_COUNT_MAX);
    if (count <= 0) {
      break;
    }

    // Write to the FRUID
    ret = _write_pdb_fruid(fru_id, offset, count, buf);
    if (ret) {
      break;
    }

    // Update counter
    offset += count;
  }

  close(fd);
  return ret;
}
