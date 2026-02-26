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
#ifdef CONFIG_GRANDCANYON2
#include <linux/i2c-dev.h>
#include <linux/i2c.h>
#include <sys/ioctl.h>
#include <facebook/fbgc_gpio.h>
#endif
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

#ifndef CONFIG_GRANDCANYON2
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
  uint8_t read_bytes = 0, max_read_bytes = 0;
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
    max_read_bytes = bytes_per_read;
    if ((offset + bytes_per_read) > file_size) {
      // Prevent to read the MD5 bytes.
      max_read_bytes = file_size - offset;
    }
    read_bytes = read(fd, buf, max_read_bytes);
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
update_bic(int fd, int file_size, bool force) {
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
  if ((force == false) && (ret < 0)) {
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
  snprintf(cmd, sizeof(cmd), DEVMEM_WRITE_CMD, I2C_BASE_MAP(bus_num) + I2C_CLK_CTRL_REG, orig_i2c_ctl_reg);
  if (system(cmd) != 0) {
      printf("%s(): command %s failed\n", __func__, cmd);
      ret = BIC_STATUS_FAILURE;
  }

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
is_valid_bic_image(uint8_t comp, int fd, int file_size, char* path){
#define BICBL_TAG 0x00
#define BICBR_TAG 0x01
#define BICBL_OFFSET 0x3f00
#define BICBR_OFFSET 0x8000
#define BICBL_PLAT_SIG_OFFSET 0x3ef0
#define BICBR_PLAT_SIG_OFFSET 0x7ff0

#define REVISION_ID(x) ((x >> 4) & 0x0f)
#define COMPONENT_ID(x) (x & 0x0f)

  int ret = BIC_STATUS_FAILURE, board_type_index = 0;
  uint8_t rbuf[BIC_VALIDATE_READ_LEN] = {0};
  uint8_t rlen = sizeof(rbuf);
  uint8_t sel_tag = 0xff, fw_rev_id = 0xff, board_rev_id = 0xff;
  uint32_t sel_offset = 0xffffffff;
  uint32_t sig_offset = 0xffffffff;
  uint32_t md5_offset = 0xffffffff;
  bool board_rev_is_invalid = false;

  switch (comp) {
    case UPDATE_BIC:
      sel_tag = BICBR_TAG;
      sel_offset = BICBR_OFFSET;
      sig_offset = BICBR_PLAT_SIG_OFFSET;
      break;
    case UPDATE_BIC_BOOTLOADER:
      sel_tag = BICBL_TAG;
      sel_offset = BICBL_OFFSET;
      sig_offset = BICBL_PLAT_SIG_OFFSET;
      break;
    default:
      syslog(LOG_WARNING, "%s() Unknown component %x", __func__, comp);
      break;
  }
  md5_offset = file_size;

  if (lseek(fd, sel_offset, SEEK_SET) != (off_t)sel_offset) {
    goto error_exit;
  }

  if (read(fd, rbuf, rlen) != (off_t)rlen) {
    goto error_exit;
  }

  if ( rbuf[0] != sel_tag || COMPONENT_ID(rbuf[1]) != COMPONENT_ID(BIC_BS) ) {
    goto error_exit;
  }

  // Check f/w and server board stage
  if (get_server_board_revision_id(&board_rev_id, sizeof(board_rev_id)) < 0) {
    goto error_exit;
  }

  fw_rev_id = REVISION_ID(rbuf[1]);
  board_type_index = board_rev_id - 1;
  if (board_type_index < 0) {
    board_type_index = 0;
  }

  if ((fw_rev_id > STAGE_MP) || (board_type_index > STAGE_MP)) {
    syslog(LOG_WARNING, "%s() wrong board revision ID, f/w REV ID: %d, board REV ID: %d", __func__, fw_rev_id, board_type_index);
    goto error_exit;
  }

  // PVT & MP firmware could be used in common
  if (board_type_index < STAGE_PVT) {
    if (fw_rev_id != board_type_index) {
      board_rev_is_invalid = true;
    }
  } else {
    if (fw_rev_id < STAGE_PVT) {
      board_rev_is_invalid = true;
    }
  }

  if (board_rev_is_invalid == true) {
    printf("If you want to update the %s f/w on the %s system, please use force update.\n",
            board_stage[fw_rev_id], board_stage[board_type_index]);
    goto error_exit;
  }

  // Compare MD5 of image
  if (check_image_md5(path, file_size, md5_offset) < 0) {
    printf("Image file has corrupted!\n");
    printf("If you are updating with old version firmware, please use force update.\n");
    goto error_exit;
  }

  // Compare signature of image
  if (check_image_signature(path, sig_offset) < 0) {
    printf("The image is not for Grand Canyon!\n");
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

  //check the content of the image
  if (force == 0) {
    // minus 16 bytes of md5
    file_size -= MD5_DIGEST_LENGTH;

    if (is_valid_bic_image(comp, fd, file_size, path) < 0) {
      printf("Invalid BIC file!\n");
      ret = -1;
      goto exit;
    }
  } else {
    // If image contains MD5 bytes, minus 16 bytes of MD5
    if (check_image_md5(path, (file_size - MD5_DIGEST_LENGTH), (file_size - MD5_DIGEST_LENGTH)) >= 0) {
      file_size -= MD5_DIGEST_LENGTH;
    }
  }
  printf("file size = %d bytes\n", file_size);

  ret = update_bic(fd, file_size, force);

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
  uint16_t read_bytes = 0, max_read_bytes = 0;
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
    max_read_bytes = bytes_per_read;
    if ((offset + max_read_bytes > file_size)) {
      // Prevent to read the MD5 bytes.
      max_read_bytes = file_size - offset;
    }

    read_bytes = read(fd, buf, max_read_bytes);
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

  //check the content of the image
  if (force == 0) {
    // minus 16 bytes of md5
    file_size -= MD5_DIGEST_LENGTH;

    if (is_valid_bic_image(comp, fd, file_size, path) < 0) {
      printf("Invalid BIC bootloader file!\n");
      ret = -1;
      goto exit;
    }
  } else {
    // If image contains MD5 bytes, minus 16 bytes of MD5.
    if (check_image_md5(path, (file_size - MD5_DIGEST_LENGTH), (file_size - MD5_DIGEST_LENGTH)) >= 0) {
      file_size -= MD5_DIGEST_LENGTH;
    }
  }
  printf("file size = %d bytes, comp = 0x%x\n", file_size, comp);

  ret = update_fw_bic_bootloader(comp, fd, file_size);

exit:
  if (fd >= 0) {
    close(fd);
  }

  return ret;
}
#endif  // !CONFIG_GRANDCANYON2

static char*
get_component_name(uint8_t comp) {
  switch (comp) {
    case FW_BIC:
      return "Bridge-IC";
    case FW_BIC_BOOTLOADER:
      return "Bridge-IC Bootloader";
#ifdef CONFIG_GRANDCANYON2
    case FW_BIC_RECOVERY:
      return "Bridge-IC(Recovery)";
#endif
    case FW_VR:
      return "VR";
    case FW_BIOS:
      return "BIOS";
    default:
      return "Unknown";
  }
}

#ifdef CONFIG_GRANDCANYON2
#define ES_FPGA_BIC_FWSPICK_MASK_SET    ((1U << 2) | (1U << 3))    // bit[2:3]= BIC_FWSPICK_CPLD:BIC_FWSPICK_BUS
#define BIC_UART_DEV                    "/dev/ttyS7"
#define MTERM_BIC_SERVICE               "mTerm-bic"
#define BIC_RESET_GPIO_SHADOW           "UIC_COMP_BIC_RST_N"
#define BAUD_115200  115200
#define BAUD_57600 57600

int fpga_read_u8(uint8_t bus,
                    uint8_t addr,
                    uint8_t reg,
                    uint8_t *val)
{
  int fd;
  char dev[32];
  struct i2c_rdwr_ioctl_data ioctl_data;
  struct i2c_msg msgs[2];
  uint8_t reg_buf = reg;
  uint8_t data_buf = 0;

  if (val == NULL) {
    return -EINVAL;
  }

  snprintf(dev, sizeof(dev), "/dev/i2c-%u", bus);

  fd = open(dev, O_RDWR);
  if (fd < 0) {
    syslog(LOG_ERR, "fpga_read_u8: open %s failed: %s",
           dev, strerror(errno));
    return -errno;
  }

  /*
   * msg[0]: write reg offset
   * msg[1]: read 1 byte
   */
  msgs[0].addr  = addr;
  msgs[0].flags = 0;               // write
  msgs[0].len   = 1;
  msgs[0].buf   = &reg_buf;

  msgs[1].addr  = addr;
  msgs[1].flags = I2C_M_RD;        // read
  msgs[1].len   = 1;
  msgs[1].buf   = &data_buf;

  ioctl_data.msgs  = msgs;
  ioctl_data.nmsgs = 2;

  if (ioctl(fd, I2C_RDWR, &ioctl_data) < 0) {
    syslog(LOG_ERR,
           "fpga_read_u8: I2C_RDWR failed (bus=%u addr=0x%02x reg=0x%02x): %s",
           bus, addr, reg, strerror(errno));
    close(fd);
    return -errno;
  }

  close(fd);
  *val = data_buf;

  syslog(LOG_DEBUG,
         "fpga_read_u8: bus=%u addr=0x%02x reg=0x%02x val=0x%02x",
         bus, addr, reg, *val);

  return 0;
}

int fpga_write_u8(uint8_t bus,
                     uint8_t addr,
                     uint8_t reg,
                     uint8_t val)
{
  int fd;
  char dev[32];
  struct i2c_rdwr_ioctl_data ioctl_data;
  struct i2c_msg msg;
  uint8_t buf[2];

  snprintf(dev, sizeof(dev), "/dev/i2c-%u", bus);

  fd = open(dev, O_RDWR);
  if (fd < 0) {
    syslog(LOG_ERR, "fpga_write_u8: open %s failed: %s",
           dev, strerror(errno));
    return -errno;
  }

  /*
   * write: <reg> <val>
   */
  buf[0] = reg;
  buf[1] = val;

  msg.addr  = addr;
  msg.flags = 0;     // write
  msg.len   = 2;
  msg.buf   = buf;

  ioctl_data.msgs  = &msg;
  ioctl_data.nmsgs = 1;

  if (ioctl(fd, I2C_RDWR, &ioctl_data) < 0) {
    syslog(LOG_ERR,
           "fpga_write_u8: I2C_RDWR failed (bus=%u addr=0x%02x reg=0x%02x val=0x%02x): %s",
           bus, addr, reg, val, strerror(errno));
    close(fd);
    return -errno;
  }

  close(fd);

  syslog(LOG_DEBUG,
         "fpga_write_u8: bus=%u addr=0x%02x reg=0x%02x val=0x%02x",
         bus, addr, reg, val);

  return 0;
}

static bool
end_with (char* str, uint8_t str_len, char* pattern, uint8_t pattern_len) {
  if ((str == NULL) || (pattern == NULL)) {
    return false;
  }
  return (strncmp(str + (str_len - pattern_len), pattern, pattern_len) == 0);
}

static int
update_bic_cpld_altera(uint8_t slot_id, char *path, uint8_t intf, uint8_t force) {
  int ret = -1;
  syslog(LOG_INFO, "BIC CPLD Update bypass in current stage, ret: %d\n", ret);
  return ret;
}

static int
is_valid_intf(uint8_t intf) {
  int ret = BIC_STATUS_FAILURE;
  switch(intf) {
    case NONE_INTF:
      ret = BIC_STATUS_SUCCESS;
      break;
  }

  return ret;
}

// Update firmware for various components
static int
_update_fw(uint8_t slot_id, uint8_t target, uint8_t type, uint32_t offset,
           uint16_t len, uint8_t *buf, uint8_t intf) {
  uint8_t tbuf[256] = {0x00};
  uint8_t rbuf[16] = {0x00};
  uint8_t tlen = 0;
  uint8_t rlen = sizeof(rbuf);
  int ret = 0, retry;

  if (buf == NULL) {
    return -1;
  }

  // Fill the IANA ID
  memcpy(tbuf, (uint8_t *)&META_IANA_ID, SIZE_IANA_ID);
  
  // Fill the component for which firmware is requested
  // Target is for update component
  tbuf[3] = target;
  memcpy(&tbuf[4], &offset, sizeof(offset));
  tbuf[8] = len & 0xFF;
  tbuf[9] = (len >> 8) & 0xFF;
  memcpy(&tbuf[10], buf, len);

  tlen = len + 10;
  for (retry = 0; retry < 3; retry++) {
#ifdef DEBUG
    print_data(__func__, NETFN_OEM_1S_REQ, CMD_OEM_1S_UPDATE_FW, tbuf, tlen);
#endif
    ret = bic_data_send(slot_id, NETFN_OEM_1S_REQ, CMD_OEM_1S_UPDATE_FW, tbuf, tlen, rbuf, &rlen, intf);
    if (ret == 0) {
      return ret;
    }

    sleep(1);
    printf("_update_fw: slot: %u, target %u, offset: %u, len: %u retrying..\n",
           slot_id, target, offset, len);
  }

  return ret;
}

static int
update_bic(uint8_t slot_id, int fd, size_t file_size, uint8_t intf) {
  struct timeval start, end;
  int update_rc = -1, cmd_rc = 0;
  uint32_t dsize, last_offset;
  uint32_t offset, boundary;
  volatile uint16_t read_count;
  uint8_t buf[256] = {0};
  uint8_t target;
  uint8_t type = TYPE_1OU_VERNAL_FALLS_WITH_AST;
  ssize_t count;

  printf("updating fw on slot %d:\n", slot_id);

  // Write chunks of binary data in a loop
  dsize = file_size/100;
  last_offset = 0;
  offset = 0;
  boundary = PKT_SIZE;
  target = UPDATE_BIC;
  gettimeofday(&start, NULL);
  while (1) {
    // send packets in blocks of 64K
    if ((offset + AST_BIC_IPMB_WRITE_COUNT_MAX) < boundary) {
      read_count = AST_BIC_IPMB_WRITE_COUNT_MAX;
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
    // Send data to Bridge-IC
    cmd_rc = _update_fw(slot_id, target, type, offset, count, buf, intf);
    if (cmd_rc) {
      goto error_exit;
    }

    // Update counter
    offset += count;
    if (offset >= boundary) {
      boundary += PKT_SIZE;
    }
    if ((last_offset + dsize) <= offset) {
      _set_fw_update_ongoing(60);
      printf("\rupdated bic: %u %%", offset/dsize);
      fflush(stdout);
      last_offset += dsize;
    }
  }
  printf("\n");

  gettimeofday(&end, NULL);
  printf("Elapsed time:  %d   sec.\n", (int)(end.tv_sec - start.tv_sec));
  if (offset >= file_size) {
    update_rc = 0;
  }

error_exit:

  printf("\n");

  if ( update_rc == 0 ) {
    update_rc = cmd_rc;
  }
  return update_rc;
}

static int
update_bic_runtime_fw(uint8_t slot_id, uint8_t comp __attribute__((unused)), uint8_t intf, char *path, uint8_t force __attribute__((unused))) {
  int ret = -1;
  int fd = -1;
  size_t file_size = 0;

  //check params
  ret = is_valid_intf(intf);
  if ( ret < 0 ) {
    syslog(LOG_WARNING, "%s() invalid intf(val=0x%x) was caught!\n", __func__, intf);
    goto exit;
  }

  //get fd and file size
  fd = open_and_get_size(path, &file_size);
  if ( fd < 0 ) {
    syslog(LOG_WARNING, "%s() cannot open the file: %s, fd=%d\n", __func__, path, fd);
    goto exit;
  }

  printf("file size = %zu bytes, slot = %u, intf = 0x%x\n", file_size, slot_id, intf);

  //run into the different function based on the interface
  ret = update_bic(slot_id, fd, file_size, intf);
  if (ret == 0) {
    syslog(LOG_INFO, "BIC Update Response Success\n");
  }
  else
  {
    syslog(LOG_INFO, "BIC Update Response Failed\n");
  }

exit:

  if ( fd >= 0 ) {
    close(fd);
  }

  return ret;
}

static int
recovery_bic_runtime_fw(uint8_t slot_id, uint8_t comp, uint8_t intf, char *path, uint8_t force){
  int ret = BIC_STATUS_FAILURE;
  int fd = -1, ttyfd = -1;
  size_t file_size = 0;
  char cmd[MAX_SYS_CMD_REQ_LEN] = {0};
  size_t cmd_size = sizeof(cmd);
  bool mterm_stopped = false, strap_set = false;
  uint8_t initial_value = 0, rcvy_mode_value = 0;
  uint8_t buf[256] = {0};

  fd = open_and_get_size(path, &file_size);
  if (fd < 0) {
    syslog(LOG_WARNING, "%s() cannot open the file: %s, fd = %d\n", __func__, path, fd);
    return -1;
  }
  printf("file size = %zu bytes, slot=%u, comp=%u, intf=0x%x\n", file_size, slot_id, comp, intf);

  if (sv_control(MTERM_BIC_SERVICE, SV_STOP) == SV_STOP) {
    mterm_stopped = true;
  } else {
    syslog(LOG_WARNING, "%s() Failed to stop %s", __func__, MTERM_BIC_SERVICE);
    goto cleanup;
  }

  //set CPLD_BIC_FWSPICK (bit2) and CPLD_BIC_FWSPICK_BUS (bit3) to high
  printf("Setting BIC boot from UART\n");
  if (fpga_read_u8(I2C_ES_FPGA_BUS, ES_FPGA_SLAVE_ADDR, 0x01, &initial_value) != 0) {
    goto cleanup;
  }
  rcvy_mode_value = initial_value | ES_FPGA_BIC_FWSPICK_MASK_SET;
  if (fpga_write_u8(I2C_ES_FPGA_BUS, ES_FPGA_SLAVE_ADDR, 0x01, rcvy_mode_value) != 0) {
    goto cleanup;
  }
  strap_set = true;
  msleep(100);

  //set UIC_COMP_BIC_RST_N to low trigger BIC_RESET_N
  snprintf(cmd, cmd_size, "/usr/bin/gpiocli -s \"%s\" set-value %d", BIC_RESET_GPIO_SHADOW, GPIO_VALUE_LOW);
  if (system(cmd) != 0) {
    syslog(LOG_WARNING, "%s() %s failed", __func__, cmd);
    goto cleanup;
  }
  sleep(1);

  snprintf(cmd, cmd_size, "/usr/bin/gpiocli -s \"%s\" set-value %d", BIC_RESET_GPIO_SHADOW, GPIO_VALUE_HIGH);
  if (system(cmd) != 0) {
    syslog(LOG_WARNING, "%s() %s failed", __func__, cmd);
    goto cleanup;
  }
  msleep(20);

  //Check if bic enter recovery mode
  if (is_bic_ready() == STATUS_BIC_NOT_READY) {
    printf("Successfully entered BIC recovery mode\n");
  } else {
    printf("Failed to enter BIC recovery mode\n");
    goto cleanup;
  }

  snprintf(cmd, sizeof(cmd), "/bin/stty -F %s %d", BIC_UART_DEV, BAUD_115200);
  if (system(cmd) != 0) {
    syslog(LOG_WARNING, "%s() %s failed", __func__, cmd);
    goto cleanup;
  }
  msleep(20);

  ttyfd = open(BIC_UART_DEV, O_RDWR | O_NOCTTY);
  if (ttyfd < 0) {
    syslog(LOG_ERR, "%s() Cannot open %s", __func__, BIC_UART_DEV);
    goto cleanup;
  }

  printf("Doing the recovery update...\n");
  size_t r_b = 0;
  uint32_t fsz = (uint32_t)file_size;
  if (write(ttyfd, &fsz, sizeof(fsz)) != (ssize_t)sizeof(fsz)) {
    syslog(LOG_ERR, "%s() write file size failed", __func__);
    goto cleanup;
  }

  size_t dsize = (file_size >= 100) ? (file_size / 100) : 1;
  size_t last_offset = 0;
  while (r_b < file_size) {
    int rc = read(fd, buf, sizeof(buf));
    if (rc <= 0) {
      if (rc < 0 && errno == EINTR) {
        continue;
      }
      syslog(LOG_ERR, "%s() read image failed rc=%d", __func__, rc);
      goto cleanup;
    }

    int w_b = 0;
    while (w_b < rc) {
      int wc = write(ttyfd, &buf[w_b], rc - w_b);
      if (wc > 0) {
        w_b += wc;
      } else {
        if (wc < 0 && errno == EINTR) {
          continue;
        }
        syslog(LOG_ERR, "%s() write tty failed wc=%d", __func__, wc);
        goto cleanup;
      }
    }

    r_b += rc;

    if ((last_offset + dsize) <= r_b) {
      _set_fw_update_ongoing(60);
      printf("\ruploaded bic: %zu %%", r_b / dsize);
      fflush(stdout);
      last_offset += dsize;
    }
  }
  printf("\n");

  if (r_b != file_size) {
    syslog(LOG_ERR, "%s() uploaded bic failed (%zu/%zu)", __func__, r_b, file_size);
    goto cleanup;
  }
  sleep(5);

  //update bic
  ret = update_bic_runtime_fw(slot_id, comp, intf, path, force);
  sleep(5);

cleanup:
  //set UART back to 57600
  snprintf(cmd, sizeof(cmd), "/bin/stty -F %s %d", BIC_UART_DEV, BAUD_57600);
  if (system(cmd) != 0) {
    syslog(LOG_WARNING, "[%s] %s failed", __func__, cmd);
  }

  if (strap_set) {
    //set CPLD_BIC_FWSPICK (bit2) and CPLD_BIC_FWSPICK_BUS (bit3) back to low
    fpga_write_u8(I2C_ES_FPGA_BUS, ES_FPGA_SLAVE_ADDR, 0x01, initial_value);

    //set UIC_COMP_BIC_RST_N to low trigger BIC_RESET_N
    snprintf(cmd, cmd_size, "/usr/bin/gpiocli -s \"%s\" set-value %d", BIC_RESET_GPIO_SHADOW, GPIO_VALUE_LOW);
    if (system(cmd) != 0) {
      syslog(LOG_WARNING, "[%s] %s failed", __func__, cmd);
    }
    sleep(1);
    snprintf(cmd, cmd_size, "/usr/bin/gpiocli -s \"%s\" set-value %d", BIC_RESET_GPIO_SHADOW, GPIO_VALUE_HIGH);
    if (system(cmd) != 0) {
      syslog(LOG_WARNING, "[%s] %s failed", __func__, cmd);
    }
  }

  //restart mTerm-bic service
  if (mterm_stopped) {
    if (sv_control(MTERM_BIC_SERVICE, SV_START) == SV_START) {
      mterm_stopped = false;
    } else {
      syslog(LOG_ERR, "Failed to start %s", MTERM_BIC_SERVICE);
    }
  }

  if (ret == BIC_STATUS_SUCCESS) {
    printf("Power-cycling the server...\n");
    //12v off
    if(fpga_write_u8(I2C_UIC_FPGA_BUS, UIC_FPGA_SLAVE_ADDR >> 1, UIC_FPGA_SLAVE_AC_POWER_OFFSET, STAT_AC_OFF ) != 0)
    {
      printf("Failed to 12V off, please manual do 12V cycle..\n");
    }
    sleep(1);
    //12v on
    if( fpga_write_u8(I2C_UIC_FPGA_BUS, UIC_FPGA_SLAVE_ADDR >> 1, UIC_FPGA_SLAVE_AC_POWER_OFFSET, STAT_AC_ON ) != 0)
    {
      printf("Failed to 12V on, please manual do 12V cycle..\n");
    }
    sleep(5);

    //DC on
    if( fpga_write_u8(I2C_ES_FPGA_BUS, ES_FPGA_SLAVE_ADDR, ES_FPGA_SLAVE_DC_POWER_OFFSET, STAT_DC_OFF ) != 0)
    {
      printf("Failed to server power off, please manual do power cycle..\n");
    }
    sleep(DELAY_DC_POWER_ON);
    if( fpga_write_u8(I2C_ES_FPGA_BUS, ES_FPGA_SLAVE_ADDR, ES_FPGA_SLAVE_DC_POWER_OFFSET, STAT_DC_ON ) != 0)
    {
      printf("Failed to server power on, please manual do power cycle..\n");
    }
  }

  if (ttyfd >= 0) close(ttyfd);
  if (fd >= 0) close(fd);

  return ret;
}

static int
bic_update_fw_path_or_fd(uint8_t slot_id, uint8_t comp, char *path, int fd, uint8_t force) {
  int ret = BIC_STATUS_SUCCESS;
  uint8_t intf = 0x0;
  char ipmb_content[] = "ipmb";
  char tmp_posfix[] = "-tmp";
  char* loc = NULL;
  char fdstr[32] = {0};
  bool fd_opened = false;
  size_t origin_len = 0;
  char origin_path[128] = {0};

  if (path == NULL) {
    if (fd < 0) {
      syslog(LOG_ERR, "%s(): Update aborted due to NULL pointer: *path", __func__);
      return -1;
    }
    snprintf(fdstr, sizeof(fdstr) - 1, "<%d>", fd);
    path = fdstr;
  } else {
    fd = open(path, O_RDONLY, 0);
    if (fd < 0) {
      syslog(LOG_ERR, "%s(): Unable to open %s: %d", __func__, path, errno);
      return -1;
    }
    fd_opened = true;
  }

  loc = strstr(path, ipmb_content);

  if (end_with(path, strlen(path), tmp_posfix, strlen(tmp_posfix))) {
    origin_len = strlen(path) - strlen(tmp_posfix) + 1;
    if (origin_len > sizeof(origin_path)) {
      origin_len = sizeof(origin_path);
    }
    snprintf(origin_path, origin_len, "%s", path);
  } else {
    snprintf(origin_path, sizeof(origin_path), "%s", path);
  }

  fprintf(stderr, "slot_id: %x, comp: %x, intf: %x, img: %s, force: %x\n", slot_id, comp, intf, origin_path, force);
  syslog(LOG_CRIT, "Updating %s on slot%d. File: %s", get_component_name(comp), slot_id, origin_path);

  //get the intf
  intf = NONE_INTF;

  //run cmd
  switch (comp) {
    case FW_BIC:
      ret = update_bic_runtime_fw(slot_id, UPDATE_BIC, intf, path, force);
      break;
    case FW_BIC_RECOVERY:
      ret = recovery_bic_runtime_fw(slot_id, comp, intf, path, force);
      break;
    case FW_BS_FPGA:
      ret = update_bic_cpld_altera(slot_id, path, intf, force);
      break;
    case FW_BIOS:
      if (loc != NULL) {
        ret = update_bic_bios(slot_id, comp, path, FORCE_UPDATE_SET);
      } else {
        ret = update_bic_usb_bios(comp, fd, force);
      }
      break;
    case FW_VR:
      ret = update_bic_vr(path, force);
      break;
    default:
      syslog(LOG_WARNING, "%s(): component %x not supported", __func__, comp);
      return -1;
  }

  syslog(LOG_CRIT, "Updated %s on slot%d. File: %s. Result: %s", get_component_name(comp), slot_id, origin_path, (ret != 0)?"Fail":"Success");
  if (fd_opened) {
    close(fd);
  }
  return ret;
}

int
bic_update_fw(uint8_t slot_id, uint8_t comp, char *path, uint8_t force) {
  return bic_update_fw_path_or_fd(slot_id, comp, path, -1, force);
}

#else  // !CONFIG_GRANDCANYON2

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
#endif  // CONFIG_GRANDCANYON2