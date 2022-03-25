/*
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
 */

#ifndef __PAL_H__
#define __PAL_H__

#include <openbmc/obmc-pal.h>
#include <openbmc/obmc-i2c.h>
#include <openbmc/ipmi.h>
#include <openbmc/kv.h>
#include "pal_sensors.h"

#ifdef __cplusplus
extern "C" {
#endif

#include <facebook/elbert_eeprom.h>
#include <facebook/wedge_eeprom.h>
#include <openbmc/ipmi.h>
#include <stdbool.h>

#define ELBERT_PLATFORM_NAME "elbert"
#define ELBERT_MAX_NUM_SLOTS 1
#define MAX_NODES 1
#define MAX_PIM 8
#define MAX_FAN 5

#define LAST_KEY "last_key"
#define SCMCPLD_PATH_FMT I2C_SYSFS_DEV_DIR(12-0043)"/%s"
#define CPU_CTRL "cpu_control"
#define SMBCPLD_PATH_FMT I2C_SYSFS_DEV_DIR(4-0023)"/%s"
#define PIM_PRSNT "pim%d_present"
#define PIM_FPGA_REV_MAJOR "pim%d_fpga_rev_major"
#define PSU_PRSNT "psu%d_present"
#define PSU_INPUT_OK "psu%d_input_ok"
#define PSU_OUTPUT_OK "psu%d_output_ok"

// For checking SCM power status
#define ELBERT_SCM_PWR_ON "0x1"
#define MAX_READ_RETRY 5

// For managing server power
#define WEDGE_POWER "/usr/local/bin/wedge_power.sh"
#define WEDGE_POWER_ON "on"
#define WEDGE_POWER_OFF "off"
#define WEDGE_POWER_RESET "reset"
#define MAX_WEDGE_POWER_CMD_SIZE 36
#define DELAY_POWER_CYCLE 1

// For checking firmware upgrades
#define PS_CMD "ps"
#define PS_BUF_SIZE 256
#define ELBERT_BIOS_UTIL "bios_util"
#define ELBERT_FPGA_UTIL "fpga_util"
#define ELBERT_BMC_FLASH "flashcp"
#define ELBERT_PSU_UTIL "psu-util"

// For elbert_eeprom API
#define BMC_TARGET "BMC"
#define SCM_TARGET "SCM"

// For defining lan interface
#define MAX_INTF_SIZE 10
#define ELBERT_INTF "eth0.4088"

#define LARGEST_DEVICE_NAME 128
#define READ_UNIT_SENSOR_TIMEOUT 5
#define ELBERT_SMB_P1_BOARD_PATH "/tmp/.smb_p1_board"

// PIM i2c bus definitions.
extern const uint8_t pim_bus[];
extern const uint8_t pim_bus_p1[];

extern const char pal_fru_list[];

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

enum {
  PIM_TYPE_UNPLUG = 0,
  PIM_TYPE_16Q = 1,
  PIM_TYPE_8DDM = 2,
  PIM_TYPE_16Q2 = 3,
  PIM_TYPE_NONE = 4,
};

enum
{
  PSU_ACOK_DOWN = 0,
  PSU_ACOK_UP = 1
};

int pal_is_psu_power_ok(uint8_t fru, uint8_t *status);
int pal_get_pim_type(uint8_t fru, int retry);
int pal_set_pim_type_to_file(uint8_t fru, char *type);
int pal_get_pim_type_from_file(uint8_t fru);
uint8_t get_pim_i2cbus(uint8_t fru);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // __PAL_H__
