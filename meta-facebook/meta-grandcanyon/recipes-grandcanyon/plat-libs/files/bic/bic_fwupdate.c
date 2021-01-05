
/*
 *
 * Copyright 2020-present Facebook. All Rights Reserved.
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
#include <sys/mman.h>
#include "bic_fwupdate.h"
#include "bic_bios_fwupdate.h"
#include "bic_vr_fwupdate.h"
//#define DEBUG

/****************************/
/*      BIC fw update       */
/****************************/
#define I2C_UPDATE_BIC 0x01

#define BIC_CMD_DOWNLOAD 0x21
#define BIC_CMD_DOWNLOAD_SIZE 11

#define BIC_CMD_RUN 0x22
#define BIC_CMD_RUN_SIZE 7

#define BIC_CMD_STS 0x23
#define BIC_CMD_STS_SIZE 3

#define BIC_CMD_DATA 0x24
#define BIC_CMD_DATA_SIZE 0xFF

#define BIC_FLASH_START 0x8000

#define BIC_UPDATE_BLOCK_SIZE 20
#define BIC_BOOTLOADER_COMP_BIT 7

#define IPMB_MAX_SEND 224

#define I2C_CLK_CTRL_REG 0x04
#define PAGE_SIZE        0x1000

#define START_BIC_UPDATE_DATA_LEN 11
#define BIC_ACK_VALIDATE_LEN 2
#define BIC_UPDATE_STAT_VALIDATE_LEN 5
#define BIC_UPDATE_COMPLETE_LEN 7
#define BIC_IMG_DATA_HEADER_LEN 3
#define BIC_IMG_DATA_LEN 256
#define BIC_VALIDATE_READ_LEN 2

#define MAX_SYS_CMD_REQ_LEN  100  // include the string terminal
#define MAX_SYS_CMD_RESP_LEN 100  // include the string terminal

#define GET_BIC_UPDATE_STAT 0xCC

#define DEVMEM_READ_CMD  "/sbin/devmem 0x%08x | cut -c 3-" // skip "0x"
#define DEVMEM_WRITE_CMD "/sbin/devmem 0x%08x w 0x%08x"


// I2C frequncy
enum {
  I2C_100K = 0x0,
  I2C_1M
};

#ifdef DEBUG
static void print_data(const char *name, uint8_t netfn, uint8_t cmd, uint8_t *buf, uint8_t len) {
  int i = 0;

  printf("[%s][%d]send: 0x%x 0x%x ", name, len, netfn, cmd);
  for ( i = 0; i < len; i++) {
    printf("0x%x ", buf[i]);
  }

  printf("\n");
}
#endif

static uint8_t
get_bic_fw_checksum(uint8_t *buf, uint8_t len) {
  int i = 0;
  uint8_t result = 0;

  for (i = 0; i < len; i++) {
    result += buf[i];
  }

  return result;
}

static int
enable_bic_update() {
  uint8_t tbuf[MAX_IPMB_BUFFER] = {0x00};
  uint8_t rbuf[MAX_IPMB_BUFFER] = {0x00};
  uint8_t tlen = 0;
  uint8_t rlen = 0;
  uint8_t netfn = 0;
  uint8_t cmd = 0;
  int ret = -1;

  memcpy(tbuf, (uint8_t *)&IANA_ID, 3);

  netfn = NETFN_OEM_1S_REQ;
  cmd = CMD_OEM_1S_ENABLE_BIC_UPDATE;
  tbuf[3] = I2C_UPDATE_BIC; //Update bic via i2c
  tlen = 4;

#ifdef DEBUG
  print_data(__func__, netfn, cmd, tbuf, tlen);
#endif

  ret = bic_ipmb_wrapper(netfn, cmd, tbuf, tlen, rbuf, &rlen);
  if ( ret < 0 ) {
    syslog(LOG_WARNING, "%s() Cannot enable bic fw update", __func__);
    return ret;
  }

#ifdef DEBUG
  print_data(__func__, netfn, cmd, rbuf, rlen);
#endif

  return ret;
}

static int
send_start_bic_update(int i2cfd, int size) {
  uint8_t data[START_BIC_UPDATE_DATA_LEN] = 
                     { BIC_CMD_DOWNLOAD_SIZE,
                       BIC_CMD_DOWNLOAD_SIZE,
                            BIC_CMD_DOWNLOAD,
                      (BIC_FLASH_START >> 24) & 0xff,
                      (BIC_FLASH_START >> 16) & 0xff,
                      (BIC_FLASH_START >>  8) & 0xff,
                      (BIC_FLASH_START) & 0xff,
                      (size >> 24) & 0xff,
                      (size >> 16) & 0xff,
                      (size >>  8) & 0xff,
                      (size) & 0xff};
  uint8_t tbuf[MAX_IPMB_BUFFER] = {0x00};
  uint8_t rbuf[MAX_IPMB_BUFFER] = {0x00};
  uint8_t tlen = 0;
  uint8_t rlen = 0;
  int ret = -1;

  memcpy(tbuf, data, sizeof(data));
  tbuf[1] = get_bic_fw_checksum(&tbuf[2], BIC_CMD_DOWNLOAD_SIZE);
  tlen = BIC_CMD_DOWNLOAD_SIZE;
  ret = i2c_io(i2cfd, tbuf, tlen, rbuf, rlen);
  if (ret < 0) {
    printf("%s() can not get checksum via I2C, ret=%d\n", __func__, ret);
  }
  
  return ret;
}

static int
read_bic_update_ack_status(int i2cfd) {
  uint8_t tbuf[MAX_IPMB_BUFFER] = {0x00};
  uint8_t rbuf[MAX_IPMB_BUFFER] = {0x00};
  uint8_t tlen = 0;
  uint8_t rlen = 0;
  int ret = -1;
  uint8_t validate_data[BIC_ACK_VALIDATE_LEN] = {0x00, 0xCC};
  tlen = 0;
  rlen = 2;

  msleep(10);
  ret = i2c_io(i2cfd, tbuf, tlen, rbuf, rlen);
  if (ret != 0 || (memcmp(rbuf, validate_data, sizeof(validate_data)) != 0)) {
    printf("%s() response %x:%x, ret=%d\n", __func__, rbuf[0], rbuf[1], ret);
    return -1;
  }

  return ret;
}

static int
send_complete_signal(int i2cfd) {
  uint8_t data[BIC_UPDATE_COMPLETE_LEN] = 
                    { BIC_CMD_RUN_SIZE,
                      BIC_CMD_RUN_SIZE,
                      BIC_CMD_RUN,
                      (BIC_FLASH_START >> 24) & 0xff,
                      (BIC_FLASH_START >> 16) & 0xff,
                      (BIC_FLASH_START >>  8) & 0xff,
                      (BIC_FLASH_START) & 0xff};
  uint8_t tbuf[MAX_IPMB_BUFFER] = {0x00};
  uint8_t rbuf[MAX_IPMB_BUFFER] = {0x00};
  uint8_t tlen = 0;
  uint8_t rlen = 0;
  int ret = -1;

  memcpy(tbuf, data, sizeof(data));
  tbuf[1] = get_bic_fw_checksum(&tbuf[2], BIC_CMD_DOWNLOAD_SIZE);
  tlen = BIC_CMD_RUN_SIZE;
  rlen = 0;
  ret = i2c_io(i2cfd, tbuf, tlen, rbuf, rlen);

  if (ret < 0) {
    printf("Failed to run the new image\n");
  }

  return ret;
}

static int
read_bic_update_status(int i2cfd) {
  const uint8_t validate_data[BIC_UPDATE_STAT_VALIDATE_LEN] = {0x00, 0xCC, 0x03, 0x40, 0x40}; // validate data defined in BIC Bootloader
  uint8_t tbuf[MAX_IPMB_BUFFER] = {0};
  uint8_t rbuf[MAX_IPMB_BUFFER] = {0};
  uint8_t tlen = BIC_CMD_STS_SIZE;
  uint8_t rlen = 0;
  int ret = -1;

  tbuf[0] = BIC_CMD_STS_SIZE;
  tbuf[1] = BIC_CMD_STS;
  tbuf[2] = BIC_CMD_STS;
  ret = i2c_io(i2cfd, tbuf, tlen, rbuf, rlen);
  if (ret < 0) {
    printf("%s() failed to get status\n", __func__);
    goto exit;
  }

  tlen = 0;
  rlen = 5;
  ret = i2c_io(i2cfd, tbuf, tlen, rbuf, rlen);
  if (ret < 0) {
    printf("%s() failed to get status ack\n", __func__);
    goto exit;
  }

  if (memcmp(rbuf, validate_data, sizeof(validate_data)) != 0) {
    printf("%s() status: %x:%x:%x:%x:%x\n", __func__, rbuf[0], rbuf[1], rbuf[2], rbuf[3], rbuf[4]);
    goto exit;
  }

  tbuf[0] = GET_BIC_UPDATE_STAT;
  tlen = 1;
  rlen = 0;
  ret = i2c_io(i2cfd, tbuf, tlen, rbuf, rlen);
  if (ret < 0) {
    printf("%s() failed to send an ack\n", __func__);
    goto exit;
  }

exit:
  return ret;
}

static int
send_bic_image_data(int i2cfd, uint16_t len, uint8_t *buf) {
  uint8_t data[BIC_IMG_DATA_HEADER_LEN] = {0x00};
  uint8_t tbuf[MAX_IPMB_BUFFER] = {0x00};
  uint8_t rbuf[MAX_IPMB_BUFFER]  = {0x00};
  uint8_t tlen = 0;
  uint8_t rlen = 0;
  int ret = -1;

  data[0] = len + 3;
  data[1] = get_bic_fw_checksum(buf, len) + BIC_CMD_DATA;
  data[2] = BIC_CMD_DATA;
 
  memcpy(tbuf, data, sizeof(data));
  memcpy(&tbuf[3], buf, len);
  tlen = data[0];
  rlen = 0;
  ret = i2c_io(i2cfd, tbuf, tlen, rbuf, rlen);  

  if (ret < 0) {
    syslog(LOG_WARNING, "%s() Cannot send data of the BIC image", __func__);
  }

  return ret;
}

static int
send_bic_runtime_image_data(int fd, int i2cfd, int file_size, const uint8_t bytes_per_read) {
  uint8_t buf[BIC_IMG_DATA_LEN] = {0};
  uint8_t read_bytes = 0;
  int ret = -1;
  int dsize = 0;
  int last_offset = 0;
  int offset = 0;;

  dsize = file_size / 20;

  //reinit the fd to the beginning
  if (lseek(fd, 0, SEEK_SET) != 0) {
    syslog(LOG_WARNING, "%s() Cannot reinit the fd to the beginning. errstr=%s", __func__, strerror(errno));
    return -1;
  }

  while (1) {
    memset(buf, 0, sizeof(buf));
    read_bytes = read(fd, buf, bytes_per_read);
    if ( read_bytes <= 0 ) {
      //no more bytes can be read
      break;
    }

    offset += read_bytes;
    if ((last_offset + dsize) <= offset) {
      printf("updated bic: %d %%\n", (offset / dsize) * 5);
      fflush(stdout);
      last_offset += dsize;
    }

    ret = read_bic_update_status(i2cfd);
    if (ret < 0) {
      return ret;
    }

    ret = send_bic_image_data(i2cfd, read_bytes, buf);
    if (ret < 0) {
      return ret;
    }

    ret = read_bic_update_ack_status(i2cfd);
    if (ret < 0) {
      return ret;
    }
  }

  return ret;
}

static int
update_bic(int fd, int file_size) {
  int ret = -1;
  int i2cfd = 0;
  int i = 0;
  char cmd[MAX_SYS_CMD_REQ_LEN] = {0};
  char buf[MAX_SYS_CMD_RESP_LEN] = {0};
  size_t cmd_size = sizeof(cmd);
  uint8_t bus_num = 0;
  uint8_t ret_byte = 0;
  const uint8_t bytes_per_read = 252;
  struct rlimit mqlim;
  FILE* fp = NULL;
  uint32_t orig_i2c_ctl_reg = 0;
  uint32_t set_i2c_ctl_reg = 0;
#if 0
  uint32_t mmap_fd = 0;
  void *reg_base;
  void *reg_offset;
#endif

  //step1 -get the bus number and open the dev of i2c
  bus_num = I2C_BIC_BUS;
  syslog(LOG_CRIT, "%s: update bic firmware\n", __func__);

  i2cfd = i2c_open(bus_num, BRIDGE_SLAVE_ADDR);
  if (i2cfd < 0) {
    goto exit;
  }

  //step2 - kill ipmbd
  snprintf(cmd, cmd_size, "sv stop ipmbd_%d", bus_num);
  if (system(cmd) != 0) {
      syslog(LOG_WARNING, "[%s] %s failed\n", __func__, cmd);
      return BIC_STATUS_FAILURE;
  }
  printf("stop ipmbd for bus %d..\n", bus_num);

  //step3 - adjust the i2c speed to 100k and set properties of mqlim
// TODO: Use memory mapping here will cause segmentation fault. Use devmem instead as quick workaround. will find out the solution.
#if 0 
  mmap_fd = open("/dev/mem", O_RDWR | O_SYNC );
  if (mmap_fd < 0) {
    printf("%s(): fail to open /dev/mem\n", __func__);
    ret = BIC_STATUS_FAILURE;
    goto exit;
  }

  reg_base = mmap(NULL, PAGE_SIZE, PROT_READ|PROT_WRITE, MAP_SHARED, mmap_fd, I2C_BASE_MAP(bus_num));
  reg_offset = (char*) reg_base + I2C_CLK_CTRL_REG;
  orig_i2c_ctl_reg = *(volatile uint32_t*) reg_offset;
  // 1M: 0xXXXCBXX2 => 100K: 0xXXXFFXX5
  // Bit [3:0]: base clock divisor
  // Bit [15:12]: tCKLow
  // Bit [19:16]: tCKHigh
  set_i2c_ctl_reg = orig_i2c_ctl_reg & 0xFFF00FF0;
  set_i2c_ctl_reg |= 0x000FF005;
  *(volatile uint32_t*) reg_offset = set_i2c_ctl_reg; 
  munmap(reg_base, PAGE_SIZE);
#else
  
  snprintf(cmd, sizeof(cmd), DEVMEM_READ_CMD, I2C_BASE_MAP(bus_num) + I2C_CLK_CTRL_REG);
  if ((fp = popen(cmd, "r")) == NULL) {
    ret = BIC_STATUS_FAILURE;
    printf("%s(): command %s failed\n", __func__, cmd);
    goto exit;
  }
  
  if (fgets(buf, sizeof(buf), fp) != NULL) {
    for (i = 0; i < 8; i += 2) { // 8 characters for one word, 2 characters for one byte
      ret_byte = string_2_byte(&buf[i]);
      if (ret_byte < 0) {
        pclose(fp);
        ret = BIC_STATUS_FAILURE;
        goto exit;
      }
      orig_i2c_ctl_reg = (orig_i2c_ctl_reg << 8) | ret_byte;
    }
  }
  pclose(fp);

  // 1M: 0xXXXCBXX2 => 100K: 0xXXXFFXX5
  // Bit [3:0]: base clock divisor
  // Bit [15:12]: tCKLow
  // Bit [19:16]: tCKHigh
  set_i2c_ctl_reg = orig_i2c_ctl_reg & 0xFFF00FF0;
  set_i2c_ctl_reg |= 0x000FF005;
  snprintf(cmd, sizeof(cmd), DEVMEM_WRITE_CMD, I2C_BASE_MAP(bus_num) + I2C_CLK_CTRL_REG, set_i2c_ctl_reg);
  if (system(cmd) != 0) {
      printf("%s(): command %s failed\n", __func__, cmd);
      ret = BIC_STATUS_FAILURE;
      goto exit;
  }
#endif
  sleep(1);

  if (is_bic_ready() < 0) {
    printf("BIC is not ready after sleep 1s\n");
    goto exit;
  }

  mqlim.rlim_cur = RLIM_INFINITY;
  mqlim.rlim_max = RLIM_INFINITY;
  if (setrlimit(RLIMIT_MSGQUEUE, &mqlim) < 0) {
    goto exit;
  }

  snprintf(cmd, cmd_size, "/usr/local/bin/ipmbd -u %d %d > /dev/null 2>&1 &", bus_num, PAYLOAD_BIC);
  if (system(cmd) != 0) {
      syslog(LOG_WARNING, "[%s] %s failed\n", __func__, cmd);
      return BIC_STATUS_FAILURE;
  }
  printf("start ipmbd -u for BIC..\n");

  //assume ipmbd that it will be ready in 2s
  sleep(2);

  //step4 - enable bic update
  ret = enable_bic_update();
  if (ret < 0) {
    syslog(LOG_WARNING, "%s() Failed to enable the bic update", __func__);
    goto exit;
  }

  //step5 - kill ipmbd
  snprintf(cmd, cmd_size, "ps -w | grep -v 'grep' | grep 'ipmbd -u %d' |awk '{print $1}'| xargs kill", bus_num);
  if (system(cmd) != 0) {
      syslog(LOG_WARNING, "[%s] %s failed\n", __func__, cmd);
      return BIC_STATUS_FAILURE;
  }
  printf("stop ipmbd -u for BIC..\n");

  //make sure that BIC enters bootloader
  sleep(3);

  //step6 - send cmd 0x21 to notice BIC the update will start
  ret = send_start_bic_update(i2cfd, file_size);
  if (ret < 0) {
    printf("Failed to send a signal to start the update of BIC\n");
    goto exit;
  }

  msleep(600);

  //step7 - check the response
  ret = read_bic_update_ack_status(i2cfd);
  if (ret < 0) {
    printf("Failed to get the response of the command\n");
    goto exit;
  }

  //step8 - loop to send all the image data
  ret = send_bic_runtime_image_data(fd, i2cfd, file_size, bytes_per_read);
  if ( ret < 0 ) {
    printf("Failed to send image data\n");
    goto exit;
  }

  msleep(500);

  //step9 - run the new image
  ret = send_complete_signal(i2cfd);
  if (ret < 0) {
    printf("Failed to send a complete signal\n");
  }

  //step10 - check the response
  ret = read_bic_update_ack_status(i2cfd);
  if (ret < 0) {
    printf("Failed to get the response of the command\n");
    goto exit;
  }

exit:
  //step11 - recover the i2c speed to 1M
#if 0
  reg_base = mmap(NULL, PAGE_SIZE, PROT_READ|PROT_WRITE, MAP_SHARED, mmap_fd, I2C_BASE_MAP(bus_num));
  reg_offset = (char*) reg_base + I2C_CLK_CTRL_REG;
  *(volatile uint32_t*) reg_offset = orig_i2c_ctl_reg;
  munmap(reg_base, PAGE_SIZE);
  if (mmap_fd >= 0) {
      close(mmap_fd);
  }
#else
  snprintf(cmd, sizeof(cmd), DEVMEM_WRITE_CMD, I2C_BASE_MAP(bus_num) + I2C_CLK_CTRL_REG, orig_i2c_ctl_reg);
  if (system(cmd) != 0) {
      printf("%s(): command %s failed\n", __func__, cmd);
      ret = BIC_STATUS_FAILURE;
  }
#endif

  msleep(500);
  //step12 - restart the ipmbd
  snprintf(cmd, cmd_size, "sv start ipmbd_%d", bus_num);
  if (system(cmd) != 0) {
      syslog(LOG_WARNING, "[%s] %s failed\n", __func__, cmd);
      return BIC_STATUS_FAILURE;
  }

  syslog(LOG_CRIT, "%s: updating bic firmware is exiting\n", __func__);

  if (i2cfd >= 0) {
    close(i2cfd);
  }

  return ret;
}

static int
is_valid_bic_image(uint8_t comp, int fd, int file_size){
#define BICBL_TAG 0x00
#define BICBR_TAG 0x01
#define BICBL_OFFSET 0x3f00
#define BICBR_OFFSET 0x8000

#define REVISION_ID(x) ((x >> 4) & 0x0f)
#define COMPONENT_ID(x) (x & 0x0f)

  int ret = BIC_STATUS_FAILURE;
  uint8_t rbuf[BIC_VALIDATE_READ_LEN] = {0};
  uint8_t rlen = sizeof(rbuf);
  uint8_t sel_tag = 0xff;
  uint32_t sel_offset = 0xffffffff;
 
  switch (comp) {
    case UPDATE_BIC:
      sel_tag = BICBR_TAG;
      sel_offset = BICBR_OFFSET;
      break;
    case UPDATE_BIC_BOOTLOADER:
      sel_tag = BICBL_TAG;
      sel_offset = BICBL_OFFSET;
      break;
    default:
      syslog(LOG_WARNING, "%s() Unknown component %x", __func__, comp);
      break;
  }

  if (lseek(fd, sel_offset, SEEK_SET) != (off_t)sel_offset) {
    goto error_exit;
  }

  if (read(fd, rbuf, rlen) != (off_t)rlen) {
    goto error_exit;
  }

  if ( rbuf[0] != sel_tag || COMPONENT_ID(rbuf[1]) != COMPONENT_ID(BIC_BS) ) {
    goto error_exit;
  }
  ret = BIC_STATUS_SUCCESS;

error_exit:
  if (ret == BIC_STATUS_FAILURE) {
    printf("This file cannot be updated to this component!\n");
  }

  return ret;
}

static int
update_bic_runtime_fw(uint8_t comp, char *path, uint8_t force) {
  int ret = -1;
  int fd = 0;
  int file_size = 0;

  //get fd and file size
  fd = open_and_get_size(path, &file_size);
  if ( fd < 0 ) {
    syslog(LOG_WARNING, "%s() cannot open the file: %s, fd = %d\n", __func__, path, fd);
    goto exit;
  }
  printf("file size = %d bytes\n", file_size);

  //check the content of the image
  if (!force && is_valid_bic_image(comp, fd, file_size)) {
    printf("Invalid BIC file!\n");
    ret = -1;
    goto exit;
  }
  ret = update_bic(fd, file_size);

exit:
  if (fd >= 0) {
    close(fd);
  }

  return ret;
}

static int
update_fw_bic_bootloader(uint8_t comp, int fd, int file_size) {
  const uint8_t bytes_per_read = IPMB_MAX_SEND;
  uint8_t buf[BIC_IMG_DATA_LEN] = {0};
  uint16_t buf_size = sizeof(buf);
  uint16_t read_bytes = 0;
  uint32_t offset = 0;
  uint32_t last_offset = 0;
  uint32_t dsize = 0;
  int ret = -1;

  dsize = file_size / BIC_UPDATE_BLOCK_SIZE;

  if (lseek(fd, 0, SEEK_SET) != 0) {
    syslog(LOG_WARNING, "%s() Cannot reinit the fd to the beginning. errstr=%s", __func__, strerror(errno));
    return -1;
  }

  printf("Update BIC bootloader\n");
  while (1) {
    memset(buf, 0, buf_size);
    read_bytes = read(fd, buf, bytes_per_read);
    if ( read_bytes <= 0 ) {
      //no more bytes can be read
      break;
    }

    if ((offset + read_bytes) >= file_size) {
      comp |= 1 << BIC_BOOTLOADER_COMP_BIT;
    }
    ret = send_image_data_via_bic(comp, offset, read_bytes, 0, buf);
    if (ret != BIC_STATUS_SUCCESS) {
      break;
    }
    offset += read_bytes;
    if ((last_offset + dsize) <= offset) {      
      printf("updated bic bootloader: %d %%\n", (offset / dsize) * 5);
      fflush(stdout);
      last_offset += dsize;
    }
  }

  return ret;
}

static int
update_bic_bootloader_fw(uint8_t comp, char *path, uint8_t force) {
  int fd = 0;
  int ret = 0;
  int file_size = 0;

  fd = open_and_get_size(path, &file_size);
  if (fd < 0) {
    syslog(LOG_WARNING, "%s() cannot open the file: %s, fd=%d", __func__, path, fd);
    goto exit;
  }
  printf("file size = %d bytes, comp = 0x%x\n", file_size, comp);

  //check the content of the image
  ret = is_valid_bic_image(comp, fd, file_size);
  if (ret < 0) {
    printf("Invalid BIC bootloader file!\n");
    goto exit;
  }

  ret = update_fw_bic_bootloader(comp, fd, file_size);

exit:
  if (fd >= 0) {
    close(fd);
  }

  return ret;
}

static char*
get_component_name(uint8_t comp) {
  switch (comp) {
    case FW_BIC:
      return "Bridge-IC";
    case FW_BIC_BOOTLOADER:
      return "Bridge-IC Bootloader";
    case FW_VR:
      return "VR";
    case FW_BIOS:
      return "BIOS";
    default:
      return "Unknown";
  }
}

int
bic_update_fw(uint8_t slot_id, uint8_t comp, char *path, uint8_t force) {
  int ret = BIC_STATUS_SUCCESS;
  char ipmb_content[] = "ipmb";
  char* loc = NULL;

  if (path == NULL) {
    syslog(LOG_ERR, "%s(): Update aborted due to NULL parameter: *path", __func__);
    return -1;
  }

  loc = strstr(path, ipmb_content);
  printf("comp: %x, img: %s, force: %x\n", comp, path, force);
  syslog(LOG_CRIT, "Updating %s. File: %s", get_component_name(comp), path);

  //run cmd
  switch (comp) {
    case FW_BIC:
      ret = update_bic_runtime_fw(UPDATE_BIC, path, force);
      break;
    case FW_BIC_BOOTLOADER:
      ret = update_bic_bootloader_fw(UPDATE_BIC_BOOTLOADER, path, force);
      break;
    case FW_BIOS:
      if (loc != NULL) {
        ret = update_bic_bios(comp, path, force);
      } else {
        ret = update_bic_usb_bios(comp, path);
      }
      break;
    case FW_VR:
      ret = update_bic_vr(path, force);
      break;
    default:
      syslog(LOG_WARNING, "Unknown compoet %x", comp);
      break;
  }
  syslog(LOG_CRIT, "Updated %s. File: %s. Result: %s", get_component_name(comp), path, (ret < 0) ? "Fail" : "Success");

  return ret;
}
