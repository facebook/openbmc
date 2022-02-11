/*
 *
 * Copyright 2015-present Facebook. All Rights Reserved.
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
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <syslog.h>
#include <string.h>
#include <stdbool.h>
#include <errno.h>
#include <syslog.h>

#define SUP_PREFIX "/sys/bus/i2c/drivers/supcpld/12-0043/"
#define SUP_LED_FREQ_H "flash_rate_upper"
#define SUP_LED_FREQ_L "flash_rate_lower"
#define COLOR_GREEN "_green"
#define COLOR_RED   "_red"
#define COLOR_BLINK "_flash"
#define BEACON_LED "beacon_len"
#define SCD_PREFIX "/sys/bus/i2c/drivers/scdcpld/4-0023/"
#define SCD_PSU_STATUS "psu_status"
#define SCD_PSU_PRESENT_BIT 0
#define SCD_PSU_OK_BIT 4
#define SCD_LED_GREEN "_led_green"
#define SCD_LED_RED "_led_red"
#define SCD_FAN_PRESENT "_present"
#define FAN_PREFIX "/sys/bus/i2c/drivers/fancpld/13-0060/"
#define FAN_RPM "_input"
#define YAMP_BIOS_UTIL "bios_util"
#define YAMP_FPGA_UTIL "fpga_util"
#define YAMP_BMC_FLASH "flashcp"
#define YAMP_MAX_LC 8
#define YAMP_LC "lc"
#define YAMP_LC_PRSNT "_prsnt_sta"

// How often LED status will be updated
// in terms of seconds
#define UPDATE_FREQ_SEC 12

// LED Blink period in terms of ms
#define BLINK_FREQ_MSEC 500

// Backdoor file to flash Beacon Led
#define BEACON_FS_STUB "/tmp/yamp_beacon"

#define YAMP_BUFFER_SZ 256
#define YAMP_READ_ERR -1
#define YAMP_PARSE_ERR -2
#define YAMP_FS_ERR -3

#define YAMP_FAN_RPM_MIN 1800
