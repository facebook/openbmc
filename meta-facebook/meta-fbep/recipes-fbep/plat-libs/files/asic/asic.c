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
#include "asic.h"
#include "amd.h"
#include "nvidia.h"

struct asic_ops {
  uint8_t (*get_gpu_id)(uint8_t);
  int (*read_gpu_temp)(uint8_t, float*);
  int (*read_board_temp)(uint8_t, float*);
  int (*read_mem_temp)(uint8_t, float*);
  int (*read_pwcs)(uint8_t, float*);
  int (*set_power_limit)(uint8_t, unsigned int);
  int (*show_ver)(uint8_t, char*);
} ops[MFR_MAX_NUM] = {
  [GPU_AMD] = {
    .get_gpu_id = amd_get_id,
    .read_gpu_temp = amd_read_die_temp,
    .read_board_temp = amd_read_edge_temp,
    .read_mem_temp = amd_read_hbm_temp,
    .read_pwcs = amd_read_pwcs,
    .set_power_limit = NULL,
    .show_ver = NULL
  },
  [GPU_NVIDIA] = {
    .get_gpu_id = nv_get_id,
    .read_gpu_temp = nv_read_gpu_temp,
    .read_board_temp = NULL,
    .read_mem_temp = nv_read_mem_temp,
    .read_pwcs = nv_read_pwcs,
    .set_power_limit = nv_set_power_limit,
    .show_ver = nv_show_vbios_ver
  }
};

uint8_t asic_get_vendor_id(uint8_t slot)
{
  static uint8_t vendor_id[8] = {
    GPU_UNKNOWN, GPU_UNKNOWN, GPU_UNKNOWN, GPU_UNKNOWN,
    GPU_UNKNOWN, GPU_UNKNOWN, GPU_UNKNOWN, GPU_UNKNOWN
  };

  if (vendor_id[slot] != GPU_UNKNOWN)
    goto exit;

  for (uint8_t i = GPU_NVIDIA; i < MFR_MAX_NUM; i++) {
    if (!ops[i].get_gpu_id)
      continue;
    vendor_id[slot] = ops[i].get_gpu_id(slot);
    if (vendor_id[slot] != GPU_UNKNOWN)
      break;
  }
exit:
  return vendor_id[slot];
}

bool is_asic_prsnt(uint8_t slot)
{
  static bool prsnt[8] = {true, true, true, true, true, true, true, true};
  static bool cached[8] = {false, false, false, false, false, false, false, false};
  char dev[64] = {0};
  gpio_desc_t *desc;
  gpio_value_t value;

  if (cached[slot])
    return prsnt[slot];

  for (int i = 0; i < 2 && prsnt[slot]; i++) {
    snprintf(dev, sizeof(dev), "PRSNT%d_N_ASIC%d", i, (int)slot);
    desc = gpio_open_by_shadow(dev);
    if (!desc) {
      syslog(LOG_WARNING, "Open GPIO %s failed", dev);
      return false;
    }
    if (gpio_get_value(desc, &value) < 0) {
      syslog(LOG_WARNING, "Get GPIO %s failed", dev);
      gpio_close(desc);
      return false;
    }
    gpio_close(desc);
    prsnt[slot] &= (value == GPIO_VALUE_LOW)? true: false;
  }

  cached[slot] = true;
  return prsnt[slot];
}

int asic_read_gpu_temp(uint8_t slot, float *value)
{
  uint8_t vendor = asic_get_vendor_id(slot);

  if (vendor == GPU_UNKNOWN)
    return ASIC_NOTSUP;
  if (!ops[vendor].read_gpu_temp)
    return ASIC_NOTSUP;

  return ops[vendor].read_gpu_temp(slot, value);
}

int asic_read_board_temp(uint8_t slot, float *value)
{
  uint8_t vendor = asic_get_vendor_id(slot);

  if (vendor == GPU_UNKNOWN)
    return ASIC_NOTSUP;
  if (!ops[vendor].read_board_temp)
    return ASIC_NOTSUP;

  return ops[vendor].read_board_temp(slot, value);
}

int asic_read_mem_temp(uint8_t slot, float *value)
{
  uint8_t vendor = asic_get_vendor_id(slot);

  if (vendor == GPU_UNKNOWN)
    return ASIC_NOTSUP;
  if (!ops[vendor].read_mem_temp)
    return ASIC_NOTSUP;

  return ops[vendor].read_mem_temp(slot, value);
}

int asic_read_pwcs(uint8_t slot, float *value)
{
  uint8_t vendor = asic_get_vendor_id(slot);

  if (vendor == GPU_UNKNOWN)
    return ASIC_NOTSUP;
  if (!ops[vendor].read_pwcs)
    return ASIC_NOTSUP;

  return ops[vendor].read_pwcs(slot, value);
}

int asic_set_power_limit(uint8_t slot, unsigned int value)
{
  uint8_t vendor = asic_get_vendor_id(slot);

  if (vendor == GPU_UNKNOWN)
    return ASIC_NOTSUP;
  if (!ops[vendor].set_power_limit)
    return ASIC_NOTSUP;

  return ops[vendor].set_power_limit(slot, value);
}

int asic_show_version(uint8_t slot, char *ver)
{
  uint8_t vendor = asic_get_vendor_id(slot);

  if (vendor == GPU_UNKNOWN)
    return ASIC_NOTSUP;
  if (!ops[vendor].show_ver)
    return ASIC_NOTSUP;

  return ops[vendor].show_ver(slot, ver);
}
