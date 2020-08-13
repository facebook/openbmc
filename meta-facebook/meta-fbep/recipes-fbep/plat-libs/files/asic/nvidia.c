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
#include <openbmc/obmc-i2c.h>
#include "asic.h"

#define SMBPBI_MAX_RETRY 3

#define SMBPBI_STATUS_INACTIVE  0x1D
#define SMBPBI_STATUS_READY     0x1E
#define SMBPBI_STATUS_SUCCESS   0x1F

#define NV_GPU_ADDR             0x4E
#define NV_COMMAND_STATUS_REG   0x5C
#define NV_DATA_REG             0x5D

/*
 * Opcodes and arguments
 */
#define SMBPBI_GET_CAPABILITY   0x01
#define SMBPBI_CAP_GPU0_TEMP    (1 << 0)
#define SMBPBI_CAP_BOARD_TEMP   (1 << 4)
#define SMBPBI_CAP_MEM_TEMP     (1 << 5)
#define SMBPBI_CAP_PWCS	        (1 << 16)

#define SMBPBI_GET_TEMPERATURE  0x03
#define SMBPBI_GPU0_TEMP        0x00
#define SMBPBI_BOARD_TEMP       0x04
#define SMBPBI_MEM_TEMP         0x05

#define SMBPBI_GET_POWER        0x04
#define SMBPBI_TOTAL_PWCS       0x00

static int nv_open_slot(uint8_t slot)
{
  return i2c_cdev_slave_open((int)slot + 20, NV_GPU_ADDR, 0);
}

static int nv_msgbox_write_reg(int fd, uint8_t opcode, uint8_t arg1, uint8_t arg2)
{
  int ret;
  uint8_t tbuf[4] = {0};

  tbuf[0] = opcode;
  tbuf[1] = arg1;
  tbuf[2] = arg2;
  tbuf[3] = 0x80;

  ret = i2c_smbus_write_block_data(fd, NV_COMMAND_STATUS_REG, 4, tbuf);

  return ret < 0? -1: 0;
}

static int nv_msgbox_read_reg(int fd, uint8_t *buf)
{
  int ret;
  uint8_t rbuf[4] = {0};

  ret = i2c_smbus_read_block_data(fd, NV_COMMAND_STATUS_REG, rbuf);
  if (ret < 0)
    return -1;

  memcpy(buf, rbuf, 4);
  return 0;
}

static int nv_msgbox_read_data(int fd, uint8_t *buf)
{
  int ret;
  uint8_t rbuf[4] = {0};

  ret = i2c_smbus_read_block_data(fd, NV_DATA_REG, rbuf);
  if (ret < 0)
    return -1;

  memcpy(buf, rbuf, 4);
  return 0;
}

static uint8_t nv_get_status(int fd)
{
  uint8_t buf[4] = {0};

  if (nv_msgbox_read_reg(fd, buf) < 0)
    return SMBPBI_STATUS_INACTIVE;

  return buf[3] & 0x1f; // reg[28:24]
}

static int nv_msgbox_cmd(int fd, uint8_t opcode, uint8_t arg1, uint8_t arg2, uint8_t *data)
{
  int i;

  if (nv_msgbox_write_reg(fd, opcode, arg1, arg2) < 0)
    return -1;

  for (i = 0; i < SMBPBI_MAX_RETRY; i++) {
    if (nv_get_status(fd) == SMBPBI_STATUS_SUCCESS)
      break;
    usleep(100);
  }
  if (i == SMBPBI_MAX_RETRY)
    return -1;

  if (nv_msgbox_read_data(fd, data) < 0)
    return -1;

  return 0;
}

static uint32_t nv_get_cap(int fd, uint8_t page)
{
  uint8_t buf[4] = {0};
  uint32_t cap;

  if (nv_msgbox_cmd(fd, SMBPBI_GET_CAPABILITY, page, 0x0, buf) < 0)
    return 0x0;

  memcpy(&cap, buf, 4);
  return cap;
}

static float nv_read_temp(uint8_t slot, uint8_t sensor)
{
  int fd;
  uint32_t cap_mask;
  uint8_t buf[4] = {0};
  char value[16] = {0};
  float temp;

  switch (sensor) {
    case SMBPBI_GPU0_TEMP:
      cap_mask = SMBPBI_CAP_GPU0_TEMP;
      break;
    case SMBPBI_BOARD_TEMP:
      cap_mask = SMBPBI_CAP_BOARD_TEMP;
      break;
    case SMBPBI_MEM_TEMP:
      cap_mask = SMBPBI_CAP_MEM_TEMP;
      break;
    default:
      return -1;
  };

  fd = nv_open_slot(slot);
  if (fd < 0)
    return -1;

  if (!(nv_get_cap(fd, 0) & cap_mask))
    goto err;

  if (nv_msgbox_cmd(fd, SMBPBI_GET_TEMPERATURE, sensor, 0x0, buf) < 0)
    goto err;

  close(fd);
  snprintf(value, sizeof(value), "%d.%d", buf[1], buf[0]);
  temp = atof(value);
  return buf[3] & 0x80? -temp: temp;

err:
  close(fd);
  return -1;
}

float nv_read_gpu_temp(uint8_t slot)
{
  return nv_read_temp(slot, SMBPBI_GPU0_TEMP);
}

float nv_read_board_temp(uint8_t slot)
{
  return nv_read_temp(slot, SMBPBI_BOARD_TEMP);
}

float nv_read_mem_temp(uint8_t slot)
{
  return nv_read_temp(slot, SMBPBI_MEM_TEMP);
}

float nv_read_pwcs(uint8_t slot)
{
  int fd = nv_open_slot(slot);
  uint8_t buf[4] = {0};
  uint32_t pwcs;

  if (fd < 0)
    return -1;

  if (!(nv_get_cap(fd, 0) & SMBPBI_CAP_PWCS))
    goto err;

  if (nv_msgbox_cmd(fd, SMBPBI_GET_POWER, SMBPBI_TOTAL_PWCS, 0x0, buf) < 0)
    goto err;

  close(fd);
  memcpy(&pwcs, buf, 4);

  return (float)pwcs / 1000; // mW -> W

err:
  close(fd);
  return -1;
}

