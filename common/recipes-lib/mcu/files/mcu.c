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
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <syslog.h>
#include <time.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <openbmc/ipmb.h>
#include <openbmc/ipmi.h>
#include <openbmc/obmc-i2c.h>
#include <openbmc/pal.h>
#include "mcu.h"

#define MCU_FLASH_START 0x8000
#define MCU_PKT_MAX 252

#define MCU_CMD_DOWNLOAD 0x21
#define MCU_CMD_RUN 0x22
#define MCU_CMD_STATUS 0x23
#define MCU_CMD_DATA 0x24

#define CMD_DOWNLOAD_SIZE 11
#define CMD_RUN_SIZE 7
#define CMD_STATUS_SIZE 3

#define IPMB_WRITE_COUNT_MAX 224

#define CMD_OEM_GET_BOOTLOADER_VER 0x40

#define EN_UPDATE_IF_I2C 0x01
#define MCU_SIGNED_FULL_SIZE    (32)
#define MCU_SIGNED_HEAD_SIZE    (5)
#define MCU_SIGNED_CHIP_SIZE    (9)
#define MCU_SIGNED_DATA_SIZE    MCU_SIGNED_FULL_SIZE - MCU_SIGNED_HEAD_SIZE - MCU_SIGNED_CHIP_SIZE

static int
mcu_ipmb_wrapper(uint8_t bus, uint8_t addr, uint8_t netfn, uint8_t cmd,
                 uint8_t *txbuf, uint16_t txlen, uint8_t *rxbuf, uint8_t *rxlen) {
  ipmb_req_t *req;
  ipmb_res_t *res;
  uint8_t rbuf[MAX_IPMB_RES_LEN] = {0};
  uint8_t tbuf[MAX_IPMB_RES_LEN] = {0};
  uint16_t tlen = 0;
  uint8_t rlen = 0;

  req = (ipmb_req_t *)tbuf;
  req->res_slave_addr = addr;
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
  lib_ipmb_handle(bus, tbuf, tlen, rbuf, &rlen);
  if (rlen < (IPMB_HDR_SIZE + IPMI_RESP_HDR_SIZE)) {
#ifdef DEBUG
    syslog(LOG_DEBUG, "%s: insufficient bytes received", __func__);
#endif
    return -1;
  }

  // Handle IPMB response
  res = (ipmb_res_t *)rbuf;
  if (res->cc) {
#ifdef DEBUG
    syslog(LOG_ERR, "%s: Completion Code: 0x%02X", __func__, res->cc);
#endif
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

int
mcu_get_fw_ver(uint8_t bus, uint8_t addr, uint8_t comp, uint8_t *ver) {
  uint8_t rbuf[32] = {0x00};
  uint8_t rlen = sizeof(rbuf);
  int ret = -1;

  switch (comp) {
    case MCU_FW_RUNTIME:  // runtime firmware version
      ret = mcu_ipmb_wrapper(bus, addr, NETFN_APP_REQ, CMD_APP_GET_DEVICE_ID, NULL, 0, rbuf, &rlen);
      if (!ret) {
        memcpy(ver, &rbuf[2], 2);
      }
      break;

    case MCU_FW_BOOTLOADER:  // boot-loader version
      ret = mcu_ipmb_wrapper(bus, addr, NETFN_OEM_REQ, CMD_OEM_GET_BOOTLOADER_VER, NULL, 0, rbuf, &rlen);
      if (!ret) {
        memcpy(ver, &rbuf[0], 2);
      }
      break;
  }

  return ret;
}

static int
mcu_enable_update(uint8_t bus, uint8_t addr) {
  uint8_t tbuf[8] = {0x15, 0xA0, 0x00};
  uint8_t rbuf[16] = {0x00};
  uint8_t rlen = sizeof(rbuf);
  int ret;

  // Update on I2C Channel, the only available option from BMC
  tbuf[3] = EN_UPDATE_IF_I2C;
  ret = mcu_ipmb_wrapper(bus, addr, NETFN_OEM_1S_REQ, CMD_OEM_1S_ENABLE_BIC_UPDATE, tbuf, 4, rbuf, &rlen);

  return ret;
}

int
mcu_update_firmware(uint8_t bus, uint8_t addr, const char *path, const char *key, uint8_t is_signed ) {
  char cmd[100] = {0};
  uint8_t tbuf[256] = {0};
  uint8_t rbuf[16] = {0};
  char str[32] = {0};
  char sbuf[32] = {0};
  uint8_t tcount;
  uint8_t rcount;
  int fd, ifd, size;
  int xcount = 0;
  int i, rc, ret = -1;
  uint32_t offset = 0, last_offset = 0, dsize;
  FILE *fp;
  struct stat buf;
  struct rlimit mqlim;

  fd = open(path, O_RDONLY, 0666);
  if (fd < 0) {
    printf("ERROR: invalid file path!\n");
    return -1;
  }

  if(is_signed == true) {
    memcpy(str, key, MCU_SIGNED_FULL_SIZE);
    lseek(fd, -(MCU_SIGNED_DATA_SIZE), SEEK_END);
    if (read(fd, sbuf, MCU_SIGNED_DATA_SIZE) == MCU_SIGNED_DATA_SIZE ) {
      if(strstr(sbuf, str) == NULL) {
        printf("Error, Signed Key not Match\n");
        return -1;
      }
    } else {
      printf("File Read Fail\n");
      return -1;
    }
    lseek(fd, 0 ,SEEK_SET);
    printf("Signed Key Match\n");
  } 

  fstat(fd, &buf);

  if (is_signed == true) {
    size = buf.st_size - MCU_SIGNED_FULL_SIZE;
  } else {
    size = buf.st_size;
  }
  printf("size of file is %d bytes\n", size);
  dsize = size/20;

  snprintf(cmd, sizeof(cmd), "/dev/i2c-%d", bus);
  ifd = open(cmd, O_RDWR);
  if (ifd < 0) {
    syslog(LOG_ERR, "%s: i2c open failed for bus#%d", __func__, bus);
    close(fd);
    return -1;
  }

  // Stop ipmbd
  snprintf(cmd, sizeof(cmd), "sv stop ipmbd_%d", bus);
  if (system(cmd)) {
    syslog(LOG_ERR, "%s: stop ipmbd for bus#%d", __func__, bus);
    close(ifd);
    close(fd);
    return -1;
  }
  printf("Stopped ipmbd_%d...\n", bus);
  msleep(500);

  if (pal_is_mcu_ready(bus)) {
    // To avoid mq_open problem that some platforms may have many ipmbd,
    // this is only effective for the calling process
    mqlim.rlim_cur = RLIM_INFINITY;
    mqlim.rlim_max = RLIM_INFINITY;
    if (setrlimit(RLIMIT_MSGQUEUE, &mqlim) < 0) {
      goto error_exit;
    }

    i = 1;
    snprintf(cmd, sizeof(cmd), "/etc/sv/ipmbd_%d/run", bus);
    fp = fopen(cmd, "r");
    if (fp != NULL) {
      while (fgets(cmd, sizeof(cmd), fp)) {
        if (sscanf(cmd, "exec /usr/local/bin/ipmbd %*d %d", &i) == 1)
          break;
      }
      fclose(fp);
    }

    // Restart IPMB daemon with "-u|--enable-bic-update" for bic update
    snprintf(cmd, sizeof(cmd), "/usr/local/bin/ipmbd -u %d %d > /dev/null 2>&1 &", bus, i);
    if (system(cmd)) {
      syslog(LOG_CRIT, "Starting IPMB on bus %d failed!\n", bus);
      goto error_exit;
    }
    printf("start ipmbd_%d -u...\n", bus);
    sleep(2);

    mcu_enable_update(bus, addr);

    // Kill ipmbd "--enable-bic-update" for this slot
    snprintf(cmd, sizeof(cmd), "ps -w | grep -v 'grep' | grep 'ipmbd -u %d' |awk '{print $1}'| xargs kill", bus);
    if (system(cmd)) {
      syslog(LOG_CRIT, "Stopping IPMB on bus: %d failed\n", bus);
      goto error_exit;
    }
    printf("stop ipmbd_%d -u...\n", bus);

    // To wait for MCU reset to bootloader for updating
    pal_wait_mcu_ready2update(bus);
  }

  // Start MCU update (0x21)
  tbuf[0] = CMD_DOWNLOAD_SIZE;
  tbuf[1] = MCU_CMD_DOWNLOAD;  // checksum
  tbuf[2] = MCU_CMD_DOWNLOAD;

  // Update flash address: 0x8000
  tbuf[3] = (MCU_FLASH_START >> 24) & 0xFF;
  tbuf[4] = (MCU_FLASH_START >> 16) & 0xFF;
  tbuf[5] = (MCU_FLASH_START >> 8) & 0xFF;
  tbuf[6] = (MCU_FLASH_START) & 0xFF;

  // Image size
  tbuf[7] = (size >> 24) & 0xFF;
  tbuf[8] = (size >> 16) & 0xFF;
  tbuf[9] = (size >> 8) & 0xFF;
  tbuf[10] = (size) & 0xFF;

  // Calcualte checksum for data portion
  for (i = 3; i < CMD_DOWNLOAD_SIZE; i++) {
    tbuf[1] += tbuf[i];
  }

  tcount = CMD_DOWNLOAD_SIZE;
  rcount = 0;
  rc = i2c_rdwr_msg_transfer(ifd, addr, tbuf, tcount, rbuf, rcount);
  if (rc) {
    printf("i2c_rdwr_msg_transfer failed download\n");
    goto error_exit;
  }

  // Delay for download command process
  msleep(500);

  tcount = 0;
  rcount = 2;
  rc = i2c_rdwr_msg_transfer(ifd, addr, tbuf, tcount, rbuf, rcount);
  if (rc) {
    printf("i2c_rdwr_msg_transfer failed download ack\n");
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
    tbuf[1] = MCU_CMD_STATUS;  // checksum same as data
    tbuf[2] = MCU_CMD_STATUS;

    tcount = CMD_STATUS_SIZE;
    rcount = 0;
    rc = i2c_rdwr_msg_transfer(ifd, addr, tbuf, tcount, rbuf, rcount);
    if (rc) {
      printf("i2c_rdwr_msg_transfer failed get status\n");
      goto error_exit;
    }

    tcount = 0;
    rcount = 5;
    rc = i2c_rdwr_msg_transfer(ifd, addr, tbuf, tcount, rbuf, rcount);
    if (rc) {
      printf("i2c_rdwr_msg_transfer failed get status ack\n");
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

    // Send ACK
    tbuf[0] = 0xcc;
    tcount = 1;
    rcount = 0;
    rc = i2c_rdwr_msg_transfer(ifd, addr, tbuf, tcount, rbuf, rcount);
    if (rc) {
      printf("i2c_rdwr_msg_transfer failed send ack\n");
      goto error_exit;
    }

    // send next packet
    if((size - offset) < MCU_PKT_MAX ) {
      xcount = read(fd, &tbuf[3], (size - offset));  
    } else {
      xcount = read(fd, &tbuf[3], MCU_PKT_MAX);
    }
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
#ifdef DEBUG
    printf("offset =%d\n", offset);
#endif
    if ((last_offset + dsize) <= offset) {
       printf("\rupdated fw: %d %%", offset/dsize*5);
       fflush(stdout);
       last_offset += dsize;
    }

    tbuf[0] = xcount + 3;
    tbuf[1] = MCU_CMD_DATA;
    tbuf[2] = MCU_CMD_DATA;
    for (i = 3; i < tbuf[0]; i++) {
      tbuf[1] += tbuf[i];
    }

    tcount = tbuf[0];
    rcount = 0;
    rc = i2c_rdwr_msg_transfer(ifd, addr, tbuf, tcount, rbuf, rcount);
    if (rc) {
      printf("i2c_rdwr_msg_transfer failed send data\n");
      goto error_exit;
    }

    msleep(10);
    tcount = 0;
    rcount = 2;
    rc = i2c_rdwr_msg_transfer(ifd, addr, tbuf, tcount, rbuf, rcount);
    if (rc) {
      printf("i2c_rdwr_msg_transfer failed send data ack\n");
      goto error_exit;
    }

    if (rbuf[0] != 0x00 || rbuf[1] != 0xcc) {
      printf("data error: %x:%x\n", rbuf[0], rbuf[1]);
      goto error_exit;
    }
  }

  // Run the new image
  tbuf[0] = CMD_RUN_SIZE;
  tbuf[1] = MCU_CMD_RUN; //checksum
  tbuf[2] = MCU_CMD_RUN;
  tbuf[3] = (MCU_FLASH_START >> 24) & 0xFF;
  tbuf[4] = (MCU_FLASH_START >> 16) & 0xFF;
  tbuf[5] = (MCU_FLASH_START >> 8) & 0xFF;
  tbuf[6] = MCU_FLASH_START & 0xFF;

  for (i = 3; i < CMD_RUN_SIZE; i++) {
    tbuf[1] += tbuf[i];
  }

  tcount = CMD_RUN_SIZE;
  rcount = 0;
  rc = i2c_rdwr_msg_transfer(ifd, addr, tbuf, tcount, rbuf, rcount);
  if (rc) {
    printf("i2c_rdwr_msg_transfer failed for run\n");
    goto error_exit;
  }

  tcount = 0;
  rcount = 2;
  rc = i2c_rdwr_msg_transfer(ifd, addr, tbuf, tcount, rbuf, rcount);
  if (rc) {
    printf("i2c_rdwr_msg_transfer failed run ack\n");
    goto error_exit;
  }

  if (rbuf[0] != 0x00 || rbuf[1] != 0xcc) {
    printf("run response: %x:%x\n", rbuf[0], rbuf[1]);
    goto error_exit;
  }

  sleep(2);
  ret = 0;

error_exit:
  // Restart ipmbd
  printf("\n");
  snprintf(cmd, sizeof(cmd), "sv start ipmbd_%d", bus);
  if (system(cmd)) {
    syslog(LOG_CRIT, "Starting ipmbd on bus %d failed\n", bus);
  }

  close(fd);
  close(ifd);

  return ret;
}

static int
mcu_update_fw(uint8_t bus, uint8_t addr, uint8_t target, uint32_t offset, uint16_t len, uint8_t *buf) {
  uint8_t tbuf[256] = {0x15, 0xA0, 0x00};
  uint8_t rbuf[16] = {0x00};
  uint16_t tlen = 0;
  uint8_t rlen = sizeof(rbuf);
  int ret = -1;
  int retries = 3;

  // Fill the component for which firmware is requested
  tbuf[3] = target;
  memcpy(&tbuf[4], &offset, sizeof(uint32_t));
  memcpy(&tbuf[8], &len, sizeof(uint16_t));
  memcpy(&tbuf[10], buf, len);
  tlen = len + 10;

  while (retries--) {
    ret = mcu_ipmb_wrapper(bus, addr, NETFN_OEM_1S_REQ, CMD_OEM_1S_UPDATE_FW, tbuf, tlen, rbuf, &rlen);
    if (!ret) {
      break;
    }

    if (retries) {
      sleep(1);
      printf("\n%s: %d[%02x], target %d, offset %d, len %d retrying...\n", __func__ , bus, addr, target, offset, len);
    }
  }

  return ret;
}

int
mcu_update_bootloader(uint8_t bus, uint8_t addr, uint8_t target, const char *path) {
  int fd, ret;
  uint32_t dsize, offset, last_offset;
  uint16_t count, read_count;
  uint8_t buf[256] = {0};
  struct stat st;

  fd = open(path, O_RDONLY, 0666);
  if (fd < 0) {
    printf("ERROR: invalid file path!\n");
    return -1;
  }

  fstat(fd, &st);
  dsize = st.st_size/20;

  // Write chunks of binary data in a loop
  offset = 0;
  last_offset = 0;
  read_count = IPMB_WRITE_COUNT_MAX;

  while (1) {
    // Read from file
    count = read(fd, buf, read_count);
    if (count <= 0) {
      break;
    }

    if ((offset + count) >= st.st_size) {
      target |= 0x80;
    }

    ret = mcu_update_fw(bus, addr, target, offset, count, buf);
    if (ret) {
      printf("%s: update failed\n", __func__);
      break;
    }

    // Update counter
    offset += count;
    if ((last_offset + dsize) <= offset) {
      printf("\rupdated bootloader: %d %%   ", (offset/dsize)*5);
      fflush(stdout);
      last_offset += dsize;
    }
  }
  printf("\n");
  close(fd);

  ret = (offset >= st.st_size) ? 0 : -1;
  return ret;
}

int
usb_dbg_reset_ioexp(uint8_t bus, uint8_t addr) {
  int fd, ret = -1;
  char fn[32];
  uint8_t tbuf[8] = {0};
  uint8_t rbuf[8] = {0};

  snprintf(fn, sizeof(fn), "/dev/i2c-%d", bus);
  fd = open(fn, O_RDWR);
  if (fd < 0) {
    syslog(LOG_ERR, "%s: i2c open failed for bus#%d", __func__, bus);
    return -1;
  }

  // Set Configuration register
  tbuf[0] = 0x06;
  tbuf[1] = 0xFF;
  tbuf[2] = 0xFF;
  ret = i2c_rdwr_msg_transfer(fd, addr, tbuf, 3, rbuf, 0);
  if (ret < 0) {
    syslog(LOG_ERR, "%s: reset IO exp failed, bus=%d, addr=%02X, ", __func__, bus, addr);
  }

  close(fd);
  return ret;
}
