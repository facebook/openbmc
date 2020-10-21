
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
#include "bic_fwupdate.h"
#include "bic_cpld_altera_fwupdate.h"
#include "bic_cpld_lattice_fwupdate.h"
#include "bic_vr_fwupdate.h"
#include "bic_bios_fwupdate.h"
//#define DEBUG

/****************************/
/*      BIC fw update       */
/****************************/
#define BIC_CMD_DOWNLOAD 0x21
#define BIC_CMD_DOWNLOAD_SIZE 11

#define BIC_CMD_RUN 0x22
#define BIC_CMD_RUN_SIZE 7

#define BIC_CMD_STS 0x23
#define BIC_CMD_STS_SIZE 3

#define BIC_CMD_DATA 0x24
#define BIC_CMD_DATA_SIZE 0xFF

#define BIC_FLASH_START 0x8000

enum {
  FEXP_BIC_I2C_WRITE   = 0x20,
  FEXP_BIC_I2C_READ    = 0x21,
  FEXP_BIC_I2C_UPDATE  = 0x22,
  FEXP_BIC_IPMI_I2C_SW = 0x23,

  REXP_BIC_I2C_WRITE   = 0x24,
  REXP_BIC_I2C_READ    = 0x25,
  REXP_BIC_I2C_UPDATE  = 0x26,
  REXP_BIC_IPMI_I2C_SW = 0x27,

  BB_BIC_I2C_WRITE     = 0x28,
  BB_BIC_I2C_READ      = 0x29,
  BB_BIC_I2C_UPDATE    = 0x2A,
  BB_BIC_IPMI_I2C_SW   = 0x2B
};

enum {
  I2C_100K = 0x0,
  I2C_1M
};

#ifdef DEBUG
static void print_data(const char *name, uint8_t netfn, uint8_t cmd, uint8_t *buf, uint8_t len) {
  int i;
  printf("[%s][%d]send: 0x%x 0x%x ", name, len, netfn, cmd);
  for ( i = 0; i < len; i++) {
    printf("0x%x ", buf[i]);
  }

  printf("\n");
}
#endif

static uint8_t
get_checksum(uint8_t *buf, uint8_t len) {
  int i;
  uint8_t result = 0;
  for ( i = 0; i < len; i++ ) {
    result += buf[i];
  }

  return result;
}

static int
enable_bic_update_with_param(uint8_t slot_id, uint8_t intf) {
  uint8_t tbuf[10] = {0x00};
  uint8_t rbuf[10] = {0x00};
  uint8_t tlen = 0;
  uint8_t rlen = 0;
  uint8_t netfn = 0;
  uint8_t cmd   = 0;
  int ret = -1;

  memcpy(tbuf, (uint8_t *)&IANA_ID, 3);

  switch (intf) {
    case FEXP_BIC_INTF:
    case BB_BIC_INTF:
    case REXP_BIC_INTF:
      netfn  = NETFN_OEM_1S_REQ;
      cmd    = CMD_OEM_1S_MSG_OUT;
      tbuf[3] = intf;
      tbuf[4] = NETFN_OEM_1S_REQ << 2;
      tbuf[5] = CMD_OEM_1S_ENABLE_BIC_UPDATE;
      memcpy(&tbuf[6], (uint8_t *)&IANA_ID, 3);
      tbuf[9] = 0x01; //Update bic via i2c
      tlen = 10;
      break;

    case NONE_INTF:
      netfn = NETFN_OEM_1S_REQ;
      cmd   = CMD_OEM_1S_ENABLE_BIC_UPDATE;
      tbuf[3] = 0x1; //Update bic via i2c
      tlen = 4;
      break;
  }

#ifdef DEBUG
  print_data(__func__, netfn, cmd, tbuf, tlen);
#endif

  ret = bic_ipmb_wrapper(slot_id, netfn, cmd, tbuf, tlen, rbuf, &rlen);
  if ( ret < 0 ) {
    syslog(LOG_WARNING, "%s() Cannot enable bic fw update, slot id:%d, intf: 0x%X", __func__, slot_id, intf);
    return ret;
  }

#ifdef DEBUG
  print_data(__func__, netfn, cmd, rbuf, rlen);
#endif

  if ( (intf != NONE_INTF) && (rbuf[6] != CC_SUCCESS)) {
    syslog(LOG_WARNING, "%s() Cannot enable bic fw update, slot id:%d, intf: 0x%X, cc: 0x%X", __func__, slot_id, intf, rbuf[6]);
    return -1;
  }

  return ret;
}

static int
enable_bic_update(uint8_t slot_id) {
  return enable_bic_update_with_param(slot_id, NONE_INTF);
}

static int
enable_remote_bic_update(uint8_t slot_id, uint8_t intf) {
  return enable_bic_update_with_param(slot_id, intf);
}

static int
setup_remote_bic_i2c_speed(uint8_t slot_id, uint8_t speed, uint8_t intf) {
  uint8_t tbuf[6]  = {0x00};
  uint8_t rbuf[16] = {0x00};
  uint8_t rlen = 0;
  int ret = -1;

  memcpy(tbuf, (uint8_t *)&IANA_ID, 3);
  tbuf[3] = intf;
  tbuf[4] = speed;

#ifdef DEBUG
  print_data(__func__, NETFN_OEM_1S_REQ, CMD_OEM_1S_MSG_OUT, tbuf, sizeof(tbuf));
#endif

  ret = bic_ipmb_wrapper(slot_id, NETFN_OEM_1S_REQ, CMD_OEM_1S_MSG_OUT, tbuf, sizeof(tbuf), rbuf, &rlen);

#ifdef DEBUG
  print_data(__func__, NETFN_OEM_1S_REQ, CMD_OEM_1S_MSG_OUT, rbuf, rlen);
#endif

  return ret;
}

static int
send_start_bic_update(uint8_t slot_id, int i2cfd, int size, uint8_t intf) {
  uint8_t data[11] = { BIC_CMD_DOWNLOAD_SIZE,
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
  uint8_t tbuf[32] = {0x00};
  uint8_t rbuf[32] = {0x00};
  uint8_t tlen = 0;
  uint8_t rlen = 0;
  int ret = -1;

  if ( intf == NONE_INTF ) {
    memcpy(tbuf, data, sizeof(data));
    tbuf[1] = get_checksum(&tbuf[2], BIC_CMD_DOWNLOAD_SIZE);
    tlen = BIC_CMD_DOWNLOAD_SIZE;
    ret = i2c_io(i2cfd, tbuf, tlen, rbuf, rlen);
  } else {
    memcpy(tbuf, (uint8_t *)&IANA_ID, 3);
    tbuf[3] = intf;
    memcpy(&tbuf[4], data, sizeof(data));
    tbuf[5] = get_checksum(&tbuf[6], BIC_CMD_DOWNLOAD_SIZE);
    tlen = 15;
#ifdef DEBUG
    print_data(__func__, NETFN_OEM_1S_REQ, CMD_OEM_1S_MSG_OUT, tbuf, tlen);
#endif
    ret = bic_ipmb_wrapper(slot_id, NETFN_OEM_1S_REQ, CMD_OEM_1S_MSG_OUT, tbuf, tlen, rbuf, &rlen);
#ifdef DEBUG
    print_data(__func__, NETFN_OEM_1S_REQ, CMD_OEM_1S_MSG_OUT, rbuf, rlen);
#endif
  }

  return ret;
}

static int
read_bic_update_ack_status(uint8_t slot_id, int i2cfd, uint8_t intf) {
  uint8_t tbuf[32] = {0x00};
  uint8_t rbuf[32] = {0x00};
  uint8_t tlen = 0;
  uint8_t rlen = 0;
  int ret = -1;

  if ( intf == NONE_INTF ) {
    uint8_t exp_data[2] = {0x00, 0xCC};
    tlen = 0;
    rlen = 2;
    msleep(10);
    ret = i2c_io(i2cfd, tbuf, tlen, rbuf, rlen);
    if ( ret || (memcmp(rbuf, exp_data, sizeof(exp_data)) != 0) ) {
      printf("%s() response %x:%x, ret=%d\n", __func__, rbuf[0], rbuf[1], ret);
      ret = -1;
    }
  } else {
    memcpy(tbuf, (uint8_t *)&IANA_ID, 3);
    tbuf[3] = intf;
    tlen = 6;
    rlen = 0;
#ifdef DEBUG
    print_data(__func__, NETFN_OEM_1S_REQ, CMD_OEM_1S_MSG_OUT, tbuf, tlen);
#endif
    ret = bic_ipmb_wrapper(slot_id, NETFN_OEM_1S_REQ, CMD_OEM_1S_MSG_OUT, tbuf, tlen, rbuf, &rlen);
    if ( rlen <= 0 ) {
      printf("%s() invalid lenth %d", __func__, rlen);
      ret = -1;
    }
#ifdef DEBUG
    print_data(__func__, NETFN_OEM_1S_REQ, CMD_OEM_1S_MSG_OUT, rbuf, rlen);
#endif
  }

  return ret;
}

static int
send_complete_signal(uint8_t slot_id, int i2cfd, uint8_t intf) {
  uint8_t data[7] = { BIC_CMD_RUN_SIZE,
                      BIC_CMD_RUN_SIZE,
                      BIC_CMD_RUN,
                      (BIC_FLASH_START >> 24) & 0xff,
                      (BIC_FLASH_START >> 16) & 0xff,
                      (BIC_FLASH_START >>  8) & 0xff,
                      (BIC_FLASH_START) & 0xff};
  uint8_t tbuf[32] = {0x00};
  uint8_t rbuf[32] = {0x00};
  uint8_t tlen = 0;
  uint8_t rlen = 0;
  int ret = -1;

  if ( intf == NONE_INTF ) {
    memcpy(tbuf, data, sizeof(data));
    tbuf[1] = get_checksum(&tbuf[2], BIC_CMD_DOWNLOAD_SIZE);
    tlen = BIC_CMD_RUN_SIZE;
    rlen = 0;
    ret = i2c_io(i2cfd, tbuf, tlen, rbuf, rlen);
  } else {
    memcpy(tbuf, (uint8_t *)&IANA_ID, 3);
    tbuf[3] = intf;
    memcpy(&tbuf[4], data, sizeof(data));

    //calculate the checksum
    tbuf[5] = get_checksum(&tbuf[6], BIC_CMD_DOWNLOAD_SIZE);
    tlen = BIC_CMD_RUN_SIZE + 4;
    rlen = 0;
#ifdef DEBUG
    print_data(__func__, NETFN_OEM_1S_REQ, CMD_OEM_1S_MSG_OUT, tbuf, tlen);
#endif
    ret = bic_ipmb_wrapper(slot_id, NETFN_OEM_1S_REQ, CMD_OEM_1S_MSG_OUT, tbuf, tlen, rbuf, &rlen);
#ifdef DEBUG
    print_data(__func__, NETFN_OEM_1S_REQ, CMD_OEM_1S_MSG_OUT, rbuf, rlen);
#endif
  }

  if ( ret < 0 ) {
    printf("Failed to run the new image\n");
  }

  return ret;
}

static int
read_bic_update_status(int i2cfd) {
  const uint8_t exp_data[5] = {0x00, 0xCC, 0x03, 0x40, 0x40};
  uint8_t tbuf[16] = {0};
  uint8_t rbuf[16] = {0};
  uint8_t tlen = 0;
  uint8_t rlen = 0;
  int ret = -1;

  tbuf[0] = BIC_CMD_STS_SIZE;
  tbuf[1] = BIC_CMD_STS;
  tbuf[2] = BIC_CMD_STS;
  tlen = BIC_CMD_STS_SIZE;
  rlen = 0;
  ret = i2c_io(i2cfd, tbuf, tlen, rbuf, rlen);
  if ( ret < 0 ) {
    printf("%s() failed to get status\n", __func__);
    goto exit;
  }

  tlen = 0;
  rlen = 5;
  ret = i2c_io(i2cfd, tbuf, tlen, rbuf, rlen);
  if ( ret < 0 ) {
    printf("%s() failed to get status ack\n", __func__);
    goto exit;
  }

  if ( memcmp(rbuf, exp_data, sizeof(exp_data)) != 0 ) {
    printf("%s() status: %x:%x:%x:%x:%x\n", __func__, rbuf[0], rbuf[1], rbuf[2], rbuf[3], rbuf[4]);
    goto exit;
  }

  tbuf[0] = 0xCC;
  tlen = 1;
  rlen = 0;
  ret = i2c_io(i2cfd, tbuf, tlen, rbuf, rlen);
  if ( ret < 0) {
    printf("%s() failed to send an ack\n", __func__);
    goto exit;
  }

exit:
  return ret;
}

static int
send_bic_image_data(uint8_t slot_id, int i2cfd, uint16_t len, uint8_t *buf, uint8_t intf) {
  uint8_t data[3] = {0x00};
  uint8_t tbuf[256] = {0x00};
  uint8_t rbuf[16]  = {0x00};
  uint8_t tlen = 0;
  uint8_t rlen = 0;
  int ret = -1;

  data[0] = len + 3;
  data[1] = get_checksum(buf, len) + BIC_CMD_DATA;
  data[2] = BIC_CMD_DATA;

  if ( intf == NONE_INTF ) {
    memcpy(tbuf, data, sizeof(data));
    memcpy(&tbuf[3], buf, len);
    tlen = data[0];
    rlen = 0;
    ret = i2c_io(i2cfd, tbuf, tlen, rbuf, rlen);
  } else {
    memcpy(tbuf, (uint8_t *)&IANA_ID, 3);
    tbuf[3] = intf;
    memcpy(&tbuf[4], data, sizeof(data));
    memcpy(&tbuf[7], buf, len);
    tlen = data[0] + 4; //IANA_ID + intf
    rlen = 0;
#ifdef DEBUG
    print_data(__func__, NETFN_OEM_1S_REQ, CMD_OEM_1S_MSG_OUT, tbuf, tlen);
#endif
    ret = bic_ipmb_wrapper(slot_id, NETFN_OEM_1S_REQ, CMD_OEM_1S_MSG_OUT, tbuf, tlen, rbuf, &rlen);
    if ( rbuf[4] == 0x0 ) {
      syslog(LOG_WARNING, "%s() unexpected rbuf[4]=0x%x", __func__, rbuf[4]);
      ret = -1;
    }
#ifdef DEBUG
    print_data(__func__, NETFN_OEM_1S_REQ, CMD_OEM_1S_MSG_OUT, rbuf, rlen);
#endif

  }

  if ( ret < 0 ) {
    syslog(LOG_WARNING, "%s() Cannot send data of the image, slot id:%d, update intf: 0x%X", __func__, slot_id, intf);
  }

  return ret;
}

static int
send_bic_runtime_image_data(uint8_t slot_id, int fd, int i2cfd, int file_size, const uint8_t bytes_per_read, uint8_t intf) {
  uint8_t buf[256] = {0};
  uint8_t read_bytes = 0;
  int ret = -1;
  int dsize = 0;
  int last_offset = 0;
  int offset = 0;;

  dsize = file_size / 20;

  //reinit the fd to the beginning
  if ( lseek(fd, 0, SEEK_SET) != 0 ) {
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
    if ( (last_offset + dsize) <= offset) {
      printf("updated bic: %d %%\n", (offset/dsize)*5);
      fflush(stdout);
      last_offset += dsize;
    }

    if ( intf != NONE_INTF ) {
      ret = send_bic_image_data(slot_id, i2cfd, read_bytes, buf, intf);
      if ( ret < 0 ) {
        return ret;
      }
    } else {

      ret = read_bic_update_status(i2cfd);
      if ( ret < 0 ) {
        return ret;
      }

      ret = send_bic_image_data(slot_id, i2cfd, read_bytes, buf, NONE_INTF);
      if ( ret < 0 ) {
        return ret;
      }

      ret = read_bic_update_ack_status(slot_id, i2cfd, NONE_INTF);
      if ( ret < 0 ) {
        return ret;
      }
    }
  }

  return ret;
}

static int
update_bic(uint8_t slot_id, int fd, int file_size) {
  int ret = -1;
  int i2cfd;
  char cmd[100] = {0};
  size_t cmd_size = sizeof(cmd);
  uint8_t bus_num;
  const uint8_t I2CBASE = 0x40;
  const uint8_t bytes_per_read = 252;
  struct rlimit mqlim;

  //step1 -get the bus number and open the dev of i2c
  bus_num = fby3_common_get_bus_id(slot_id);
  syslog(LOG_CRIT, "%s: update bic firmware on slot %d\n", __func__, slot_id);

  i2cfd = i2c_open(bus_num, BRIDGE_SLAVE_ADDR);
  if ( i2cfd < 0 ) {
    printf("Cannot open /dev/i2c-%d\n", bus_num);
    goto exit;
  }

  //step2 - kill ipmbd
  snprintf(cmd, cmd_size, "sv stop ipmbd_%d", bus_num);
  if (system(cmd) != 0) {
      syslog(LOG_WARNING, "[%s] %s failed\n", __func__, cmd);
      return BIC_STATUS_FAILURE;
  }
  printf("stop ipmbd for slot %x..\n", slot_id);

  //step3 - adjust the i2c speed and set properties of mqlim
  snprintf(cmd, cmd_size, "devmem 0x1e78a%03X w 0xFFFFE303", I2CBASE + (I2CBASE * bus_num) + 4);
  if (system(cmd) != 0) {
      syslog(LOG_WARNING, "[%s] %s failed\n", __func__, cmd);
      return BIC_STATUS_FAILURE;
  }
  sleep(1);
  printf("stopped ipmbd for slot %x..\n", slot_id);

  if ( is_bic_ready(slot_id, NONE_INTF) < 0 ) {
    printf("BIC is not ready after sleep 1s\n");
    goto exit;
  }

  mqlim.rlim_cur = RLIM_INFINITY;
  mqlim.rlim_max = RLIM_INFINITY;
  if ( setrlimit(RLIMIT_MSGQUEUE, &mqlim) < 0) {
    goto exit;
  }

  snprintf(cmd, cmd_size, "/usr/local/bin/ipmbd -u %d %d > /dev/null 2>&1 &", bus_num, slot_id);
  if (system(cmd) != 0) {
      syslog(LOG_WARNING, "[%s] %s failed\n", __func__, cmd);
      return BIC_STATUS_FAILURE;
  }
  printf("start ipmbd -u for this slot %x..\n", slot_id);

  //assume ipmbd that it will be ready in 2s
  sleep(2);

  //step4 - enable bic update
  ret = enable_bic_update(slot_id);
  if ( ret < 0 ) {
    syslog(LOG_WARNING, "%s() Failed to enable the bic update", __func__);
    //goto exit;
  }

  //step5 - kill ipmbd
  snprintf(cmd, cmd_size, "ps -w | grep -v 'grep' | grep 'ipmbd -u %d' |awk '{print $1}'| xargs kill", bus_num);
  if (system(cmd) != 0) {
      syslog(LOG_WARNING, "[%s] %s failed\n", __func__, cmd);
      return BIC_STATUS_FAILURE;
  }
  printf("stop ipmbd -u for slot %x..\n", slot_id);

  //make sure that BIC enters bootloader
  sleep(3);

  //step6 - send cmd 0x21 to notice BIC the update will start
  ret = send_start_bic_update(slot_id, i2cfd, file_size, NONE_INTF);
  if ( ret < 0 ) {
    printf("Failed to send a signal to start the update of BIC\n");
    goto exit;
  }

  msleep(600);

  //step7 - check the response
  ret = read_bic_update_ack_status(slot_id, i2cfd, NONE_INTF);
  if ( ret < 0 ) {
    printf("Failed to get the response of the command\n");
    goto exit;
  }

  //step8 - loop to send all the image data
  ret = send_bic_runtime_image_data(slot_id, fd, i2cfd, file_size, bytes_per_read, NONE_INTF);
  if ( ret < 0 ) {
    printf("Failed to send image data\n");
    goto exit;
  }

  msleep(500);

  //step9 - run the new image
  ret = send_complete_signal(slot_id, i2cfd, NONE_INTF);
  if ( ret < 0 ) {
    printf("Failed to send a complete signal\n");
  }

  //step10 - check the response
  ret = read_bic_update_ack_status(slot_id, i2cfd, NONE_INTF);
  if ( ret < 0 ) {
    printf("Failed to get the response of the command\n");
    goto exit;
  }

exit:
  //step11 - recover the i2c speed
  snprintf(cmd, cmd_size, "devmem 0x1e78a%03X w 0xFFFCB300", I2CBASE + (I2CBASE * bus_num) + 4);
  if (system(cmd) != 0) {
      syslog(LOG_WARNING, "[%s] %s failed\n", __func__, cmd);
      return BIC_STATUS_FAILURE;
  }
  msleep(500);

  //step12 - restart the ipmbd
  snprintf(cmd, cmd_size, "sv start ipmbd_%d", bus_num);
  if (system(cmd) != 0) {
      syslog(LOG_WARNING, "[%s] %s failed\n", __func__, cmd);
      return BIC_STATUS_FAILURE;
  }

  syslog(LOG_CRIT, "%s: updating bic firmware is exiting on slot %d\n", __func__, slot_id);

  if ( i2cfd > 0 ) {
    close(i2cfd);
  }

  return ret;
}


static int
update_remote_bic(uint8_t slot_id, uint8_t intf, int fd, int file_size) {
  int ret;
  uint8_t is_fail = 0;
  const uint8_t bytes_per_read = 244;
  uint8_t bic_read   = 0xff;
  uint8_t bic_write  = 0xff;
  uint8_t bic_update = 0xff;
  uint8_t bic_i2c    = 0xff;

  switch (intf) {
    case FEXP_BIC_INTF:
      bic_read   = FEXP_BIC_I2C_READ;
      bic_write  = FEXP_BIC_I2C_WRITE;
      bic_update = FEXP_BIC_I2C_UPDATE;
      bic_i2c    = FEXP_BIC_IPMI_I2C_SW;
      break;
    case BB_BIC_INTF:
      bic_read   = BB_BIC_I2C_READ;
      bic_write  = BB_BIC_I2C_WRITE;
      bic_update = BB_BIC_I2C_UPDATE;
      bic_i2c    = BB_BIC_IPMI_I2C_SW;
      break;
    case REXP_BIC_INTF:
      bic_read   = REXP_BIC_I2C_READ;
      bic_write  = REXP_BIC_I2C_WRITE;
      bic_update = REXP_BIC_I2C_UPDATE;
      bic_i2c    = REXP_BIC_IPMI_I2C_SW;
      break;
    default:
      syslog(LOG_WARNING, "%s() unknown information, slot id:%d, intf: 0x%X", __func__, slot_id, intf);
      return -1;
  }

  printf("bytes/per read = %d\n", bytes_per_read);

  //step1 - make the remote bic enter bootloader
  ret = enable_remote_bic_update(slot_id, intf);
  if ( ret < 0 ) {
    syslog(LOG_WARNING, "Enable the remote bic update, Fail, ret = %d\n", ret);
    // goto exit;
  }

  //step2 - adjust the bus of i2c speed of bridge BIC
  ret = setup_remote_bic_i2c_speed(slot_id, I2C_100K, bic_i2c);
  if ( ret < 0 ) {
    syslog(LOG_WARNING, "Set the speed of the remote bic to 100K, Fail, ret = %d\n", ret);
    goto exit;
  }

  //wait for it ready
  sleep(3);

  //step3 - send a signal to notice it that starts the update process
  ret = send_start_bic_update(slot_id, 0, file_size, bic_write);
  if ( ret < 0 ) {
    syslog(LOG_WARNING, "Send start remote bic update, Fail, ret = %d\n", ret);
    goto exit;
  }

  //wait for it ready
  msleep(500);

  //step4 - check the response of the signal
  ret = read_bic_update_ack_status(slot_id, 0, bic_read);
  if ( ret < 0 ) {
    syslog(LOG_WARNING, "Check remote bic update status, Fail, ret = %d\n", ret);
    goto exit;
  }

  //step5 - send the pieces data of the image till the end
  ret = send_bic_runtime_image_data(slot_id, fd, 0, file_size, bytes_per_read, bic_update);
  if ( ret < 0 ) {
    syslog(LOG_WARNING, "Send image data, Fail, ret = %d\n", ret);
    goto exit;
  }

  //step6 - send a signal to complete it
  ret = send_complete_signal(slot_id, 0, bic_write);
  if ( ret < 0 ) {
    syslog(LOG_WARNING, "Send complete signal, Fail, ret = %d\n", ret);
    goto exit;
  }

  //step7 - check the response of the signal
  ret = read_bic_update_ack_status(slot_id, 0, bic_read);
  if ( ret < 0 ) {
    syslog(LOG_WARNING, "Get the bic status, Fail, ret = %d\n", ret);
  }

exit:
  //record the value
  is_fail = ret;

  //step8 - recover the bus of the i2c speed
  ret = setup_remote_bic_i2c_speed(slot_id, I2C_1M, bic_i2c);
  if ( ret < 0 ) {
    syslog(LOG_WARNING, "Set the speed of the remote bic to 1M, Fail, ret = %d\n", ret);
  }

  if ( ret == 0 ) ret = is_fail;

  return ret;
}

static int
is_valid_intf(uint8_t intf) {
  int ret = BIC_STATUS_FAILURE;
  switch(intf) {
    case FEXP_BIC_INTF:
    case BB_BIC_INTF:
    case REXP_BIC_INTF:
    case NONE_INTF:
      ret = BIC_STATUS_SUCCESS;
      break;
  }

  return ret;
}

static int
is_valid_bic_image(uint8_t slot_id, uint8_t comp, uint8_t intf, int fd, int file_size){
#define BICBL_TAG 0x00
#define BICBR_TAG 0x01
#define BICBL_OFFSET 0x3f00
#define BICBR_OFFSET 0x8000

#define REVISION_ID(x) ((x >> 4) & 0x0f)
#define COMPONENT_ID(x) (x & 0x0f)

  int ret = BIC_STATUS_FAILURE;
  uint8_t rbuf[2] = {0};
  uint8_t rlen = sizeof(rbuf);
  uint8_t sel_comp = 0xff;
  uint8_t sel_tag = 0xff;
  uint32_t sel_offset = 0xffffffff;
  uint8_t board_type = 0;

  switch (comp) {
    case UPDATE_BIC:
      sel_tag = BICBR_TAG;
      sel_offset = BICBR_OFFSET;
      break;
    case UPDATE_BIC_BOOTLOADER:
      sel_tag = BICBL_TAG;
      sel_offset = BICBL_OFFSET;
      break;
  }

  switch (intf) {
    case FEXP_BIC_INTF:
      sel_comp = BIC1OU;
      break;
    case BB_BIC_INTF:
      sel_comp = BICBB;
      break;
    case REXP_BIC_INTF:
      if ( fby3_common_get_2ou_board_type(slot_id, &board_type) < 0 ) {
        syslog(LOG_ERR, "%s() Cannot get board_type", __func__);
        goto error_exit;
      }
      if ( board_type == M2_BOARD ) {
        sel_comp = BIC2OU;
      } else if ( board_type == E1S_BOARD ) {
        sel_comp = BICSPE;
      } else {
        sel_comp = BICGPV3;
      }
      break;
    case NONE_INTF:
      sel_comp = BICDL;
      break;
  }

  if ( lseek(fd, sel_offset, SEEK_SET) != (off_t)sel_offset ) {
    goto error_exit;
  }

  if ( read(fd, rbuf, rlen) != (off_t)rlen ) {
    goto error_exit;
  }

  if ( rbuf[0] != sel_tag || COMPONENT_ID(rbuf[1]) != COMPONENT_ID(sel_comp) ) {
    goto error_exit;
  }

  ret = BIC_STATUS_SUCCESS;

error_exit:
  if ( ret == BIC_STATUS_FAILURE) {
    printf("This file cannot be updated to this component!\n");
  }

  return ret;
}

static int
open_and_get_size(char *path, int *file_size) {
  struct stat finfo;
  int fd;

  fd = open(path, O_RDONLY, 0666);
  if ( fd < 0 ) {
    return fd;
  }

  fstat(fd, &finfo);
  *file_size = finfo.st_size;

  return fd;
}

static int
update_bic_runtime_fw(uint8_t slot_id, uint8_t comp,uint8_t intf, char *path, uint8_t force) {
  int ret = -1;
  int fd = 0;
  int file_size;

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

  printf("file size = %d bytes, slot = %d, intf = 0x%x\n", file_size, slot_id, intf);

  //check the content of the image
  if( !force && is_valid_bic_image(slot_id, comp, intf, fd, file_size) ) {
    printf("Invalid BIC file!\n");
    ret = -1;
    goto exit;
  }

  //run into the different function based on the interface
  switch (intf) {
    case FEXP_BIC_INTF:
    case BB_BIC_INTF:
    case REXP_BIC_INTF:
      ret = update_remote_bic(slot_id, intf, fd, file_size);
      break;
    case NONE_INTF:
      ret = update_bic(slot_id, fd, file_size);
      break;
  }

exit:

  if ( fd > 0 ) {
    close(fd);
  }

  return ret;
}

static int
send_image_data_via_bic(uint8_t slot_id, uint8_t comp, uint8_t intf, uint32_t offset, uint16_t len, uint8_t *buf)
{
  uint8_t tbuf[256] = {0};
  uint8_t rbuf[16] = {0};
  uint8_t tlen = 0;
  uint8_t rlen = 0;
  int ret = -1;
  int retries = 3;

  memcpy(tbuf, (uint8_t *)&IANA_ID, 3);
  tbuf[3] = comp;
  tbuf[4] = (offset) & 0xFF;
  tbuf[5] = (offset >>  8) & 0xFF;
  tbuf[6] = (offset >> 16) & 0xFF;
  tbuf[7] = (offset >> 24) & 0xFF;
  tbuf[8] = len & 0xFF;
  tbuf[9] = (len >> 8) & 0xFF;
  memcpy(&tbuf[10], buf, len);
  tlen = len + 10;

  do {
    ret = bic_ipmb_send(slot_id, NETFN_OEM_1S_REQ, CMD_OEM_1S_UPDATE_FW, tbuf, tlen, rbuf, &rlen, intf);
    if (ret != BIC_STATUS_SUCCESS) {
      if (ret == BIC_STATUS_NOT_SUPP_IN_CURR_STATE)
        return ret;
      printf("%s() slot: %d, target: %d, offset: %d, len: %d retrying..\n", __func__, slot_id, comp, offset, len);
    }
  } while( (ret < 0) && (retries--));

  return ret;
}

#define IPMB_MAX_SEND 224
static int
update_fw_bic_bootloader(uint8_t slot_id, uint8_t comp, uint8_t intf, int fd, int file_size) {
  const uint8_t bytes_per_read = IPMB_MAX_SEND;
  uint8_t buf[256] = {0};
  uint16_t buf_size = sizeof(buf);
  uint16_t read_bytes = 0;
  uint32_t offset = 0;
  uint32_t last_offset = 0;
  uint32_t dsize = 0;
  int ret = -1;

  dsize = file_size / 20;

  if ( lseek(fd, 0, SEEK_SET) != 0 ) {
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
      comp |= 0x80;
    }
    ret = send_image_data_via_bic(slot_id, comp, intf, offset, read_bytes, buf);
    if (ret != BIC_STATUS_SUCCESS)
      break;

    offset += read_bytes;
    if ((last_offset + dsize) <= offset) {
      printf("updated bic bootloader: %d %%\n", (offset/dsize)*5);
      fflush(stdout);
      last_offset += dsize;
    }
  }
  return ret;
}

static int
update_bic_bootloader_fw(uint8_t slot_id, uint8_t comp, uint8_t intf, char *path, uint8_t force) {
  int fd = 0;
  int ret = 0;
  int file_size = 0;

  fd = open_and_get_size(path, &file_size);
  if (fd < 0) {
    syslog(LOG_WARNING, "%s() cannot open the file: %s, fd=%d", __func__, path, fd);
    goto exit;
  }

  printf("file size = %d bytes, slot = %d, comp = 0x%x\n", file_size, slot_id, comp);

  //check the content of the image
  ret = is_valid_bic_image(slot_id, comp, intf, fd, file_size);
  if (ret < 0) {
    printf("Invalid BIC bootloader file!\n");
    goto exit;
  }

  ret = update_fw_bic_bootloader(slot_id, comp, intf, fd, file_size);

exit:
  if (fd > 0) {
    close(fd);
  }

  return ret;
}

static char*
get_component_name(uint8_t comp) {
  switch (comp) {
    case FW_CPLD:
      return "SB CPLD";
    case FW_BIC:
      return "SB BIC";
    case FW_BIC_BOOTLOADER:
      return "SB BIC Bootloader";
    case FW_VR:
      return "VR";
    case FW_BIOS:
      return "BIOS";
    case FW_1OU_BIC:
      return "1OU BIC";
    case FW_1OU_BIC_BOOTLOADER:
      return "1OU BIC Bootloader";
    case FW_1OU_CPLD:
      return "1OU CPLD";
    case FW_2OU_BIC:
      return "2OU BIC";
    case FW_2OU_BIC_BOOTLOADER:
      return "2OU BIC Bootloader";
    case FW_2OU_CPLD:
      return "2OU CPLD";
    case FW_BB_BIC:
      return "BB BIC";
    case FW_BB_BIC_BOOTLOADER:
      return "BB BIC Bootloader";
    case FW_BB_CPLD:
      return "BB CPLD";
    case FW_BIOS_CAPSULE:
      return "BIOS Capsule, Target to Active Region";
    case FW_BIOS_RCVY_CAPSULE:
      return "BIOS Capsule, Target to Recovery Region";
    case FW_CPLD_CAPSULE:
      return "CPLD Capsule, Target to Active Region";
    case FW_CPLD_RCVY_CAPSULE:
      return "CPLD Capsule, Target to Recovery Region";
    default:
      return "Unknown";
  }

  return "NULL";
}

int
bic_update_fw(uint8_t slot_id, uint8_t comp, char *path, uint8_t force) {
  int ret = BIC_STATUS_SUCCESS;
  uint8_t intf = 0x0;
  char ipmb_content[] = "ipmb";
  char* loc = strstr(path, ipmb_content);

  printf("slot_id: %x, comp: %x, intf: %x, img: %s, force: %x\n", slot_id, comp, intf, path, force);
  syslog(LOG_CRIT, "Updating %s on slot%d. File: %s", get_component_name(comp), slot_id, path);

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
      intf = REXP_BIC_INTF;
      break;
    case FW_BB_BIC:
    case FW_BB_BIC_BOOTLOADER:
    case FW_BB_CPLD:
      intf = BB_BIC_INTF;
      break;
  }

  //run cmd
  switch (comp) {
    case FW_BIC:
    case FW_1OU_BIC:
    case FW_2OU_BIC:
    case FW_BB_BIC:
      ret = update_bic_runtime_fw(slot_id, UPDATE_BIC, intf, path, force);
      break;
    case FW_BIC_BOOTLOADER:
    case FW_1OU_BIC_BOOTLOADER:
    case FW_2OU_BIC_BOOTLOADER:
    case FW_BB_BIC_BOOTLOADER:
      ret = update_bic_bootloader_fw(slot_id, UPDATE_BIC_BOOTLOADER, intf , path, force);
      break;
    case FW_1OU_CPLD:
    case FW_2OU_CPLD:
      if (loc != NULL) {
        ret = update_bic_cpld_lattice(slot_id, path, intf, force);
      } else {
        ret = update_bic_cpld_lattice_usb(slot_id, path, intf, force);
      }
      break;
    case FW_BB_CPLD:
    case FW_CPLD:
      ret = update_bic_cpld_altera(slot_id, path, intf, force);
      break;
    case FW_BIOS:
    case FW_BIOS_CAPSULE:
    case FW_CPLD_CAPSULE:
    case FW_BIOS_RCVY_CAPSULE:
    case FW_CPLD_RCVY_CAPSULE:
      if (loc != NULL) {
        ret = update_bic_bios(slot_id, comp, path, FORCE_UPDATE_SET);
      } else {
        ret = update_bic_usb_bios(slot_id, comp, path);
      }
      break;
    case FW_VR:
      ret = update_bic_vr(slot_id, path, force);
      break;
  }

  syslog(LOG_CRIT, "Updated %s on slot%d. File: %s. Result: %s", get_component_name(comp), slot_id, path, (ret < 0)?"Fail":"Success");
  return ret;
}
