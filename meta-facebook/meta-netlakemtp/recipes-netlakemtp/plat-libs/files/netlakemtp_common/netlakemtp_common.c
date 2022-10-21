/*
 *
 * Copyright 2020-present Facebook. All Rights Reserved.
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

/*
 * TODO: Add common function as following
 * get server standby power status
 * calculate crc8
 * hex to int
 * string to byte
 * char to C-style string
 * get system stage
*/
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <unistd.h>
#include <math.h>
#include <fcntl.h>
#include <errno.h>
#include <syslog.h>
#include <pthread.h>
#include <string.h>
#include <openbmc/obmc-i2c.h>
#include "netlakemtp_common.h"
#include <openbmc/ipmb.h>

const char platform_signature[PLAT_SIG_SIZE] = "Netlake";

int
netlakemtp_common_check_image_md5(const char* image_path, int cal_size, uint8_t *data) {
  int fd = 0, sum = 0, byte_read_length = 0 , ret = 0, read_bytes = 0;
  char read_buf[MD5_READ_BYTES] = {0};
  char md5_digest[MD5_DIGEST_LENGTH] = {0};
  MD5_CTX context;

  if (image_path == NULL) {
    syslog(LOG_WARNING, "%s(): failed to calculate MD5 due to NULL parameters.", __func__);
    return -1;
  }

  if (cal_size <= 0) {
    syslog(LOG_WARNING, "%s(): failed to calculate MD5 due to wrong calculate size: %d.", __func__, cal_size);
    return -1;
  }

  fd = open(image_path, O_RDONLY);

  if (fd < 0) {
    syslog(LOG_WARNING, "%s(): failed to open %s to calculate MD5.", __func__, image_path);
    return -1;
  }

  lseek(fd, 0, SEEK_SET);

  ret = MD5_Init(&context);
  if (ret == 0) {
    syslog(LOG_WARNING, "%s(): failed to initialize MD5 context.", __func__);
    ret = -1;
    goto exit;
  }

  while (sum < cal_size) {
    read_bytes = MD5_READ_BYTES;
    if ((sum + MD5_READ_BYTES) > cal_size) {
      read_bytes = cal_size - sum;
    }

    byte_read_length = read(fd, read_buf, read_bytes);
    ret = MD5_Update(&context, read_buf, byte_read_length);
    if (ret == 0) {
      syslog(LOG_WARNING, "%s(): failed to update context to calculate MD5 of %s.", __func__, image_path);
      ret = -1;
      goto exit;
    }
    sum += byte_read_length;
  }

  ret = MD5_Final((uint8_t*)md5_digest, &context);
  if (ret == 0) {
    syslog(LOG_WARNING, "%s(): failed to calculate MD5 of %s.", __func__, image_path);
    ret = -1;
    goto exit;
  }

#ifdef DEBUG
  int i = 0;
  printf("calculated MD5:\n")
  for(i = 0; i < 16; i++) {
    printf("%02X ", ((uint8_t*)md5_digest)[i]);
  }
  printf("\nImage MD5");
  for(i = 0; i < 16; i++) {
    printf("%02X ", data[i]);
  }
  printf("\n");
#endif

  if (strncmp(md5_digest, (char*)data, sizeof(md5_digest)) != 0) {
    printf("Checksum incorrect. This image is corrupted or unsigned\n");
    ret = -1;
  }

exit:
  close(fd);
  return ret;
}

int
netlakemtp_common_check_image_signature(uint8_t* data) {
  int ret = 0;

  if (strncmp(platform_signature, (char*)data, PLAT_SIG_SIZE) != 0) {
    printf("This image is not for Netlake\n");
    ret = -1;
  }
  return ret;
}

bool
netlakemtp_common_is_valid_img(const char* img_path, FW_IMG_INFO* img_info, uint8_t rev_id) {
  const char* board_type[] = {"POC1", "POC2", "EVT", "DVT", "PVT", "MP"};
  uint8_t signed_byte = 0x0;
  uint8_t bmc_location = 0;
  uint8_t stage_idx = 0;
  struct stat file_info;

  if (stat(img_path, &file_info) < 0) {
    syslog(LOG_WARNING, "%s(): failed to open %s to check file infomation.", __func__, img_path);
    return false;
  }

  if (netlakemtp_common_check_image_signature(img_info->plat_sig) < 0) {
    syslog(LOG_WARNING, "%s(): failed to Check image signature: %d", __func__, platform_signature[PLAT_SIG_SIZE]);
    return false;
  }

  if (netlakemtp_common_check_image_md5(img_path, file_info.st_size - IMG_POSTFIX_SIZE, img_info->md5_sum) < 0) {
    syslog(LOG_WARNING, "%s(): failed to Check image md5", __func__);
    return false;
  }

  signed_byte = img_info->err_proof;

  switch (rev_id) {
    case FW_REV_POC:
      if (REVISION_ID(signed_byte) != FW_REV_POC) {
        printf("Please use POC firmware on POC system\nTo force the update, please use the --force option.\n");
        return false;
      }
      break;
    case FW_REV_PVT:
    case FW_REV_MP:
      // PVT & MP firmware could be used in common
      if (REVISION_ID(signed_byte) < FW_REV_PVT) {
        printf("Please use firmware after PVT on %s system\nTo force the update, please use the --force option.\n",
              board_type[rev_id]);
        return false;
      }
      break;
    default:
      if (REVISION_ID(signed_byte) != rev_id) {
        printf("Please use %s firmware on %s system\n To force the update, please use the --force option.\n",
              board_type[rev_id], board_type[rev_id]);
        return false;
      }
  }

  return true;
}

int
netlakemtp_common_get_img_ver(const char* image_path, char* ver) {
  int fd = 0;
  int byte_read_length = 0;
  int ret = 0;
  char buf[FW_VER_SIZE] = {0};
  struct stat file_info;
  uint32_t offset = 0x0;

  if(image_path == NULL)
  {
    syslog(LOG_ERR, "%s() Pointer \"image_path\" is NULL.\n", __func__);
    return -1;
  }

  if (stat(image_path, &file_info) < 0) {
    syslog(LOG_WARNING, "%s(): failed to open %s to check file infomation.", __func__, image_path);
    return false;
  }

  offset = file_info.st_size - IMG_POSTFIX_SIZE + IMG_FW_VER_OFFSET;
  fd = open(image_path, O_RDONLY);
  if (fd < 0 ) {
    syslog(LOG_WARNING, "%s(): failed to open %s to check version.", __func__, image_path);
    ret = -1;
    goto exit;
  }

  lseek(fd, offset, SEEK_SET);
  byte_read_length = read(fd, buf, FW_VER_SIZE);
  if (byte_read_length != FW_VER_SIZE) {
    syslog(LOG_WARNING, "%s(): failed to get image version", __func__);
    ret = -1;
    goto exit;
  }

  for (int i = 0; i < FW_VER_SIZE; i++) {
    if (snprintf(ver + (i * sizeof(uint16_t)), sizeof(buf), "%02X", buf[(FW_VER_SIZE - 1) - i]) < 0) {
      syslog(LOG_WARNING, "%s(): failed to show image version", __func__);
      ret = -1;
      goto exit;
    }
  }

exit:
  if (fd >= 0)
    close(fd);

  return ret;
}

int
netlakemtp_common_get_board_rev(uint8_t* rev_id) {
  int ret = 0;
  int i2cfd = 0;
  int retry = 0;
  uint8_t rbuf[CPLD_REV_ID_BYTE] = {0};
  uint8_t rlen = sizeof(rbuf);
  uint8_t tlen = 1;
  uint8_t rev_id_reg = CPLD_REV_ID_REG;

  if(rev_id == NULL)
  {
    syslog(LOG_ERR, "%s() Pointer \"rev_id\" is NULL.\n", __func__);
    return -1;
  }

  i2cfd = i2c_cdev_slave_open(CPLD_BUS, CPLD_ADDR >> 1, I2C_SLAVE_FORCE_CLAIM);
  if ( i2cfd < 0 ) {
    syslog(LOG_ERR, "Failed to open CPLD, fd=%x\n", i2cfd);
    return -1;
  }

  while (retry < CPLD_GET_REV_RETRY_TIME) {
    ret = i2c_rdwr_msg_transfer(i2cfd, CPLD_ADDR,
				&rev_id_reg, tlen, rbuf, rlen);
    if ( ret < 0 ) {
      retry++;
      usleep(100000);
    } else {
      break;
    }
  }

  if (retry == CPLD_GET_REV_RETRY_TIME) {
    syslog(LOG_ERR, "Failed to do i2c_rdwr_msg_transfer, retry = %d\n", retry);
    ret = -1;
  }
  else {
    *rev_id = rbuf[0];
  }

  close(i2cfd);

  return ret;
}

/*
 * PMBus Linear-11 Data Format
 * X = Y*2^N
 * X is the real value;
 * Y is an 11 bit, two's complement integer;
 * N is a 5 bit, two's complement integer.
*/
int
netlakemtp_common_linear11_convert(uint8_t *value_raw, float *value_linear11) {
  if (value_raw == NULL) {
    syslog(LOG_ERR, "%s: invalid parameter: value pointer is NULL", __func__);
    return -1;
  }

  uint16_t data = (value_raw[1] << 8) | value_raw[0];

  int msb_y, msb_n, data_n;
  double data_x = 0, data_y = 0;

  msb_y = (data >> 10) & 0x1;
  msb_n = (data >> 15) & 0x1;

  data_y = (msb_y == 1) ? -1 * ((~data & 0x3ff) + 1)
                  : data & 0x3ff;

  if (msb_n) {
    data_n = (~(data >> 11) & 0xf) + 1;
    *value_linear11 = data_y / pow(2, data_n);
  } else {
    data_n = ((data >> 11) & 0xf);
    *value_linear11 = data_y * pow(2, data_n);
  }
  return 0;
}

/*
 * PMBus Linear-16 Data Format
 * X = Y*2^N
 * X is the real value;
 * Y is an 16 bit, integer;
 * N is a 5 bit, two's complement integer.
*/
int
netlakemtp_common_linear16_convert(uint8_t *value_raw, uint8_t mode, float *value_linear16) {
  if (value_raw == NULL) {
    syslog(LOG_ERR, "%s: invalid parameter: value pointer is NULL", __func__);
    return -1;
  }

  uint8_t exponent = 0;
  uint16_t raw = (value_raw[1] << 8) | value_raw[0];
  //decide formula for calculating two's complement integer from bit 4
  if ((mode >> 4) == 1) {
    exponent = (~mode & 0x1f) + 1;
    *value_linear16 = ((float)raw / pow(2, exponent));
  } else {
    exponent = mode & 0x1f;
    *value_linear16 = ((float)raw * pow(2, exponent));
  }
  return 0;
}

// The following function is known to work with ME of ICE-LAKE D.
// The code can be used as a reference code for future platforms that 
// uses Netlake as COMe
int
netlakemtp_common_me_ipmb_wrapper(uint8_t netfn, uint8_t cmd,
                  uint8_t *txbuf, uint8_t txlen,
                  uint8_t *rxbuf, uint8_t *rxlen) {

  if (rxlen == NULL) {
    syslog(LOG_WARNING, "%s(): failed to get rxlen from NULL pointer.", __func__);
    return -1;
  }

  ipmb_req_t *req;
  ipmb_res_t *res;
  uint8_t rbuf[MAX_IPMB_RES_LEN] = {0};
  uint8_t tbuf[MAX_IPMB_RES_LEN] = {0};
  uint8_t tlen = 0;
  uint8_t rlen = 0;

  req = (ipmb_req_t*)tbuf;
  req->res_slave_addr = 0x16 << 1;
  req->netfn_lun = netfn << LUN_OFFSET;
  req->hdr_cksum = req->res_slave_addr + req->netfn_lun;
  req->hdr_cksum = ZERO_CKSUM_CONST - req->hdr_cksum;
  req->req_slave_addr = BMC_SLAVE_ADDR << 1;
  req->seq_lun = 0x00;
  req->cmd = cmd;

  //copy the data to be send
  if (txlen) {
    if (txbuf == NULL) {
      syslog(LOG_WARNING, "%s(): failed to get txbuf from NULL pointer.", __func__);
      return -1;
    }
    memcpy(req->data, txbuf, txlen);
  }

  tlen = IPMB_HDR_SIZE + IPMI_REQ_HDR_SIZE + txlen;

  // Invoke IPMB library handler
  lib_ipmb_handle(ME_BUS, tbuf, tlen, rbuf, &rlen);

  if (rlen == 0) {
    syslog(LOG_DEBUG, "me_ipmb_wrapper: Zero bytes received\n");
    return -1;
  }

  // Handle IPMB response
  res  = (ipmb_res_t*) rbuf;

  if (res->cc) {
    syslog(LOG_ERR, "me_ipmb_wrapper: Completion Code: 0x%X\n", res->cc);
    return -1;
  }

  // copy the received data back to caller
  *rxlen = rlen - IPMB_HDR_SIZE - IPMI_RESP_HDR_SIZE;
  if ((*rxlen != 0) && (rxbuf == NULL)) {
    syslog(LOG_WARNING, "%s(): failed to get rxbuf from NULL pointer.", __func__);
    return -1;
  }

  memcpy(rxbuf, res->data, *rxlen);

  return 0;
}

int
netlakemtp_common_me_get_fw_ver(ipmi_dev_id_t *dev_id) {
  int ret;
  uint8_t rlen = 0;

  if (dev_id == NULL) {
    syslog(LOG_WARNING, "%s(): failed to put device id to NULL pointer.", __func__);
    return -1;
  }

  ret = netlakemtp_common_me_ipmb_wrapper(NETFN_APP_REQ, CMD_APP_GET_DEVICE_ID, NULL, 0, (uint8_t *)dev_id, &rlen);
  return ret;
}
