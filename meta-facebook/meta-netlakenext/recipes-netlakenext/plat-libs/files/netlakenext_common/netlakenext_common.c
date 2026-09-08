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
#include "netlakenext_common.h"
#include <openbmc/ipmb.h>
#include <openbmc/kv.h>

const char platform_signature[PLAT_SIG_SIZE] = "Netlake";

int
netlakenext_common_check_image_md5(const char* image_path, int cal_size, uint8_t *data) {
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
netlakenext_common_check_image_signature(uint8_t* data) {
  int ret = 0;

  if (strncmp(platform_signature, (char*)data, PLAT_SIG_SIZE) != 0) {
    printf("This image is not for Netlake\n");
    ret = -1;
  }
  return ret;
}

bool
netlakenext_common_is_valid_img(const char* img_path, FW_IMG_INFO* img_info, uint8_t rev_id) {
  const char* board_type[] = {"POC1", "POC2", "EVT", "DVT", "PVT", "MP"};
  uint8_t signed_byte = 0x0;
  struct stat file_info;

  if (stat(img_path, &file_info) < 0) {
    syslog(LOG_WARNING, "%s(): failed to open %s to check file infomation.", __func__, img_path);
    return false;
  }

  if (netlakenext_common_check_image_signature(img_info->plat_sig) < 0) {
    syslog(LOG_WARNING, "%s(): failed to Check image signature: %d", __func__, platform_signature[PLAT_SIG_SIZE-1]);
    return false;
  }

  if (netlakenext_common_check_image_md5(img_path, file_info.st_size - IMG_POSTFIX_SIZE, img_info->md5_sum) < 0) {
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
netlakenext_common_i2c_transfer(uint8_t bus, uint8_t addr, uint8_t *tbuf, uint8_t tlen, uint8_t *rbuf, uint8_t rlen) {
  int ret = 0;
  int i2cfd = 0;
  int retry = 0;

  if (tbuf == NULL && tlen != 0) {
    syslog(LOG_ERR, "%s() Pointer \"tbuf\" is NULL.\n", __func__);
    return -1;
  }
  if (rbuf == NULL && rlen != 0) {
    syslog(LOG_ERR, "%s() Pointer \"rbuf\" is NULL.\n", __func__);
    return -1;
  }

  i2cfd = i2c_cdev_slave_open(bus, addr >> 1, I2C_SLAVE_FORCE_CLAIM);
  if (i2cfd < 0) {
    return -1;
  }

  while (retry < I2C_RETRY_TIME) {
    ret = i2c_rdwr_msg_transfer(i2cfd, addr, tbuf, tlen, rbuf, rlen);
    if (ret < 0) {
      retry++;
      usleep(100000);
    } else {
      ret = 0;
      break;
    }
  }

  if (retry == I2C_RETRY_TIME) {
    ret = -1;
  }

  close(i2cfd);

  return ret;
}

void
netlakenext_vr_dump(void) {
  int ret = 0;
  uint8_t addr_list[] = {VR_PVDDCR_ADDR, VR_PVDD_MISC_ADDR};
  uint8_t page_list[] = {VR_PAGE_0, VR_PAGE_1};
  uint8_t reg_list[] = {VR_STATUS_BYTE_REG, VR_STATUS_WORD_REG, VR_STATUS_IOUT_REG};
  uint8_t tbuf[PMBUS_RW_WORD] = {0};
  uint8_t rbuf[PMBUS_RW_WORD] = {0};
  char val[MAX_VALUE_LEN] = {0};

  ret = kv_get(VR_DUMP_KV_KEY, val, NULL, 0);
  if ((ret == 0) && (strcmp(val, HIGH_STR) == 0)) {
    syslog(LOG_ERR, "%s: VR dump is already in progress, skipping\n", __func__);
    return;
  }

  ret = kv_set(VR_DUMP_KV_KEY, HIGH_STR, 0, 0);
  if (ret < 0) {
    syslog(LOG_ERR, "%s: Failed to set VR dump key to HIGH in kv\n", __func__);
    return;
  }

  /*
   * Allow ongoing PMBus polling transaction to complete
   * Reading a single VR sensor may take around 200 ms, so wait 500 ms here.
   */
  usleep(500000);

  for (size_t addr_idx = 0; addr_idx < ARRAY_SIZE(addr_list); addr_idx++) {
    uint8_t addr = addr_list[addr_idx];
    for (size_t page_idx = 0; page_idx < ARRAY_SIZE(page_list); page_idx++) {
      uint8_t page = page_list[page_idx];
      if (addr == VR_PVDD_MISC_ADDR && page == VR_PAGE_1) {
        continue;
      }
      tbuf[0] = VR_PAGE_REG;
      tbuf[1] = page;
      ret = netlakenext_common_i2c_transfer(VR_BUS, addr, tbuf, PMBUS_RW_WORD, NULL, 0);
      if (ret < 0) {
        syslog(LOG_ERR, "VR: failed to set page bus %d, addr 0x%02x, page %d\n", VR_BUS, addr >> 1, page);
        continue;
      }
      syslog(LOG_CRIT, "VR: set page bus %d, addr 0x%02x, page %d\n", VR_BUS, addr >> 1, page);
      for (size_t reg_idx = 0; reg_idx < ARRAY_SIZE(reg_list); reg_idx++) {
        uint8_t reg = reg_list[reg_idx];
        int rlen = (reg == VR_STATUS_WORD_REG) ? PMBUS_RW_WORD : PMBUS_RW_BYTE;
        tbuf[0] = reg;
        ret = netlakenext_common_i2c_transfer(VR_BUS, addr, tbuf, PMBUS_RW_BYTE, rbuf, rlen);
        if (ret < 0) {
          syslog(LOG_ERR, "VR: failed to read bus %d, addr 0x%02x, offset 0x%02x\n", VR_BUS, addr >> 1, reg);
          continue;
        }
        if (rlen == PMBUS_RW_WORD) {
          syslog(LOG_CRIT, "VR: read bus %d, addr 0x%02x, offset 0x%02x, value 0x%02x%02x\n", VR_BUS, addr >> 1, reg, rbuf[1], rbuf[0]);
        } else {
          syslog(LOG_CRIT, "VR: read bus %d, addr 0x%02x, offset 0x%02x, value 0x%02x\n", VR_BUS, addr >> 1, reg, rbuf[0]);
        }
      }
    }
  }

  ret = kv_set(VR_DUMP_KV_KEY, LOW_STR, 0, 0);
  if (ret < 0) {
    syslog(LOG_ERR, "%s: Failed to set VR dump key to LOW in kv\n", __func__);
  }

  return;
}

int
netlakenext_common_get_img_ver(const char* image_path, char* ver) {
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
netlakenext_get_cpld_data(int bus, uint8_t addr, uint8_t reg, uint8_t* value) {
  int ret = 0;
  uint8_t rbuf[CPLD_REG_BYTE] = { 0 };
  uint8_t rlen = CPLD_REG_BYTE;
  uint8_t tbuf[CPLD_REG_BYTE] = { reg };
  uint8_t tlen = CPLD_REG_BYTE;

  if (value == NULL) {
    syslog(LOG_ERR, "%s() Pointer \"value\" is NULL.\n", __func__);
    return -1;
  }

  ret = netlakenext_common_i2c_transfer(bus, addr, tbuf, tlen, rbuf, rlen);

  if (ret == 0) {
    *value = rbuf[0];
  }

  return ret;
}

int
netlakenext_common_get_sys_cfg(uint8_t* sys_cfg) {
  int ret = 0;
  ret = netlakenext_get_cpld_data(CPLD_BUS_2, CPLD_ADDR_BUS_2, CPLD_SYS_CONFIG_REG_REG, sys_cfg);
  if (ret == 0) {
    return ret;
  }
  syslog(LOG_ERR, "Failed to get system config on bus %d, addr %02x, reg %02x\n",
    CPLD_BUS_2, CPLD_ADDR_BUS_2, CPLD_SYS_CONFIG_REG_REG);
  ret = netlakenext_get_cpld_data(CPLD_BUS_4, CPLD_ADDR_BUS_4, CPLD_SYS_CONFIG_REG_REG, sys_cfg);
  if (ret != 0) {
    syslog(LOG_ERR, "Failed to get system config on backup bus %d, addr %02x, reg %02x\n",
      CPLD_BUS_4, CPLD_ADDR_BUS_4, CPLD_SYS_CONFIG_REG_REG);
  }
  return ret;
}

int
netlakenext_common_get_vr_source(uint8_t bus, uint8_t addr, uint8_t* sku) {
  int ret = 0;
  int retry = 0;
  uint8_t rbuf[VR_MFR_ID_MAX_LEN] = {0};
  uint8_t tbuf[1] = { VR_MFR_ID_REG };

  for (retry = 0; retry < VR_RETRY_TIME; retry++) {
    ret = netlakenext_common_i2c_transfer(bus, addr, tbuf, 1, rbuf, VR_MFR_ID_MAX_LEN);

    if (ret < 0) {
      continue;
    }

    if (rbuf[0] == VR_MFR_ID_MPS_LEN &&
        rbuf[1] == (VR_MFR_ID_MPS & 0xFF) &&
        rbuf[2] == ((VR_MFR_ID_MPS >> 8) & 0xFF) &&
        rbuf[3] == ((VR_MFR_ID_MPS >> 16) & 0xFF)) {
      *sku = MPS;
      break;
    } else if (rbuf[0] == VR_MFR_ID_INF_LEN &&
               rbuf[1] == (VR_MFR_ID_INF & 0xFF) &&
               rbuf[2] == ((VR_MFR_ID_INF >> 8) & 0xFF)) {
      *sku = INFINEON;
      break;
    } else if (rbuf[0] == VR_MFR_ID_RNS_LEN &&
               rbuf[1] == (VR_MFR_ID_RNS & 0xFF) &&
               rbuf[2] == ((VR_MFR_ID_RNS >> 8) & 0xFF) &&
               rbuf[3] == ((VR_MFR_ID_RNS >> 16) & 0xFF) &&
               rbuf[4] == ((VR_MFR_ID_RNS >> 24) & 0xFF)) {
      *sku = RENESAS;
      break;
    }
  }

  if (retry == VR_RETRY_TIME) {
    syslog(LOG_ERR, "%s() Failed to get VR manufacturer ID, bus %d addr %02x\n", __func__, bus, addr);
    return -1;
  }

  return 0;
}

int
netlakenext_common_get_vr_sku(uint8_t* sku) {
  int ret = 0;
  uint8_t sys_cfg = 0;

  ret = netlakenext_common_get_vr_source(VR_BUS, VR_PVDD_MISC_ADDR, sku);
  if (ret < 0) {
    syslog(LOG_ERR, "%s(): failed to get VR source, bus=%d.\n",
      __func__, VR_BUS);
    ret = netlakenext_common_get_sys_cfg(&sys_cfg);
    if (ret < 0) {
      syslog(LOG_ERR, "%s(): failed to get system config.\n", __func__);
    } else {
      switch ((sys_cfg & CPLD_VR_SOURCE_BIT) >> 3) {
        case 0b10:
            *sku = MPS;
            break;
        case 0b11:
            *sku = INFINEON;
            break;
        case 0b01:
            *sku = RENESAS;
            break;
        default:
          syslog(LOG_ERR, "%s(): invalid system config: %d.\n", __func__, sys_cfg);
          return -1;
      }
    }
  }

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
netlakenext_common_linear11_convert(uint8_t *value_raw, float *value_linear11) {
  if (value_raw == NULL) {
    syslog(LOG_ERR, "%s: invalid parameter: value pointer is NULL", __func__);
    return -1;
  }

  uint16_t data = (value_raw[1] << 8) | value_raw[0];

  int msb_y, msb_n, data_n;
  double data_y = 0;

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
netlakenext_common_linear16_convert(uint8_t *value_raw, uint8_t mode, float *value_linear16) {
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