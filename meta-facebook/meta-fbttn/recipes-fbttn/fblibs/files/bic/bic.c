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
#include <fcntl.h>
#include <syslog.h>
#include <errno.h>
#include <sys/stat.h>
#include "bic.h"
#include <openbmc/obmc-i2c.h>

#define FRUID_READ_COUNT_MAX 0x30
#define FRUID_WRITE_COUNT_MAX 0x30
#define IPMB_WRITE_COUNT_MAX 224
#define BIOS_ERASE_PKT_SIZE (64*1024)
#define BIOS_VERIFY_PKT_SIZE (32*1024)
#define SDR_READ_COUNT_MAX 0x1A
#define SIZE_SYS_GUID 16
#define SIZE_IANA_ID 3
#define GPIO_MAX 31

#define BIC_UPDATE_RETRIES 12
#define BIC_UPDATE_TIMEOUT 500

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

enum {
  IPMB_BUS_SLOT1 = 3,
};

#pragma pack(push, 1)
typedef struct _sdr_rec_hdr_t {
  uint16_t rec_id;
  uint8_t ver;
  uint8_t type;
  uint8_t len;
} sdr_rec_hdr_t;
#pragma pack(pop)

const static uint8_t gpio_bic_update_ready[] = { 0, 55};

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

static uint8_t
_is_bic_update_ready(uint8_t slot_id) {
  int val;
  char path[64] = {0};

  if (slot_id < 1 || slot_id > 4) {
    return 0;
  }

  sprintf(path, GPIO_VAL, gpio_bic_update_ready[slot_id]);

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
  case 1:
    bus_id = IPMB_BUS_SLOT1;
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
  int count = 0;
  int i = 0;
  int ret;
  uint8_t bus_id;
  uint8_t dataCksum;
  int retry = 0;

  ret = get_ipmb_bus_id(slot_id);
  if (ret < 0) {
#ifdef DEBUG
    syslog(LOG_ERR, "%s: Wrong Slot ID %d\n", __func__, slot_id);
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

  while(retry < 5) {
    // Invoke IPMB library handler
    lib_ipmb_handle(bus_id, tbuf, tlen, &rbuf, &rlen);

    if (rlen == 0) {
      retry++;
      msleep(20);
    }
    else
      break;
  }

  if (rlen == 0) {
#ifdef DEBUG
    syslog(LOG_DEBUG, "%s: Zero bytes received, retry:%d\n", __func__, retry);
#endif
    return -1;
  }


  // Handle IPMB response
  res  = (ipmb_res_t*) rbuf;

  if (res->cc) {
#ifdef DEBUG
    syslog(LOG_ERR, "%s: Completion Code: 0x%X\n", __func__, res->cc);
#endif
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
    syslog(LOG_CRIT, "%s: Receive Data cksum does not match (expectative 0x%x, actual 0x%x)", __func__, dataCksum, rbuf[rlen - 1]);
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

// Get GPIO value and configuration
int
bic_get_gpio(uint8_t slot_id, bic_gpio_t *gpio) {
  uint8_t tbuf[3] = {0x15, 0xA0, 0x00}; // IANA ID
  uint8_t rbuf[7] = {0x00};
  uint8_t rlen = 0;
  int ret;

  ret = bic_ipmb_wrapper(slot_id, NETFN_OEM_1S_REQ, CMD_OEM_1S_GET_GPIO, tbuf, 0x03, rbuf, &rlen);

  // Ignore first 3 bytes of IANA ID
  memcpy((uint8_t*) gpio, &rbuf[3], 4);

  return ret;
}

int
bic_set_gpio(uint8_t slot_id, uint8_t gpio, uint8_t value) {
  uint8_t tbuf[11] = {0x15, 0xA0, 0x00}; // IANA ID
  uint8_t rbuf[3] = {0x00};
  uint8_t rlen = 0;
  int ret;

  // Check for boundary conditions
  if (gpio > GPIO_MAX) {
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
    memset(&tbuf[7] , 0x00, 4);
  }

  ret = bic_ipmb_wrapper(slot_id, NETFN_OEM_1S_REQ, CMD_OEM_1S_SET_GPIO, tbuf, 11, rbuf, &rlen);

  return ret;
}

int
bic_get_gpio_config(uint8_t slot_id, uint8_t gpio, bic_gpio_config_t *gpio_config) {
  uint8_t tbuf[7] = {0x15, 0xA0, 0x00}; // IANA ID
  uint8_t rbuf[4] = {0x00};
  uint8_t rlen = 0;
  uint8_t tlen = 0;
  uint32_t pin;
  int ret;

  pin = 1 << gpio;

  tbuf[3] = pin & 0xFF;
  tbuf[4] = (pin >> 8) & 0xFF;
  tbuf[5] = (pin >> 16) & 0xFF;
  tbuf[6] = (pin >> 24) & 0xFF;

  tlen = 7;

  ret = bic_ipmb_wrapper(slot_id, NETFN_OEM_1S_REQ, CMD_OEM_1S_GET_GPIO_CONFIG, tbuf, tlen, rbuf, &rlen);

  // Ignore IANA ID
  *(uint8_t *) gpio_config = rbuf[3];

  return ret;
}

int
bic_set_gpio_config(uint8_t slot_id, uint8_t gpio, bic_gpio_config_t *gpio_config) {
  uint8_t tbuf[8] = {0x15, 0xA0, 0x00}; // IANA ID
  uint8_t rbuf[4] = {0x00};
  uint8_t rlen = 0;
  uint8_t tlen = 0;
  uint32_t pin;
  int ret;

  pin = 1 << gpio;

  tbuf[3] = pin & 0xFF;
  tbuf[4] = (pin >> 8) & 0xFF;
  tbuf[5] = (pin >> 16) & 0xFF;
  tbuf[6] = (pin >> 24) & 0xFF;
  tbuf[7] = (*(uint8_t *) gpio_config) & 0x1F;

  tlen = 8;

  ret = bic_ipmb_wrapper(slot_id, NETFN_OEM_1S_REQ, CMD_OEM_1S_SET_GPIO_CONFIG,
      tbuf, tlen, rbuf, &rlen);
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

  // Fill the component for which firmware is requested
  tbuf[3] = comp;

  ret = bic_ipmb_wrapper(slot_id, NETFN_OEM_1S_REQ, CMD_OEM_1S_GET_FW_VER, tbuf, 0x04, rbuf, &rlen);
  // fw version has to be between 2 and 5 bytes based on component
  if (ret || (rlen < 2+SIZE_IANA_ID) || (rlen > 5+SIZE_IANA_ID)) {
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
bic_get_fw_cksum(uint8_t slot_id, uint8_t comp, uint32_t offset, uint32_t len, uint8_t *ver) {
  uint8_t tbuf[12] = {0x15, 0xA0, 0x00}; // IANA ID
  uint8_t rbuf[16] = {0x00};
  uint8_t rlen = 0;
  int ret;

  // Fill the component for which firmware is requested
  tbuf[3] = comp;

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


  ret = bic_ipmb_wrapper(slot_id, NETFN_OEM_1S_REQ, CMD_OEM_1S_GET_FW_CKSUM, tbuf, 12, rbuf, &rlen);
  // checksum has to be 4 bytes
  if (ret || (rlen != 4+SIZE_IANA_ID)) {
    syslog(LOG_ERR, "bic_get_fw_cksum: ret: %d, rlen: %d\n", ret, rlen);
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

static int
_update_bic_main(uint8_t slot_id, char *path) {
  int fd;
  int ifd;
  char cmd[100] = {0};
  struct stat buf;
  int size;
  uint8_t tbuf[256] = {0};
  uint8_t rbuf[16] = {0};
  uint8_t tcount;
  uint8_t rcount;
  volatile int xcount;
  int i = 0;
  int ret;
  uint8_t xbuf[256] = {0};
  uint32_t offset = 0, last_offset = 0, dsize;

  // Open the file exclusively for read
  fd = open(path, O_RDONLY, 0666);
  if (fd < 0) {
    syslog(LOG_ERR, "bic_update_fw: open fails for path: %s\n", path);
    goto error_exit;
  }

  fstat(fd, &buf);
  size = buf.st_size;
  printf("size of file is %d bytes\n", size);
  dsize = size/20;

  // Open the i2c driver
  ifd = i2c_open(get_ipmb_bus_id(slot_id));
  if (ifd < 0) {
    printf("ifd error\n");
    goto error_exit2;
  }

  // Kill ipmb daemon for this slot
  sprintf(cmd, "sv stop ipmbd_%d", get_ipmb_bus_id(slot_id));
  system(cmd);
  printf("Stopped ipmbd for this slot %x..\n",slot_id);

  // The I2C high speed clock (1M) could cause to read BIC data abnormally.
  // So reduce I2C bus 3 clock speed which is a workaround for BIC update.
  printf("Set the I2C bus 3 clock to 100K\n");
  system("devmem 0x1e78a104 w 0xFFF77304");
  sleep(1);

  // Restart ipmb daemon with "-u|--enable-bic-update" for bic update
  memset(cmd, 0, sizeof(cmd));
  sprintf(cmd, "/usr/local/bin/ipmbd -u %d %d > /dev/null 2>&1 &", get_ipmb_bus_id(slot_id), slot_id);
  system(cmd);
  printf("start ipmbd -u for this slot %x..\n",slot_id);
  sleep(1);

  // Enable Bridge-IC update
  if (!_is_bic_update_ready(slot_id)) {
      _enable_bic_update(slot_id);
  }

  // Kill ipmb daemon "--enable-bic-update" for this slot
  memset(cmd, 0, sizeof(cmd));
  sprintf(cmd, "ps | grep -v 'grep' | grep 'ipmbd -u %d' |awk '{print $1}'| xargs kill", get_ipmb_bus_id(slot_id));
  system(cmd);
  printf("killed ipmbd for this slot..\n");

  // Wait for SMB_BMC_3v3SB_ALRT_N
  for (i = 0; i < BIC_UPDATE_RETRIES; i++) {
    if (_is_bic_update_ready(slot_id)) {
      printf("bic ready after %d tries\n", i);
      break;
    }
    msleep(BIC_UPDATE_TIMEOUT);
  }

  if (i == BIC_UPDATE_RETRIES) {
    printf("bic is NOT ready\n");
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

  ret = i2c_io(ifd, tbuf, tcount, rbuf, rcount);
  if (ret) {
    printf("i2c_io failed download\n");
    goto error_exit;
  }

  //delay for download command process ---
  msleep(500);
  tcount = 0;
  rcount = 2;
  ret = i2c_io(ifd, tbuf, tcount, rbuf, rcount);
  if (ret) {
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

    rcount = 5;

    ret = i2c_io(ifd, tbuf, tcount, rbuf, rcount);
    if (ret) {
printf("i2c_io failed\n");
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
    ret = i2c_io(ifd, tbuf, tcount, rbuf, rcount);
    if (ret) {
      printf("i2c_io failed, Send ACK\n");
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
    // write data only
    tcount = tbuf[0];
    rcount = 0;
    ret = i2c_io(ifd, tbuf, tcount, rbuf, rcount);
    if (ret) {
      printf("i2c_io write error\n");
      goto error_exit;
    }
    msleep(10); // Waiting for BIC flash data.
    // read data only
    tcount = 0;
    rcount = 2;
    ret = i2c_io(ifd, tbuf, tcount, rbuf, rcount);
    if (ret) {
      printf("i2c_io read error\n");
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

  rcount = 2;

  ret = i2c_io(ifd, tbuf, tcount, rbuf, rcount);
  if (ret) {
    printf("i2c_io failed for run\n");
    goto error_exit;
  }

  if (rbuf[0] != 0x00 || rbuf[1] != 0xcc) {
    printf("run response: %x:%x\n", rbuf[0], rbuf[1]);
    goto error_exit;
  }
  msleep(500);

error_exit:
  // Restore the I2C bus 3 clock to 1M.
  printf("Set the I2C bus 3 clock to 1M\n");
  system("devmem 0x1e78a104 w 0xFFF99300");
  sleep(1);
  // Restart ipmbd daemon
  memset(cmd, 0, sizeof(cmd));
  sprintf(cmd, "sv start ipmbd_%d", get_ipmb_bus_id(slot_id));
  system(cmd);

error_exit2:
  if (fd > 0) {
    close(fd);
  }

  return 0;
}

int
bic_update_fw(uint8_t slot_id, uint8_t comp, char *path) {
  int ret;
  uint32_t offset;
  volatile uint16_t count, read_count;
  uint8_t buf[256] = {0};
  char    cmd[100] = {0};
  char    temp[8];
  uint8_t len = 0;
  uint8_t target;
  int fd;
  int i;
  uint32_t tcksum;
  uint32_t gcksum;
  uint8_t *tbuf = NULL;

  printf("updating fw on slot %d:\n", slot_id);
  // Handle Bridge IC firmware separately as the process differs significantly from others
  if (comp == UPDATE_BIC) {
   return  _update_bic_main(slot_id, path);
  }

  uint32_t dsize, last_offset;
  struct stat st;
  // Open the file exclusively for read
  fd = open(path, O_RDONLY, 0666);
  if (fd < 0) {
#ifdef DEBUG
    syslog(LOG_ERR, "bic_update_fw: open fails for path: %s\n", path);
#endif
    goto error_exit;
  }

  // Kill sendor daemon for this slot
  if (comp == UPDATE_BIOS ) {
    system("sv stop sensord");
    printf("Stopped sensord for this slot.\n");
  }
  stat(path, &st);
  if (comp == UPDATE_BIOS) {
    dsize = st.st_size/100;
  } else {
    dsize = st.st_size/20;
  }
  // Write chunks of binary data in a loop
  offset = 0;
  last_offset = 0;
  i = 1;
  while (1) {
    // For BIOS, send packets in blocks of 64K
    if (comp == UPDATE_BIOS && ((offset+IPMB_WRITE_COUNT_MAX) > (i * BIOS_ERASE_PKT_SIZE))) {
      read_count = (i * BIOS_ERASE_PKT_SIZE) - offset;
      i++;
    } else {
      read_count = IPMB_WRITE_COUNT_MAX;
    }

    // Read from file
    count = read(fd, buf, read_count);
    if (count <= 0) {
      break;
    }

    // For non-BIOS update, the last packet is indicated by extra flag
    if ((comp != UPDATE_BIOS) && (count < read_count)) {
      target = comp | 0x80;
    } else {
      target = comp;
    }

    // Send data to Bridge-IC
    ret = _update_fw(slot_id, target, offset, count, buf);
    if (ret) {
      break;
    }

    // Update counter
    offset += count;
    if((last_offset + dsize) <= offset) {
       switch(comp) {
         case UPDATE_BIOS:
           printf("updated bios: %d %%\n", offset/dsize);
           break;
         case UPDATE_CPLD:
           printf("updated cpld: %d %%\n", offset/dsize*5);
           break;
         default:
           printf("updated bic boot loader: %d %%\n", offset/dsize*5);
           break;
       }
       last_offset += dsize;
    }
  }

  if (comp != UPDATE_BIOS) {
    goto update_done;
  }

  // Checksum calculation for BIOS image
  tbuf = malloc(BIOS_VERIFY_PKT_SIZE * sizeof(uint8_t));
  if (!tbuf) {
    goto error_exit;
  }

  lseek(fd, 0, SEEK_SET);
  offset = 0;
  while (1) {
    count = read(fd, tbuf, BIOS_VERIFY_PKT_SIZE);
    if (count <= 0) {
      break;
    }

    tcksum = 0;
    for (i = 0; i < BIOS_VERIFY_PKT_SIZE; i++) {
      tcksum += tbuf[i];
    }

    // Get the checksum of binary image
    ret = bic_get_fw_cksum(slot_id, comp, offset, BIOS_VERIFY_PKT_SIZE, &gcksum);
    if (ret) {
      goto error_exit;
    }

    // Compare both and see if they match or not
    if (gcksum != tcksum) {
      printf("checksum does not match offset:0x%x, 0x%x:0x%x\n", offset, tcksum, gcksum);
      goto error_exit;
    }

    offset += BIOS_VERIFY_PKT_SIZE;
  }

update_done:
error_exit:
  if (fd > 0 ) {
    close(fd);
  }

  if (tbuf) {
    free(tbuf);
  }
  if (comp == UPDATE_BIOS ) {
    system("sv start sensord");
  }
  return ret;
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
  memcpy(rxbuf, &rbuf[7], rlen-7);

  *rxlen = rlen-7;

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
  uint8_t tlen = 0;

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
  nread = (info.size_msb << 6) + (info.size_lsb);
  *fru_size = nread;
  if (*fru_size == 0) {
    syslog(LOG_ERR, "%s: FRU %d - FRU size is zero\n", __func__, slot_id);
    goto error_exit;
  }

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
  int ret;
  uint32_t offset;
  uint8_t count;
  uint8_t buf[64] = {0};
  uint8_t len = 0;
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

static int
_get_sel_rsv(uint8_t slot_id, uint16_t *rsv) {
  int ret;
  uint8_t rlen = 0;

  ret = bic_ipmb_wrapper(slot_id, NETFN_STORAGE_REQ, CMD_STORAGE_RSV_SEL, NULL, 0, (uint8_t *) rsv, &rlen);
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
_get_sdr_rsv(uint8_t slot_id, uint16_t *rsv) {
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
    syslog(LOG_WARNING, "%s: SDR rsv_id: 0x%x, record_id: 0x%x, offset: 0x%x. Received Data Length does not match (expectative: 0x%x, actual: 0x%x)",
          __func__, req->rsv_id, req->rec_id, req->offset, req->nbytes, tlen - 2);
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
      syslog(LOG_WARNING, "%s: SDR rsv_id: 0x%x, record_id: 0x%x, offset: 0x%x. Received Data Length does not match (expectative: 0x%x, actual: 0x%x)",
            __func__, req->rsv_id, req->rec_id, req->offset, req->nbytes, tlen - 2);
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
    syslog(LOG_ERR, "%s: FRU %d - SDR size is zero\n", __func__, slot_id);
  }

  return 0;
}

int
bic_read_sensor(uint8_t slot_id, uint8_t sensor_num, ipmi_sensor_reading_t *sensor) {
  int ret;
  int rlen = 0;

  ret = bic_ipmb_wrapper(slot_id, NETFN_SENSOR_REQ, CMD_SENSOR_GET_SENSOR_READING, (uint8_t *)&sensor_num, 1, (uint8_t *)sensor, &rlen);

  return ret;
}

int
bic_get_sys_guid(uint8_t slot_id, uint8_t *guid) {
  int ret;
  int rlen = 0;

  ret = bic_ipmb_wrapper(slot_id, NETFN_APP_REQ, CMD_APP_GET_SYSTEM_GUID, NULL, 0, guid, &rlen);
  if (rlen != SIZE_SYS_GUID) {
#ifdef DEBUG
    syslog(LOG_ERR, "bic_get_sys_guid: returned rlen of %d\n");
#endif
    return -1;
  }

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
    syslog(LOG_CRIT, "%s: Command: Restart using Recovery Firmware failed..., retried: %d", __func__, retry);
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
    if ( (command == RECOVERY_MODE) && (rbuf[0] == 0x81) && (rbuf[1] == 0x02) ) {
      return 0;
    }
    else if ( (command == RESTORE_FACTORY_DEFAULT) && (rbuf[0] == 0x55) && (rbuf[1] == 0x00) ) {
      return 0;
    }
    else {
      return -1;
    }
  }
  if (retry == 4) { //if the third retry still failed, return -1
    syslog(LOG_CRIT, "%s: Command: Restore Factory Default failed..., retried: %d", __func__, retry);
    return -1;
  }

}
