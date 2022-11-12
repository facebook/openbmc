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
#include <sys/time.h>
#include <facebook/fbwc_common.h>
#include "expander.h"


#define FRUID_DATA_SIZE        512
#define MAX_FRUID_QUERY_SIZE   32

#define RETRY_TIME 3
#define IPMB_RETRY_DELAY_TIME 100*1000 // 100 millisecond

int
exp_ipmb_wrapper(uint8_t fru_id, uint8_t netfn, uint8_t cmd,
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
  uint8_t bus = 0;
  uint8_t address = 0;
  
  ret = get_exp_bus_addr(fru_id, &bus, &address);
  if (ret) {
    syslog(LOG_ERR,"%s %d fail to get exp mapping bus/addr, fru_id:%d ", __FUNCTION__, __LINE__, fru_id);
    return -1;
  }

  req->res_slave_addr = address << 1;
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
    ret = lib_ipmb_handle(bus, tbuf, tlen, rbuf, &rlen);

    if (ret < 0 || rlen == 0 || res->cc) {
      retry++;
      usleep(IPMB_RETRY_DELAY_TIME);
    } else {
      break;
    }
  }

  if (rlen == 0) {
    syslog(LOG_ERR, "%s(): Zero bytes received, retry:%d, netfn:0x%02X cmd:0x%02X ", __FUNCTION__,  retry, netfn, cmd);
    return -1;
  }

  if (res->cc) {
    syslog(LOG_ERR, "%s(): retry:%d, netfn:0x%02X, cmd:0x%02X, Completion Code: 0x%0xX ", __FUNCTION__,  retry, netfn, cmd, res->cc);
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
get_target_id_map(uint8_t target_id, uint8_t *fru_id, uint8_t *exp_fru_id) {
  target_id_info target_id_table[MAX_FRUS] = {
    // The order must be same as fbwc_common.h
    {0xFF, 0xFF},// FRU_ALL = 0,
    {0xFF, 0xFF},// FRU_MB,
    {0xFF, 0xFF},// FRU_IOM,
    {FRU_SCB, EXP_FRU_ID_SCB},// FRU_SCB,
    {FRU_SCB_PEER, EXP_FRU_ID_SCB},// FRU_SCB_PEER,
    {0xFF, 0xFF},// FRU_BMC,
    {0xFF, 0xFF},// FRU_SCM,
    {0xFF, 0xFF},// FRU_BSM,
    {FRU_SCB, EXP_FRU_ID_HDBP},// FRU_HDBP,
    {FRU_SCB_PEER, EXP_FRU_ID_HDBP},// FRU_HDBP_PEER,
    {FRU_SCB, EXP_FRU_ID_PDB},// FRU_PDB,
    {0xFF, 0xFF},// FRU_NIC,
    {FRU_SCB, EXP_FRU_ID_FAN0},// FRU_FAN0,
    {FRU_SCB, EXP_FRU_ID_FAN1},// FRU_FAN1,
    {FRU_SCB, EXP_FRU_ID_FAN2},// FRU_FAN2,
    {FRU_SCB, EXP_FRU_ID_FAN3},// FRU_FAN3,
    //JBOD1
    {FRU_JBOD1_L_SCB, EXP_FRU_ID_SCB},// FRU_JBOD1_L_SCB
    {FRU_JBOD1_R_SCB, EXP_FRU_ID_SCB},// FRU_JBOD1_R_SCB
    {FRU_JBOD1_L_SCB, EXP_FRU_ID_HDBP},// FRU_JBOD1_L_HDBP
    {FRU_JBOD1_R_SCB, EXP_FRU_ID_HDBP},// FRU_JBOD1_R_HDBP
    {FRU_JBOD1_L_SCB, EXP_FRU_ID_PDB},// FRU_JBOD1_PDB
    {FRU_JBOD1_L_SCB, EXP_FRU_ID_FAN0},// FRU_JBOD1_FAN0
    {FRU_JBOD1_L_SCB, EXP_FRU_ID_FAN1},// FRU_JBOD1_FAN1
    {FRU_JBOD1_L_SCB, EXP_FRU_ID_FAN2},// FRU_JBOD1_FAN2
    {FRU_JBOD1_L_SCB, EXP_FRU_ID_FAN3},// FRU_JBOD1_FAN3
    //JBOD2
    {FRU_JBOD2_L_SCB, EXP_FRU_ID_SCB},// FRU_JBOD2_L_SCB
    {FRU_JBOD2_R_SCB, EXP_FRU_ID_SCB},// FRU_JBOD2_R_SCB
    {FRU_JBOD2_L_SCB, EXP_FRU_ID_HDBP},// FRU_JBOD2_L_HDBP
    {FRU_JBOD2_R_SCB, EXP_FRU_ID_HDBP},// FRU_JBOD2_R_HDBP
    {FRU_JBOD2_L_SCB, EXP_FRU_ID_PDB},// FRU_JBOD2_PDB
    {FRU_JBOD2_L_SCB, EXP_FRU_ID_FAN0},// FRU_JBOD2_FAN0
    {FRU_JBOD2_L_SCB, EXP_FRU_ID_FAN1},// FRU_JBOD2_FAN1
    {FRU_JBOD2_L_SCB, EXP_FRU_ID_FAN2},// FRU_JBOD2_FAN2
    {FRU_JBOD2_L_SCB, EXP_FRU_ID_FAN3},// FRU_JBOD2_FAN3
  };
  
  if (target_id_table[target_id].board_fru_id == 0xFF ||
      target_id_table[target_id].exp_fru_id == 0xFF) {
    return -1;
  } else {
    *fru_id = target_id_table[target_id].board_fru_id;
    *exp_fru_id = target_id_table[target_id].exp_fru_id;
  }
  
  return 0;
}

int
exp_read_fruid(uint8_t target_id, char *path) {
  int ret = 0;
  uint32_t nread = 0;
  uint32_t offset = 0;
  uint8_t count = 0;
  uint8_t rbuf[MAX_IPMB_RES_LEN] = {0};
  uint8_t rlen = 0;
  int fd = 0;
  ssize_t bytes_wr = 0;
  ipmi_fruid_info_t info = {0};
  uint8_t fru_id = 0;
  uint8_t exp_fru_id = 0;

  // Remove the file if exists already
  unlink(path);

  // Open the file exclusively for write
  fd = open(path, O_WRONLY | O_CREAT | O_EXCL, 0666);
  if (fd < 0) {
    syslog(LOG_ERR, "%s open fails for path: %s\n", __func__, path);
    goto error_exit;
  }

  ret = get_target_id_map(target_id, &fru_id, &exp_fru_id);
  if (ret) {
    syslog(LOG_ERR, "%s, target_id:%d not supported", __func__, target_id);
    goto error_exit;
  }

  // Read the FRUID information
  ret = exp_get_fruid_info(fru_id, exp_fru_id, &info);
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

    ret = exp_read_fru_data(fru_id, exp_fru_id, offset, count, rbuf, &rlen);
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
exp_write_fruid(uint8_t target_id, char *path) {
  int ret = -1;
  uint32_t offset = 0;
  ssize_t count = 0;
  uint8_t buf[64] = {0};
  uint8_t fru_id = 0;
  uint8_t exp_fru_id = 0;
  int fd = 0;

  // Open the file exclusively for read
  fd = open(path, O_RDONLY, 0666);
  if (fd < 0) {
    syslog(LOG_ERR, "%s() Failed to open %s\n", __func__, path);
    goto error_exit;
  }
  
  ret = get_target_id_map(target_id, &fru_id, &exp_fru_id);
  if (ret) {
    syslog(LOG_ERR, "%s, target_id:%d not supported", __func__, target_id);
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
    ret = exp_write_fru_data(fru_id, exp_fru_id, offset, (uint8_t)count, buf);
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
exp_read_fru_data(uint8_t fru_id, uint8_t exp_fru_id, uint32_t offset, uint8_t count, uint8_t *rbuf, uint8_t *rlen) {
  uint8_t tbuf[4] = {0};
  uint8_t tlen;

  tbuf[0] = exp_fru_id;
  tbuf[1] = offset & 0xFF;
  tbuf[2] = (offset >> 8) & 0xFF;
  tbuf[3] = count;
  tlen = 4;

  return exp_ipmb_wrapper(fru_id, NETFN_STORAGE_REQ, CMD_STORAGE_READ_FRUID_DATA, tbuf, tlen, rbuf, rlen);
}

// Storage - Write FRU Data
// Netfn: 0x0A, Cmd: 0x12
int
exp_write_fru_data(uint8_t fru_id, uint8_t exp_fru_id, uint32_t offset, uint8_t count, uint8_t *buf) {
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

  ret = exp_ipmb_wrapper(fru_id, NETFN_STORAGE_REQ, CMD_STORAGE_WRITE_FRUID_DATA, tbuf, tlen, rbuf, &rlen);
  if (rbuf[0] != count) {
    syslog(LOG_WARNING, "%s() Failed to write fruid data. %d != %d \n", __func__, rbuf[0], count);
    return -1;
  }

  return ret;
}

// Storage - Get FRUID info
// Netfn: 0x0A, Cmd: 0x10
int
exp_get_fruid_info(uint8_t fru_id, uint8_t exp_fru_id, ipmi_fruid_info_t *info) {
  uint8_t rlen = 0;
  uint8_t fruid = 0;

  return exp_ipmb_wrapper(fru_id, NETFN_STORAGE_REQ, CMD_STORAGE_GET_FRUID_INFO, &fruid, 1, (uint8_t *) info, &rlen);
}

// APP - Get Self Test Results
// Netfn: 0x06, Cmd: 0x04
int
exp_get_self_test_result(uint8_t fru_id, uint8_t *self_test_result) {
  uint8_t rlen = 0;

  return exp_ipmb_wrapper(fru_id, NETFN_APP_REQ, CMD_APP_GET_SELFTEST_RESULTS, NULL, 0, (uint8_t *)self_test_result, &rlen);
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
exp_get_accur_sensor(uint8_t fru_id, uint8_t sensor_num, exp_extend_sensor_reading_t *sensor) {
  int ret = 0;
  uint8_t tbuf[2] = {0x01, sensor_num};
  uint8_t tlen = sizeof(tbuf);
  uint8_t rbuf[16] = {0};
  uint8_t rlen = 0;

  ret = exp_ipmb_wrapper(fru_id, NETFN_OEM_REQ, CMD_OEM_EXP_GET_SENSOR_READING, tbuf, tlen, rbuf, &rlen);
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
exp_sled_cycle(uint8_t fru_id) {
  int ret = 0;
  uint8_t rbuf[8] = {0};
  uint8_t rlen = 0;

  ret = exp_ipmb_wrapper(fru_id, NETFN_OEM_REQ, CMD_OEM_EXP_SLED_CYCLE, NULL, 0, rbuf, &rlen);
  if (ret != 0) {
    printf("cmd CMD_OEM_EXP_SLED_CYCLE fail \n");
    syslog(LOG_WARNING, "cmd CMD_OEM_EXP_SLED_CYCLE fail");
  }

  return ret;
}

int
exp_get_exp_ver(uint8_t fru_id, uint8_t *ver, uint8_t ver_len) {
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

  ret = exp_ipmb_wrapper(fru_id, NETFN_OEM_REQ, CMD_GET_EXP_VERSION, tbuf, tlen, rbuf, &rlen);
  if ((ret != 0) || (rlen != sizeof(exp_ver))) {
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
exp_update_firmware(uint8_t fru_id, uint8_t target, uint32_t offset, uint16_t len, uint8_t *buf) {
  uint8_t tbuf[256] = {0x00};
  uint8_t rbuf[16] = {0x00};
  uint8_t tlen = 0;
  uint8_t rlen = 0;
  int ret;
  int retries = 3;
  int index = 3;
  int offset_size = sizeof(offset) / sizeof(tbuf[0]);

  // File the IANA ID
  memcpy(tbuf, (uint8_t *)&IANA_ID, 3);

  // Fill the component for which firmware is requested
  tbuf[index++] = target;

  memcpy(&tbuf[index], (uint8_t *)&offset, offset_size);
  index += offset_size;

  tbuf[index++] = len & 0xFF;
  tbuf[index++] = (len >> 8) & 0xFF;

  memcpy(&tbuf[index], buf, len);

  tlen = len + index;

exp_send:
  ret = exp_ipmb_wrapper(fru_id, NETFN_OEM_1S_REQ, CMD_OEM_1S_UPDATE_FW, tbuf, tlen, rbuf, &rlen);
  if ((ret) && (retries--)) {
    sleep(1);
    printf("_update_fw: target %d, offset: %u, len: %d retrying..\
           \n",   target, offset, len);
    goto exp_send;
  }

  return ret;
}

static int
update_exp_runtime_fw(uint8_t comp, char *path, uint8_t force) {
  int ret = -1;
  int fd = -1;
  int file_size;
  struct timeval start, end;
  struct stat finfo;
  uint32_t dsize, last_offset;
  uint32_t offset, boundary;
  volatile uint16_t read_count;
  uint8_t buf[256] = {0};
  uint8_t target;
  ssize_t count;

  fd = open(path, O_RDONLY, 0666);
  if ( fd < 0 ) {
    syslog(LOG_WARNING, "%s() cannot open the file: %s, fd=%d\n", __func__, path, fd);
    return fd;
  }
  
  fstat(fd, &finfo);
  file_size = finfo.st_size;

  printf("updating fw on SCB, file size = %d bytes \n", file_size);

  // Write chunks of binary data in a loop
  dsize = file_size/100;
  last_offset = 0;
  offset = 0;
  boundary = PKT_SIZE;
  target = comp;
  gettimeofday(&start, NULL);
  while (1) {
    // send packets in blocks of 64K
    if ((offset + EXP_IPMB_WRITE_COUNT_MAX) < boundary) {
      read_count = EXP_IPMB_WRITE_COUNT_MAX;
    } else {
      read_count = boundary - offset;
    }

    // Read from file
    count = read(fd, buf, read_count);
    if ((count < 0) && (errno == EINTR)) {
      continue;
    }
    if (count <= 0 || count > read_count) {
      break;
    }

    if ((offset + count) >= file_size) {
      target |= 0x80;
    }

    ret = exp_update_firmware(FRU_SCB, target, offset, count, buf);
    if (ret) {
      goto error_exit;
    }

    // Update counter
    offset += count;
    if (offset >= boundary) {
      boundary += PKT_SIZE;
    }
    if ((last_offset + dsize) <= offset) {
      set_fw_update_ongoing(FRU_SCB, 60);
      printf("\rupdated exp: %u %%", offset/dsize);
      fflush(stdout);
      last_offset += dsize;
    }
  }
  printf("\n");

  gettimeofday(&end, NULL);
  printf("Elapsed time:  %d   sec.\n", (int)(end.tv_sec - start.tv_sec));

error_exit:
  printf("\n");
  if ( fd >= 0 ) {
    close(fd);
  }

  return ret;
}

int
exp_update_fw(uint8_t fru_id, uint8_t comp, char *path, int fd, uint8_t force) {
  int ret = -1;

  if (path == NULL) {
    syslog(LOG_ERR, "%s(): Update aborted due to NULL parameter: *path", __func__);
    return -1;
  }

  switch (comp) {
    case FW_EXP:
      ret = update_exp_runtime_fw(UPDATE_EXP, path, force);
      break;

    default:
      syslog(LOG_WARNING, "Unknown compoet %x", comp);
      break;
  }
  syslog(LOG_CRIT, "Updated %s on SCB. File: %s. Result: %s", get_component_name(comp), path, (ret != 0)? "Fail": "Success");

  return ret;
}

int exp_set_fan_speed(uint8_t fru_id, uint8_t fan, uint8_t pwm) {
  int ret = 0;
  uint8_t tbuf[5] = {0x00, 0xFF, 0xFF, 0xFF, 0xFF};
  uint8_t tlen = sizeof(tbuf);
  uint8_t rbuf[24] = {0};
  uint8_t rlen = 0;
  uint8_t mode = 0;
  
  if (fan >= PWM_NUM) {
    syslog(LOG_WARNING, "%s() invalid fan#:%d", __func__, fan);
    return -1;
  }
  
  ret = exp_ipmb_wrapper(fru_id, NETFN_OEM_REQ, CMD_OEM_EXP_GET_FAN_TACH, NULL, 0, rbuf, &rlen);
  if (ret) {
    printf("command NETFN_OEM_REQ(0x30) CMD_OEM_EXP_GET_FAN_TACH(0xD0) fail \n");
  }
  mode = rbuf[0];
  
  tbuf[0] = mode;
  tbuf[fan + 1] = pwm;
  ret = exp_ipmb_wrapper(fru_id, NETFN_OEM_REQ, CMD_OEM_EXP_SET_FAN_PWM, tbuf, tlen, rbuf, &rlen);
  if (ret) {
    printf("command NETFN_OEM_REQ(0x30) CMD_OEM_EXP_SET_FAN_PWM(0xD1) fail \n");
  }

  return ret;
}

int exp_get_fan_pwm(uint8_t fru_id, uint8_t pwm_index, uint8_t *pwm) {
  int ret = 0;
  uint8_t rbuf[24] = {0};
  uint8_t rlen = 0;

  if (pwm_index >= PWM_NUM) {
    // the index should be 0, 1, 2, 3
    syslog(LOG_WARNING, "%s() invalid PWM#%d", __func__, pwm_index);
    return -1;
  }
  
  ret = exp_ipmb_wrapper(fru_id, NETFN_OEM_REQ, CMD_OEM_EXP_GET_FAN_TACH, NULL, 0, rbuf, &rlen);
  *pwm = rbuf[pwm_index + 1];

  return ret;
}

int exp_set_fan_mode(uint8_t fru_id, uint8_t mode) {
  int ret = 0;
  uint8_t tbuf[5] = {0x00, 0xFF, 0xFF, 0xFF, 0xFF};
  uint8_t tlen = sizeof(tbuf);
  uint8_t rbuf[4] = {0};
  uint8_t rlen = 0;

  tbuf[0] = mode;
  ret = exp_ipmb_wrapper(fru_id, NETFN_OEM_REQ, CMD_OEM_EXP_SET_FAN_PWM, tbuf, tlen, rbuf, &rlen);
  if (ret) {
    printf("command NETFN_OEM_REQ(0x30) CMD_OEM_EXP_SET_FAN_PWM(0xD1) fail \n");
  }

  return ret;
}

int exp_get_fan_mode(uint8_t fru_id, uint8_t *mode) {
  int ret = 0;
  uint8_t rbuf[24] = {0};
  uint8_t rlen = 0;

  ret = exp_ipmb_wrapper(fru_id, NETFN_OEM_REQ, CMD_OEM_EXP_GET_FAN_TACH, NULL, 0, rbuf, &rlen);
  if (ret) {
    printf("command NETFN_OEM_REQ(0x30) CMD_OEM_EXP_GET_FAN_TACH(0xD0) fail \n");
  }
  *mode = rbuf[0];

  return ret;
}
