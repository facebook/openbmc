/*
 *
 * Copyright 2019-present Facebook. All Rights Reserved.
 *
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
#include <stdbool.h>
#include <stdint.h>
#include <syslog.h>
#include <openbmc/libgpio.h>
#include <openbmc/obmc-i2c.h>
#include <switchtec/switchtec.h>
#include <switchtec/gas.h>
#include "pal.h"

#define MRPC_PART_INFO_GET_ALL_INFO_EX 0x3

#define PAX_I2C_LOCK "/tmp/pax_lock"

static int pax_lock()
{
  int fd;

  fd = open(PAX_I2C_LOCK, O_RDONLY | O_CREAT, 0666);
  if (fd < 0)
    return -1;

  if (flock(fd, LOCK_EX) < 0) {
    syslog(LOG_WARNING, "%s: errno = %d", __func__, errno);
    return -1;
  }

  return fd;
}

static int pax_unlock(int fd)
{
  if (fd < 0)
    return -1;

  if (flock(fd, LOCK_UN) < 0) {
    syslog(LOG_WARNING, "%s: errno = %d", __func__, errno);
  }

  close(fd);
  return 0;
}

int pal_check_pax_fw_type(uint8_t comp, const char *fwimg)
{
  int fd;
  uint8_t board_id = 0xFF;
  struct switchtec_fw_image_info info;

  pal_get_platform_id(&board_id);

  if ((board_id & 0x7) == 0x3) {
    return -1;
  }

  fd = open(fwimg, O_RDONLY);
  if (fd < 0 || switchtec_fw_file_info(fd, &info) < 0)
    return -1;

  close(fd);

  if ((comp == PAX_BL2 && info.type == SWITCHTEC_FW_TYPE_BL2) ||
      (comp == PAX_IMG && info.type == SWITCHTEC_FW_TYPE_IMG) ||
      (comp == PAX_CFG && info.type == SWITCHTEC_FW_TYPE_CFG) ) {
    return 0;
  } else {
    return -1;
  }
}

void pal_clear_pax_cache(uint8_t paxid)
{
  char key[MAX_KEY_LEN];
  snprintf(key, sizeof(key), "pax%d_bl2", paxid);
  kv_del(key, 0);
  snprintf(key, sizeof(key), "pax%d_img", paxid);
  kv_del(key, 0);
  snprintf(key, sizeof(key), "pax%d_cfg", paxid);
  kv_del(key, 0);
}

int pal_pax_fw_update(uint8_t paxid, const char *fwimg)
{
  int fd, ret;
  char cmd[128] = {0};

  if (pal_is_server_off())
    return -1;

  snprintf(cmd, sizeof(cmd), SWITCHTEC_UPDATE_CMD, SWITCH_BASE_ADDR + paxid, fwimg);
  fd = pax_lock();
  if (fd < 0)
    return -1;

  ret = system(cmd);
  pax_unlock(fd);
  if (WIFEXITED(ret) && (WEXITSTATUS(ret) == 0)) {
    return 0;
  } else {
    return -1;
  }
}

int pal_read_pax_dietemp(uint8_t sensor_num, float *value)
{
  int fd, ret = 0;
  uint8_t addr;
  uint8_t paxid = sensor_num - 0;
  uint32_t temp, sub_cmd_id;
  char device_name[LARGEST_DEVICE_NAME] = {0};
  struct switchtec_dev *dev;

  if (pal_is_server_off())
    return ERR_SENSOR_NA;

  addr = SWITCH_BASE_ADDR + paxid;
  snprintf(device_name, LARGEST_DEVICE_NAME, SWITCHTEC_DEV, addr);

  fd = pax_lock();
  if (fd < 0)
    return ERR_SENSOR_NA;

  dev = switchtec_open(device_name);
  if (dev == NULL) {
    syslog(LOG_WARNING, "%s: switchtec open %s failed", __func__, device_name);
    pax_unlock(fd);
    return ERR_SENSOR_NA;
  }

  sub_cmd_id = MRPC_DIETEMP_GET_GEN4;
  ret = switchtec_cmd(dev, MRPC_DIETEMP, &sub_cmd_id,
                      sizeof(sub_cmd_id), &temp, sizeof(temp));
  switchtec_close(dev);
  pax_unlock(fd);

  if (ret == 0)
    *value = (float) temp / 100.0;

  return ret < 0? ERR_SENSOR_NA: 0;
}

struct switchtec_flash_info_gen4_Ex {
  uint32_t firmware_version;
  uint32_t flash_size;
  uint16_t device_id;
  uint8_t ecc_enable;
  uint8_t rsvd1;
  uint8_t running_bl2_flag;
  uint8_t running_cfg_flag;
  uint8_t running_img_flag;
  uint8_t running_key_flag;
  uint8_t redundancy_key_flag;
  uint8_t redundancy_bl2_flag;
  uint8_t redundancy_cfg_flag;
  uint8_t redundancy_img_flag;
  uint32_t rsvd2[11];
  struct switchtec_flash_part_info_gen4 {
    uint32_t image_crc;
    uint32_t image_len;
    uint32_t image_version;
    uint8_t valid;
    uint8_t active;
    uint8_t read_only;
    uint8_t is_using;
    uint32_t part_start;
    uint32_t part_end;
    uint32_t part_offset;
    uint32_t part_size_dw;
  } map0, map1, keyman0, keyman1, bl20, bl21, cfg0, cfg1, img0, img1, nvlog, vendor[8];
};

static int get_pax_ver(uint8_t paxid, uint8_t type, char *ver)
{
  int fd, ret;
  uint8_t subcmd;
  uint16_t* p;
  char device_name[LARGEST_DEVICE_NAME] = {0};
  struct switchtec_dev *dev;
  struct switchtec_flash_info_gen4_Ex all_info;
  struct switchtec_flash_part_info_gen4 *info_0, *info_1;

  if (pal_is_server_off())
    return -1;

  snprintf(device_name, LARGEST_DEVICE_NAME, SWITCHTEC_DEV, SWITCH_BASE_ADDR + paxid);

  fd = pax_lock();
  if (fd < 0)
    return -1;

  dev = switchtec_open(device_name);
  if (dev == NULL) {
    syslog(LOG_WARNING, "%s: switchtec open %s failed", __func__, device_name);
    pax_unlock(fd);
    return -1;
  }

  subcmd = MRPC_PART_INFO_GET_ALL_INFO_EX;
  ret = switchtec_cmd(dev, MRPC_PART_INFO, &subcmd,
                      sizeof(subcmd), &all_info, sizeof(all_info));

  switchtec_close(dev);
  pax_unlock(fd);

  if (ret == 0) {
    switch (type) {
      case SWITCHTEC_FW_TYPE_BL2:
	info_0 = &(all_info.bl20);
	info_1 = &(all_info.bl21);
	break;
      case SWITCHTEC_FW_TYPE_IMG:
	info_0 = &(all_info.img0);
	info_1 = &(all_info.img1);
	break;
      default:
	return -1;
    };
    if (info_0->is_using)
      p = (uint16_t*)&info_0->image_version;
    else
      p = (uint16_t*)&info_1->image_version;

    snprintf(ver, MAX_VALUE_LEN, "%X.%02X B%03X",
	     p[1] >> 8, p[1] & 0xFF, p[0] & 0xFFF);
  } else {
    syslog(LOG_WARNING, "%s: switchtec get %s version failed", __func__, device_name);
  }
  return ret;
}

int pal_get_brcm_pax_ver(uint8_t paxid, char *ver)
{
  int fd, lock, ret;
  uint8_t tbuf[8] = {0x04, 0x00, 0x3C, 0x83};
  uint8_t rbuf[8] = {0};
  char dev[32] = {0};

  lock = pax_lock();
  if (lock < 0)
    return -1;

  snprintf(dev, sizeof(dev), "/dev/i2c-%d", I2C_BUS_24 + paxid);

  fd = open(dev, O_RDWR);
  if (fd < 0) {
    return -1;
  }

  ret = i2c_rdwr_msg_transfer(fd, 0xB2, tbuf, 4, rbuf, 4);

  snprintf(ver, MAX_VALUE_LEN, "0x%02X%02X%02X%02X",
	     rbuf[0], rbuf[1], rbuf[2], rbuf[3]);

  if(fd > 0) {
    close(fd);
  }

  pax_unlock(lock);
  return ret;
}

int pal_get_pax_bl2_ver(uint8_t paxid, char *ver)
{
  uint8_t board_id = 0xFF;

  pal_get_platform_id(&board_id);

  if ((board_id & 0x7) == 0x3) {
    return pal_get_brcm_pax_ver(paxid, ver);
  }
  else {
    return get_pax_ver(paxid, SWITCHTEC_FW_TYPE_BL2, ver);
  }
}

int pal_get_pax_fw_ver(uint8_t paxid, char *ver)
{
  uint8_t board_id = 0xFF;

  pal_get_platform_id(&board_id);

  if ((board_id & 0x7) == 0x3) {
    return pal_get_brcm_pax_ver(paxid, ver);
  }
  else {
    return get_pax_ver(paxid, SWITCHTEC_FW_TYPE_BL2, ver);
  }
}

int pal_get_pax_cfg_ver(uint8_t paxid, char *ver)
{
  int fd, ret = 0;
  char device_name[LARGEST_DEVICE_NAME] = {0};
  struct switchtec_dev *dev;
  size_t map_size;
  unsigned int x;
  uint8_t board_id = 0xFF;

  gasptr_t map;

  if (pal_is_server_off())
    return -1;

  pal_get_platform_id(&board_id);

  if ((board_id & 0x7) == 0x3) {
    return pal_get_brcm_pax_ver(paxid, ver);
  }

  snprintf(device_name, LARGEST_DEVICE_NAME, SWITCHTEC_DEV, SWITCH_BASE_ADDR + paxid);

  fd = pax_lock();
  if (fd < 0)
    return -1;

  dev = switchtec_open(device_name);
  if (dev == NULL) {
    syslog(LOG_WARNING, "%s: switchtec open %s failed", __func__, device_name);
    pax_unlock(fd);
    return -1;
  }

  map = switchtec_gas_map(dev, 0, &map_size);
  if (map != SWITCHTEC_MAP_FAILED) {
    ret = gas_read32(dev, (void __gas*)map + 0x2104, &x);
    switchtec_gas_unmap(dev, map);
  } else {
    ret = -1;
  }

  switchtec_close(dev);
  pax_unlock(fd);

  if (ret == 0)
    snprintf(ver, MAX_VALUE_LEN, "%X", x);

  return ret < 0? -1: 0;
}
