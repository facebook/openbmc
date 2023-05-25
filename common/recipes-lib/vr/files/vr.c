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
#include <fcntl.h>
#include <syslog.h>
#include <string.h>
#include <openbmc/obmc-i2c.h>
#include <openbmc/obmc-pal.h>
#include <openbmc/kv.h>
#include "vr.h"

#define CFG_FREE(p) cfg_free((void**)&(p));

struct vr_info *dev_list = NULL;
int dev_list_count = 0;
void *plat_configs = NULL;

int vr_device_register(struct vr_info *info, int count)
{
  dev_list = info;
  dev_list_count = count;
  return 0;
}

void vr_device_unregister()
{
  dev_list = NULL;
  dev_list_count = 0;
}

int vr_probe()
{
  return (plat_vr_init() < 0) ? -1 : 0;
}

void vr_remove()
{
  plat_vr_exit();
  vr_device_unregister();
}

static void cfg_free(void **ptr) {
  if (ptr && *ptr) {
    free(*ptr);
    *ptr = NULL;
  }
}

int vr_fw_version(int index, const char *vr_name, char *ver_str)
{
  struct vr_info *info = dev_list;
  int i;

  do {
    if (index >= 0) {
      if (index >= dev_list_count) {
        break;
      }

      info += index;
    } else {
      if (!vr_name) {
        break;
      }

      for (i = 0; i < dev_list_count; i++, info++) {
        if (!strcmp(info->dev_name, vr_name))
          break;
      }
      if (i >= dev_list_count) {
        syslog(LOG_WARNING, "%s: device %s not found", __func__, vr_name);
        break;
      }
    }

    if (!info->ops->get_fw_ver) {
      break;
    }

    if (info->ops->get_fw_ver(info, ver_str) < 0) {
      syslog(LOG_WARNING, "%s: get VR %s version failed", __func__, info->dev_name);
      break;
    }

    return VR_STATUS_SUCCESS;
  } while (0);

  return VR_STATUS_FAILURE;
}

int vr_fw_full_version(int index, const char *vr_name, char *ver_str , char *active_ver_key)
{
  struct vr_info *info = dev_list;
  int i;

  do {
    if (index >= 0) {
      if (index >= dev_list_count) {
        break;
      }

      info += index;
    } else {
      if (!vr_name) {
        break;
      }

      for (i = 0; i < dev_list_count; i++, info++) {
        if (!strcmp(info->dev_name, vr_name))
          break;
      }
      if (i >= dev_list_count) {
        syslog(LOG_WARNING, "%s: device %s not found", __func__, vr_name);
        break;
      }
    }

    if (!info->ops->get_fw_ver) {
      break;
    }

    if (info->ops->get_fw_ver(info, ver_str) < 0) {
      syslog(LOG_WARNING, "%s: get VR %s version failed", __func__, info->dev_name);
      break;
    }

    vr_get_fw_avtive_key(info, active_ver_key);

    return VR_STATUS_SUCCESS;
  } while (0);

  return VR_STATUS_FAILURE;
}

void vr_get_fw_avtive_key(struct vr_info *info, char* key)
{
  if (info->private_data) {
    snprintf(key, MAX_KEY_LEN, "%s_vr_%02xh_new_crc", (char *)info->private_data, info->addr);
  } else {
    snprintf(key, MAX_KEY_LEN, "vr_%02xh_new_crc", info->addr);
  }
}

int vr_fw_update(const char *vr_name, const char *path, bool force)
{
  struct vr_info *info = dev_list;
  int ret, i;

  for (i = 0; i < dev_list_count; i++, info++) {
    if (!vr_name || !strcmp(info->dev_name, vr_name)) {  // traverse all for unspecified vr_name
      if (!info->ops ||
          !info->ops->parse_file ||
          !info->ops->fw_update) {
        syslog(LOG_WARNING, "%s: incomplete ops: %s", __func__, info->dev_name);
        break;
      }

      if (plat_configs == NULL) {
        if (info->ops->validate_file &&
            info->ops->validate_file(info, path) < 0) {
          if (!vr_name) {
            continue;
          }
          syslog(LOG_WARNING, "%s: validate file failed", __func__);
          break;
        }

        if ((plat_configs = info->ops->parse_file(info, path)) == NULL) {
          if (!vr_name) {
            continue;
          }
          syslog(LOG_WARNING, "%s: parse file failed", __func__);
          break;
        }
      }

      info->force = force;
      if ((ret = info->ops->fw_update(info, plat_configs)) < 0) {
        if (!vr_name && (ret == VR_STATUS_SKIP)) {
          continue;
        }
        syslog(LOG_WARNING, "%s: update VR %s failed", __func__, info->dev_name);
        break;
      }

      if (info->ops->fw_verify &&
          info->ops->fw_verify(info, plat_configs) < 0) {
        syslog(LOG_WARNING, "%s: verify VR %s failed", __func__, info->dev_name);
        break;
      }

      CFG_FREE(plat_configs);
      return VR_STATUS_SUCCESS;
    }
  }

  if (i >= dev_list_count) {
    syslog(LOG_WARNING, "%s: device %s not found", __func__, vr_name);
  }

  CFG_FREE(plat_configs);
  return VR_STATUS_FAILURE;
}

int vr_unlock_reg(const char *vr_name) {
  struct vr_info *info = dev_list;
  int i = 0;

  for (i = 0; i < dev_list_count; i++, info++) {
    if (!vr_name || !strcmp(info->dev_name, vr_name)) {  // traverse all for unspecified vr_name
      if (!info->ops->unlock_reg) {
        return VR_STATUS_FAILURE;
      }
      return info->ops->unlock_reg(info);
    }
  }
  return VR_STATUS_FAILURE;
}

int vr_lock_reg(const char *vr_name) {
  struct vr_info *info = dev_list;
  int i = 0;

  for (i = 0; i < dev_list_count; i++, info++) {
    if (!vr_name || !strcmp(info->dev_name, vr_name)) {  // traverse all for unspecified vr_name
      if (!info->ops->lock_reg) {
        return VR_STATUS_FAILURE;
      }
      return info->ops->lock_reg(info);
    }
  }
  return VR_STATUS_FAILURE;
}

int i2c_io(int fd, uint8_t addr, uint8_t *tbuf, uint8_t tcnt, uint8_t *rbuf, uint8_t rcnt)
{
  int ret = -1;
  int retry = 3;
  uint8_t buf[64];

  if (tcnt > sizeof(buf)) {
    syslog(LOG_WARNING, "%s: unexpected write length %u", __func__, tcnt);
    return -1;
  }

  while (retry > 0) {
    memcpy(buf, tbuf, tcnt);
    ret = i2c_rdwr_msg_transfer(fd, addr, buf, tcnt, rbuf, rcnt);
    if (ret) {
      syslog(LOG_WARNING, "%s: i2c rw failed for dev 0x%x", __func__, addr);
      retry--;
      msleep(100);
      continue;
    }

    msleep(1);  // delay between transactions
    break;
  }

  return ret;
}

int vr_rdwr(uint8_t bus, uint8_t addr, uint8_t *tbuf, uint8_t tcnt,
            uint8_t *rbuf, uint8_t rcnt) {
  int ret, fd;

  if ((fd = i2c_cdev_slave_open(bus, (addr>>1), I2C_SLAVE_FORCE_CLAIM)) < 0) {
    return -1;
  }

  ret = i2c_io(fd, addr, tbuf, tcnt, rbuf, rcnt);
  close(fd);

  return ret;
}
