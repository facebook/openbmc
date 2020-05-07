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
#include <syslog.h>
#include <openbmc/libgpio.h>
#include <openbmc/obmc-i2c.h>
#include "pal.h"

#define AMD_GPU_ADDR 0x41
#define AMD_CMD_LD_SCRATCH_ADDR 0x21
#define AMD_CMD_RD_SCRATCH_ADDR 0x23

static int amd_cmd_ldrd_scratch_addr(int fd, uint8_t offset, uint8_t words, uint8_t *buf)
{
  int ret;
  uint8_t bytes = words << 2;
  uint8_t tbuf[64] = {0};
  uint8_t rbuf[64] = {0};

  tbuf[0] = AMD_CMD_LD_SCRATCH_ADDR;
  tbuf[1] = 0x4;
  tbuf[2] = 0x8f;
  tbuf[3] = bytes;
  tbuf[4] = 0x0;
  tbuf[5] = offset;

  ret = i2c_rdwr_msg_transfer(fd, AMD_GPU_ADDR << 1, tbuf, 6, rbuf, 0);
  if (ret < 0) {
#ifdef DEBUG
    printf("Read AMD SMU failed, check if server is power-on\n");
#endif
    return -1;
  }

  tbuf[0] = AMD_CMD_RD_SCRATCH_ADDR;
  ret = i2c_rdwr_msg_transfer(fd, AMD_GPU_ADDR << 1, tbuf, 1, rbuf, bytes + 1);
  if (ret < 0 || rbuf[0] != bytes) {
#ifdef DEBUG
    printf("Read AMD SMU failed, check if server is power-on\n");
#endif
    return -1;
  }

#ifdef DEBUG
  printf("%s: read offset %d with %d bytes:\n", __func__, offset*4, bytes);
  for (int i = 1; i <= bytes; i++) {
    printf("\t0x%2X ", rbuf[i]);
  }
  printf("\n");
#endif

  memcpy(buf, rbuf+1, bytes);
  return 0;
}

static uint8_t get_gpu_id()
{
  uint8_t vendor_id = GPU_AMD; // default
  char vendor[MAX_VALUE_LEN] = {0};

  if (kv_get("asic_manf", vendor, NULL, 0) < 0)
    goto exit;

  if (!strcmp(vendor, MANF_AMD))
    return GPU_AMD;
  else if (!strcmp(vendor, MANF_NV))
    return GPU_NV;
exit:
  return vendor_id;
}

bool pal_is_asic_prsnt(uint8_t slot)
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

static int open_by_check(uint8_t slot)
{
  char dev[64] = {0};

  if (!pal_is_asic_prsnt(slot))
    return -1;
  if (pal_is_server_off())
    return -1;

  snprintf(dev, sizeof(dev), "/dev/i2c-2%d", (int)slot);
  return open(dev, O_RDWR);
}

static bool is_amd_gpu_ready(int fd)
{
  uint8_t buf[4] = {0};

  // Check if SMU is ready by reading id
  if (amd_cmd_ldrd_scratch_addr(fd, 0, 1, buf) == 0 && buf[2])
    return true;
  else
    return false;
}

static float amd_read_die_temp(uint8_t slot)
{
  int fd = open_by_check(slot);
  int ret = -1;
  uint8_t buf[4] = {0};

  if (fd < 0)
    return -1;

  if (is_amd_gpu_ready(fd))
    ret = amd_cmd_ldrd_scratch_addr(fd, 9, 1, buf);

  close(fd);
  return ret < 0? -1: (buf[0] << 8) | buf[1];
}

static float amd_read_edge_temp(uint8_t slot)
{
  int fd = open_by_check(slot);
  int ret = -1;
  uint8_t buf[4] = {0};

  if (fd < 0)
    return -1;

  if (is_amd_gpu_ready(fd))
    ret = amd_cmd_ldrd_scratch_addr(fd, 9, 1, buf);

  close(fd);
  return ret < 0? -1: (buf[2] << 8) | buf[3];
}

static float amd_read_hbm_temp(uint8_t slot)
{
  int fd = open_by_check(slot);
  int ret = -1;
  uint8_t buf[4] = {0};

  if (fd < 0)
    return -1;

  if (is_amd_gpu_ready(fd))
    ret = amd_cmd_ldrd_scratch_addr(fd, 10, 1, buf);

  close(fd);
  return ret < 0? -1: (buf[2] << 8) | buf[3];
}

static float amd_read_pwcs(uint8_t slot)
{
  int fd = open_by_check(slot);
  int ret = -1;
  uint8_t buf[4] = {0};

  if (fd < 0)
    return -1;

  if (is_amd_gpu_ready(fd))
    ret = amd_cmd_ldrd_scratch_addr(fd, 11, 1, buf);

  close(fd);
  return ret < 0? -1: (buf[0] << 8) | buf[1];
}

int pal_read_gpu_temp(uint8_t sensor_num, float *value)
{
  uint8_t vendor = get_gpu_id();
  uint8_t slot = sensor_num - MB_GPU0_TEMP;

  if (vendor == GPU_AMD) {
    *value = amd_read_die_temp(slot);
  } else if (vendor == GPU_NV){
    // Not supported yet
    return ERR_SENSOR_NA;
  } else {
    return ERR_SENSOR_NA;
  }

  return *value < 0? ERR_SENSOR_NA: 0;
}

int pal_read_edge_temp(uint8_t sensor_num, float *value)
{
  uint8_t vendor = get_gpu_id();
  uint8_t slot = sensor_num - MB_GPU0_EDGE_TEMP;

  if (vendor == GPU_AMD) {
    *value = amd_read_edge_temp(slot);
  } else if (vendor == GPU_NV){
    // Not supported yet
    return ERR_SENSOR_NA;
  } else {
    return ERR_SENSOR_NA;
  }

  return *value < 0? ERR_SENSOR_NA: 0;
}

int pal_read_hbm_temp(uint8_t sensor_num, float *value)
{
  uint8_t vendor = get_gpu_id();
  uint8_t slot = sensor_num - MB_GPU0_HBM_TEMP;

  if (vendor == GPU_AMD) {
    *value = amd_read_hbm_temp(slot);
  } else if (vendor == GPU_NV){
    // Not supported yet
    return ERR_SENSOR_NA;
  } else {
    return ERR_SENSOR_NA;
  }

  return *value < 0? ERR_SENSOR_NA: 0;
}

int pal_read_gpu_pwcs(uint8_t sensor_num, float *value)
{
  uint8_t vendor = get_gpu_id();
  uint8_t slot = sensor_num - MB_GPU0_PWCS;

  if (vendor == GPU_AMD) {
    *value = amd_read_pwcs(slot);
  } else if (vendor == GPU_NV){
    // Not supported yet
    return ERR_SENSOR_NA;
  } else {
    return ERR_SENSOR_NA;
  }

  return *value < 0? ERR_SENSOR_NA: 0;
}
