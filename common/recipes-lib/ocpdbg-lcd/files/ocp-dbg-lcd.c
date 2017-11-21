/*
 *
 * Copyright 2017-present Facebook. All Rights Reserved.
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
#include <stdlib.h>
#include <stdint.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <syslog.h>
#include <unistd.h>
#include <time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <openbmc/ipmb.h>
#include <openbmc/ipmi.h>
#include <openbmc/obmc-i2c.h>

#define MCU_UPDATE_RETRIES 12
#define MCU_UPDATE_TIMEOUT 500

#define MCU_FLASH_START 0x8000
#define MCU_PKT_MAX 252

#define MCU_CMD_DOWNLOAD 0x21
#define MCU_CMD_RUN 0x22
#define MCU_CMD_STATUS 0x23
#define MCU_CMD_DATA 0x24

#define CMD_DOWNLOAD_SIZE 11
#define CMD_RUN_SIZE 7
#define CMD_STATUS_SIZE 3
#define CMD_DATA_SIZE 0xFF
#define IPMB_WRITE_COUNT_MAX 224

static uint8_t io_expander_addr = 0x4E;
static uint8_t mcu_bus_id       = 0x9;
static uint8_t MCU_addr         = 0x60;

// Helper function for msleep
static void
msleep(int msec) {
  struct timespec req;

  req.tv_sec = 0;
  req.tv_nsec = msec * 1000 * 1000;

  while(nanosleep(&req, &req) == -1 && errno == EINTR) {
    continue;
  }
}

int usb_dbg_init(uint8_t bus, uint8_t mcu_addr, uint8_t io_exp_addr)
{
  mcu_bus_id       = bus;
  MCU_addr         = mcu_addr;
  io_expander_addr = io_exp_addr;
  return 0;
}

static int
mcu_ipmb_wrapper(uint8_t netfn, uint8_t cmd, uint8_t *txbuf, uint8_t txlen, uint8_t *rxbuf, uint8_t *rxlen)
{
  ipmb_req_t *req;
  ipmb_res_t *res;
  uint8_t rbuf[MAX_IPMB_RES_LEN] = {0};
  uint8_t tbuf[MAX_IPMB_RES_LEN] = {0};
  uint8_t tlen = 0;
  uint8_t rlen = 0;

  req = (ipmb_req_t *)tbuf;
  req->res_slave_addr = MCU_addr;
  req->netfn_lun = netfn << LUN_OFFSET;
  req->hdr_cksum = req->res_slave_addr + req->netfn_lun;
  req->hdr_cksum = ZERO_CKSUM_CONST - req->hdr_cksum;

  req->req_slave_addr = BMC_SLAVE_ADDR << 1;
  req->seq_lun = 0x00;
  req->cmd = cmd;

  // copy the data to be send
  if (txlen) {
    memcpy(req->data, txbuf, txlen);
  }

  tlen = IPMB_HDR_SIZE + IPMI_REQ_HDR_SIZE + txlen;

  // Invoke IPMB library handler
  lib_ipmb_handle(mcu_bus_id, tbuf, tlen, rbuf, &rlen);

  if (rlen == 0) {
    syslog(LOG_DEBUG, "mcu_ipmb_wrapper: Zero bytes received\n");
    return -1;
  }

  // Handle IPMB response
  res = (ipmb_res_t *)rbuf;

  if (res->cc) {
#ifdef DEBUG
    syslog(LOG_ERR, "mcu_ipmb_wrapper: Completion Code: 0x%X\n", res->cc);
#endif
    return -1;
  }

  // copy the received data back to caller
  if ((rlen -= (IPMB_HDR_SIZE + IPMI_RESP_HDR_SIZE)) > *rxlen) {
    return -1;
  }
  *rxlen = rlen;
  memcpy(rxbuf, res->data, *rxlen);

  return 0;
}

static int
enable_MCU_update(void)
{
  uint8_t tbuf[4] = {0x15, 0xA0, 0x00};
  uint8_t rbuf[16] = {0x00};
  uint8_t rlen = sizeof(rbuf);
  int ret;

  // 0x1: Update on I2C Channel, the only available option from BMC
  tbuf[3] = 0x1;

  ret = mcu_ipmb_wrapper(NETFN_OEM_1S_REQ, CMD_OEM_1S_ENABLE_BIC_UPDATE, tbuf, 4, rbuf, &rlen);

  return ret;
}

int
usb_dbg_get_fw_ver(uint8_t comp, uint8_t *ver) {
  uint8_t rbuf[32] = {0x00};
  uint8_t rlen = sizeof(rbuf);
  int ret = -1;

  switch (comp) {
    case 0x00:  // runtime firmware version
      ret = mcu_ipmb_wrapper(NETFN_APP_REQ, CMD_APP_GET_DEVICE_ID, NULL, 0, rbuf, &rlen);
      if (!ret) {
        memcpy(ver, &rbuf[2], 2);
      }
      break;
    case 0x01:  // boot-loader version
      ret = mcu_ipmb_wrapper(NETFN_OEM_REQ, 0x40, NULL, 0, rbuf, &rlen);
      if (!ret) {
        memcpy(ver, &rbuf[0], 2);
      }
      break;
  }

  return ret;
}

int
usb_dbg_update_fw(char *path, uint8_t en_mcu_upd)
{
  int fd = -1;
  int ifd = -1;
  char cmd[100] = {0};
  struct stat buf;
  int size;
  uint8_t tbuf[256] = {0};
  uint8_t rbuf[16] = {0};
  uint8_t xbuf[256] = {0};
  uint8_t tcount;
  uint8_t rcount;
  volatile int xcount;
  int i = 0;
  int ret = 0;
  uint32_t offset = 0;
  uint32_t last_offset = 0;
  uint32_t dsize;
  char fn[32];

  // Open the file exclusively for read
  fd = open(path, O_RDONLY, 0666);
  if (fd < 0) {
    syslog(LOG_ERR, "%s(%d) : open fails for path: %s\n",
          __func__, __LINE__, path);
    goto error_exit;
  }

  fstat(fd, &buf);
  size = buf.st_size;
  printf("size of file is %d bytes\n", size);
  dsize = size/20;

  // Open the i2c driver
  snprintf(fn, sizeof(fn), "/dev/i2c-%d", mcu_bus_id);
  ifd = open(fn, O_RDWR);
  if (ifd < 0) {
    syslog(LOG_WARNING, "%s(%d): i2c_open failed for bus#%d\n",
            __func__, __LINE__, mcu_bus_id);
    goto error_exit2;
  }

  // Enable Bridge-IC update
  if (en_mcu_upd) {
    enable_MCU_update();
  }

  // Kill ipmb daemon
  memset(cmd, 0, sizeof(cmd));
  sprintf(cmd, "sv stop ipmbd_%d", mcu_bus_id);
  system(cmd);
  printf("Stopped ipmbd %d..\n",mcu_bus_id);

  //Waiting for MCU reset to bootloader for updating
  sleep(1);

  // Start Bridge IC update(0x21)

  tbuf[0] = CMD_DOWNLOAD_SIZE;
  tbuf[1] = 0x00; //Checksum, will fill later

  tbuf[2] = MCU_CMD_DOWNLOAD;

  // update flash address: 0x8000
  tbuf[3] = (MCU_FLASH_START >> 24) & 0xff;
  tbuf[4] = (MCU_FLASH_START >> 16) & 0xff;
  tbuf[5] = (MCU_FLASH_START >> 8) & 0xff;
  tbuf[6] = (MCU_FLASH_START) & 0xff;

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

  ret = i2c_rdwr_msg_transfer(ifd, MCU_addr, tbuf, tcount, rbuf, rcount);
  if (ret) {
    printf("i2c_rdwr_msg_transfer failed download\n");
    goto error_exit;
  }

  //delay for download command process ---
  msleep(500);
  tcount = 0;
  rcount = 2;
  ret = i2c_rdwr_msg_transfer(ifd, MCU_addr, tbuf, tcount, rbuf, rcount);
  if (ret) {
    printf("i2c_rdwr_msg_transfer failed download ack\n");
    goto error_exit;
  }

  if (rbuf[0] != 0x00 || rbuf[1] != 0xcc) {
    printf("download response: %x:%x\n", rbuf[0], rbuf[1]);
    ret = -1;
    goto error_exit;
  }

  // Loop to send all the image data
  while (1) {
    // Get Status
    tbuf[0] = CMD_STATUS_SIZE;
    tbuf[1] = MCU_CMD_STATUS;// checksum same as data
    tbuf[2] = MCU_CMD_STATUS;

    tcount = CMD_STATUS_SIZE;

    rcount = 5;

    ret = i2c_rdwr_msg_transfer(ifd, MCU_addr, tbuf, tcount, rbuf, rcount);
    if (ret) {
      printf("i2c_rdwr_msg_transfer failed - MCU_CMD_STATUS\n");
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
    ret = i2c_rdwr_msg_transfer(ifd, MCU_addr, tbuf, tcount, rbuf, rcount);
    if (ret) {
      printf("i2c_rdwr_msg_transfer failed, Send ACK\n");
      goto error_exit;
    }

    // send next packet

    xcount = read(fd, xbuf, MCU_PKT_MAX);
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
       printf("updated debug card FW: %d %%\n", offset/dsize*5);
       last_offset += dsize;
    }

    tbuf[0] = xcount + 3;
    tbuf[1] = MCU_CMD_DATA;
    tbuf[2] = MCU_CMD_DATA;
    memcpy(&tbuf[3], xbuf, xcount);

    for (i = 0; i < xcount; i++) {
      tbuf[1] += xbuf[i];
    }

    tcount = tbuf[0];
    rcount = 2;

    ret = i2c_rdwr_msg_transfer(ifd, MCU_addr, tbuf, tcount, rbuf, rcount);
    if (ret) {
      printf("i2c_rdwr_msg_transfer error\n");
      goto error_exit;
    }

    if (rbuf[0] != 0x00 || rbuf[1] != 0xcc) {
      printf("data error: %x:%x\n", rbuf[0], rbuf[1]);
      goto error_exit;
    }
  }

  // Run the new image
  tbuf[0] = CMD_RUN_SIZE;
  tbuf[1] = 0x00; //checksum
  tbuf[2] = MCU_CMD_RUN;
  tbuf[3] = (MCU_FLASH_START >> 24) & 0xff;
  tbuf[4] = (MCU_FLASH_START >> 16) & 0xff;
  tbuf[5] = (MCU_FLASH_START >> 8) & 0xff;
  tbuf[6] = (MCU_FLASH_START) & 0xff;

  for (i = 2; i < CMD_RUN_SIZE; i++) {
    tbuf[1] += tbuf[i];
  }

  tcount = CMD_RUN_SIZE;

  rcount = 2;

  ret = i2c_rdwr_msg_transfer(ifd, MCU_addr, tbuf, tcount, rbuf, rcount);
  if (ret) {
    printf("i2c_rdwr_msg_transfer failed for run\n");
    goto error_exit;
  }

  if (rbuf[0] != 0x00 || rbuf[1] != 0xcc) {
    printf("run response: %x:%x\n", rbuf[0], rbuf[1]);
    goto error_exit;
  }

error_exit:
  // Restart ipmbd daemon
  memset(cmd, 0, sizeof(cmd));
  sprintf(cmd, "sv start ipmbd_%d", mcu_bus_id);
  system(cmd);

error_exit2:
  if (fd > 0) {
    close(fd);
  }
  if (ifd > 0) {
    close(ifd);
  }

  return ret;
}

static int
update_MCU_bl_fw(uint8_t target, uint32_t offset, uint16_t len, uint8_t *buf)
{
  uint8_t tbuf[256] = {0x00};
  uint8_t rbuf[256] = {0x00};
  ipmb_req_t *req;
  ipmb_res_t *res;
  uint8_t tlen = 0;
  uint8_t rlen = 0;
  int retries = 3;

  req = (ipmb_req_t*)tbuf;

  req->res_slave_addr = MCU_addr;
  req->netfn_lun = NETFN_OEM_1S_REQ<<2;
  req->hdr_cksum = req->res_slave_addr + req->netfn_lun;
  req->hdr_cksum = ZERO_CKSUM_CONST - req->hdr_cksum;

  req->req_slave_addr = 0x20;
  req->seq_lun = 0x00;

  rlen = 0;
  memset( rbuf,0x00,sizeof(rbuf) );
  req->cmd = CMD_OEM_1S_UPDATE_FW;
  req->data[0] = 0x15;
  req->data[1] = 0xA0;
  req->data[2] = 0x00;
  // Fill the component for which firmware is requested
  req->data[3] = target;
  req->data[4] = (offset) & 0xFF;
  req->data[5] = (offset >> 8) & 0xFF;
  req->data[6] = (offset >> 16) & 0xFF;
  req->data[7] = (offset >> 24) & 0xFF;

  req->data[8] = len & 0xFF;
  req->data[9] = (len >> 8) & 0xFF;

  memcpy(&req->data[10], buf, len);

  tlen = len + 16;

bic_send:
  lib_ipmb_handle(mcu_bus_id, tbuf, tlen+1, rbuf, &rlen);
  if ((rlen == 0) && (retries--)) {
    sleep(1);
    syslog(LOG_DEBUG, "%s(%d): target %d, offset: %d, len: %d retrying..\n", __func__, __LINE__, target, offset, len);
    goto bic_send;
  }

  res = (ipmb_res_t *)rbuf;
  if (rlen && res->cc) {
    syslog(LOG_DEBUG, "%s(%d): Completion Code: 0x%X\n", __func__, __LINE__, res->cc);
    return 0;
  }

  return rlen;
}

int
usb_dbg_update_boot_loader(char *path)
{
  int ret = -1;
  uint32_t offset;
  uint16_t count, read_count;
  uint8_t buf[256] = {0};
  uint8_t target;
  int fd;
  uint32_t dsize, last_offset;
  struct stat st;

  // Open the file exclusively for read
  fd = open(path, O_RDONLY, 0666);
  if (fd < 0) {
#ifdef DEBUG
    syslog(LOG_ERR, "%s(%d) : open fails for path: %s\n", __func__, __LINE__, path);
#endif
    goto error_exit;
  }
  stat(path, &st);
  dsize = st.st_size/20;

  // Write chunks of binary data in a loop
  offset = 0;
  last_offset = 0;

  while (1) {
    read_count = IPMB_WRITE_COUNT_MAX;

    // Read from file
    count = read(fd, buf, read_count);
    if (count <= 0) {
      break;
    }
    if( count < read_count )
      target = 2| 0x80; //UPDATE_BIC_BOOTLOADER
    else
      target = 2;

    // Send data to Bridge-IC
    ret = update_MCU_bl_fw(target, offset, count, buf);
    if (ret == 0) {
      printf("%s(%d) : update MCU bl fw fail %d \n", __func__, __LINE__,count);
      break;
    }

    // Update counter
    offset += count;
    if((last_offset + dsize) <= offset) {
      printf("updated MCU boot loader: %d %%\n", offset/dsize*5);
      last_offset += dsize;
    }
  }

error_exit:
  if (fd > 0 ) {
    close(fd);
  }

  return ret;
}

void
usb_dbg_reset(void)
{
  int fd = 0;
  char fn[32];
  uint8_t tbuf[3] = {0};
  uint8_t rbuf[2] = {0};
  int ret;

  snprintf(fn, sizeof(fn), "/dev/i2c-%d", mcu_bus_id);
  fd = open(fn, O_RDWR);
  if (fd < 0) {
    syslog(LOG_ERR, "%s(%d) : open fails for path: %s\n", __func__, __LINE__, fn);
    goto error_exit;
  }
  //Set Configuration register
  tbuf[0] = 0x06; tbuf[1] = 0xFF; tbuf[2] = 0xFF;
  ret = i2c_rdwr_msg_transfer(fd, io_expander_addr, tbuf, 3, rbuf, 0);
  if (ret < 0) {
    syslog(LOG_ERR, "%s(%d) : reset_PCA9555 fail.\n", __func__, __LINE__ );
    goto error_exit;
  }
error_exit:
  if (fd > 0 ) {
    close(fd);
  }
}


