/*
 *
 * Copyright 2020-present Facebook. All Rights Reserved.
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
 *
 */

#include <stdio.h>
#include <stdint.h>
#include <syslog.h>
#include <openbmc/libgpio.h>
#include <openbmc/kv.h>
#include "asic.h"
#include "amd.h"
#include "nvidia.h"

static uint8_t get_gpu_id()
{
  static uint8_t vendor_id = GPU_UNKNOWN;
  char vendor[MAX_VALUE_LEN] = {0};

  if (vendor_id != GPU_UNKNOWN)
    goto exit;
  if (kv_get("asic_mfr", vendor, NULL, KV_FPERSIST) < 0) {
    vendor_id = GPU_AMD; // default
    goto exit;
  }

  if (!strcmp(vendor, MFR_AMD))
    return GPU_AMD;
  else if (!strcmp(vendor, MFR_NV))
    return GPU_NV;
exit:
  return vendor_id;
}

bool is_asic_prsnt(uint8_t slot)
{
  bool prsnt = true;
  char dev[64] = {0};
  gpio_desc_t *desc;
  gpio_value_t value;

  for (int i = 0; i < 2 && prsnt; i++) {
    snprintf(dev, sizeof(dev), "PRSNT%d_N_ASIC%d", i, (int)slot);
    desc = gpio_open_by_shadow(dev);
    if (!desc) {
      syslog(LOG_WARNING, "Open GPIO %s failed", dev);
      return false;
    }
    if (gpio_get_value(desc, &value) < 0) {
      syslog(LOG_WARNING, "Get GPIO %s failed", dev);
      value = GPIO_VALUE_INVALID;
    }
    gpio_close(desc);
    prsnt &= (value == GPIO_VALUE_LOW)? true: false;
    if (!prsnt)
      return false;
  }

  return true;
}

int asic_read_gpu_temp(uint8_t slot, float *value)
{
  uint8_t vendor = get_gpu_id();

  if (vendor == GPU_AMD) {
    *value = amd_read_die_temp(slot);
  } else if (vendor == GPU_NV){
    *value = nv_read_gpu_temp(slot);
  } else {
    return -1;
  }

  return *value < 0? -1: 0;
}

int asic_read_board_temp(uint8_t slot, float *value)
{
  uint8_t vendor = get_gpu_id();

  if (vendor == GPU_AMD) {
    *value = amd_read_edge_temp(slot);
  } else if (vendor == GPU_NV){
    *value = nv_read_board_temp(slot);
  } else {
    return -1;
  }

  return *value < 0? -1: 0;
}

int asic_read_mem_temp(uint8_t slot, float *value)
{
  uint8_t vendor = get_gpu_id();

  if (vendor == GPU_AMD) {
    *value = amd_read_hbm_temp(slot);
  } else if (vendor == GPU_NV){
    *value = nv_read_mem_temp(slot);
  } else {
    return -1;
  }

  return *value < 0? -1: 0;
}

int asic_read_pwcs(uint8_t slot, float *value)
{
  uint8_t vendor = get_gpu_id();

  if (vendor == GPU_AMD) {
    *value = amd_read_pwcs(slot);
  } else if (vendor == GPU_NV){
    *value = nv_read_pwcs(slot);
  } else {
    return -1;
  }

  return *value < 0? -1: 0;
}
