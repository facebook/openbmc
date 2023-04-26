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
#include <sys/stat.h>
#include <sys/types.h>
#include <openbmc/kv.h>
#include <openbmc/obmc-i2c.h>
#include <libpldm/base.h>
#include <libpldm-oem/pldm.h>
#include "bic_xfer.h"

const uint32_t IANA_ID = 0x00A015;
const uint32_t VF_IANA_ID = 0x009C9C;

void msleep(int msec) {
  struct timespec req;

  req.tv_sec = 0;
  req.tv_nsec = msec * 1000 * 1000;

  while(nanosleep(&req, &req) == -1 && errno == EINTR) {
    continue;
  }
}

int i2c_open(uint8_t bus_id, uint8_t addr_7bit) {
  int fd = -1;

  fd = i2c_cdev_slave_open(bus_id, addr_7bit, I2C_SLAVE_FORCE_CLAIM);
  if ( fd < 0 ) {
    syslog(LOG_ERR, "Failed to open /dev/i2c-%d\n", bus_id);
    return BIC_STATUS_FAILURE;
  }
  return fd;
}

int is_bic_ready(uint8_t slot_id, uint8_t intf __attribute__((unused))) {
  int ret;
  uint8_t bic_ready = 0;

  // check the ready pin of server bic by reading GPIO
  // TODO: check expansion BIC status
  ret = fby35_common_is_bic_ready(slot_id, &bic_ready);
  if (!ret && bic_ready != 1) {
    // still treat BIC is ready when failing to access CPLD
    return BIC_STATUS_FAILURE;
  }

  return BIC_STATUS_SUCCESS;
}

// Repack data according to the interface
int
bic_data_send(uint8_t slot_id, uint8_t netfn, uint8_t cmd, uint8_t *tbuf, uint8_t tlen, uint8_t *rbuf, uint8_t *rlen, uint8_t intf) {
  int ret = 0;
  uint8_t tmp_buf[MAX_IPMB_RES_LEN] = {0x0};
  uint8_t rsp_buf[MAX_IPMB_RES_LEN] = {0x0};
  uint8_t tmp_len = 0;
  uint8_t rsp_len = 0;

  switch(intf) {
    case NONE_INTF:
      ret = bic_data_wrapper(slot_id, netfn, cmd, tbuf, tlen, rbuf, rlen);
      break;
    case FEXP_BIC_INTF:
    case BB_BIC_INTF:
    case REXP_BIC_INTF:
    case EXP3_BIC_INTF:
    case EXP4_BIC_INTF:
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

      ret = bic_data_wrapper(slot_id, NETFN_OEM_1S_REQ, CMD_OEM_1S_MSG_OUT, tmp_buf, tmp_len, rsp_buf, &rsp_len);
      //rsp_buf[6] is the completion code
      if ( (ret < 0) || (ret == BIC_STATUS_SUCCESS && rsp_buf[6] != CC_SUCCESS) ) {
        syslog(LOG_WARNING, "%s() The 2nd BIC cannot be reached. CC: 0x%02X, intf: 0x%x, ret = %d\n", __func__, rsp_buf[6], intf, ret);
        syslog(LOG_WARNING, "%s() Netfn:%02X, Cmd: %02X\n", __func__, netfn << 2, cmd);
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

int bic_data_wrapper(uint8_t slot_id, uint8_t netfn, uint8_t cmd,
                     uint8_t *txbuf, uint16_t txlen, uint8_t *rxbuf, uint8_t *rxlen) {
  //char sys_conf[16] = {0};
  int ret = 0;

#ifdef CONFIG_IPMB_XFER
  ret = bic_ipmb_wrapper(slot_id, netfn, cmd, txbuf, txlen, rxbuf, rxlen);
#else
  ret = bic_pldm_wrapper(slot_id, netfn, cmd, txbuf, txlen, rxbuf, rxlen);
#endif

  return ret;
}

int bic_pldm_wrapper(uint8_t slot_id, uint8_t netfn, uint8_t cmd,
                     uint8_t *txbuf, uint16_t txlen, uint8_t *rxbuf, uint8_t *rxlen) {
  int ret = 0;
  uint8_t bus_id = 0;
  int retry = 0;
  uint8_t status_12v = 0, smbus_isolated = ISOLATED_DISABLE;
  size_t rlen = 0;

  ret = fby35_common_get_bus_id(slot_id);
  if (ret < 0) {
    syslog(LOG_ERR, "%s: Wrong Slot ID %d\n", __func__, slot_id);
    return ret;
  }

  bus_id = (uint8_t) ret;

  while (retry < BIC_XFER_RETRY_TIME) {
    // avoid meaningless retry
    ret = fby35_common_server_smbus_isolated(slot_id, &smbus_isolated);
    if ((ret == 0) && (smbus_isolated == ISOLATED_ENABLE)) {
      return BIC_STATUS_NOT_READY;
    }

    ret = fby35_common_server_stby_pwr_sts(slot_id, &status_12v);
    if (ret < 0 || status_12v == 0) {
      return BIC_STATUS_NOT_READY;
    }
    if (is_bic_ready(slot_id, NONE_INTF) != BIC_STATUS_SUCCESS) {
      return BIC_STATUS_NOT_READY;
    }

    ret = oem_pldm_ipmi_send_recv(bus_id, 0xA, netfn, cmd, txbuf, txlen, rxbuf, &rlen, false);
    if (ret) {
      msleep(BIC_XFER_RETRY_DELAY);
      retry++;
      continue;
    }
    break;
  }
  if (retry >= BIC_XFER_RETRY_TIME) {
    syslog(LOG_ERR, "%s: slot%u netfn: 0x%02X cmd: 0x%02X, retry exceeded", __func__, slot_id, netfn, cmd);
    return BIC_STATUS_FAILURE;
  }

  if (ret) {
    syslog(LOG_ERR, "%s: slot%u netfn: 0x%02X cmd: 0x%02X, ret: %d", __func__, slot_id, netfn, cmd, ret);
    return BIC_STATUS_FAILURE;
  }
  *rxlen = rlen;

  return BIC_STATUS_SUCCESS;
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
  uint8_t status_12v = 0, smbus_isolated = ISOLATED_DISABLE;

  ret = fby35_common_get_bus_id(slot_id);
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

  while (retry < BIC_XFER_RETRY_TIME) {
    // avoid meaningless retry
    ret = fby35_common_server_smbus_isolated(slot_id, &smbus_isolated);
    if ((ret == 0) && (smbus_isolated == ISOLATED_ENABLE)) {
      return BIC_STATUS_FAILURE;
    }
    ret = fby35_common_server_stby_pwr_sts(slot_id, &status_12v);
    if ( ret < 0 || status_12v == 0) {
      return BIC_STATUS_FAILURE;
    }
#ifndef CONFIG_HALFDOME
    if (is_bic_ready(slot_id, NONE_INTF) != BIC_STATUS_SUCCESS) {
      msleep(BIC_XFER_RETRY_DELAY);
      retry++;
      continue;
    }
#endif
    // Invoke IPMB library handler
    lib_ipmb_handle(bus_id, tbuf, tlen, rbuf, &rlen);

    if (rlen == 0) {
      if (is_bic_ready(slot_id, NONE_INTF) != BIC_STATUS_SUCCESS) {
        break;
      }

      if (++retry < BIC_XFER_RETRY_TIME) {
        msleep(BIC_XFER_RETRY_DELAY);
      }
    } else {
      res = (ipmb_res_t*) rbuf;
      if ((res->cc == CC_NODE_BUSY) && (++retry < BIC_XFER_RETRY_TIME)) {
        msleep(BIC_XFER_RETRY_DELAY);
        continue;
      }
      break;
    }
  }

  if (rlen == 0) {
    syslog(LOG_ERR, "bic_ipmb_wrapper: slot%d netfn: 0x%02X cmd: 0x%02X, Zero bytes received, retry:%d ", slot_id, netfn, cmd, retry);
    return BIC_STATUS_FAILURE;
  }

  // Handle IPMB response
  if (res->cc) {
    syslog(LOG_ERR, "bic_ipmb_wrapper: slot%d netfn: 0x%02X cmd: 0x%02X, Completion Code: 0x%02X ", slot_id, netfn, cmd, res->cc);
    if (res->cc == CC_INVALID_CMD) {
      return BIC_STATUS_NOT_SUPP_IN_CURR_STATE;
    }
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
    syslog(LOG_ERR, "bic_data_wrapper: slot%d netfn: 0x%02X cmd: 0x%02X, Receive Data cksum does not match (expectative 0x%x, actual 0x%x)", slot_id, netfn, cmd, dataCksum, rbuf[rlen-1]);
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
  memcpy(tbuf, (uint8_t *)&IANA_ID, IANA_ID_SIZE);

  // Fill the interface number as ME
  tbuf[3] = BIC_INTF_ME;

  // Fill the data to be sent
  memcpy(&tbuf[4], txbuf, txlen);

  // Send data length includes IANA ID and interface number
  tlen = txlen + 4;

  ret = bic_data_send(slot_id, NETFN_OEM_1S_REQ, CMD_OEM_1S_MSG_OUT, tbuf, tlen, rbuf, &rlen, NONE_INTF);
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

int
_set_fw_update_ongoing(uint8_t slot_id, uint16_t tmout) {
  char key[64];
  char value[64] = {0};
  struct timespec ts;

  sprintf(key, "fru%u_fwupd", slot_id);

  clock_gettime(CLOCK_MONOTONIC, &ts);
  ts.tv_sec += tmout;
  sprintf(value, "%ld", ts.tv_sec);

  if (kv_set(key, value, 0, 0) < 0) {
     return -1;
  }

  return 0;
}

int
send_image_data_via_bic(uint8_t slot_id, uint8_t comp, uint8_t intf, uint32_t offset, uint16_t len, uint32_t image_len, uint8_t *buf)
{
  uint8_t tbuf[256] = {0};
  uint8_t rbuf[16] = {0};
  uint8_t tlen = 0;
  uint8_t rlen = 0;
  int ret = -1;
  int retries = 3;

  memcpy(tbuf, (uint8_t *)&IANA_ID, IANA_ID_SIZE);
  tbuf[3] = comp;
  tbuf[4] = (offset) & 0xFF;
  tbuf[5] = (offset >>  8) & 0xFF;
  tbuf[6] = (offset >> 16) & 0xFF;
  tbuf[7] = (offset >> 24) & 0xFF;
  tbuf[8] = len & 0xFF;
  tbuf[9] = (len >> 8) & 0xFF;
  if ( image_len > 0 ) {
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
    ret = bic_data_send(slot_id, NETFN_OEM_1S_REQ, CMD_OEM_1S_UPDATE_FW, tbuf, tlen, rbuf, &rlen, intf);
    if (ret != BIC_STATUS_SUCCESS) {
      if (ret == BIC_STATUS_NOT_SUPP_IN_CURR_STATE)
        return ret;
      printf("%s() slot: %d, target: %d, offset: %u, len: %d retrying..\n", __func__, slot_id, comp, offset, len);
    }
  } while( (ret < 0) && (retries--));

  return ret;
}

int
open_and_get_size(char *path, size_t *file_size) {
  struct stat finfo;
  int fd;

  fd = open(path, O_RDONLY, 0666);
  if ( fd < 0 ) {
    return fd;
  }

  fstat(fd, &finfo);
  *file_size = finfo.st_size;

  return fd;
}
