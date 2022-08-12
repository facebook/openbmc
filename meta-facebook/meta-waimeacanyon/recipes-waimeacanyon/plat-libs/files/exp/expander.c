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
#include "expander.h"

#define FRUID_DATA_SIZE        512
#define MAX_FRUID_QUERY_SIZE   32

#define RETRY_TIME 3
#define IPMB_RETRY_DELAY_TIME 100*1000 // 100 millisecond

int
exp_ipmb_wrapper(uint8_t netfn, uint8_t cmd,
                 uint8_t *txbuf, uint16_t txlen, uint8_t *rxbuf, uint8_t *rxlen) {
  uint8_t rbuf[MAX_IPMB_RES_LEN] = {0};
  uint8_t tbuf[MAX_IPMB_REQ_LEN] = {0};
  ipmb_req_t *req = (ipmb_req_t*)tbuf;
  ipmb_res_t *res = (ipmb_res_t*)rbuf;
  uint16_t tlen = 0;
  uint8_t rlen = 0;
  int i = 0;
  int ret = 0;
  uint8_t dataCksum = 0;
  int retry = 0;

  req->res_slave_addr = EXP_SLAVE_ADDR << 1;
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
    ret = lib_ipmb_handle(I2C_BUS_EXP, tbuf, tlen, rbuf, &rlen);

    if (ret < 0 || rlen == 0 || res->cc) {
      retry++;
      usleep(IPMB_RETRY_DELAY_TIME);
    } else {
      break;
    }
  }

  if (rlen == 0) {
    syslog(LOG_ERR, "exp_ipmb_wrapper: Zero bytes received, retry:%d, netfn:0x%02X cmd:0x%02X ", retry, netfn, cmd);
    return -1;
  }

  if (res->cc) {
    syslog(LOG_ERR, "exp_ipmb_wrapper: retry:%d, netfn:0x%02X, cmd:0x%02X, Completion Code: 0x%0xX ", retry, netfn, cmd, res->cc);
    return -1;
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
    return -1;
  }

  return 0;
}

int
exp_read_fruid(uint8_t exp_fru_id, char *path) {
  int ret = 0;
  uint32_t nread = 0;
  uint32_t offset = 0;
  uint8_t count = 0;
  uint8_t rbuf[MAX_IPMB_RES_LEN] = {0};
  uint8_t rlen = 0;
  int fd = 0;
  ssize_t bytes_wr = 0;
  ipmi_fruid_info_t info = {0};

  // Remove the file if exists already
  unlink(path);

  // Open the file exclusively for write
  fd = open(path, O_WRONLY | O_CREAT | O_EXCL, 0666);
  if (fd < 0) {
    syslog(LOG_ERR, "%s open fails for path: %s\n", __func__, path);
    goto error_exit;
  }

  // Read the FRUID information
  ret = exp_get_fruid_info(FRU_SCB, exp_fru_id, &info);
  if (ret) {
    syslog(LOG_ERR, "%s exp_read_fruid_info returns %d \n", __func__, ret);
    goto error_exit;
  }

  // Indicates the size of the FRUID
  nread = (info.size_msb << 8) | info.size_lsb;
  if (nread > MAX_FRU_DATA_SIZE) {
    nread = MAX_FRU_DATA_SIZE;
  } else if (nread == 0) {
    goto error_exit;
  }

  // Read chunks of FRUID binary data in a loop
  offset = 0;
  while (nread > 0) {
    if (nread > EXP_MAX_FRU_DATA_WRITE_COUNT) {
      count = EXP_MAX_FRU_DATA_WRITE_COUNT;
    } else {
      count = nread;
    }

    ret = exp_read_fru_data(FRU_MB, exp_fru_id, offset, count, rbuf, &rlen);
    if (ret) {
      syslog(LOG_ERR, "%s _read_fruid failed, ret: %d\n", __func__, ret);
      goto error_exit;
    }

    // Ignore the first byte as it indicates length of response
    bytes_wr = write(fd, &rbuf[1], rlen-1);
    if (bytes_wr != rlen-1) {
      syslog(LOG_ERR, "%s write fruid failed, write bytes: %d\n", __func__, bytes_wr);
      return -1;
    }

    // Update offset
    offset += (rlen-1);
    nread -= (rlen-1);
  }

error_exit:
  if (fd >= 0) {
    close(fd);
  }

  return ret;
}

int
exp_write_fruid(uint8_t exp_fru_id, char *path) {
  int ret = -1;
  uint32_t offset = 0;
  ssize_t count = 0;
  uint8_t buf[64] = {0};
  int fd = 0;

  // Open the file exclusively for read
  fd = open(path, O_RDONLY, 0666);
  if (fd < 0) {
    syslog(LOG_ERR, "%s() Failed to open %s\n", __func__, path);
    goto error_exit;
  }

  // Write chunks of FRUID binary data in a loop
  offset = 0;
  while (1) {
    // Read from file
    count = read(fd, buf, EXP_MAX_FRU_DATA_WRITE_COUNT);
    if (count <= 0) {
      break;
    }

    // Write to the FRUID
    ret = exp_write_fru_data(FRU_SCB, exp_fru_id, offset, (uint8_t)count, buf);
    if (ret) {
      break;
    }

    // Update counter
    offset += count;
  }

error_exit:
  if (fd >= 0 ) {
    close(fd);
  }

  return ret;
}

// Storage - Read FRU Data
// Netfn: 0x0A, Cmd: 0x11
int
exp_read_fru_data(uint8_t slot_id, uint8_t exp_fru_id, uint32_t offset, uint8_t count, uint8_t *rbuf, uint8_t *rlen) {
  uint8_t tbuf[4] = {0};
  uint8_t tlen;

  tbuf[0] = exp_fru_id;
  tbuf[1] = offset & 0xFF;
  tbuf[2] = (offset >> 8) & 0xFF;
  tbuf[3] = count;
  tlen = 4;

  return exp_ipmb_wrapper(NETFN_STORAGE_REQ, CMD_STORAGE_READ_FRUID_DATA, tbuf, tlen, rbuf, rlen);
}

// Storage - Write FRU Data
// Netfn: 0x0A, Cmd: 0x12
int
exp_write_fru_data(uint8_t slot_id, uint8_t exp_fru_id, uint32_t offset, uint8_t count, uint8_t *buf) {
  int ret = 0;
  uint8_t tbuf[MAX_FRU_DATA_SIZE + 3] = {0};
  uint8_t rbuf[8] = {0};
  uint8_t tlen = 0;
  uint8_t rlen = 0;

  tbuf[0] = exp_fru_id;
  tbuf[1] = offset & 0xFF;
  tbuf[2] = (offset >> 8) & 0xFF;
  memcpy(&tbuf[3], buf, count);
  tlen = count + 3;

  ret = exp_ipmb_wrapper(NETFN_STORAGE_REQ, CMD_STORAGE_WRITE_FRUID_DATA, tbuf, tlen, rbuf, &rlen);
  if (rbuf[0] != count) {
    syslog(LOG_WARNING, "%s() Failed to write fruid data. %d != %d \n", __func__, rbuf[0], count);
    return -1;
  }

  return ret;
}

// Storage - Get FRUID info
// Netfn: 0x0A, Cmd: 0x10
int
exp_get_fruid_info(uint8_t slot_id, uint8_t exp_fru_id, ipmi_fruid_info_t *info) {
  uint8_t rlen = 0;
  uint8_t fruid = 0;

  return exp_ipmb_wrapper(NETFN_STORAGE_REQ, CMD_STORAGE_GET_FRUID_INFO, &fruid, 1, (uint8_t *) info, &rlen);
}

// APP - Get Self Test Results
// Netfn: 0x06, Cmd: 0x04
int
exp_get_self_test_result(uint8_t slot_id, uint8_t *self_test_result) {
  uint8_t rlen = 0;

  return exp_ipmb_wrapper(NETFN_APP_REQ, CMD_APP_GET_SELFTEST_RESULTS, NULL, 0, (uint8_t *)self_test_result, &rlen);
}

int
exp_get_sensor_reading(uint8_t sensor_num, float *value) {
#define BIC_SENSOR_READ_NA 0x20
  int ret = 0;
  exp_extend_sensor_reading_t sensor = {0};

  ret = exp_get_accur_sensor(FRU_SCB, sensor_num, &sensor);
  if (ret < 0) {
    syslog(LOG_WARNING, "%s() Failed, snr#0x%x", __func__, sensor_num);
    return READING_NA;
  } else {
    if (sensor.flags & BIC_SENSOR_READ_NA) {
      return READING_NA;
    }

    *value = ((int16_t)((sensor.value & 0xFFFF0000) >> 16)) * 0.001 + ((int16_t)(sensor.value & 0x0000FFFF)) ;
  }

  return ret;
}

// Netfn: 0x30, Cmd: 0x2D
int
exp_get_accur_sensor(uint8_t slot_id, uint8_t sensor_num, exp_extend_sensor_reading_t *sensor) {
  int ret = 0;
  uint8_t tbuf[2] = {0x01, sensor_num};
  uint8_t tlen = sizeof(tbuf);
  uint8_t rbuf[16] = {0};
  uint8_t rlen = 0;

  ret = exp_ipmb_wrapper(NETFN_OEM_REQ, CMD_OEM_EXP_GET_SENSOR_READING, tbuf, tlen, rbuf, &rlen);
  if (ret != 0) {
    return ret;
  }

  if (rlen == 7) {
    // Byte 1 : sensor_num
    // Byte 2 - 5 : sensor raw data
    // Byte 6 : flags
    // Byte 7 : reserve 0xFF
    if (rbuf[0] != sensor_num) {
      syslog(LOG_WARNING, "%s() sensor_num is mismatch, sensor_num=0x%02X, rbuf[0]=0x%02X", __func__, sensor_num, rbuf[0]);
      return READING_NA;
    }
    memcpy((uint8_t *)&sensor->value, rbuf+1, 4);
    sensor->flags = rbuf[5];
  } else {
    syslog(LOG_WARNING, "%s() invalid rlen=%d", __func__, rlen);
    return READING_NA;
  }

  return ret;
}

// Netfn: 0x30, Cmd: 0x30
int
exp_sled_cycle(uint8_t slot_id) {
  int ret = 0;
  uint8_t rbuf[8] = {0};
  uint8_t rlen = 0;

  ret = exp_ipmb_wrapper(NETFN_OEM_REQ, CMD_OEM_EXP_SLED_CYCLE, NULL, 0, rbuf, &rlen);
  if (ret != 0) {
    syslog(LOG_WARNING, "cmd CMD_OEM_EXP_SLED_CYCLE fail");
  }

  return ret;
}
