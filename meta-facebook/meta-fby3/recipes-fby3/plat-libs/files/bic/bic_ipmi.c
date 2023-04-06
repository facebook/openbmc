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
#include <fcntl.h>
#include <syslog.h>
#include <errno.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/file.h>
#include <openbmc/obmc-i2c.h>
#include <openbmc/kv.h>
#include "bic_ipmi.h"

//FRU
#define FRUID_READ_COUNT_MAX 0x20
#define FRUID_WRITE_COUNT_MAX 0x20
#define FRUID_SIZE 512

//GPIO
#define GPIO_LOW 0
#define GPIO_HIGH 1
#define GPIO_HSC_MUX_SWITCH 0x2F

//SDR
#define SDR_READ_COUNT_MAX 0x1A
#pragma pack(push, 1)
typedef struct _sdr_rec_hdr_t {
  uint16_t rec_id;
  uint8_t ver;
  uint8_t type;
  uint8_t len;
} sdr_rec_hdr_t;
#pragma pack(pop)

#define SIZE_SYS_GUID 16

#define MAX_VER_STR_LEN 80

#define KV_SLOT_IS_M2_EXP_PRESENT "slot%x_is_m2_exp_prsnt"
#define KV_SLOT_GET_1OU_TYPE      "slot%x_get_1ou_type"
#define KV_SLOT_GET_1OU_CARD_TYPE      "slot%x_1ou_card_type"
#define KV_MB_INDEX               "get_mb_index"

#define KV_SLOT_IS_2U_TOPBOT_PRESENT "slot%x_is_2u_topbot_prsnt"

#define MIN(x,y) (((x) < (y)) ? (x) : (y))

#define BIC_SENSOR_SYSTEM_STATUS  0x46

#define BB_FW_UPDATE_STAT_FILE "/tmp/cache_store/bb_fw_update"

#define GPIO_FALSE_DIR "/tmp/gpio/bic%d_%d"

enum {
  M2_PWR_OFF = 0x00,
  M2_PWR_ON  = 0x01,
  M2_REG_2OU = 0x04,
  M2_REG_1OU = 0x05,
};

enum {
  M2_ROOT_PORT0 = 0x0,
  M2_ROOT_PORT1 = 0x1,
  M2_ROOT_PORT2 = 0x2,
  M2_ROOT_PORT3 = 0x3,
  M2_ROOT_PORT4 = 0x4,
  M2_ROOT_PORT5 = 0x5,
};


uint8_t mapping_m2_prsnt[2][6] = { {M2_ROOT_PORT0, M2_ROOT_PORT1, M2_ROOT_PORT5, M2_ROOT_PORT4, M2_ROOT_PORT2, M2_ROOT_PORT3},
                                   {M2_ROOT_PORT4, M2_ROOT_PORT3, M2_ROOT_PORT2, M2_ROOT_PORT1}};
uint8_t mapping_e1s_prsnt[2][6] = { {M2_ROOT_PORT4, M2_ROOT_PORT5, M2_ROOT_PORT3, M2_ROOT_PORT2, M2_ROOT_PORT1, M2_ROOT_PORT0},
                                    {M2_ROOT_PORT1, M2_ROOT_PORT2, M2_ROOT_PORT3, M2_ROOT_PORT4}};
uint8_t mapping_e1s_pwr[2][6] = { {M2_ROOT_PORT3, M2_ROOT_PORT2, M2_ROOT_PORT5, M2_ROOT_PORT4, M2_ROOT_PORT1, M2_ROOT_PORT0},
                                  {M2_ROOT_PORT4, M2_ROOT_PORT3, M2_ROOT_PORT2, M2_ROOT_PORT1} };

// S/E - Get Sensor reading
// Netfn: 0x04, Cmd: 0x2d
int
bic_get_sensor_reading(uint8_t slot_id, uint8_t sensor_num, ipmi_sensor_reading_t *sensor, uint8_t intf) {
  uint8_t rlen = 0;
  return bic_ipmb_send(slot_id, NETFN_SENSOR_REQ, CMD_SENSOR_GET_SENSOR_READING, &sensor_num, 1, (uint8_t *)sensor, &rlen, intf);
}

int
bic_get_oem_sensor_reading(uint8_t slot_id, uint8_t index, ipmi_sensor_reading_t *sensor, uint8_t mul, uint8_t intf) {
#define BIC_SENSOR_READ_NA 0x20
  uint8_t tbuf[4] = {0x9c, 0x9c, 0x00, index};
  uint8_t rbuf[16] = {0x00};
  uint8_t rlen = 0;
  uint32_t val = 0;
  int ret = bic_ipmb_send(slot_id, NETFN_OEM_1S_REQ, BIC_CMD_GPV3_GET_DUAL_M2_PWR, tbuf, 4, rbuf, &rlen, intf);

  // rbuf[0 - 2] IANA Data
  // rbuf[3 - 4] Dual Sensor Value
  // rbuf[5] Sensor Flag
  if (ret == 0 && rlen == 6) {
    if (mul > 0) {
      val = (rbuf[3] + rbuf[4]) / mul;
    } else {
      val = (rbuf[3] + rbuf[4]);
    }
    sensor->flags = rbuf[5];
    sensor->status = 0;
    sensor->ext_status = 0;

    if (val > 0xff) {
      sensor->value = 0xff;
    } else {
      sensor->value = (uint8_t)val;
    }
  } else {
    sensor->flags = BIC_SENSOR_READ_NA;
  }

  return ret;
}

// APP - Get Device ID
// Netfn: 0x06, Cmd: 0x01
int
bic_get_dev_id(uint8_t slot_id, ipmi_dev_id_t *dev_id, uint8_t intf) {
  uint8_t rlen = 0;
  return bic_ipmb_send(slot_id, NETFN_APP_REQ, CMD_APP_GET_DEVICE_ID, NULL, 0, (uint8_t *) dev_id, &rlen, intf);
}

// APP - Get Device ID
// Netfn: 0x06, Cmd: 0x04
int
bic_get_self_test_result(uint8_t slot_id, uint8_t *self_test_result, uint8_t intf) {
  uint8_t rlen = 0;
  return bic_ipmb_send(slot_id, NETFN_APP_REQ, CMD_APP_GET_SELFTEST_RESULTS, NULL, 0, (uint8_t *)self_test_result, &rlen, intf);
}

// Storage - Get FRUID info
// Netfn: 0x0A, Cmd: 0x10
int
bic_get_fruid_info(uint8_t slot_id, uint8_t fru_id __attribute__((unused)), ipmi_fruid_info_t *info, uint8_t intf) {
  uint8_t rlen = 0;
  uint8_t fruid = 0;
  return bic_ipmb_send(slot_id, NETFN_STORAGE_REQ, CMD_STORAGE_GET_FRUID_INFO, &fruid, 1, (uint8_t *) info, &rlen, intf);
}

// Storage - Reserve SDR
// Netfn: 0x0A, Cmd: 0x22
static int
_get_sdr_rsv(uint8_t slot_id, uint8_t *rsv, uint8_t intf) {
  uint8_t rlen = 0;
  return bic_ipmb_send(slot_id, NETFN_STORAGE_REQ, CMD_STORAGE_RSV_SDR, NULL, 0, (uint8_t *) rsv, &rlen, intf);
}

// Storage - Get SDR
// Netfn: 0x0A, Cmd: 0x23
static int
_get_sdr(uint8_t slot_id, ipmi_sel_sdr_req_t *req, ipmi_sel_sdr_res_t *res, uint8_t *rlen, uint8_t intf) {
  return bic_ipmb_send(slot_id, NETFN_STORAGE_REQ, CMD_STORAGE_GET_SDR, (uint8_t *)req, sizeof(ipmi_sel_sdr_req_t), (uint8_t*)res, rlen, intf);
}

int
bic_get_sdr(uint8_t slot_id, ipmi_sel_sdr_req_t *req, ipmi_sel_sdr_res_t *res, uint8_t *rlen, uint8_t intf) {
  int ret;
  uint8_t tbuf[MAX_IPMB_RES_LEN] = {0};
  uint8_t rbuf[MAX_IPMB_RES_LEN] = {0};
  uint8_t tlen;
  uint8_t len;
  ipmi_sel_sdr_res_t *tres;
  sdr_rec_hdr_t *hdr;

  tres = (ipmi_sel_sdr_res_t *) tbuf;

  // Get SDR reservation ID for the given record
  ret = _get_sdr_rsv(slot_id, rbuf, intf);
  if (ret) {
    syslog(LOG_ERR, "%s() _get_sdr_rsv returns %d\n", __func__, ret);
    return ret;
  }

  req->rsv_id = (rbuf[1] << 8) | rbuf[0];

  // Initialize the response length to zero
  *rlen = 0;

  // Read SDR Record Header
  req->offset = 0;
  req->nbytes = sizeof(sdr_rec_hdr_t);

  ret = _get_sdr(slot_id, req, (ipmi_sel_sdr_res_t *)tbuf, &tlen, intf);
  if (ret) {
    syslog(LOG_ERR, "%s() _get_sdr returns %d\n", __func__, ret);
    return ret;
  }

  // Copy the next record id to response
  res->next_rec_id = tres->next_rec_id;

  // Copy the header excluding first two bytes(next_rec_id)
  memcpy(res->data, tres->data, tlen-2);

  // Update response length and offset for next requesdr_rec_hdr_t
  *rlen += tlen-2;
  req->offset = tlen-2;

  // Find length of data from header info
  hdr = (sdr_rec_hdr_t *) tres->data;
  len = hdr->len;

  // Keep reading chunks of SDR record in a loop
  while (len > 0) {
    if (len > SDR_READ_COUNT_MAX) {
      req->nbytes = SDR_READ_COUNT_MAX;
    } else {
      req->nbytes = len;
    }

    ret = _get_sdr(slot_id, req, (ipmi_sel_sdr_res_t *)tbuf, &tlen, intf);
    if (ret) {
      syslog(LOG_ERR, "%s() _get_sdr returns %d\n", __func__, ret);
      return ret;
    }

    // Copy the data excluding the first two bytes(next_rec_id)
    memcpy(&res->data[req->offset], tres->data, tlen-2);

    // Update response length, offset for next request, and remaining length
    *rlen += tlen-2;
    req->offset += tlen-2;
    len -= tlen-2;
  }

  return BIC_STATUS_SUCCESS;
}

// Storage - Get SDR
// Netfn: 0x0A, Cmd: 0x11
static int
_read_fruid(uint8_t slot_id, uint8_t fru_id, uint32_t offset, uint8_t count, uint8_t *rbuf, uint8_t *rlen, uint8_t intf) {
  uint8_t tbuf[4] = {0};
  uint8_t tlen;

  tbuf[0] = fru_id;
  tbuf[1] = offset & 0xFF;
  tbuf[2] = (offset >> 8) & 0xFF;
  tbuf[3] = count;
  tlen = 4;
  return bic_ipmb_send(slot_id, NETFN_STORAGE_REQ, CMD_STORAGE_READ_FRUID_DATA, tbuf, tlen, rbuf, rlen, intf);
}

static int
_zero_chksum(uint8_t *data, uint16_t len) {
  bool is_valid = false;
  uint8_t checksum = 0;

  for (int i = 0; i < (len - 1); i++ ) {
    // if all data is 0, its checksum would be 0.
    // try to avoid the false positive since one of them should be > 0
    if ( is_valid == false && data[i] > 0 ) is_valid = true;

    // calc zero-checksum
    checksum += data[i];
  }
  checksum = ~(checksum) + 1;

  if ( (is_valid == false) || (checksum != data[len-1]) ) {
    return BIC_STATUS_FAILURE;
  }

  return BIC_STATUS_SUCCESS;
}

static int
_verify_fruid(uint8_t *data, int fru_size) {
  enum {
    HDR_CHASSIS_AREA_IDX = 0x2,
    HDR_BOARD_AREA_IDX,
    HDR_PRODUCT_AREA_IDX,
    HDR_FIELD_END,
  };

  // check header - 8 bytes including its checksum
  if ( _zero_chksum(data, 8) < 0 ) {
    syslog(LOG_WARNING,"%s() check header failed", __func__);
    return BIC_STATUS_FAILURE;
  }

  // start checking from chassis(2) to product(5)
  // for internal and multi-record area, they are not supported.
  for (int i = HDR_CHASSIS_AREA_IDX; i < HDR_FIELD_END && i < fru_size; i++ ) {
    uint8_t *st_idx = &data[i];
    if ( *st_idx == 0 ) continue;
    *st_idx *= 8; // in multiples of 8 bytes
    if ( _zero_chksum(&data[*st_idx], data[*(st_idx)+1]*8) < 0 ) {
      syslog(LOG_WARNING,"%s() check %d fru field failed", __func__, i);
      return BIC_STATUS_FAILURE;
    }
  }

  return BIC_STATUS_SUCCESS;
}

bool
bic_is_crit_act_ongoing(uint8_t fruid __attribute__((unused))) {
  char value[MAX_VALUE_LEN] = {0};
  int ret = kv_get("pwr_lock", value, NULL, 0);

  if (ret < 0) {
    // power lock hasn't been asserted
    if (errno == ENOENT) {
      // return false because there is no critical activity in progress
      return false;
    }

    syslog(LOG_WARNING, "%s() Cannot get pwr_lock", __func__);
    return true;
  }

  // if pwr_lock is 1, return true
  return (strncmp(value, "1", 1) == 0)?true:false;
}

#define FW_UPDATING_STR "fru%d_fwupd"
bool
bic_is_fw_update_ongoing(uint8_t fruid) {
  char key[MAX_KEY_LEN] = {0};
  char value[MAX_VALUE_LEN] = {0};
  int ret = BIC_STATUS_SUCCESS;
  struct timespec ts;

  sprintf(key, FW_UPDATING_STR, fruid);
  ret = kv_get(key, value, NULL, 0);
  if (ret < 0) {
     return false;
  }

  clock_gettime(CLOCK_MONOTONIC, &ts);
  if (strtoul(value, NULL, 10) > (long unsigned int)ts.tv_sec)
     return true;

  return false;
}

int
_set_fw_update_ongoing(uint8_t slot_id, uint16_t tmout) {
  char key[MAX_KEY_LEN] = {0};
  char value[MAX_VALUE_LEN] = {0};
  struct timespec ts;

  sprintf(key, FW_UPDATING_STR, slot_id);

  clock_gettime(CLOCK_MONOTONIC, &ts);
  ts.tv_sec += tmout;
  sprintf(value, "%ld", ts.tv_sec);

  if (kv_set(key, value, 0, 0) < 0) {
     return -1;
  }

  return 0;
}

int
bic_read_fruid(uint8_t slot_id, uint8_t fru_id, const char *path, int *fru_size, uint8_t intf, uint8_t less_retry) {
#define RETRY_DELAY 3
  int ret = BIC_STATUS_FAILURE;
  uint32_t nread = 0;
  uint32_t offset = 0;
  uint8_t count = 0;
  uint8_t rbuf[MAX_IPMB_RES_LEN] = {0};
  uint8_t rlen = 0;
  int fd = 0;
  ipmi_fruid_info_t info = {0};
  uint8_t *buf = NULL;
  int retry = 0, retryMax = less_retry == 1 ? RETRY_3_TIME : RETRY_TIME;

  // Remove the file if exists already
  unlink(path);

  // Open the file exclusively for write
  fd = open(path, O_WRONLY | O_CREAT | O_EXCL, 0666);
  if (fd < 0) {
    syslog(LOG_ERR, "bic_read_fruid: open fails for path: %s\n", path);
    goto error_exit;
  }

  while ( retry < RETRY_3_TIME ) {
    if ( bic_is_fw_update_ongoing(slot_id) == true ) {
      sleep(5);
      continue;
    }

    // Read the FRUID information
    ret = bic_get_fruid_info(slot_id, fru_id, &info, intf);
    if ( ret < 0 ) {
      syslog(LOG_ERR, "bic_read_fruid: bic_read_fruid_info returns %d\n", ret);
      sleep(RETRY_DELAY * (retry + 1));
    } else break;
    retry++;
  }

  if ( retry == RETRY_3_TIME ) goto error_exit;
  else ret = BIC_STATUS_FAILURE;

  // Indicates the size of the FRUID
  nread = (info.size_msb << 8) | info.size_lsb;
  if ( nread > FRUID_SIZE ) nread = FRUID_SIZE;
  else if ( nread == 0 ) goto error_exit;

  *fru_size = nread;

  // Allocate buffer
  buf = (uint8_t *)malloc(*fru_size);
  if ( buf == NULL ) {
    printf("Failed to malloc memory for fruid\n");
    goto error_exit;
  }

  retry = 0;
  while ( retry < retryMax) {
    if ( bic_is_fw_update_ongoing(slot_id) == true ) {
      sleep(5);
      continue;
    }

    // Read chunks of FRUID binary data in a loop
    offset = 0;
    nread = *fru_size;
    lseek(fd, 0, SEEK_SET);

    while ( nread > 0 ) {
      if (nread > FRUID_READ_COUNT_MAX) count = FRUID_READ_COUNT_MAX;
      else count = nread;

      ret = _read_fruid(slot_id, fru_id, offset, count, rbuf, &rlen, intf);
      if ( ret < 0 ) {
        syslog(LOG_ERR, "bic_read_fruid: ipmb_wrapper fails\n");
        break;
      }

      // Ignore the first byte as it indicates length of response
      if ( write(fd, &rbuf[1], rlen-1) != (rlen-1) ) {
        syslog(LOG_ERR, "bic_read_fruid: write to FRU failed\n");
        ret = BIC_STATUS_FAILURE;
        break;
      }

      // Update offset
      memcpy(&buf[offset], &rbuf[1], rlen-1);
      offset += (rlen-1);
      nread -= (rlen-1);
    }

    if ( ret == BIC_STATUS_SUCCESS ) {
      ret = _verify_fruid(buf, *fru_size);
    }

    if ( ret < 0 ) {
      syslog(LOG_WARNING, "%s() verified fruid failed, slot:%d, retry:%d, intf:%02X, size: %d\n", __func__, slot_id, retry, intf, offset);

      if (less_retry) {
        sleep(3);
      } else {
        sleep(RETRY_DELAY * (retry + 1));
      }
    } else break;

    retry++;
  }

error_exit:
  if ( buf != NULL ) free(buf);
  if ( fd > 0 ) close(fd);

  return ret;
}

// Storage - Get SDR
// Netfn: 0x0A, Cmd: 0x12
static int
_write_fruid(uint8_t slot_id, uint8_t fru_id, uint32_t offset, uint8_t count, uint8_t *buf, uint8_t intf) {
  int ret;
  uint8_t tbuf[64] = {0};
  uint8_t rbuf[MAX_IPMB_RES_LEN] = {0};
  uint8_t tlen = 0;
  uint8_t rlen = 0;

  tbuf[0] = fru_id;
  tbuf[1] = offset & 0xFF;
  tbuf[2] = (offset >> 8) & 0xFF;
  memcpy(&tbuf[3], buf, count);
  tlen = count + 3;

  ret = bic_ipmb_send(slot_id, NETFN_STORAGE_REQ, CMD_STORAGE_WRITE_FRUID_DATA, tbuf, tlen, rbuf, &rlen, intf);
  if ( rbuf[0] != count ) {
    syslog(LOG_WARNING, "%s() Failed to write fruid data. %d != %d \n", __func__, rbuf[0], count);
    return -1;
  }

  return ret;
}

int
bic_write_fruid(uint8_t slot_id, uint8_t fru_id, const char *path, uint8_t intf) {
  int ret = -1;
  uint32_t offset;
  int8_t count;
  uint8_t buf[64] = {0};
  int fd;

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
    count = read(fd, buf, FRUID_WRITE_COUNT_MAX);
    if (count <= 0) {
      break;
    }

    // Write to the FRUID
    ret = _write_fruid(slot_id, fru_id, offset, count, buf, intf);
    if (ret) {
      break;
    }

    // Update counter
    offset += count;
  }

error_exit:
  if (fd > 0 ) {
    close(fd);
  }

  return ret;
}

// OEM - Get Firmware Version
// Netfn: 0x38, Cmd: 0x0B
static int
_bic_get_fw_ver(uint8_t slot_id, uint8_t fw_comp, uint8_t *ver, uint8_t intf) {
  uint8_t tbuf[4] = {0x9c, 0x9c, 0x00, fw_comp}; //IANA ID + FW_COMP
  uint8_t rbuf[16] = {0x00};
  uint8_t rlen = 0;
  int ret = BIC_STATUS_FAILURE;

  ret = bic_ipmb_send(slot_id, NETFN_OEM_1S_REQ, CMD_OEM_1S_GET_FW_VER, tbuf, 4, rbuf, &rlen, intf);
  // rlen should be greater than or equal to 4 (IANA + Data1 +...+ DataN)
  if ( ret < 0 || rlen < 4 ) {
    syslog(LOG_ERR, "%s: ret: %d, rlen: %d, slot_id:%x, intf:%x\n", __func__, ret, rlen, slot_id, intf);
  } else {
    //Ignore IANA ID
    memcpy(ver, &rbuf[3], rlen-3);
    ret = BIC_STATUS_SUCCESS;
  }

  return ret;
}

// It's an API and provided to fw-util
int
bic_get_fw_ver(uint8_t slot_id, uint8_t comp, uint8_t *ver) {
  uint8_t fw_comp = 0x0;
  uint8_t intf = 0x0;
  int ret = BIC_STATUS_FAILURE;

  //get the component
  switch (comp) {
    case FW_1OU_BIC:
    case FW_2OU_BIC:
    case FW_BB_BIC:
    case FW_CWC_BIC:
    case FW_GPV3_TOP_BIC:
    case FW_GPV3_BOT_BIC:
      fw_comp = FW_BIC;
      break;
    case FW_1OU_BIC_BOOTLOADER:
    case FW_2OU_BIC_BOOTLOADER:
    case FW_BB_BIC_BOOTLOADER:
    case FW_CWC_BIC_BL:
    case FW_GPV3_TOP_BIC_BL:
    case FW_GPV3_BOT_BIC_BL:
      fw_comp = FW_BIC_BOOTLOADER;
      break;
    case FW_CWC_CPLD:
    case FW_GPV3_TOP_CPLD:
    case FW_GPV3_BOT_CPLD:
      fw_comp = FW_2OU_CPLD;
      break;
    case FW_CWC_PESW:
    case FW_GPV3_TOP_PESW:
    case FW_GPV3_BOT_PESW:
      fw_comp = FW_2OU_PESW_FW_VER;
      break;
    case FW_CWC_PESW_VR:
    case FW_GPV3_TOP_PESW_VR:
    case FW_GPV3_BOT_PESW_VR:
      fw_comp = FW_2OU_PESW_VR;
      break;
    case FW_2U_TOP_PESW_CFG_VER:
    case FW_2U_BOT_PESW_CFG_VER:
      fw_comp = FW_2OU_PESW_CFG_VER;
      break;
    case FW_2U_TOP_PESW_FW_VER:
    case FW_2U_BOT_PESW_FW_VER:
      fw_comp = FW_2OU_PESW_FW_VER;
      break;
    case FW_2U_TOP_PESW_BL0_VER:
    case FW_2U_BOT_PESW_BL0_VER:
      fw_comp = FW_2OU_PESW_BL0_VER;
      break;
    case FW_2U_TOP_PESW_BL1_VER:
    case FW_2U_BOT_PESW_BL1_VER:
      fw_comp = FW_2OU_PESW_BL1_VER;
      break;
    case FW_2U_TOP_PESW_PART_MAP0_VER:
    case FW_2U_BOT_PESW_PART_MAP0_VER:
      fw_comp = FW_2OU_PESW_PART_MAP0_VER;
      break;
    case FW_2U_TOP_PESW_PART_MAP1_VER:
    case FW_2U_BOT_PESW_PART_MAP1_VER:
      fw_comp = FW_2OU_PESW_PART_MAP1_VER;
      break;
    case FW_2U_TOP_3V3_VR1:
    case FW_2U_BOT_3V3_VR1:
      fw_comp = FW_2OU_3V3_VR1;
      break;
    case FW_2U_TOP_3V3_VR2:
    case FW_2U_BOT_3V3_VR2:
      fw_comp = FW_2OU_3V3_VR2;
      break;
    case FW_2U_TOP_3V3_VR3:
    case FW_2U_BOT_3V3_VR3:
      fw_comp = FW_2OU_3V3_VR3;
      break;
    case FW_2U_TOP_1V8_VR:
    case FW_2U_BOT_1V8_VR:
      fw_comp = FW_2OU_1V8_VR;
      break;
    default:
      fw_comp = comp;
      break;
  }

  //get the intf
  switch (comp) {
    case FW_CPLD:
    case FW_ME:
    case FW_BIC:
    case FW_BIC_BOOTLOADER:
      intf = NONE_INTF;
      break;
    case FW_1OU_BIC:
    case FW_1OU_BIC_BOOTLOADER:
    case FW_1OU_CPLD:
      intf = FEXP_BIC_INTF;
      break;
    case FW_2OU_BIC:
    case FW_2OU_BIC_BOOTLOADER:
    case FW_2OU_CPLD:
    case FW_2OU_3V3_VR1:
    case FW_2OU_3V3_VR2:
    case FW_2OU_3V3_VR3:
    case FW_2OU_1V8_VR:
    case FW_2OU_PESW_VR:
    case FW_2OU_PESW_CFG_VER:
    case FW_2OU_PESW_FW_VER:
    case FW_2OU_PESW_BL0_VER:
    case FW_2OU_PESW_BL1_VER:
    case FW_2OU_PESW_PART_MAP0_VER:
    case FW_2OU_PESW_PART_MAP1_VER:
    case FW_CWC_BIC:
    case FW_CWC_BIC_BL:
    case FW_CWC_CPLD:
    case FW_CWC_PESW:
    case FW_CWC_PESW_VR:
      intf = REXP_BIC_INTF;
      break;
    case FW_BB_BIC:
    case FW_BB_BIC_BOOTLOADER:
    case FW_BB_CPLD:
      intf = BB_BIC_INTF;
      break;
    case FW_GPV3_TOP_BIC:
    case FW_GPV3_TOP_BIC_BL:
    case FW_GPV3_TOP_CPLD:
    case FW_GPV3_TOP_PESW:
    case FW_GPV3_TOP_PESW_VR:
    case FW_2U_TOP_PESW_CFG_VER:
    case FW_2U_TOP_PESW_FW_VER:
    case FW_2U_TOP_PESW_BL0_VER:
    case FW_2U_TOP_PESW_BL1_VER:
    case FW_2U_TOP_PESW_PART_MAP0_VER:
    case FW_2U_TOP_PESW_PART_MAP1_VER:
    case FW_2U_TOP_3V3_VR1:
    case FW_2U_TOP_3V3_VR2:
    case FW_2U_TOP_3V3_VR3:
    case FW_2U_TOP_1V8_VR:
      intf = RREXP_BIC_INTF1;
      break;
    case FW_GPV3_BOT_BIC:
    case FW_GPV3_BOT_BIC_BL:
    case FW_GPV3_BOT_CPLD:
    case FW_GPV3_BOT_PESW:
    case FW_GPV3_BOT_PESW_VR:
    case FW_2U_BOT_PESW_CFG_VER:
    case FW_2U_BOT_PESW_FW_VER:
    case FW_2U_BOT_PESW_BL0_VER:
    case FW_2U_BOT_PESW_BL1_VER:
    case FW_2U_BOT_PESW_PART_MAP0_VER:
    case FW_2U_BOT_PESW_PART_MAP1_VER:
    case FW_2U_BOT_3V3_VR1:
    case FW_2U_BOT_3V3_VR2:
    case FW_2U_BOT_3V3_VR3:
    case FW_2U_BOT_1V8_VR:
      intf = RREXP_BIC_INTF2;
      break;
  }

  //run cmd
  switch (comp) {
    case FW_CPLD:
    case FW_ME:
    case FW_BIC:
    case FW_BIC_BOOTLOADER:
    case FW_1OU_BIC:
    case FW_1OU_BIC_BOOTLOADER:
    case FW_2OU_BIC:
    case FW_2OU_BIC_BOOTLOADER:
    case FW_2OU_3V3_VR1:
    case FW_2OU_3V3_VR2:
    case FW_2OU_3V3_VR3:
    case FW_2OU_1V8_VR:
    case FW_2OU_PESW_VR:
    case FW_2OU_PESW_CFG_VER:
    case FW_2OU_PESW_FW_VER:
    case FW_2OU_PESW_BL0_VER:
    case FW_2OU_PESW_BL1_VER:
    case FW_2OU_PESW_PART_MAP0_VER:
    case FW_2OU_PESW_PART_MAP1_VER:
    case FW_BB_BIC:
    case FW_BB_BIC_BOOTLOADER:
    case FW_CWC_BIC:
    case FW_GPV3_TOP_BIC:
    case FW_GPV3_BOT_BIC:
    case FW_CWC_BIC_BL:
    case FW_GPV3_TOP_BIC_BL:
    case FW_GPV3_BOT_BIC_BL:
    case FW_CWC_PESW:
    case FW_GPV3_TOP_PESW:
    case FW_GPV3_BOT_PESW:
    case FW_CWC_CPLD:
    case FW_GPV3_TOP_CPLD:
    case FW_GPV3_BOT_CPLD:
    case FW_CWC_PESW_VR:
    case FW_GPV3_TOP_PESW_VR:
    case FW_GPV3_BOT_PESW_VR:
    case FW_2U_TOP_PESW_CFG_VER:
    case FW_2U_TOP_PESW_FW_VER:
    case FW_2U_TOP_PESW_BL0_VER:
    case FW_2U_TOP_PESW_BL1_VER:
    case FW_2U_TOP_PESW_PART_MAP0_VER:
    case FW_2U_TOP_PESW_PART_MAP1_VER:
    case FW_2U_BOT_PESW_CFG_VER:
    case FW_2U_BOT_PESW_FW_VER:
    case FW_2U_BOT_PESW_BL0_VER:
    case FW_2U_BOT_PESW_BL1_VER:
    case FW_2U_BOT_PESW_PART_MAP0_VER:
    case FW_2U_BOT_PESW_PART_MAP1_VER:
    case FW_2U_TOP_3V3_VR1:
    case FW_2U_TOP_3V3_VR2:
    case FW_2U_TOP_3V3_VR3:
    case FW_2U_TOP_1V8_VR:
    case FW_2U_BOT_3V3_VR1:
    case FW_2U_BOT_3V3_VR2:
    case FW_2U_BOT_3V3_VR3:
    case FW_2U_BOT_1V8_VR:
      ret = _bic_get_fw_ver(slot_id, fw_comp, ver, intf);
      break;
    case FW_1OU_CPLD:
      ret = bic_get_exp_cpld_ver(slot_id, fw_comp, ver, 0/*bus 0*/, 0x80/*8-bit addr*/, intf);
      break;
    case FW_2OU_CPLD:
      {
        uint8_t board_type = 0;
        if ( fby3_common_get_2ou_board_type(slot_id, &board_type) < 0 ) {
          syslog(LOG_WARNING, "Failed to get 2ou board type\n");
        }

        if ( board_type == GPV3_MCHP_BOARD ||
             board_type == GPV3_BRCM_BOARD ) {
          ret = _bic_get_fw_ver(slot_id, fw_comp, ver, intf);
        } else {
          ret = bic_get_exp_cpld_ver(slot_id, fw_comp, ver, 0/*bus 0*/, 0x80/*8-bit addr*/, intf);
        }
      }
      break;
    case FW_BB_CPLD:
      ret = bic_get_cpld_ver(slot_id, fw_comp, ver, 0/*bus 0*/, 0x80/*8-bit addr*/, intf);
      break;
  }

  return ret;
}

// OEM - Get Firmware Version
// Netfn: 0x38, Cmd: 0x0B
static int
_bic_get_vr_vendor_fw_ver(uint8_t slot_id, uint8_t fw_comp, uint8_t *ver, uint8_t intf, uint8_t *rlen) {
  uint8_t tbuf[4] = {0x9c, 0x9c, 0x00, fw_comp}; //IANA ID + FW_COMP
  uint8_t rbuf[16] = {0x00};
  int ret = BIC_STATUS_FAILURE;

  ret = bic_ipmb_send(slot_id, NETFN_OEM_1S_REQ, CMD_OEM_1S_GET_FW_VER, tbuf, 4, rbuf, rlen, intf);
  if ( ret < 0 || *rlen < 4 ) {
    syslog(LOG_ERR, "%s: ret: %d, rlen: %d, slot_id:%x, intf:%x\n", __func__, ret, *rlen, slot_id, intf);
  } else {
    //Ignore IANA ID
    *rlen = *rlen - 3;
    memcpy(ver, &rbuf[3], *rlen);
    ret = BIC_STATUS_SUCCESS;
  }
  return ret;
}

int
bic_get_vr_vendor_fw_ver(uint8_t slot_id, uint8_t comp, uint8_t *ver, uint8_t *rlen) {
  uint8_t fw_comp = 0x0;
  uint8_t intf = 0x0;
  int ret = BIC_STATUS_FAILURE;

  //get the component
  switch(comp) {
    case FW_CWC_PESW_VR:
    case FW_GPV3_TOP_PESW_VR:
    case FW_GPV3_BOT_PESW_VR:
      fw_comp = FW_2OU_PESW_VR;
      break;
    case FW_2U_TOP_3V3_VR1:
    case FW_2U_BOT_3V3_VR1:
      fw_comp = FW_2OU_3V3_VR1;
      break;
    case FW_2U_TOP_3V3_VR2:
    case FW_2U_BOT_3V3_VR2:
      fw_comp = FW_2OU_3V3_VR2;
      break;
    case FW_2U_TOP_3V3_VR3:
    case FW_2U_BOT_3V3_VR3:
      fw_comp = FW_2OU_3V3_VR3;
      break;
    case FW_2U_TOP_1V8_VR:
    case FW_2U_BOT_1V8_VR:
      fw_comp = FW_2OU_1V8_VR;
      break;
    default:
      fw_comp = comp;
      break;
  }

  // get the intf
  switch (comp) {
    case FW_2OU_3V3_VR1:
    case FW_2OU_3V3_VR2:
    case FW_2OU_3V3_VR3:
    case FW_2OU_1V8_VR:
    case FW_2OU_PESW_VR:
    case FW_CWC_PESW_VR:
      intf = REXP_BIC_INTF;
      break;
    case FW_GPV3_TOP_PESW_VR:
    case FW_2U_TOP_3V3_VR1:
    case FW_2U_TOP_3V3_VR2:
    case FW_2U_TOP_3V3_VR3:
    case FW_2U_TOP_1V8_VR:
      intf = RREXP_BIC_INTF1;
      break;
    case FW_GPV3_BOT_PESW_VR:
    case FW_2U_BOT_3V3_VR1:
    case FW_2U_BOT_3V3_VR2:
    case FW_2U_BOT_3V3_VR3:
    case FW_2U_BOT_1V8_VR:
      intf = RREXP_BIC_INTF2;
      break;
  }

  // run cmd
  switch (comp) {
    case FW_2OU_3V3_VR1:
    case FW_2OU_3V3_VR2:
    case FW_2OU_3V3_VR3:
    case FW_2OU_1V8_VR:
    case FW_2OU_PESW_VR:
    case FW_CWC_PESW_VR:
    case FW_GPV3_TOP_PESW_VR:
    case FW_GPV3_BOT_PESW_VR:
    case FW_2U_TOP_3V3_VR1:
    case FW_2U_TOP_3V3_VR2:
    case FW_2U_TOP_3V3_VR3:
    case FW_2U_TOP_1V8_VR:
    case FW_2U_BOT_3V3_VR1:
    case FW_2U_BOT_3V3_VR2:
    case FW_2U_BOT_3V3_VR3:
    case FW_2U_BOT_1V8_VR:
      ret = _bic_get_vr_vendor_fw_ver(slot_id, fw_comp, ver, intf, rlen);
      break;
  }
  return ret;
}

uint8_t
get_gpv3_bus_number(uint8_t dev_id) {
  switch(dev_id) {
    case FW_2OU_M2_DEV0:
    case FW_2OU_M2_DEV1:
    case DEV_ID0_2OU:
    case DEV_ID1_2OU:
      return 0x2;
    case FW_2OU_M2_DEV2:
    case FW_2OU_M2_DEV3:
    case DEV_ID2_2OU:
    case DEV_ID3_2OU:
      return 0x4;
    case FW_2OU_M2_DEV4:
    case FW_2OU_M2_DEV5:
    case DEV_ID4_2OU:
    case DEV_ID5_2OU:
      return 0x6;
    case FW_2OU_M2_DEV6:
    case FW_2OU_M2_DEV7:
    case DEV_ID6_2OU:
    case DEV_ID7_2OU:
      return 0x5;
    case FW_2OU_M2_DEV8:
    case FW_2OU_M2_DEV9:
    case DEV_ID8_2OU:
    case DEV_ID9_2OU:
      return 0x7;
    case FW_2OU_M2_DEV10:
    case FW_2OU_M2_DEV11:
    case DEV_ID10_2OU:
    case DEV_ID11_2OU:
      return 0x3;
    case DEV_ID12_2OU:
    case DEV_ID13_2OU:
      return 0x9;
  }

  return 0xff;
}

uint8_t
get_gpv3_channel_number(uint8_t dev_id) {
  switch(dev_id) {
    case FW_2OU_M2_DEV0:
    case FW_2OU_M2_DEV2:
    case FW_2OU_M2_DEV4:
    case FW_2OU_M2_DEV6:
    case FW_2OU_M2_DEV8:
    case FW_2OU_M2_DEV10:
    case DEV_ID0_2OU:
    case DEV_ID2_2OU:
    case DEV_ID4_2OU:
    case DEV_ID6_2OU:
    case DEV_ID8_2OU:
    case DEV_ID10_2OU:
      return 0x0;
    case FW_2OU_M2_DEV1:
    case FW_2OU_M2_DEV3:
    case FW_2OU_M2_DEV5:
    case FW_2OU_M2_DEV7:
    case FW_2OU_M2_DEV9:
    case FW_2OU_M2_DEV11:
    case DEV_ID1_2OU:
    case DEV_ID3_2OU:
    case DEV_ID5_2OU:
    case DEV_ID7_2OU:
    case DEV_ID9_2OU:
    case DEV_ID11_2OU:
      return 0x1;
    case DEV_ID12_2OU: // E1.S A
      return 0x4;
    case DEV_ID13_2OU: // E1.S B
      return 0x8;
  }

  return 0xff;
}

int
bic_enable_vr_fault_monitor(uint8_t slot_id, bool enable, uint8_t intf) {
  uint8_t tbuf[4] = {0x9c, 0x9c, 0x00, ( enable == true )?0x1:0x0};
  uint8_t tlen = 4;
  uint8_t rbuf[24] = {0};
  uint8_t rlen = 0;
  return bic_ipmb_send(slot_id, NETFN_OEM_1S_REQ, BIC_CMD_OEM_BIC_VR_MONITOR, tbuf, tlen, rbuf, &rlen, intf);
}

int
bic_set_vr_monitor_enable(uint8_t slot_id, bool enable, uint8_t intf) {
  uint8_t tbuf[SIZE_IANA_ID + 1] = {0};
  uint8_t rbuf[MAX_IPMB_RES_LEN] = {0};
  uint8_t rlen = 0;

  memcpy(tbuf, (uint8_t *)&IANA_ID, SIZE_IANA_ID);
  tbuf[3] = (uint8_t)enable;  // 1: enable vr monitor; 0: disable vr monitor

  return bic_ipmb_send(slot_id, NETFN_OEM_1S_REQ, CMD_OEM_1S_SET_VR_MON_ENABLE, tbuf, (uint8_t)sizeof(tbuf), rbuf, &rlen, intf);
}

int
bic_get_gpv3_pci_link(uint8_t slot_id, uint8_t *rbuf, uint8_t *rlen, uint8_t intf) {
  uint8_t tbuf[3] = {0x9c, 0x9c, 0x00};
  uint8_t tlen = 3;
  return bic_ipmb_send(slot_id, NETFN_OEM_1S_REQ, BIC_CMD_OEM_GET_PCI_LINK_STATUS, tbuf, tlen, rbuf, rlen, intf);
}

int
bic_enable_ssd_sensor_monitor(uint8_t slot_id, bool enable, uint8_t intf) {
  uint8_t tbuf[4] = {0x9c, 0x9c, 0x00, ( enable == true )?0x1:0x0};
  uint8_t tlen = 4;
  uint8_t rbuf[16] = {0};
  uint8_t rlen = 0;
  return bic_ipmb_send(slot_id, NETFN_OEM_1S_REQ, BIC_CMD_OEM_BIC_SNR_MONITOR, tbuf, tlen, rbuf, &rlen, intf);
}

int
bic_get_1ou_type(uint8_t slot_id, uint8_t *type) {
  uint8_t tbuf[3] = {0x9c, 0x9c, 0x00};
  uint8_t rbuf[16] = {0};
  uint8_t rlen = 0;
  int ret = 0;
  int retry = 0;
  char key[MAX_KEY_LEN] = {0};
  char tmp_str[MAX_VALUE_LEN] = {0};
  int val = 0;

  snprintf(key, sizeof(key), KV_SLOT_GET_1OU_TYPE, slot_id);

  while (retry < 3) {
    ret = bic_ipmb_send(slot_id, NETFN_OEM_1S_REQ, BIC_CMD_OEM_GET_BOARD_ID, tbuf, 3, rbuf, &rlen, FEXP_BIC_INTF);
    if (ret == 0) break;
    retry++;
  }

  if (ret == 0) {
    *type = rbuf[3];
    val = *type;
  } else {
    syslog(LOG_WARNING, "[%s] fail at slot%d", __func__, slot_id);
    val = ret;
  }

  snprintf(tmp_str, sizeof(tmp_str), "%d", val);
  kv_set(key, tmp_str, 0, 0);
  return ret;
}

int
bic_get_1ou_type_cache(uint8_t slot_id, uint8_t *type) {
  char key[MAX_KEY_LEN] = {0};
  char tmp_str[MAX_VALUE_LEN] = {0};
  int val = 0;

  snprintf(key, sizeof(key), KV_SLOT_GET_1OU_TYPE, slot_id);
  if (kv_get(key, tmp_str, NULL, 0))
    return -1;

  val = atoi(tmp_str);
  if (val < 0 || val > 255)
    return -1;

  *type = val;
  return 0;
}

int
bic_get_card_type(uint8_t slot_id, uint8_t select, uint8_t *type) {
  uint8_t tbuf[4] = {0x9c, 0x9c, 0x00, select};
  uint8_t rbuf[16] = {0};
  uint8_t tlen = sizeof(tbuf);
  uint8_t rlen = 0;

  int ret = 0;
  int val = 0;
  char key[MAX_KEY_LEN] = {0};
  char tmp_str[MAX_VALUE_LEN] = {0};

  if (type == NULL) {
    return -1;
  }

  if (select != GET_1OU) {
    printf("[%s] only support get 1OU card type \n", __func__);
    return -1;
  }

  snprintf(key, sizeof(key), KV_SLOT_GET_1OU_CARD_TYPE, slot_id);
  if (kv_get(key, tmp_str, NULL, 0) == 0) {
    *type = atoi(tmp_str);
    return 0;
  } else {
    ret = bic_ipmb_send(slot_id, NETFN_OEM_1S_REQ, BIC_CMD_OEM_GET_CARD_TYPE, tbuf, tlen, rbuf, &rlen, NONE_INTF);
    if (ret == 0) {
      *type = rbuf[3];
      val = *type;
      snprintf(tmp_str, sizeof(tmp_str), "%d", val);
      kv_set(key, tmp_str, 0, 0);
    }
  }

  return ret;
}

int
bic_set_amber_led(uint8_t slot_id, uint8_t dev_id, uint8_t status) {
  uint8_t tbuf[5] = {0x9c, 0x9c, 0x00, 0x00, 0x00};
  uint8_t rbuf[2] = {0};
  uint8_t rlen = 0;
  int ret = 0;
  int retry = 0;

  tbuf[3] = dev_id; // 0'base
  tbuf[4] = status; // 0->off, 1->on
  while (retry < 3) {
    ret = bic_ipmb_send(slot_id, NETFN_OEM_1S_REQ, BIC_CMD_OEM_SET_AMBER_LED, tbuf, 5, rbuf, &rlen, FEXP_BIC_INTF);
    if (ret == 0) break;
    retry++;
  }

  if (ret != 0) {
    syslog(LOG_WARNING, "[%s] fail at slot%u dev%u", __func__, slot_id, dev_id);
  }

  return ret;
}

int
bic_get_amber_led_status(uint8_t slot_id, uint8_t dev_id, uint8_t *status) {
  uint8_t tbuf[4] = {0x9c, 0x9c, 0x00, 0x00};
  uint8_t rbuf[4] = {0};
  uint8_t rlen = 0;
  int ret = 0;
  int retry = 0;

  tbuf[3] = dev_id; // 0'base
  while (retry < 3) {
    ret = bic_ipmb_send(slot_id, NETFN_OEM_1S_REQ, BIC_CMD_OEM_GET_AMBER_LED_STATUS, tbuf, 4, rbuf, &rlen, FEXP_BIC_INTF);
    if (ret == 0) break;
    retry++;
  }

  if (ret != 0) {
    syslog(LOG_WARNING, "[%s] fail at slot%u dev%u", __func__, slot_id, dev_id);
  } else {
    *status = rbuf[3];
  }

  return ret;
}

int
bic_spe_led_ctrl(uint8_t dev_id, uint8_t option, uint8_t* status) {
  uint8_t tbuf[MAX_IPMB_REQ_LEN] = {0x9c, 0x9c, 0x00, 0x00, 0x00};
  uint8_t rbuf[MAX_IPMB_RES_LEN] = {0};
  uint8_t rlen = 0;
  int ret = 0;
  int retry = 0;

  if (status == NULL) {
    syslog(LOG_WARNING, "%s() status is missing", __func__);
    return -1;
  }
  tbuf[3] = dev_id - DEV_ID0_2OU;
  tbuf[4] = option;
  while (retry < 3) {
    ret = bic_ipmb_send(FRU_SLOT1, NETFN_OEM_1S_REQ, BIC_CMD_OEM_SET_AMBER_LED, tbuf, 5, rbuf, &rlen, REXP_BIC_INTF);
    if (ret == 0) {
      break;
    }
    retry++;
  }

  if (ret != 0) {
    syslog(LOG_WARNING, "%s() fail at dev%u", __func__, dev_id);
  }
  if (option == GET_LED_STAT) {
    if (rlen != 4) {
      syslog(LOG_WARNING, "%s() failed to get LED status, rlen = %d", __func__, rlen);
      return -1;
    }
    // byte 0~2: IANA ID
    // byte 3: status
    *status = rbuf[3];
  }

  return ret;
}

// OEM - Get Post Code buffer
// Netfn: 0x38, Cmd: 0x12
int
bic_get_80port_record(uint8_t slot_id, uint8_t *rbuf, uint8_t *rlen, uint8_t intf) {
  int ret = 0;
  uint8_t tbuf[3] = {0};
  uint8_t tlen = sizeof(tbuf);

  // File the IANA ID
  memcpy(tbuf, (uint8_t *)&IANA_ID, 3);

  ret = bic_ipmb_send(slot_id, NETFN_OEM_1S_REQ, CMD_OEM_1S_GET_POST_BUF, tbuf, tlen, rbuf, rlen, intf);
  if ( ret < 0 ) {
    syslog(LOG_WARNING, "[%s] Cannot get the postcode buffer from slot%d", __func__, slot_id);
  } else {
    *rlen -= 3;
    memmove(rbuf, &rbuf[3], *rlen);
  }

  return ret;
}

// Custom Command for getting cpld version
int
bic_get_cpld_ver(uint8_t slot_id, uint8_t comp __attribute__((unused)), uint8_t *ver, uint8_t bus, uint8_t addr, uint8_t intf) {
  uint8_t tbuf[32] = {0};
  uint8_t rbuf[4] = {0};
  uint8_t tlen = 0;
  uint8_t rlen = 0;
  const uint32_t reg = 0x00200028; //for altera cpld

  tbuf[0] = (bus << 1) + 1;
  tbuf[1] = addr;
  tbuf[2] = 0x04; //read back 4 bytes
  tbuf[3] = (reg >> 24) & 0xff;
  tbuf[4] = (reg >> 16) & 0xff;
  tbuf[5] = (reg >> 8) & 0xff;
  tbuf[6] = (reg >> 0) & 0xff;
  tlen = 7;
  int ret = bic_ipmb_send(slot_id, NETFN_APP_REQ, CMD_APP_MASTER_WRITE_READ, tbuf, tlen, rbuf, &rlen, intf);
  for (int i = 0; i < rlen; i++) ver[i] = rbuf[3-i];
  return ret;
}

// Custom Command for getting vr version/device id
int
bic_get_vr_device_id(uint8_t slot_id, uint8_t comp __attribute__((unused)), uint8_t *rbuf, uint8_t *rlen, uint8_t bus, uint8_t addr, uint8_t intf) {
  uint8_t tbuf[32] = {0};
  uint8_t tlen = 0;
  int ret = 0;

  tbuf[0] = (bus << 1) + 1;
  tbuf[1] = addr;
  tbuf[2] = 0x07; //read back 7 bytes
  tbuf[3] = 0xAD; //get device id command
  tlen = 4;
  ret = bic_ipmb_send(slot_id, NETFN_APP_REQ, CMD_APP_MASTER_WRITE_READ, tbuf, tlen, rbuf, rlen, intf);
  if ( ret < 0 ) {
    syslog(LOG_WARNING, "%s() Failed to get vr device id, ret=%d", __func__, ret);
  } else {
    *rlen = rbuf[0];//read cnt
    memmove(rbuf, &rbuf[1], *rlen);
  }

  return ret;
}

int
bic_get_ifx_vr_remaining_writes(uint8_t slot_id, uint8_t bus, uint8_t addr, uint8_t *writes, uint8_t intf) {
#define REMAINING_TIMES(x) (((x[1] << 8) + x[0]) & 0xFC0) >> 6
  uint8_t tbuf[16] = {0};
  uint8_t rbuf[16] = {0};
  uint8_t tlen = 0;
  uint8_t rlen = 0;
  int ret = 0;

  tbuf[0] = (bus << 1) + 1;
  tbuf[1] = addr;
  tbuf[2] = 0x00; //read cnt
  tbuf[3] = VR_PAGE;
  tbuf[4] = VR_PAGE50;
  tlen = 5;
  ret = bic_ipmb_send(slot_id, NETFN_APP_REQ, CMD_APP_MASTER_WRITE_READ, tbuf, tlen, rbuf, &rlen, intf);
  if ( ret < 0 ) {
    syslog(LOG_WARNING, "%s() Couldn't set page to 0x%02X", __func__, tbuf[4]);
    goto error_exit;
  }

  tbuf[2] = 0x02; //read cnt
  tbuf[3] = 0x82;
  tlen = 4;
  ret = bic_ipmb_send(slot_id, NETFN_APP_REQ, CMD_APP_MASTER_WRITE_READ, tbuf, tlen, rbuf, &rlen, intf);
  if ( ret < 0 ) {
    syslog(LOG_WARNING, "%s() Couldn't get data from 0x%02X", __func__, tbuf[3]);
    goto error_exit;
  }

  *writes = REMAINING_TIMES(rbuf);

error_exit:
  return ret;
}

int
bic_get_isl_vr_remaining_writes(uint8_t slot_id, uint8_t bus, uint8_t addr, uint8_t *writes, uint8_t intf) {
  uint8_t tbuf[16] = {0};
  uint8_t rbuf[16] = {0};
  uint8_t tlen = 0;
  uint8_t rlen = 0;
  int ret = 0;

  tbuf[0] = (bus << 1) + 1;
  tbuf[1] = addr;
  tbuf[2] = 0x00; //read cnt
  tbuf[3] = 0xC7; //command code
  tbuf[4] = 0xC2; //data1
  tbuf[5] = 0x00; //data2
  tlen = 6;
  ret = bic_ipmb_send(slot_id, NETFN_APP_REQ, CMD_APP_MASTER_WRITE_READ, tbuf, tlen, rbuf, &rlen, intf);
  if ( ret < 0 ) {
    syslog(LOG_WARNING, "%s() Failed to send command and data...", __func__);
    goto error_exit;
  }

  tbuf[2] = 0x04; //read cnt
  tbuf[3] = 0xC5; //command code
  tlen = 4;
  ret = bic_ipmb_send(slot_id, NETFN_APP_REQ, CMD_APP_MASTER_WRITE_READ, tbuf, tlen, rbuf, &rlen, intf);
  if ( ret < 0 ) {
    syslog(LOG_WARNING, "%s() Failed to read NVM slots...", __func__);
    goto error_exit;
  }

  *writes = rbuf[0];

error_exit:
  return ret;
}

int
ast_bic_get_vr_ver(uint8_t slot_id, uint8_t intf, uint8_t addr, char *key, char *ver_str) {
  uint8_t tbuf[SIZE_IANA_ID + 1]; //IANA ID + FW_COMP
  uint8_t rbuf[MAX_IPMB_RES_LEN] = {0};
  uint8_t rlen = 0;
  uint8_t component = 0;
  ast_vr_ver_resp *vr_ver;
  int ret = BIC_STATUS_FAILURE;

  if (key == NULL || ver_str == NULL) {
    syslog(LOG_ERR, "%s: Null parameter\n", __func__);
    return BIC_STATUS_FAILURE;
  }

  switch(addr) {
  case VCCIN_ADDR:
    component = DL_COMPNT_PVCCIN;
    break;
  case VCCIO_ADDR:
    component = DL_COMPNT_PVCCIO;
    break;
  case VDDQ_ABC_ADDR:
    component = DL_COMPNT_PVDDR_ABC;
    break;
  case VDDQ_DEF_ADDR:
    component = DL_COMPNT_PVDDR_DEF;
    break;
  default:
    syslog(LOG_ERR, "%s: unsupported component %x\n", __func__, addr);
    return BIC_STATUS_FAILURE;
  }

  memcpy(&tbuf[0], (uint8_t *)&IANA_ID, SIZE_IANA_ID);
  tbuf[SIZE_IANA_ID] = component;

  ret = bic_ipmb_send(slot_id, NETFN_OEM_1S_REQ, CMD_OEM_1S_GET_FW_VER, tbuf, sizeof(tbuf), rbuf, &rlen, intf);
  if ( ret < 0 || rlen < sizeof(ast_vr_ver_resp)) {
    syslog(LOG_ERR, "%s: ret: %d, rlen: %d, slot_id:%x, intf:%x\n", __func__, ret, rlen, slot_id, intf);
  } else {
    vr_ver = (ast_vr_ver_resp*)rbuf;

    switch (vr_ver->vendor) {
    case VR_ISL:
      snprintf(ver_str, MAX_VALUE_LEN, "Renesas %02X%02X%02X%02X, Remaining Writes: %d",
        vr_ver->checksum[0], vr_ver->checksum[1], vr_ver->checksum[2], vr_ver->checksum[3], vr_ver->remaining_writes);
      break;
    case VR_IFX:
      snprintf(ver_str, MAX_VALUE_LEN, "Infineon %02X%02X%02X%02X, Remaining Writes: %d",
        vr_ver->checksum[0], vr_ver->checksum[1], vr_ver->checksum[2], vr_ver->checksum[3], vr_ver->remaining_writes);
      break;
    default:
      syslog(LOG_ERR, "%s: unsupported vendor %x\n", __func__, vr_ver->vendor);
      return BIC_STATUS_FAILURE;
    }
    
    kv_set(key, ver_str, 0, 0);
    ret = BIC_STATUS_SUCCESS;
  }

  return ret;
}

int
bic_get_vr_ver(uint8_t slot_id, uint8_t intf, uint8_t bus, uint8_t addr, char *key, char *ver_str) {
  uint8_t tbuf[32] = {0};
  uint8_t tlen = 0;
  uint8_t rbuf[32] = {0};
  uint8_t rlen = 0;
  uint8_t remaining_writes = 0;
  char path[128];
  int fd = 0;
  int ret = 0, rc = 0;

  ret = bic_get_vr_device_id(slot_id, 0, rbuf, &rlen, bus, addr, intf);
  if ( ret < 0 ) {
    syslog(LOG_WARNING, "%s() Failed to get vr device id, ret=%d", __func__, ret);
    return ret;
  }

  tbuf[0] = (bus << 1) + 1;
  tbuf[1] = addr;

  //The length of GET_DEVICE_ID is different.
  if ( rlen == 2 ) {
    //Infineon
    //to avoid sensord changing the page of VRs, so use the LOCK file
    //to stop monitoring sensors of VRs
    sprintf(path, SLOT_SENSOR_LOCK, slot_id);
    fd = open(path, O_CREAT | O_RDWR, 0666);
    rc = flock(fd, LOCK_EX | LOCK_NB);
    if(rc) {
      if(EWOULDBLOCK == errno) {
        return -1;
      }
    }

    //get the remaining writes of VRs
    ret = bic_get_ifx_vr_remaining_writes(slot_id, bus, addr, &remaining_writes, intf);
    if ( ret < 0 ) {
      syslog(LOG_WARNING, "%s():%d Failed to send command code to get vr remaining writes. ret=%d", __func__,__LINE__, ret);
      goto error_exit;
    }

    //get the CRC32 of the VR
    tbuf[2] = 0x00; //read cnt
    tbuf[3] = 0x00; //command code
    tbuf[4] = 0x62;
    tlen = 5;

    ret = bic_ipmb_send(slot_id, NETFN_APP_REQ, CMD_APP_MASTER_WRITE_READ, tbuf, tlen, rbuf, &rlen, intf);
    if ( ret < 0 ) {
      syslog(LOG_WARNING, "%s():%d Failed to send command code to get vr ver. ret=%d", __func__,__LINE__, ret);
      goto error_exit;
    }

    tbuf[2] = 0x2; //read cnt
    tbuf[3] = 0x42; //command code
    tlen = 4;
    ret = bic_ipmb_send(slot_id, NETFN_APP_REQ, CMD_APP_MASTER_WRITE_READ, tbuf, tlen, rbuf, &rlen, intf);
    if ( ret < 0 ) {
      syslog(LOG_WARNING, "%s():%d Failed to send command code to get vr ver. ret=%d", __func__,__LINE__, ret);
      goto error_exit;
    }

    tbuf[2] = 0x2; //read cnt
    tbuf[3] = 0x43; //command code
    tlen = 4;
    ret = bic_ipmb_send(slot_id, NETFN_APP_REQ, CMD_APP_MASTER_WRITE_READ, tbuf, tlen, &rbuf[2], &rlen, intf);
    if ( ret < 0 ) {
      syslog(LOG_WARNING, "%s():%d Failed to send command code to get vr ver. ret=%d", __func__,__LINE__, ret);
      goto error_exit;
    }
    snprintf(ver_str, MAX_VALUE_LEN, "Infineon %02X%02X%02X%02X, Remaining Writes: %d", rbuf[3], rbuf[2], rbuf[1], rbuf[0], remaining_writes);
    kv_set(key, ver_str, 0, 0);

  error_exit:
    rc = flock(fd, LOCK_UN);
    if (rc == -1) {
      syslog(LOG_WARNING, "%s: failed to unflock on %s", __func__, path);
      close(fd);
      return rc;
    }
    close(fd);
    remove(path);
    return ret;

  } else if ( rlen > 4 ) {
    //TI
    tbuf[2] = 0x02; //read cnt
    tbuf[3] = 0xF0; //command code
    tlen = 4;
    ret = bic_ipmb_send(slot_id, NETFN_APP_REQ, CMD_APP_MASTER_WRITE_READ, tbuf, tlen, rbuf, &rlen, intf);
    if ( ret < 0 ) {
      syslog(LOG_WARNING, "%s():%d Failed to send command code to get vr ver. ret=%d", __func__,__LINE__, ret);
      return ret;
    }

    snprintf(ver_str, MAX_VALUE_LEN, "Texas Instruments %02X%02X", rbuf[1], rbuf[0]);
    kv_set(key, ver_str, 0, 0);
  } else {
    //ISL
    //get the reamaining write
    ret = bic_get_isl_vr_remaining_writes(slot_id, bus, addr, &remaining_writes, intf);
    if ( ret < 0 ) {
      syslog(LOG_WARNING, "%s():%d Failed to send command code to get vr remaining writes. ret=%d", __func__,__LINE__, ret);
      goto error_exit;
    }

    //get the CRC32 of the VR
    tbuf[2] = 0x00; //read cnt
    tbuf[3] = 0xC7; //command code
    tbuf[4] = 0x3F; //reg
    tbuf[5] = 0x00; //dummy data
    tlen = 6;
    ret = bic_ipmb_send(slot_id, NETFN_APP_REQ, CMD_APP_MASTER_WRITE_READ, tbuf, tlen, rbuf, &rlen, intf);
    if ( ret < 0 ) {
      syslog(LOG_WARNING, "%s():%d Failed to send command code to get vr ver. ret=%d", __func__,__LINE__, ret);
      return ret;
    }

    tbuf[2] = 0x04; //read cnt
    tbuf[3] = 0xC5; //command code
    tlen = 4;
    ret = bic_ipmb_send(slot_id, NETFN_APP_REQ, CMD_APP_MASTER_WRITE_READ, tbuf, tlen, rbuf, &rlen, intf);
    if ( ret < 0 ) {
      syslog(LOG_WARNING, "%s():%d Failed to send command code to get vr ver. ret=%d", __func__,__LINE__, ret);
      return ret;
    }

    snprintf(ver_str, MAX_VALUE_LEN, "Renesas %02X%02X%02X%02X, Remaining Writes: %d", rbuf[3], rbuf[2], rbuf[1], rbuf[0], remaining_writes);
    kv_set(key, ver_str, 0, 0);
  }

  return ret;
}

int
bic_get_vr_ver_cache(uint8_t slot_id, uint8_t intf, uint8_t bus, uint8_t addr, char *ver_str) {
  char key[MAX_KEY_LEN], tmp_str[MAX_VALUE_LEN] = {0};
  bool is_sb_ast1030 = false;

  snprintf(key, sizeof(key), "slot%x_vr_%02xh_crc", slot_id, addr);
  if (kv_get(key, tmp_str, NULL, 0)) {
    if (get_sb_bic_solution(slot_id, &is_sb_ast1030) < 0) {
      return -1;
    }

    if (is_sb_ast1030 == true) {
      if (ast_bic_get_vr_ver(slot_id, intf, addr, key, tmp_str)) {
        return -1;
      }
    } else {
      if (bic_get_vr_ver(slot_id, intf, bus, addr, key, tmp_str)) {
        return -1;
      }
    }
  }

  if (snprintf(ver_str, MAX_VER_STR_LEN, "%s", tmp_str) > (MAX_VER_STR_LEN-1)) {
    return -1;
  }

  return 0;
}

int
bic_get_exp_cpld_ver(uint8_t slot_id, uint8_t comp __attribute__((unused)), uint8_t *ver, uint8_t bus, uint8_t addr, uint8_t intf) {
  uint8_t tbuf[32] = {0};
  uint8_t rbuf[4] = {0};
  uint8_t tlen = 0;
  uint8_t rlen = 0;
  int ret = 0;

  //mux
  tbuf[0] = (bus << 1) + 1; //bus
  tbuf[1] = 0xE2; //mux addr
  tbuf[2] = 0x00;
  tbuf[3] = 0x02;
  tlen = 4;

  ret = bic_ipmb_send(slot_id, NETFN_APP_REQ, CMD_APP_MASTER_WRITE_READ, tbuf, tlen, rbuf, &rlen, intf);
  if ( ret < 0 ) {
    syslog(LOG_WARNING, "%s() Failed to send the command to switch the mux. ret=%d", __func__, ret);
    goto error_exit;
  }

  //read cpld
  tbuf[1] = addr;
  tbuf[2] = 0x04; //read cnt
  tbuf[3] = 0xC0; //data 1
  tbuf[4] = 0x00; //data 2
  tbuf[5] = 0x00; //data 3
  tbuf[6] = 0x00; //data 4
  tlen = 7;

  ret = bic_ipmb_send(slot_id, NETFN_APP_REQ, CMD_APP_MASTER_WRITE_READ, tbuf, tlen, ver, &rlen, intf);
  if ( ret < 0 ) {
    syslog(LOG_WARNING, "%s() Failed to send the command code to get cpld ver. ret=%d", __func__, ret);
  }

error_exit:
  return ret;
}

int
bic_is_m2_exp_prsnt(uint8_t slot_id) {
  uint8_t tbuf[4] = {0};
  uint8_t rbuf[MAX_IPMB_RES_LEN] = {0};
  uint8_t tlen = 4;
  uint8_t rlen = 0;
  int ret = 0;
  int present = 0;
  char key[MAX_KEY_LEN] = {0};
  char tmp_str[MAX_VALUE_LEN] = {0};
  int val = 0;

  snprintf(key, sizeof(key), KV_SLOT_IS_M2_EXP_PRESENT, slot_id);

  if (kv_get(key, tmp_str, NULL, 0)) {
    // get form bic
    tbuf[0] = 0x05; //bus id
    tbuf[1] = 0x42; //slave addr
    tbuf[2] = 0x01; //read 1 byte
    tbuf[3] = 0x0D; //register offset

    ret = bic_ipmb_wrapper(slot_id, NETFN_APP_REQ, CMD_APP_MASTER_WRITE_READ, tbuf, tlen, rbuf, &rlen);

    present = rbuf[0] & 0xC;

    if ( ret < 0 ) {
      syslog(LOG_WARNING, "%s() slot%d Failed to get expansion present status. ret=%d", __func__, slot_id, ret);
      return ret;
    } else {
      if ( present == 0) {
        val = 3; //1OU+2OU present
      } else if ( present == 8) {
        val = 1; //1OU present
      } else if ( present == 4) {
        val = 2; //2OU present
      }
    }

    snprintf(tmp_str, sizeof(tmp_str), "%d", val);
    kv_set(key, tmp_str, 0, 0);
    return val;

  } else {
    // get from cache
    val = atoi(tmp_str);
    return val;
  }
}

int
bic_is_m2_exp_prsnt_cache(uint8_t slot_id) {
  char key[MAX_KEY_LEN] = {0};
  char tmp_str[MAX_VALUE_LEN] = {0};
  int val = 0;

  snprintf(key, sizeof(key), KV_SLOT_IS_M2_EXP_PRESENT, slot_id);
  if (kv_get(key, tmp_str, NULL, 0))
    return -1;

  val = atoi(tmp_str);
  return val;
}

int
bic_is_2u_top_bot_prsnt(uint8_t slot_id) {
#define REG_CWC_CPLD_GPV3_PRSNT 0x04
  char key[MAX_KEY_LEN] = {0};
  char tmp_str[MAX_VALUE_LEN] = {0};
  int val = 0;

  snprintf(key, sizeof(key), KV_SLOT_IS_2U_TOPBOT_PRESENT, slot_id);
  
  if (kv_get(key, tmp_str, NULL, 0)) {
    int i2cfd = BIC_STATUS_FAILURE;
    int ret = BIC_STATUS_FAILURE;
    uint8_t tbuf = 0x0;
    uint8_t rbuf = 0x0;
    uint8_t tlen = 0x0;
    uint8_t rlen = 0x0;
    uint8_t retry = MAX_READ_RETRY;

    i2cfd = i2c_cdev_slave_open(FRU_SLOT1 + SLOT_BUS_BASE, CWC_CPLD_ADDRESS >> 1, I2C_SLAVE_FORCE_CLAIM);
    if ( i2cfd < 0 ) {
      syslog(LOG_WARNING, "%s() Failed to open i2c device %d", __func__, CWC_CPLD_ADDRESS);
      return i2cfd;
    }

    tbuf = REG_CWC_CPLD_GPV3_PRSNT;
    tlen = 1;
    rlen = 1;
    do {
      ret = i2c_rdwr_msg_transfer(i2cfd, CWC_CPLD_ADDRESS, &tbuf, tlen, &rbuf, rlen);
      if ( ret < 0 ) {
        msleep(100);
      } else {
        break;
      }
    } while( retry-- > 0 );

    if ( ret < 0 ) {
      syslog(LOG_WARNING, "%s() Failed to do i2c_rdwr_msg_transfer, tlen=%d", __func__, tlen);
    } else {
      if ( (rbuf & 0x03) == 0 ) {
        val |= PRESENT_2U_TOP;
      }
      if ( (rbuf & 0x0C) == 0 ) {
        val |= PRESENT_2U_BOT;
      }
    }

    if ( i2cfd > 0 ) {
      close(i2cfd);
    }

    snprintf(tmp_str, sizeof(tmp_str), "%d", val);
    kv_set(key, tmp_str, 0, 0);
    return val;
  } else {
    // get from cache
    val = atoi(tmp_str);
    return val;
  }
}

int
bic_is_2u_top_bot_prsnt_cache(uint8_t slot_id) {
  char key[MAX_KEY_LEN] = {0};
  char tmp_str[MAX_VALUE_LEN] = {0};
  int val = 0;

  snprintf(key, sizeof(key), KV_SLOT_IS_2U_TOPBOT_PRESENT, slot_id);
  if (kv_get(key, tmp_str, NULL, 0))
    return -1;

  val = atoi(tmp_str);
  return val;
}

/*
    0x2E 0xDF: Force Intel ME Recovery
Request
  Byte 1:3 = Intel Manufacturer ID - 000157h, LS byte first.

  Byte 4 - Command
    = 01h Restart using Recovery Firmware
      (Intel ME FW configuration is not restored to factory defaults)
    = 02h Restore Factory Default Variable values and restart the Intel ME FW
    = 03h PTT Initial State Restore
Response
  Byte 1 - Completion Code
  = 00h - Success
  (Remaining standard Completion Codes are shown in Section 2.12)
  = 81h - Unsupported Command parameter value in the Byte 4 of the request.

  Byte 2:4 = Intel Manufacturer ID - 000157h, LS byte first.
*/
int
me_recovery(uint8_t slot_id, uint8_t command) {
  uint8_t tbuf[256] = {0x00};
  uint8_t rbuf[256] = {0x00};
  uint8_t tlen = 0;
  uint8_t rlen = 0;
  int ret = 0;
  int retry = 0;

  while (retry <= RETRY_3_TIME) {
    tbuf[0] = 0xB8;
    tbuf[1] = 0xDF;
    tbuf[2] = 0x57;
    tbuf[3] = 0x01;
    tbuf[4] = 0x00;
    tbuf[5] = command;
    tlen = 6;

    ret = bic_me_xmit(slot_id, tbuf, tlen, rbuf, &rlen);
    if (ret) {
      retry++;
      sleep(1);
      continue;
    }
    else
      break;
  }
  if (retry > RETRY_3_TIME) { //if the third retry still failed, return -1
    syslog(LOG_CRIT, "%s: Restart using Recovery Firmware failed..., retried: %d", __func__,  retry);
    return -1;
  }

  sleep(10);
  retry = 0;
  memset(&tbuf, 0, 256);
  memset(&rbuf, 0, 256);
  /*
      0x6 0x4: Get Self-Test Results
    Byte 1 - Completion Code
    Byte 2
      = 55h - No error. All Self-Tests Passed.
      = 81h - Firmware entered Recovery bootloader mode
    Byte 3 For byte 2 = 55h, 56h, FFh:
      =00h
      =02h - recovery mode entered by IPMI command "Force ME Recovery"
  */
  //Using ME self-test result to check if the ME Recovery Command Success or not
  while (retry <= RETRY_3_TIME) {
    tbuf[0] = 0x18;
    tbuf[1] = 0x04;
    tlen = 2;
    ret = bic_me_xmit(slot_id, tbuf, tlen, rbuf, &rlen);
    if (ret) {
      retry++;
      sleep(1);
      continue;
    }

    //if Get Self-Test Results is 0x55 0x00, means No error. All Self-Tests Passed.
    //if Get Self-Test Results is 0x81 0x02, means Firmware entered Recovery bootloader mode
    if ( (command == RECOVERY_MODE) && (rbuf[1] == 0x81) && (rbuf[2] == 0x02) ) {
      return 0;
    }
    else if ( (command == RESTORE_FACTORY_DEFAULT) && (rbuf[1] == 0x55) && (rbuf[2] == 0x00) ) {
      return 0;
    }
    else {
      return -1;
    }
  }
  if (retry > RETRY_3_TIME) { //if the third retry still failed, return -1
    syslog(LOG_CRIT, "%s: Restore Factory Default failed..., retried: %d", __func__,  retry);
    return -1;
  }
  return 0;
}

int
me_reset(uint8_t slot_id) {
  uint8_t tbuf[2] = {0x00};
  uint8_t rbuf[2] = {0x00};
  uint8_t tlen = 0;
  uint8_t rlen = 0;
  int ret = 0;
  int retry = 0;

  while (retry <= RETRY_3_TIME) {
    tbuf[0] = 0x18;
    tbuf[1] = 0x02;
    tlen = 2;
    ret = bic_me_xmit(slot_id, tbuf, tlen, rbuf, &rlen);
    if (ret) {
      retry++;
      sleep(1);
      continue;
    }
    break;
  }
  if (retry > RETRY_3_TIME) { //if the third retry still failed, return -1
    syslog(LOG_CRIT, "%s: ME Reset failed..., retried: %d", __func__,  retry);
    return -1;
  }
  return 0;
}

int
bic_switch_mux_for_bios_spi(uint8_t slot_id, uint8_t mux) {
  uint8_t tbuf[5] = {0};
  uint8_t rbuf[1] = {0};
  uint8_t tlen = 5;
  uint8_t rlen = 0;
  int ret = 0;

  tbuf[0] = 0x05; //bus id
  tbuf[1] = 0x42; //slave addr
  tbuf[2] = 0x01; //read 1 byte
  tbuf[3] = 0x00; //register offset
  tbuf[4] = mux;  //mux setting

  ret = bic_ipmb_wrapper(slot_id, NETFN_APP_REQ, CMD_APP_MASTER_WRITE_READ, tbuf, tlen, rbuf, &rlen);

  if ( ret < 0 ) {
    return -1;
  }

  return 0;
}

int
bic_asd_init(uint8_t slot_id, uint8_t cmd) {
  uint8_t tbuf[4] = {0x00};
  uint8_t rbuf[8] = {0x00};
  uint8_t tlen = 4;
  uint8_t rlen = 0;

  memcpy(tbuf, (uint8_t *)&IANA_ID, 3);
  tbuf[3] = cmd;
  return bic_ipmb_wrapper(slot_id, NETFN_OEM_1S_REQ, CMD_OEM_1S_ASD_INIT, tbuf, tlen, rbuf, &rlen);
}

int
bic_get_gpio_config(uint8_t slot_id, uint8_t gpio, uint8_t *data) {
  uint8_t tbuf[13] = {0x00};
  uint8_t rbuf[6] = {0x00};
  uint8_t rlen = 0;
  uint8_t tlen = sizeof(tbuf);
  uint8_t index = 0;
  uint8_t pin = 0;
  int ret = 0;

  // File the IANA ID
  memcpy(tbuf, (uint8_t *)&IANA_ID, 3);

  //get the buffer index
  index = (gpio / 8) + 3; //3 is the size of IANA ID
  pin = 1 << (gpio % 8);
  tbuf[index] = pin;

  ret = bic_ipmb_wrapper(slot_id, NETFN_OEM_1S_REQ, CMD_OEM_1S_GET_GPIO_CONFIG, tbuf, tlen, rbuf, &rlen);
  *data = rbuf[3];
  return ret;
}

int
remote_bic_get_gpio_config(uint8_t slot_id, uint8_t gpio, uint8_t *data, uint8_t intf) {
  uint8_t tbuf[16] = {0x00};
  uint8_t rbuf[6] = {0x00};
  uint8_t rlen = 0;
  uint8_t tlen = sizeof(tbuf);
  uint8_t index = 0;
  uint8_t pin = 0;
  int ret = 0;

  // File the IANA ID
  memcpy(tbuf, (uint8_t *)&IANA_ID, 3);

  //get the buffer index
  index = (gpio / 8) + 3; //3 is the size of IANA ID
  pin = 1 << (gpio % 8);
  tbuf[index] = pin;
  
  ret = bic_ipmb_send(slot_id, NETFN_OEM_1S_REQ, CMD_OEM_1S_GET_GPIO_CONFIG, tbuf, tlen, rbuf, &rlen, intf);
  *data = rbuf[3];
  return ret;
}

int
bic_set_gpio_config(uint8_t slot_id, uint8_t gpio, uint8_t data) {
  uint8_t tbuf[15] = {0x00};
  uint8_t rbuf[6] = {0x00};
  uint8_t rlen = 0;
  uint8_t tlen = sizeof(tbuf);
  uint8_t index = 0;
  uint8_t pin = 0;
  int ret = 0;

  // File the IANA ID
  memcpy(tbuf, (uint8_t *)&IANA_ID, 3);

  //get the buffer index
  index = (gpio / 8) + 3; //3 is the size of IANA ID
  pin = 1 << (gpio % 8);
  tbuf[index] = pin;
  tbuf[14] = data & 0x1f;

  ret = bic_ipmb_wrapper(slot_id, NETFN_OEM_1S_REQ, CMD_OEM_1S_SET_GPIO_CONFIG, tbuf, tlen, rbuf, &rlen);
  return ret;
}

int
remote_bic_set_gpio_config(uint8_t slot_id, uint8_t gpio, uint8_t data, uint8_t intf) {
  uint8_t tbuf[17] = {0x00};
  uint8_t rbuf[6] = {0x00};
  uint8_t rlen = 0;
  uint8_t tlen = sizeof(tbuf);
  uint8_t index = 0;
  uint8_t pin = 0;
  int ret = 0;

  // File the IANA ID
  memcpy(tbuf, (uint8_t *)&IANA_ID, 3);

  //get the buffer index
  index = (gpio / 8) + 3; //3 is the size of IANA ID
  pin = 1 << (gpio % 8);
  tbuf[index] = pin;
  tbuf[16] = data & 0x1f;

  ret = bic_ipmb_send(slot_id, NETFN_OEM_1S_REQ, CMD_OEM_1S_SET_GPIO_CONFIG, tbuf, tlen, rbuf, &rlen, intf);
  return ret;
}

int
bic_set_gpio(uint8_t slot_id, uint8_t gpio_num, uint8_t value) {
  uint8_t tbuf[6] = {0x9c, 0x9c, 0x00};
  uint8_t rbuf[1] = {0};
  uint8_t tlen = 6;
  uint8_t rlen = 0;
  int ret = 0;

  tbuf[3] = 0x01;
  tbuf[4] = gpio_num;
  tbuf[5] = value;

  ret = bic_ipmb_wrapper(slot_id, NETFN_OEM_1S_REQ, BIC_CMD_OEM_GET_SET_GPIO, tbuf, tlen, rbuf, &rlen);

  if ( ret < 0 ) {
    return -1;
  }

  return 0;
}

int
remote_bic_set_gpio(uint8_t slot_id, uint8_t gpio_num, uint8_t value, uint8_t intf) {
  uint8_t tbuf[6] = {0x9c, 0x9c, 0x00};
  uint8_t rbuf[1] = {0};
  uint8_t tlen = 6;
  uint8_t rlen = 0;
  int ret = 0;

  tbuf[3] = 0x01;
  tbuf[4] = gpio_num;
  tbuf[5] = value;

  ret = bic_ipmb_send(slot_id, NETFN_OEM_1S_REQ, BIC_CMD_OEM_GET_SET_GPIO, tbuf, tlen, rbuf, &rlen, intf);

  if ( ret < 0 ) {
    return -1;
  }

  return 0;
}

// Get all GPIO pin status
int
bic_get_gpio(uint8_t slot_id, bic_gpio_t *gpio, uint8_t intf) {
  uint8_t tbuf[4] = {0x9c, 0x9c, 0x00}; // IANA ID
  uint8_t rbuf[13] = {0x00};
  uint8_t rlen = 0;
  int ret;

  memset(gpio, 0, sizeof(*gpio));

  ret = bic_ipmb_send(slot_id, NETFN_OEM_1S_REQ, CMD_OEM_1S_GET_GPIO, tbuf, 3, rbuf, &rlen, intf);
  if (ret != 0 || rlen < 3)
    return -1;

  rlen -= 3;
  if (rlen > sizeof(bic_gpio_t))
    rlen = sizeof(bic_gpio_t);

  // Ignore first 3 bytes of IANA ID
  memcpy((uint8_t*) gpio, &rbuf[3], rlen);

 #ifdef ENABLE_INJECTION
  // After getting the real values, check for injected vals
  uint32_t i, bmask;
  int fd, flen;
  char fpath[30], fdata;
  for (i = 0; i < MAX_GPIO_PINS; i++) {
    sprintf(fpath, GPIO_FALSE_DIR, slot_id, i);
    if (!access(fpath, F_OK)) {
      fd = open(fpath, O_RDONLY);
      if (fd == -1) {
        syslog(LOG_WARNING, "Could not open bic injection file for gpio %d", i);
        return -1;
      }

      flen = read(fd, &fdata, 1);
      if (flen == 0) {
        close(fd);
        return -1;
      }

      int fret = close(fd);
      if (fret) {
        syslog(LOG_WARNING, "Failed to close bic injection file for gpio %d", i);
        return -1;
      }

      // First set the bit to 0
      bmask = 1 << (i % 32);
      gpio->gpio[i/32] &= ~bmask;

      // Now we can change it to whatever we want
      gpio->gpio[i/32] |= (((fdata - 0x30) & 0x1) << (i % 32));
      syslog(LOG_INFO, "Injected %d into gpio %d at slot %d", (fdata - 0x30), i, slot_id);
    }
  }
#endif // ENABLE_INJECITON

  return ret;
}

// Get an GPIO pin status
int
bic_get_one_gpio_status(uint8_t slot_id, uint8_t gpio_num, uint8_t *value){
  uint8_t tbuf[5] = {0x00};
  uint8_t rbuf[5] = {0x00};
  uint8_t tlen = 5;
  uint8_t rlen = 0;
  int ret = 0;

  // File the IANA ID
  memcpy(tbuf, (uint8_t *)&IANA_ID, 3);
  tbuf[3] = 0x00;
  tbuf[4] = gpio_num;
  ret = bic_ipmb_wrapper(slot_id, NETFN_OEM_1S_REQ, BIC_CMD_OEM_GET_SET_GPIO, tbuf, tlen, rbuf, &rlen);
  *value = rbuf[4] & 0x01;
  return ret;
}

int
bic_get_sys_guid(uint8_t slot_id, uint8_t *guid) {
  int ret;
  uint8_t rlen = 0;

  ret = bic_ipmb_wrapper(slot_id, NETFN_APP_REQ, CMD_APP_GET_SYSTEM_GUID, NULL, 0, guid, &rlen);
  if (rlen != SIZE_SYS_GUID) {
#ifdef DEBUG
    syslog(LOG_ERR, "bic_get_sys_guid: returned rlen of %d\n");
#endif
    return -1;
  }

  return ret;
}

int
bic_set_sys_guid(uint8_t slot_id, uint8_t *guid) {
  int ret;
  uint8_t rlen = 0;
  uint8_t rbuf[MAX_IPMB_RES_LEN]={0x00};

  ret = bic_ipmb_wrapper(slot_id, NETFN_OEM_REQ, CMD_OEM_SET_SYSTEM_GUID, guid, SIZE_SYS_GUID, rbuf, &rlen);

  return ret;
}

int
bic_do_sled_cycle(uint8_t slot_id) {
  uint8_t tbuf[4] = {0};
  uint8_t tlen = 4;

  tbuf[0] = 0x05; //bus id
  tbuf[1] = 0x80; //slave addr
  tbuf[2] = 0x00; //read 0 byte
  tbuf[3] = 0xd9; //register offset
  tlen = 4;
  return bic_ipmb_send(slot_id, NETFN_APP_REQ, CMD_APP_MASTER_WRITE_READ, tbuf, tlen, NULL, 0, BB_BIC_INTF);
}

// Only For Class 2
int
bic_set_fan_auto_mode(uint8_t crtl, uint8_t *status) {
  uint8_t tbuf[1] = {0};
  uint8_t rbuf[1] = {0};
  uint8_t tlen = 1;
  uint8_t rlen = 0;
  int ret = 0;
  int retry = 0;

  tbuf[0] = crtl;

  while (retry < 3) {
    ret = bic_ipmb_send(FRU_SLOT1, NETFN_OEM_REQ, BIC_CMD_OEM_FAN_CTRL_STAT, tbuf, tlen, rbuf, &rlen, BB_BIC_INTF);
    if (ret == 0) break;
    retry++;
  }
  if (ret != 0) {
    return -1;
  }

  *status = rbuf[0];

  return 0;
}

// Only For Class 2
int
bic_set_fan_speed(uint8_t fan_id, uint8_t pwm) {
  uint8_t tbuf[5] = {0x9c, 0x9c, 0x00};
  uint8_t rbuf[1] = {0};
  uint8_t tlen = 5;
  uint8_t rlen = 0;
  int ret = 0;
  int retry = 0;

  tbuf[3] = fan_id;
  tbuf[4] = pwm;

  while (retry < 3) {
    ret = bic_ipmb_send(FRU_SLOT1, NETFN_OEM_1S_REQ, BIC_CMD_OEM_BMC_FAN_CTRL, tbuf, tlen, rbuf, &rlen, BB_BIC_INTF);
    if ( ret == 0 ) break;
    retry++;
  }
  if (ret != 0) {
    return -1;
  }

  return 0;
}

// Only For Class 2
int
bic_manual_set_fan_speed(uint8_t fan_id, uint8_t pwm) {
  uint8_t tbuf[2] = {0};
  uint8_t rbuf[1] = {0};
  uint8_t tlen = 2;
  uint8_t rlen = 0;
  int ret = 0;
  int retry = 0;

  tbuf[0] = fan_id;
  tbuf[1] = pwm;

  while (retry < 3) {
    ret = bic_ipmb_send(FRU_SLOT1, NETFN_OEM_REQ, BIC_CMD_OEM_SET_FAN_DUTY, tbuf, tlen, rbuf, &rlen, BB_BIC_INTF);
    if (ret == 0) break;
    retry++;
  }
  if (ret != 0) {
    return -1;
  }

  return 0;
}

// Only For Class 2
int
bic_get_fan_speed(uint8_t fan_id, float *value) {
  uint8_t tbuf[4] = {0x9c, 0x9c, 0x00};
  uint8_t rbuf[5] = {0};
  uint8_t tlen = 4;
  uint8_t rlen = 0;
  int ret = 0;
  int retry = 0;

  tbuf[3] = fan_id;

  while (retry < 3) {
    ret = bic_ipmb_send(FRU_SLOT1, NETFN_OEM_1S_REQ, BIC_CMD_OEM_GET_FAN_RPM, tbuf, tlen, rbuf, &rlen, BB_BIC_INTF);
    if (ret == 0) break;
    retry++;
  }
  if (ret != 0) {
    return -1;
  }

  *value = (float)((rbuf[3] << 8)+ rbuf[4]);

  return 0;
}

// Only For Class 2
int
bic_get_fan_pwm(uint8_t fan_id, float *value) {
  uint8_t tbuf[4] = {0x9c, 0x9c, 0x00};
  uint8_t rbuf[4] = {0};
  uint8_t tlen = 4;
  uint8_t rlen = 0;
  int ret = 0;
  int retry = 0;

  tbuf[3] = fan_id;

  while (retry < 3) {
    ret = bic_ipmb_send(FRU_SLOT1, NETFN_OEM_1S_REQ, BIC_CMD_OEM_GET_FAN_DUTY, tbuf, tlen, rbuf, &rlen, BB_BIC_INTF);
    if (ret == 0) break;
    retry++;
  }
  if (ret != 0) {
    return -1;
  }

  *value = (float)rbuf[3];

  return 0;
}

// Only for class 2 system to get the power lock bit
int
bic_set_crit_act_flag(uint8_t dir_type) {
  int ret = BIC_STATUS_SUCCESS;
  uint8_t tbuf[5] = {CPLD_BB_BUS, CPLD_FLAG_REG_ADDR, 0x0/*read cnt*/, 0x10/*base reg*/, 0x0/*data*/};
  uint8_t rbuf[5] = {0};
  uint8_t tlen = 5;
  uint8_t rlen = 0;
  uint8_t mb_index = 0;

  ret = bic_get_mb_index(&mb_index);
  if (ret < 0) {
    printf("Failed to get MB index\n");
    return ret;
  }

  // set the register for the other side of BMC
  tbuf[3] += (mb_index == FRU_SLOT1)?FRU_SLOT3:FRU_SLOT1;

  // the default is high, so 0x00 means it's asserted
  tbuf[4] = (dir_type == SEL_ASSERT)?0x00:0x01;

  ret = bic_ipmb_send(FRU_SLOT1, NETFN_APP_REQ, CMD_APP_MASTER_WRITE_READ, tbuf, tlen, rbuf, &rlen, BB_BIC_INTF);
  if ( ret < 0 ) {
    printf("Failed to set power lock flag\n");
  }

  return ret;
}

int
bic_get_dev_info(uint8_t slot_id, uint8_t dev_id, uint8_t *nvme_ready, uint8_t *status, uint8_t *type) {
  int ret = 0;
  uint8_t retry = MAX_READ_RETRY;
  uint16_t reversed_vender_sph = 0;
  uint8_t type_2ou = UNKNOWN_BOARD;
  M2_DEV_INFO m2_dev_info = {0};

  if (slot_id == FRU_2U_TOP || slot_id == FRU_2U_BOT) {
    type_2ou = CWC_MCHP_BOARD;
  } else {
    ret = bic_is_m2_exp_prsnt(slot_id);
    if (ret < 0) {
      syslog(LOG_WARNING,"%s() Failed to get 1ou & 2ou present status", __func__);
      goto err_out;
    }
    if ( (ret & PRESENT_2OU) != PRESENT_2OU ) {
      syslog(LOG_WARNING,"%s() Cannot get 2ou board", __func__);
      goto err_out;
    }
    ret = fby3_common_get_2ou_board_type(slot_id, &type_2ou);
    if ( ret < 0 ) {
      syslog(LOG_WARNING,"%s() Cannot get 2ou board type", __func__);
      goto err_out;
    }
  }
  if ( type_2ou == GPV3_MCHP_BOARD || type_2ou == GPV3_BRCM_BOARD || type_2ou == CWC_MCHP_BOARD ) {

    while (retry) {
      if (slot_id == FRU_2U_TOP) {
        ret = bic_get_dev_power_status(FRU_SLOT1, dev_id, &m2_dev_info, RREXP_BIC_INTF1);
      } else if (slot_id == FRU_2U_BOT) {
        ret = bic_get_dev_power_status(FRU_SLOT1, dev_id, &m2_dev_info, RREXP_BIC_INTF2);
      } else {
        ret = bic_get_dev_power_status(slot_id, dev_id, &m2_dev_info, REXP_BIC_INTF);
      }
      if (!ret)
        break;
      msleep(50);
      retry--;
    }

    *nvme_ready = m2_dev_info.nvme_ready;
    *status = m2_dev_info.status;

    //syslog(LOG_WARNING, "bic_get_dev_power_status: dev_id %d nvme_ready %d status %d",dev_id, *nvme_ready, *status);
    reversed_vender_sph = (((VENDOR_SPH << 8) & 0xFF00) | ((VENDOR_SPH >> 8) & 0x00FF));

    if (*nvme_ready) {
      if ( m2_dev_info.meff == MEFF_DUAL_M2 ) {
        *type = DEV_TYPE_DUAL_M2;
      } else{
        if (m2_dev_info.ffi == FFI_ACCELERATOR) {
          if (m2_dev_info.vendor_id == VENDOR_BRCM) {
            *type = DEV_TYPE_BRCM_ACC;
          } else if (m2_dev_info.vendor_id == VENDOR_SPH || m2_dev_info.vendor_id == reversed_vender_sph) {
            *type = DEV_TYPE_SPH_ACC;
          } else {
            *type = DEV_TYPE_M2;
          }
        } else {
          *type = DEV_TYPE_SSD;
        }
      }
    } else {
      *type = DEV_TYPE_UNKNOWN;
    }
    return 0;
  }

err_out:
  *type = DEV_TYPE_UNKNOWN;
  return -1;
}

int
bic_get_dev_power_status(uint8_t slot_id, uint8_t dev_id, M2_DEV_INFO *m2_dev_info, uint8_t intf) {
  uint8_t tbuf[5] = {0x9c, 0x9c, 0x00}; // IANA ID
  uint8_t rbuf[MAX_IPMB_RES_LEN] = {0x00};
  uint8_t tlen = 5;
  uint8_t rlen = 0;
  int ret = 0;
  uint8_t table = 0, board_type = 0;

  if (intf == FEXP_BIC_INTF) {
    table = 1;
    ret = bic_get_1ou_type(slot_id, &board_type);
    if (ret < 0) {
      syslog(LOG_ERR, "%s() Cannot get 1ou board_type", __func__);
      board_type = M2_BOARD;
    }
  } else if (intf == REXP_BIC_INTF) {
    table = 0;
    ret = fby3_common_get_2ou_board_type(slot_id, &board_type);
    if (ret < 0) {
      syslog(LOG_ERR, "%s() Cannot get 2ou board_type", __func__);
      board_type = M2_BOARD;
    }
  } else if (intf == RREXP_BIC_INTF1 || intf == RREXP_BIC_INTF2) {
    board_type = M2_BOARD;
  } else {
    return -1;
  }

  //Send the command
  memcpy(tbuf, (uint8_t *)&IANA_ID, 3);
  if (board_type == EDSFF_1U) {
    // case 1OU E1S
    tbuf[3] = mapping_e1s_pwr[table][dev_id - 1];
  } else if (board_type == E1S_BOARD) {
    // case 2OU E1S
    tbuf[3] = mapping_e1s_pwr[table][dev_id - 1] + 1; // device ID 1 based in power control
  } else {
    // case 1/2OU M.2
    if (dev_id >= DUAL_DEV_ID0_2OU && dev_id <= DUAL_DEV_ID5_2OU) {
      tbuf[3] = dev_id - DUAL_DEV_ID0_2OU + DUAL_DEV_BIC_ID0;
    } else {
      tbuf[3] = dev_id;
    }
  }

  tbuf[4] = 0x3;  //get power status
  ret = bic_ipmb_send(slot_id, NETFN_OEM_1S_REQ, CMD_OEM_1S_DEV_POWER, tbuf, tlen, rbuf, &rlen, intf);

  if (dev_id >= DUAL_DEV_ID0_2OU && dev_id <= DUAL_DEV_ID5_2OU) { //dual m.2 sitauation, it will return two byte.
    m2_dev_info->status = (rbuf[3] << 8) | rbuf[4];
  } else {
    // Ignore first 3 bytes of IANA ID
    m2_dev_info->status = rbuf[3];
    if (intf == REXP_BIC_INTF || intf == RREXP_BIC_INTF1 || intf == RREXP_BIC_INTF2) {
      m2_dev_info->nvme_ready = rbuf[4]; //1 is assigned directly. for REXP_EXP_INTF, we assige it from rbuf[4]
    } else {
      m2_dev_info->nvme_ready = 0x1;
    }
    
    if ( rlen == M2_DEV_FREYA_RLEN ) {
      m2_dev_info->is_freya = true;
    }

    m2_dev_info->ffi = rbuf[5];   // FFI_0 0:Storage 1:Accelerator
    m2_dev_info->meff = rbuf[6];  // MEFF  0x35: M.2 22110 0xF0: Dual M.2
    m2_dev_info->vendor_id = (rbuf[7] << 8 ) | rbuf[8]; // PCIe Vendor ID
    m2_dev_info->major_ver = rbuf[9];  //FW version Major Revision
    m2_dev_info->minor_ver = rbuf[10]; //FW version Minor Revision
    m2_dev_info->additional_ver = rbuf[11]; //Additional FW version
    m2_dev_info->sec_major_ver = rbuf[12]; //Secondary Major Revision
    m2_dev_info->sec_minor_ver = rbuf[13]; //Secondary Minor Revision
  }

  return ret;
}

int
bic_set_dev_power_status(uint8_t slot_id, uint8_t dev_id, uint8_t status, uint8_t intf) {
  uint8_t tbuf[5]  = {0x00};
  uint8_t rbuf[10] = {0x00};
  uint8_t tlen = 0;
  uint8_t rlen = 0;
  uint8_t bus_num = 0;
  uint8_t table = 0, board_type = 0;
  uint8_t prsnt_bit = 0;
  int fd = 0;
  int ret = 0;

  do {
    if (intf == FEXP_BIC_INTF) {
      table = 1;
      ret = bic_get_1ou_type(slot_id, &board_type);
      if (ret < 0) {
        syslog(LOG_ERR, "%s() Cannot get 1ou board_type", __func__);
        board_type = M2_BOARD;
      }
    } else if (intf == REXP_BIC_INTF) {
      table = 0;
      ret = fby3_common_get_2ou_board_type(slot_id, &board_type);
      if (ret < 0) {
        syslog(LOG_ERR, "%s() Cannot get 2ou board_type", __func__);
        board_type = M2_BOARD;
      }

      // No VPP and hotplug on GPv3, skip it
      if ( board_type == GPV3_MCHP_BOARD || board_type == GPV3_BRCM_BOARD ) break;
    } else if (intf == RREXP_BIC_INTF1 || intf == RREXP_BIC_INTF2) {
      board_type = M2_BOARD;
      break;
    } else {
      return -1;
    }

    bus_num = fby3_common_get_bus_id(slot_id) + 4;

    //set the present status of M.2
    fd = i2c_open(bus_num, CPLD_ADDRESS >> 1);
    if ( fd < 0 ) {
      printf("Cannot open /dev/i2c-%d\n", bus_num);
      ret = BIC_STATUS_FAILURE;
      goto error_exit;
    }

    tbuf[0] = (intf == REXP_BIC_INTF )?M2_REG_2OU:M2_REG_1OU;
    tlen = 1;
    rlen = 1;
    ret = i2c_rdwr_msg_transfer(fd, CPLD_ADDRESS, tbuf, tlen, rbuf, rlen);
    if ( ret < 0 ) {
      syslog(LOG_WARNING, "%s() Failed to do i2c_rdwr_msg_transfer, tlen=%d, ret", __func__, tlen);
      goto error_exit;
    }

    if (board_type == EDSFF_1U || board_type == E1S_BOARD) {
      // case 1/2OU E1S
      prsnt_bit = mapping_e1s_prsnt[table][dev_id - 1];
    } else {
      // case 1/2OU M.2
      prsnt_bit = mapping_m2_prsnt[table][dev_id - 1];
    }

    if ( status == M2_PWR_OFF ) {
      tbuf[1] = (rbuf[0] | (0x1 << (prsnt_bit)));
    } else {
      tbuf[1] = (rbuf[0] & ~(0x1 << (prsnt_bit)));
    }

    tlen = 2;
    rlen = 0;
    ret = i2c_rdwr_msg_transfer(fd, CPLD_ADDRESS, tbuf, tlen, rbuf, rlen);
    if ( ret < 0 ) {
      syslog(LOG_WARNING, "%s() Failed to do i2c_rdwr_msg_transfer, tlen=%d", __func__, tlen);
      goto error_exit;
    }
  } while(0);

  //Send the command
  memcpy(tbuf, (uint8_t *)&IANA_ID, 3);

  if (board_type == EDSFF_1U) {
    // case 1OU E1S
    tbuf[3] = mapping_e1s_pwr[table][dev_id - 1];
  } else if (board_type == E1S_BOARD) {
    // case 2OU E1S
    tbuf[3] = mapping_e1s_pwr[table][dev_id - 1] + 1; // device ID 1 based in power control
  } else {
    // case 1/2OU M.2
    if (dev_id >= DUAL_DEV_ID0_2OU && dev_id <= DUAL_DEV_ID5_2OU) {
      tbuf[3] = dev_id - DUAL_DEV_ID0_2OU + DUAL_DEV_BIC_ID0;
    } else {
      tbuf[3] = dev_id;
    }
  }

  tbuf[4] = status;  //set power status
  tlen = 5;

  ret = bic_ipmb_send(slot_id, NETFN_OEM_1S_REQ, CMD_OEM_1S_DEV_POWER, tbuf, tlen, rbuf, &rlen, intf);

error_exit:
  if ( fd > 0 ) close(fd);
  return ret;
}

int
bic_master_write_read(uint8_t slot_id, uint8_t bus, uint8_t addr, uint8_t *wbuf, uint8_t wcnt, uint8_t *rbuf, uint8_t rcnt) {
  uint8_t tbuf[256];
  uint8_t tlen = 3, rlen = 0;
  int ret;

  tbuf[0] = bus;
  tbuf[1] = addr;
  tbuf[2] = rcnt;
  if (wcnt) {
    memcpy(&tbuf[3], wbuf, wcnt);
    tlen += wcnt;
  }
  ret = bic_ipmb_wrapper(slot_id, NETFN_APP_REQ, CMD_APP_MASTER_WRITE_READ, tbuf, tlen, rbuf, &rlen);

  return ret;
}

int
bic_usb_hub_reset(uint8_t slot_id, uint8_t board_type, uint8_t intf) {
  uint8_t rst_list[] = {GPV3_GPIO_RST_USB_HUB1, GPV3_GPIO_RST_USB_HUB2, GPV3_GPIO_RST_USB_HUB3, CWC_GPIO_RST_USB_HUB, GPIO_RST_USB_HUB};
  size_t st_idx = 0;
  size_t end_idx = sizeof(rst_list);
  int ret = BIC_STATUS_FAILURE;

  // if intf is REXP_BIC_INTF and board_type is CWC_MCHP_BOARD
  //   rst seq: CWC HUB -> SB HUB
  // else if intf is REXP_BIC_INTF
  //   rst seq: GPV3 HUB1/2/3 -> SB HUB
  // else if intf is RREXP_BIC_INTF1 or RREXP_BIC_INTF2
  //   rst seq: GPV3 HUB1/2/3 -> CWC HUB -> SB HUB
  if ( intf == REXP_BIC_INTF ) {
    if ( board_type == CWC_MCHP_BOARD ) {
      st_idx = 3;
    } else {
      // 2U config doesnt contain CWC
      rst_list[3] = GPIO_RST_USB_HUB;
      end_idx--;
    }
  } else if ( intf == RREXP_BIC_INTF1 || intf == RREXP_BIC_INTF2 ) {
    // do nothing
  } else {
    syslog(LOG_WARNING, "intf(%02Xh) and board_type(%02Xh) are not supported\n", intf, board_type);
    return ret;
  }

  // reset
  printf("HUB reset...");
  for ( size_t i = 0; i < 2; i++ ) {
    for ( size_t j = st_idx; j < end_idx; j++ ) {
      ret = remote_bic_set_gpio(slot_id, rst_list[j], i, intf);
      if ( ret < 0 ) {
        printf("Failed to reset USB HUB, pin#:%02zX, val:%02zuX, intf:%02X, board_type:%02X\n", rst_list[i], i, intf, board_type);
        break;
      }
      if ( i == VALUE_HIGH ) msleep(500);
    }
  }
  printf("Done\n");

  return ret;
}

int
bic_reset(uint8_t slot_id, uint8_t intf) {
  uint8_t tbuf[3] = {0x9c, 0x9c, 0x00}; // IANA ID
  uint8_t rbuf[8] = {0x00};
  uint8_t rlen = 0;
  int ret = BIC_STATUS_FAILURE;

  ret = bic_ipmb_send(slot_id, NETFN_APP_REQ, CMD_APP_COLD_RESET, tbuf, 0, rbuf, &rlen, intf);
  return ret;
}

int
bic_clear_cmos(uint8_t slot_id) {
  uint8_t tbuf[3] = {0x9c, 0x9c, 0x00}; // IANA ID
  uint8_t rbuf[8] = {0x00};
  uint8_t rlen = 0;

  return bic_ipmb_wrapper(slot_id, NETFN_OEM_1S_REQ, CMD_OEM_1S_CLEAR_CMOS, tbuf, 3, rbuf, &rlen);
}

// Only For Class 2
int
bic_inform_sled_cycle(void) {
  uint8_t tbuf[3] = {0x9c, 0x9c, 0x00};
  uint8_t rbuf[1] = {0x00};
  uint8_t tlen = 3;
  uint8_t rlen = 0;
  int ret = 0;
  int retry = 0;

  while (retry < 3) {
    ret = bic_ipmb_send(FRU_SLOT1, NETFN_OEM_1S_REQ, BIC_CMD_OEM_INFORM_SLED_CYCLE, tbuf, tlen, rbuf, &rlen, BB_BIC_INTF);
    if (ret == 0) break;
    retry++;
  }
  if (ret != 0) {
    return -1;
  }

  return 0;
}

// For Discovery Point, get pcie config
int
bic_get_dp_pcie_config(uint8_t slot_id, uint8_t *pcie_config) {
  uint8_t tbuf[3] = {0x9c, 0x9c, 0x00};
  uint8_t rbuf[4] = {0x00};
  uint8_t tlen = 3;
  uint8_t rlen = 0;
  int ret = 0;
  int retry = 0;

  while (retry < 3) {
    ret = bic_ipmb_wrapper(slot_id, NETFN_OEM_1S_REQ, CMD_OEM_1S_GET_PCIE_CONFIG, tbuf, tlen, rbuf, &rlen);
    if (ret == 0) break;
    retry++;
  }
  if (ret != 0) {
    return -1;
  }

  (*pcie_config) = (rbuf[3] & 0x0f);
  return 0;
}

// For class 2 system, BMC need to get MB index from BB BIC
int
bic_get_mb_index(uint8_t *index) {
  GET_MB_INDEX_RESP resp = {0};
  uint8_t rlen = 0;
  uint8_t tbuf[MAX_IPMB_REQ_LEN] = {0};
  char key[MAX_KEY_LEN] = {0};
  char tmp_str[MAX_VALUE_LEN] = {0};

  snprintf(key, sizeof(key), KV_MB_INDEX);

  if (index == NULL) {
    syslog(LOG_WARNING, "%s(): invalid index parameter", __func__);
    return -1;
  }

  if (kv_get(key, tmp_str, NULL, 0)) {
    memset(tbuf, 0, sizeof(tbuf));
    memset(&resp, 0, sizeof(resp));
    if (bic_ipmb_send(FRU_SLOT1, NETFN_OEM_REQ, BIC_CMD_OEM_GET_MB_INDEX, tbuf, 0, (uint8_t*) &resp, &rlen, BB_BIC_INTF) < 0) {
      syslog(LOG_WARNING, "%s(): fail to get MB index", __func__);
      return -1;
    }
    if (rlen == sizeof(GET_MB_INDEX_RESP)) {
      *index = resp.index;
    } else {
      syslog(LOG_WARNING, "%s(): wrong response length (%d), while getting MB index, expected = %d", 
            __func__, rlen, sizeof(GET_MB_INDEX_RESP));
      return -1;
    }

    snprintf(tmp_str, sizeof(tmp_str), "%d", *index);
    kv_set(key, tmp_str, 0, 0);
  }
  else {
    // get from cache
    *index = atoi(tmp_str);
  }

  return 0;
}

// For class 2 system, bypass command to another slot BMC
int
bic_bypass_to_another_bmc(uint8_t* data, uint8_t len) {
  uint8_t tlen = 0;
  uint8_t rlen = 0;
  uint8_t rbuf[MAX_IPMB_RES_LEN] = {0};
  char iana_id[IANA_LEN] = {0x9c, 0x9c, 0x0};
  size_t llen = (size_t)len;
  BYPASS_MSG req = {0};

  if (data == NULL) {
    syslog(LOG_WARNING, "%s(): NULL bypass data", __func__);
    return -1;
  }

  memset(&req, 0, sizeof(req));
  memset(rbuf, 0, sizeof(rbuf));

  memcpy(req.iana_id, iana_id, MIN(sizeof(req.iana_id), sizeof(iana_id)));
  req.bypass_intf = BMC_INTF;
  memcpy(req.bypass_data, data, MIN(sizeof(req.bypass_data), llen));

  tlen = sizeof(BYPASS_MSG_HEADER) + len;

  if (bic_ipmb_send(FRU_SLOT1, NETFN_OEM_1S_REQ, CMD_OEM_1S_MSG_OUT, (uint8_t*) &req, tlen, rbuf, &rlen, BB_BIC_INTF) < 0) {
    syslog(LOG_WARNING, "%s(): fail to bypass command to another BMC", __func__);
    return -1;
  }

  return 0;
}

int
bic_enable_ina233_alert(uint8_t fru, bool enable) {
  uint8_t tbuf[5] = {0x00};
  uint8_t tlen = sizeof(tbuf);
  uint8_t rbuf[MAX_IPMB_RES_LEN] = {0};
  uint8_t rlen = 0;
  uint8_t ret = BIC_STATUS_FAILURE;
  // File the IANA ID
  memcpy(tbuf, (uint8_t *)&IANA_ID, 3);
  tbuf[3] = 0x1;
  tbuf[4] = enable?0x0:0x1;

  switch(fru) {
    case FRU_2U_TOP:
      ret = bic_ipmb_send(FRU_SLOT1, NETFN_OEM_1S_REQ, BIC_CMD_OEM_INA233_ALERT_CTRL, tbuf, tlen, rbuf, &rlen, RREXP_BIC_INTF1);
      break;
    case FRU_2U_BOT:
      ret = bic_ipmb_send(FRU_SLOT1, NETFN_OEM_1S_REQ, BIC_CMD_OEM_INA233_ALERT_CTRL, tbuf, tlen, rbuf, &rlen, RREXP_BIC_INTF2);
      break;
    case FRU_CWC:
      ret = bic_ipmb_send(FRU_SLOT1, NETFN_OEM_1S_REQ, BIC_CMD_OEM_INA233_ALERT_CTRL, tbuf, tlen, rbuf, &rlen, RREXP_BIC_INTF1);
      if ( ret != BIC_STATUS_SUCCESS ) break;
      ret = bic_ipmb_send(FRU_SLOT1, NETFN_OEM_1S_REQ, BIC_CMD_OEM_INA233_ALERT_CTRL, tbuf, tlen, rbuf, &rlen, RREXP_BIC_INTF2);
      if ( ret != BIC_STATUS_SUCCESS ) break;
      break;
    default:
      printf("not expantion fru: %d\n", fru);
      return ret;
  }
  return ret;
}

int
cpld_update_flag(uint8_t slot_id, uint8_t data, uint8_t read_cnt, uint8_t *rbuf) {
  int ret = BIC_STATUS_SUCCESS;
  int retries = RETRY_3_TIME;
  uint8_t txbuf[32] = {0};
  uint8_t rxbuf[32] = {0};
  uint8_t txlen = 0;
  uint8_t rxlen = 0;

  txbuf[0] = CPLD_BB_BUS;
  txbuf[1] = CPLD_FLAG_REG_ADDR;
  txbuf[2] = read_cnt;
  txbuf[3] = 0x0D;
  //only do one action at the time.
  //use read_cnt to check which actions should be performed.
  if ( read_cnt > 0 ) {
    txlen = 4;
  } else {
    txbuf[4] = data;
    txlen = 5;
  }

  do {
    ret = bic_ipmb_send(slot_id, NETFN_APP_REQ, CMD_APP_MASTER_WRITE_READ, txbuf, txlen, rxbuf, &rxlen, BB_BIC_INTF);
    if ( ret < 0 ) {
      sleep(1);
    } else break;
  } while ( retries-- > 0 );

  if ( ret < 0 ) {
    syslog(LOG_WARNING, "%s() Failed to set/get the flag of CPLD", __func__);
  } else {
    if ( read_cnt > 0 ) {
      *rbuf = rxbuf[0];
    }
  }

  return ret;
}

// In class 2 system, to prevent two BMC update bb firmware at the same time,
// use BB CPLD register to determine whether to update firmware or not.
// 1. Read the register to check no one is updating.
// 2. Set register to notify the update will be started.
// 3. Check the register not be overrided by another BMC.
int
bb_fw_update_prepare(uint8_t slot_id) {
  uint8_t mb_index;
  uint8_t rxbuf = 0x0;

  if (cpld_update_flag(slot_id, 0x0, 1, &rxbuf) || rxbuf != 0) {
    syslog(LOG_WARNING, "another slot is updating baseboard firmware");
    return -1;
  }

  if (bic_get_mb_index(&mb_index) < 0) {
    syslog(LOG_WARNING, "Failed to get MB index");
    return -1;
  }
  // write register to notify update wiil be started
  if (cpld_update_flag(slot_id, mb_index, 0, NULL)) {
    return -1;
  }
  sleep(1);
  if (cpld_update_flag(slot_id, 0x0, 1, &rxbuf)) {
    return -1;
  }
  if (rxbuf != mb_index) {
    syslog(LOG_WARNING, "slot%d is updating baseboard firmware", mb_index);
    return -1;
  }

  return 0;
}

int
bb_fw_update_finish(uint8_t slot_id) {
  return cpld_update_flag(slot_id, 0x0, 0, NULL);
}

int
bic_get_m2_config(uint8_t *config, uint8_t slot, uint8_t intf) {
  uint8_t tbuf[4] = {0x9c, 0x9c, 0x00, 0x00};
  uint8_t rbuf[8] = {0x00};
  uint8_t tlen = 4;
  uint8_t rlen = 0;
  int ret = 0;

  ret = bic_ipmb_send(slot, NETFN_OEM_1S_REQ, BIC_CMD_GPV3_GET_M2_CONFIG, tbuf, tlen, rbuf, &rlen, intf);

  if (ret == 0 && rlen == 4) {
    *config = rbuf[3];
  }

  return ret;
}

void
bic_open_cwc_usb(uint8_t slot) {
  bic_gpio_t gpio = {0};
  printf("Opening USB hubs...\n");
  if (bic_get_gpio(slot, &gpio, NONE_INTF) == 0 &&
      (gpio.gpio[0] & (1 << RST_USB_HUB_N)) == GPIO_LOW) { //not to open all hubs at once
    remote_bic_set_gpio(slot, CWC_GPIO_RST_USB_HUB, GPIO_LOW, REXP_BIC_INTF);
    remote_bic_set_gpio(slot, GPV3_GPIO_RST_USB_HUB1, GPIO_LOW, RREXP_BIC_INTF1);
    remote_bic_set_gpio(slot, GPV3_GPIO_RST_USB_HUB2, GPIO_LOW, RREXP_BIC_INTF1);
    remote_bic_set_gpio(slot, GPV3_GPIO_RST_USB_HUB3, GPIO_LOW, RREXP_BIC_INTF1);
    remote_bic_set_gpio(slot, GPV3_GPIO_RST_USB_HUB1, GPIO_LOW, RREXP_BIC_INTF2);
    remote_bic_set_gpio(slot, GPV3_GPIO_RST_USB_HUB2, GPIO_LOW, RREXP_BIC_INTF2);
    remote_bic_set_gpio(slot, GPV3_GPIO_RST_USB_HUB3, GPIO_LOW, RREXP_BIC_INTF2);
  }

  bic_set_gpio(slot, RST_USB_HUB_N, GPIO_HIGH);
  sleep(SW_USB_HUB_DELAY);
  remote_bic_set_gpio(slot, CWC_GPIO_RST_USB_HUB, GPIO_HIGH, REXP_BIC_INTF);
  sleep(SW_USB_HUB_DELAY);
  remote_bic_set_gpio(slot, GPV3_GPIO_RST_USB_HUB1, GPIO_HIGH, RREXP_BIC_INTF1);
  remote_bic_set_gpio(slot, GPV3_GPIO_RST_USB_HUB2, GPIO_HIGH, RREXP_BIC_INTF1);
  remote_bic_set_gpio(slot, GPV3_GPIO_RST_USB_HUB3, GPIO_HIGH, RREXP_BIC_INTF1);
  sleep(SW_USB_HUB_DELAY);
  remote_bic_set_gpio(slot, GPV3_GPIO_RST_USB_HUB1, GPIO_HIGH, RREXP_BIC_INTF2);
  remote_bic_set_gpio(slot, GPV3_GPIO_RST_USB_HUB2, GPIO_HIGH, RREXP_BIC_INTF2);
  remote_bic_set_gpio(slot, GPV3_GPIO_RST_USB_HUB3, GPIO_HIGH, RREXP_BIC_INTF2);
  sleep(SW_USB_HUB_DELAY);
  return;
}

void
bic_close_cwc_usb(uint8_t slot) {
  printf("Closing USB hubs...\n");
  remote_bic_set_gpio(slot, GPV3_GPIO_RST_USB_HUB1, GPIO_LOW, RREXP_BIC_INTF1);
  remote_bic_set_gpio(slot, GPV3_GPIO_RST_USB_HUB2, GPIO_LOW, RREXP_BIC_INTF1);
  remote_bic_set_gpio(slot, GPV3_GPIO_RST_USB_HUB3, GPIO_LOW, RREXP_BIC_INTF1);
  sleep(SW_USB_HUB_DELAY);
  remote_bic_set_gpio(slot, GPV3_GPIO_RST_USB_HUB1, GPIO_LOW, RREXP_BIC_INTF2);
  remote_bic_set_gpio(slot, GPV3_GPIO_RST_USB_HUB2, GPIO_LOW, RREXP_BIC_INTF2);
  remote_bic_set_gpio(slot, GPV3_GPIO_RST_USB_HUB3, GPIO_LOW, RREXP_BIC_INTF2);
  sleep(SW_USB_HUB_DELAY);
  remote_bic_set_gpio(slot, CWC_GPIO_RST_USB_HUB, GPIO_LOW, REXP_BIC_INTF);
  sleep(SW_USB_HUB_DELAY);
  bic_set_gpio(slot, RST_USB_HUB_N, GPIO_LOW);
  sleep(SW_USB_HUB_DELAY);
  //wait for usb devices to be released

  remote_bic_set_gpio(slot, CWC_GPIO_RST_USB_HUB, GPIO_HIGH, REXP_BIC_INTF);
  remote_bic_set_gpio(slot, GPV3_GPIO_RST_USB_HUB1, GPIO_HIGH, RREXP_BIC_INTF1);
  remote_bic_set_gpio(slot, GPV3_GPIO_RST_USB_HUB2, GPIO_HIGH, RREXP_BIC_INTF1);
  remote_bic_set_gpio(slot, GPV3_GPIO_RST_USB_HUB3, GPIO_HIGH, RREXP_BIC_INTF1);
  remote_bic_set_gpio(slot, GPV3_GPIO_RST_USB_HUB1, GPIO_HIGH, RREXP_BIC_INTF2);
  remote_bic_set_gpio(slot, GPV3_GPIO_RST_USB_HUB2, GPIO_HIGH, RREXP_BIC_INTF2);
  remote_bic_set_gpio(slot, GPV3_GPIO_RST_USB_HUB3, GPIO_HIGH, RREXP_BIC_INTF2);
  return;
}
