/*
 *
 * Copyright 2015-present Facebook. All Rights Reserved.
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
#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <assert.h>
#include <syslog.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/resource.h>
#include <linux/limits.h>

#include <openbmc/kv.h>
#include <openbmc/log.h>
#include <openbmc/obmc-i2c.h>

#include "bic.h"

#define SIZE_IANA_ID 3
#define SDR_READ_COUNT_MAX 0x1A
#define FRUID_WRITE_COUNT_MAX 0x30
#define GPIO_MAX 31

#define IPMB_WRITE_COUNT_MAX 224
#define BIOS_ERASE_PKT_SIZE (64 * 1024)
#define BIOS_VERIFY_PKT_SIZE (32 * 1024)
#define BIOS_VER_REGION_SIZE (4 * 1024 * 1024)
#define BIOS_VER_STR "XG1_"

#define BIC_CMD_RUN 0x22
#define BIC_CMD_DOWNLOAD 0x21
#define BIC_FLASH_START 0x8000
#define BIC_CMD_STATUS 0x23
#define BIC_PKT_MAX 252
#define BIC_CMD_DATA 0x24
#define BIC_IANA_ID {0x15, 0xA0, 0x00}
#define BIC_IANA_ID_SIZE 3

#define CMD_RUN_SIZE 7
#define CMD_DOWNLOAD_SIZE 11
#define CMD_STATUS_SIZE 3

#define FRU_SCM 1

#pragma pack(push, 1)
typedef struct _sdr_rec_hdr_t {
  uint16_t rec_id;
  uint8_t ver;
  uint8_t type;
  uint8_t len;
} sdr_rec_hdr_t;
#pragma pack(pop)

/*
 * BIC GPIO pin names.
 */
#define BIC_GPIO_DEF(type, name)  [type] = name
static const char *gpio_pin_names[BIC_GPIO_MAX] = {
  BIC_GPIO_LIST,
};
#undef BIC_GPIO_DEF

#define BIC_ASSERT(expr, fmt, args...)                      \
  do {                                                      \
    if (!(expr)) {                                          \
      OBMC_CRIT("%s %s: " fmt, __FILE__, __func__, ##args); \
      assert(0);                                            \
    }                                                       \
  } while (0)

#define IPMB_XFER_VERIFY(ret, res_size, exp_res_size) \
  do {                                                \
    if ((ret) != 0) {                                 \
      return -1;                                      \
    } else if ((res_size) != (exp_res_size)) {        \
      errno = EBADMSG;                                \
      return -1;                                      \
    }                                                 \
  } while (0)

/*
 * NOTE:
 *   - callers need to set "rxlen" to "rxbuf" size to avoid buffer
 *     overflow.
 *   - if the function returns successfully, "rxlen" would be set to the
 *     actual response length.
 */
int bic_ipmb_wrapper(uint8_t slot_id, uint8_t netfn, uint8_t cmd,
                     uint8_t* txbuf, size_t txlen, uint8_t* rxbuf,
                     size_t* rxlen) {
  unsigned short tlen;
  unsigned char rlen = 0;
  uint8_t rbuf[MAX_IPMB_RES_LEN] = {0};
  uint8_t tbuf[MAX_IPMB_RES_LEN] = {0};
  ipmb_req_t* req = (ipmb_req_t*)tbuf;
  ipmb_res_t* res = (ipmb_res_t*)rbuf;

  if (txlen) {
    if ((txbuf == NULL) ||
        (IPMB_HDR_SIZE + IPMI_REQ_HDR_SIZE + txlen > sizeof(tbuf))) {
      errno = EINVAL;
      return -1;
    }
    memcpy(req->data, txbuf, txlen);
  }
  req->res_slave_addr = BRIDGE_SLAVE_ADDR << 1;
  req->netfn_lun = netfn << LUN_OFFSET;
  req->hdr_cksum = req->res_slave_addr + req->netfn_lun;
  req->hdr_cksum = ZERO_CKSUM_CONST - req->hdr_cksum;
  req->req_slave_addr = BMC_SLAVE_ADDR << 1;
  req->seq_lun = 0x00;
  req->cmd = cmd;

  // Invoke IPMB library handler
  tlen = IPMB_HDR_SIZE + IPMI_REQ_HDR_SIZE + txlen;
  if (lib_ipmb_handle(slot_id, tbuf, tlen, rbuf, &rlen) != 0) {
    return -1;
  }

  // Handle IPMB response
  if (res->cc || rlen < (IPMB_HDR_SIZE + IPMI_RESP_HDR_SIZE)) {
    errno = EBADMSG;
    return -1;
  }

  // copy the received data back to caller
  if (rxbuf != NULL && rxlen != NULL) {
    rlen -= (IPMB_HDR_SIZE + IPMI_RESP_HDR_SIZE);
    if (rlen > *rxlen) {
      OBMC_WARN("%s: rxbuf truncated: expect %u, actual %u", __func__,
                rlen, *rxlen);
      rlen = *rxlen;
    }
    *rxlen = rlen;
    memcpy(rxbuf, res->data, *rxlen);
  }
  return 0;
}

const char *bic_gpio_name(unsigned int pin)
{
  if ((pin < BIC_GPIO_MAX) && (gpio_pin_names[pin] != NULL)) {
    return gpio_pin_names[pin];
  }

  return "";
}

int bic_get_gpio_config(uint8_t slot_id, uint8_t gpio,
                        bic_gpio_config_t* gpio_config) {
  uint8_t tbuf[7] = BIC_IANA_ID; // IANA ID
  uint8_t rbuf[4] = {0x00};
  size_t rlen = sizeof(rbuf);
  uint32_t pin;
  int ret;

  if (gpio > GPIO_MAX || gpio_config == NULL) {
    errno = EINVAL;
    return -1;
  }

  pin = 1 << gpio;
  tbuf[3] = pin & 0xFF;
  tbuf[4] = (pin >> 8) & 0xFF;
  tbuf[5] = (pin >> 16) & 0xFF;
  tbuf[6] = (pin >> 24) & 0xFF;

  ret = bic_ipmb_wrapper(slot_id, NETFN_OEM_1S_REQ, CMD_OEM_1S_GET_GPIO_CONFIG,
                         tbuf, sizeof(tbuf), rbuf, &rlen);
  IPMB_XFER_VERIFY(ret, rlen, sizeof(rbuf));

  *((uint8_t*)gpio_config) = rbuf[BIC_IANA_ID_SIZE];
  return 0;
}

// Get GPIO value and configuration
int bic_get_gpio(uint8_t slot_id, bic_gpio_t* gpio) {
  uint8_t tbuf[3] = BIC_IANA_ID; // IANA ID
  uint8_t rbuf[7] = {0x00};
  size_t rlen = sizeof(rbuf);
  int ret;

  if (gpio == NULL) {
    errno = EINVAL;
    return -1;
  }

  ret = bic_ipmb_wrapper(slot_id, NETFN_OEM_1S_REQ, CMD_OEM_1S_GET_GPIO, tbuf,
                         0x03, rbuf, &rlen);
  IPMB_XFER_VERIFY(ret, rlen, sizeof(rbuf));

  memcpy((uint8_t*)gpio, &rbuf[BIC_IANA_ID_SIZE], 4);
  return 0;
}

// Read 1S server's FRUID
int bic_get_fruid_info(uint8_t slot_id, uint8_t fru_id,
                       ipmi_fruid_info_t* info) {
  int ret;
  size_t rlen = sizeof(*info);

  if (info == NULL) {
    errno = EINVAL;
    return -1;
  }
  ret = bic_ipmb_wrapper(slot_id, NETFN_STORAGE_REQ, CMD_STORAGE_GET_FRUID_INFO,
                         &fru_id, 1, (uint8_t*)info, &rlen);
  IPMB_XFER_VERIFY(ret, rlen, 3);

  return 0;
}

int bic_set_gpio(uint8_t slot_id, uint8_t gpio, uint8_t value) {
  uint8_t tbuf[11] = BIC_IANA_ID; // IANA ID
  uint8_t rbuf[3] = {0x00};
  size_t rlen = sizeof(rbuf);
  int ret;

  // Check for boundary conditions
  if (gpio > GPIO_MAX) {
    errno = EINVAL;
    return -1;
  }

  // Create the mask bytes for the given GPIO#
  if (gpio < 7) {
    tbuf[3] = 1 << gpio;
    tbuf[4] = 0x00;
    tbuf[5] = 0x00;
    tbuf[6] = 0x00;
  } else if (gpio < 15) {
    gpio -= 8;
    tbuf[3] = 0x00;
    tbuf[4] = 1 << gpio;
    tbuf[5] = 0x00;
    tbuf[6] = 0x00;
  } else if (gpio < 23) {
    gpio -= 16;
    tbuf[3] = 0x00;
    tbuf[4] = 0x00;
    tbuf[5] = 1 << gpio;
    tbuf[6] = 0x00;
  } else {
    gpio -= 24;
    tbuf[3] = 0x00;
    tbuf[4] = 0x00;
    tbuf[5] = 0x00;
    tbuf[6] = 1 << gpio;
  }

  // Fill the value
  if (value) {
    memset(&tbuf[7], 0xFF, 4);
  } else {
    memset(&tbuf[7], 0x00, 4);
  }

  ret = bic_ipmb_wrapper(slot_id, NETFN_OEM_1S_REQ, CMD_OEM_1S_SET_GPIO, tbuf,
                         11, rbuf, &rlen);
  IPMB_XFER_VERIFY(ret, rlen, sizeof(rbuf));

  return 0;
}

// Get BIC Configuration
int bic_get_config(uint8_t slot_id, bic_config_t* cfg) {
  uint8_t tbuf[3] = BIC_IANA_ID; // IANA ID
  uint8_t rbuf[4] = {0x00};
  size_t rlen = sizeof(rbuf);
  int ret;

  ret = bic_ipmb_wrapper(slot_id, NETFN_OEM_1S_REQ, CMD_OEM_1S_GET_CONFIG, tbuf,
                         0x03, rbuf, &rlen);
  IPMB_XFER_VERIFY(ret, rlen, sizeof(rbuf));

  *(uint8_t*)cfg = rbuf[BIC_IANA_ID_SIZE];
  return 0;
}

// Set BIC Configuration
int bic_set_config(uint8_t slot_id, bic_config_t* cfg) {
  uint8_t tbuf[4] = BIC_IANA_ID; // IANA ID
  uint8_t rbuf[4] = {0};
  size_t rlen = sizeof(rbuf);
  int ret;

  if (cfg == NULL) {
    errno = EINVAL;
    return -1;
  }

  tbuf[3] = *(uint8_t*)cfg;
  ret = bic_ipmb_wrapper(slot_id, NETFN_OEM_1S_REQ, CMD_OEM_1S_SET_CONFIG, tbuf,
                         sizeof(tbuf), rbuf, &rlen);
  return ret;
}

// Read POST Buffer
int bic_get_post_buf(uint8_t slot_id, uint8_t* buf, uint8_t* len) {
  uint8_t tbuf[3] = BIC_IANA_ID; // IANA ID
  uint8_t rbuf[255] = {0x00};
  size_t rlen = sizeof(rbuf);
  int ret;

  if (buf == NULL || len == NULL) {
    errno = EINVAL;
    return -1;
  }

  ret = bic_ipmb_wrapper(slot_id, NETFN_OEM_1S_REQ, CMD_OEM_1S_GET_POST_BUF,
                         tbuf, sizeof(tbuf), rbuf, &rlen);
  if (ret != 0) {
    return -1;
  } else if (rlen < BIC_IANA_ID_SIZE) {
    errno = EBADMSG;
    return -1;
  }

  /*
   * XXX potential buffer overflow: callers need to set <len> to <buf>
   * size before calling the function.
   */
  rlen -= BIC_IANA_ID_SIZE;
  memcpy(buf, &rbuf[BIC_IANA_ID_SIZE], rlen);
  *len = rlen;

  return ret;
}

// Read System Event Log (SEL)
int bic_get_sel_info(uint8_t slot_id, ipmi_sel_sdr_info_t* info) {
  int ret;
  size_t rlen = sizeof(*info);

  if (info == NULL) {
    errno = EINVAL;
    return -1;
  }

  ret = bic_ipmb_wrapper(slot_id, NETFN_STORAGE_REQ, CMD_STORAGE_GET_SEL_INFO,
                         NULL, 0, (uint8_t*)info, &rlen);
  return ret;
}

int bic_get_sel(uint8_t slot_id, ipmi_sel_sdr_req_t* req,
                ipmi_sel_sdr_res_t* res, size_t* rlen) {
  int ret;

  if (req == NULL || res == NULL || rlen == NULL) {
    errno = EINVAL;
    return -1;
  }

  ret = bic_ipmb_wrapper(slot_id, NETFN_STORAGE_REQ, CMD_STORAGE_GET_SEL,
                         (uint8_t*)req, sizeof(*req), (uint8_t*)res, rlen);
  return ret;
}

// Read Sensor Data Records (SDR)
int bic_get_sdr_info(uint8_t slot_id, ipmi_sel_sdr_info_t* info) {
  int ret;
  size_t rlen = sizeof(*info);

  if (info == NULL) {
    errno = EINVAL;
    return -1;
  }

  ret = bic_ipmb_wrapper(slot_id, NETFN_STORAGE_REQ, CMD_STORAGE_GET_SDR_INFO,
                         NULL, 0, (uint8_t*)info, &rlen);
  return ret;
}

static int _get_sdr(uint8_t slot_id, ipmi_sel_sdr_req_t* req,
                    ipmi_sel_sdr_res_t* res, size_t* rlen) {
  int ret;

  ret = bic_ipmb_wrapper(slot_id, NETFN_STORAGE_REQ, CMD_STORAGE_GET_SDR,
                         (uint8_t*)req, sizeof(*req), (uint8_t*)res, rlen);
  return ret;
}

static int _get_sdr_rsv(uint8_t slot_id, uint16_t* rsv) {
  int ret;
  size_t rlen = sizeof(*rsv);

  ret = bic_ipmb_wrapper(slot_id, NETFN_STORAGE_REQ, CMD_STORAGE_RSV_SDR, NULL,
                         0, (uint8_t*)rsv, &rlen);

  return ret;
}

int bic_get_sdr(uint8_t slot_id, ipmi_sel_sdr_req_t* req,
                ipmi_sel_sdr_res_t* res, size_t* rlen) {
  int ret;
  uint8_t tbuf[MAX_IPMB_RES_LEN] = {0};
  size_t tlen = sizeof(tbuf);
  uint8_t len;
  ipmi_sel_sdr_res_t* tres = (ipmi_sel_sdr_res_t*)tbuf;
  sdr_rec_hdr_t* hdr;

  if (req == NULL || res == NULL || rlen == NULL) {
    errno = EINVAL;
    return -1;
  }

  // Get SDR reservation ID for the given record
  ret = _get_sdr_rsv(slot_id, &req->rsv_id);
  if (ret) {
    return ret;
  }

  // Initialize the response length to zero
  *rlen = 0;

  // Read SDR Record Header
  req->offset = 0;
  req->nbytes = sizeof(sdr_rec_hdr_t);

  ret = _get_sdr(slot_id, req, (ipmi_sel_sdr_res_t*)tbuf, &tlen);
  if (ret) {
    return ret;
  }

  // Copy the next record id to response
  res->next_rec_id = tres->next_rec_id;

  // Copy the header excluding first two bytes(next_rec_id)
  memcpy(res->data, tres->data, tlen - 2);

  // Update response length and offset for next request
  *rlen += tlen - 2;
  req->offset = tlen - 2;

  // Find length of data from header info
  hdr = (sdr_rec_hdr_t*)tres->data;
  len = hdr->len;

  // Keep reading chunks of SDR record in a loop
  while (len > 0) {
    if (len > SDR_READ_COUNT_MAX) {
      req->nbytes = SDR_READ_COUNT_MAX;
    } else {
      req->nbytes = len;
    }

    tlen = sizeof(tbuf);
    ret = _get_sdr(slot_id, req, (ipmi_sel_sdr_res_t*)tbuf, &tlen);
    if (ret) {
      return ret;
    }

    // Copy the data excluding the first two bytes(next_rec_id)
    memcpy(&res->data[req->offset], tres->data, tlen - 2);

    // Update response length, offset for next request, and remaining length
    *rlen += tlen - 2;
    req->offset += tlen - 2;
    len -= tlen - 2;
  }

  return 0;
}

// Get Device ID
int bic_get_dev_id(uint8_t slot_id, ipmi_dev_id_t* dev_id) {
  size_t rlen = sizeof(*dev_id);
  int ret;

  if (dev_id == NULL) {
    errno = EINVAL;
    return -1;
  }

  ret = bic_ipmb_wrapper(slot_id, NETFN_APP_REQ, CMD_APP_GET_DEVICE_ID, NULL, 0,
                         (uint8_t*)dev_id, &rlen);

  return ret;
}

int bic_read_sensor(uint8_t slot_id, uint8_t sensor_num,
                    ipmi_sensor_reading_t* sensor) {
  int ret;
  size_t rlen = sizeof(*sensor);

  if (sensor == NULL) {
    errno = EINVAL;
    return -1;
  }

  ret =
      bic_ipmb_wrapper(slot_id, NETFN_SENSOR_REQ, CMD_SENSOR_GET_SENSOR_READING,
                       (uint8_t*)&sensor_num, 1, (uint8_t*)sensor, &rlen);

  return ret;
}

static int _read_fruid(uint8_t slot_id, uint8_t fru_id, uint32_t offset,
                       uint8_t count, uint8_t* rbuf, size_t* rlen) {
  int ret;
  uint8_t tbuf[4] = {0};

  tbuf[0] = fru_id;
  tbuf[1] = offset & 0xFF;
  tbuf[2] = (offset >> 8) & 0xFF;
  tbuf[3] = count;

  ret =
      bic_ipmb_wrapper(slot_id, NETFN_STORAGE_REQ, CMD_STORAGE_READ_FRUID_DATA,
                       tbuf, sizeof(tbuf), rbuf, rlen);
  return ret;
}

int bic_read_fruid(uint8_t slot_id, uint8_t fru_id, const char* path,
                   int* fru_size) {
  int ret = 0;
  uint32_t nread;
  uint32_t offset;
  uint8_t count;
  uint8_t rbuf[MAX_IPMI_MSG_SIZE] = {0};
  size_t rlen = 0;
  int fd = -1;
  ipmi_fruid_info_t info;

  if (path == NULL || fru_size == NULL) {
    errno = EINVAL;
    return -1;
  }

  // Remove the file if exists already
  unlink(path);

  // Open the file exclusively for write
  fd = open(path, O_WRONLY | O_CREAT | O_EXCL, 0666);
  if (fd < 0) {
    return -1;
  }

  // Read the FRUID information
  ret = bic_get_fruid_info(slot_id, fru_id, &info);
  if (ret) {
    goto error_exit;
  }

  // Indicates the size of the FRUID
  nread = (info.size_msb << 8) + (info.size_lsb);
  *fru_size = nread;
  if (*fru_size == 0)
    goto error_exit;

  // Read chunks of FRUID binary data in a loop
  offset = 0;
  while (nread > 0) {
    if (nread > FRUID_READ_COUNT_MAX) {
      count = FRUID_READ_COUNT_MAX;
    } else {
      count = nread;
    }

    rlen = sizeof(rbuf);
    ret = _read_fruid(slot_id, fru_id, offset, count, rbuf, &rlen);
    if (ret) {
      goto error_exit;
    }

    // Ignore the first byte as it indicates length of response
    ret = write(fd, &rbuf[1], rlen - 1);
    if (ret < 0) {
      OBMC_ERROR(errno, "failed to write %s", path);
      goto error_exit;
    } else if (ret != rlen - 1) {
      OBMC_WARN("data truncated (write %s): expect %u, actual %d\n",
                path, rlen - 1, ret);
      /*
       * XXX shall we exit or continue?
       */
    }

    // Update offset
    offset += (rlen - 1);
    nread -= (rlen - 1);
  }

  close(fd);
  return 0;

error_exit:
  if (fd >= 0) {
    close(fd);
  }
  return -1;
}

int bic_read_mac(uint8_t slot_id, char* rbuf, size_t rlen) {
  uint8_t tbuf[2] = {0x00, 0x01};
  int ret;

  ret = bic_ipmb_wrapper(slot_id, NETFN_OEM_REQ, CMD_OEM_GET_MAC_ADDR, tbuf,
                         sizeof(tbuf), (uint8_t*)rbuf, &rlen);
  if (ret)
    return -1;

  return 0;
}

static int set_fw_update_ongoing(uint8_t fru_id, uint16_t timeout) {
  char key[64];
  char value[64];
  struct timespec ts;

  snprintf(key, sizeof(key), "fru%d_fwupd", fru_id);
  clock_gettime(CLOCK_MONOTONIC, &ts);
  ts.tv_sec += timeout;
  snprintf(value, sizeof(value), "%d", (int)ts.tv_sec);

  if (kv_set(key, value, 0, 0) < 0) {
    return -1;
  }

  return 0;
}

// Update firmware for various components
static int _update_fw(uint8_t slot_id, uint8_t target, uint32_t offset,
                      uint16_t len, uint8_t* buf) {
  uint8_t tbuf[MAX_IPMI_MSG_SIZE] = BIC_IANA_ID; // IANA ID
  uint8_t rbuf[16] = {0x00};
  size_t tlen, rlen;
  int ret;
  int retries = 3;

  // Fill the component for which firmware is requested
  tbuf[3] = target;
  tbuf[4] = (offset)&0xFF;
  tbuf[5] = (offset >> 8) & 0xFF;
  tbuf[6] = (offset >> 16) & 0xFF;
  tbuf[7] = (offset >> 24) & 0xFF;
  tbuf[8] = len & 0xFF;
  tbuf[9] = (len >> 8) & 0xFF;

  tlen = len + 10;
  BIC_ASSERT(tlen < sizeof(tbuf), "txbuf overflow: txlen=%u, buflen=%u",
             tlen, sizeof(tbuf));
  memcpy(&tbuf[10], buf, len);

  while (retries-- > 0) {
    rlen = sizeof(rbuf);
    ret = bic_ipmb_wrapper(slot_id, NETFN_OEM_1S_REQ, CMD_OEM_1S_UPDATE_FW,
                           tbuf, tlen, rbuf, &rlen);
    if (ret == 0)
      break;

    sleep(1);
    OBMC_INFO("%s: slot: %d, target %d, offset: %d, len: %d retrying..\n",
              __func__, slot_id, target, offset, len);
  }

  return ret;
}

// Read Firwmare Versions of various components
static int _enable_bic_update(uint8_t slot_id) {
  uint8_t tbuf[4] = BIC_IANA_ID;
  uint8_t rbuf[16] = {0x00};
  size_t rlen = sizeof(rbuf);
  int ret;

  // 0x1: Update on I2C Channel, the only available option from BMC
  tbuf[3] = 0x1;
  ret = bic_ipmb_wrapper(slot_id, NETFN_OEM_1S_REQ,
                         CMD_OEM_1S_ENABLE_BIC_UPDATE,
                         tbuf, sizeof(tbuf), rbuf, &rlen);
  return ret;
}

// Helper Functions
static void msleep(int msec) {
  struct timespec req;

  req.tv_sec = 0;
  req.tv_nsec = msec * 1000 * 1000;

  while (nanosleep(&req, &req) == -1 && errno == EINTR) {
    continue;
  }
}

static int bic_i2c_xfer(int fd, uint8_t* tbuf, uint8_t tcount, uint8_t* rbuf,
                        uint8_t rcount) {
  struct i2c_rdwr_ioctl_data data;
  struct i2c_msg msg[2];
  int n_msg = 0;
  int rc;

  memset(&msg, 0, sizeof(msg));

  if (tcount) {
    msg[n_msg].addr = BRIDGE_SLAVE_ADDR;
    msg[n_msg].flags = 0;
    msg[n_msg].len = tcount;
    msg[n_msg].buf = tbuf;
    n_msg++;
  }

  if (rcount) {
    msg[n_msg].addr = BRIDGE_SLAVE_ADDR;
    msg[n_msg].flags = I2C_M_RD;
    msg[n_msg].len = rcount;
    msg[n_msg].buf = rbuf;
    n_msg++;
  }

  data.msgs = msg;
  data.nmsgs = n_msg;

  OBMC_DEBUG("start i2c xfer with bic: tx=%u, rx=%u bytes", tcount, rcount);
  rc = ioctl(fd, I2C_RDWR, &data);
  if (rc < 0) {
    return -1;
  }

  return 0;
}

static int run_shell_cmd(const char* cmd) {
  OBMC_DEBUG("running <%s> command\n", cmd);
  return system(cmd);
}

static int prepare_update_bic(uint8_t slot_id, int ifd, int size) {
  int i = 0;
  uint8_t tbuf[MAX_IPMI_MSG_SIZE] = {0};
  uint8_t rbuf[16] = {0};
  uint8_t tcount;
  uint8_t rcount;

  /* Kill ipmb daemon for this slot */
  run_shell_cmd("sv stop ipmbd_0");
  sleep(1);
  OBMC_INFO("stopped ipmbd for slot %u\n", slot_id);

  /* Restart ipmb daemon with "-u|--enable-bic-update" for bic update */
  run_shell_cmd("/usr/local/bin/ipmbd -u 0 1 > /dev/null 2>&1 &");
  OBMC_INFO("started 'ipmbd -u' for minilake\n");

  sleep(2);

  /* Enable Bridge-IC update */
  _enable_bic_update(slot_id);

  /* Kill ipmb daemon "--enable-bic-update" for this slot */
  run_shell_cmd(
      "ps -w | grep -v 'grep' | grep 'ipmbd -u 0' | awk '{print $1}' "
      "| xargs kill");
  OBMC_INFO("stopped 'ipmbd -u' for minilake\n");

  // The I2C fast speed clock (400KHz) may cause to read BIC data abnormally.
  // So reduce I2C bus clock speed which is a workaround for BIC update.
  run_shell_cmd("devmem 0x1e78a044 w 0xfff77304");
  OBMC_INFO("updated i2c-0 timing settings\n");

  sleep(1);

  /* Start Bridge IC update(0x21) */
  tbuf[0] = CMD_DOWNLOAD_SIZE;
  tbuf[1] = 0x00; // Checksum, will fill later
  tbuf[2] = BIC_CMD_DOWNLOAD;

  /* update flash address: 0x8000 */
  tbuf[3] = (BIC_FLASH_START >> 24) & 0xff;
  tbuf[4] = (BIC_FLASH_START >> 16) & 0xff;
  tbuf[5] = (BIC_FLASH_START >> 8) & 0xff;
  tbuf[6] = (BIC_FLASH_START)&0xff;

  /* image size */
  tbuf[7] = (size >> 24) & 0xff;
  tbuf[8] = (size >> 16) & 0xff;
  tbuf[9] = (size >> 8) & 0xff;
  tbuf[10] = (size)&0xff;

  /* calcualte checksum for data portion */
  for (i = 2; i < CMD_DOWNLOAD_SIZE; i++) {
    tbuf[1] += tbuf[i];
  }

  tcount = CMD_DOWNLOAD_SIZE;
  rcount = 0;

  if (bic_i2c_xfer(ifd, tbuf, tcount, rbuf, rcount)) {
    OBMC_ERROR(errno, "bic_i2c_xfer failed download");
    return -1;
  }
  msleep(500); // delay for download command process

  tcount = 0;
  rcount = 2;
  if (bic_i2c_xfer(ifd, tbuf, tcount, rbuf, rcount)) {
    OBMC_ERROR(errno, "bic_i2c_xfer failed download ack");
    return -1;
  }

  if (rbuf[0] != 0x00 || rbuf[1] != 0xcc) {
    OBMC_WARN("unexpected download response: %x:%x\n", rbuf[0], rbuf[1]);
    return -1;
  }

  return 0;
}

static int update_bic_status(uint8_t slot_id, int ifd) {
  uint8_t tbuf[MAX_IPMI_MSG_SIZE] = {0};
  uint8_t rbuf[16] = {0};
  uint8_t tcount;
  uint8_t rcount;

  /* Get Status */
  tbuf[0] = CMD_STATUS_SIZE;
  tbuf[1] = BIC_CMD_STATUS; // checksum same as data
  tbuf[2] = BIC_CMD_STATUS;

  tcount = CMD_STATUS_SIZE;
  rcount = 0;

  if (bic_i2c_xfer(ifd, tbuf, tcount, rbuf, rcount)) {
    OBMC_ERROR(errno, "failed to get BIC_CMD_STATUS");
    return -1;
  }

  tcount = 0;
  rcount = 5;
  if (bic_i2c_xfer(ifd, tbuf, tcount, rbuf, rcount)) {
    OBMC_ERROR(errno, "failed to get status ack");
    return -1;
  }

  if (rbuf[0] != 0x00 || rbuf[1] != 0xcc || rbuf[2] != 0x03 ||
      rbuf[3] != 0x40 || rbuf[4] != 0x40) {
    OBMC_WARN("unexpected status resp: %x:%x:%x:%x:%x\n", rbuf[0], rbuf[1],
              rbuf[2], rbuf[3], rbuf[4]);
    return -1;
  }

  return 0;
}

static int _update_bic_main(uint8_t slot_id, const char* path) {
  int fd;
  int ifd = -1;
  struct stat buf;
  int size;
  uint8_t tbuf[MAX_IPMI_MSG_SIZE] = {0};
  uint8_t rbuf[16] = {0};
  uint8_t tcount;
  uint8_t rcount;
  volatile int xcount;
  int i = 0, retry = 5;
  int ret = -1, rc;
  uint8_t xbuf[MAX_IPMI_MSG_SIZE] = {0};
  uint32_t offset = 0, last_offset = 0, dsize;

  syslog(LOG_CRIT, "update bic firmware on minilake\n");

  // Open the file exclusively for read
  fd = open(path, O_RDONLY, 0666);
  if (fd < 0) {
    syslog(LOG_ERR, "bic_update_fw: open fails for path: %s\n", path);
    goto error_exit;
  }

  fstat(fd, &buf);
  size = buf.st_size;
  OBMC_INFO("size of %s is %d bytes\n", path, size);
  dsize = size / 20;

  // Open the i2c driver
  ifd = i2c_cdev_slave_open(0, BRIDGE_SLAVE_ADDR, 0); // bus 0
  if (ifd < 0) {
    printf("ifd error\n");
    goto error_exit;
  }

  if (prepare_update_bic(slot_id, ifd, size)) {
    goto error_exit;
  }

  // Loop to send all the image data
  while (1) {
    /* Sometimes BIC update status get fail, so add retry for workaround */
    while ((rc = update_bic_status(slot_id, ifd)) != 0 && retry--) {
      OBMC_WARN("update_bic, BIC status get fail retrying ...\n");
      offset = 0;
      last_offset = 0;
      lseek(fd, 0, SEEK_SET);
      prepare_update_bic(slot_id, ifd, size);
    }
    if (rc) {
      goto error_exit;
    }

    // Send ACK ---
    tbuf[0] = 0xcc;
    tcount = 1;
    rcount = 0;
    rc = bic_i2c_xfer(ifd, tbuf, tcount, rbuf, rcount);
    if (rc) {
      OBMC_ERROR(errno, "failed send ack");
      goto error_exit;
    }

    // send next packet
    xcount = read(fd, xbuf, BIC_PKT_MAX);
    if (xcount <= 0) {
      break;
    }

    offset += xcount;
    if ((last_offset + dsize) <= offset) {
      OBMC_INFO("updated bic: %d %%\n", offset / dsize * 5);
      last_offset += dsize;
    }

    tbuf[0] = xcount + 3;
    tbuf[1] = BIC_CMD_DATA;
    tbuf[2] = BIC_CMD_DATA;
    memcpy(&tbuf[3], xbuf, xcount);

    for (i = 0; i < xcount; i++) {
      tbuf[1] += xbuf[i];
    }

    tcount = tbuf[0];
    rcount = 0;

    rc = bic_i2c_xfer(ifd, tbuf, tcount, rbuf, rcount);
    if (rc) {
      OBMC_ERROR(errno, "error send data");
      goto error_exit;
    }

    msleep(10);
    tcount = 0;
    rcount = 2;

    rc = bic_i2c_xfer(ifd, tbuf, tcount, rbuf, rcount);
    if (rc) {
      OBMC_ERROR(errno, "error send data ack");
      goto error_exit;
    }

    if (rbuf[0] != 0x00 || rbuf[1] != 0xcc) {
      OBMC_WARN("data error: %#x:%#x\n", rbuf[0], rbuf[1]);
      goto error_exit;
    }
  }
  msleep(500);

  // Run the new image
  tbuf[0] = CMD_RUN_SIZE;
  tbuf[1] = 0x00; // checksum
  tbuf[2] = BIC_CMD_RUN;
  tbuf[3] = (BIC_FLASH_START >> 24) & 0xff;
  tbuf[4] = (BIC_FLASH_START >> 16) & 0xff;
  tbuf[5] = (BIC_FLASH_START >> 8) & 0xff;
  tbuf[6] = (BIC_FLASH_START)&0xff;

  for (i = 2; i < CMD_RUN_SIZE; i++) {
    tbuf[1] += tbuf[i];
  }

  tcount = CMD_RUN_SIZE;
  rcount = 0;

  rc = bic_i2c_xfer(ifd, tbuf, tcount, rbuf, rcount);
  if (rc) {
    OBMC_ERROR(errno, "bic_i2c_xfer failed run");
    goto error_exit;
  }

  tcount = 0;
  rcount = 2;

  rc = bic_i2c_xfer(ifd, tbuf, tcount, rbuf, rcount);
  if (rc) {
    OBMC_ERROR(errno, "failed run ack");
    goto error_exit;
  }

  if (rbuf[0] != 0x00 || rbuf[1] != 0xcc) {
    OBMC_WARN("unexpected run response: %#x:%#x\n", rbuf[0], rbuf[1]);
    goto error_exit;
  }
  msleep(500);

  ret = 0;

  // Restore the I2C bus clock to 400KHz.
  OBMC_INFO("restoring i2c-0 timing settings");
  run_shell_cmd("devmem 0x1e78a044 w 0xfff77302");

  // Restart ipmbd daemon
  sleep(1);
  run_shell_cmd("sv start ipmbd_0");

error_exit:
  syslog(LOG_CRIT, "bic_update_fw: updating bic firmware is exiting\n");
  if (fd > 0) {
    close(fd);
  }
  if (ifd > 0) {
    close(ifd);
  }
  set_fw_update_ongoing(FRU_SCM, 0);

  return ret;
}

static int check_bios_image(int fd, long size) {
  int i, rcnt, end;
  uint8_t* buf;
  uint8_t ver_sig[] = {0x46, 0x49, 0x44, 0x04, 0x78, 0x00};
  if (size < BIOS_VER_REGION_SIZE)
    return -1;

  buf = (uint8_t*)malloc(BIOS_VER_REGION_SIZE);
  if (!buf) {
    return -1;
  }
  lseek(fd, (size - BIOS_VER_REGION_SIZE), SEEK_SET);
  i = 0;
  while (i < BIOS_VER_REGION_SIZE) {
    rcnt = read(fd, (buf + i), BIOS_ERASE_PKT_SIZE);
    if ((rcnt < 0) && (errno != EINTR)) {
      free(buf);
      return -1;
    }
    i += rcnt;
  }
  end = BIOS_VER_REGION_SIZE - (sizeof(ver_sig) + strlen(BIOS_VER_STR));
  for (i = 0; i < end; i++) {
    if (!memcmp(buf + i, ver_sig, sizeof(ver_sig))) {
      if (memcmp(buf + i + sizeof(ver_sig), BIOS_VER_STR,
                 strlen(BIOS_VER_STR))) {
        i = end;
      }
      break;
    }
  }
  free(buf);

  if (i >= end)
    return -1;
  lseek(fd, 0, SEEK_SET);
  return 0;
}

static int check_cpld_image(int fd, long size) {
  int i, j, rcnt;
  uint8_t *buf, data;
  uint16_t crc_exp, crc_val = 0xffff;
  uint32_t dword, crc_offs;

  if (size < 52)
    return -1;

  buf = (uint8_t*)malloc(size);
  if (!buf) {
    return -1;
  }

  i = 0;
  while (i < size) {
    rcnt = read(fd, (buf + i), size);
    if ((rcnt < 0) && (errno != EINTR)) {
      free(buf);
      return -1;
    }
    i += rcnt;
  }

  dword = (buf[0] << 24) | (buf[1] << 16) | (buf[2] << 8) | buf[3];
  if ((dword != 0x4A414D00) && (dword != 0x4A414D01)) {
    free(buf);
    return -1;
  }

  i = 32 + (dword & 0x1) * 8;
  crc_offs =
      (buf[i] << 24) | (buf[i + 1] << 16) | (buf[i + 2] << 8) | buf[i + 3];
  if ((crc_offs + sizeof(crc_exp)) > size) {
    free(buf);
    return -1;
  }
  crc_exp = (buf[crc_offs] << 8) | buf[crc_offs + 1];

  for (i = 0; i < crc_offs; i++) {
    data = buf[i];
    for (j = 0; j < 8; j++, data >>= 1) {
      crc_val = ((data ^ crc_val) & 0x1) ?
                ((crc_val >> 1) ^ 0x8408) : (crc_val >> 1);
    }
  }
  crc_val = ~crc_val;
  free(buf);

  if (crc_exp != crc_val)
    return -1;

  lseek(fd, 0, SEEK_SET);
  return 0;
}

// Read checksum of various components
int bic_get_fw_cksum(uint8_t slot_id, uint8_t comp, uint32_t offset,
                     uint32_t len, uint8_t* ver) {
  uint8_t tbuf[12] = BIC_IANA_ID; // IANA ID
  uint8_t rbuf[16] = {0x00};
  size_t rlen = sizeof(rbuf);
  int ret;

  if (ver == NULL) {
    errno = EINVAL;
    return -1;
  }

  // Fill the component for which firmware is requested
  tbuf[3] = comp;

  // Fill the offset
  tbuf[4] = (offset)&0xFF;
  tbuf[5] = (offset >> 8) & 0xFF;
  tbuf[6] = (offset >> 16) & 0xFF;
  tbuf[7] = (offset >> 24) & 0xFF;

  // Fill the length
  tbuf[8] = (len)&0xFF;
  tbuf[9] = (len >> 8) & 0xFF;
  tbuf[10] = (len >> 16) & 0xFF;
  tbuf[11] = (len >> 24) & 0xFF;

  ret = bic_ipmb_wrapper(slot_id, NETFN_OEM_1S_REQ, CMD_OEM_1S_GET_FW_CKSUM,
                         tbuf, 12, rbuf, &rlen);
  // checksum has to be 4 bytes
  if (ret || (rlen != 4 + SIZE_IANA_ID)) {
    syslog(LOG_ERR, "bic_get_fw_cksum: ret: %d, rlen: %d\n", ret, rlen);
    return -1;
  }

  OBMC_DEBUG("cksum returns: %x:%x:%x::%x:%x:%x:%x\n", rbuf[0], rbuf[1],
             rbuf[2], rbuf[3], rbuf[4], rbuf[5], rbuf[6]);

  // Ignore IANA ID
  memcpy(ver, &rbuf[SIZE_IANA_ID], rlen - SIZE_IANA_ID);

  return ret;
}

static int check_vr_image(int fd, long size) {
  uint8_t buf[32];
  uint8_t hdr[] = {0x00, 0x01, 0x4c, 0x1c, 0x00, 0x58, 0x47, 0x31,
                   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

  if (size < 32)
    return -1;

  lseek(fd, 1, SEEK_SET);

  if (read(fd, buf, sizeof(hdr)) != sizeof(hdr))
    return -1;

  if (memcmp(buf, hdr, sizeof(hdr)))
    return -1;

  lseek(fd, 0, SEEK_SET);
  return 0;
}

static struct {
  const char* name;
  int (*check_image)(int fd, long size);
} fw_update_info[UPDATE_VR + 1] = {
    [UPDATE_BIOS] =
        {
            .name = "bios",
            check_bios_image,
        },
    [UPDATE_CPLD] =
        {
            .name = "cpld",
            .check_image = check_cpld_image,
        },
    [UPDATE_BIC_BOOTLOADER] =
        {
            .name = "bic bootloader",
        },
    [UPDATE_BIC] =
        {
            .name = "bic",
        },
    [UPDATE_VR] =
        {
            .name = "vr",
            .check_image = check_vr_image,
        },
};

int bic_update_fw(uint8_t slot_id, uint8_t comp, const char* image_file) {
  uint16_t count, read_count;
  uint8_t buf[MAX_IPMI_MSG_SIZE] = {0};
  int i, fd, rc, ret = -1;
  uint8_t* tbuf = NULL;
  uint32_t dsize, offset, last_offset;
  struct stat st;

  OBMC_INFO("updating fw on slot %d:\n", slot_id);
  // Handle Bridge IC firmware separately as the process differs significantly
  // from others
  if (comp == UPDATE_BIC) {
    set_fw_update_ongoing(FRU_SCM, 60);
    return _update_bic_main(slot_id, image_file);
  }

  // Open the file exclusively for read
  fd = open(image_file, O_RDONLY, 0666);
  if (fd < 0) {
    OBMC_ERROR(errno, "failed to open %s for read", image_file);
    goto error_exit;
  }

  if (stat(image_file, &st) != 0) {
    OBMC_ERROR(errno, "failed to get %s status", image_file);
    goto error_exit;
  }
  switch (comp) {
    case UPDATE_BIOS:
      set_fw_update_ongoing(FRU_SCM, 30);
      dsize = st.st_size / 100;
      break;

    case UPDATE_VR:
      dsize = st.st_size / 5;
      break;

    case UPDATE_CPLD:
    case UPDATE_BIC_BOOTLOADER:
      set_fw_update_ongoing(FRU_SCM, 20);
      dsize = st.st_size / 20;
      break;

    default:
      OBMC_WARN("unsupported firmware component %u\n", comp);
      goto error_exit;
  }
  if ((fw_update_info[comp].check_image != NULL) &&
      (fw_update_info[comp].check_image(fd, st.st_size) < 0)) {
    OBMC_WARN("invalid %s firmware image!", fw_update_info[comp].name);
    goto error_exit;
  }
  syslog(LOG_CRIT, "bic_update_fw: update %s firmware on slot %d\n",
         fw_update_info[comp].name, slot_id);

  // Write chunks of binary data in a loop
  offset = 0;
  last_offset = 0;
  i = 1;
  while (1) {
    uint8_t target;

    // For BIOS, send packets in blocks of 64K
    if (comp == UPDATE_BIOS &&
        ((offset + IPMB_WRITE_COUNT_MAX) > (i * BIOS_ERASE_PKT_SIZE))) {
      read_count = (i * BIOS_ERASE_PKT_SIZE) - offset;
      i++;
    } else {
      read_count = IPMB_WRITE_COUNT_MAX;
    }

    // Read from file
    count = read(fd, buf, read_count);
    if (count < 0) {
      OBMC_ERROR(errno, "failed to read %s", image_file);
      break;
    } else if (count == 0) { /* EOF */
      break;
    }

    // For non-BIOS update, the last packet is indicated by extra flag
    if ((comp != UPDATE_BIOS) && (count < read_count)) {
      target = comp | 0x80;
    } else {
      target = comp;
    }

    // Send data to Bridge-IC
    if (_update_fw(slot_id, target, offset, count, buf) != 0) {
      goto error_exit;
    }

    // Update counter
    offset += count;
    if ((last_offset + dsize) <= offset) {
      switch (comp) {
        case UPDATE_BIOS:
          set_fw_update_ongoing(FRU_SCM, 25);
          printf("updated bios: %d %%\n", offset / dsize);
          break;
        case UPDATE_CPLD:
          printf("updated cpld: %d %%\n", offset / dsize * 5);
          break;
        case UPDATE_VR:
          printf("updated vr: %d %%\n", offset / dsize * 20);
          break;
        default:
          printf("updated bic boot loader: %d %%\n", offset / dsize * 5);
          break;
      }
      last_offset += dsize;
    }
  } /* while */

  if (comp != UPDATE_BIOS) {
    goto update_done;
  }
  set_fw_update_ongoing(FRU_SCM, 55);

  // Checksum calculation for BIOS image
  tbuf = malloc(BIOS_VERIFY_PKT_SIZE * sizeof(uint8_t));
  if (!tbuf) {
    goto error_exit;
  }

  if (lseek(fd, 0, SEEK_SET) == (off_t)-1) {
    OBMC_ERROR(errno, "failed to set %s file offset", image_file);
    goto error_exit;
  }
  offset = 0;
  while (1) {
    uint32_t tcksum, gcksum;

    count = read(fd, tbuf, BIOS_VERIFY_PKT_SIZE);
    if (count < 0) {
      OBMC_ERROR(errno, "failed to read %s", image_file);
      break;
    } else if (count == 0) {
      break; /* EOF */
    }

    tcksum = 0;
    for (i = 0; i < BIOS_VERIFY_PKT_SIZE; i++) {
      tcksum += tbuf[i];
    }

    // Get the checksum of binary image
    rc = bic_get_fw_cksum(slot_id, comp, offset, BIOS_VERIFY_PKT_SIZE,
                          (uint8_t*)&gcksum);
    if (rc) {
      goto error_exit;
    }

    // Compare both and see if they match or not
    if (gcksum != tcksum) {
      OBMC_WARN("checksum does not match offset: %#x, %#x:%#x\n", offset,
                tcksum, gcksum);
      goto error_exit;
    }

    offset += BIOS_VERIFY_PKT_SIZE;
  } /* while */

update_done:
  ret = 0;
error_exit:
  syslog(LOG_CRIT, "bic_update_fw: updating %s firmware is exiting\n",
         fw_update_info[comp].name);
  if (fd > 0) {
    close(fd);
  }
  if (tbuf) {
    free(tbuf);
  }

  set_fw_update_ongoing(FRU_SCM, 0);

  return ret;
}

// Read Firwmare Versions of various components
int bic_get_fw_ver(uint8_t slot_id, uint8_t comp, uint8_t* ver) {
  uint8_t tbuf[4] = BIC_IANA_ID; // IANA ID
  uint8_t rbuf[16] = {0x00};
  size_t rlen = sizeof(rbuf);
  int ret;

  if (ver == NULL) {
    errno = EINVAL;
    return -1;
  }

  // Fill the component for which firmware is requested
  tbuf[3] = comp;
  ret = bic_ipmb_wrapper(slot_id, NETFN_OEM_1S_REQ, CMD_OEM_1S_GET_FW_VER, tbuf,
                         0x04, rbuf, &rlen);
  // fw version has to be between 2 and 5 bytes based on component
  if (ret) {
    return -1;
  } else if ((rlen < 2 + SIZE_IANA_ID) || (rlen > 5 + SIZE_IANA_ID)) {
    errno = EBADMSG;
    return -1;
  }

  // Ignore IANA ID
  // XXX potential buffer overflow (when ver is not big enough to hold
  // <rlen - 3> bytes).
  memcpy(ver, &rbuf[3], rlen - 3);
  return ret;
}

int bic_get_self_test_result(uint8_t slot_id, uint8_t* self_test_result) {
  int ret;
  size_t rlen = sizeof(*self_test_result);

  if (self_test_result == NULL) {
    errno = EINVAL;
    return -1;
  }

  ret = bic_ipmb_wrapper(slot_id, NETFN_APP_REQ, CMD_APP_GET_SELFTEST_RESULTS,
                         NULL, 0, (uint8_t*)self_test_result, &rlen);
  return ret;
}

int bic_me_xmit(uint8_t slot_id, uint8_t* txbuf, size_t txlen, uint8_t* rxbuf,
                size_t* rxlen) {
  uint8_t tbuf[MAX_IPMI_MSG_SIZE] = BIC_IANA_ID; // IANA ID
  uint8_t rbuf[MAX_IPMI_MSG_SIZE] = {0x00};
  size_t rlen = sizeof(rbuf);
  size_t tlen = 0;
  int ret;

  if (txbuf == NULL || rxbuf == NULL || rxlen == NULL) {
    errno = EINVAL;
    return -1;
  }

  // Fill the interface number as ME
  tbuf[3] = BIC_INTF_ME;

  // Fill the data to be sent
  tlen = txlen + 4;
  BIC_ASSERT(tlen < sizeof(tbuf), "txbuf overflow");
  memcpy(&tbuf[4], txbuf, txlen);

  // Send data length includes IANA ID and interface number
  ret = bic_ipmb_wrapper(slot_id, NETFN_OEM_1S_REQ, CMD_OEM_1S_MSG_OUT, tbuf,
                         tlen, rbuf, &rlen);
  if (ret) {
    return -1;
  } else if (rlen < 6 || rbuf[3] != tbuf[3]) {
    // Make sure the received interface number is same
    errno = EBADMSG;
    return -1;
  }

  // Copy the received data to caller skipping header
  // XXX potential buffer overflow (when rxbuf is not big enough to hold
  // *rxlen bytes).
  *rxlen = rlen - 6;
  memcpy(rxbuf, &rbuf[6], *rxlen);
  return 0;
}

/*
 * 0x2E 0xDF: Force Intel ME Recovery
 * Request
 *   Byte 1:3 = Intel Manufacturer ID - 000157h, LS byte first.
 *   Byte 4 - Command
 *     = 01h Restart using Recovery Firmware
 *     (Intel ME FW configuration is not restored to factory defaults)
 *     = 02h Restore Factory Default Variable values and restart the Intel ME FW
 *     = 03h PTT Initial State Restore
 *
 * Response
 *   Byte 1 - Completion Code
 *     = 00h - Success
 *     (Remaining standard Completion Codes are shown in Section 2.12)
 *     = 81h - Unsupported Command parameter value in the Byte 4 of the request.
 *
 *   Byte 2:4 = Intel Manufacturer ID - 000157h, LS byte first.
 */
int me_recovery(uint8_t slot_id, uint8_t command) {
  uint8_t tbuf[256] = {0x00};
  uint8_t rbuf[256] = {0x00};
  size_t tlen = 0;
  size_t rlen = 0;
  int ret = 0;
  int retry = 0;

  while (retry <= 3) {
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
    } else {
      break;
    }
  }
  if (retry == 4) { /* if the third retry still failed, return -1 */
    OBMC_CRIT("%s: Restart using Recovery Firmware failed..., retried: %d",
              __func__,  retry);
    return -1;
  }

  sleep(10);
  retry = 0;
  memset(&tbuf, 0, sizeof(tbuf));
  memset(&rbuf, 0, sizeof(rbuf));

  /*
  * 0x6 0x4: Get Self-Test Results
  * Byte 1 - Completion Code
  * Byte 2
  *   = 55h - No error. All Self-Tests Passed.
  *   = 81h - Firmware entered Recovery bootloader mode
  * Byte 3 For byte 2 = 55h, 56h, FFh:
  *   =00h
  *   =02h - recovery mode entered by IPMI command "Force ME Recovery"
  *
  * Using ME self-test result to check if the ME Recovery Command Success or not
  */
  while (retry <= 3) {
    tbuf[0] = 0x18;
    tbuf[1] = 0x04;
    tlen = 2;
    ret = bic_me_xmit(slot_id, tbuf, tlen, rbuf, &rlen);
    if (ret) {
      retry++;
      sleep(1);
      continue;
    }

    /*
     * If Get Self-Test Results is 0x55 0x00,
     * means No error, all Self-Tests Passed.
     *
     * If Get Self-Test Results is 0x81 0x02,
     * means Firmware entered Recovery bootloader mode.
     */
    if ((command == RECOVERY_MODE) && (rbuf[1] == 0x81) && (rbuf[2] == 0x02)) {
      return 0;
    } else if ((command == RESTORE_FACTORY_DEFAULT) &&
               (rbuf[1] == 0x55) && (rbuf[2] == 0x00)) {
      return 0;
    } else {
      return -1;
    }
  }
  if (retry == 4) { /* if the third retry still failed, return -1 */
    OBMC_CRIT("%s: Restore Factory Default failed..., retried: %d",
              __func__,  retry);
    return -1;
  }

  return 0;
}
