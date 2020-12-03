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

#define SMBPBI_MAX_RETRY 5

#define SMBPBI_STATUS_MASK      0x1F

#define SMBPBI_STATUS_ACCEPTED  0x1C
#define SMBPBI_STATUS_INACTIVE  0x1D
#define SMBPBI_STATUS_READY     0x1E
#define SMBPBI_STATUS_SUCCESS   0x1F

#define ASYNC_STATUS_SUCCESS    0x00
#define ASYNC_STATUS_MORE_PROC  0x31
#define ASYNC_STATUS_TIMEOUT    0x33
#define ASYNC_STATUS_NOT_READY  0x34
#define ASYNC_STATUS_IN_RESET   0x36
#define ASYNC_STATUS_BUSY_RETRY 0x41

#define NV_GPU_ADDR             0x4E
#define NV_COMMAND_STATUS_REG   0x5C
#define NV_DATA_REG             0x5D

#define NV_VNDID_LOWER_REG      0x62
#define NV_VNDID_UPPER_REG      0x63
#define NVIDIA_ID               0x10DE

/*
 * Opcodes and arguments
 */
#define SMBPBI_GET_CAPABILITY   0x01
// Page 0
#define SMBPBI_CAP_GPU0_TEMP    (1 << 0)
#define SMBPBI_CAP_BOARD_TEMP   (1 << 4)
#define SMBPBI_CAP_MEM_TEMP     (1 << 5)
#define SMBPBI_CAP_PWCS         (1 << 16)
// Page 1
#define SMBPBI_CAP_FW_VER       (1 << 8)

#define SMBPBI_GET_TEMPERATURE  0x02
#define SMBPBI_GPU0_TEMP        0x00
#define SMBPBI_BOARD_TEMP       0x04
#define SMBPBI_MEM_TEMP         0x05

#define SMBPBI_GET_POWER        0x04
#define SMBPBI_TOTAL_PWCS       0x00

#define SMBPBI_GET_GPU_INFO     0x05
#define SMBPBI_FRIMWARE_VER     0x08

#define SMBPBI_READ_SCRMEM      0x0D
#define SMBPBI_WRITE_SCRMEM     0x0E

#define SMBPBI_ASYNC_REQUEST    0x10
#define SMBPBI_ASYNC_POLLREQ    0xFF

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

static int nv_msgbox_write_data(int fd, uint8_t *buf)
{
  return i2c_smbus_write_block_data(fd, NV_DATA_REG, 4, buf);
}

static uint8_t nv_get_status(int fd)
{
  uint8_t buf[4] = {0};

  usleep(50000);
  if (nv_msgbox_read_reg(fd, buf) < 0)
    return SMBPBI_STATUS_INACTIVE;

  return buf[3] & SMBPBI_STATUS_MASK; // reg[28:24]
}

static int nv_check_status(int fd, uint8_t opcode)
{
  int retry = SMBPBI_MAX_RETRY;
  uint8_t status;

  while (retry--) {
    status = nv_get_status(fd);
    if (status != SMBPBI_STATUS_SUCCESS &&
        (opcode != SMBPBI_ASYNC_REQUEST || status != SMBPBI_STATUS_ACCEPTED)) {
      continue;
    }
    return 0;
  }
  return -1;
}

static int nv_msgbox_cmd(int fd, uint8_t opcode, uint8_t arg1, uint8_t arg2,
                         uint8_t* data_in, uint8_t* data_out)
{
  int retry = SMBPBI_MAX_RETRY;

  while (retry--) {
    usleep(50000);
    if (data_in && nv_msgbox_write_data(fd, data_in) < 0)
      continue;
    if (nv_msgbox_write_reg(fd, opcode, arg1, arg2) < 0)
      continue;
    if (nv_check_status(fd, opcode) < 0)
      continue;
    if (nv_msgbox_read_data(fd, data_out) < 0)
      continue;
    return 0;
  }
  return -1;
}

static uint32_t nv_get_cap(int fd, uint8_t page)
{
  uint8_t buf[4] = {0};
  uint32_t cap;

  if (nv_msgbox_cmd(fd, SMBPBI_GET_CAPABILITY, page, 0x0, NULL, buf) < 0)
    return 0x0;

  memcpy(&cap, buf, 4);
  return cap;
}

uint8_t nv_get_id(uint8_t slot)
{
  int ret;
  int fd = nv_open_slot(slot);
  uint8_t id = GPU_UNKNOWN;

  if (fd < 0)
    return GPU_UNKNOWN;

  ret = (i2c_smbus_read_byte_data(fd, NV_VNDID_UPPER_REG) & 0xFF) << 8 |
        (i2c_smbus_read_byte_data(fd, NV_VNDID_LOWER_REG) & 0xFF);
  if (ret == NVIDIA_ID)
    id = GPU_NVIDIA;

  close(fd);
  return id;
}

static float nv_read_temp(uint8_t slot, uint8_t sensor, float *temp)
{
  int fd, value;
  uint32_t cap_mask;
  uint8_t buf[4] = {0};

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
      return ASIC_ERROR;
  };

  fd = nv_open_slot(slot);
  if (fd < 0)
    return ASIC_ERROR;

  if (!(nv_get_cap(fd, 0) & cap_mask))
    goto err;

  if (nv_msgbox_cmd(fd, SMBPBI_GET_TEMPERATURE, sensor, 0x0, NULL, buf) < 0)
    goto err;

  close(fd);
  memcpy(&value, buf, sizeof(value));
  value = value >> 8;  // Remove fractional bits(7:0 all zero)
  *temp = (float)value;

  return ASIC_SUCCESS;

err:
  close(fd);
  return ASIC_ERROR;
}

int nv_read_gpu_temp(uint8_t slot, float *value)
{
  return nv_read_temp(slot, SMBPBI_GPU0_TEMP, value);
}

int nv_read_board_temp(uint8_t slot, float *value)
{
  return nv_read_temp(slot, SMBPBI_BOARD_TEMP, value);
}

int nv_read_mem_temp(uint8_t slot, float *value)
{
  return nv_read_temp(slot, SMBPBI_MEM_TEMP, value);
}

int nv_read_pwcs(uint8_t slot, float *pwcs)
{
  int fd = nv_open_slot(slot);
  uint8_t buf[4] = {0};
  uint32_t value;

  if (fd < 0)
    return ASIC_ERROR;

  if (!(nv_get_cap(fd, 0) & SMBPBI_CAP_PWCS))
    goto err;

  if (nv_msgbox_cmd(fd, SMBPBI_GET_POWER, SMBPBI_TOTAL_PWCS, 0x0, NULL, buf) < 0)
    goto err;

  close(fd);
  memcpy(&value, buf, 4);

  *pwcs = (float)value / 1000; // mW -> W
  return ASIC_SUCCESS;

err:
  close(fd);
  return ASIC_ERROR;
}

int nv_set_power_limit(uint8_t slot, unsigned int watt)
{
  int fd, retry = SMBPBI_MAX_RETRY;
  unsigned int mwatt = watt * 1000;
  uint8_t tbuf[4], rbuf[4];
  uint8_t async_id;
  uint8_t scratch_offset = 0x0;

  fd = nv_open_slot(slot);
  if (fd < 0)
    return ASIC_ERROR;

  tbuf[0] = (uint8_t)(mwatt & 0xff);
  tbuf[1] = (uint8_t)((mwatt >> 8) & 0xff);
  tbuf[2] = (uint8_t)((mwatt >> 16) & 0xff);
  tbuf[3] = (uint8_t)((mwatt >> 24) & 0xff);
  // Write mWatts to scratch memory at offset 0x0
  if (nv_msgbox_cmd(fd, SMBPBI_WRITE_SCRMEM, scratch_offset, 0x0, tbuf, rbuf) < 0)
    goto err;
  // Request GPU to read scratch memory at offset 0x0
  if (nv_msgbox_cmd(fd, SMBPBI_ASYNC_REQUEST, scratch_offset, 0x0, NULL, rbuf) < 0)
    goto err;

  async_id = rbuf[0];
  // Retry if needed
  while (retry--) {
    if (nv_msgbox_cmd(fd, SMBPBI_ASYNC_REQUEST, SMBPBI_ASYNC_POLLREQ, async_id, NULL, rbuf) < 0)
      goto err;
    if (rbuf[0] == ASYNC_STATUS_MORE_PROC || rbuf[0] == ASYNC_STATUS_TIMEOUT ||
        rbuf[0] == ASYNC_STATUS_NOT_READY || rbuf[0] == ASYNC_STATUS_IN_RESET ||
        rbuf[0] == ASYNC_STATUS_BUSY_RETRY ) {
      continue;
    }
    if (rbuf[0] == ASYNC_STATUS_SUCCESS) {
      close(fd);
      syslog(LOG_CRIT, "Set power limit of GPU on slot %d to %d Watts", (int)slot, watt);
      return ASIC_SUCCESS;
    }
    break;
  }

err:
  close(fd);
  return ASIC_ERROR;
}

int nv_show_vbios_ver(uint8_t slot, char *ver)
{
  int fd, retry = SMBPBI_MAX_RETRY;
  uint8_t buf[16] = {0};
  uint8_t offset;

  fd = nv_open_slot(slot);
  if (fd < 0)
    return ASIC_ERROR;

  // Sometimes this capability would be turned off at run-time
  // Add retry as workaround here
  while (retry--) {
    if (!(nv_get_cap(fd, 1) & SMBPBI_CAP_FW_VER))
      continue;
    else
      break;
  }
  if (retry == 0)
    goto err;

  for (offset = 0; offset < 4; offset++) {
    if (nv_msgbox_cmd(fd, SMBPBI_GET_GPU_INFO, SMBPBI_FRIMWARE_VER, offset, NULL, buf+offset*4) < 0)
      goto err;
  }

  memcpy(ver, buf, 14);
  close(fd);
  return ASIC_SUCCESS;

err:
  close(fd);
  return ASIC_ERROR;
}
