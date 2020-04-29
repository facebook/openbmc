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
#include <time.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <sys/types.h>
#include "bic.h"
#include <openbmc/kv.h>
#include <openbmc/obmc-i2c.h>
#include <openbmc/misc-utils.h>

#define FRUID_READ_COUNT_MAX 0x20
#define FRUID_WRITE_COUNT_MAX 0x20
#define FRUID_SIZE 256
#define IPMB_READ_COUNT_MAX 224
#define IPMB_WRITE_COUNT_MAX 224
#define BIOS_ERASE_PKT_SIZE (64*1024)
#define BIOS_VERIFY_PKT_SIZE (32*1024)
#define SDR_READ_COUNT_MAX 0x1A
#define SIZE_SYS_GUID 16
#define SIZE_IANA_ID 3

#define BIOS_VER_REGION_SIZE (4*1024*1024)
#define BIOS_VER_STR "F09_"
#define Y2_BIOS_VER_STR "YMV2"
#define GPV2_BIOS_VER_STR "F09B"
#define ND_BIOS_VER_STR "F09C"

#define BIC_SIGN_SIZE 32
#define BIC_UPDATE_RETRIES 12
#define BIC_UPDATE_TIMEOUT 500
#define GPV2_BIC_PLAT_STR "F09B"
#define ND_BIC_PLAT_STR "ND"

#define BIC_FLASH_START 0x8000
#define BIC_PKT_MAX 252

#define BIC_CMD_DOWNLOAD 0x21
#define BIC_CMD_RUN 0x22
#define BIC_CMD_STATUS 0x23
#define BIC_CMD_DATA 0x24

#define CMD_DOWNLOAD_SIZE 11
#define CMD_RUN_SIZE 7
#define CMD_STATUS_SIZE 3
#define CMD_DATA_SIZE 0xFF

#define GPIO_VAL "/sys/class/gpio/gpio%d/value"

#define IMC_VER_SIZE 8

#define SLOT_FILE "/tmp/slot%d.bin"
#define SLOT_RECORD_FILE "/tmp/slot%d.rc"
#define SERVER_TYPE_FILE "/tmp/server_type%d.bin"

#define RC_BIOS_SIG_OFFSET 0x3F00000
#define RC_BIOS_IMAGE_SIZE (64*1024*1024)

#define MAX_FW_PCIE_SWITCH_BLOCK_SIZE 1008
#define LAST_FW_PCIE_SWITCH_TRUNK_SIZE (1008%224)

#define MAX_POST_CODE_PAGE 17

#define BRCM_UPDATE_STATUS 128
#define BRCM_WRITE_CMD 130
#define BRCM_READ_CMD 131

#define VSI_UPDATE_STATUS 133
#define VSI_FW_TYPE 135
#define VSI_WRITE_CMD 136

#define PCIE_PHY_FW 0x00
#define BOOT_ROM_PATCH_FW 0x01
#define UNKOWN_FW -1

#define PCIE_SW_MAX_RETRY 50

#pragma pack(push, 1)
typedef struct _sdr_rec_hdr_t {
  uint16_t rec_id;
  uint8_t ver;
  uint8_t type;
  uint8_t len;
} sdr_rec_hdr_t;
#pragma pack(pop)

const static uint8_t gpio_bic_ready[] = { 0, GPIO_I2C_SLOT1_ALERT_N, GPIO_I2C_SLOT2_ALERT_N, GPIO_I2C_SLOT3_ALERT_N, GPIO_I2C_SLOT4_ALERT_N };
const static uint8_t gpio_12v[] = { 0, GPIO_P12V_STBY_SLOT1_EN, GPIO_P12V_STBY_SLOT2_EN, GPIO_P12V_STBY_SLOT3_EN, GPIO_P12V_STBY_SLOT4_EN };
const static uint8_t gpio_power_en[] = { 0, GPIO_SLOT1_POWER_EN, GPIO_SLOT2_POWER_EN, GPIO_SLOT3_POWER_EN, GPIO_SLOT4_POWER_EN };

// Helper Functions
static void
msleep(int msec) {
  struct timespec req;

  req.tv_sec = 0;
  req.tv_nsec = msec * 1000 * 1000;

  while(nanosleep(&req, &req) == -1 && errno == EINTR) {
    continue;
  }
}

static int
i2c_open(uint8_t bus_id) {
  int fd;
  char fn[32];
  int rc;

  snprintf(fn, sizeof(fn), "/dev/i2c-%d", bus_id);
  fd = open(fn, O_RDWR);
  if (fd == -1) {
    syslog(LOG_ERR, "Failed to open i2c device %s", fn);
    return -1;
  }

  rc = ioctl(fd, I2C_SLAVE, BRIDGE_SLAVE_ADDR);
  if (rc < 0) {
    syslog(LOG_ERR, "Failed to open slave @ address 0x%x", BRIDGE_SLAVE_ADDR);
    close(fd);
  }

  return fd;
}

static int
i2c_io(int fd, uint8_t *tbuf, uint8_t tcount, uint8_t *rbuf, uint8_t rcount) {
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

  rc = ioctl(fd, I2C_RDWR, &data);
  if (rc < 0) {
    syslog(LOG_ERR, "Failed to do raw io");
    return -1;
  }

  return 0;
}

static int
write_device(const char *device, const char *value) {
  FILE *fp;
  int rc;

  fp = fopen(device, "w");
  if (!fp) {
    int err = errno;
#ifdef DEBUG
    syslog(LOG_INFO, "failed to open device for write %s", device);
#endif
    return err;
  }

  rc = fputs(value, fp);
  fclose(fp);

  if (rc < 0) {
#ifdef DEBUG
    syslog(LOG_INFO, "failed to write device %s", device);
#endif
    return ENOENT;
  } else {
    return 0;
  }
}

static int
read_device(const char *device, int *value) {
  FILE *fp;
  int rc;

  fp = fopen(device, "r");
  if (!fp) {
    int err = errno;
#ifdef DEBUG
    syslog(LOG_INFO, "failed to open device %s", device);
#endif
    return err;
  }

  rc = fscanf(fp, "%d", value);
  fclose(fp);
  if (rc != 1) {
#ifdef DEBUG
    syslog(LOG_INFO, "failed to read device %s", device);
#endif
    return ENOENT;
  } else {
    return 0;
  }
}

uint8_t
is_bic_ready(uint8_t slot_id) {
  int val;
  char path[64] = {0};

  if (slot_id < 1 || slot_id > 4) {
    return 0;
  }

  sprintf(path, GPIO_VAL, gpio_bic_ready[slot_id]);
  if (read_device(path, &val)) {
    return 0;
  }

  if (val == 0x0) {
    return 1;
  } else {
    return 0;
  }
}

int
bic_is_slot_12v_on(uint8_t slot_id) {
  int val;
  char path[64] = {0};

  if (slot_id < 1 || slot_id > 4) {
    return 0;
  }

  sprintf(path, GPIO_VAL, gpio_12v[slot_id]);
  if (read_device(path, &val)) {
    return 0;
  }

  if (val == 0x0) {
    return 0;
  } else {
    return 1;
  }
}

int bic_set_slot_12v(uint8_t slot_id, uint8_t status)
{
  char path[64] = {0};
  const char *val = status ? "1" : "0";

  if (slot_id < 1 || slot_id > 4) {
    return -1;
  }
  sprintf(path, GPIO_VAL, gpio_12v[slot_id]);
  if (write_device(path, val)) {
    return -1;
  }
  return 0;
}

int
bic_is_slot_power_en(uint8_t slot_id) {
  int val;
  char path[64] = {0};

  if (slot_id < 1 || slot_id > 4) {
    return 0;
  }

  sprintf(path, GPIO_VAL, gpio_power_en[slot_id]);
  if (read_device(path, &val)) {
    return 0;
  }

  if (val == 0x0) {
    return 0;
  } else {
    return 1;
  }
}


// Common IPMB Wrapper function

static int
get_ipmb_bus_id(uint8_t slot_id) {
  int bus_id;

  switch(slot_id) {
  case FRU_SLOT1:
    bus_id = IPMB_BUS_SLOT1;
    break;
  case FRU_SLOT2:
    bus_id = IPMB_BUS_SLOT2;
    break;
  case FRU_SLOT3:
    bus_id = IPMB_BUS_SLOT3;
    break;
  case FRU_SLOT4:
    bus_id = IPMB_BUS_SLOT4;
    break;
  default:
    bus_id = -1;
    break;
  }

  return bus_id;
}

int
bic_ipmb_wrapper(uint8_t slot_id, uint8_t netfn, uint8_t cmd,
                  uint8_t *txbuf, uint16_t txlen,
                  uint8_t *rxbuf, uint8_t *rxlen) {
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

  if (!is_bic_ready(slot_id)) {
    return -1;
  }

  ret = get_ipmb_bus_id(slot_id);
  if (ret < 0) {
#ifdef DEBUG
    syslog(LOG_ERR, "bic_ipmb_wrapper: Wrong Slot ID %d\n", slot_id);
#endif
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

  while(retry < 3) {
    // Invoke IPMB library handler
    int ret = lib_ipmb_handle(bus_id, tbuf, tlen, rbuf, &rlen);
    if (ret != 0) {
      syslog(LOG_ERR, "%s: Failed to send.\n", __func__);
    }

    if (rlen == 0) {
      if (!is_bic_ready(slot_id)) {
        syslog(LOG_ERR, "%s: BIC is busy.\n", __func__);
        break;
      }

      retry++;
      msleep(20);
    }
    else {
      break;
    }
  }

  if (rlen == 0) {
#ifdef DEBUG
    syslog(LOG_DEBUG, "bic_ipmb_wrapper: Zero bytes received, retry:%d\n", retry);
#endif
    return -1;
  }

  // Handle IPMB response
  res  = (ipmb_res_t*) rbuf;

  if (res->cc) {
#ifdef DEBUG
    syslog(LOG_ERR, "bic_ipmb_wrapper: Completion Code: 0x%X\n", res->cc);
#endif
    if (res->cc == CC_BIC_RETRY) { // Completion Code for BIC retry
      syslog(LOG_ERR, "bic_ipmb_wrapper: Completion Code: 0x%X for BIC retry\n", res->cc);
      return BIC_RETRY_ACTION;
    }
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

// Get Self-Test result
int
bic_get_self_test_result(uint8_t slot_id, uint8_t *self_test_result) {
  int ret;
  uint8_t rlen = 0;

  ret = bic_ipmb_wrapper(slot_id, NETFN_APP_REQ, CMD_APP_GET_SELFTEST_RESULTS, NULL, 0, (uint8_t *) self_test_result, &rlen);

  return ret;
}

// Get Device ID
int
bic_get_dev_id(uint8_t slot_id, ipmi_dev_id_t *dev_id) {
  uint8_t rlen = 0;
  int ret;

  ret = bic_ipmb_wrapper(slot_id, NETFN_APP_REQ, CMD_APP_GET_DEVICE_ID, NULL, 0, (uint8_t *) dev_id, &rlen);

  return ret;
}

int
bic_get_dev_power_status(uint8_t slot_id, uint8_t dev_id, uint8_t *nvme_ready, uint8_t *status, uint8_t *ffi, uint8_t *meff, uint16_t *vendor_id, uint8_t *major_ver, uint8_t *minor_ver) {
  uint8_t tbuf[5] = {0x15, 0xA0, 0x00}; // IANA ID
  uint8_t rbuf[11] = {0x00};
  uint8_t rlen = 0;
  int ret;

  tbuf[3] = 0x3;  //get power status
  tbuf[4] = dev_id;

  ret = bic_ipmb_wrapper(slot_id, NETFN_OEM_1S_REQ, CMD_OEM_1S_DEV_POWER, tbuf, 5, rbuf, &rlen);

  // Ignore first 3 bytes of IANA ID
  *status = rbuf[3];
  *nvme_ready = rbuf[4];
  *ffi = rbuf[5];   // FFI_0 0:Storage 1:Accelerator
  *meff = rbuf[6];  // MEFF  0x35: M.2 22110 0xF0: Dual M.2
  *vendor_id = (rbuf[7] << 8 ) | rbuf[8]; // PCIe Vendor ID
  *major_ver = rbuf[9];  //FW version Major Revision
  *minor_ver = rbuf[10]; //FW version Minor Revision

  return ret;
}

int
bic_set_dev_power_status(uint8_t slot_id, uint8_t dev_id, uint8_t status) {
  uint8_t tbuf[5]; // IANA ID
  uint8_t rbuf[4] = {0x00};
  uint8_t rlen = 0;
  int ret;

#if defined(CONFIG_FBY2_GPV2)
  int spb_type = 0;
  spb_type = fby2_common_get_spb_type();
  if (spb_type == TYPE_SPB_YV250) {
    tbuf[0] = 0x9C;  // WiWynn IANA
    tbuf[1] = 0x9C;
    tbuf[2] = 0x00;
  }
  else
#endif
  {
    tbuf[0] = 0x15;  // FB IANA
    tbuf[1] = 0xA0;
    tbuf[2] = 0x00;
  }

  tbuf[3] = status;  //set power status
  tbuf[4] = dev_id;

  ret = bic_ipmb_wrapper(slot_id, NETFN_OEM_1S_REQ, CMD_OEM_1S_DEV_POWER, tbuf, 5, rbuf, &rlen);

  return ret;
}

// Get GPIO value and configuration
int
bic_get_gpio(uint8_t slot_id, bic_gpio_t *gpio) {
  uint8_t tbuf[4] = {0x15, 0xA0, 0x00}; // IANA ID
  uint8_t rbuf[12] = {0x00};
  uint8_t rlen = 0;
  int ret;

  // Ensure the reserved bits are set to 0.
  memset(gpio, 0, sizeof(*gpio));

  ret = bic_ipmb_wrapper(slot_id, NETFN_OEM_1S_REQ, CMD_OEM_1S_GET_GPIO, tbuf, 3, rbuf, &rlen);
  if (ret != 0 || rlen < 3)
    return -1;

  rlen -= 3;
  if (rlen > sizeof(bic_gpio_t))
    rlen = sizeof(bic_gpio_t);

  // Ignore first 3 bytes of IANA ID
  memcpy((uint8_t*) gpio, &rbuf[3], rlen);

  return ret;
}

int
bic_get_gpio_status(uint8_t slot_id, uint8_t pin, uint8_t *status)
{
  bic_gpio_t gpio;
  if (bic_get_gpio(slot_id, &gpio)) {
    return -1;
  }
  *status = (uint8_t)((gpio.gpio >> pin) & 1);
  return 0;
}

int
bic_set_gpio(uint8_t slot_id, uint8_t gpio, uint8_t value) {
  uint8_t tbuf[16] = {0x15, 0xA0, 0x00}; // IANA ID
  uint8_t rbuf[4] = {0x00};
  uint8_t rlen = 0;
  uint64_t pin;
  int ret;

  pin = 1LL << gpio;

  tbuf[3] = pin & 0xFF;
  tbuf[4] = (pin >> 8) & 0xFF;
  tbuf[5] = (pin >> 16) & 0xFF;
  tbuf[6] = (pin >> 24) & 0xFF;
  tbuf[7] = (pin >> 32) & 0xFF;
  tbuf[8] = (pin >> 40) & 0xFF;

  // Fill the value
  if (value) {
    memset(&tbuf[9], 0xFF, 6);
  } else {
    memset(&tbuf[9] , 0x00, 6);
  }

  ret = bic_ipmb_wrapper(slot_id, NETFN_OEM_1S_REQ, CMD_OEM_1S_SET_GPIO, tbuf, 15, rbuf, &rlen);

  return ret;
}

int
bic_set_gpio64(uint8_t slot_id, uint8_t gpio, uint8_t value) {
  uint8_t tbuf[20] = {0x15, 0xA0, 0x00}; // IANA ID
  uint8_t rbuf[4] = {0x00};
  uint8_t rlen = 0;
  uint64_t pin;
  int ret;

  pin = 1LL << gpio;

  tbuf[3] = pin & 0xFF;
  tbuf[4] = (pin >> 8) & 0xFF;
  tbuf[5] = (pin >> 16) & 0xFF;
  tbuf[6] = (pin >> 24) & 0xFF;
  tbuf[7] = (pin >> 32) & 0xFF;
  tbuf[8] = (pin >> 40) & 0xFF;
  tbuf[9] = (pin >> 48) & 0xFF;
  tbuf[10] = (pin >> 56) & 0xFF;

  // Fill the value
  if (value) {
    memset(&tbuf[11], 0xFF, 8);
  } else {
    memset(&tbuf[11], 0x00, 8);
  }

  ret = bic_ipmb_wrapper(slot_id, NETFN_OEM_1S_REQ, CMD_OEM_1S_SET_GPIO, tbuf, 19, rbuf, &rlen);

  return ret;
}

int
bic_get_gpio_config(uint8_t slot_id, uint8_t gpio, bic_gpio_config_t *gpio_config) {
  uint8_t tbuf[12] = {0x15, 0xA0, 0x00}; // IANA ID
  uint8_t rbuf[8] = {0x00};
  uint8_t rlen = 0;
  uint64_t pin;
  int ret;

  pin = 1LL << gpio;

  tbuf[3] = pin & 0xFF;
  tbuf[4] = (pin >> 8) & 0xFF;
  tbuf[5] = (pin >> 16) & 0xFF;
  tbuf[6] = (pin >> 24) & 0xFF;
  tbuf[7] = (pin >> 32) & 0xFF;
  tbuf[8] = (pin >> 40) & 0xFF;

  ret = bic_ipmb_wrapper(slot_id, NETFN_OEM_1S_REQ, CMD_OEM_1S_GET_GPIO_CONFIG, tbuf, 9, rbuf, &rlen);

  // Ignore IANA ID
  *(uint8_t *) gpio_config = rbuf[3];

  return ret;
}

int
bic_get_gpio64_config(uint8_t slot_id, uint8_t gpio, bic_gpio_config_t *gpio_config) {
  uint8_t tbuf[12] = {0x15, 0xA0, 0x00}; // IANA ID
  uint8_t rbuf[8] = {0x00};
  uint8_t rlen = 0;
  uint64_t pin;
  int ret;

  pin = 1LL << gpio;

  tbuf[3] = pin & 0xFF;
  tbuf[4] = (pin >> 8) & 0xFF;
  tbuf[5] = (pin >> 16) & 0xFF;
  tbuf[6] = (pin >> 24) & 0xFF;
  tbuf[7] = (pin >> 32) & 0xFF;
  tbuf[8] = (pin >> 40) & 0xFF;
  tbuf[9] = (pin >> 48) & 0xFF;
  tbuf[10] = (pin >> 56) & 0xFF;

  ret = bic_ipmb_wrapper(slot_id, NETFN_OEM_1S_REQ, CMD_OEM_1S_GET_GPIO_CONFIG, tbuf, 11, rbuf, &rlen);

  // Ignore IANA ID
  *(uint8_t *) gpio_config = rbuf[3];

  return ret;
}

int
bic_set_gpio_config(uint8_t slot_id, uint8_t gpio, bic_gpio_config_t *gpio_config) {
  uint8_t tbuf[12] = {0x15, 0xA0, 0x00}; // IANA ID
  uint8_t rbuf[4] = {0x00};
  uint8_t rlen = 0;
  uint64_t pin;
  int ret;

  pin = 1LL << gpio;

  tbuf[3] = pin & 0xFF;
  tbuf[4] = (pin >> 8) & 0xFF;
  tbuf[5] = (pin >> 16) & 0xFF;
  tbuf[6] = (pin >> 24) & 0xFF;
  tbuf[7] = (pin >> 32) & 0xFF;
  tbuf[8] = (pin >> 40) & 0xFF;
  tbuf[9] = (*(uint8_t *) gpio_config) & 0x1F;

  ret = bic_ipmb_wrapper(slot_id, NETFN_OEM_1S_REQ, CMD_OEM_1S_SET_GPIO_CONFIG, tbuf, 10, rbuf, &rlen);

  return ret;
}

int
bic_set_gpio64_config(uint8_t slot_id, uint8_t gpio, bic_gpio_config_t *gpio_config) {
  uint8_t tbuf[12] = {0x15, 0xA0, 0x00}; // IANA ID
  uint8_t rbuf[4] = {0x00};
  uint8_t rlen = 0;
  uint64_t pin;
  int ret;

  pin = 1LL << gpio;

  tbuf[3] = pin & 0xFF;
  tbuf[4] = (pin >> 8) & 0xFF;
  tbuf[5] = (pin >> 16) & 0xFF;
  tbuf[6] = (pin >> 24) & 0xFF;
  tbuf[7] = (pin >> 32) & 0xFF;
  tbuf[8] = (pin >> 40) & 0xFF;
  tbuf[9] = (pin >> 48) & 0xFF;
  tbuf[10] = (pin >> 56) & 0xFF;
  tbuf[11] = (*(uint8_t *) gpio_config) & 0x1F;

  ret = bic_ipmb_wrapper(slot_id, NETFN_OEM_1S_REQ, CMD_OEM_1S_SET_GPIO_CONFIG, tbuf, 12, rbuf, &rlen);

  return ret;
}

// Get BIC Configuration
int
bic_get_config(uint8_t slot_id, bic_config_t *cfg) {
  uint8_t tbuf[3] = {0x15, 0xA0, 0x00}; // IANA ID
  uint8_t rbuf[4] = {0x00};
  uint8_t rlen = 0;
  int ret;

  ret = bic_ipmb_wrapper(slot_id, NETFN_OEM_1S_REQ, CMD_OEM_1S_GET_CONFIG,
              tbuf, 0x03, rbuf, &rlen);
  // Ignore IANA ID
  *(uint8_t *) cfg = rbuf[3];

  return ret;
}

// Set BIC Configuration
int
bic_set_config(uint8_t slot_id, bic_config_t *cfg) {
  uint8_t tbuf[4] = {0x15, 0xA0, 0x00}; // IANA ID
  uint8_t rlen = 0;
  uint8_t rbuf[4] = {0};
  int ret;

  tbuf[3] = *(uint8_t *) cfg;

  ret = bic_ipmb_wrapper(slot_id, NETFN_OEM_1S_REQ, CMD_OEM_1S_SET_CONFIG,
              tbuf, 0x04, rbuf, &rlen);
  return ret;
}

// Read POST Buffer
int
bic_get_post_buf(uint8_t slot_id, uint8_t *buf, uint8_t *len) {
  uint8_t tbuf[3] = {0x15, 0xA0, 0x00}; // IANA ID
  uint8_t rbuf[255] = {0x00};
  uint8_t rlen = 0;
  int ret;

  ret = bic_ipmb_wrapper(slot_id, NETFN_OEM_1S_REQ, CMD_OEM_1S_GET_POST_BUF, tbuf, 0x03, rbuf, &rlen);

  //Ignore IANA ID
  memcpy(buf, &rbuf[3], rlen-3);

  *len = rlen - 3;

  return ret;
}

// Read Firwmare Versions of various components
int
bic_get_fw_ver(uint8_t slot_id, uint8_t comp, uint8_t *ver) {
  uint8_t tbuf[4] = {0x15, 0xA0, 0x00}; // IANA ID
  uint8_t rbuf[16] = {0x00};
  uint8_t rlen = 0;
  int ret;

#ifdef CONFIG_FBY2_RC
  uint8_t server_type = 0xFF;

  if (comp == FW_ME) {
    ret = bic_get_server_type(slot_id, &server_type);
    if (ret) {
      syslog(LOG_ERR, "%s, Get server type failed for slot%u", __func__, slot_id);
      return -1;
    }

    switch (server_type) {
      case SERVER_TYPE_RC:
        return get_imc_version(slot_id, ver);
      case SERVER_TYPE_TL:
      default:
        break;
    }
  }
#endif

  // Fill the component for which firmware is requested
  tbuf[3] = comp;

  ret = bic_ipmb_wrapper(slot_id, NETFN_OEM_1S_REQ, CMD_OEM_1S_GET_FW_VER, tbuf, 0x04, rbuf, &rlen);
  // fw version has to be between 1 and 5 bytes based on component
  if (ret || (rlen < 1+SIZE_IANA_ID) || (rlen > 5+SIZE_IANA_ID)) {
#ifdef DEBUG
    syslog(LOG_ERR, "bic_get_fw_ver: ret: %d, rlen: %d\n", ret, rlen);
#endif
    return -1;
  }

  //Ignore IANA ID
  memcpy(ver, &rbuf[3], rlen-3);

  return ret;
}

// Read checksum of various components
int
bic_get_fw_cksum(uint8_t slot_id, uint8_t target, uint32_t offset, uint32_t len, uint8_t *ver) {
  uint8_t tbuf[12] = {0x15, 0xA0, 0x00}; // IANA ID
  uint8_t rbuf[16] = {0x00};
  uint8_t rlen = 0;
  int ret;
  int retries = 3;

  // Fill the component for which firmware is requested
  tbuf[3] = target;

  // Fill the offset
  tbuf[4] = (offset) & 0xFF;
  tbuf[5] = (offset >> 8) & 0xFF;
  tbuf[6] = (offset >> 16) & 0xFF;
  tbuf[7] = (offset >> 24) & 0xFF;

  // Fill the length
  tbuf[8] = (len) & 0xFF;
  tbuf[9] = (len >> 8) & 0xFF;
  tbuf[10] = (len >> 16) & 0xFF;
  tbuf[11] = (len >> 24) & 0xFF;

bic_send:
  ret = bic_ipmb_wrapper(slot_id, NETFN_OEM_1S_REQ, CMD_OEM_1S_GET_FW_CKSUM, tbuf, 12, rbuf, &rlen);
  if ((ret || (rlen != 4+SIZE_IANA_ID)) && (retries--)) {  // checksum has to be 4 bytes
    sleep(1);
    syslog(LOG_ERR, "bic_get_fw_cksum: slot: %d, target %d, offset: %d, ret: %d, rlen: %d\n", slot_id, target, offset, ret, rlen);
    goto bic_send;
  }
  if (ret || (rlen != 4+SIZE_IANA_ID)) {
    return -1;
  }

#ifdef DEBUG
  printf("cksum returns: %x:%x:%x::%x:%x:%x:%x\n", rbuf[0], rbuf[1], rbuf[2], rbuf[3], rbuf[4], rbuf[5], rbuf[6]);
#endif

  //Ignore IANA ID
  memcpy(ver, &rbuf[SIZE_IANA_ID], rlen-SIZE_IANA_ID);

  return ret;
}

// Read Firwmare Versions of various components
static int
_enable_bic_update(uint8_t slot_id) {
  uint8_t tbuf[4] = {0x15, 0xA0, 0x00};
  uint8_t rbuf[16] = {0x00};
  uint8_t rlen = 0;
  int ret;

  // 0x1: Update on I2C Channel, the only available option from BMC
  tbuf[3] = 0x1;

  ret = bic_ipmb_wrapper(slot_id, NETFN_OEM_1S_REQ, CMD_OEM_1S_ENABLE_BIC_UPDATE, tbuf, 4, rbuf, &rlen);

  return ret;
}

// Update firmware for various components
static int
_update_fw(uint8_t slot_id, uint8_t target, uint32_t offset, uint16_t len, uint8_t *buf) {
  uint8_t tbuf[256] = {0x15, 0xA0, 0x00}; // IANA ID
  uint8_t rbuf[16] = {0x00};
  uint8_t tlen = 0;
  uint8_t rlen = 0;
  int ret;
  int retries = 3;

  // Fill the component for which firmware is requested
  tbuf[3] = target;

  tbuf[4] = (offset) & 0xFF;
  tbuf[5] = (offset >> 8) & 0xFF;
  tbuf[6] = (offset >> 16) & 0xFF;
  tbuf[7] = (offset >> 24) & 0xFF;

  tbuf[8] = len & 0xFF;
  tbuf[9] = (len >> 8) & 0xFF;

  memcpy(&tbuf[10], buf, len);

  tlen = len + 10;

bic_send:
  ret = bic_ipmb_wrapper(slot_id, NETFN_OEM_1S_REQ, CMD_OEM_1S_UPDATE_FW, tbuf, tlen, rbuf, &rlen);
  if ((ret) && (retries--)) {
    sleep(1);
    printf("_update_fw: slot: %d, target %d, offset: %d, len: %d retrying..\
           \n",    slot_id, target, offset, len);
    goto bic_send;
  }

  return ret;
}

int
_check_brcm_fw_status(uint8_t slot_id, uint8_t drv_num) {
  uint8_t bus, wbuf[256], rbuf[256];
  int ret = 0;
  int rlen = 4; // read
  bus = (2 + drv_num/2) * 2 + 1;
  wbuf[0] = BRCM_READ_CMD;  // offset 131
  wbuf[1] = 9;
  wbuf[2] = 0x20;
  wbuf[3] = 0x04;
  wbuf[4] = 0x07;
  wbuf[5] = 0x40;
  wbuf[6] = 4;//count;
  ret = bic_master_write_read(slot_id, bus, 0xd4, wbuf, 7, rbuf, rlen);
  if (ret != 0) {
    syslog(LOG_DEBUG,"%s(): bic_master_write_read offset=%d  failed", __func__,wbuf[0]);
  } else {
    syslog(LOG_DEBUG,"%s(): rbuf[0]=0x%X rbuf[1]=0x%X rbuf[2]=0x%X rbuf[3]=0x%X", __func__,rbuf[0],rbuf[1],rbuf[2],rbuf[3]);
    uint32_t result;
    memcpy(&result,&rbuf[0], sizeof(uint32_t));
    syslog(LOG_DEBUG,"%s(): result=0x%X", __func__,result);
  }
  return ret;
}

int
_update_brcm_fw(uint8_t slot_id, uint8_t drv_num, uint8_t target, uint32_t offset, uint16_t count, uint8_t * buf) {
  uint8_t bus, wbuf[256], rbuf[256];
  int ret = 0;
  int rlen = 0; // write
  bus = (2 + drv_num/2) * 2 + 1;
  wbuf[0] = BRCM_WRITE_CMD;  // offset 130
  wbuf[1] = count+5;
  offset += 0x00400000;
  memcpy(&wbuf[2],&offset, sizeof(uint32_t));
  wbuf[6] = 5;
  memcpy(&wbuf[7],buf, sizeof(uint8_t)*count);
  ret = bic_master_write_read(slot_id, bus, 0xd4, wbuf, count+7, rbuf, rlen);
  if (ret != 0) {
    syslog(LOG_DEBUG,"%s(): bic_master_write_read offset=%d  failed", __func__,wbuf[0]);
  }

  return ret;
}

int
_update_vsi_fw(uint8_t slot_id, uint8_t drv_num, uint8_t target, uint32_t offset, uint16_t count, uint8_t * buf) {
  uint8_t bus, wbuf[256], rbuf[256];
  int ret = 0;
  int rlen = 0; // write
  bus = (2 + drv_num/2) * 2 + 1;
  wbuf[0] = VSI_WRITE_CMD;  // offset 136
  memcpy(&wbuf[1],buf, sizeof(uint8_t)*count);
  ret = bic_master_write_read(slot_id, bus, 0xd4, wbuf, count+1, rbuf, rlen);
  if (ret != 0) {
    syslog(LOG_DEBUG,"%s(): bic_master_write_read offset=%d  failed\n", __func__,wbuf[0]);
  }

  return ret;
}

// Update firmware for various components
static int
_update_pcie_sw_fw(uint8_t slot_id, uint8_t target, uint32_t offset, uint16_t len, u_int32_t image_len, uint8_t *buf) {
  uint8_t tbuf[256] = {0x15, 0xA0, 0x00}; // IANA ID
  uint8_t rbuf[16] = {0x00};
  uint8_t tlen = 0;
  uint8_t rlen = 0;
  int ret;
  int retries = 3;

  // Fill the component for which firmware is requested
  tbuf[3] = target;

  tbuf[4] = (offset) & 0xFF;
  tbuf[5] = (offset >> 8) & 0xFF;
  tbuf[6] = (offset >> 16) & 0xFF;
  tbuf[7] = (offset >> 24) & 0xFF;

  tbuf[8] = len & 0xFF;
  tbuf[9] = (len >> 8) & 0xFF;

  tbuf[10] = (image_len) & 0xFF;
  tbuf[11] = (image_len >> 8) & 0xFF;
  tbuf[12] = (image_len >> 16) & 0xFF;
  tbuf[13] = (image_len >> 24) & 0xFF;

  memcpy(&tbuf[14], buf, len);

  tlen = len + 14;

bic_send:
  ret = bic_ipmb_wrapper(slot_id, NETFN_OEM_1S_REQ, CMD_OEM_1S_UPDATE_FW, tbuf, tlen, rbuf, &rlen);
  if ((ret) && (retries--)) {
    sleep(1);
    printf("_update_fw: slot: %d, target %d, offset: %d, len: %d retrying..\
           \n",    slot_id, target, offset, len);
    goto bic_send;
  }

  return ret;
}

// Get PCIE switch update status
static int
_get_pcie_sw_update_status(uint8_t slot_id, uint8_t *status) {
  uint8_t tbuf[4] = {0x15, 0xA0, 0x00};  // IANA ID
  uint8_t rbuf[16] = {0x00};
  uint8_t rlen = 0;
  int ret;
  int retries = 3;

  tbuf[3] = 0x01;

bic_send:
  ret = bic_ipmb_wrapper(slot_id, NETFN_OEM_1S_REQ, CMD_OEM_1S_GET_PCIE_SWITCH_STATUS, tbuf, 4, rbuf, &rlen);
  if ((ret) && (retries--)) {
    sleep(1);
    syslog(LOG_ERR,"_get_pcie_sw_update_status: slot: %d, retrying..\n", slot_id);
    goto bic_send;
  }

  // Ignore IANA ID
  memcpy(status, &rbuf[3], 2);

  return ret;
}

// Reset PCIE switch update status
static int
_reset_pcie_sw_update_status(uint8_t slot_id) {
  uint8_t tbuf[4] = {0x15, 0xA0, 0x00};  // IANA ID
  uint8_t rbuf[16] = {0x00};
  uint8_t rlen = 0;
  int ret;
  int retries = 3;

  tbuf[3] = 0x00;

bic_send:
  ret = bic_ipmb_wrapper(slot_id, NETFN_OEM_1S_REQ, CMD_OEM_1S_GET_PCIE_SWITCH_STATUS, tbuf, 4, rbuf, &rlen);
  if ((ret) && (retries--)) {
    sleep(1);
    printf("_reset_pcie_sw_update_status: slot: %d, retrying..\n", slot_id);
    goto bic_send;
  }

  return ret;
}

// Terminate PCIE switch update
static int
_terminate_pcie_sw_update(uint8_t slot_id) {
  uint8_t tbuf[4] = {0x15, 0xA0, 0x00};  // IANA ID
  uint8_t rbuf[16] = {0x00};
  uint8_t rlen = 0;
  int ret;
  int retries = 3;

  tbuf[3] = 0x02;

bic_send:
  ret = bic_ipmb_wrapper(slot_id, NETFN_OEM_1S_REQ, CMD_OEM_1S_GET_PCIE_SWITCH_STATUS, tbuf, 4, rbuf, &rlen);
  if ((ret) && (retries--)) {
    sleep(1);
    printf("_terminate_pcie_sw_update: slot: %d, retrying..\n", slot_id);
    goto bic_send;
  }

  return ret;
}

// Read firmware for various components
static int
_dump_fw(uint8_t slot_id, uint8_t target, uint32_t offset, uint8_t len, uint8_t *rbuf, uint8_t *rlen) {
  uint8_t tbuf[16] = {0x15, 0xA0, 0x00};  // IANA ID
  int ret;
  int retries = 3;

  // Fill the component for which firmware is requested
  tbuf[3] = target;
  memcpy(&tbuf[4], &offset, sizeof(uint32_t));
  tbuf[8] = len;

  do {
    ret = bic_ipmb_wrapper(slot_id, NETFN_OEM_1S_REQ, CMD_OEM_1S_READ_FW_IMAGE, tbuf, 9, rbuf, rlen);
    if (!ret && (len == (*rlen -= 3)))
      return 0;

    sleep(1);
    printf("_dump_fw: slot: %d, target %d, offset: %d, len: %d retrying..\n", slot_id, target, offset, len);
  } while ((--retries));

  return -1;
}

// Get CPLD update progress
static int
_get_cpld_update_progress(uint8_t slot_id, uint8_t *progress) {
  uint8_t tbuf[4] = {0x15, 0xA0, 0x00};  // IANA ID
  uint8_t rbuf[16] = {0x00};
  uint8_t rlen = 0;
  int ret;
  int retries = 3;

bic_send:
  ret = bic_ipmb_wrapper(slot_id, NETFN_OEM_1S_REQ, CMD_OEM_1S_GET_CPLD_UPDATE_PROGRESS, tbuf, 3, rbuf, &rlen);
  if ((ret) && (retries--)) {
    sleep(1);
    printf("_get_cpld_update_progress: slot: %d, retrying..\n", slot_id);
    goto bic_send;
  }

  // Ignore IANA ID
  memcpy(progress, &rbuf[3], 1);

  return ret;
}

static int
check_bic_image(uint8_t slot_id, int fd, long size) {
  int offs, rcnt;
  uint8_t slot_type;
  uint8_t server_type;
  uint8_t buf[64];
  const uint8_t hdr_common[] = {0x01,0x00,0x4c,0x1c,0x00,0x53,0x4e,0x4f,0x57,0x46,0x4c,0x41,0x4b,0x45};
  const char *plat_str = NULL;

  if (size < BIC_SIGN_SIZE) {
    return -1;
  }

  slot_type = bic_get_slot_type(slot_id);
  switch (slot_type) {
    case SLOT_TYPE_GPV2:
      plat_str = GPV2_BIC_PLAT_STR;
      break;
    case SLOT_TYPE_SERVER:
      bic_get_server_type(slot_id, &server_type);
      if(server_type == SERVER_TYPE_ND)
        plat_str = ND_BIC_PLAT_STR;
      else
        return 0;
      break;
    default:
      return 0;
  }

  offs = size - BIC_SIGN_SIZE;
  if (lseek(fd, offs, SEEK_SET) != (off_t)offs) {
    syslog(LOG_ERR, "%s: lseek to %d failed", __func__, offs);
    return -1;
  }

  offs = 0;
  while (offs < BIC_SIGN_SIZE) {
    rcnt = file_read_bytes(fd, (buf + offs), BIC_SIGN_SIZE);
    if (rcnt <= 0) {
      syslog(LOG_ERR, "%s: unexpected rcnt: %d", __func__, rcnt);
      return -1;
    }
    offs += rcnt;
  }

  if (memcmp(buf, hdr_common, sizeof(hdr_common))) {
    return -1;
  }

  if (memcmp(buf+sizeof(hdr_common), plat_str, strlen(plat_str)+1)) {
    return -1;
  }

  if ((offs = lseek(fd, 0, SEEK_SET))) {
    syslog(LOG_ERR, "%s: fail to init file offset %d, errno=%d", __func__, offs, errno);
    return -1;
  }
  return 0;
}

static int
check_gpv2_rp_image(int *rp_type, int fd, long size) {
  int offs;
  uint8_t buf[4];
  const uint8_t hdr_pcie_phy[] = {0x45,0x49,0x43,0x50}; //EICP
  const uint8_t hdr_rom_patch[] = {0x48,0x54,0x41,0x50}; //HTAP

  if (size < 4)
    return -1;

  if (read(fd, buf, 4) != 4)
    return -1;

  if (memcmp(buf, hdr_pcie_phy, 4) == 0) {
    printf("Update PCIE PHY FW\n");
    syslog(LOG_CRIT, "Update VSI: PCIE PHY FW\n");
    *rp_type = PCIE_PHY_FW;
  } else if (memcmp(buf, hdr_rom_patch, 4) == 0) {
    printf("Update ROM PATCH FW\n");
    syslog(LOG_CRIT, "Update VSI: ROM PATCH FW\n");
    *rp_type = BOOT_ROM_PATCH_FW;
  } else {
    printf("invalid file for RP!\n");
    *rp_type = UNKOWN_FW;
    return -1;
  }

  if ((offs = lseek(fd, 0, SEEK_SET))) {
    syslog(LOG_ERR, "%s: fail to init file offset %d, errno=%d", __func__, offs, errno);
    return -1;
  }
  return 0;
}


static int
_update_bic_main(uint8_t slot_id, char *path, uint8_t force) {
#define MAX_CMD_LEN 100

  int fd;
  int ifd = -1;
  char cmd[MAX_CMD_LEN] = {0};
  struct stat buf;
  int size;
  uint8_t tbuf[256] = {0};
  uint8_t rbuf[16] = {0};
  uint8_t tcount;
  uint8_t rcount;
  volatile int xcount;
  int i = 0;
  int ret = -1, rc;
  uint8_t xbuf[256] = {0};
  uint32_t offset = 0, last_offset = 0, dsize;
  struct rlimit mqlim;
  uint8_t done = 0;

  // Open the file exclusively for read
  fd = open(path, O_RDONLY, 0666);
  if (fd < 0) {
    syslog(LOG_ERR, "bic_update_fw: open fails for path: %s\n", path);
    goto error_exit2;
  }

  fstat(fd, &buf);
  size = buf.st_size;
  printf("size of file is %d bytes\n", size);
  dsize = size/20;

  if (!force && check_bic_image(slot_id, fd, size)) {
    printf("invalid BIC file!\n");
    goto error_exit2;
  }

  syslog(LOG_CRIT, "bic_update_fw: update bic firmware on slot %d\n", slot_id);

  // Open the i2c driver
  ifd = i2c_open(get_ipmb_bus_id(slot_id));
  if (ifd < 0) {
    printf("ifd error\n");
    goto error_exit2;
  }

  // Kill ipmb daemon for this slot
  sprintf(cmd, "sv stop ipmbd_%d", get_ipmb_bus_id(slot_id));
  system(cmd);
  printf("stop ipmbd for slot %x..\n", slot_id);

  // The I2C high speed clock (1M) could cause to read BIC data abnormally.
  // So reduce I2C bus clock speed which is a workaround for BIC update.
  switch(slot_id)
  {
     case FRU_SLOT1:
       system("devmem 0x1e78a084 w 0xFFF77304");
       break;
     case FRU_SLOT2:
       system("devmem 0x1e78a104 w 0xFFF77304");
       break;
     case FRU_SLOT3:
       system("devmem 0x1e78a184 w 0xFFF77304");
       break;
     case FRU_SLOT4:
       system("devmem 0x1e78a304 w 0xFFF77304");
       break;
     default:
       syslog(LOG_CRIT, "bic_update_fw: incorrect slot_id %d\n", slot_id);
       goto error_exit;
       break;
  }
  sleep(1);
  printf("Stopped ipmbd for this slot %x..\n",slot_id);

  if (is_bic_ready(slot_id)) {
    mqlim.rlim_cur = RLIM_INFINITY;
    mqlim.rlim_max = RLIM_INFINITY;
    if (setrlimit(RLIMIT_MSGQUEUE, &mqlim) < 0) {
      goto error_exit;
    }

    // Restart ipmb daemon with "-u|--enable-bic-update" for bic update
    memset(cmd, 0, sizeof(cmd));
    sprintf(cmd, "/usr/local/bin/ipmbd -u %d %d > /dev/null 2>&1 &", get_ipmb_bus_id(slot_id), slot_id);
    system(cmd);
    printf("start ipmbd -u for this slot %x..\n",slot_id);

    sleep(2);

    // Enable Bridge-IC update
    _enable_bic_update(slot_id);

    // Kill ipmb daemon "--enable-bic-update" for this slot
    memset(cmd, 0, sizeof(cmd));
    sprintf(cmd, "ps -w | grep -v 'grep' | grep 'ipmbd -u %d' |awk '{print $1}'| xargs kill", get_ipmb_bus_id(slot_id));
    system(cmd);
    printf("stop ipmbd for slot %x..\n", slot_id);
  }

  // Wait for SMB_BMC_3v3SB_ALRT_N
  for (i = 0; i < BIC_UPDATE_RETRIES; i++) {
    if (!is_bic_ready(slot_id)) {
      printf("bic ready for update after %d tries\n", i);
      break;
    }
    msleep(BIC_UPDATE_TIMEOUT);
  }

  if (i == BIC_UPDATE_RETRIES) {
    printf("bic is NOT ready for update\n");
    syslog(LOG_CRIT, "bic_update_fw: bic is NOT ready for update\n");
    goto error_exit;
  }

  sleep(1);

  // Start Bridge IC update(0x21)

  tbuf[0] = CMD_DOWNLOAD_SIZE;
  tbuf[1] = 0x00; //Checksum, will fill later

  tbuf[2] = BIC_CMD_DOWNLOAD;

  // update flash address: 0x8000
  tbuf[3] = (BIC_FLASH_START >> 24) & 0xff;
  tbuf[4] = (BIC_FLASH_START >> 16) & 0xff;
  tbuf[5] = (BIC_FLASH_START >> 8) & 0xff;
  tbuf[6] = (BIC_FLASH_START) & 0xff;

  // image size
  tbuf[7] = (size >> 24) & 0xff;
  tbuf[8] = (size >> 16) & 0xff;
  tbuf[9] = (size >> 8) & 0xff;
  tbuf[10] = (size) & 0xff;

  // calcualte checksum for data portion
  for (i = 2; i < CMD_DOWNLOAD_SIZE; i++) {
    tbuf[1] += tbuf[i];
  }

  tcount = CMD_DOWNLOAD_SIZE;

  rcount = 0;

  rc = i2c_io(ifd, tbuf, tcount, rbuf, rcount);
  if (rc) {
    printf("i2c_io failed download\n");
    goto error_exit;
  }

  //delay for download command process ---
  msleep(500);
  tcount = 0;
  rcount = 2;
  rc = i2c_io(ifd, tbuf, tcount, rbuf, rcount);
  if (rc) {
    printf("i2c_io failed download ack\n");
    goto error_exit;
  }

  if (rbuf[0] != 0x00 || rbuf[1] != 0xcc) {
    printf("download response: %x:%x\n", rbuf[0], rbuf[1]);
    goto error_exit;
  }

  // Loop to send all the image data
  while (1) {
    // Get Status
    tbuf[0] = CMD_STATUS_SIZE;
    tbuf[1] = BIC_CMD_STATUS;// checksum same as data
    tbuf[2] = BIC_CMD_STATUS;

    tcount = CMD_STATUS_SIZE;
    rcount = 0;

    rc = i2c_io(ifd, tbuf, tcount, rbuf, rcount);
    if (rc) {
      printf("i2c_io failed get status\n");
      goto error_exit;
    }

    tcount = 0;
    rcount = 5;

    rc = i2c_io(ifd, tbuf, tcount, rbuf, rcount);
    if (rc) {
      printf("i2c_io failed get status ack\n");
      goto error_exit;
    }

    if (rbuf[0] != 0x00 ||
        rbuf[1] != 0xcc ||
        rbuf[2] != 0x03 ||
        rbuf[3] != 0x40 ||
        rbuf[4] != 0x40) {
      printf("status resp: %x:%x:%x:%x:%x", rbuf[0], rbuf[1], rbuf[2], rbuf[3], rbuf[4]);
      goto error_exit;
    }

    // Send ACK ---
    tbuf[0] = 0xcc;
    tcount = 1;
    rcount = 0;
    rc = i2c_io(ifd, tbuf, tcount, rbuf, rcount);
    if (rc) {
      printf("i2c_io failed send ack\n");
      goto error_exit;
    }

    // send next packet

    xcount = read(fd, xbuf, BIC_PKT_MAX);
    if (xcount <= 0) {
#ifdef DEBUG
      printf("read returns %d\n", xcount);
#endif
      break;
    }
#ifdef DEBUG
    printf("read returns %d bytes\n", xcount);
#endif

    offset += xcount;
    if((last_offset + dsize) <= offset) {
       printf("updated bic: %d %%\n", offset/dsize*5);
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

    rc = i2c_io(ifd, tbuf, tcount, rbuf, rcount);
    if (rc) {
      printf("i2c_io error send data\n");
      goto error_exit;
    }

    msleep(10);
    tcount = 0;
    rcount = 2;

    rc = i2c_io(ifd, tbuf, tcount, rbuf, rcount);
    if (rc) {
      printf("i2c_io error send data ack\n");
      goto error_exit;
    }

    if (rbuf[0] != 0x00 || rbuf[1] != 0xcc) {
      printf("data error: %x:%x\n", rbuf[0], rbuf[1]);
      goto error_exit;
    }
  }
  msleep(500);

  // Run the new image
  tbuf[0] = CMD_RUN_SIZE;
  tbuf[1] = 0x00; //checksum
  tbuf[2] = BIC_CMD_RUN;
  tbuf[3] = (BIC_FLASH_START >> 24) & 0xff;
  tbuf[4] = (BIC_FLASH_START >> 16) & 0xff;
  tbuf[5] = (BIC_FLASH_START >> 8) & 0xff;
  tbuf[6] = (BIC_FLASH_START) & 0xff;

  for (i = 2; i < CMD_RUN_SIZE; i++) {
    tbuf[1] += tbuf[i];
  }

  tcount = CMD_RUN_SIZE;
  rcount = 0;

  rc = i2c_io(ifd, tbuf, tcount, rbuf, rcount);
  if (rc) {
    printf("i2c_io failed run\n");
    goto error_exit;
  }

  tcount = 0;
  rcount = 2;

  rc = i2c_io(ifd, tbuf, tcount, rbuf, rcount);
  if (rc) {
    printf("i2c_io failed run ack\n");
    goto error_exit;
  }

  if (rbuf[0] != 0x00 || rbuf[1] != 0xcc) {
    printf("run response: %x:%x\n", rbuf[0], rbuf[1]);
    goto error_exit;
  }
  msleep(500);

  // Wait for SMB_BMC_3v3SB_ALRT_N
  for (i = 0; i < BIC_UPDATE_RETRIES; i++) {
    if (is_bic_ready(slot_id))
      break;

    msleep(BIC_UPDATE_TIMEOUT);
  }
  if (i == BIC_UPDATE_RETRIES) {
    printf("bic is NOT ready\n");
    goto error_exit;
  }

  ret = 0;
  done = 1;

error_exit:
  // Restore the I2C bus clock to 1M.
  switch(slot_id)
  {
     case FRU_SLOT1:
       system("devmem 0x1e78a084 w 0xFFF5E700");
       break;
     case FRU_SLOT2:
       system("devmem 0x1e78a104 w 0xFFF5E700");
       break;
     case FRU_SLOT3:
       system("devmem 0x1e78a184 w 0xFFF5E700");
       break;
     case FRU_SLOT4:
       system("devmem 0x1e78a304 w 0xFFF5E700");
       break;
     default:
       syslog(LOG_ERR, "bic_update_fw: incorrect slot_id %d\n", slot_id);
       break;
  }

  // Restart ipmbd daemon
  sleep(1);
  memset(cmd, 0, sizeof(cmd));
  sprintf(cmd, "sv start ipmbd_%d", get_ipmb_bus_id(slot_id));
  system(cmd);

error_exit2:
  syslog(LOG_CRIT, "bic_update_fw: updating bic firmware is exiting on slot %d\n", slot_id);
  if (fd > 0) {
    close(fd);
  }

  if (ifd > 0) {
     close(ifd);
  }

  // do not get sdr during sdr update
  bic_set_sdr_threshold_update_flag(slot_id, 0);
  if (done == 1) {    //update successfully
    memset(cmd, 0, sizeof(cmd));
    snprintf(cmd, MAX_CMD_LEN, "(/usr/local/bin/bic-cached %d; echo 1 >/tmp/cache_store/slot%d_sdr_thresh_update) &", slot_id, slot_id);   //retrieve SDR data after BIC FW update
    system(cmd);
  }

  return ret;
}

static int
check_vr_image(uint8_t slot_id, int fd, long size) {
  uint8_t buf[32];
  uint8_t hdr_tl[] = {0x00,0x01,0x4c,0x1c,0x00,0x46,0x30,0x39};
  uint8_t *hdr = hdr_tl, hdr_size = sizeof(hdr_tl);
  int offs;
#if defined(CONFIG_FBY2_EP) || defined(CONFIG_FBY2_RC)
  int ret;
  uint8_t server_type = 0xFF;
  uint8_t hdr_ep[] = {0x00,0x01,0x4c,0x1c,0x00,0x46,0x30,0x39,0x41};

  ret = bic_get_server_type(slot_id, &server_type);
  if (ret) {
    syslog(LOG_ERR, "%s, Get server type failed\n", __func__);
    return -1;
  }

  switch (server_type) {
    case SERVER_TYPE_RC:
      return 0;
    case SERVER_TYPE_EP:
      hdr = hdr_ep;
      hdr_size = sizeof(hdr_ep);
      break;
  }
#endif

  if (size < 16)
    return -1;

  offs = 1;
  if (lseek(fd, offs, SEEK_SET) != (off_t)offs) {
    syslog(LOG_ERR, "%s: lseek to %d failed", __func__, offs);
    return -1;
  }

  if (read(fd, buf, hdr_size) != hdr_size)
    return -1;

  if (memcmp(buf, hdr, hdr_size))
    return -1;

  if ((offs = lseek(fd, 0, SEEK_SET))) {
    syslog(LOG_ERR, "%s: fail to init file offset %d, errno=%d", __func__, offs, errno);
    return -1;
  }
  return 0;
}

static int
check_cpld_image(uint8_t slot_id, int fd, long size) {
  uint8_t buf[32];
  uint8_t hdr_tl[] = {0x01,0x00,0x4c,0x1c,0x00,0x01,0x2b,0xb0,0x43,0x46,0x30,0x39};
  uint8_t *hdr = hdr_tl, hdr_size = sizeof(hdr_tl);
  int offs;
#if defined(CONFIG_FBY2_RC) || defined(CONFIG_FBY2_EP) || defined(CONFIG_FBY2_ND)
  int ret;
  uint8_t server_type = 0xFF;
  uint8_t hdr_ep[] = {0x01,0x00,0x4c,0x1c,0x00,0xe1,0x2b,0xc0,0x43,0x46,0x30,0x39,0x41};
  uint8_t hdr_nd[] = {0x01,0x00,0x4c,0x1c,0x00,0x01,0x2b,0xb0,0x43,0x46,0x30,0x39,0x43};

  ret = bic_get_server_type(slot_id, &server_type);
  if (ret) {
    syslog(LOG_ERR, "%s, Get server type failed\n", __func__);
    return -1;
  }

  switch (server_type) {
    case SERVER_TYPE_RC:
      return 0;
    case SERVER_TYPE_EP:
      hdr = hdr_ep;
      hdr_size = sizeof(hdr_ep);
      break;
    case SERVER_TYPE_ND:
      hdr = hdr_nd;
      hdr_size = sizeof(hdr_nd);
      break;
  }
#endif

  if (size < 32)
    return -1;

  if (read(fd, buf, hdr_size) != hdr_size)
    return -1;

  if (memcmp(buf, hdr, hdr_size))
    return -1;

  if ((offs = lseek(fd, 0, SEEK_SET))) {
    syslog(LOG_ERR, "%s: fail to init file offset %d, errno=%d", __func__, offs, errno);
    return -1;
  }
  return 0;
}

#if defined(CONFIG_FBY2_RC)
static int
check_bios_image_rc(uint8_t slot_id, int fd, long size) {

  uint8_t sig_rc[] = { 0x52, 0x43, 0x5f, 0x55, 0x45, 0x46, 0x49 };
  uint8_t buf[16] = {0};
  uint8_t sig_size = sizeof(sig_rc);

  if (size < RC_BIOS_IMAGE_SIZE)
    return -1;

  lseek(fd, RC_BIOS_SIG_OFFSET, SEEK_SET);

  if(read(fd, buf, sig_size) != sig_size)
    return -1;

  if (memcmp(buf, sig_rc, sig_size))
    return -1;

  lseek(fd, 0, SEEK_SET);
  return 0;
}
#endif

static int
check_bios_image(uint8_t slot_id, int fd, long size) {
  int offs, rcnt, end;
  uint8_t *buf;
  uint8_t ver_sig[] = { 0x46, 0x49, 0x44, 0x04, 0x78, 0x00 };
#if defined(CONFIG_FBY2_EP) || defined(CONFIG_FBY2_RC)
  int ret;
  uint8_t server_type = 0xFF;

  ret = bic_get_server_type(slot_id, &server_type);
  if (ret) {
    syslog(LOG_ERR, "%s, Get server type failed\n", __func__);
    return -1;
  }

  switch (server_type) {
    case SERVER_TYPE_EP:
      return 0;
#if defined(CONFIG_FBY2_RC)
    case SERVER_TYPE_RC:
      ret = check_bios_image_rc(slot_id, fd, size);
      return ret;
#endif
  }
#endif

  if (size < BIOS_VER_REGION_SIZE)
    return -1;

  buf = (uint8_t *)malloc(BIOS_VER_REGION_SIZE);
  if (!buf) {
    return -1;
  }

  offs = size - BIOS_VER_REGION_SIZE;
  if (lseek(fd, offs, SEEK_SET) != (off_t)offs) {
    syslog(LOG_ERR, "%s: lseek to %d failed", __func__, offs);
    return -1;
  }

  offs = 0;
  while (offs < BIOS_VER_REGION_SIZE) {
    rcnt = read(fd, (buf + offs), BIOS_ERASE_PKT_SIZE);
    if (rcnt <= 0) {
      if (errno == EINTR) {
        continue;
      }
      syslog(LOG_ERR, "check_bios_image: unexpected rcnt: %d", rcnt);
      free(buf);
      return -1;
    }
    offs += rcnt;
  }

  end = BIOS_VER_REGION_SIZE - (sizeof(ver_sig) + strlen(BIOS_VER_STR));
  for (offs = 0; offs < end; offs++) {
    if (!memcmp(buf+offs, ver_sig, sizeof(ver_sig))) {
#if defined(CONFIG_FBY2_ND)
      if (memcmp(
              buf + offs + sizeof(ver_sig),
              ND_BIOS_VER_STR,
              strlen(ND_BIOS_VER_STR))) {
        offs = end;
      }
      break;
#endif
      if (memcmp(
              buf + offs + sizeof(ver_sig),
              BIOS_VER_STR,
              strlen(BIOS_VER_STR)) &&
          memcmp(
              buf + offs + sizeof(ver_sig),
              Y2_BIOS_VER_STR,
              strlen(Y2_BIOS_VER_STR)) &&
          memcmp(
              buf + offs + sizeof(ver_sig),
              GPV2_BIOS_VER_STR,
              strlen(GPV2_BIOS_VER_STR))) {
        offs = end;
      }
      break;
    }
  }
  free(buf);

  if (offs >= end)
    return -1;

  if ((offs = lseek(fd, 0, SEEK_SET))) {
    syslog(LOG_ERR, "%s: fail to init file offset %d, errno=%d", __func__, offs, errno);
    return -1;
  }
  return 0;
}

static int
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

static int
verify_bios_image(uint8_t slot_id, int fd, long size) {
  int ret = -1;
  int rc, i;
  uint32_t offset;
  uint32_t tcksum, gcksum;
  volatile uint16_t count;
  uint8_t target, last_pkt = 0x00;
  uint8_t *tbuf = NULL;
#if defined(CONFIG_FBY2_EP)
  uint8_t server_type = 0xFF;

  rc = bic_get_server_type(slot_id, &server_type);
  if (rc) {
    syslog(LOG_ERR, "%s, Get server type failed\n", __func__);
    return -1;
  }

  switch (server_type) {
    case SERVER_TYPE_EP:
      last_pkt = 0x80;
      break;
  }
#endif

  // Checksum calculation for BIOS image
  tbuf = malloc(BIOS_VERIFY_PKT_SIZE * sizeof(uint8_t));
  if (!tbuf) {
    return -1;
  }

  if ((offset = lseek(fd, 0, SEEK_SET))) {
    syslog(LOG_ERR, "%s: fail to init file offset %d, errno=%d", __func__, offset, errno);
    return -1;
  }
  while (1) {
    count = read(fd, tbuf, BIOS_VERIFY_PKT_SIZE);
    if (count <= 0) {
      if (offset >= size) {
        ret = 0;
      }
      break;
    }

    tcksum = 0;
    for (i = 0; i < count; i++) {
      tcksum += tbuf[i];
    }

    target = ((offset + count) >= size) ? (UPDATE_BIOS | last_pkt) : UPDATE_BIOS;

    // Get the checksum of binary image
    rc = bic_get_fw_cksum(slot_id, target, offset, count, (uint8_t*)&gcksum);
    if (rc) {
      printf("get checksum failed, offset:0x%x\n", offset);
      break;
    }

    // Compare both and see if they match or not
    if (gcksum != tcksum) {
      printf("checksum does not match, offset:0x%x, 0x%x:0x%x\n", offset, tcksum, gcksum);
      break;
    }

    offset += count;
  }

  free(tbuf);
  return ret;
}

int
bic_dump_fw(uint8_t slot_id, uint8_t comp, char *path) {
  int ret = -1, rc, fd;
  uint32_t offset, next_doffset;
  uint32_t img_size = 0x2000000, dsize;
  uint8_t count, read_count;
  uint8_t buf[256], rlen;

  if (comp != DUMP_BIOS) {
    printf("ERROR: only support dump BIOS image!\n");
    return ret;
  }
  printf("dumping fw on slot %d:\n", slot_id);

  fd = open(path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
  if (fd < 0) {
    printf("ERROR: invalid file path!\n");
#ifdef DEBUG
    syslog(LOG_ERR, "bic_dump_fw: open fails for path: %s\n", path);
#endif
    goto error_exit;
  }

  // Write chunks of binary data in a loop
  dsize = img_size / 100;
  offset = 0;
  next_doffset = offset + dsize;
  while (1) {
    read_count = ((offset + IPMB_READ_COUNT_MAX) <= img_size) ? IPMB_READ_COUNT_MAX : (img_size - offset);

    // read image from Bridge-IC
    rc = _dump_fw(slot_id, comp, offset, read_count, buf, &rlen);
    if (rc) {
      goto error_exit;
    }

    // Write to file
    count = write(fd, &buf[3], rlen);
    if (count <= 0) {
      goto error_exit;
    }

    // Update counter
    offset += count;
    if (offset >= next_doffset) {
      switch (comp) {
        case DUMP_BIOS:
          _set_fw_update_ongoing(slot_id, 60);
          printf("\rdumped bios: %d %%", offset/dsize);
          break;
      }
      fflush(stdout);
      next_doffset += dsize;
    }

    if (offset >= img_size)
      break;
  }

  if (comp == DUMP_BIOS) {
    _set_fw_update_ongoing(slot_id, 60 * 2);
  }

  ret = 0;

error_exit:
  printf("\n");
  if (fd > 0 ) {
    close(fd);
  }

  return ret;
}

int
bic_update_dev_firmware(uint8_t slot_id, uint8_t dev_id, uint8_t comp, char *path, uint8_t force) {
  int ret = -1;
  uint32_t offset;
  volatile uint16_t count, read_count;
  uint8_t buf[256] = {0};
  int fd;
  int rp_fw_type =  UNKOWN_FW;

  printf("updating fw on slot %d device %d:\n", slot_id,dev_id);

  uint32_t dsize, last_offset;
  struct stat st;
  // Open the file exclusively for read
  fd = open(path, O_RDONLY, 0666);
  if (fd < 0) {
    printf("ERROR: invalid file path!\n");
#ifdef DEBUG
    syslog(LOG_ERR, "bic_update_fw: open fails for path: %s\n", path);
#endif
    goto error_exit;
  }

  stat(path, &st);
  if (comp == UPDATE_BRCM) {
    syslog(LOG_CRIT, "Update BRCM: update brcm vk firmware on slot %d device %d\n", slot_id, dev_id);
  } else if (comp == UPDATE_VSI) {
    if (check_gpv2_rp_image(&rp_fw_type, fd, st.st_size) < 0) {
      goto error_exit;
    }
    syslog(LOG_CRIT, "Update VSI: update vsi rp firmware on slot %d device %d\n", slot_id, dev_id);
  }

  if (st.st_size/100 < 100)
    dsize = st.st_size;
  else
    dsize = st.st_size/100;

  bic_disable_sensor_monitor(slot_id, 1);
  msleep(100);

  offset = 0;
  last_offset = 0;
  uint8_t bus, wbuf[256], rbuf[256];
  int rlen = 0;

  bus = (2 + dev_id/2) * 2 + 1;
  wbuf[0] = 1 << (dev_id%2);

  // MUX
  ret = bic_master_write_read(slot_id, bus, 0xe2, wbuf, 1, rbuf, 0);
  if (ret != 0) {
    syslog(LOG_DEBUG, "%s(): bic_master_write_read failed\n", __func__);
    goto error_exit;
  }

  if (comp == UPDATE_BRCM) {
    wbuf[0] = BRCM_UPDATE_STATUS;  // offset 128 process bit == 0?
    rlen = 1;
    ret = bic_master_write_read(slot_id, bus, 0xd4, wbuf, 1, rbuf, rlen);
    if (ret != 0) {
      syslog(LOG_DEBUG,"%s(): process bit == 0? offset=%d read length=%d failed", __func__,wbuf[0],rlen);
      goto error_exit;
    }
    if (rbuf[0] & 2) {
      syslog(LOG_DEBUG,"%s(): process bit == 1 offset=%d rbuf[0]=%d", __func__,wbuf[0],rbuf[0]);
      ret = -1;
      goto error_exit;
    } else {
      syslog(LOG_DEBUG,"%s(): process bit == 0 offset=%d rbuf[0]=%d", __func__,wbuf[0],rbuf[0]);
    }
    wbuf[0] = BRCM_UPDATE_STATUS;  // offset 128 set download init
    wbuf[1] = rbuf[0] | 0x80;
    rlen = 0; // write
    ret = bic_master_write_read(slot_id, bus, 0xd4, wbuf, 2, rbuf, rlen);
    if (ret != 0) {
      syslog(LOG_DEBUG,"%s(): set download init offset=%d read length=%d failed", __func__,wbuf[0],rlen);
      goto error_exit;
    } else {
      syslog(LOG_DEBUG,"%s(): set download init offset=%d", __func__,wbuf[0]);
    }

    sleep(2);
    wbuf[0] = BRCM_UPDATE_STATUS;  // offset 128 reday bit == 1?
    rlen = 1;
    ret = bic_master_write_read(slot_id, bus, 0xd4, wbuf, 1, rbuf, rlen);
    if (ret != 0) {
      syslog(LOG_DEBUG,"%s(): reday bit == 1? offset=%d read length=%d failed", __func__,wbuf[0],rlen);
      goto error_exit;
    }

    if (!(rbuf[0] & 0x40)) {
      syslog(LOG_DEBUG,"%s(): reday bit == 0", __func__);
      ret = -1;
      goto error_exit;
    } else {
      syslog(LOG_DEBUG,"%s(): reday bit == 1 offset=%d rbuf[0]=%d", __func__,wbuf[0],rbuf[0]);
    }

    wbuf[0] = BRCM_WRITE_CMD;  // offset 130
    wbuf[1] = 9;
    uint32_t addr = 0x00400001;
    memcpy(&wbuf[2],&addr, sizeof(uint32_t));
    wbuf[6] = 4;
    wbuf[7] = 0x00;
    wbuf[8] = 0x00;
    wbuf[9] = 0x00;
    wbuf[10] = 0x00;
    rlen = 0; // write
    ret = bic_master_write_read(slot_id, bus, 0xd4, wbuf, 11, rbuf, rlen);
    if (ret != 0) {
      syslog(LOG_DEBUG,"%s(): bic_master_write_read offset=%d read length=%d failed", __func__,wbuf[0],rlen);
      goto error_exit;
    }
  } else if (comp == UPDATE_VSI) { // update VSI
    //# Step 1: host informs ASIC: host ready
    wbuf[0] = VSI_UPDATE_STATUS;  // offset 133
    wbuf[1] = 0x80;
    rlen = 0; //write
    ret = bic_master_write_read(slot_id, bus, 0xd4, wbuf, 2, rbuf, rlen);
    if (ret != 0) {
      syslog(LOG_DEBUG,"%s(): host informs ASIC: host ready offset=%d read length=%d failed", __func__,wbuf[0],rlen);
      goto error_exit;
    }

    sleep(1);

    //# Step 2: host informs ASIC: host will trans the image of bootrom patch
    wbuf[0] = VSI_FW_TYPE;  // offset 135
    wbuf[1] = rp_fw_type;
    rlen = 0; //write
    ret = bic_master_write_read(slot_id, bus, 0xd4, wbuf, 2, rbuf, rlen);
    if (ret != 0) {
      syslog(LOG_DEBUG,"%s(): host will trans the image offset=%d read length=%d failed", __func__,wbuf[0],rlen);
      goto error_exit;
    }

    sleep(1);

    //# Step 3: ASIC informs host: ASIC ready
    wbuf[0] = VSI_UPDATE_STATUS;  // offset 133
    rlen = 1;
    ret = bic_master_write_read(slot_id, bus, 0xd4, wbuf, 1, rbuf, rlen);
    if (ret != 0) {
      syslog(LOG_DEBUG,"%s():ASIC ready offset=%d read length=%d failed", __func__,wbuf[0],rlen);
      goto error_exit;
    } else {
      syslog(LOG_DEBUG,"%s(): ASIC ready offset=%d rbuf[0]=%d", __func__,wbuf[0],rbuf[0]);
    }

    sleep(2);
  }

  // Write chunks of binary data in a loop
  syslog(LOG_DEBUG,"%s(): Start a loop", __func__);
  while (1) {
    if (comp == UPDATE_BRCM) {
      read_count = IPMB_WRITE_COUNT_MAX;
    } else {
      read_count = 1;
    }

    // Read from file
    count = read(fd, buf, read_count);
    if (count <= 0 || count > read_count) {
      break;
    }

    if (comp == UPDATE_BRCM) {
      ret = _update_brcm_fw(slot_id, dev_id, comp, offset, count, buf);
    } else if (comp == UPDATE_VSI) {
      ret = _update_vsi_fw(slot_id, dev_id, comp, offset, count, buf);
    }

    if (ret) {
      goto error_exit;
    }

    if (comp == UPDATE_BRCM) {
      msleep(1); // wait

      wbuf[0] = BRCM_UPDATE_STATUS;  // offset 128 process bit == 1?
      rlen = 1;
      ret = bic_master_write_read(slot_id, bus, 0xd4, wbuf, 1, rbuf, rlen);
      if (ret != 0) {
        syslog(LOG_DEBUG,"%s(): process bit == 1? offset=%d read length=%d failed", __func__,wbuf[0],rlen);
        goto error_exit;
      }
      if (!(rbuf[0] & 2)) {
        syslog(LOG_DEBUG,"%s(): process bit == 0", __func__);
        ret = -1;
        goto error_exit;
      }
    } else {
      msleep(1); // wait
    }

    // Update counter
    offset += count;
    if ((last_offset + dsize) <= offset) {
       switch(comp) {
        case UPDATE_BRCM:
           _set_fw_update_ongoing(slot_id, 60);
           if (st.st_size/100 < 100)
             printf("\rupdated brcm vk: %d %%", offset/dsize*100);
           else
             printf("\rupdated brcm vk: %d %%", offset/dsize);
           break;
        case UPDATE_VSI:
           _set_fw_update_ongoing(slot_id, 60);
           if (st.st_size/100 < 100)
             printf("\rupdated vsi rp: %d %%", offset/dsize*100);
           else
             printf("\rupdated vsi rp: %d %%", offset/dsize);
           break;
       }
       fflush(stdout);
       last_offset += dsize;
    }
  }
  syslog(LOG_DEBUG,"%s(): End a loop", __func__);

  if (comp == UPDATE_BRCM) {
    wbuf[0] = BRCM_UPDATE_STATUS;  // offset 128 set activation
    wbuf[1] = rbuf[0] | 0x8;
    rlen = 0; // write
    ret = bic_master_write_read(slot_id, bus, 0xd4, wbuf, 2, rbuf, rlen);
    if (ret != 0) {
      syslog(LOG_DEBUG,"%s(): set activation offset=%d read length=%d failed", __func__,wbuf[0],rlen);
      goto error_exit;
    }

    wbuf[0] = BRCM_UPDATE_STATUS;  // offset 128 readiness bit == 1?
    rlen = 1;
    ret = bic_master_write_read(slot_id, bus, 0xd4, wbuf, 1, rbuf, rlen);
    if (ret != 0) {
      syslog(LOG_DEBUG,"%s(): readiness bit == 1? offset=%d read length=%d failed", __func__,wbuf[0],rlen);
      goto error_exit;
    }

    if (!(rbuf[0] & 4)) {
      syslog(LOG_DEBUG,"%s(): readiness bit == 0", __func__);
      goto error_exit;
    } else {
      syslog(LOG_DEBUG,"%s(): readiness bit == 1 offset=%d rbuf[0]=%d", __func__,wbuf[0],rbuf[0]);
    }

    for (int i=0;i<5;i++) {
      _check_brcm_fw_status(slot_id,dev_id);
      sleep(1);
    }

  } else {
    sleep(2);
    //# Step 5: host informs ASIC: host Tx done
    wbuf[0] = VSI_UPDATE_STATUS;  // offset 133
    wbuf[1] = 0x08;
    rlen = 0; //write
    ret = bic_master_write_read(slot_id, bus, 0xd4, wbuf, 2, rbuf, rlen);
    if (ret != 0) {
      syslog(LOG_DEBUG,"%s(): host informs ASIC: host Tx done offset=%d read length=%d failed", __func__,wbuf[0],rlen);
      goto error_exit;
    }
    sleep(2);
    //# Step 6: ASIC inform host: ASIC Rx done
    wbuf[0] = VSI_UPDATE_STATUS;  // offset 133
    rlen = 1;
    ret = bic_master_write_read(slot_id, bus, 0xd4, wbuf, 1, rbuf, rlen);
    if (ret != 0) {
      syslog(LOG_DEBUG,"%s(): ASIC inform host: ASIC Rx done offset=%d read length=%d failed", __func__,wbuf[0],rlen);
      goto error_exit;
    } else {
      syslog(LOG_DEBUG,"%s(): ASIC inform host: ASIC Rx done offset=%d rbuf[0]=%d", __func__,wbuf[0],rbuf[0]);
      if (rbuf[0] != 6) {
        ret = -1;
        goto error_exit;
      }
    }
    sleep(1);
  }

  ret = 0;
error_exit:
  bic_disable_sensor_monitor(slot_id, 0);
  printf("\n");
  switch(comp) {
    case UPDATE_BRCM:
      syslog(LOG_CRIT, "Update BRCM: updating brcm vk device firmware is exiting on slot %d device %d\n", slot_id, dev_id);
      break;
    case UPDATE_VSI:
      syslog(LOG_CRIT, "Update VSI: updating vsi rp device firmware is exiting on slot %d device %d\n", slot_id, dev_id);
      break;
  }
  if (fd > 0 ) {
    close(fd);
  }

  return ret;
}

int
bic_update_firmware(uint8_t slot_id, uint8_t comp, char *path, uint8_t force) {
  int ret = -1, rc;
  uint32_t offset;
  volatile uint16_t count, read_count;
  uint8_t buf[256] = {0};
  uint8_t target;
  uint8_t slot_num = 0, dev_id = 0;
  int fd;
  int i;

  if (comp == UPDATE_SPH) {
    slot_num = (slot_id >> 4);
    dev_id = (slot_id & 0xF);
    slot_id = slot_num;
    syslog(LOG_CRIT, "Update Intel: update intel sph firmware on slot %d device %d\n", slot_id, dev_id);
  }

  printf("updating fw on slot %d:\n", slot_id);
  // Handle Bridge IC firmware separately as the process differs significantly from others
  if (comp == UPDATE_BIC) {
    return _update_bic_main(slot_id, path, force);
  } else if (comp == UPDATE_SPH) {
    return update_dev_firmware(slot_num, dev_id, path);
  }

  uint32_t dsize, last_offset, block_offset;
  struct stat st;
  // Open the file exclusively for read
  fd = open(path, O_RDONLY, 0666);
  if (fd < 0) {
    printf("ERROR: invalid file path!\n");
#ifdef DEBUG
    syslog(LOG_ERR, "bic_update_fw: open fails for path: %s\n", path);
#endif
    goto error_exit;
  }

  stat(path, &st);
  if (comp == UPDATE_BIOS) {
    if (!force && check_bios_image(slot_id, fd, st.st_size)) {
      printf("invalid BIOS file!\n");
      goto error_exit;
    }
    syslog(LOG_CRIT, "Update BIOS: update bios firmware on slot %d\n", slot_id);
    dsize = st.st_size/100;
  } else if (comp == UPDATE_VR) {
    if (check_vr_image(slot_id, fd, st.st_size) < 0) {
      printf("invalid VR file!\n");
      goto error_exit;
    }
    syslog(LOG_CRIT, "Update VR: update vr firmware on slot %d\n", slot_id);
    dsize = st.st_size/5;
  } else if (comp == UPDATE_PCIE_SWITCH) {
    if (_reset_pcie_sw_update_status(slot_id) != 0) {
      goto error_exit;
    }
    syslog(LOG_CRIT, "Update PCIE SWITCH: update pcie_sw firmware on slot %d\n", slot_id);
    if (st.st_size/100 < 100)
      dsize = st.st_size;
    else
      dsize = st.st_size/100;
  } else {
    if ((comp == UPDATE_CPLD) && (check_cpld_image(slot_id, fd, st.st_size) < 0)) {
      printf("invalid CPLD file!\n");
      goto error_exit;
    }
    switch(comp){
      case UPDATE_CPLD:
        syslog(LOG_CRIT, "Update CPLD: update cpld firmware on slot %d\n", slot_id);
        break;
      case UPDATE_BIC_BOOTLOADER:
        syslog(LOG_CRIT, "Update BIC BL: update bic bootloader firmware on slot %d\n", slot_id);
        break;
    }
    dsize = st.st_size/20;
  }
  // Write chunks of binary data in a loop
  offset = 0;
  last_offset = 0;
  block_offset = 0;// for pcie switch update
  i = 1;
  int last_tunk_pcie_sw_flag = 0;
  while (1) {
    // For BIOS, send packets in blocks of 64K
    if (comp == UPDATE_BIOS && ((offset+IPMB_WRITE_COUNT_MAX) > (i * BIOS_ERASE_PKT_SIZE))) {
      read_count = (i * BIOS_ERASE_PKT_SIZE) - offset;
      i++;
    } else if (comp == UPDATE_PCIE_SWITCH) {
      if (i%5 == 0) {
        //last trunk at a block
        read_count = ((offset + LAST_FW_PCIE_SWITCH_TRUNK_SIZE) <= st.st_size) ? LAST_FW_PCIE_SWITCH_TRUNK_SIZE : (st.st_size - offset);
        last_tunk_pcie_sw_flag = 1;
      } else {
        last_tunk_pcie_sw_flag = ((offset + IPMB_WRITE_COUNT_MAX) < st.st_size) ? 0 : 1;
        if (last_tunk_pcie_sw_flag) {
          read_count = st.st_size - offset;
        } else {
          read_count = IPMB_WRITE_COUNT_MAX;
        }
      }
      i++;
    } else {
      read_count = IPMB_WRITE_COUNT_MAX;
    }

    // Read from file
    count = read(fd, buf, read_count);
    if (count <= 0 || count > read_count) {
      break;
    }

    if (comp == UPDATE_PCIE_SWITCH){
      // For PCIE switch update, the last trunk of a block is indicated by extra flag
      if (last_tunk_pcie_sw_flag) {
        target = comp | 0x80;
      } else {
        target = comp;
      }
    } else if ((comp != UPDATE_BIOS) && ((offset + count) >= st.st_size)) {
      // For non-BIOS update, the last packet is indicated by extra flag
      target = comp | 0x80;
    } else {
      target = comp;
    }

    if (comp == UPDATE_PCIE_SWITCH) {
      rc = _update_pcie_sw_fw(slot_id, target, block_offset, count, st.st_size, buf);
    } else {
      // Send data to Bridge-IC
      rc = _update_fw(slot_id, target, offset, count, buf);
    }

    if (rc) {
      goto error_exit;
    }

    // Update counter
    offset += count;
    if ((last_offset + dsize) <= offset) {
       switch(comp) {
         case UPDATE_BIOS:
           _set_fw_update_ongoing(slot_id, 60);
           printf("\rupdated bios: %d %%", offset/dsize);
           break;
         case UPDATE_CPLD:
           printf("\ruploaded cpld: %d %%", offset/dsize*5);
           break;
         case UPDATE_VR:
           printf("\rupdated vr: %d %%", offset/dsize*20);
           break;
         case UPDATE_PCIE_SWITCH:
           _set_fw_update_ongoing(slot_id, 60);
           if (st.st_size/100 < 100)
             printf("\rupdated pcie switch: %d %%", offset/dsize*100);
           else
             printf("\rupdated pcie switch: %d %%", offset/dsize);
           break;
         default:
           printf("\rupdated bic boot loader: %d %%", offset/dsize*5);
           break;
       }
       fflush(stdout);
       last_offset += dsize;
    }

    //wait for writing  to pcie switch
    if (comp == UPDATE_PCIE_SWITCH && (target & 0x80) ) {
      uint8_t status[2] = {0};
      int j = 0;
      block_offset = offset; //update block offset
      for (j=0;j<PCIE_SW_MAX_RETRY;j++) {
        rc = _get_pcie_sw_update_status(slot_id,status);
        if (rc) {
          syslog(LOG_WARNING,"_get_pcie_sw_update_status status: %d %d",status[0],status[1]);
          goto error_exit;
        }
        if (status[0] == FW_PCIE_SWITCH_DLSTAT_INPROGRESS || status[0] == FW_PCIE_SWITCH_DLSTAT_READY) {
          // check background status
          if ( status[1] == FW_PCIE_SWITCH_STAT_INPROGRESS || status[1] == FW_PCIE_SWITCH_STAT_IDLE ) {
            // do nothing
          } else if (status[1] == FW_PCIE_SWITCH_STAT_DONE) {
            // In the middle of FW upfdate, wait for download status done
            break;
          } else {
            _terminate_pcie_sw_update(slot_id);
            syslog(LOG_WARNING,"_get_pcie_sw_update_status status: %d %d",status[0],status[1]);
            goto error_exit;
          }
        } else if (status[0] == FW_PCIE_SWITCH_DLSTAT_COMPLETES
          || status[0] == FW_PCIE_SWITCH_DLSTAT_SUCESS_FIRM_ACT
          || status[0] == FW_PCIE_SWITCH_DLSTAT_SUCESS_DATA_ACT) {
          // check background status
          if ( status[1] == FW_PCIE_SWITCH_STAT_INPROGRESS) {
            // do nothing
          } else if (status[1] == FW_PCIE_SWITCH_STAT_IDLE || status[1] == FW_PCIE_SWITCH_STAT_DONE) {
            // At the end of FW update, after done then chenage to idle
            break;
          } else {
            _terminate_pcie_sw_update(slot_id);
            syslog(LOG_WARNING,"_get_pcie_sw_update_status status: %d %d",status[0],status[1]);
            goto error_exit;
          }
        } else {
          _terminate_pcie_sw_update(slot_id);
          syslog(LOG_WARNING,"_get_pcie_sw_update_status status: %d %d",status[0],status[1]);
          goto error_exit;
        }

        msleep(100);
      }
      if (j == PCIE_SW_MAX_RETRY) {
        _terminate_pcie_sw_update(slot_id);
        syslog(LOG_WARNING,"_get_pcie_sw_update_status retry == %d status: %d %d",PCIE_SW_MAX_RETRY,status[0],status[1]);
        goto error_exit;
      }
      if (offset == st.st_size) {
        _terminate_pcie_sw_update(slot_id);
      }
    }
  }

  if (comp == UPDATE_CPLD) {
    printf("\n");
    for (i = 0; i < 60; i++) {  // wait 60s at most
      rc = _get_cpld_update_progress(slot_id, buf);
      if (rc) {
        goto error_exit;
      }

      if (buf[0] == 0xFD) {  // error code
        goto error_exit;
      }

      printf("\rupdated cpld: %d %%", buf[0]);
      fflush(stdout);
      buf[0] %= 101;
      if (buf[0] == 100)
        break;

      sleep(2);
    }
  }

  if (comp == UPDATE_BIOS) {
    _set_fw_update_ongoing(slot_id, 60 * 2);
    if (verify_bios_image(slot_id, fd, st.st_size))
      goto error_exit;
  }

  ret = 0;
error_exit:
  printf("\n");
  switch(comp) {
    case UPDATE_BIOS:
      syslog(LOG_CRIT, "Update BIOS: updating bios firmware is exiting on slot %d\n", slot_id);
      break;
    case UPDATE_CPLD:
      syslog(LOG_CRIT, "Update CPLD: updating cpld firmware is exiting on slot %d\n", slot_id);
      break;
    case UPDATE_VR:
      syslog(LOG_CRIT, "Update VR: updating vr firmware is exiting on slot %d\n", slot_id);
      break;
    case UPDATE_BIC_BOOTLOADER:
      syslog(LOG_CRIT, "Update BIC BL: updating bic bootloader firmware is exiting on slot %d\n", slot_id);
      break;
    case UPDATE_PCIE_SWITCH:
      syslog(LOG_CRIT, "Update PCIE SWITCH: updating pcie switch firmware is exiting on slot %d\n", slot_id);
      break;
    case UPDATE_SPH:
      syslog(LOG_CRIT, "Update Intel: updating intel sph device firmware is exiting on slot %d device %d\n", slot_id, dev_id);
      break;
  }
  if (fd > 0 ) {
    close(fd);
  }

  return ret;
}

int
bic_update_fw(uint8_t slot_id, uint8_t comp, char *path) {
  return bic_update_firmware(slot_id, comp, path, 0);
}

int
bic_imc_xmit(uint8_t slot_id, uint8_t *txbuf, uint8_t txlen, uint8_t *rxbuf, uint8_t *rxlen) {
  uint8_t tbuf[256] = {0x15, 0xA0, 0x00}; // IANA ID
  uint8_t rbuf[256] = {0x00};
  uint8_t rlen = 0;
  uint8_t tlen = 0;
  int ret;

  // Fill the interface number as IMC
  tbuf[3] = BIC_INTF_IMC;

  // Fill the data to be sent
  memcpy(&tbuf[4], txbuf, txlen);

  // Send data length includes IANA ID and interface number
  tlen = txlen + 4;

  ret = bic_ipmb_wrapper(slot_id, NETFN_OEM_1S_REQ, CMD_OEM_1S_MSG_OUT, tbuf, tlen, rbuf, &rlen);
  if (ret ) {
    return -1;
  }

  // Make sure the received interface number is same
  if (rbuf[3] != tbuf[3]) {
    return -1;
  }

  // Copy the received data to caller skipping header
  memcpy(rxbuf, &rbuf[4], rlen-4);

  *rxlen = rlen-4;

  return 0;
}

int
bic_me_xmit(uint8_t slot_id, uint8_t *txbuf, uint8_t txlen, uint8_t *rxbuf, uint8_t *rxlen) {
  uint8_t tbuf[256] = {0x15, 0xA0, 0x00}; // IANA ID
  uint8_t rbuf[256] = {0x00};
  uint8_t rlen = 0;
  uint8_t tlen = 0;
  int ret;

  // Fill the interface number as ME
  tbuf[3] = BIC_INTF_ME;

  // Fill the data to be sent
  memcpy(&tbuf[4], txbuf, txlen);

  // Send data length includes IANA ID and interface number
  tlen = txlen + 4;

  ret = bic_ipmb_wrapper(slot_id, NETFN_OEM_1S_REQ, CMD_OEM_1S_MSG_OUT, tbuf, tlen, rbuf, &rlen);
  if (ret ) {
    return -1;
  }

  // Make sure the received interface number is same
  if (rbuf[3] != tbuf[3]) {
    return -1;
  }

  // Copy the received data to caller skipping header
  memcpy(rxbuf, &rbuf[6], rlen-6);

  *rxlen = rlen-6;

  return 0;
}

// Read 1S server's FRUID
int
bic_get_fruid_info(uint8_t slot_id, uint8_t fru_id, ipmi_fruid_info_t *info) {
  int ret;
  uint8_t rlen = 0;

  ret = bic_ipmb_wrapper(slot_id, NETFN_STORAGE_REQ, CMD_STORAGE_GET_FRUID_INFO, &fru_id, 1, (uint8_t *) info, &rlen);

  return ret;
}

static int
_read_fruid(uint8_t slot_id, uint8_t fru_id, uint32_t offset, uint8_t count, uint8_t *rbuf, uint8_t *rlen) {
  int ret;
  uint8_t tbuf[4] = {0};

  tbuf[0] = fru_id;
  tbuf[1] = offset & 0xFF;
  tbuf[2] = (offset >> 8) & 0xFF;
  tbuf[3] = count;

  ret = bic_ipmb_wrapper(slot_id, NETFN_STORAGE_REQ, CMD_STORAGE_READ_FRUID_DATA, tbuf, 4, rbuf, rlen);

  return ret;
}

int
bic_read_fruid(uint8_t slot_id, uint8_t fru_id, const char *path, int *fru_size) {
  int ret = 0;
  uint32_t nread;
  uint32_t offset;
  uint8_t count;
  uint8_t rbuf[256] = {0};
  uint8_t rlen = 0;
  int fd;
  ipmi_fruid_info_t info;

  // Remove the file if exists already
  unlink(path);

  // Open the file exclusively for write
  fd = open(path, O_WRONLY | O_CREAT | O_EXCL, 0666);
  if (fd < 0) {
#ifdef DEBUG
    syslog(LOG_ERR, "bic_read_fruid: open fails for path: %s\n", path);
#endif
    goto error_exit;
  }

  // Read the FRUID information
  ret = bic_get_fruid_info(slot_id, fru_id, &info);
  if (ret) {
#ifdef DEBUG
    syslog(LOG_ERR, "bic_read_fruid: bic_read_fruid_info returns %d\n", ret);
#endif
    goto error_exit;
  }

  // Indicates the size of the FRUID
  nread = (info.size_msb << 8) | info.size_lsb;
  if (nread > FRUID_SIZE) {
    nread = FRUID_SIZE;
  }
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

    ret = _read_fruid(slot_id, fru_id, offset, count, rbuf, &rlen);
    if (ret) {
#ifdef DEBUG
      syslog(LOG_ERR, "bic_read_fruid: ipmb_wrapper fails\n");
#endif
      goto error_exit;
    }

    // Ignore the first byte as it indicates length of response
    write(fd, &rbuf[1], rlen-1);

    // Update offset
    offset += (rlen-1);
    nread -= (rlen-1);
  }

 close(fd);
 return ret;

error_exit:
  if (fd > 0 ) {
    close(fd);
  }

  return -1;
}

static int
_write_fruid(uint8_t slot_id, uint8_t fru_id, uint32_t offset, uint8_t count, uint8_t *buf) {
  int ret;
  uint8_t tbuf[64] = {0};
  uint8_t rbuf[4] = {0};
  uint8_t tlen = 0;
  uint8_t rlen = 0;

  tbuf[0] = fru_id;
  tbuf[1] = offset & 0xFF;
  tbuf[2] = (offset >> 8) & 0xFF;

  memcpy(&tbuf[3], buf, count);
  tlen = count + 3;

  ret = bic_ipmb_wrapper(slot_id, NETFN_STORAGE_REQ, CMD_STORAGE_WRITE_FRUID_DATA, tbuf, tlen, rbuf, &rlen);

  if (ret) {
    return ret;
  }

  if (rbuf[0] != count) {
    return -1;
  }

  return ret;
}

int
bic_write_fruid(uint8_t slot_id, uint8_t fru_id, const char *path) {
  int ret = -1;
  uint32_t offset;
  uint8_t count;
  uint8_t buf[64] = {0};
  int fd;

  // Open the file exclusively for read
  fd = open(path, O_RDONLY, 0666);
  if (fd < 0) {
#ifdef DEBUG
    syslog(LOG_ERR, "bic_write_fruid: open fails for path: %s\n", path);
#endif
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
    ret = _write_fruid(slot_id, fru_id, offset, count, buf);
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

// Read System Event Log (SEL)
int
bic_get_sel_info(uint8_t slot_id, ipmi_sel_sdr_info_t *info) {
  int ret;
  uint8_t rlen = 0;

  ret = bic_ipmb_wrapper(slot_id, NETFN_STORAGE_REQ, CMD_STORAGE_GET_SEL_INFO, NULL, 0, (uint8_t *)info, &rlen);

  return ret;
}

int
bic_get_sel(uint8_t slot_id, ipmi_sel_sdr_req_t *req, ipmi_sel_sdr_res_t *res, uint8_t *rlen) {

  int ret;

  ret = bic_ipmb_wrapper(slot_id, NETFN_STORAGE_REQ, CMD_STORAGE_GET_SEL, (uint8_t *)req, sizeof(ipmi_sel_sdr_req_t), (uint8_t*)res, rlen);

  return ret;
}

// Read Sensor Data Records (SDR)
int
bic_get_sdr_info(uint8_t slot_id, ipmi_sel_sdr_info_t *info) {
  int ret;
  uint8_t rlen = 0;

  ret = bic_ipmb_wrapper(slot_id, NETFN_STORAGE_REQ, CMD_STORAGE_GET_SDR_INFO, NULL, 0, (uint8_t *) info, &rlen);

  return ret;
}

static int
_get_sdr_rsv(uint8_t slot_id, uint8_t *rsv) {
  int ret;
  uint8_t rlen = 0;

  ret = bic_ipmb_wrapper(slot_id, NETFN_STORAGE_REQ, CMD_STORAGE_RSV_SDR, NULL, 0, (uint8_t *) rsv, &rlen);

  return ret;
}

static int
_get_sdr(uint8_t slot_id, ipmi_sel_sdr_req_t *req, ipmi_sel_sdr_res_t *res, uint8_t *rlen) {
  int ret;

  ret = bic_ipmb_wrapper(slot_id, NETFN_STORAGE_REQ, CMD_STORAGE_GET_SDR, (uint8_t *)req, sizeof(ipmi_sel_sdr_req_t), (uint8_t*)res, rlen);

  return ret;
}

int
bic_get_sdr(uint8_t slot_id, ipmi_sel_sdr_req_t *req, ipmi_sel_sdr_res_t *res, uint8_t *rlen) {
  int ret;
  uint8_t tbuf[MAX_IPMB_RES_LEN] = {0};
  uint8_t rbuf[MAX_IPMB_RES_LEN] = {0};
  uint8_t tlen;
  uint8_t len;
  ipmi_sel_sdr_res_t *tres;
  sdr_rec_hdr_t *hdr;

  tres = (ipmi_sel_sdr_res_t *) tbuf;

  // Get SDR reservation ID for the given record
  ret = _get_sdr_rsv(slot_id, rbuf);
  if (ret) {
#ifdef DEBUG
    syslog(LOG_ERR, "bic_read_sdr: _get_sdr_rsv returns %d\n", ret);
#endif
    return ret;
  }

  req->rsv_id = (rbuf[1] << 8) | rbuf[0];

  // Initialize the response length to zero
  *rlen = 0;

  // Read SDR Record Header
  req->offset = 0;
  req->nbytes = sizeof(sdr_rec_hdr_t);

  ret = _get_sdr(slot_id, req, (ipmi_sel_sdr_res_t *)tbuf, &tlen);
  if (ret) {
#ifdef DEBUG
    syslog(LOG_ERR, "bic_read_sdr: _get_sdr returns %d\n", ret);
#endif
    return ret;
  }

#ifdef DEBUG
  syslog(LOG_DEBUG, "rsv_id: 0x%x, rec_id: 0x%x, offset: 0x%x, nbytes: %d\n", req->rsv_id, req->rec_id, req->offset, req->nbytes);
#endif
  if ((tlen - 2) != req->nbytes) {  // subtract the first two bytes(rsv_id) length from tlen and we will have the expected data length
    syslog(LOG_WARNING, "%s: SLOT%u SDR rsv_id: 0x%x, record_id: 0x%x, offset: 0x%x. Received Data Length does not match (expectative: 0x%x, actual: 0x%x)",
          __func__, slot_id, req->rsv_id, req->rec_id, req->offset, req->nbytes, tlen - 2);
    return -1;
  }

  // Copy the next record id to response
  res->next_rec_id = tres->next_rec_id;

  // Copy the header excluding first two bytes(next_rec_id)
  memcpy(res->data, tres->data, tlen-2);

  // Update response length and offset for next request
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

    ret = _get_sdr(slot_id, req, (ipmi_sel_sdr_res_t *)tbuf, &tlen);
    if (ret) {
#ifdef DEBUG
      syslog(LOG_ERR, "bic_read_sdr: _get_sdr returns %d\n", ret);
#endif
      return ret;
    }

#ifdef DEBUG
    syslog(LOG_DEBUG, "rsv_id: 0x%x, rec_id: 0x%x, offset: 0x%x, nbytes: %d\n", req->rsv_id, req->rec_id, req->offset, req->nbytes);
#endif
    if ((tlen - 2) != req->nbytes) {
      syslog(LOG_WARNING, "%s: SLOT%u SDR rsv_id: 0x%x, record_id: 0x%x, offset: 0x%x. Received Data Length does not match (expectative: 0x%x, actual: 0x%x)",
            __func__, slot_id, req->rsv_id, req->rec_id, req->offset, req->nbytes, tlen - 2);
      continue;
    }

    // Copy the data excluding the first two bytes(next_rec_id)
    memcpy(&res->data[req->offset], tres->data, tlen-2);

    // Update response length, offset for next request, and remaining length
    *rlen += tlen-2;
    req->offset += tlen-2;
    len -= tlen-2;
  }

  if (*rlen == 0) {
    syslog(LOG_ERR, "%s: SLOT%u - SDR size is zero\n", __func__, slot_id);
  }

  return 0;
}

int
bic_read_sensor(uint8_t slot_id, uint8_t sensor_num, ipmi_sensor_reading_t *sensor) {
  int ret;
  uint8_t rlen = 0;

  ret = bic_ipmb_wrapper(slot_id, NETFN_SENSOR_REQ, CMD_SENSOR_GET_SENSOR_READING, (uint8_t *)&sensor_num, 1, (uint8_t *)sensor, &rlen);

  return ret;
}

int
bic_read_device_sensors(uint8_t slot_id, uint8_t dev_id, ipmi_device_sensor_reading_t *sensor, uint8_t *len) {
  uint8_t tbuf[4] = {0x15, 0xA0, 0x00, 0x00}; // IANA ID + Sensor Num
  uint8_t rbuf[255] = {0x00};
  uint8_t rlen = 0;
  int ret;

  tbuf[3] = dev_id;
  ret = bic_ipmb_wrapper(slot_id, NETFN_OEM_1S_REQ, CMD_OEM_1S_GET_DEVICE_SENSOR_READING, tbuf, 0x04, rbuf, &rlen);

  if (rlen >= DEV_SENSOR_INFO_LEN + 3) { // at least one sensor
    memcpy(sensor, &rbuf[3], rlen-3);  // Ignore IANA ID
    *len = rlen - 3;
  } else {
    *len = 0;
    return -1;  // unavailable
  }

  return ret;
}

int
bic_read_accuracy_sensor(uint8_t slot_id, uint8_t sensor_num, ipmi_accuracy_sensor_reading_t *sensor) {
  uint8_t tbuf[4] = {0x15, 0xA0, 0x00, 0x00}; // IANA ID + Sensor Num
  uint8_t rbuf[255] = {0x00};
  uint8_t rlen = 0;
  int ret;

  tbuf[3] = sensor_num;
  ret = bic_ipmb_wrapper(slot_id, NETFN_OEM_1S_REQ, CMD_OEM_1S_ACCURACY_SENSOR_READING, tbuf, 0x04, rbuf, &rlen);

  if (rlen == 6) {
    memcpy(sensor, &rbuf[3], rlen-3);  // Ignore IANA ID
  } else {
    sensor->flags = 0x20;  // unavailable
  }

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
bic_request_post_buffer_data(uint8_t slot_id, uint8_t *port_buff, uint8_t *len) {
  int ret;
  uint8_t tbuf[3] = {0x15, 0xA0, 0x00}; // IANA ID
  uint8_t rbuf[MAX_IPMB_RES_LEN]={0x00};
  uint8_t rlen = 0;

  ret = bic_ipmb_wrapper(slot_id, NETFN_OEM_1S_REQ, CMD_OEM_1S_GET_POST_BUF, tbuf, 0x03, rbuf, &rlen);

  if(0 != ret)
    goto exit_done;

  // Ignore first 3 bytes of IANA ID
  memcpy(port_buff, &rbuf[3], rlen - 3);
  *len = rlen - 3;

exit_done:
  return ret;
}

int
bic_request_post_buffer_page_data(uint8_t slot_id, uint8_t page_num, uint8_t *port_buff, uint8_t *len) {
  int ret;
  uint8_t tbuf[4] = {0x15, 0xA0, 0x00}; // IANA ID
  uint8_t rbuf[MAX_IPMB_RES_LEN]={0x00};
  uint8_t rlen = 0;

  tbuf[3] = page_num & 0xFF;
  ret = bic_ipmb_wrapper(slot_id, NETFN_OEM_1S_REQ, CMD_OEM_1S_GET_POST_CODE_BUF, tbuf, 0x04, rbuf, &rlen);

  if(0 != ret)
    goto exit_done;

  // Ignore first 3 bytes of IANA ID
  memcpy(port_buff, &rbuf[3], rlen - 3);
  *len = rlen - 3;

exit_done:
  return ret;
}

int
bic_request_post_buffer_dword_data(uint8_t slot_id, uint32_t *port_buff, uint32_t input_len, uint32_t *output_len) {
  int ret = 0;
  uint8_t tbuf[4] = {0x15, 0xA0, 0x00}; // IANA ID
  uint8_t rbuf[MAX_IPMB_RES_LEN]={0x00};
  uint8_t rlen = 0;
  int totol_length = 0;

  for(int i = 1; i < MAX_POST_CODE_PAGE; i++)
  {
    tbuf[3] = i & 0xFF;
    ret = bic_ipmb_wrapper(slot_id, NETFN_OEM_1S_REQ, CMD_OEM_1S_GET_POST_CODE_BUF, tbuf, 0x04, rbuf, &rlen);

    if(0 != ret) {
      #ifdef DEBUG
      syslog(LOG_ERR, "bic_get_post_code_buf error, ret:%d", ret);
      #endif

      ret = -1;
      return ret;
    }

    // Ignore first 3 bytes of IANA ID
    for(int k = 3; (k < rlen-3) && (totol_length < input_len); k+=(sizeof(uint32_t)/sizeof(uint8_t)))
    {
      port_buff[totol_length] = rbuf[k] | (rbuf[k+1] << 8) | (rbuf[k+2] << 16) | (rbuf[k+3] << 24);
      totol_length++;
    }
  }

  *output_len = totol_length;

  return ret;
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
  //ND system does not have me
#ifndef CONFIG_FBY2_ND
  uint8_t tbuf[256] = {0x00};
  uint8_t rbuf[256] = {0x00};
  uint8_t tlen = 0;
  uint8_t rlen = 0;
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
    }
    else
      break;
  }
  if (retry == 4) { //if the third retry still failed, return -1
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
  if (retry == 4) { //if the third retry still failed, return -1
    syslog(LOG_CRIT, "%s: Restore Factory Default failed..., retried: %d", __func__,  retry);
    return -1;
  }
#endif
  return 0;
}

int
bic_get_slot_type(uint8_t fru) {
  int type = 3;   //set default to 3(Empty Slot)
  int retry = 3;
  char key[MAX_KEY_LEN] = {0};

  if ((fru < FRU_SLOT1) || (fru > FRU_SLOT4))
    return type;

  snprintf(key, sizeof(key), SLOT_FILE, fru);
  do {
    if (read_device(key, &type) == 0)
      break;
    syslog(LOG_WARNING,"bic_get_slot_type failed");
    msleep(10);
  } while (--retry);

  return type;
}

int
bic_get_record_slot_type(uint8_t fru) {
  int type = 3;   //set default to 3(Empty Slot)
  int retry = 3;
  char key[MAX_KEY_LEN] = {0};

  if ((fru < FRU_SLOT1) || (fru > FRU_SLOT4))
    return type;

  snprintf(key, sizeof(key), SLOT_RECORD_FILE, fru);
  do {
    if (read_device(key, &type) == 0)
      break;
    syslog(LOG_WARNING,"bic_get_record_slot_type failed");
    msleep(10);
  } while (--retry);

  return type;
}

int
bic_set_slot_type(uint8_t fru,uint8_t type) {
  int retry = 3;
  char key[MAX_KEY_LEN] = {0};
  char value[MAX_VALUE_LEN] = {0};

  if ((fru < FRU_SLOT1) || (fru > FRU_SLOT4))
    return type;

  snprintf(key, sizeof(key), SLOT_FILE, fru);
  snprintf(value, sizeof(value),"%d", type);
  do {
    if (write_device(key, value) == 0)
      break;
    syslog(LOG_WARNING,"bic_set_slot_type failed");
    msleep(10);
  } while (--retry);

  return 0;
}

int
bic_get_server_type(uint8_t fru, uint8_t *type) {
  int ret;
  int retries = 3;
  int server_type = SERVER_TYPE_NONE;
  ipmi_dev_id_t id = {0};
  char key[MAX_KEY_LEN] = {0};

  if ((fru < FRU_SLOT1) || (fru > FRU_SLOT4)) {
    *type = SERVER_TYPE_NONE;
    return 0;
  }

  // SERVER_TYPE = 0(TwinLake), 1(RC), 2(EP), 3(unknown)
  sprintf(key, SERVER_TYPE_FILE, fru);
  do {
    if (read_device(key, &server_type) == 0)
      break;
    syslog(LOG_WARNING,"bic_get_server_type failed");
    msleep(10);
  } while (--retries);

  if (retries == 0) {
    retries = 3;
    do {
      ret = bic_get_dev_id(fru, &id);
      if (!ret) {
        // Use product ID to identify the server type
        if (id.prod_id[0] == 0x43 && id.prod_id[1] == 0x52) {
          *type = SERVER_TYPE_RC;
        } else if (id.prod_id[0] == 0x50 && id.prod_id[1] == 0x45) {
          *type = SERVER_TYPE_EP;
        } else if (id.prod_id[0] == 0x39 && id.prod_id[1] == 0x30) {
          *type = SERVER_TYPE_TL;
        } else if (id.prod_id[0] == 0x44 && id.prod_id[1] == 0x4E) {
          *type = SERVER_TYPE_ND;
        } else {
          *type = SERVER_TYPE_NONE;
        }
        break;
      }
    } while ((--retries));

    if (retries == 0) {
      *type = SERVER_TYPE_NONE;
      syslog(LOG_ERR, "%s : Get server type failed for slot%u", __func__, fru);
      return -1;
    }
  } else {
    *type = server_type;
  }

  return 0;
}

int
bic_asd_init(uint8_t slot_id, uint8_t cmd) {
  uint8_t tbuf[8] = {0x15, 0xA0, 0x00}; // IANA ID
  uint8_t rbuf[8] = {0x00};
  uint8_t rlen = 0;
  int ret;

  tbuf[3] = cmd;
  ret = bic_ipmb_wrapper(slot_id, NETFN_OEM_1S_REQ, CMD_OEM_1S_ASD_INIT, tbuf, 4, rbuf, &rlen);

  return ret;
}

int
bic_clear_cmos(uint8_t slot_id) {
  uint8_t tbuf[3] = {0x15, 0xa0, 0x00}; // IANA ID
  uint8_t rbuf[8] = {0x00};
  uint8_t rlen = 0;

  return bic_ipmb_wrapper(slot_id, NETFN_OEM_1S_REQ, CMD_OEM_1S_CLEAR_CMOS, tbuf, 3, rbuf, &rlen);
}


int
bic_reset(uint8_t slot_id) {
  uint8_t tbuf[3] = {0x00, 0x00, 0x00}; // IANA ID
  uint8_t rbuf[8] = {0x00};
  uint8_t rlen = 0;
  int ret;

  ret = bic_ipmb_wrapper(slot_id, NETFN_APP_REQ, CMD_APP_COLD_RESET, tbuf, 0, rbuf, &rlen);

  return ret;
}

int
bic_set_pcie_config(uint8_t slot_id, uint8_t config) {
  uint8_t tbuf[4] = {0x15, 0xA0, 0x00}; // IANA ID
  uint8_t rlen = 0;
  uint8_t rbuf[16] = {0};
  int ret;

  if (bic_is_slot_power_en(slot_id)) {
    return 0;
  }

  tbuf[3] = config;
  ret = bic_ipmb_wrapper(slot_id, NETFN_OEM_1S_REQ, CMD_OEM_1S_SET_PCIE_CONFIG, tbuf, 0x04, rbuf, &rlen);

  return ret;
}

int get_imc_version(uint8_t slot, uint8_t *ver) {
  int i;
  int ret;
  char key[MAX_KEY_LEN] = {0};
  char str[MAX_VALUE_LEN] = {0};

  sprintf(key, "slot%d_imc_ver", (int)slot);
  ret = kv_get(key, str, NULL, KV_FPERSIST);
  if (ret) {
    return ret;
  }

  for (i = 0; i < IMC_VER_SIZE; i++) {
    ver[i] = str[i] - '0';
  }
  return 0;
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
bic_disable_sensor_monitor(uint8_t slot_id, uint8_t dis) {
  uint8_t tbuf[8] = {0x15, 0xA0, 0x00}; // IANA ID
  uint8_t rbuf[8] = {0x00};
  uint8_t rlen = 0;
  int ret;

  tbuf[3] = dis;  // 1: disable sensor monitor; 0: enable sensor monitor
  ret = bic_ipmb_wrapper(slot_id, NETFN_OEM_1S_REQ, CMD_OEM_1S_DISABLE_SEN_MON, tbuf, 4, rbuf, &rlen);

  return ret;
}

int
bic_send_jtag_instruction(uint8_t slot_id, uint8_t dev_id, uint8_t *rbuf, uint8_t ir) {
  uint8_t tbuf[8] = {0x15, 0xA0, 0x00}; // IANA ID
  uint8_t rlen = 0;
  int ret;

  tbuf[3] = dev_id;  // 0-based
  tbuf[4] = ir;
  ret = bic_ipmb_wrapper(slot_id, NETFN_OEM_1S_REQ, 0x39, tbuf, 5, rbuf, &rlen);

  return ret;
}

int
bic_get_debug_mode(uint8_t slot_id, uint8_t *debug_mode) {
  uint8_t tbuf[8] = {0x15, 0xA0, 0x00}; // IANA ID
  uint8_t rbuf[8] = {0}; // IANA ID
  uint8_t rlen = 0;
  int ret;

  tbuf[3] = 1; // 0: write 1: read
  ret = bic_ipmb_wrapper(slot_id, NETFN_OEM_1S_REQ, 0x3C, tbuf, 4, rbuf, &rlen);

  if ((ret == 0) && (rlen == 4)){
    *debug_mode = rbuf[3];
  } else {
    ret = -1;
  }

  return ret;
}

int
bic_set_sdr_threshold_update_flag(uint8_t slot, uint8_t update) {
  char key[MAX_KEY_LEN] = {0};
  char str[MAX_VALUE_LEN] = {0};

  snprintf(key,MAX_KEY_LEN, "slot%u_sdr_thresh_update", slot);
  snprintf(str,MAX_VALUE_LEN, "%u",update);
  return kv_set(key, str, 0, 0);
}

int
bic_get_sdr_threshold_update_flag(uint8_t slot) {
  int ret;
  char key[MAX_KEY_LEN] = {0};
  char cvalue[MAX_VALUE_LEN] = {0};
  sprintf(key, "slot%u_sdr_thresh_update", slot);

  ret = kv_get(key, cvalue,NULL,0);
  if (ret) {
    return 0;
  }
  return atoi(cvalue);
}

int
bic_get_device_type(uint8_t slot_id, uint8_t dev_num) {
  int ret = 0;
  int rlen = 0;
  uint8_t bus, wbuf[8], rbuf[64], fb_defined, ffi_0, offset_base = 0;

  bus = (2 + (dev_num / 2)) * 2 + 1;
  wbuf[0] = 1 << (dev_num % 2);

  ret = bic_master_write_read(slot_id, bus, 0xe2, wbuf, 1, rbuf, 0);
  if (ret != 0) {
    syslog(LOG_DEBUG, "%s(): bic_master_write_read failed", __func__);
    return ret;
  }

  wbuf[0] = 0x20;  // offset 32
  rlen = 55;
  ret = bic_master_write_read(slot_id, bus, 0xd4, wbuf, 1, rbuf, rlen);
  if (ret != 0) {
    syslog(LOG_DEBUG, "%s(): bic_master_write_read offset=%d read length=%d failed", __func__,wbuf[0],rlen);
    return -1;
  }
  if (rbuf[0] == wbuf[0]) {
    offset_base = SPRINGHILL_M2_OFFSET_BASE;
  }
  fb_defined = rbuf[offset_base + 1];
  ffi_0 = rbuf[offset_base + 43];

  if (fb_defined == 1) {
    if (ffi_0 == 0x01) {
      return DEV_TYPE_ASIC;
    } else {
      return DEV_TYPE_M_2;
    }
  } else {
    return DEV_TYPE_M_2;
  }
}

int reverse_bits(int raw_val)
{
  int c, reverse_val = 0;

  for (c = 7; c > 0; c--) {
    if (raw_val & 1) {
      reverse_val++;
    }
    reverse_val = reverse_val << 1;
    raw_val = raw_val >> 1;
  }
  if(raw_val & 1) {
    reverse_val++;
  }
  return reverse_val;
}

int program_dev_fw(uint8_t slot_id, uint8_t dev_id, int bus, char *image, int start, int end) {
  uint8_t wbuf[DEV_UPDATE_BATCH_SIZE + DEV_UPDATE_IPMI_HEAD_SIZE + 2]; //2 means buffer for ipmb
  uint8_t rlen = 0;
  uint8_t rbuf[DEV_UPDATE_BATCH_SIZE];
  FILE *fp1 = NULL;
  unsigned char buffer[DEV_UPDATE_BATCH_SIZE];
  int addr = 0, tmp, i, ret = 0;
  float cur_progress = 0.05;
  float run = 1;
  int total_run;
  int fileSize;

  printf("* Reading image...\n");
  addr = start;
  wbuf[0] = bus;
  wbuf[1] = PROGRAM_DEV_FW;
  fp1 = fopen(image, "rb");
  if (!fp1){
    printf("Image %s not found", image);
    return -1;
  }
  fseek(fp1, 0 , SEEK_END);
  fileSize = ftell(fp1);
  fseek(fp1, 0 , SEEK_SET);
  total_run = fileSize / DEV_UPDATE_BATCH_SIZE;
  if ((fileSize / 4) > (end - start + 1)) {
    printf("File size too large\n");
    fclose(fp1);
    return -1;
  }
  while(fread(buffer, DEV_UPDATE_BATCH_SIZE, 1, fp1)) {
    tmp = addr;
    for(i = DEV_UPDATE_IPMI_HEAD_SIZE - 1; i > 1; i--) { //offset is 0-based
      wbuf[i] = (tmp & 0xFF);
      tmp /= 0x100;
    }
    for(i = 0; i < DEV_UPDATE_BATCH_SIZE; i++) {
      wbuf[i + DEV_UPDATE_IPMI_HEAD_SIZE] = reverse_bits(buffer[i]);
    }
    ret = bic_ipmb_wrapper(slot_id, NETFN_OEM_STORAGE_REQ, DEV_UPDATE, wbuf,
                          (DEV_UPDATE_BATCH_SIZE + DEV_UPDATE_IPMI_HEAD_SIZE), rbuf, &rlen);
    if (ret < 0) {
      printf("Failed to send IPMB command!\n");
      goto err_exit;
    }
    if (rbuf[0] == CC_CAN_NOT_RESPOND) {
      printf("Command response could not be provided\n");
      fclose(fp1);
      return -1;
    }
    addr += DEV_UPDATE_BATCH_SIZE / 4;
    if (((float)(run/total_run) >= cur_progress) && ((float)((run-1)/total_run) <= cur_progress)) {
      printf("updated device: %d %%\n", (int)(cur_progress*100));
      cur_progress += 0.05;
    }
    run++;
  }
  printf("updated device: 100 %%\n");

err_exit:
  fclose(fp1);
  return ret;
}

/* Below IPMI command support to device fw update
  Request:
    Byte 1 – I2C Bus Number (0-based)
    Byte 2 – Device Upgrade Option
      00h: Erase device sector
      01h: Programming the device image
      02h: Reload device image
      03h: Get the image location of device booting
    Byte 3:6 – Image Offset
      only for Byte 1 = 01h
      Byte 7:N – Image Raw Data
      only for Byte 1 = 01h

  Response:
    Byte 1 – Completion Code
      00h: Success
      CEh: Command response could not be provided
    Byte 2 – device Command Status
      only for Byte 1 = 00h or 03h
*/
int update_dev_firmware (uint8_t slot_id, uint8_t dev_id, char* image) {
  uint8_t wbuf[2];
  uint8_t rlen = 0;
  uint8_t rbuf[DEV_UPDATE_BATCH_SIZE];
  uint8_t bus = (2 + dev_id/2) * 2 + 1;
  int ret = 0;

  printf("* Turning off sensor monitor...\n");
  ret = bic_disable_sensor_monitor(slot_id, 1);
  if (ret < 0) {
    printf("Turn off slot%u sensor monitor failed\n", slot_id);
    return -1;
  }
  sleep(2);

  printf("* Mux selecting...\n");
  wbuf[0] = 1 << (dev_id % 2);
  ret = bic_master_write_read(slot_id, bus, 0xe2, wbuf, 1, rbuf, 0);
  if (ret < 0) {
    printf("Select mux failed\n");
    return -1;
  }
  sleep(2);

  bus = (dev_id / 2) + 2;
  printf("* Erasing Device CFM0 sector...\n");
  wbuf[0] = bus;
  wbuf[1] = ERASE_DEV_FW;
  ret = bic_ipmb_wrapper(slot_id, NETFN_OEM_STORAGE_REQ, DEV_UPDATE, wbuf, 2, rbuf, &rlen);
  if (ret < 0 ) {
    printf("Failed to send IPMB command!\n");
    return ret;
  }
  if (rbuf[0] == CC_CAN_NOT_RESPOND) {
    printf("Command response could not be provided\n");
    return -1;
  }

  ret = program_dev_fw(slot_id, dev_id, bus, image, CFM0_START, CFM0_END);
  if (ret < 0) {
    return ret;
  }

  printf("* Turning on sensor monitor...\n");
  ret = bic_disable_sensor_monitor(slot_id, 0);
  if (ret < 0) {
    printf("Turn on slot%u sensor monitor failed\n", slot_id);
    return ret;
  }
  return ret;
}

int
bic_fget_device_info(uint8_t slot_id, uint8_t dev_num, uint8_t *ffi, uint8_t *meff, uint16_t *vendor_id, uint8_t *major_ver, uint8_t *minor_ver) {
  int ret = 0;
  int rlen = 0;
  uint8_t bus, wbuf[8], rbuf[64];

  dev_num = dev_num - 1;

  bus = (2 + (dev_num / 2)) * 2 + 1;
  ret = bic_disable_sensor_monitor(slot_id, 1);
  if (ret < 0) {
    printf("Turn off slot%u sensor monitor failed\n", slot_id);
    return -1;
  }

  wbuf[0] = 1 << (dev_num % 2);
  ret = bic_master_write_read(slot_id, bus, 0xe2, wbuf, 1, rbuf, 0);
  if (ret != 0) {
    syslog(LOG_DEBUG, "%s(): bic_master_write_read failed", __func__);
    return ret;
  }

  msleep(50);

  wbuf[0] = 0x08;  // offset 08
  rlen = 24;
  ret = bic_master_write_read(slot_id, bus, 0xd4, wbuf, 1, rbuf, rlen);
  if (ret != 0) {
    syslog(LOG_DEBUG, "%s(): bic_master_write_read offset=%d read length=%d failed", __func__,wbuf[0],rlen);
    return ret;
  }
  *vendor_id = (rbuf[1] << 8) | rbuf[2];

  wbuf[0] = 0x20;  // offset 32
  rlen = 55;
  ret = bic_master_write_read(slot_id, bus, 0xd4, wbuf, 1, rbuf, rlen);
  if (ret != 0) {
    syslog(LOG_DEBUG, "%s(): bic_master_write_read offset=%d read length=%d failed", __func__,wbuf[0],rlen);
    return ret;
  }
  *meff = rbuf[42];
  *ffi = rbuf[43];

  wbuf[0] = 0x68;  // offset 104
  rlen = 8;
  ret = bic_master_write_read(slot_id, bus, 0xd4, wbuf, 1, rbuf, rlen);
  if (ret != 0) {
    syslog(LOG_DEBUG, "%s(): bic_master_write_read offset=%d read length=%d failed", __func__,wbuf[0],rlen);
    return ret;
  }
  *major_ver = rbuf[2];
  *minor_ver = rbuf[3];

  ret = bic_disable_sensor_monitor(slot_id, 0);
  if (ret < 0) {
    printf("Turn on slot%u sensor monitor failed\n", slot_id);
    return ret;
  }

  return ret;
}
