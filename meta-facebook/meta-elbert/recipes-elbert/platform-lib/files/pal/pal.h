#ifndef FILES_PAL_H
#define FILES_PAL_H

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

#include <openbmc/obmc-pal.h>
#include <openbmc/obmc-i2c.h>
#include <openbmc/ipmi.h>

#ifdef __cplusplus
extern "C" {
#endif

#include <facebook/elbert_eeprom.h>
#include <facebook/wedge_eeprom.h>
#include <openbmc/ipmi.h>
#include <openbmc/kv.h>
#include <stdbool.h>

#define ELBERT_PLATFORM_NAME "elbert"
#define ELBERT_MAX_NUM_SLOTS 8

#define MAX_NODES 2
#define MAX_NUM_FRUS 14

#define LAST_KEY "last_key"
#define SCMCPLD_PATH_FMT I2C_SYSFS_DEV_DIR(12-0043)"/%s"
#define CPU_CTRL "cpu_control"
#define CPU_READY "cpu_ready"
#define SMB_POWERGOOD "switchcard_powergood"
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

// For elbert_eeprom API
#define BMC_TARGET "BMC"
#define SCM_TARGET "SCM"

// For defining lan interface
#define MAX_INTF_SIZE 10
#define ELBERT_INTF "eth0.4088"

#define GUID_SIZE 16
#define LARGEST_DEVICE_NAME 128

extern const char pal_fru_list[];

enum {
  FRU_ALL = 0,
  FRU_CHASSIS = 1,
  FRU_BMC = 2,
  FRU_SCM = 3,
  FRU_SMB = 4,
  FRU_SMB_EXTRA = 5,
  FRU_PIM2 = 6,
  FRU_PIM3 = 7,
  FRU_PIM4 = 8,
  FRU_PIM5 = 9,
  FRU_PIM6 = 10,
  FRU_PIM7 = 11,
  FRU_PIM8 = 12,
  FRU_PIM9 = 13,
  FRU_PSU1 = 14,
  FRU_PSU2 = 15,
  FRU_PSU3 = 16,
  FRU_PSU4 = 17,
};

#ifdef __cplusplus
} // extern "C"
#endif

#endif // FILES_PAL_H
