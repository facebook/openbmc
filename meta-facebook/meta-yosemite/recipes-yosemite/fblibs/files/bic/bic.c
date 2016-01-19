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
#include "bic.h"

#define FRUID_READ_COUNT_MAX 0x30
#define FRUID_WRITE_COUNT_MAX 0x30
#define IPMB_WRITE_COUNT_MAX 224
#define BIOS_ERASE_PKT_SIZE (64*1024)
#define BIOS_VERIFY_PKT_SIZE (32*1024)
#define SDR_READ_COUNT_MAX 0x1A
#define SIZE_SYS_GUID 16
#define SIZE_IANA_ID 3
#define GPIO_MAX 31

enum {
  IPMB_BUS_SLOT1 = 3,
  IPMB_BUS_SLOT2 = 1,
  IPMB_BUS_SLOT3 = 7,
  IPMB_BUS_SLOT4 = 5,
};

#pragma pack(push, 1)
typedef struct _sdr_rec_hdr_t {
  uint16_t rec_id;
  uint8_t ver;
  uint8_t type;
  uint8_t len;
} sdr_rec_hdr_t;
#pragma pack(pop)

// Common IPMB Wrapper function

static int
get_ipmb_bus_id(uint8_t slot_id) {
  int bus_id;

  switch(slot_id) {
  case 1:
    bus_id = IPMB_BUS_SLOT1;
    break;
  case 2:
    bus_id = IPMB_BUS_SLOT2;
    break;
  case 3:
    bus_id = IPMB_BUS_SLOT3;
    break;
  case 4:
    bus_id = IPMB_BUS_SLOT4;
    break;
  default:
    bus_id = -1;
    break;
  }

  return bus_id;
}

static int
bic_ipmb_wrapper(uint8_t slot_id, uint8_t netfn, uint8_t cmd,
                  uint8_t *txbuf, uint8_t txlen,
                  uint8_t *rxbuf, uint8_t *rxlen) {
  ipmb_req_t *req;
  ipmb_res_t *res;
  uint8_t rbuf[MAX_IPMB_RES_LEN] = {0};
  uint8_t tbuf[MAX_IPMB_RES_LEN] = {0};
  uint8_t tlen = 0;
  uint8_t rlen = 0;
  int count = 0;
  int i = 0;
  int ret;
  uint8_t bus_id;

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

  // Invoke IPMB library handler
  lib_ipmb_handle(bus_id, tbuf, tlen, &rbuf, &rlen);

  if (rlen == 0) {
#ifdef DEBUG
    syslog(LOG_DEBUG, "bic_ipmb_wrapper: Zero bytes received\n");
#endif
    return -1;
  }

  // Handle IPMB response
  res  = (ipmb_res_t*) rbuf;

  if (res->cc) {
#ifdef DEBUG
    syslog(LOG_ERR, "bic_ipmb_wrapper: Completion Code: 0x%X\n", res->cc);
#endif
    return -1;
  }

  // copy the received data back to caller
  *rxlen = rlen - IPMB_HDR_SIZE - IPMI_RESP_HDR_SIZE;
  memcpy(rxbuf, res->data, *rxlen);

  return 0;
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

  printf("_update_fw: target: %d, offset: %d, len: %d\n", target, offset, len);

  tlen = len + 10;

bic_send:
  ret = bic_ipmb_wrapper(slot_id, NETFN_OEM_1S_REQ, CMD_OEM_1S_UPDATE_FW, tbuf, tlen, rbuf, &rlen);
  if ((ret) && (retries--)) {
    sleep(1);
    printf("_update_fw: retrying..\n");
    goto bic_send;
  }

  return ret;
}

int
bic_update_fw(uint8_t slot_id, uint8_t comp, char *path) {
  int ret;
  uint32_t offset;
  volatile uint16_t count, read_count;
  uint8_t buf[256] = {0};
  uint8_t len = 0;
  uint8_t target;
  int fd;
  int i;
  uint32_t tcksum;
  uint32_t gcksum;
  uint8_t *tbuf;

  // Open the file exclusively for read
  fd = open(path, O_RDONLY, 0666);
  if (fd < 0) {
#ifdef DEBUG
    syslog(LOG_ERR, "bic_update_fw: open fails for path: %s\n", path);
#endif
    goto error_exit;
  }

  // Write chunks of binary data in a loop
  offset = 0;
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
bic_read_fruid(uint8_t slot_id, uint8_t fru_id, const char *path) {
  int ret;
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

error_exit:
  if (fd > 0 ) {
    close(fd);
  }

  return ret;
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
  uint8_t tlen;
  uint8_t len;
  ipmi_sel_sdr_res_t *tres;
  sdr_rec_hdr_t *hdr;

  tres = (ipmi_sel_sdr_res_t *) tbuf;

  // Get SDR reservation ID for the given record
  ret = _get_sdr_rsv(slot_id, &req->rsv_id);
  if (ret) {
#ifdef DEBUG
    syslog(LOG_ERR, "bic_read_sdr: _get_sdr_rsv returns %d\n", ret);
#endif
    return ret;
  }

  // Initialize the response length to zero
  *rlen = 0;

  // Read SDR Record Header
  req->offset = 0;
  req->nbytes = sizeof(sdr_rec_hdr_t);

  ret = _get_sdr(slot_id, req, tbuf, &tlen);
  if (ret) {
#ifdef DEBUG
    syslog(LOG_ERR, "bic_read_sdr: _get_sdr returns %d\n", ret);
#endif
    return ret;
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

    ret = _get_sdr(slot_id, req, tbuf, &tlen);
    if (ret) {
#ifdef DEBUG
      syslog(LOG_ERR, "bic_read_sdr: _get_sdr returns %d\n", ret);
#endif
      return ret;
    }

    // Copy the data excluding the first two bytes(next_rec_id)
    memcpy(&res->data[req->offset], tres->data, tlen-2);

    // Update response length, offset for next request, and remaining length
    *rlen += tlen-2;
    req->offset += tlen-2;
    len -= tlen-2;
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
