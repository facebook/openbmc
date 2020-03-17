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
#include <pthread.h>
#include <openbmc/libgpio.h>
#include <switchtec/switchtec.h>
#include <switchtec/gas.h>
#include "pal.h"

static pthread_mutex_t pax_mutex = PTHREAD_MUTEX_INITIALIZER;

int pal_pax_fw_update(uint8_t paxid, const char *fwimg)
{
  int ret;
  char cmd[128] = {0};

  if (pal_is_server_off())
    return -1;

  snprintf(cmd, sizeof(cmd), SWITCHTEC_UPDATE_CMD, SWITCH_BASE_ADDR + paxid, fwimg);

  pthread_mutex_lock(&pax_mutex);
  ret = system(cmd);
  pthread_mutex_unlock(&pax_mutex);
  if (WIFEXITED(ret) && (WEXITSTATUS(ret) == 0)) {
    snprintf(cmd, sizeof(cmd), "/tmp/cache_store/pax%d_img", paxid);
    unlink(cmd);
    snprintf(cmd, sizeof(cmd), "/tmp/cache_store/pax%d_cfg", paxid);
    unlink(cmd);
    return 0;
  } else {
    return -1;
  }
}

int pal_read_pax_dietemp(uint8_t sensor_num, float *value)
{
  int ret = 0;
  uint8_t addr;
  uint8_t paxid = sensor_num - MB_SWITCH_PAX0_DIE_TEMP;
  uint32_t temp, sub_cmd_id;
  char device_name[LARGEST_DEVICE_NAME] = {0};
  struct switchtec_dev *dev;

  if (pal_is_server_off())
    return ERR_SENSOR_NA;

  addr = SWITCH_BASE_ADDR + paxid;
  snprintf(device_name, LARGEST_DEVICE_NAME, SWITCHTEC_DEV, addr);

  pthread_mutex_lock(&pax_mutex);
  dev = switchtec_open(device_name);
  if (dev == NULL) {
    syslog(LOG_WARNING, "%s: switchtec open %s failed", __func__, device_name);
    pthread_mutex_unlock(&pax_mutex);
    return ERR_SENSOR_NA;
  }

  sub_cmd_id = MRPC_DIETEMP_GET_GEN4;
  ret = switchtec_cmd(dev, MRPC_DIETEMP, &sub_cmd_id,
                      sizeof(sub_cmd_id), &temp, sizeof(temp));

  switchtec_close(dev);
  pthread_mutex_unlock(&pax_mutex);

  if (ret == 0)
    *value = (float) temp / 100.0;

  return ret < 0? ERR_SENSOR_NA: 0;
}

int pal_get_pax_version(uint8_t paxid, char *img_ver, char *cfg_ver)
{
  int ret;
  char device_name[LARGEST_DEVICE_NAME] = {0};
  char img_key[MAX_KEY_LEN], cfg_key[MAX_KEY_LEN];
  struct switchtec_dev *dev;
  size_t map_size;
  unsigned int x;
  gasptr_t map;

  snprintf(img_key, sizeof(img_key), "pax%d_img", paxid);
  snprintf(cfg_key, sizeof(cfg_key), "pax%d_cfg", paxid);
  if (kv_get(img_key, img_ver, NULL, 0) == 0 &&
      kv_get(cfg_key, cfg_ver, NULL, 0) == 0) {
    return 0;
  }

  if (pal_is_server_off())
    return -1;

  snprintf(device_name, LARGEST_DEVICE_NAME, SWITCHTEC_DEV, SWITCH_BASE_ADDR + paxid);

  pthread_mutex_lock(&pax_mutex);
  dev = switchtec_open(device_name);
  if (dev == NULL) {
    syslog(LOG_WARNING, "%s: switchtec open %s failed", __func__, device_name);
    pthread_mutex_unlock(&pax_mutex);
    return -1;
  }

  ret = switchtec_get_fw_version(dev, img_ver, MAX_VALUE_LEN);
  map = switchtec_gas_map(dev, 0, &map_size);
  if (map != SWITCHTEC_MAP_FAILED) {
    x = gas_read32(dev, (void __gas*)map + 0x2104);
    switchtec_gas_unmap(dev, map);
  } else {
    ret = -1;
  }

  switchtec_close(dev);
  pthread_mutex_unlock(&pax_mutex);

  if (ret == 0) {
    kv_set(img_key, img_ver, 0, 0);
    snprintf(cfg_ver, MAX_VALUE_LEN, "%x", x);
    kv_set(cfg_key, cfg_ver, 0, 0);
  }

  return ret < 0? -1: 0;
}
