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
#include "i2c.h"
#include "i2c-dev.h"


#define MCU_BUS_ID 0x9
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
#define MCU_addr 0x60
#define IPMB_WRITE_COUNT_MAX 224
#define PCA9555_addr 0x4E

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



// Helper Functions
static int
i2c_io(int fd, uint8_t addr, uint8_t *tbuf, uint8_t tcount, uint8_t *rbuf, uint8_t rcount)
{
  struct i2c_rdwr_ioctl_data data;
  struct i2c_msg msg[2];
  int n_msg = 0;
  int rc;

  memset(&msg, 0, sizeof(msg));

  if (tcount) {
    msg[n_msg].addr = addr >> 1;
    msg[n_msg].flags = 0;
    msg[n_msg].len = tcount;
    msg[n_msg].buf = tbuf;
    n_msg++;
  }

  if (rcount) {
    msg[n_msg].addr = addr >> 1;
    msg[n_msg].flags = I2C_M_RD;
    msg[n_msg].len = rcount;
    msg[n_msg].buf = rbuf;
    n_msg++;
  }

  data.msgs = msg;
  data.nmsgs = n_msg;

  rc = ioctl(fd, I2C_RDWR, &data);
  if (rc < 0) {
    // syslog(LOG_ERR, "Failed to do raw io");
    return -1;
  }
  return 0;
}

static int
enable_MCU_update(void)
{
  uint8_t tbuf[256] = {0x00};
  uint8_t rbuf[256] = {0x00};
  ipmb_req_t *req;
  uint8_t tlen = 0;
  uint8_t rlen = 0;

  req = (ipmb_req_t*)tbuf;

  req->res_slave_addr = MCU_addr;
  req->netfn_lun = NETFN_OEM_1S_REQ<<2;
  req->hdr_cksum = req->res_slave_addr + req->netfn_lun;
  req->hdr_cksum = ZERO_CKSUM_CONST - req->hdr_cksum;

  req->req_slave_addr = 0x20;
  req->seq_lun = 0x00;

  rlen = 0;
  memset( rbuf,0x00,sizeof(rbuf) );
  req->cmd = CMD_OEM_1S_ENABLE_BIC_UPDATE;
  req->data[0] = 0x15;
  req->data[1] = 0xA0;
  req->data[2] = 0x00;
  req->data[3] = 0x1;
  tlen = 10;

  lib_ipmb_handle(MCU_BUS_ID, tbuf, tlen+1, rbuf, &rlen);
  if (rlen == 0) {
    syslog(LOG_DEBUG, "%s(%d): Zero bytes received from MCU\n", __func__, __LINE__);
  }
#ifdef DEBUG
  syslog(LOG_WARNING, "%s(%d): reset MCU, ComCode:%x\n", __func__, __LINE__,rbuf[6]);
#endif

  return rlen;
}

int
usb_dbg_update_fw(char *path)
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
  int ret;
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
  snprintf(fn, sizeof(fn), "/dev/i2c-%d", MCU_BUS_ID);
  ifd = open(fn, O_RDWR);
  if (ifd < 0) {
    syslog(LOG_WARNING, "%s(%d): i2c_open failed for bus#%d\n",
            __func__, __LINE__, MCU_BUS_ID);
    goto error_exit2;
  }

  // Enable Bridge-IC update
  enable_MCU_update();

  // Kill ipmb daemon
  memset(cmd, 0, sizeof(cmd));
  sprintf(cmd, "ps | grep -v 'grep' | grep 'ipmbd %d' |awk '{print $1}'| xargs kill", MCU_BUS_ID);
  system(cmd);
  printf("killed ipmbd %x..\n",MCU_BUS_ID);

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

  ret = i2c_io(ifd, MCU_addr, tbuf, tcount, rbuf, rcount);
  if (ret) {
    printf("i2c_io failed download\n");
    goto error_exit;
  }

  //delay for download command process ---
  msleep(500);
  tcount = 0;
  rcount = 2;
  ret = i2c_io(ifd, MCU_addr, tbuf, tcount, rbuf, rcount);
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
    tbuf[1] = MCU_CMD_STATUS;// checksum same as data
    tbuf[2] = MCU_CMD_STATUS;

    tcount = CMD_STATUS_SIZE;

    rcount = 5;

    ret = i2c_io(ifd, MCU_addr, tbuf, tcount, rbuf, rcount);
    if (ret) {
      printf("i2c_io failed - MCU_CMD_STATUS\n");
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
    ret = i2c_io(ifd, MCU_addr, tbuf, tcount, rbuf, rcount);
    if (ret) {
      printf("i2c_io failed, Send ACK\n");
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
       printf("updated bic: %d %%\n", offset/dsize*5);
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

    ret = i2c_io(ifd, MCU_addr, tbuf, tcount, rbuf, rcount);
    if (ret) {
      printf("i2c_io error\n");
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

  ret = i2c_io(ifd, MCU_addr, tbuf, tcount, rbuf, rcount);
  if (ret) {
    printf("i2c_io failed for run\n");
    goto error_exit;
  }

  if (rbuf[0] != 0x00 || rbuf[1] != 0xcc) {
    printf("run response: %x:%x\n", rbuf[0], rbuf[1]);
    goto error_exit;
  }

error_exit:
  // Restart ipmbd daemon
  memset(cmd, 0, sizeof(cmd));
  sprintf(cmd, "/usr/local/bin/ipmbd %d 0x30", MCU_BUS_ID);
  system(cmd);

error_exit2:
  if (fd > 0) {
    close(fd);
  }
  if (ifd > 0) {
    close(fd);
  }

  return 0;
}

static int
update_MCU_bl_fw(uint8_t target, uint32_t offset, uint16_t len, uint8_t *buf)
{
  uint8_t tbuf[256] = {0x00};
  uint8_t rbuf[256] = {0x00};
  ipmb_req_t *req;
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
  lib_ipmb_handle(MCU_BUS_ID, tbuf, tlen+1, rbuf, &rlen);
  if ((rlen == 0) && (retries--)) {
    sleep(1);
    syslog(LOG_DEBUG, "%s(%d): target %d, offset: %d, len: %d retrying..\n", __func__, __LINE__, target, offset, len);
    goto bic_send;
  }

  return rlen;
}

int
usb_dbg_update_boot_loader(char *path)
{
  int ret;
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
  uint8_t tbuf[2] = {0};
  uint8_t rbuf[2] = {0};
  int ret;

  snprintf(fn, sizeof(fn), "/dev/i2c-%d", MCU_BUS_ID);
  fd = open(fn, O_RDWR);
  if (fd < 0) {
    syslog(LOG_ERR, "%s(%d) : open fails for path: %s\n", __func__, __LINE__, fn);
    goto error_exit;
  }
  //Set Configuration register
  tbuf[0] = 0x06; tbuf[1] = 0xFF; tbuf[2] = 0xFF;
  ret = i2c_io(fd, PCA9555_addr, tbuf, 3, rbuf, 0);
  if (ret < 0) {
    syslog(LOG_ERR, "%s(%d) : reset_PCA9555 fail.\n", __func__, __LINE__ );
    goto error_exit;
  }
error_exit:
  if (fd > 0 ) {
    close(fd);
  }
}


