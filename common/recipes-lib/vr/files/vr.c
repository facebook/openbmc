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
#include <stdlib.h>
#include <fcntl.h>
#include <syslog.h>
#include <string.h>
#include <unistd.h>
#include "vr.h"

struct vr_dev *dev_list = NULL;
void *plat_priv_data = NULL;

int vr_device_register(struct vr_info *info)
{
  struct vr_dev *new_dev = (struct vr_dev *)malloc(sizeof(struct vr_dev));
  static struct vr_dev *curr;

  if (new_dev == NULL) {
    return -1;
  }

  new_dev->info = info;
  new_dev->next = NULL;

  if (dev_list != NULL) {
    curr->next = new_dev;
  } else {
    dev_list = new_dev;
  }

  curr = new_dev;
  return 0;
}

void vr_device_unregister()
{
  struct vr_dev *tmp, *dev;

  dev = dev_list;
  while (dev != NULL) {
    tmp = dev;
    dev = dev->next;
    free(tmp);
  }

  dev_list = NULL;
}

int vr_probe()
{
  return plat_vr_init() < 0? -1: 0;
}

void vr_remove()
{
  plat_vr_exit();
  vr_device_unregister();
}

int vr_fw_version(const char *vr_name, char *ver_str)
{
  struct vr_dev *dev;

  dev = dev_list;
  while (dev != NULL) {
    if (!strcmp(dev->info->dev_name, vr_name)) {
      if (!dev->info->ops->get_fw_ver) {
        break;
      }
      if (dev->info->ops->get_fw_ver(dev->info, ver_str) < 0) {
        syslog(LOG_WARNING, "%s: get VR %s version failed", __func__, vr_name);
        break;
      }

      return VR_STATUS_SUCCESS;
    }
    dev = dev->next;
  }

  if (dev == NULL) {
    syslog(LOG_WARNING, "%s: device %s not found", __func__, vr_name);
  }

  return VR_STATUS_FAILURE;
}

int vr_fw_update(const char *vr_name, const char *path)
{
  struct vr_dev *dev;

  dev = dev_list;
  while (dev != NULL) {
    if (!strcmp(dev->info->dev_name, vr_name)) {
      if (!dev->info->ops ||
          !dev->info->ops->parse_file ||
          !dev->info->ops->fw_update) {
        break;
      }

      if (plat_priv_data == NULL) {
        if (dev->info->ops->validate_file &&
            dev->info->ops->validate_file(dev->info, path) < 0) {
          syslog(LOG_WARNING, "%s: validate file failed", __func__);
          break;
      }

        if ((plat_priv_data = dev->info->ops->parse_file(dev->info, path)) == NULL) {
          syslog(LOG_WARNING, "%s: parse file failed", __func__);
          break;
        }
      }

      if (dev->info->ops->fw_update(dev->info, plat_priv_data) < 0) {
        syslog(LOG_WARNING, "%s: update VR %s failed", __func__, vr_name);
        break;
      }

      if (dev->info->ops->fw_verify &&
          dev->info->ops->fw_verify(dev->info, plat_priv_data) < 0) {
        syslog(LOG_WARNING, "%s: verify VR %s failed", __func__, vr_name);
        break;
      }

      return VR_STATUS_SUCCESS;
    }
    dev = dev->next;
  }

  if (dev == NULL) {
    syslog(LOG_WARNING, "%s: device %s not found", __func__, vr_name);
  }

  return VR_STATUS_FAILURE;
}
