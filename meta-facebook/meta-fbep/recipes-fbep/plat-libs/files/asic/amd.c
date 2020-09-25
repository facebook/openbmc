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
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <openbmc/obmc-i2c.h>
#include "asic.h"

#define AMD_GPU_ADDR 0x41
#define AMD_CMD_LD_SCRATCH_ADDR 0x21
#define AMD_CMD_RD_SCRATCH_ADDR 0x23

#define AMD_ID_MI100 0x81

int amd_open_slot(uint8_t slot)
{
  char dev[64] = {0};

  snprintf(dev, sizeof(dev), "/dev/i2c-2%d", (int)slot);
  return open(dev, O_RDWR);
}

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
  if (ret < 0)
    return -1;

  tbuf[0] = AMD_CMD_RD_SCRATCH_ADDR;
  ret = i2c_rdwr_msg_transfer(fd, AMD_GPU_ADDR << 1, tbuf, 1, rbuf, bytes + 1);
  if (ret < 0 || rbuf[0] != bytes)
    return -1;

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

uint8_t amd_get_id(uint8_t slot)
{
  int fd = amd_open_slot(slot);
  uint8_t buf[4] = {0};
  uint8_t id = GPU_AMD;

  if (fd < 0)
    return GPU_UNKNOWN;

  if (amd_cmd_ldrd_scratch_addr(fd, 0, 1, buf) < 0 || buf[2] != AMD_ID_MI100)
    id = GPU_UNKNOWN;

  close(fd);
  return id;
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

int amd_read_die_temp(uint8_t slot, float *value)
{
  int fd = amd_open_slot(slot);
  int ret = -1;
  uint8_t buf[4] = {0};

  if (fd < 0)
    return ASIC_ERROR;

  if (is_amd_gpu_ready(fd))
    ret = amd_cmd_ldrd_scratch_addr(fd, 9, 1, buf);

  close(fd);

  if (ret < 0)
    return ASIC_ERROR;

  *value = (buf[0] << 8) | buf[1];
  return ASIC_SUCCESS;
}

int amd_read_edge_temp(uint8_t slot, float *value)
{
  int fd = amd_open_slot(slot);
  int ret = -1;
  uint8_t buf[4] = {0};

  if (fd < 0)
    return ASIC_ERROR;

  if (is_amd_gpu_ready(fd))
    ret = amd_cmd_ldrd_scratch_addr(fd, 9, 1, buf);

  close(fd);

  if (ret < 0)
    return ASIC_ERROR;

  *value = (buf[2] << 8) | buf[3];
  return ASIC_SUCCESS;
}

int amd_read_hbm_temp(uint8_t slot, float *value)
{
  int fd = amd_open_slot(slot);
  int ret = -1;
  uint8_t buf[4] = {0};

  if (fd < 0)
    return ASIC_ERROR;

  if (is_amd_gpu_ready(fd))
    ret = amd_cmd_ldrd_scratch_addr(fd, 10, 1, buf);

  close(fd);

  if (ret < 0)
    return ASIC_ERROR;

  *value = (buf[2] << 8) | buf[3];
  return ASIC_SUCCESS;
}

int amd_read_pwcs(uint8_t slot, float *value)
{
  int fd = amd_open_slot(slot);
  int ret = -1;
  uint8_t buf[4] = {0};

  if (fd < 0)
    return ASIC_ERROR;

  if (is_amd_gpu_ready(fd))
    ret = amd_cmd_ldrd_scratch_addr(fd, 11, 1, buf);

  close(fd);

  if (ret < 0)
    return ASIC_ERROR;

  *value = (buf[0] << 8) | buf[1];
  return ASIC_SUCCESS;
}
