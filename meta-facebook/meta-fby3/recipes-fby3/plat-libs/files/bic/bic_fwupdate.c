
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
#include <openbmc/obmc-i2c.h>
#include "bic_fwupdate.h"
#include "bic_cpld_altera_fwupdate.h"
#include "bic_cpld_lattice_fwupdate.h"
#include "bic_vr_fwupdate.h"
#include "bic_bios_fwupdate.h"
#include "bic_mchp_pciesw_fwupdate.h"
#include "bic_brcm_pciesw_fwupdate.h"
#include "bic_m2_fwupdate.h"
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

#define IPMB_BIC_RETRY 3
#define SELF_TEST_RESP_LEN 2

#define IPMB_BRIDGE_OVERHEAD 9

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
  BB_BIC_IPMI_I2C_SW   = 0x2B,

  RREXP2_BIC_I2C_WRITE   = 0x41,
  RREXP2_BIC_I2C_READ    = 0x42,
  RREXP2_BIC_I2C_UPDATE  = 0x43,
  RREXP2_BIC_IPMI_I2C_SW = 0x44,
  RREXP1_BIC_I2C_WRITE   = 0x46,
  RREXP1_BIC_I2C_READ    = 0x47,
  RREXP1_BIC_I2C_UPDATE  = 0x48,
  RREXP1_BIC_IPMI_I2C_SW = 0x49,
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
  uint8_t tbuf[16] = {0x00};
  uint8_t rbuf[20] = {0x00};
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

    case RREXP_BIC_INTF1:
    case RREXP_BIC_INTF2:
      netfn  = NETFN_OEM_1S_REQ;
      cmd    = CMD_OEM_1S_MSG_OUT;
      tbuf[3] = REXP_BIC_INTF;
      tbuf[4] = NETFN_OEM_1S_REQ << 2;
      tbuf[5] = CMD_OEM_1S_MSG_OUT;
      memcpy(&tbuf[6], (uint8_t *)&IANA_ID, 3);
      tbuf[9] = intf;
      tbuf[10] = NETFN_OEM_1S_REQ << 2;
      tbuf[11] = CMD_OEM_1S_ENABLE_BIC_UPDATE;
      memcpy(&tbuf[12], (uint8_t *)&IANA_ID, 3);
      tbuf[15] = 0x01; //Update bic via i2c
      tlen = 16;
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

  if ((intf == RREXP_BIC_INTF1 || intf == RREXP_BIC_INTF2) && rbuf[13] != CC_SUCCESS) {
    syslog(LOG_WARNING, "%s() Cannot enable bic fw update, slot id:%d, intf: 0x%X, cc: 0x%X", __func__, slot_id, intf, rbuf[13]);
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
  uint8_t tbuf[16] = {0x00};
  uint8_t rbuf[20] = {0x00};
  uint8_t tlen = 0;
  uint8_t rlen = 0;
  int ret = -1;

  switch (intf) {
    case RREXP1_BIC_IPMI_I2C_SW:
    case RREXP2_BIC_IPMI_I2C_SW:
      memcpy(tbuf, (uint8_t *)&IANA_ID, 3);
      tbuf[3] = REXP_BIC_INTF;
      tbuf[4] = NETFN_OEM_1S_REQ << 2;
      tbuf[5] = CMD_OEM_1S_MSG_OUT;
      memcpy(&tbuf[6], (uint8_t *)&IANA_ID, 3);
      tbuf[9] = intf;
      tbuf[10] = speed;
      tlen = 11;
      break;

    default:
      memcpy(tbuf, (uint8_t *)&IANA_ID, 3);
      tbuf[3] = intf;
      tbuf[4] = speed;
      tlen = 5;
  }

#ifdef DEBUG
  print_data(__func__, NETFN_OEM_1S_REQ, CMD_OEM_1S_MSG_OUT, tbuf, sizeof(tbuf));
#endif

  ret = bic_ipmb_wrapper(slot_id, NETFN_OEM_1S_REQ, CMD_OEM_1S_MSG_OUT, tbuf, tlen, rbuf, &rlen);

#ifdef DEBUG
  print_data(__func__, NETFN_OEM_1S_REQ, CMD_OEM_1S_MSG_OUT, rbuf, rlen);
#endif

  if ((intf == RREXP1_BIC_IPMI_I2C_SW || intf == RREXP2_BIC_IPMI_I2C_SW) && rbuf[6] != CC_SUCCESS) {
    return -1;
  }

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
  } else if ( intf == RREXP1_BIC_I2C_WRITE || intf == RREXP2_BIC_I2C_WRITE ) {
    memcpy(tbuf, (uint8_t *)&IANA_ID, 3);
    tbuf[3] = REXP_BIC_INTF;
    tbuf[4] = NETFN_OEM_1S_REQ << 2;
    tbuf[5] = CMD_OEM_1S_MSG_OUT;
    memcpy(&tbuf[6], (uint8_t *)&IANA_ID, 3);
    tbuf[9] = intf;
    memcpy(&tbuf[10], data, sizeof(data));
    tbuf[11] = get_checksum(&tbuf[12], BIC_CMD_DOWNLOAD_SIZE);
    tlen = 21;

    ret = bic_ipmb_wrapper(slot_id, NETFN_OEM_1S_REQ, CMD_OEM_1S_MSG_OUT, tbuf, tlen, rbuf, &rlen);
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

  if ((intf == RREXP1_BIC_I2C_WRITE || intf == RREXP2_BIC_I2C_WRITE) && rbuf[6] != CC_SUCCESS) {
    return -1;
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
  } else if (intf == RREXP1_BIC_I2C_READ || intf == RREXP2_BIC_I2C_READ) {
    memcpy(tbuf, (uint8_t *)&IANA_ID, 3);
    tbuf[3] = REXP_BIC_INTF;
    tbuf[4] = NETFN_OEM_1S_REQ << 2;
    tbuf[5] = CMD_OEM_1S_MSG_OUT;
    memcpy(&tbuf[6], (uint8_t *)&IANA_ID, 3);
    tbuf[9] = intf;
    tlen = 12;
    rlen = 0;

    ret = bic_ipmb_wrapper(slot_id, NETFN_OEM_1S_REQ, CMD_OEM_1S_MSG_OUT, tbuf, tlen, rbuf, &rlen);
    if ( rlen == 0 ) {
      printf("%s() invalid lenth %d", __func__, rlen);
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
    if ( rlen == 0 ) {
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
  } else if ( intf == RREXP1_BIC_I2C_WRITE || intf == RREXP2_BIC_I2C_WRITE ) {
    memcpy(tbuf, (uint8_t *)&IANA_ID, 3);
    tbuf[3] = REXP_BIC_INTF;
    tbuf[4] = NETFN_OEM_1S_REQ << 2;
    tbuf[5] = CMD_OEM_1S_MSG_OUT;
    memcpy(&tbuf[6], (uint8_t *)&IANA_ID, 3);
    tbuf[9] = intf;
    memcpy(&tbuf[10], data, sizeof(data));

    //calculate the checksum
    tbuf[11] = get_checksum(&tbuf[12], BIC_CMD_DOWNLOAD_SIZE);
    tlen = BIC_CMD_RUN_SIZE + 4 + 6;
    rlen = 0;

    ret = bic_ipmb_wrapper(slot_id, NETFN_OEM_1S_REQ, CMD_OEM_1S_MSG_OUT, tbuf, tlen, rbuf, &rlen);
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

  if ((intf == RREXP1_BIC_I2C_WRITE || intf == RREXP2_BIC_I2C_WRITE) && rbuf[6] != CC_SUCCESS) {
    return -1;
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
  } else if ( intf == RREXP1_BIC_I2C_UPDATE || intf == RREXP2_BIC_I2C_UPDATE ) {
    memcpy(tbuf, (uint8_t *)&IANA_ID, 3);
    tbuf[3] = REXP_BIC_INTF;
    tbuf[4] = NETFN_OEM_1S_REQ << 2;
    tbuf[5] = CMD_OEM_1S_MSG_OUT;
    memcpy(&tbuf[6], (uint8_t *)&IANA_ID, 3);
    tbuf[9] = intf;
    memcpy(&tbuf[10], data, sizeof(data));
    memcpy(&tbuf[13], buf, len);
    tlen = data[0] + 4 + 6; //IANA_ID + intf
    rlen = 0;

    ret = bic_ipmb_wrapper(slot_id, NETFN_OEM_1S_REQ, CMD_OEM_1S_MSG_OUT, tbuf, tlen, rbuf, &rlen);
    if ( rbuf[10] == 0x0 ) {
      syslog(LOG_WARNING, "%s() unexpected rbuf[4]=0x%x", __func__, rbuf[4]);
      ret = -1;
    }
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

  if ((intf == RREXP1_BIC_I2C_UPDATE || intf == RREXP2_BIC_I2C_UPDATE) && rbuf[6] != CC_SUCCESS) {
    return -1;
  }

  return ret;
}

static int
send_bic_runtime_image_data(uint8_t slot_id, int fd, int i2cfd, int file_size, uint8_t bytes_per_read, uint8_t intf) {
  uint8_t buf[256] = {0};
  ssize_t read_bytes = 0;
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
  uint8_t bytes_per_read = 252;
  struct rlimit mqlim;

  //step1 -get the bus number and open the dev of i2c
  bus_num = fby3_common_get_bus_id(slot_id);

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

  sleep(3);

  if ( i2cfd > 0 ) {
    close(i2cfd);
  }

  return ret;
}

// Wait for warm reset finished
void
wait_bic_warm_reset(uint8_t slot_id, uint8_t intf) {
  int retry = IPMB_BIC_RETRY;
  int ret;
  uint8_t self_test_result[SELF_TEST_RESP_LEN] = {0};

  sleep(3);
  while (retry > 0){
    ret = bic_get_self_test_result(slot_id, (uint8_t *)&self_test_result, intf);
    if (ret == 0) {
      break;
    } else {
      retry--;
    }
    sleep(1);
  }
  if (retry == 0) {
    syslog(LOG_WARNING, "%s: BIC IPMB is not ready after %d retry, intf: %x\n", __func__, IPMB_BIC_RETRY, intf);
  }
}

static int
update_remote_bic(uint8_t slot_id, uint8_t intf, int fd, int file_size) {
  int ret = 0;
  uint8_t is_fail = 0;
  uint8_t bytes_per_read = 244;
  uint8_t bic_read   = 0xff;
  uint8_t bic_write  = 0xff;
  uint8_t bic_update = 0xff;
  uint8_t bic_i2c    = 0xff;
  uint8_t bmc_location = 0;

  if (fby3_common_get_bmc_location(&bmc_location) != 0) {
    printf("Cannot get the location of BMC");
    return -1;
  }

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
    case RREXP_BIC_INTF1:
      bic_read   = RREXP1_BIC_I2C_READ;
      bic_write  = RREXP1_BIC_I2C_WRITE;
      bic_update = RREXP1_BIC_I2C_UPDATE;
      bic_i2c    = RREXP1_BIC_IPMI_I2C_SW;
      bytes_per_read = 224;
      break;
    case RREXP_BIC_INTF2:
      bic_read   = RREXP2_BIC_I2C_READ;
      bic_write  = RREXP2_BIC_I2C_WRITE;
      bic_update = RREXP2_BIC_I2C_UPDATE;
      bic_i2c    = RREXP2_BIC_IPMI_I2C_SW;
      bytes_per_read = 224;
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
  sleep(5);

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

  if ((intf == BB_BIC_INTF) && (bmc_location == NIC_BMC)) {
    wait_bic_warm_reset(slot_id, intf);
  }

  if ( ret == 0 ) ret = is_fail;

  return ret;
}

static int
is_valid_bic_image(uint8_t slot_id, uint8_t comp, uint8_t intf, int fd, int file_size){
#define BICBL_TAG 0x00
#define BICBR_TAG 0x01
#define BICBL_OFFSET 0x3f00
#define BICBR_OFFSET 0x8000

  int ret = BIC_STATUS_FAILURE;
  uint8_t rbuf[2] = {0};
  uint8_t signed_bytes[2] = {0};
  uint8_t rlen = sizeof(rbuf);
  uint8_t sel_comp = 0xff;
  uint8_t sel_tag = 0xff;
  uint32_t sel_offset = 0xffffffff;
  uint8_t board_type = 0;
  uint8_t board_revision_id = 0xff;
  uint8_t tbuf[4] = {0};
  uint8_t tlen = 0;
  int ret_val = 0 , retry = 0;
  int board_type_index = 0;
  bool board_rev_is_invalid = false;
  bool check_board_revision = true;
  bic_gpio_t gpio = {0};
  uint8_t hsc_det = 0;

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
      check_board_revision = false;
      bic_get_1ou_type(slot_id, &board_type);
      if (board_type == EDSFF_1U) {
        sel_comp = BIC1OU_E1S;
      } else {
        sel_comp = BIC1OU;
      }
      break;
    case BB_BIC_INTF:
      sel_comp = BICBB;
      break;
    case REXP_BIC_INTF:
      check_board_revision = false;
      if ( fby3_common_get_2ou_board_type(slot_id, &board_type) < 0 ) {
        syslog(LOG_ERR, "%s() Cannot get board_type", __func__);
        goto error_exit;
      }
      if ( board_type == M2_BOARD ) {
        sel_comp = BIC2OU;
      } else if ( board_type == E1S_BOARD ) {
        sel_comp = BICSPE;
      } else if ( board_type == CWC_MCHP_BOARD ) {
        sel_comp = BICCWC;
      } else {
        sel_comp = BICGPV3;
      }
      break;
    case RREXP_BIC_INTF1:
    case RREXP_BIC_INTF2:
      check_board_revision = false;
      sel_comp = BICGPV3;
      break;
    case NONE_INTF:
      sel_comp = BICDL;
      break;
  }

  if ( lseek(fd, sel_offset, SEEK_SET) != (off_t)sel_offset ) {
    goto error_exit;
  }

  rlen = sizeof(rbuf);
  if ( read(fd, rbuf, rlen) != (off_t)rlen ) {
    goto error_exit;
  }
  memcpy(signed_bytes, rbuf, sizeof(signed_bytes));

  if ( signed_bytes[0] != sel_tag || COMPONENT_ID(signed_bytes[1]) != COMPONENT_ID(sel_comp) ) {
    goto error_exit;
  }

  if (check_board_revision) {
    switch (intf) {
      /* need to Get the corresponding Board Revision */
      case BB_BIC_INTF:
        // Read Board Revision from BB CPLD
        tbuf[0] = CPLD_BB_BUS;
        tbuf[1] = CPLD_FLAG_REG_ADDR;
        tbuf[2] = 0x01;
        tbuf[3] = BB_CPLD_BOARD_REV_ID_REGISTER;
        tlen = 4;
        retry = 0;
        while (retry < RETRY_TIME) {
          ret_val = bic_ipmb_send(slot_id, NETFN_APP_REQ, CMD_APP_MASTER_WRITE_READ, tbuf, tlen, rbuf, &rlen, BB_BIC_INTF);
          if ( ret_val < 0 ) {
            retry++;
            msleep(100);
          } else {
            break;
          }
        }
        if (retry == RETRY_TIME) {
          syslog(LOG_WARNING, "%s() Failed to get board revision via BB CPLD, tlen=%d", __func__, tlen);
          goto error_exit;
        }

        board_revision_id = rbuf[0];
        break;
      case NONE_INTF:
        // Read Board Revision from SB CPLD
        if ( fby3_common_get_sb_board_rev(slot_id, &board_revision_id) ) {
          syslog(LOG_WARNING, "Failed to get sb board rev");
          goto error_exit;
        }
        // respin board, Need to process BOARD_REV through HSC_DETECT, keeping the original logic work
        ret = bic_get_gpio(slot_id, &gpio, NONE_INTF);
        if ( ret < 0 ) {
          syslog(LOG_WARNING, "%s() bic_get_gpio returns %d\n", __func__, ret);
          goto error_exit;
        }
        hsc_det |= ((GET_GPIO_BIT_VALUE(gpio, 77)) << 0); // 77    HSC_DETECT0
        hsc_det |= ((GET_GPIO_BIT_VALUE(gpio, 78)) << 1); // 78    HSC_DETECT1
        hsc_det |= ((GET_GPIO_BIT_VALUE(gpio, 79)) << 2); // 79    HSC_DETECT2
        if ( hsc_det == HSC_DET_ADM1278 ) {
          // old board, BOARD_REV_ID3 is floating.
          board_revision_id &= 0x7;
        } else {
          // new respin board is the MP stage,
          // rise bit3 to keep "if (board_type_index < CPLD_BOARD_PVT_REV)" work.
          board_revision_id |= 0x8;
        }
        break;
      case FEXP_BIC_INTF:
      case REXP_BIC_INTF:
      case RREXP_BIC_INTF1:
      case RREXP_BIC_INTF2:
        //TBD : get the Board Revision of expansion board ?
        break;
    }

    board_type_index = board_revision_id - 1;
    if (board_type_index < 0) {
      board_type_index = 0;
    }
    // PVT & MP firmware could be used in common
    if (board_type_index < CPLD_BOARD_PVT_REV) {
      if (REVISION_ID(signed_bytes[1]) != board_type_index) {
        board_rev_is_invalid = true;
      }
    } else {
      if (REVISION_ID(signed_bytes[1]) < CPLD_BOARD_PVT_REV) {
        board_rev_is_invalid = true;
      }
    }

    if ( board_rev_is_invalid) {
      printf("To prevent this update on slot%d , please use the f/w of %s on the %s system\n",
                    slot_id, board_stage[board_type_index], board_stage[board_type_index]);
      printf("To force the update, please use the --force option.\n");
      goto error_exit;
    }
  }

  ret = BIC_STATUS_SUCCESS;

error_exit:
  if ( ret == BIC_STATUS_FAILURE) {
    printf("This file cannot be updated to this component!\n");
  }

  return ret;
}

static int
is_valid_ast_bic_image(uint8_t slot_id, uint8_t comp, uint8_t intf, char *img_path, int fd, int file_size){
  uint8_t err_proof_board_id = 0;
  uint8_t err_proof_stage = 0;
  uint8_t err_proof_component = 0;
  off_t info_offs;
  FW_IMG_INFO img_info;
  bool is_first = true;

  if (file_size < IMG_SIGNED_INFO_SIZE) {
    return BIC_STATUS_FAILURE;
  }

  info_offs = file_size - IMG_SIGNED_INFO_SIZE;
  if (lseek(fd, info_offs, SEEK_SET) != info_offs) {
    return BIC_STATUS_FAILURE;
  }

  if (read(fd, &img_info, IMG_SIGNED_INFO_SIZE) != IMG_SIGNED_INFO_SIZE) {
    return BIC_STATUS_FAILURE;
  }

  if (fby3_common_check_ast_image_signature(img_info.plat_sig) < 0) {
    return BIC_STATUS_FAILURE;
  }

  if (img_path == NULL) {
    return BIC_STATUS_FAILURE;
  }

  if (fby3_common_check_ast_image_md5(img_path, info_offs, img_info.md5_sum, is_first, comp) < 0) {
    return BIC_STATUS_FAILURE;
  }

  if (fby3_common_check_ast_image_md5(img_path, info_offs, img_info.md5_sum_second, !is_first, comp) < 0) {
    return BIC_STATUS_FAILURE;
  }

  err_proof_board_id = img_info.err_proof[0] & 0x1F;  //bit[4:0]
  err_proof_stage = img_info.err_proof[0] >> 5;       //bit[7:5]
  err_proof_component = img_info.err_proof[1] & 0x07; //bit[10:8]

  if (err_proof_component != COMP_BIC) {
    printf("Not a valid BIC firmware image.\n");
    return BIC_STATUS_FAILURE;
  }

  if (err_proof_board_id != BOARD_ID_SB) {
    printf("Wrong firmware image component.\n");
    return BIC_STATUS_FAILURE;
  }

  if (err_proof_stage != FW_REV_MP) {
    printf("Please use firmware after PVT system\nTo force the update, please use the --force option.\n");
    return BIC_STATUS_FAILURE;
  }

  return BIC_STATUS_SUCCESS;
}

static int
_update_fw(uint8_t slot_id, uint8_t target, uint32_t offset, uint16_t len, uint8_t *buf, uint8_t intf) {
  uint8_t tbuf[256] = {0x00};
  uint8_t rbuf[16] = {0x00};
  uint8_t tlen = 0;
  uint8_t rlen = 0;
  int ret = 0;
  int retries = 5;

  // File the IANA ID
  memcpy(tbuf, (uint8_t *)&IANA_ID, 3);

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

  for (int curr = 0; curr < retries; ++curr) {
    ret = bic_ipmb_send(slot_id, NETFN_OEM_1S_REQ, CMD_OEM_1S_UPDATE_FW, tbuf, tlen, rbuf, &rlen, intf);
    if (ret >= 0) {
      return ret;
    }
    
    sleep(1);
    printf("_update_fw: slot: %u, target %u, offset: %u, len: %u retrying..\n", slot_id, target, offset, len);
  }

  return ret;
}

static int
update_ast_bic(uint8_t slot_id, int fd, int file_size, uint8_t intf) {
#define PKT_SIZE (64*1024)
#define AST_BIC_IPMB_WRITE_COUNT_MAX 224
  struct timeval start, end;
  int update_rc = -1, cmd_rc = 0;
  uint32_t dsize, last_offset;
  uint32_t offset, boundary;
  volatile uint16_t read_count;
  uint8_t buf[256] = {0};
  uint8_t target;
  ssize_t count;

  printf("updating fw on slot %d:\n", slot_id);

  //reinit the fd to the beginning
  if ( lseek(fd, 0, SEEK_SET) != 0 ) {
    syslog(LOG_WARNING, "%s() Cannot reinit the fd to the beginning. errstr=%s", __func__, strerror(errno));
    goto error_exit;
  }

  // Write chunks of binary data in a loop
  dsize = file_size/100;
  last_offset = 0;
  offset = 0;
  boundary = PKT_SIZE;
  target = 2;
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
    cmd_rc = _update_fw(slot_id, target, offset, count, buf, intf);
    if (cmd_rc) {
      goto error_exit;
    }

    // Update counter
    offset += count;
    if (offset >= boundary) {
      boundary += PKT_SIZE;
    }
    if ((last_offset + dsize) <= offset) {
      _set_fw_update_ongoing(slot_id, 60);
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

int
get_sb_bic_solution(uint8_t slot_id, bool *is_ast1030) {
  bic_gpio_t gpio = {0};
  uint8_t hsc_det = 0;
  uint8_t board_rev = 0;

  if (is_ast1030 == NULL) {
    syslog(LOG_ERR, "%s(): Error: null parameter", __func__);
    return -1;
  }

  *is_ast1030 = false; //default

  if (bic_get_gpio(slot_id, &gpio, NONE_INTF) < 0) {
    syslog(LOG_ERR, "%s(): Failed to get slot%u bic gpio", __func__, slot_id);
    return -1;
  }

  hsc_det |= ((GET_GPIO_BIT_VALUE(gpio, HSC_DETECT0)) << 0);
  hsc_det |= ((GET_GPIO_BIT_VALUE(gpio, HSC_DETECT1)) << 1);
  hsc_det |= ((GET_GPIO_BIT_VALUE(gpio, HSC_DETECT2)) << 2);

  // old board BOARD_REV_ID3 is floating
  // check server board revision on new re-spin board

  if (hsc_det != HSC_DET_ADM1278) {
    if (fby3_common_get_sb_board_rev(slot_id, &board_rev) < 0) {
      syslog(LOG_ERR, "%s(): Failed to get slot%u board rev", __func__, slot_id);
      return -1;
    }

    if (board_rev == BOARD_TYPE_AST1030_LTC4282 ||
        board_rev == BOARD_TYPE_AST1030_MPS5990 ) {
      *is_ast1030 = true;
    }
  }

  return 0;
}

static int
update_bic_runtime_fw(uint8_t slot_id, uint8_t comp,uint8_t intf, char *path, uint8_t force) {
  int ret = BIC_STATUS_FAILURE;
  int fd = 0;
  int file_size;
  uint8_t type = NO_EXPECTED_TYPE;
  bool is_sb_ast1030 = false;

  //get fd and file size
  fd = open_and_get_size(path, &file_size);
  if ( fd < 0 ) {
    syslog(LOG_WARNING, "%s() cannot open the file: %s, fd=%d\n", __func__, path, fd);
    goto exit;
  }

  printf("file size = %d bytes, slot = %d, intf = 0x%x\n", file_size, slot_id, intf);

  if (intf == FEXP_BIC_INTF) {
    bic_get_card_type(slot_id, GET_1OU, &type);
    if (type == VERNAL_FALLS_AST1030) {
      force = 1;  //there is no signature in AST BIC IMAGE currently
    }
  }

  if (intf == NONE_INTF) {
    if (get_sb_bic_solution(slot_id, &is_sb_ast1030) < 0) {
      syslog(LOG_WARNING, "%s() Failed to get SB_BIC solution", __func__);
      is_sb_ast1030 = false;
    }
  }

  //check the content of the image
  if (is_sb_ast1030 == true) {
    if ( !force && ( is_valid_ast_bic_image(slot_id, comp, intf, path, fd, file_size) < 0) ) {
      printf("Invalid BIC file!\n");
      goto exit;
    }
  } else {
    if ( !force && ( is_valid_bic_image(slot_id, comp, intf, fd, file_size) < 0) ) {
      printf("Invalid BIC file!\n");
      goto exit;
    }
  }

  //run into the different function based on the interface
  switch (intf) {
    case FEXP_BIC_INTF:
      if (type == VERNAL_FALLS_AST1030) {
        syslog(LOG_WARNING, "%s() go AST 1OU BIC update", __func__);
        ret = update_ast_bic(slot_id, fd, file_size, intf);
      } else {
        syslog(LOG_WARNING, "%s() go TI 1OU BIC update", __func__);
        ret = update_remote_bic(slot_id, intf, fd, file_size);
      }
      break;
    case BB_BIC_INTF:
    case REXP_BIC_INTF:
    case RREXP_BIC_INTF1:
    case RREXP_BIC_INTF2:
      ret = update_remote_bic(slot_id, intf, fd, file_size);
      break;
    case NONE_INTF:
      if (is_sb_ast1030 == true) {
        ret = update_ast_bic(slot_id, fd, file_size, intf);
      } else {
        ret = update_bic(slot_id, fd, file_size);
      }
      break;
  }

exit:

  if ( fd > 0 ) {
    close(fd);
  }

  return ret;
}

#define IPMB_MAX_SEND 224
static int
update_fw_bic_bootloader(uint8_t slot_id, uint8_t comp, uint8_t intf, int fd, int file_size) {
  uint8_t bytes_per_read = IPMB_MAX_SEND;
  uint8_t buf[256] = {0};
  uint16_t buf_size = sizeof(buf);
  ssize_t read_bytes = 0;
  uint32_t offset = 0;
  uint32_t last_offset = 0;
  uint32_t dsize = 0;
  int ret = -1, retry = IPMB_BIC_RETRY;
  uint8_t self_test_result[2] = {0};
  uint8_t bmc_location = 0;

  ret = fby3_common_get_bmc_location(&bmc_location);
  if (ret < 0) {
    syslog(LOG_WARNING, "%s() Cannot get the location of BMC", __func__);
    return -1;
  }

  dsize = file_size / 20;

  if ( lseek(fd, 0, SEEK_SET) != 0 ) {
    syslog(LOG_WARNING, "%s() Cannot reinit the fd to the beginning. errstr=%s", __func__, strerror(errno));
    return -1;
  }

  if (intf == RREXP_BIC_INTF1 || intf == RREXP_BIC_INTF2) {
    bytes_per_read -= IPMB_BRIDGE_OVERHEAD;
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
    ret = send_image_data_via_bic(slot_id, comp, intf, offset, read_bytes, 0x0, buf);
    if (ret != BIC_STATUS_SUCCESS)
      break;

    offset += read_bytes;
    if ((last_offset + dsize) <= offset) {
      printf("updated bic bootloader: %d %%\n", (offset/dsize)*5);
      fflush(stdout);
      last_offset += dsize;
    }
  }

  // Wait for warm reset finished
  sleep(3);
  while (retry > 0){
    sleep(1);
    ret = bic_get_self_test_result(slot_id, (uint8_t *)&self_test_result, NONE_INTF);
    if (ret == 0) {
      break;
    } else {
      retry--;
    }
  }

  return ret;
}

static int
update_bic_bootloader_fw(uint8_t slot_id, uint8_t comp, uint8_t intf, char *path, uint8_t force) {
  int fd = 0;
  int ret = BIC_STATUS_FAILURE;
  int file_size = 0;

  fd = open_and_get_size(path, &file_size);
  if (fd < 0) {
    syslog(LOG_WARNING, "%s() cannot open the file: %s, fd=%d", __func__, path, fd);
    goto exit;
  }

  printf("file size = %d bytes, slot = %d, comp = 0x%x\n", file_size, slot_id, comp);

  //check the content of the image
  if ( !force && (is_valid_bic_image(slot_id, comp, intf, fd, file_size) < 0) ) {
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

static int
ctrl_bic_sensor_monitor(uint8_t slot_id, uint8_t intf, bool stop_bic_montr_en) {
  int ret = 0;

  printf("* Turning %s BIC sensor monitor...\n", (stop_bic_montr_en == true)?"off":"on");

  ret = bic_enable_ssd_sensor_monitor(slot_id, !stop_bic_montr_en, intf);
  if ( ret < 0 ) {
    printf("* Failed to %s bic sensor monitor aborted!\n", (stop_bic_montr_en == true)?"stop":"start");
  } else sleep(2);

  return ret;
}

static int
ctrl_pesw_error_monitor(uint8_t slot_id, uint8_t intf, bool stop_bic_montr_en) {
  uint8_t tbuf[4] = {0x9c, 0x9c, 0x00, (stop_bic_montr_en == true)?0x00:0x01};
  uint8_t rbuf[16] = {0};
  uint8_t rlen = 0;
  int ret = BIC_STATUS_SUCCESS;

  printf("* Turning %s PESW error monitor...\n", (stop_bic_montr_en == true)?"off":"on");
  ret = bic_ipmb_send(slot_id, NETFN_OEM_1S_REQ, BIC_CMD_OEM_PESW_ERR_MONITOR, tbuf, 4, rbuf, &rlen, intf);
  if ( ret < 0 ) {
    printf("Failed to %s PESW error monitor, is the BIC firmware too old(<= D02)? aborted!\n", (stop_bic_montr_en == true)?"stop":"start");
  } else sleep(2);

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
    case FW_2OU_PESW:
      return "2OU PCIe Switch";
    case FW_2OU_PESW_VR:
      return "2OU PCIe VR";
    case FW_2OU_3V3_VR1:
      return "2OU VR_P3V3_STBY1";
    case FW_2OU_3V3_VR2:
      return "2OU VR_P3V3_STBY2";
    case FW_2OU_3V3_VR3:
      return "2OU VR_P3V3_STBY3";
    case FW_2OU_1V8_VR:
      return "2OU VR_P1V8";
    case FW_2OU_M2_DEV0:
      return "2OU M2 Dev0";
    case FW_2OU_M2_DEV1:
      return "2OU M2 Dev1";
    case FW_2OU_M2_DEV2:
      return "2OU M2 Dev2";
    case FW_2OU_M2_DEV3:
      return "2OU M2 Dev3";
    case FW_2OU_M2_DEV4:
      return "2OU M2 Dev4";
    case FW_2OU_M2_DEV5:
      return "2OU M2 Dev5";
    case FW_2OU_M2_DEV6:
      return "2OU M2 Dev6";
    case FW_2OU_M2_DEV7:
      return "2OU M2 Dev7";
    case FW_2OU_M2_DEV8:
      return "2OU M2 Dev8";
    case FW_2OU_M2_DEV9:
      return "2OU M2 Dev9";
    case FW_2OU_M2_DEV10:
      return "2OU M2 Dev10";
    case FW_2OU_M2_DEV11:
      return "2OU M2 Dev11";
    case FW_CWC_BIC:
      return "CWC BIC";
    case FW_CWC_BIC_BL:
      return "CWC BIC Bootloader";
    case FW_CWC_CPLD:
      return "CWC CPLD";
    case FW_CWC_PESW:
      return "CWC PCIe Switch";
    case FW_GPV3_TOP_BIC:
      return "2U Top BIC";
    case FW_GPV3_TOP_BIC_BL:
      return "2U Top BIC Bootloader";
    case FW_GPV3_TOP_CPLD:
      return "2U Top CPLD";
    case FW_GPV3_TOP_PESW:
      return "2U Top PCIe Switch";
    case FW_GPV3_BOT_BIC:
      return "2U Bottom BIC";
    case FW_GPV3_BOT_BIC_BL:
      return "2U Bottom BIC Bootloader";
    case FW_GPV3_BOT_CPLD:
      return "2U Bottom CPLD";
    case FW_GPV3_BOT_PESW:
      return "2U Bottom PCIe Switch";
    case FW_CWC_PESW_VR:
      return "CWC PCIe VR";
    case FW_GPV3_TOP_PESW_VR:
      return "2U Top PCIe VR";
    case FW_GPV3_BOT_PESW_VR:
      return "2U Bottom PCIe VR";
    case FW_2U_TOP_3V3_VR1:
      return "2U Top VR_P3V3_STBY1";
    case FW_2U_TOP_3V3_VR2:
      return "2U Top VR_P3V3_STBY2";
    case FW_2U_TOP_3V3_VR3:
      return "2U Top VR_P3V3_STBY3";
    case FW_2U_TOP_1V8_VR:
      return "2U Top VR_P1V8";
    case FW_2U_BOT_3V3_VR1:
      return "2U Bottom VR_P3V3_STBY1";
    case FW_2U_BOT_3V3_VR2:
      return "2U Bottom VR_P3V3_STBY2";
    case FW_2U_BOT_3V3_VR3:
      return "2U Bottom VR_P3V3_STBY3";
    case FW_2U_BOT_1V8_VR:
      return "2U Bottom VR_P1V8";
    case FW_TOP_M2_DEV0:
      return "2U Top M2 Dev0";
    case FW_TOP_M2_DEV1:
      return "2U Top M2 Dev1";
    case FW_TOP_M2_DEV2:
      return "2U Top M2 Dev2";
    case FW_TOP_M2_DEV3:
      return "2U Top M2 Dev3";
    case FW_TOP_M2_DEV4:
      return "2U Top M2 Dev4";
    case FW_TOP_M2_DEV5:
      return "2U Top M2 Dev5";
    case FW_TOP_M2_DEV6:
      return "2U Top M2 Dev6";
    case FW_TOP_M2_DEV7:
      return "2U Top M2 Dev7";
    case FW_TOP_M2_DEV8:
      return "2U Top M2 Dev8";
    case FW_TOP_M2_DEV9:
      return "2U Top M2 Dev9";
    case FW_TOP_M2_DEV10:
      return "2U Top M2 Dev10";
    case FW_TOP_M2_DEV11:
      return "2U Top M2 Dev11";
    case FW_BOT_M2_DEV0:
      return "2U Bottom M2 Dev0";
    case FW_BOT_M2_DEV1:
      return "2U Bottom M2 Dev1";
    case FW_BOT_M2_DEV2:
      return "2U Bottom M2 Dev2";
    case FW_BOT_M2_DEV3:
      return "2U Bottom M2 Dev3";
    case FW_BOT_M2_DEV4:
      return "2U Bottom M2 Dev4";
    case FW_BOT_M2_DEV5:
      return "2U Bottom M2 Dev5";
    case FW_BOT_M2_DEV6:
      return "2U Bottom M2 Dev6";
    case FW_BOT_M2_DEV7:
      return "2U Bottom M2 Dev7";
    case FW_BOT_M2_DEV8:
      return "2U Bottom M2 Dev8";
    case FW_BOT_M2_DEV9:
      return "2U Bottom M2 Dev9";
    case FW_BOT_M2_DEV10:
      return "2U Bottom M2 Dev10";
    case FW_BOT_M2_DEV11:
      return "2U Bottom M2 Dev11";
    default:
      return "Unknown";
  }

  return "NULL";
}

static int
bic_update_fw_path_or_fd(uint8_t slot_id, uint8_t comp, char *path, int fd, uint8_t force) {
  int ret = BIC_STATUS_SUCCESS;
  uint8_t intf = 0x0;
  char ipmb_content[] = "ipmb";
  char* loc = NULL;
  bool stop_bic_monitoring = false;
  bool reset_usbhub = false;
  char fdstr[32] = {0};
  bool fd_opened = false;

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


  fprintf(stderr, "slot_id: %x, comp: %x, intf: %x, img: %s, force: %x\n", slot_id, comp, intf, path, force);
  syslog(LOG_CRIT, "Updating %s on slot%d. File: %s", get_component_name(comp), slot_id, path);

  uint8_t board_type = 0;
  if ( fby3_common_get_2ou_board_type(slot_id, &board_type) < 0 ) {
    syslog(LOG_WARNING, "Failed to get 2ou board type\n");
  } else if ( board_type == GPV3_MCHP_BOARD ||
              board_type == GPV3_BRCM_BOARD ||
              board_type == CWC_MCHP_BOARD ) {
    stop_bic_monitoring = true;
  }

  //get the intf
  switch (comp) {
    case FW_CPLD:
    case FW_ME:
    case FW_BIC:
    case FW_BIC_BOOTLOADER:
    case FW_VR:
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
    case FW_2OU_PESW:
    case FW_2OU_PESW_VR:
    case FW_2OU_3V3_VR1:
    case FW_2OU_3V3_VR2:
    case FW_2OU_3V3_VR3:
    case FW_2OU_1V8_VR:
    case FW_2OU_M2_DEV0:
    case FW_2OU_M2_DEV1:
    case FW_2OU_M2_DEV2:
    case FW_2OU_M2_DEV3:
    case FW_2OU_M2_DEV4:
    case FW_2OU_M2_DEV5:
    case FW_2OU_M2_DEV6:
    case FW_2OU_M2_DEV7:
    case FW_2OU_M2_DEV8:
    case FW_2OU_M2_DEV9:
    case FW_2OU_M2_DEV10:
    case FW_2OU_M2_DEV11:
    case FW_CWC_BIC:
    case FW_CWC_BIC_BL:
    case FW_CWC_CPLD:
    case FW_CWC_PESW:
    case FW_CWC_PESW_VR:
      reset_usbhub = true;
      intf = REXP_BIC_INTF;
      break;
    case FW_BB_BIC:
    case FW_BB_BIC_BOOTLOADER:
    case FW_BB_CPLD:
      intf = BB_BIC_INTF;
      break;
    case FW_GPV3_TOP_BIC:
    case FW_GPV3_TOP_BIC_BL:
    case FW_GPV3_TOP_CPLD:
    case FW_GPV3_TOP_PESW:
    case FW_GPV3_TOP_PESW_VR:
    case FW_2U_TOP_3V3_VR1:
    case FW_2U_TOP_3V3_VR2:
    case FW_2U_TOP_3V3_VR3:
    case FW_2U_TOP_1V8_VR:
    case FW_TOP_M2_DEV0:
    case FW_TOP_M2_DEV1:
    case FW_TOP_M2_DEV2:
    case FW_TOP_M2_DEV3:
    case FW_TOP_M2_DEV4:
    case FW_TOP_M2_DEV5:
    case FW_TOP_M2_DEV6:
    case FW_TOP_M2_DEV7:
    case FW_TOP_M2_DEV8:
    case FW_TOP_M2_DEV9:
    case FW_TOP_M2_DEV10:
    case FW_TOP_M2_DEV11:
      reset_usbhub = true;
      intf = RREXP_BIC_INTF1;
      break;
    case FW_GPV3_BOT_BIC:
    case FW_GPV3_BOT_BIC_BL:
    case FW_GPV3_BOT_CPLD:
    case FW_GPV3_BOT_PESW:
    case FW_GPV3_BOT_PESW_VR:
    case FW_2U_BOT_3V3_VR1:
    case FW_2U_BOT_3V3_VR2:
    case FW_2U_BOT_3V3_VR3:
    case FW_2U_BOT_1V8_VR:
    case FW_BOT_M2_DEV0:
    case FW_BOT_M2_DEV1:
    case FW_BOT_M2_DEV2:
    case FW_BOT_M2_DEV3:
    case FW_BOT_M2_DEV4:
    case FW_BOT_M2_DEV5:
    case FW_BOT_M2_DEV6:
    case FW_BOT_M2_DEV7:
    case FW_BOT_M2_DEV8:
    case FW_BOT_M2_DEV9:
    case FW_BOT_M2_DEV10:
    case FW_BOT_M2_DEV11:
      reset_usbhub = true;
      intf = RREXP_BIC_INTF2;
      break;
  }

  // if intf is BB_BIC_INTF and the critical activity bit is asserted
  if ( (intf == BB_BIC_INTF) && (bic_is_crit_act_ongoing(slot_id) == true) ) {
    printf("A critical activity is ongoing on the sled, exit.\n");
    ret = BIC_STATUS_FAILURE;
    goto error_exit;
  }

  if (intf == BB_BIC_INTF) {
    if (bb_fw_update_prepare(slot_id) < 0) {
      printf("Please check another slot BMC is not updating BB firmware\n");
      ret = BIC_STATUS_FAILURE;
      goto error_exit;
    }
  }

  // reset HUB if needed
  // because many devices are attached to the USB HUB,
  // perform the reset to make sure the HUB can work normally with the power change
  if ( reset_usbhub == true ) {
    uint8_t status = 0;
    ret = bic_get_server_power_status(slot_id, &status);
    if ( ret < 0 ) {
      printf("Failed to get the power status\n");
      goto error_exit;
    } else if ( status == 0/*SERVER_POWER_OFF*/ ) {
      // run reset
      ret = bic_usb_hub_reset(slot_id, board_type, intf);
      if ( ret < 0 ) {
        goto error_exit;
      }
    }
  }

  //run cmd
  switch (comp) {
    case FW_BIC:
    case FW_1OU_BIC:
    case FW_2OU_BIC:
    case FW_BB_BIC:
    case FW_CWC_BIC:
    case FW_GPV3_TOP_BIC:
    case FW_GPV3_BOT_BIC:
      ret = update_bic_runtime_fw(slot_id, UPDATE_BIC, intf, path, force);
      break;
    case FW_BIC_BOOTLOADER:
    case FW_1OU_BIC_BOOTLOADER:
    case FW_2OU_BIC_BOOTLOADER:
    case FW_BB_BIC_BOOTLOADER:
    case FW_CWC_BIC_BL:
    case FW_GPV3_TOP_BIC_BL:
    case FW_GPV3_BOT_BIC_BL:
      ret = update_bic_bootloader_fw(slot_id, UPDATE_BIC_BOOTLOADER, intf , path, force);
      break;
    case FW_1OU_CPLD:
    case FW_2OU_CPLD:
    case FW_CWC_CPLD:
    case FW_GPV3_TOP_CPLD:
    case FW_GPV3_BOT_CPLD:
      if ( stop_bic_monitoring && (ret = ctrl_bic_sensor_monitor(slot_id, intf, stop_bic_monitoring)) < 0 ) {
        break;
      }

      ret = (loc != NULL)?update_bic_cpld_lattice(slot_id, path, intf, force): \
                          update_bic_cpld_lattice_usb(slot_id, path, intf, force);

      if ( (ret == BIC_STATUS_SUCCESS && stop_bic_monitoring) && \
           (ret = ctrl_bic_sensor_monitor(slot_id, intf, !stop_bic_monitoring)) < 0 );

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
        ret = update_bic_usb_bios(slot_id, comp, fd);
      }
      break;
    case FW_VR:
      ret = update_bic_vr(slot_id, comp, path, intf, force, false/*usb update?*/);
      break;
    case FW_2OU_3V3_VR1:
    case FW_2OU_3V3_VR2:
    case FW_2OU_3V3_VR3:
    case FW_2OU_1V8_VR:
    case FW_2U_TOP_3V3_VR1:
    case FW_2U_TOP_3V3_VR2:
    case FW_2U_TOP_3V3_VR3:
    case FW_2U_TOP_1V8_VR:
    case FW_2U_BOT_3V3_VR1:
    case FW_2U_BOT_3V3_VR2:
    case FW_2U_BOT_3V3_VR3:
    case FW_2U_BOT_1V8_VR:
      ret = update_bic_vr(slot_id, comp, path, intf, force, true/*usb update*/);
      break;
    case FW_2OU_PESW_VR:
    case FW_CWC_PESW_VR:
    case FW_GPV3_TOP_PESW_VR:
    case FW_GPV3_BOT_PESW_VR:
      ret = (loc != NULL)?update_bic_vr(slot_id, comp, path, intf, force, false/*usb update*/): \
                          update_bic_vr(slot_id, comp, path, intf, force, true/*usb update*/);
      break;
    case FW_2OU_PESW:
    case FW_CWC_PESW:
    case FW_GPV3_TOP_PESW:
    case FW_GPV3_BOT_PESW:
      // we should stop polling sensors while updating PESW
      if ( (stop_bic_monitoring && (ret = ctrl_bic_sensor_monitor(slot_id, intf, stop_bic_monitoring)) < 0) || \
           (stop_bic_monitoring && (ret = ctrl_pesw_error_monitor(slot_id, intf, stop_bic_monitoring)) < 0) ) {
        break;
      }

      // run the update
      if (board_type == GPV3_BRCM_BOARD) {
        ret = update_bic_brcm(slot_id, comp, fd, intf);
      } else {
        ret = update_bic_mchp(slot_id, comp, path, intf, force, (loc != NULL)?false:true);
      }

      // start polling again
      if ( (ret == BIC_STATUS_SUCCESS && stop_bic_monitoring) && \
           (((ret = ctrl_bic_sensor_monitor(slot_id, intf, !stop_bic_monitoring)) < 0) || \
           ((ret = ctrl_pesw_error_monitor(slot_id, intf, !stop_bic_monitoring)) < 0)) );

      break;
    case FW_2OU_M2_DEV0:
    case FW_2OU_M2_DEV1:
    case FW_2OU_M2_DEV2:
    case FW_2OU_M2_DEV3:
    case FW_2OU_M2_DEV4:
    case FW_2OU_M2_DEV5:
    case FW_2OU_M2_DEV6:
    case FW_2OU_M2_DEV7:
    case FW_2OU_M2_DEV8:
    case FW_2OU_M2_DEV9:
    case FW_2OU_M2_DEV10:
    case FW_2OU_M2_DEV11:
    case FW_TOP_M2_DEV0:
    case FW_TOP_M2_DEV1:
    case FW_TOP_M2_DEV2:
    case FW_TOP_M2_DEV3:
    case FW_TOP_M2_DEV4:
    case FW_TOP_M2_DEV5:
    case FW_TOP_M2_DEV6:
    case FW_TOP_M2_DEV7:
    case FW_TOP_M2_DEV8:
    case FW_TOP_M2_DEV9:
    case FW_TOP_M2_DEV10:
    case FW_TOP_M2_DEV11:
    case FW_BOT_M2_DEV0:
    case FW_BOT_M2_DEV1:
    case FW_BOT_M2_DEV2:
    case FW_BOT_M2_DEV3:
    case FW_BOT_M2_DEV4:
    case FW_BOT_M2_DEV5:
    case FW_BOT_M2_DEV6:
    case FW_BOT_M2_DEV7:
    case FW_BOT_M2_DEV8:
    case FW_BOT_M2_DEV9:
    case FW_BOT_M2_DEV10:
    case FW_BOT_M2_DEV11:
      if ( stop_bic_monitoring && (ret = ctrl_bic_sensor_monitor(slot_id, intf, stop_bic_monitoring)) < 0 ) {
        break;
      }

      uint8_t nvme_ready = 0, status = 0, type = 0, m2_dev = 0;
      if (comp >= FW_TOP_M2_DEV0 && comp <= FW_TOP_M2_DEV11) {
        m2_dev = (comp - FW_TOP_M2_DEV0) + FW_2OU_M2_DEV0;
        ret = bic_get_dev_info(FRU_2U_TOP, (comp - FW_TOP_M2_DEV0) + 1, &nvme_ready ,&status, &type);
      } else if (comp >= FW_BOT_M2_DEV0 && comp <= FW_BOT_M2_DEV11) {
        m2_dev = (comp - FW_BOT_M2_DEV0) + FW_2OU_M2_DEV0;
        ret = bic_get_dev_info(FRU_2U_BOT, (comp - FW_BOT_M2_DEV0) + 1, &nvme_ready ,&status, &type);
      } else {
        m2_dev = comp;
        ret = bic_get_dev_info(slot_id, (comp - FW_2OU_M2_DEV0) + 1, &nvme_ready ,&status, &type);
      }
      
      if (ret) {
        printf("* Failed to read m.2 device's info\n");
        ret = BIC_STATUS_FAILURE;
        break;
      } else {
        ret = update_bic_m2_fw(slot_id, m2_dev, path, intf, force, type);
      }

      if ( (ret == BIC_STATUS_SUCCESS && stop_bic_monitoring) && \
           (ret = ctrl_bic_sensor_monitor(slot_id, intf, !stop_bic_monitoring)) < 0 );

      break;
  } /*end of switch*/

  if (intf == BB_BIC_INTF) {
    if (bb_fw_update_finish(slot_id) < 0) {
      printf("Failed to clear BB update register\n");
    }
  }
error_exit:
  syslog(LOG_CRIT, "Updated %s on slot%d. File: %s. Result: %s", get_component_name(comp), slot_id, path, (ret != 0)?"Fail":"Success");
  if (fd_opened) {
    close(fd);
  }
  return ret;
}

int
bic_update_fw(uint8_t slot_id, uint8_t comp, char *path, uint8_t force) {
  return bic_update_fw_path_or_fd(slot_id, comp, path, -1, force);
}

int
bic_update_fw_fd(uint8_t slot_id, uint8_t comp, int fd, uint8_t force) {
  return bic_update_fw_path_or_fd(slot_id, comp, NULL, fd, force);
}
