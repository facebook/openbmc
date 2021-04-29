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
 */

#include <errno.h>
#include <openbmc/pal.h>
#include <openbmc/pal_sensors.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <unistd.h>

// Register definitions.
#define SCM_PREFIX "/sys/bus/i2c/drivers/scmcpld/12-0043/"
#define SCM_LED_FREQ_H "flash_rate_upper"
#define SCM_LED_FREQ_L "flash_rate_lower"
#define COLOR_GREEN "_green"
#define COLOR_RED "_red"
#define COLOR_BLUE "_blue"
#define COLOR_AMBER "_amber"
#define COLOR_BLINK "_flash"
#define BEACON_LED "beacon_led"
#define BEACON_LED_BLINK "beacon_led_flash"
#define SMB_PREFIX "/sys/bus/i2c/drivers/smbcpld/4-0023/"
#define SMB_PSU_STATUS "psu_status"
#define SMB_PSU_OUTPUT_BIT 0
#define SMB_PSU_INPUT_BIT 4
#define SMB_PSU_PRESENT "psu_present"
#define SMB_PWR_GOOD "switchcard_powergood"
#define SMB_LED_GREEN "_led_green"
#define SMB_LED_RED "_led_red"
#define SMB_LED_BLUE "_led_blue"
#define SMB_LED_AMBER "_led_amber"
#define SMB_FAN_PRESENT "_present"
#define FAN_PREFIX "/sys/bus/i2c/drivers/fancpld/6-0060/"
#define FAN_RPM "_input"
#define ELBERT_PIM_PRESENT(p) "pim" #p "_present"

// Firmware upgrade utilities to check for.
#define ELBERT_BIOS_UTIL "bios_util"
#define ELBERT_FPGA_UTIL "fpga_util"
#define ELBERT_BMC_FLASH "flashcp"
#define ELBERT_PSU_UTIL "psu-util"

// How often LED status will be updated (in seconds).
#define UPDATE_FREQ_SEC 12

// LED blink period when upgrading firmware (.5 seconds).
#define UPGRADE_FLASH_FREQ_USEC 500000

// LED Blink period (in ms).
#define BLINK_FREQ_MSEC 500

// Backdoor file to flash Beacon LED.
#define BEACON_FS_STUB "/tmp/.elbert_beacon"

#define ELBERT_BUFFER_SZ 256
#define ELBERT_READ_ERR -1
#define ELBERT_PARSE_ERR -2
#define ELBERT_FS_ERR -3

#define ELBERT_FAN_RPM_MIN 4000

#define ELBERT_MAX_FAN 5
#define ELBERT_MAX_PSU 4
#define ELBERT_MAX_PIM_ID 9

// System status information which informs LED policy.
struct system_status {
  bool all_psu_ok;
  bool psu_ok[ELBERT_MAX_PSU];
  bool prev_all_psu_ok;
  bool all_fan_ok;
  bool fan_ok[ELBERT_MAX_FAN];
  bool prev_all_fan_ok;
  bool smb_ok;
  bool prev_smb_ok;
  bool sys_ok;
  bool beacon_on;
  int smb_pwr_good;
  int smb_snr_health;
};
