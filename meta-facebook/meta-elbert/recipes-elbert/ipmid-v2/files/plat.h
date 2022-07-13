/*
 * Copyright 2022-present Meta PLatform Inc. All Rights Reserved.
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

#pragma once
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/un.h>
#include <unistd.h>

#define MAX_NODES 1
#define MAX_NUM_SLOTS 1
#define LARGEST_DEVICE_NAME 128
#define ELBERT_SMB_P1_BOARD_PATH "/tmp/.smb_p1_board"
#define SMBCPLD_PATH_FMT I2C_SYSFS_DEV_DIR(4 - 0023) "/%s"
#define PIM_PRSNT "pim%d_present"
#define MAX_WEDGE_POWER_CMD_SIZE 36
#define WEDGE_POWER "/usr/local/bin/wedge_power.sh"
#define WEDGE_POWER_ON "on"
#define WEDGE_POWER_OFF "off"
#define WEDGE_POWER_RESET "reset"
#define MAX_WEDGE_POWER_CMD_SIZE 36
#define DELAY_POWER_CYCLE 1
#define MAX_READ_RETRY 5
#define ELBERT_SCM_PWR_ON "0x1"
#define PS_BUF_SIZE 256
#define ELBERT_BIOS_UTIL "bios_util"
#define ELBERT_FPGA_UTIL "fpga_util"
#define ELBERT_BMC_FLASH "flashcp"
#define ELBERT_PSU_UTIL "psu-util"
#define PS_CMD "ps"

// For elbert_eeprom API
#define BMC_TARGET "BMC"
#define SCM_TARGET "SCM"

// For defining lan interface
#define MAX_INTF_SIZE 10
#define ELBERT_INTF "eth0.4088"

#define LARGEST_DEVICE_NAME 128
#define READ_UNIT_SENSOR_TIMEOUT 5
#define ELBERT_SMB_P1_BOARD_PATH "/tmp/.smb_p1_board"

#define PSU_PRSNT "psu%d_present"
#define PSU_INPUT_OK "psu%d_input_ok"
#define PSU_OUTPUT_OK "psu%d_output_ok"
#define SCMCPLD_PATH_FMT I2C_SYSFS_DEV_DIR(12 - 0043) "/%s"
#define CPU_CTRL "cpu_control"

enum {
  FRU_ALL = 0,
  FRU_SCM = 1,
  FRU_SMB = 2,
  FRU_PIM2 = 3,
  FRU_PIM3 = 4,
  FRU_PIM4 = 5,
  FRU_PIM5 = 6,
  FRU_PIM6 = 7,
  FRU_PIM7 = 8,
  FRU_PIM8 = 9,
  FRU_PIM9 = 10,
  FRU_PSU1 = 11,
  FRU_PSU2 = 12,
  FRU_PSU3 = 13,
  FRU_PSU4 = 14,
  FRU_FAN = 15,
  MAX_NUM_FRUS = 15,
  FRU_CHASSIS = 16,
  FRU_BMC = 17,
  FRU_SMB_EXTRA = 18,
};
