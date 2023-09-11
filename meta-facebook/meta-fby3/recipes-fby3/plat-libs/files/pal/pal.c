/*
 *
 * Copyright 2015-present Facebook. All Rights Reserved.
 *
 * This file contains code to support IPMI2.0 Specification available @
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
#include <stdbool.h>
#include <ctype.h>
#include <errno.h>
#include <syslog.h>
#include <string.h>
#include <unistd.h>
#include <stddef.h>
#include <time.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/types.h>
#include <openbmc/kv.h>
#include <openbmc/libgpio.h>
#include <openbmc/obmc-i2c.h>
#include <openbmc/ncsi.h>
#include <openbmc/nl-wrapper.h>
#include <sys/socket.h>
#include <linux/netlink.h>
#include <sys/un.h>
#include "pal.h"

#define PLATFORM_NAME "yosemitev3"
#define LAST_KEY "last_key"
#define CWC_LAST_KEY "cwc_last_key"

#define OFFSET_SYS_GUID 0x17F0
#define OFFSET_DEV_GUID 0x1800

#define PFR_NICEXP_BUS 9  // NICEXP PFR
#define PFR_BB_BUS 12     // Baseboard PFR
#define PFR_MAILBOX_ADDR (0x70)
#define NUM_SERVER_FRU  4
#define NUM_NIC_FRU     1
#define NUM_BMC_FRU     1

#define ERROR_ID_LOG_LEN 15

#define FAN_FAIL_RECORD_PATH "/tmp/cache_store/fan_fail_boost"

#define MAX_CMD_LEN 64

const char pal_guid_fru_list[] = "slot1, slot2, slot3, slot4, bmc";
const char pal_dev_fru_list[] = "all, 1U, 2U, 1U-dev0, 1U-dev1, 1U-dev2, 1U-dev3, 2U-dev0, 2U-dev1, 2U-dev2, 2U-dev3, 2U-dev4, 2U-dev5, " \
                            "2U-dev6, 2U-dev7, 2U-dev8, 2U-dev9, 2U-dev10, 2U-dev11, 2U-dev12, 2U-dev13";
const char pal_dev_pwr_list[] = "all, 1U-dev0, 1U-dev1, 1U-dev2, 1U-dev3, 2U-dev0, 2U-dev1, 2U-dev2, 2U-dev3, 2U-dev4, 2U-dev5, " \
                            "2U-dev6, 2U-dev7, 2U-dev8, 2U-dev9, 2U-dev10, 2U-dev11, 2U-dev12, 2U-dev13, " \
                            "2U-dev0_1, 2U-dev2_3, 2U-dev4_5, 2U-dev6_7, 2U-dev8_9, 2U-dev10_11";
const char pal_dev_pwr_option_list[] = "status, off, on, cycle";
const char pal_m2_dual_list[] = "";

static char sel_error_record[NUM_SERVER_FRU] = {0};

static bool cycle_thread_is_running[NUM_SERVER_FRU + 1] = {0};

const char pal_fru_list[] = "all, slot1, slot2, slot3, slot4, bmc, nic, slot1-2U-exp, slot1-2U-top, slot1-2U-bot";

#define SYSFW_VER "sysfw_ver_slot"
#define SYSFW_VER_STR SYSFW_VER "%d"
#define BOOR_ORDER_STR "slot%d_boot_order"
#define SEL_ERROR_STR  "slot%d_sel_error"
#define SNR_HEALTH_STR "slot%d_sensor_health"
#define GPIO_OCP_DEBUG_BMC_PRSNT_N "OCP_DEBUG_BMC_PRSNT_N"
#define PCIE_CONFIG "slot%d_fru%d_config"
#define DEBUG_CARD_PRSNT_KEY "DEBUG_CARD_PRSNT"

#define SLOT1_POSTCODE_OFFSET 0x02
#define SLOT2_POSTCODE_OFFSET 0x03
#define SLOT3_POSTCODE_OFFSET 0x04
#define SLOT4_POSTCODE_OFFSET 0x05
#define DEBUG_CARD_UART_MUX 0x06

#define BB_CPLD_IO_BASE_OFFSET 0x16

#define ENABLE_STR "enable"
#define DISABLE_STR "disable"
#define STATUS_STR "status"
#define WAKEUP_STR "wakeup"
#define SLEEP_STR  "sleep"
#define FAN_MODE_FILE "/tmp/cache_store/fan_mode"
#define FAN_MODE_STR_LEN 8 // include the string terminal

#define IPMI_GET_VER_FRU_NUM  5
#define IPMI_GET_VER_MAX_COMP 9
#define MAX_FW_VER_LEN        32  //include the string terminal

#define MAX_COMPONENT_LEN 32 //include the string terminal

#define MAX_PWR_LOCK_STR 32

#ifndef FRU_CAPABILITY_SENSOR_SLAVE
#define FRU_CAPABILITY_SENSOR_SLAVE (1UL << 19)
#endif

#define BIC_READ_EEPROM_FAILED 0xE0

static int key_func_por_cfg(int event, void *arg);
static int key_func_pwr_last_state(int event, void *arg);

enum key_event {
  KEY_BEFORE_SET,
  KEY_AFTER_INI,
};

enum sel_event_data_index {
  DATA_INDEX_0 = 3,
  DATA_INDEX_1 = 4,
  DATA_INDEX_2 = 5,
};

struct pal_key_cfg {
  char *name;
  char *def_val;
  int (*function)(int, void*);
} key_cfg[] = {
  /* name, default value, function */
  {SYSFW_VER "1", "0", NULL},
  {SYSFW_VER "2", "0", NULL},
  {SYSFW_VER "3", "0", NULL},
  {SYSFW_VER "4", "0", NULL},
  {"pwr_server1_last_state", "on", key_func_pwr_last_state},
  {"pwr_server2_last_state", "on", key_func_pwr_last_state},
  {"pwr_server3_last_state", "on", key_func_pwr_last_state},
  {"pwr_server4_last_state", "on", key_func_pwr_last_state},
  {"timestamp_sled", "0", NULL},
  {"slot1_por_cfg", "lps", key_func_por_cfg},
  {"slot2_por_cfg", "lps", key_func_por_cfg},
  {"slot3_por_cfg", "lps", key_func_por_cfg},
  {"slot4_por_cfg", "lps", key_func_por_cfg},
  {"slot1_boot_order", "0100090203ff", NULL},
  {"slot2_boot_order", "0100090203ff", NULL},
  {"slot3_boot_order", "0100090203ff", NULL},
  {"slot4_boot_order", "0100090203ff", NULL},
  {"slot1_cpu_ppin", "0", NULL},
  {"slot2_cpu_ppin", "0", NULL},
  {"slot3_cpu_ppin", "0", NULL},
  {"slot4_cpu_ppin", "0", NULL},
  {"fru1_restart_cause", "3", NULL},
  {"fru2_restart_cause", "3", NULL},
  {"fru3_restart_cause", "3", NULL},
  {"fru4_restart_cause", "3", NULL},
  {"slot1_sensor_health", "1", NULL},
  {"slot2_sensor_health", "1", NULL},
  {"slot3_sensor_health", "1", NULL},
  {"slot4_sensor_health", "1", NULL},
  {"slot1_sel_error", "1", NULL},
  {"slot2_sel_error", "1", NULL},
  {"slot3_sel_error", "1", NULL},
  {"slot4_sel_error", "1", NULL},
  {"ntp_server", "", NULL},
  /* Add more Keys here */
  {LAST_KEY, LAST_KEY, NULL}, /* This is the last key of the yv3 list */
  // cwc fru_health
  {"cwc_fru10_sensor_health", "1", NULL},
  {"cwc_fru11_sensor_health", "1", NULL},
  {"cwc_fru12_sensor_health", "1", NULL},
  {"cwc_fru10_sel_error", "1", NULL},
  {"cwc_fru11_sel_error", "1", NULL},
  {"cwc_fru12_sel_error", "1", NULL},
  {CWC_LAST_KEY, CWC_LAST_KEY, NULL} /* This is the last key of the cwc list */
};

MAPTOSTRING root_port_common_mapping[] = {
    // NIC
    { 0xB2, 0, 0x4A, "Class 2", "NIC"}, // Class 2 NIC
    { 0x63, 3, 0x2D, "Class 1", "NIC"}, // Class 1 NIC

    // DL
    { 0x63, 2, 0x2C, "Num 0", "SB" },   // PVT switch Num 1 to 0
    { 0x00, 0x1D, 0xFF, "Num 1", "SB"}, // PVT -> Remove

    // CWC/GPv3 root port
    { 0x15, 0, 0x1A, "CPU X16", "CWC/GPv3"}, //Port 0x1A
    { 0x63, 0, 0x2A, "CPU X8",  "CWC/GPv3"}, //Port 0x2A

    // CWC/GPv3 Up Strem Ports
    { 0x16, 0, 0x1A, "USP X16", "CWC/GPv3"}, //Port 0x1A
    { 0x64, 0, 0x2A, "USP X8",  "CWC/GPv3"}, //Port 0x2A

    // CWC TOP/BOT USBP0
    { 0x17, 0, 0x1A, "USP0", "TOP"}, //Port 0x1A
    { 0x17, 1, 0x2A, "USP0", "BOT"}, //Port 0x2A

    // CWC TOP/BOT USBP1
    { 0x65, 0, 0x1A, "USP1", "TOP"}, //Port 0x1A
    { 0x65, 1, 0x2A, "USP1", "BOT"}, //Port 0x2A
};

MAPTOSTRING root_port_mapping[] = {
    { 0xB2, 3, 0x3D, "Num 0", "1OU"}, //Port 0x4D
    { 0xB2, 2, 0x3C, "Num 1", "1OU"}, //Port 0x4C
    { 0xB2, 1, 0x3B, "Num 2", "1OU"}, //Port 0x4B
    { 0xB2, 0, 0x3A, "Num 3", "1OU"}, //Port 0x4A
    { 0x15, 0, 0x1A, "Num 0", "2OU"}, //Port 0x1A
    { 0x15, 1, 0x1B, "Num 1", "2OU"}, //Port 0x1B
    { 0x63, 1, 0x2B, "Num 2", "2OU"}, //Port 0x2B
    { 0x63, 0, 0x2A, "Num 3", "2OU"}, //Port 0x2A
    { 0x15, 2, 0x1C, "Num 4", "2OU"}, //Port 0x1C
    { 0x15, 3, 0x1D, "Num 5", "2OU"}, //Port 0x1D
};

// dual M2 for 4U
MAPTOSTRING root_port_mapping_4u_dual_m2_gpv3[] = {
    // bus, device, port, silk screen, location
    // Down Stream Ports
    { 0x20, 0, 0x01, "Num 1",  "BOT"},
    { 0x20, 1, 0x02, "Num 3",  "BOT"},
    { 0x20, 2, 0x03, "Num 5",  "BOT"},
    { 0x20, 3, 0x04, "Num 7",  "BOT"},
    { 0x6C, 0, 0x05, "Num 9",  "BOT"},
    { 0x6C, 1, 0x06, "Num 11", "BOT"},
    { 0x20, 4, 0x07, "E1S 0",  "BOT"},
    { 0x6C, 2, 0x08, "E1S 1",  "BOT"},
    { 0x1F, 0, 0x09, "PESW",   "BOT"},
    { 0x6B, 0, 0x0A, "PESW",   "BOT"},

    { 0x19, 0, 0x01, "Num 1",  "TOP"},
    { 0x19, 1, 0x02, "Num 3",  "TOP"},
    { 0x19, 2, 0x03, "Num 5",  "TOP"},
    { 0x19, 3, 0x04, "Num 7",  "TOP"},
    { 0x67, 0, 0x05, "Num 9",  "TOP"},
    { 0x67, 1, 0x06, "Num 11", "TOP"},
    { 0x19, 4, 0x07, "E1S 0",  "TOP"},
    { 0x67, 2, 0x08, "E1S 1",  "TOP"},
    { 0x18, 0, 0x09, "PESW",   "TOP"},
    { 0x66, 0, 0x0A, "PESW",   "TOP"},

    // End devices
    { 0x21, 0, 0x01, "Num 1",  "BOT"},
    { 0x22, 0, 0x02, "Num 3",  "BOT"},
    { 0x23, 0, 0x03, "Num 5",  "BOT"},
    { 0x24, 0, 0x04, "Num 7",  "BOT"},
    { 0x6D, 0, 0x05, "Num 9",  "BOT"},
    { 0x6E, 0, 0x06, "Num 11", "BOT"},
    { 0x25, 0, 0x07, "E1S 0",  "BOT"},
    { 0x6F, 0, 0x08, "E1S 1",  "BOT"},

    { 0x1A, 0, 0x01, "Num 1",  "TOP"},
    { 0x1B, 0, 0x02, "Num 3",  "TOP"},
    { 0x1C, 0, 0x03, "Num 5",  "TOP"},
    { 0x1D, 0, 0x04, "Num 7",  "TOP"},
    { 0x68, 0, 0x05, "Num 9",  "TOP"},
    { 0x69, 0, 0x06, "Num 11", "TOP"},
    { 0x1E, 0, 0x07, "E1S 0",  "TOP"},
    { 0x6A, 0, 0x08, "E1S 1",  "TOP"},
};

// singel M2 for 4U
MAPTOSTRING root_port_mapping_4u_sgl_m2_gpv3[] = {
    // bus, device, port, silk screen, location
    // Down Stream Ports
    { 0x24, 0, 0x01, "Num 0",  "BOT"},
    { 0x24, 1, 0x02, "Num 1",  "BOT"},
    { 0x24, 2, 0x03, "Num 2",  "BOT"},
    { 0x24, 3, 0x04, "Num 3",  "BOT"},
    { 0x24, 4, 0x05, "Num 4",  "BOT"},
    { 0x24, 5, 0x06, "Num 5",  "BOT"},
    { 0x24, 6, 0x07, "Num 6",  "BOT"},
    { 0x24, 7, 0x08, "Num 7",  "BOT"},
    { 0x6E, 0, 0x09, "Num 8",  "BOT"},
    { 0x6E, 1, 0x0A, "Num 9",  "BOT"},
    { 0x6E, 2, 0x0B, "Num 10", "BOT"},
    { 0x6E, 3, 0x0C, "Num 11", "BOT"},
    { 0x24, 8, 0x0D, "E1S 0",  "BOT"},
    { 0x6E, 4, 0x0E, "E1S 1",  "BOT"},
    { 0x23, 0, 0x0F, "PESW",   "BOT"},
    { 0x6D, 0, 0x10, "PESW",   "BOT"},

    { 0x19, 0, 0x01, "Num 0",  "TOP"},
    { 0x19, 1, 0x02, "Num 1",  "TOP"},
    { 0x19, 2, 0x03, "Num 2",  "TOP"},
    { 0x19, 3, 0x04, "Num 3",  "TOP"},
    { 0x19, 4, 0x05, "Num 4",  "TOP"},
    { 0x19, 5, 0x06, "Num 5",  "TOP"},
    { 0x19, 6, 0x07, "Num 6",  "TOP"},
    { 0x19, 7, 0x08, "Num 7",  "TOP"},
    { 0x67, 0, 0x09, "Num 8",  "TOP"},
    { 0x67, 1, 0x0A, "Num 9",  "TOP"},
    { 0x67, 2, 0x0B, "Num 10", "TOP"},
    { 0x67, 3, 0x0C, "Num 11", "TOP"},
    { 0x19, 8, 0x0D, "E1S 0",  "TOP"},
    { 0x67, 4, 0x0E, "E1S 1",  "TOP"},
    { 0x18, 0, 0x0F, "PESW",   "TOP"},
    { 0x66, 0, 0x10, "PESW",   "TOP"},

    // End devices
    { 0x25, 0, 0x01, "Num 0",  "BOT"},
    { 0x26, 0, 0x02, "Num 1",  "BOT"},
    { 0x27, 0, 0x03, "Num 2",  "BOT"},
    { 0x28, 0, 0x04, "Num 3",  "BOT"},
    { 0x29, 0, 0x05, "Num 4",  "BOT"},
    { 0x2A, 0, 0x06, "Num 5",  "BOT"},
    { 0x2B, 0, 0x07, "Num 6",  "BOT"},
    { 0x2C, 0, 0x08, "Num 7",  "BOT"},
    { 0x6F, 0, 0x09, "Num 8",  "BOT"},
    { 0x70, 0, 0x0A, "Num 9",  "BOT"},
    { 0x71, 0, 0x0B, "Num 10", "BOT"},
    { 0x72, 0, 0x0C, "Num 11", "BOT"},
    { 0x2d, 0, 0x0D, "E1S 0",  "BOT"},
    { 0x73, 0, 0x0E, "E1S 1",  "BOT"},

    { 0x1A, 0, 0x01, "Num 0",  "TOP"},
    { 0x1B, 0, 0x02, "Num 1",  "TOP"},
    { 0x1C, 0, 0x03, "Num 2",  "TOP"},
    { 0x1D, 0, 0x04, "Num 3",  "TOP"},
    { 0x1E, 0, 0x05, "Num 4",  "TOP"},
    { 0x1F, 0, 0x06, "Num 5",  "TOP"},
    { 0x20, 0, 0x07, "Num 6",  "TOP"},
    { 0x21, 0, 0x08, "Num 7",  "TOP"},
    { 0x68, 0, 0x09, "Num 8",  "TOP"},
    { 0x69, 0, 0x0A, "Num 9",  "TOP"},
    { 0x6A, 0, 0x0B, "Num 10", "TOP"},
    { 0x6B, 0, 0x0C, "Num 11", "TOP"},
    { 0x22, 0, 0x0D, "E1S 0",  "TOP"},
    { 0x6C, 0, 0x0E, "E1S 1",  "TOP"},
};

// dual M2 for 2U
MAPTOSTRING root_port_mapping_2u_dual_m2_gpv3[] = {
    // bus, device, port, silk screen, location
    // Down Stream Ports
    { 0x17, 0, 0x01, "Num 1",  "2OU"},
    { 0x17, 1, 0x02, "Num 3",  "2OU"},
    { 0x17, 2, 0x03, "Num 5",  "2OU"},
    { 0x17, 3, 0x04, "Num 7",  "2OU"},
    { 0x65, 0, 0x05, "Num 9",  "2OU"},
    { 0x65, 1, 0x06, "Num 11", "2OU"},
    { 0x17, 4, 0x07, "E1S 0",  "2OU"},
    { 0x65, 2, 0x08, "E1S 1",  "2OU"},
    // End devices
    { 0x18, 0, 0x01, "Num 1",  "2OU"},
    { 0x19, 0, 0x02, "Num 3",  "2OU"},
    { 0x1A, 0, 0x03, "Num 5",  "2OU"},
    { 0x1B, 0, 0x04, "Num 7",  "2OU"},
    { 0x66, 0, 0x05, "Num 9",  "2OU"},
    { 0x67, 0, 0x06, "Num 11", "2OU"},
    { 0x1C, 0, 0x07, "E1S 0",  "2OU"},
    { 0x68, 0, 0x08, "E1S 1",  "2OU"},
};

// single M2 for 2U
MAPTOSTRING root_port_mapping_2u_sgl_m2_gpv3[] = {
    // bus, device, port, silk screen, location
    // Down Stream Ports
    { 0x17, 0, 0x01, "Num 0",  "2OU"},
    { 0x17, 1, 0x02, "Num 1",  "2OU"},
    { 0x17, 2, 0x03, "Num 2",  "2OU"},
    { 0x17, 3, 0x04, "Num 3",  "2OU"},
    { 0x17, 4, 0x05, "Num 4",  "2OU"},
    { 0x17, 5, 0x06, "Num 5",  "2OU"},
    { 0x17, 6, 0x07, "Num 6",  "2OU"},
    { 0x17, 7, 0x08, "Num 7",  "2OU"},
    { 0x65, 0, 0x09, "Num 8",  "2OU"},
    { 0x65, 1, 0x0A, "Num 9",  "2OU"},
    { 0x65, 2, 0x0B, "Num 10", "2OU"},
    { 0x65, 3, 0x0C, "Num 11", "2OU"},
    { 0x17, 8, 0x0D, "E1S 0",  "2OU"},
    { 0x65, 4, 0x0E, "E1S 1",  "2OU"},

    // End devices
    { 0x18, 0, 0x01, "Num 0",  "2OU"},
    { 0x19, 0, 0x02, "Num 1",  "2OU"},
    { 0x1A, 0, 0x03, "Num 2",  "2OU"},
    { 0x1B, 0, 0x04, "Num 3",  "2OU"},
    { 0x1C, 0, 0x05, "Num 4",  "2OU"},
    { 0x1D, 0, 0x06, "Num 5",  "2OU"},
    { 0x1E, 0, 0x07, "Num 6",  "2OU"},
    { 0x1F, 0, 0x08, "Num 7",  "2OU"},
    { 0x66, 0, 0x09, "Num 8",  "2OU"},
    { 0x67, 0, 0x0A, "Num 9",  "2OU"},
    { 0x68, 0, 0x0B, "Num 10", "2OU"},
    { 0x69, 0, 0x0C, "Num 11", "2OU"},
    { 0x20, 0, 0x0D, "E1S 0",  "2OU"},
    { 0x6A, 0, 0x0E, "E1S 1",  "2OU"},
};

// single M2 for BRCM 2U
MAPTOSTRING root_port_mapping_2u_sgl_m2_gpv3_brcm[] = {
    // bus, device, port, silk screen, location
    // Down Stream Ports
    { 0x17, 8, 0x01, "Num 0",  "2OU"},
    { 0x17, 7, 0x02, "Num 1",  "2OU"},
    { 0x17, 1, 0x03, "Num 2",  "2OU"},
    { 0x17, 2, 0x04, "Num 3",  "2OU"},
    { 0x17, 3, 0x05, "Num 4",  "2OU"},
    { 0x17, 4, 0x06, "Num 5",  "2OU"},
    { 0x17, 6, 0x07, "Num 6",  "2OU"},
    { 0x17, 5, 0x08, "Num 7",  "2OU"},
    { 0x65, 3, 0x09, "Num 8",  "2OU"},
    { 0x65, 4, 0x0A, "Num 9",  "2OU"},
    { 0x65, 1, 0x0B, "Num 10", "2OU"},
    { 0x65, 0, 0x0C, "Num 11", "2OU"},
    { 0x17, 0, 0x0D, "E1S 0",  "2OU"},
    { 0x65, 2, 0x0E, "E1S 1",  "2OU"},
};

MAPTOSTRING root_port_mapping_e1s[] = {
    // bus, device, port, silk screen, location
    { 0xB2, 0, 0x3A, "Num 0", "1OU"},
    { 0xB2, 1, 0x3B, "Num 1", "1OU"},
    { 0xB2, 2, 0x3C, "Num 2", "1OU"},
    { 0xB2, 3, 0x3D, "Num 3", "1OU"},
    { 0x63, 0, 0x2A, "Num 0", "2OU"},
    { 0x63, 1, 0x2B, "Num 1", "2OU"},
    { 0x15, 3, 0x1D, "Num 2", "2OU"},
    { 0x15, 2, 0x1C, "Num 3", "2OU"},
    { 0x15, 1, 0x1B, "Num 4", "2OU"},
    { 0x15, 0, 0x1A, "Num 5", "2OU"},
};

PCIE_ERR_DECODE pcie_err_tab[] = {
    {0x00, "Receiver Error"},
    {0x01, "Bad TLP"},
    {0x02, "Bad DLLP"},
    {0x03, "Replay Time-out"},
    {0x04, "Replay Rollover"},
    {0x05, "Advisory Non-Fatal"},
    {0x06, "Corrected Internal Error"},
    {0x07, "Header Log Overflow"},
    {0x20, "Data Link Protocol Error"},
    {0x21, "Surprise Down Error"},
    {0x22, "Poisoned TLP"},
    {0x23, "Flow Control Protocol Error"},
    {0x24, "Completion Timeout"},
    {0x25, "Completer Abort"},
    {0x26, "Unexpected Completion"},
    {0x27, "Receiver Buffer Overflow"},
    {0x28, "Malformed TLP"},
    {0x29, "ECRC Error"},
    {0x2A, "Unsupported Request"},
    {0x2B, "ACS Violation"},
    {0x2C, "Uncorrectable Internal Error"},
    {0x2D, "MC Blocked TLP"},
    {0x2E, "AtomicOp Egress Blocked"},
    {0x2F, "TLP Prefix Blocked Error"},
    {0x30, "Poisoned TLP Egress Blocked"},
    {0x50, "Received ERR_COR Message"},
    {0x51, "Received ERR_NONFATAL Message"},
    {0x52, "Received ERR_FATAL Message"},
    {0x59, "LER was triggered by ERR_NONFATAL"},
    {0x5A, "LER was triggered by ERR_FATAL"},
    {0xA0, "PERR (non-AER)"},
    {0xA1, "SERR (non-AER)"},
    {0xFF, "None"}
};

err_t minor_auth_error[] = {
  /*MAJOR_ERROR_BMC_AUTH_FAILED or MAJOR_ERROR_PCH_AUTH_FAILED */
  {0x01, "MINOR_ERROR_AUTH_ACTIVE"},
  {0x02, "MINOR_ERROR_AUTH_RECOVERY"},
  {0x03, "MINOR_ERROR_AUTH_ACTIVE_AND_RECOVERY"},
  {0x04, "MINOR_ERROR_AUTH_ALL_REGIONS"},
};
err_t minor_update_error[] = {
  /* MAJOR_ERROR_PCH_UPDATE_FAIELD or MAJOR_ERROR_BMC_UPDATE_FAIELD */
  {0x01, "MINOR_ERROR_INVALID_UPDATE_INTENT"},
  {0x02, "MINOR_ERROR_FW_UPDATE_INVALID_SVN"},
  {0x03, "MINOR_ERROR_FW_UPDATE_AUTH_FAILED"},
  {0x04, "MINOR_ERROR_FW_UPDATE_EXCEEDED_MAX_FAILED_ATTEMPTS"},
  {0x05, "MINOR_ERROR_FW_UPDATE_ACTIVE_UPDATE_NOT_ALLOWED"},
  /* MAJOR_ERROR_CPLD_UPDATE_FAIELD */
  {0x06, "MINOR_ERROR_CPLD_UPDATE_INVALID_SVN"},
  {0x07, "MINOR_ERROR_CPLD_UPDATE_AUTH_FAILED"},
  {0x08, "MINOR_ERROR_CPLD_UPDATE_EXCEEDED_MAX_FAILED_ATTEMPTS"},
};

size_t minor_auth_size = sizeof(minor_auth_error)/sizeof(err_t);
size_t minor_update_size = sizeof(minor_update_error)/sizeof(err_t);

static int
pal_key_index(char *key) {

  int i;

  i = 0;
  char last_key[MAX_KEY_LEN] = {0};

  if (pal_is_cwc() == PAL_EOK) {
    strcpy(last_key, CWC_LAST_KEY);
  } else {
    strcpy(last_key, LAST_KEY);
  }

  while(strcmp(key_cfg[i].name, last_key)) {

    // If Key is valid, return success
    if (!strcmp(key, key_cfg[i].name))
      return i;

    i++;
  }

#ifdef DEBUG
  syslog(LOG_WARNING, "pal_key_index: invalid key - %s", key);
#endif
  return -1;
}

static int
key_func_pwr_last_state(int event, void *arg) {
  if (event == KEY_BEFORE_SET) {
    if (strcmp((char *)arg, "on") && strcmp((char *)arg, "off"))
      return -1;
  }

  return 0;
}

static int
key_func_por_cfg(int event, void *arg) {
  if (event == KEY_BEFORE_SET) {
    if (strcmp((char *)arg, "lps") && strcmp((char *)arg, "on") && strcmp((char *)arg, "off"))
      return -1;
  }

  return 0;
}

int
pal_get_key_value(char *key, char *value) {
  int index;

  // Check is key is defined and valid
  if ((index = pal_key_index(key)) < 0)
    return -1;

  return kv_get(key, value, NULL, KV_FPERSIST);
}

int
pal_set_key_value(char *key, char *value) {
  int index, ret;
  // Check is key is defined and valid
  if ((index = pal_key_index(key)) < 0)
    return -1;

  if (key_cfg[index].function) {
    ret = key_cfg[index].function(KEY_BEFORE_SET, value);
    if (ret < 0)
      return ret;
  }

  return kv_set(key, value, 0, KV_FPERSIST);
}

void
pal_dump_key_value(void) {
  int ret;
  int i = 0;
  char value[MAX_VALUE_LEN] = {0x0};
  char last_key[MAX_KEY_LEN] = {0};

  if ( pal_is_cwc() == PAL_EOK ) {
    strcpy(last_key, CWC_LAST_KEY);
  } else {
    strcpy(last_key, LAST_KEY);
  }

  while (strcmp(key_cfg[i].name, last_key)) {
    if ( pal_is_cwc() == PAL_EOK ) {
      if (!strcmp(key_cfg[i].name, LAST_KEY)) {
        i++;
        continue;
      }
    }
    printf("%s:", key_cfg[i].name);
    if ((ret = kv_get(key_cfg[i].name, value, NULL, KV_FPERSIST)) < 0) {
    printf("\n");
  } else {
    printf("%s\n",  value);
  }
    i++;
    memset(value, 0, MAX_VALUE_LEN);
  }
}

int
pal_set_def_key_value() {
  int i;
  char last_key[MAX_KEY_LEN] = {0};
  //char key[MAX_KEY_LEN] = {0};

  if ( pal_is_cwc() == PAL_EOK ) {
    strcpy(last_key, CWC_LAST_KEY);
  } else {
    strcpy(last_key, LAST_KEY);
  }

  for(i = 0; strcmp(key_cfg[i].name, last_key) != 0; i++) {

    if ( pal_is_cwc() == PAL_EOK ) {
      if (!strcmp(key_cfg[i].name, LAST_KEY)) {
        continue;
      }
    }

    if (kv_set(key_cfg[i].name, key_cfg[i].def_val, 0, KV_FCREATE | KV_FPERSIST)) {
#ifdef DEBUG
      syslog(LOG_WARNING, "pal_set_def_key_value: kv_set failed.");
#endif
    }
    if (key_cfg[i].function) {
      key_cfg[i].function(KEY_AFTER_INI, key_cfg[i].name);
    }
  }

  return 0;
}

int
pal_get_boot_order(uint8_t slot_id, uint8_t *req_data, uint8_t *boot, uint8_t *res_len) {
  int i = 0, j = 0;
  int ret = PAL_EOK;
  char key[MAX_KEY_LEN] = {0};
  char str[MAX_VALUE_LEN] = {0};
  char tstr[4] = {0};

  *res_len = 0;

  sprintf(key, BOOR_ORDER_STR, slot_id);
  ret = pal_get_key_value(key, str);
  if ( ret < 0 ) {
    *res_len = 0;
    goto error_exit;
  }

  for (i = 0; i < 2*SIZE_BOOT_ORDER; i += 2) {
    sprintf(tstr, "%c%c\n", str[i], str[i+1]);
    boot[j++] = strtol(tstr, NULL, 16);
  }

  *res_len = SIZE_BOOT_ORDER;

error_exit:
  return ret;
}

int
pal_set_boot_order(uint8_t slot_id, uint8_t *boot, uint8_t *res_data, uint8_t *res_len) {
  int i = 0;
  int j = 0;
  int network_dev = 0;
  int ret = PAL_EOK;
  char key[MAX_KEY_LEN] = {0};
  char str[MAX_VALUE_LEN] = {0};
  char tstr[10] = {0};
  enum {
    BOOT_DEVICE_IPV4 = 0x1,
    BOOT_DEVICE_IPV6 = 0x9,
    BOOT_DEVICE_RSVD = 0xff,
  };

  *res_len = 0;

  for (i = 0; i < SIZE_BOOT_ORDER; i++) {
    if ( (i > 0) && (boot[i] != BOOT_DEVICE_RSVD) ) {  // byte[0] is boot mode, byte[1:5] are boot order
      for (j = i+1; j < SIZE_BOOT_ORDER; j++) {
        //not allow having the same boot devcie in the boot order
        if ( boot[i] == boot[j] ) {
          syslog(LOG_WARNING, "Not allow having the same boot devcie in the boot order");
          ret = CC_INVALID_PARAM;
          goto error_exit;
        }
      }

      if ((boot[i] == BOOT_DEVICE_IPV4) || (boot[i] == BOOT_DEVICE_IPV6)) {
        network_dev++;
      }
    }

    snprintf(tstr, 3, "%02x", boot[i]);
#pragma GCC diagnostic push
// avoid the following compililatin error
//     error: '__builtin___strncat_chk' output may be truncated copying 3 bytes from a string of length 9 [-Werror=stringop-truncation]
//
// per https://stackoverflow.com/questions/50198319/gcc-8-wstringop-truncation-what-is-the-good-practice
// this warning was was added in gcc8
//
// here we do want to truncate the string
#pragma GCC diagnostic ignored "-Wstringop-truncation"
    strncat(str, tstr, 3);
#pragma GCC diagnostic pop
  }

  //not allow having more than 1 network boot device in the boot order
  if ( network_dev > 1 ) {
    syslog(LOG_WARNING, "Not allow having more than 1 network boot device in the boot order");
    ret = CC_INVALID_PARAM;
    goto error_exit;
  }

  sprintf(key, BOOR_ORDER_STR, slot_id);
  ret = pal_set_key_value(key, str);

error_exit:
  return ret;
}

int
pal_set_ppin_info(uint8_t slot, uint8_t *req_data, uint8_t req_len, uint8_t *res_data, uint8_t *res_len) {
  char key[MAX_KEY_LEN];
  char str[MAX_VALUE_LEN] = {0};
  char tstr[8];
  int i, comp_code = CC_UNSPECIFIED_ERROR;

  *res_len = 0;

  for (i = 0; i < SIZE_CPU_PPIN; i++) {
    sprintf(tstr, "%02x", req_data[i]);
    strcat(str, tstr);
  }

  sprintf(key, "slot%u_cpu_ppin", slot);
  if (!pal_set_key_value(key, str)) {
    comp_code = CC_SUCCESS;
  }

  return comp_code;
}

int
pal_get_80port_record(uint8_t slot_id, uint8_t *res_data, size_t max_len, size_t *res_len) {
  int ret;
  uint8_t status;
  uint8_t len;

  ret = fby3_common_check_slot_id(slot_id);
  if (ret < 0 ) {
    ret = PAL_ENOTSUP;
    goto error_exit;
  }

  ret = pal_is_fru_prsnt(slot_id, &status);
  if ( ret < 0 || status == 0 ) {
    ret = PAL_ENOTREADY;
    goto error_exit;
  }

  ret = pal_get_server_12v_power(slot_id, &status);
  if(ret < 0 || SERVER_12V_OFF == status) {
    ret = PAL_ENOTREADY;
    goto error_exit;
  }

  // Send command to get 80 port record from Bridge IC
  ret = bic_get_80port_record(slot_id, res_data, &len, NONE_INTF);
  if (ret == 0)
    *res_len = (size_t)len;

error_exit:
  return ret;
}

int
pal_set_fw_update_ongoing(uint8_t fruid, uint16_t tmout) {
  static uint8_t bmc_location = 0;
  static bool is_called = false;
  uint8_t slot = fruid;

  // get the location
  if ( (bmc_location == 0) && (fby3_common_get_bmc_location(&bmc_location) < 0) ) {
    syslog(LOG_WARNING, "%s() Cannot get the location of BMC", __func__);
    return PAL_ENOTSUP;
  }

  if ( is_called == true ) return PAL_EOK;

  // postprocess function
  // the destructor in fw-util(system.cpp) will call set_update_ongoing twice
  // add the flag to avoid running it again
  if ( tmout == 0 ) {
    is_called = true;
  }

  if (pal_is_cwc() == PAL_EOK) {
    pal_get_fru_slot(fruid, &slot);
  }

  // set fw_update_ongoing flag
  if ( _set_fw_update_ongoing(slot, tmout) < 0 ) {
    printf("Failed to set fw update ongoing\n");
    return PAL_ENOTSUP;
  }

  // set pwr_lock_flag to prevent unexpected power control
  if ( (bmc_location == NIC_BMC) && \
       (bic_set_crit_act_flag((tmout > 0)?SEL_ASSERT:SEL_DEASSERT) < 0) ) {
    printf("Failed to set power lock, dir_type = %s\n", (tmout > 0)?"ASSERT":"DEASSERT");
    return PAL_ENOTSUP;
  }

  return PAL_EOK;
}

int
pal_set_delay_after_fw_update_ongoing()
{
  // when fw_update_ongoing is set, need to wait for a while
  // make sure all daemons pending by pal_is_fw_update_ongoing
  if (pal_is_cwc() == PAL_EOK) {
    sleep(15);
  } else {
    sleep(5);
  }

  return PAL_EOK;
}

bool
pal_get_crit_act_status(int fd) {
#define SB_CPLD_PWR_LOCK_REGISTER 0x10
  int ret = PAL_EOK;
  uint8_t tbuf = SB_CPLD_PWR_LOCK_REGISTER;
  uint8_t rbuf = 0x0;
  uint8_t tlen = 1;
  uint8_t rlen = 1;
  uint8_t retry = MAX_READ_RETRY;

  do {
    ret = i2c_rdwr_msg_transfer(fd, CPLD_ADDRESS, &tbuf, tlen, &rbuf, rlen);
    if ( ret < 0 ) msleep(100);
    else break;
  } while( retry-- > 0 );

  if ( ret < 0 ) {
    syslog(LOG_WARNING, "%s() Failed to do I2C Rd/Wr, something went wrong?", __func__);
    rbuf = 1; // If something went wrong, don't lock the power or users can't run sled-cycle
  }

  return (rbuf == 0); // rbuf == 0 means power lock flag is asserted;
}

// for class 2 only
bool
pal_is_fan_manual_mode(uint8_t slot_id) {
  char value[MAX_VALUE_LEN] = {0};
  int ret = kv_get("fan_manual", value, NULL, 0);

  if (ret < 0) {
    // return false because there is no critical activity in progress
    if (errno == ENOENT) {
      return false;
    }

    syslog(LOG_WARNING, "%s() Cannot get fan_manual", __func__);
    return true;
  }

  return (strncmp(value, "1", 1) == 0)?true:false;
}

bool
pal_is_fw_update_ongoing(uint8_t fruid) {
  uint8_t slot = fruid;
  if (pal_is_cwc() == PAL_EOK) {
    pal_get_fru_slot(fruid, &slot);
  }
  return bic_is_fw_update_ongoing(slot);
}

int
pal_set_sysfw_ver(uint8_t slot, uint8_t *ver) {
  int i;
  char key[MAX_KEY_LEN] = {0};
  char str[MAX_VALUE_LEN] = {0};
  char tstr[10] = {0};

  sprintf(key, SYSFW_VER_STR, (int) slot);
  for (i = 0; i < SIZE_SYSFW_VER; i++) {
    sprintf(tstr, "%02x", ver[i]);
    strcat(str, tstr);
  }

  return pal_set_key_value(key, str);
}

void
pal_update_ts_sled()
{
  char key[MAX_KEY_LEN] = {0};
  char tstr[MAX_VALUE_LEN] = {0};
  struct timespec ts;

  clock_gettime(CLOCK_REALTIME, &ts);
  sprintf(tstr, "%lld", ts.tv_sec);

  sprintf(key, "timestamp_sled");

  pal_set_key_value(key, tstr);
}

int
pal_get_platform_name(char *name) {
  strcpy(name, PLATFORM_NAME);
  return PAL_EOK;
}

int
pal_get_num_slots(uint8_t *num) {
  *num = MAX_NODES;
  return 0;
}

int
pal_get_fru_list(char *list) {
  strcpy(list, pal_fru_list);
  return PAL_EOK;
}

int
pal_get_dev_list(uint8_t fru, char *list)
{
  strcpy(list, pal_dev_fru_list);
  return 0;
}

int
pal_get_fru_capability(uint8_t fru, unsigned int *caps)
{
  int ret = 0;

  switch (fru) {
    case FRU_ALL:
      *caps = FRU_CAPABILITY_SENSOR_HISTORY | FRU_CAPABILITY_SENSOR_READ;
      break;
    case FRU_SLOT1:
    case FRU_SLOT2:
    case FRU_SLOT3:
    case FRU_SLOT4:
      *caps = FRU_CAPABILITY_FRUID_ALL | FRU_CAPABILITY_SENSOR_ALL |
        FRU_CAPABILITY_SERVER | FRU_CAPABILITY_POWER_ALL |
        FRU_CAPABILITY_POWER_12V_ALL | FRU_CAPABILITY_HAS_DEVICE;
      break;
    case FRU_BMC:
      *caps = FRU_CAPABILITY_FRUID_ALL | FRU_CAPABILITY_SENSOR_ALL |
        FRU_CAPABILITY_MANAGEMENT_CONTROLLER;
      break;
    case FRU_NIC:
      *caps = FRU_CAPABILITY_FRUID_READ | FRU_CAPABILITY_SENSOR_ALL |
        FRU_CAPABILITY_NETWORK_CARD;
      break;
    case FRU_NICEXP:
      *caps = FRU_CAPABILITY_FRUID_ALL | FRU_CAPABILITY_SENSOR_READ;
      break;
    case FRU_BB:
      *caps = FRU_CAPABILITY_FRUID_ALL | FRU_CAPABILITY_SENSOR_READ;
      break;
    case FRU_2U:
    case FRU_2U_SLOT3:
      *caps = 0;
      break;
    case FRU_CWC:
      if (pal_is_cwc() == PAL_EOK) {
        *caps = FRU_CAPABILITY_FRUID_ALL | FRU_CAPABILITY_SENSOR_ALL |
          FRU_CAPABILITY_POWER_ALL;
      } else {
        *caps = 0;
      }
      break;
    case FRU_2U_TOP:
    case FRU_2U_BOT:
      if (pal_is_cwc() == PAL_EOK) {
        *caps = FRU_CAPABILITY_FRUID_ALL | FRU_CAPABILITY_HAS_DEVICE |
          FRU_CAPABILITY_SENSOR_ALL | FRU_CAPABILITY_POWER_ALL;
      } else {
        *caps = 0;
      }
      break;
    case FRU_OCPDBG:
      *caps = 0;
      break;
    default:
      ret = -1;
      break;
  }
  return ret;
}

int
pal_get_dev_capability(uint8_t fru, uint8_t dev, unsigned int *caps)
{
  if (pal_is_cwc() == PAL_EOK) {
    if (fru != FRU_SLOT1 && fru != FRU_2U_TOP && fru != FRU_2U_BOT) {
        return -1;
    }
  } else if (fru < FRU_SLOT1 || fru > FRU_SLOT4) {
      return -1;
  }
  if (dev >= DEV_ID0_1OU && dev <= DEV_ID13_2OU) {
    *caps = FRU_CAPABILITY_FRUID_ALL | FRU_CAPABILITY_SENSOR_ALL |
      (FRU_CAPABILITY_POWER_ALL & (~FRU_CAPABILITY_POWER_RESET));
  } else if (dev >= BOARD_1OU && dev <= BOARD_2OU) {
    *caps = FRU_CAPABILITY_FRUID_ALL | FRU_CAPABILITY_SENSOR_ALL;
  } else if (dev >= BOARD_2OU_TOP && dev <= BOARD_2OU_CWC) {
    if (pal_is_cwc() == PAL_EOK) {
      *caps = FRU_CAPABILITY_FRUID_ALL | FRU_CAPABILITY_SENSOR_ALL;
    } else {
      *caps = 0;
    }
  } else {
    *caps = 0;
  }
  return 0;
}

int
pal_get_board_id(uint8_t slot, uint8_t *req_data, uint8_t req_len, uint8_t *res_data, uint8_t *res_len) {
  int ret;
  uint8_t *data = res_data;
  uint8_t bmc_location = 0; //the value of bmc_location is board id.
  *res_len = 0;

  ret = fby3_common_get_bmc_location(&bmc_location);
  if ( ret < 0 ) {
    syslog(LOG_WARNING, "%s() Cannot get the location of BMC", __func__);
    return CC_UNSPECIFIED_ERROR;
  }

  *data++ = bmc_location;

  if ( (bmc_location == BB_BMC) || (bmc_location == DVT_BB_BMC) ) {
    uint8_t bb_rev = 0x00;

    ret = fby3_common_get_bb_board_rev(&bb_rev);
    if (ret) {
      *data++ = 0x00; //board rev id
    } else {
      *data++ = bb_rev; //board rev id
    }
  } else {
    // Config C can not get rev id form NIC EXP CPLD so far
    *data++ = 0x00; //board rev id
  }

  *data++ = slot; //slot id
  *data++ = 0x00; //slot type. server = 0x00
  *res_len = data - res_data;

  return CC_SUCCESS;
}

int
pal_get_slot_index(unsigned char payload_id)
{
  uint8_t bmc_location = 0;
  uint8_t slot_index = 0;
  uint8_t tbuf[16] = {0};
  uint8_t rbuf[16] = {0};
  uint8_t tlen = 0;
  uint8_t rlen = 0;
  int ret = -1;

  ret = fby3_common_get_bmc_location(&bmc_location);
  if ( ret < 0 ) {
    syslog(LOG_WARNING, "%s() Cannot get the location of BMC", __func__);
    return payload_id;
  }

  if ( bmc_location == NIC_BMC ) {
    ret = bic_ipmb_send(payload_id, NETFN_OEM_REQ, 0xF0, tbuf, tlen, rbuf, &rlen, BB_BIC_INTF);
    if (ret) {
      return payload_id;
    } else {
      slot_index = rbuf[0];
      return slot_index;
    }
  } else {
    return payload_id;
  }
}

int
pal_is_fru_prsnt(uint8_t fru, uint8_t *status) {
  int ret = PAL_EOK;
  int val = 0;
  uint8_t bmc_location = 0, board_type = 0;

  ret = fby3_common_get_bmc_location(&bmc_location);
  if ( ret < 0 ) {
    syslog(LOG_WARNING, "%s() Cannot get the location of BMC", __func__);
    return ret;
  }
  if ( bmc_location == NIC_BMC ) {
    if (fby3_common_get_2ou_board_type(FRU_SLOT1, &board_type) < 0) {
      syslog(LOG_WARNING, "Failed to get slot1 2ou board type\n");
    }
  }

  switch (fru) {
    case FRU_SLOT1:
    case FRU_SLOT2:
    case FRU_SLOT3:
    case FRU_SLOT4:
      if ( (bmc_location == BB_BMC) || (bmc_location == DVT_BB_BMC) ) {
        ret = fby3_common_is_fru_prsnt(fru, status);
      } else {
        if ( fru == FRU_SLOT1 ) {
          *status = 1;
        } else {
          *status = 0;
        }
      }
      break;
    case FRU_BB:
      *status = 1;
      break;
    case FRU_NICEXP:
      *status = (bmc_location == NIC_BMC)?1:0;
      break;
    case FRU_NIC:
      *status = 1;
      break;
    case FRU_BMC:
      *status = 1;
      break;
    case FRU_2U:
      if (board_type == GPV3_MCHP_BOARD || board_type == GPV3_BRCM_BOARD) {
        *status = 1;
      } else {
        *status = 0;
      }
      break;
    case FRU_CWC:
      if (board_type == CWC_MCHP_BOARD) {
        *status = 1;
      } else {
        *status = 0;
      }
      break;
    case FRU_2U_TOP:
    case FRU_2U_BOT:
      if (board_type == CWC_MCHP_BOARD) {
        if ((val = bic_is_2u_top_bot_prsnt(FRU_SLOT1)) > 0) {
          if ((fru == FRU_2U_TOP && (val & PRESENT_2U_TOP)) ||
              (fru == FRU_2U_BOT && (val & PRESENT_2U_BOT))) {
            *status = 1;
          } else {
            *status = 0;
          }
        } else {
          *status = 0;
        }
      } else {
        *status = 0;
      }
      break;
    case FRU_2U_SLOT3:
      *status = 0;
      break;
    default:
      *status = 0;
      syslog(LOG_WARNING, "%s() wrong fru id 0x%02x", __func__, fru);
      ret = PAL_ENOTSUP;
  }

  return ret;
}

int
pal_get_fru_id(char *str, uint8_t *fru) {
  int ret = 0;

  ret = fby3_common_get_fru_id(str, fru);
  if ( ret < 0 ) {
    syslog(LOG_WARNING, "%s() wrong fru %s", __func__, str);
    return -1;
  }

  return ret;
}

int
pal_get_fruid_name(uint8_t fru, char *name) {
  return fby3_get_fruid_name(fru, name);
}

int
pal_get_fru_name(uint8_t fru, char *name) {
  int ret = 0;
  uint8_t bmc_location = 0;

  ret = fby3_common_get_bmc_location(&bmc_location);
  if ( ret < 0 ) {
    syslog(LOG_WARNING, "%s() Cannot get the location of BMC", __func__);
    return ret;
  }

  switch(fru) {
    case FRU_ALL:
      sprintf(name, "all");
      break;
    case FRU_SLOT1:
      sprintf(name, "slot1");
      break;
    case FRU_SLOT2:
      sprintf(name, "slot2");
      break;
    case FRU_SLOT3:
      sprintf(name, "slot3");
      break;
    case FRU_SLOT4:
      sprintf(name, "slot4");
      break;
    case FRU_BMC:
      sprintf(name, "bmc");
      break;
    case FRU_NIC:
      sprintf(name, "nic");
      break;
    case FRU_BB:
      sprintf(name, "bb");
      break;
    case FRU_NICEXP:
      sprintf(name, "nicexp");
      break;
    case FRU_2U:
      sprintf(name, "slot1-2U");
      break;
    case FRU_CWC:
      sprintf(name, "slot1-2U-exp");
      break;
    case FRU_2U_TOP:
      sprintf(name, "slot1-2U-top");
      break;
    case FRU_2U_BOT:
      sprintf(name, "slot1-2U-bot");
      break;
    case FRU_2U_SLOT3:
      sprintf(name, "slot3-2U");
      break;
    case FRU_OCPDBG:
      sprintf(name, "ocpdbg");
      break;
    case FRU_AGGREGATE:
      ret = PAL_EOK; //it's the virtual FRU.
      break;
    default:
      syslog(LOG_WARNING, "%s() unknown fruid %d", __func__, fru);
      ret = PAL_ENOTSUP;
  }

  return ret;
}

int
pal_get_fruid_eeprom_path(uint8_t fru, char *path) {
  int ret = 0;
  uint8_t bmc_location = 0;
  uint8_t fru_bus = 0;
  ret = fby3_common_get_bmc_location(&bmc_location);
  if ( ret < 0 ) {
    syslog(LOG_WARNING, "%s() Cannot get the location of BMC", __func__);
    return ret;
  }

  switch(fru) {
  case FRU_BMC:
    if ( (bmc_location == BB_BMC) || (bmc_location == DVT_BB_BMC) ) {
      fru_bus = CLASS1_FRU_BUS;
    } else {
      fru_bus = CLASS2_FRU_BUS;
    }

    sprintf(path, EEPROM_PATH, fru_bus, BMC_FRU_ADDR);
    break;
  case FRU_BB:
    if ( bmc_location == NIC_BMC ) {
      //The FRU of baseboard is owned by BIC on class 2.
      //And so, there is no eeprom path.
      ret = PAL_ENOTSUP;
    } else {
      sprintf(path, EEPROM_PATH, CLASS1_FRU_BUS, BB_FRU_ADDR);
    }
    break;
  case FRU_NICEXP:
    sprintf(path, EEPROM_PATH, CLASS2_FRU_BUS, NICEXP_FRU_ADDR);
    break;
  case FRU_NIC:
    sprintf(path, EEPROM_PATH, NIC_FRU_BUS, NIC_FRU_ADDR);
    break;
  default:
    ret = PAL_ENOTSUP;
  }

  return ret;
}

int
pal_get_fruid_path(uint8_t fru, char *path) {
  char fname[16] = {0};
  int ret = 0;
  uint8_t bmc_location = 0;

  ret = fby3_common_get_bmc_location(&bmc_location);
  if ( ret < 0 ) {
    syslog(LOG_WARNING, "%s() Cannot get the location of BMC", __func__);
    return ret;
  }

  switch(fru) {
  case FRU_SLOT1:
    sprintf(fname, "slot1");
    break;
  case FRU_SLOT2:
    sprintf(fname, "slot2");
    break;
  case FRU_SLOT3:
    sprintf(fname, "slot3");
    break;
  case FRU_SLOT4:
    sprintf(fname, "slot4");
    break;
  case FRU_BMC:
    sprintf(fname, "bmc");
    break;
  case FRU_NIC:
    sprintf(fname, "nic");
    break;
  case FRU_BB:
    sprintf(fname, "bb");
    break;
  case FRU_NICEXP:
    sprintf(fname, "nicexp");
    break;
  case FRU_CWC:
    sprintf(fname, "2U-cwc");
    break;
  case FRU_2U_TOP:
    sprintf(fname, "2U-top");
    break;
  case FRU_2U_BOT:
    sprintf(fname, "2U-bot");
    break;
  default:
    syslog(LOG_WARNING, "%s() unknown fruid %d", __func__, fru);
    ret = PAL_ENOTSUP;
  }

  if ( ret != PAL_ENOTSUP ) {
    sprintf(path, "/tmp/fruid_%s.bin", fname);
  }

  return ret;
}

int
pal_fruid_write(uint8_t fru, char *path)
{
  if (fru == FRU_NIC) {
    syslog(LOG_WARNING, "%s() nic is not supported", __func__);
    return PAL_ENOTSUP;
  } else if (fru == FRU_BB) {
    return bic_write_fruid(FRU_SLOT1, 0, path, BB_BIC_INTF);
  } else if (fru == FRU_CWC) {
    return bic_write_fruid(FRU_SLOT1, 0, path, REXP_BIC_INTF);
  } else if (fru == FRU_2U_TOP) {
    return bic_write_fruid(FRU_SLOT1, 0, path, RREXP_BIC_INTF1);
  } else if (fru == FRU_2U_BOT) {
    return bic_write_fruid(FRU_SLOT1, 0, path, RREXP_BIC_INTF2);
  }

  return bic_write_fruid(fru, 0, path, NONE_INTF);
}

int
pal_dev_fruid_write(uint8_t fru, uint8_t dev_id, char *path) {
  int ret = PAL_ENOTSUP;
  uint8_t config_status = 0;
  uint8_t bmc_location = 0;
  uint8_t type_2ou = UNKNOWN_BOARD;
  uint8_t slot = (fru == FRU_2U_TOP || fru == FRU_2U_BOT) ? FRU_SLOT1 : fru;

  ret = fby3_common_get_bmc_location(&bmc_location);
  if ( ret < 0 ) {
    printf("%s() Couldn't get the location of BMC\n", __func__);
    return ret;
  }

  ret = bic_is_m2_exp_prsnt(slot);
  if ( ret < 0 ) {
    printf("%s() Couldn't get the status of 1OU/2OU\n", __func__);
    return ret;
  }

  config_status = (uint8_t) ret;

  if ( (dev_id == BOARD_1OU) && ((config_status & PRESENT_1OU) == PRESENT_1OU) && (bmc_location != NIC_BMC) ) { // 1U
    return bic_write_fruid(fru, 0, path, FEXP_BIC_INTF);
  } else if ( (config_status & PRESENT_2OU) == PRESENT_2OU ) {
    if ( fby3_common_get_2ou_board_type(slot, &type_2ou) < 0 ) {
      syslog(LOG_WARNING, "%s() Failed to get 2OU board type\n", __func__);
    }

    if ( dev_id == BOARD_2OU && type_2ou == DP_RISER_BOARD ) {
      return bic_write_fruid(fru, 1, path, NONE_INTF);
    } else if ( dev_id == BOARD_2OU && type_2ou != CWC_MCHP_BOARD ) {
      return bic_write_fruid(fru, 0, path, REXP_BIC_INTF);
    } else if ( dev_id >= DEV_ID0_2OU && dev_id <= DEV_ID11_2OU ) {
      // BMC shouldn't have the access to write the FRU that belongs to the vendor since we don't know the writing rules
      // We only have the read access
      if ( type_2ou != GPV3_MCHP_BOARD && type_2ou != CWC_MCHP_BOARD ) {
        return bic_write_fruid(fru, dev_id - DEV_ID0_2OU + 1, path, REXP_BIC_INTF);
      }
      printf("The device doesn't support the function\n");
    } else {
      printf("The illegal dev%d is detected!\n", dev_id);
    }

    // if we reach here, it means something went wrong
    // normally, we should return in the previous statements
    ret = PAL_ENOTSUP;
  } else {
    printf("%s is not present!\n", (dev_id == BOARD_1OU)?"1OU":"2OU");
    return PAL_ENOTSUP;
  }

  return ret;
}

int
pal_is_bmc_por(void) {
  FILE *fp;
  int por = 0;

  fp = fopen("/tmp/ast_por", "r");
  if (fp != NULL) {
    if (fscanf(fp, "%d", &por) != 1) {
      por = 0;
    }
    fclose(fp);
  }

  return (por)?1:0;
}

int
pal_get_sysfw_ver(uint8_t slot, uint8_t *ver) {
  int i;
  int j = 0;
  int ret = 0;
  char key[MAX_KEY_LEN] = {0};
  char str[MAX_VALUE_LEN] = {0};
  char tstr[4] = {0};

  sprintf(key, SYSFW_VER_STR, (int) slot);
  ret = pal_get_key_value(key, str);
  if (ret) {
    syslog(LOG_WARNING, "%s() Failed to run pal_get_key_value. key:%s", __func__, key);
    ret = PAL_ENOTSUP;
    goto error_exit;
  }

  for (i = 0; i < 2*SIZE_SYSFW_VER; i += 2) {
    sprintf(tstr, "%c%c\n", str[i], str[i+1]);
    ver[j++] = strtol(tstr, NULL, 16);
  }

error_exit:
  return ret;
}

int
pal_is_fru_ready(uint8_t fru, uint8_t *status) {
  int ret = PAL_EOK;
  uint8_t bmc_location = 0;
  uint8_t status_12v = 1;

  ret = fby3_common_get_bmc_location(&bmc_location);
  if ( ret < 0 ) {
    syslog(LOG_WARNING, "%s() Cannot get the location of BMC", __func__);
    return ret;
  }

  switch (fru) {
    case FRU_SLOT1:
    case FRU_SLOT2:
    case FRU_SLOT3:
    case FRU_SLOT4:
    case FRU_2U:
    case FRU_CWC:
    case FRU_2U_TOP:
    case FRU_2U_BOT:
    case FRU_2U_SLOT3:
      if (fru == FRU_2U ) {
        fru = FRU_SLOT1;
      } else if (fru == FRU_2U_SLOT3) {
        fru = FRU_SLOT3;
      }
      ret = pal_get_server_12v_power(fru, &status_12v);
      if(ret < 0 || status_12v == SERVER_12V_OFF) {
        *status = 0;
      } else {
        *status = 1;
      }
      break;
    case FRU_BB:
      *status = 1;
      break;
    case FRU_NICEXP:
      *status = (bmc_location == NIC_BMC)?1:0;
      break;
    case FRU_NIC:
      *status = 1;
      break;
    case FRU_BMC:
      *status = 1;
      break;
    default:
      ret = PAL_ENOTSUP;
      *status = 0;
      syslog(LOG_WARNING, "%s() wrong fru id 0x%02x", __func__, fru);
      break;
  }

  return ret;
}

// GUID for System and Device
static int
pal_get_guid(uint16_t offset, char *guid) {
  char path[128] = {0};
  int fd;
  uint8_t bmc_location = 0;
  uint8_t fru_bus = 0;
  ssize_t bytes_rd;

  errno = 0;

  if ( fby3_common_get_bmc_location(&bmc_location) < 0 ) {
    syslog(LOG_ERR, "%s() Cannot get the location of BMC", __func__);
    return -1;
  }

  if ( (bmc_location == BB_BMC) || (bmc_location == DVT_BB_BMC) ) {
    fru_bus = CLASS1_FRU_BUS;
  } else {
    fru_bus = CLASS2_FRU_BUS;
  }

  snprintf(path, sizeof(path), EEPROM_PATH, fru_bus, BB_FRU_ADDR);

  // check for file presence
  if (access(path, F_OK)) {
    syslog(LOG_ERR, "%s() unable to access %s: %s", __func__, path, strerror(errno));
    return errno;
  }

  fd = open(path, O_RDONLY);
  if (fd < 0) {
    syslog(LOG_ERR, "%s() unable to open %s: %s", __func__, path, strerror(errno));
    return errno;
  }

  lseek(fd, offset, SEEK_SET);

  bytes_rd = read(fd, guid, GUID_SIZE);
  if (bytes_rd != GUID_SIZE) {
    syslog(LOG_ERR, "%s() read from %s failed: %s", __func__, path, strerror(errno));
  }

  if (fd > 0 ) {
    close(fd);
  }

  return errno;
}

static int
pal_set_guid(uint16_t offset, char *guid) {
  char path[128] = {0};
  int fd;
  uint8_t bmc_location = 0;
  uint8_t fru_bus = 0;
  ssize_t bytes_wr;

  errno = 0;

  if ( fby3_common_get_bmc_location(&bmc_location) < 0 ) {
    syslog(LOG_ERR, "%s() Cannot get the location of BMC", __func__);
    return -1;
  }

  if ( (bmc_location == BB_BMC) || (bmc_location == DVT_BB_BMC) ) {
    fru_bus = CLASS1_FRU_BUS;
  } else {
    fru_bus = CLASS2_FRU_BUS;
  }

  snprintf(path, sizeof(path), EEPROM_PATH, fru_bus, BB_FRU_ADDR);

  // check for file presence
  if (access(path, F_OK)) {
    syslog(LOG_ERR, "%s() unable to open %s: %s", __func__, path, strerror(errno));
    return errno;
  }

  fd = open(path, O_WRONLY);
  if (fd < 0) {
    syslog(LOG_ERR, "%s() read from %s failed: %s", __func__, path, strerror(errno));
    return errno;
  }

  lseek(fd, offset, SEEK_SET);

  bytes_wr = write(fd, guid, GUID_SIZE);
  if (bytes_wr != GUID_SIZE) {
    syslog(LOG_ERR, "%s() write to %s failed: %s", __func__, path, strerror(errno));
  }

  close(fd);
  return errno;
}

int
pal_get_sys_guid(uint8_t fru, char *guid) {
  if (fru == FRU_SLOT1 || fru == FRU_SLOT2 || fru == FRU_SLOT3 || fru == FRU_SLOT4) {
    return bic_get_sys_guid(fru, (uint8_t *)guid);
  } else {
    return -1;
  }
}

int
pal_set_sys_guid(uint8_t fru, char *str) {
  char guid[GUID_SIZE] = {0};

  if (fru == FRU_SLOT1 || fru == FRU_SLOT2 || fru == FRU_SLOT3 || fru == FRU_SLOT4) {
    pal_populate_guid(guid, str);
    return bic_set_sys_guid(fru, (uint8_t *)guid);
  } else {
    return -1;
  }
}

int
pal_get_dev_guid(uint8_t fru, char *guid) {
  return pal_get_guid(OFFSET_DEV_GUID, guid);
}

int
pal_set_dev_guid(uint8_t fru, char *str) {
  char guid[GUID_SIZE] = {0};

  pal_populate_guid(guid, str);
  if (fru == FRU_BMC) {
    return pal_set_guid(OFFSET_DEV_GUID, guid);
  } else {
    return -1;
  }
}

bool
pal_is_fw_update_ongoing_system(void) {
  uint8_t i;

  for (i = 1; i <= MAX_NUM_FRUS; i++) {
    if (pal_is_fw_update_ongoing(i) == true)
      return true;
  }

  return false;
}

/*
DP Riser bifurcation table
-------------------------------------------------
              | P3  P2  P1  P0 |  Speed
-------------------------------------------------
Retimer 1x16  | 0   1   0   0  |  x16 (0x08)
Retimer 2x8   | 0   0   0   1  |  x8  (0x09)
Retimer 4x4   | 0   0   0   0  |  x4  (0x0A)
Others Cards  | 1   X   X   X  |  x16 (0x08)
*/
static int pal_get_dp_pcie_config(uint8_t slot_id, uint8_t *pcie_config) {
  const uint8_t dp_pcie_card_mask = 0x08;
  uint8_t dp_pcie_conf;
  if (bic_get_dp_pcie_config(slot_id, &dp_pcie_conf)) {
    syslog(LOG_ERR, "%s() Cannot get DP PCIE configuration\n", __func__);
    return -1;
  }

  syslog(LOG_INFO, "%s() DP PCIE config: %u\n", __func__, dp_pcie_conf);

  if (dp_pcie_conf & dp_pcie_card_mask) {
    // PCIE Card
    (*pcie_config) = CONFIG_D_DP_X16;
  } else {
    // Retimer Card
    switch(dp_pcie_conf) {
      case DP_RETIMER_X16:
        (*pcie_config) = CONFIG_D_DP_X16;
        break;
      case DP_RETIMER_X8:
        (*pcie_config) = CONFIG_D_DP_X8;
        break;
      case DP_RETIMER_X4:
        (*pcie_config) = CONFIG_D_DP_X4;
        break;
      default:
        syslog(LOG_ERR, "%s() Unable to get correct DP PCIE configuration\n", __func__);
        return -1;
    }
  }

  return 0;
}

static int pal_get_e1s_pcie_config(uint8_t slot_id, uint8_t *pcie_config) {
  int retry = 0;
  int ret = 0;
  int ssd_count = 0;
  int i2cfd = -1;
  uint8_t bus = 0;
  uint8_t tbuf[2] = {0x05};
  uint8_t rbuf[2] = {0};

  ret = fby3_common_get_bus_id(slot_id);
  if ( ret < 0 ) {
    syslog(LOG_WARNING, "%s() Cannot get the bus with fru%d", __func__, slot_id);
    return -1;
  }

  bus = (uint8_t)ret + 4;
  i2cfd = i2c_cdev_slave_open(bus, CPLD_ADDRESS >> 1, I2C_SLAVE_FORCE_CLAIM);
  if ( i2cfd < 0 ) {
    syslog(LOG_WARNING, "%s() Failed to open bus %d. Err: %s", __func__, bus, strerror(errno));
    return -1;
  }

  while (retry < 3) {
    ret = i2c_rdwr_msg_transfer(i2cfd, CPLD_ADDRESS, tbuf, 1, rbuf, 1);
    if (ret == 0)
      break;
    retry++;
  }
  if ( ret < 0 ) {
    syslog(LOG_WARNING, "%s() Failed to do i2c_rdwr_msg_transfer", __func__);
    return -1;
  }

  if ( i2cfd > 0 ) close(i2cfd);

  for(int i=1; i<=4; i++) {
    if ( (rbuf[0] & (0x1<<i)) == 0 ) {
      ssd_count++;
    }
  }
  syslog(LOG_WARNING, "%s() ssd_count: %d", __func__, ssd_count);

  if (ssd_count >= 2) {
    *pcie_config = CONFIG_B_E1S_T3;
  } else {
    *pcie_config = CONFIG_B_E1S_T10;
  }

  return 0;
}

int
pal_get_m2_config(uint8_t fru, uint8_t *config)
{
  uint8_t tbuf[8] = {0x9c, 0x9c, 0x00, 0x00};
  uint8_t rbuf[8] = {0};
  uint8_t tlen = 4;
  uint8_t rlen = 0;
  uint8_t intf = 0;
  int ret = -1;
  char key[MAX_KEY_LEN] = {0};
  char value[MAX_VALUE_LEN] = {0};
  uint8_t slot = fru;

  switch (fru) {
    case FRU_2U:
    case FRU_CWC:
    case FRU_2U_SLOT3:
      intf = REXP_BIC_INTF;
      break;
    case FRU_2U_TOP:
      intf = RREXP_BIC_INTF1;
      break;
    case FRU_2U_BOT:
      intf = RREXP_BIC_INTF2;
      break;
    default:
      return -1;
  }

  pal_get_root_fru(fru, &slot);
  snprintf(key, sizeof(key), PCIE_CONFIG, slot, fru);
  if (kv_get(key, value, NULL, 0) == 0) {
    *config = atoi(value);
    return 0;
  }

  ret = bic_ipmb_send(slot, NETFN_OEM_1S_REQ, BIC_CMD_GPV3_GET_M2_CONFIG, tbuf, tlen, rbuf, &rlen, intf);
  if (ret == 0 && rlen == 4) {
    if (rbuf[3] == CONFIG_M2_SINGLE) {
      *config = CONFIG_C_CWC_SINGLE;
    } else if (rbuf[3] == CONFIG_M2_DUAL) {
      *config = CONFIG_C_CWC_DUAL;
    } else {
      return -1;
    }
    snprintf(value, sizeof(value), "%d", *config);
    kv_set(key, value, 0, 0);
  }

  return ret;
}

int pal_get_poss_pcie_config(uint8_t slot, uint8_t *req_data, uint8_t req_len, uint8_t *res_data, uint8_t *res_len) {
  uint8_t pcie_conf = 0xff;
  uint8_t *data = res_data;
  int ret = 0, config_status = 0;
  uint8_t bmc_location = 0;
  uint8_t type_1ou = 0;
  uint8_t type_2ou = UNKNOWN_BOARD;
  ret = fby3_common_get_bmc_location(&bmc_location);
  if (ret < 0) {
    syslog(LOG_ERR, "%s() Cannot get the location of BMC", __func__);
    return -1;
  }

  config_status = bic_is_m2_exp_prsnt(slot);
  if ( (config_status & PRESENT_2OU) == PRESENT_2OU ) {
    //check if GPv3 is installed
    if ( fby3_common_get_2ou_board_type(slot, &type_2ou) < 0 ) {
      syslog(LOG_WARNING, "%s() Failed to get 2OU board type\n", __func__);
    }
  }

  if ( (bmc_location == BB_BMC) || (bmc_location == DVT_BB_BMC) ) {
    if ( type_2ou == GPV3_MCHP_BOARD || type_2ou == GPV3_BRCM_BOARD ) {
      pcie_conf = CONFIG_D_GPV3;
    } else if (type_2ou == DP_RISER_BOARD) {
      if (pal_get_dp_pcie_config(slot, &pcie_conf)) {
        // Unable to get correct DP PCIE configuration
        pcie_conf = CONFIG_UNKNOWN;
      }
      syslog(LOG_INFO, "%s() pcie_conf = %u\n", __func__, pcie_conf);
    } else if (config_status == 0) {
      pcie_conf = CONFIG_A;
    } else if (config_status == 1) {
      bic_get_1ou_type(slot, &type_1ou);
      if (type_1ou == EDSFF_1U) {
        if (pal_get_e1s_pcie_config(slot, &pcie_conf)) {
          // Unable to get correct PCIE configuration
          pcie_conf = CONFIG_B_E1S_T10;
        }
        syslog(LOG_INFO, "%s() pcie_conf = %02X\n", __func__, pcie_conf);
      } else {
        pcie_conf = CONFIG_B;
      }
    } else if (config_status == 3) {
      pcie_conf = CONFIG_D;
    } else {
      pcie_conf = CONFIG_B;
    }
  } else {
    if ( type_2ou == GPV3_MCHP_BOARD || type_2ou == GPV3_BRCM_BOARD ) {
      pcie_conf = CONFIG_C_GPV3;
    } else if ( type_2ou == CWC_MCHP_BOARD ) {
      if (pal_get_m2_config(FRU_2U_TOP, &pcie_conf) != 0) {
        pcie_conf = CONFIG_C_CWC_SINGLE;
        syslog(LOG_WARNING, "%s() fail to get pcie_conf, set default value = %02X\n", __func__, pcie_conf);
      }
    } else {
      pcie_conf = CONFIG_C;
    }
  }

  *data++ = pcie_conf;
  *res_len = data - res_data;
  return ret;
}

int
pal_is_slot_server(uint8_t fru)
{
  if ( SERVER_TYPE_DL == fby3_common_get_slot_type(fru) ) {
    return 1;
  }
  return 0;
}

int
pal_is_cmd_valid(uint8_t *data)
{
  uint8_t bus_num = ((data[0] & 0x7E) >> 1); //extend bit[7:1] for bus ID;
  uint8_t address = data[1];

  // protect slot1,2,3,4 BIC
  if ( address == 0x40 && bus_num >= 0 && bus_num <= 3) {
    return -1;
  }

  return PAL_EOK;
}

static int
pal_get_custom_event_sensor_name(uint8_t fru, uint8_t sensor_num, char *name) {
  int ret = PAL_EOK;
  uint8_t board_type = 0;

  switch(fru) {
    case FRU_SLOT1:
    case FRU_SLOT2:
    case FRU_SLOT3:
    case FRU_SLOT4:
      switch(sensor_num) {
        case BIC_SENSOR_VRHOT:
          sprintf(name, "VR_HOT");
          break;
        case BIC_SENSOR_SYSTEM_STATUS:
          sprintf(name, "SYSTEM_STATUS");
          break;
        case ME_SENSOR_SMART_CLST:
          sprintf(name, "SmaRT&CLST");
          break;
        case BIC_SENSOR_PROC_FAIL:
          sprintf(name, "PROC_FAIL");
          break;
        case BIC_SENSOR_SSD_HOT_PLUG:
          if (fby3_common_get_2ou_board_type(fru, &board_type) < 0) {
            syslog(LOG_ERR, "%s() Cannot get board_type", __func__);
            board_type = M2_BOARD;
          }
          if (board_type == E1S_BOARD) {
            snprintf(name, MAX_SNR_NAME, "E1S_NOT_PRESENT");
          } else {
            snprintf(name, MAX_SNR_NAME, "SSD_HOT_PLUG");
          }
          break;
        case BB_BIC_SENSOR_POWER_DETECT:
          sprintf(name, "POWER_DETECT");
          break;
        case BB_BIC_SENSOR_BUTTON_DETECT:
          sprintf(name, "BUTTON_DETECT");
          break;
        default:
          sprintf(name, "Unknown");
          ret = PAL_ENOTSUP;
          break;
      }
      break;
    default:
      sprintf(name, "Unknown");
      ret = PAL_ENOTSUP;
      break;
  }

  return ret;
}

int
pal_get_event_sensor_name(uint8_t fru, uint8_t *sel, char *name) {
  uint8_t snr_type = sel[10];
  uint8_t snr_num = sel[11];

  switch (snr_type) {
    // If SNR_TYPE is OS_BOOT, sensor name is OS
    case OS_BOOT:
      // OS_BOOT used by OS
      sprintf(name, "OS");
      return PAL_EOK;
    default:
      if ( pal_get_custom_event_sensor_name(fru, snr_num, name) == PAL_EOK ) {
        return PAL_EOK;
      }
  }

  // Otherwise, translate it based on snr_num
  return pal_get_x86_event_sensor_name(fru, snr_num, name);
}

static int
pal_parse_proc_fail(uint8_t fru, uint8_t *event_data, char *error_log) {
  enum {
    FRB3                  = 0x04,
  };

  switch(event_data[0]) {
    case FRB3:
      strcat(error_log, "FRB3, ");
      break;
    default:
      strcat(error_log, "Undefined data, ");
      break;
  }

  return PAL_EOK;
}

static int
pal_parse_smart_clst_event(uint8_t fru, uint8_t *event_data, char *error_log) {
  enum {
    /*
    00h - transition to OK
    01h - transition to noncritical from OK
    02h - transition to critical from less severe
    03h - transition to unrecoverable from less severe
    04h - transition to noncritical from more severe
    05h - transition to critical from unrecoverable
    06h - transition to unrecoverable
    07h - monitor
    08h - informational*/
    TRANS_TO_OK                    = 0x00,
    TRANS_TO_NON_CRIT_FROM_OK      = 0x01,
    TRANS_TO_CRIT_FROM_LESS_SEVERE = 0x02,
    TRANS_TO_UNR_FROM_LESS_SEVERE  = 0x03,
    TRANS_TO_NCR_FROM_MORE_SEVERE  = 0x04,
    TRANS_TO_CRIT_FROM_UNR         = 0x05,
    TRANS_TO_UNR                   = 0x06,
    MONITOR                        = 0x07,
    INFORMATIONAL                  = 0x08,

    /*
    0h – State Deasserted (throttling released)
    1h – State Asserted (throttling enforced)
    */
    THROTTLING_RELEASED = 0x00,
    THROTTLING_ENFORCED = 0x01,
  };
  uint8_t code = (event_data[0] & 0x1);
  uint8_t severity = (event_data[1] >> 4) & 0x0f;
  const uint8_t ME_FW_ASSERTION[3] = {0x61, 0x2F, 0x00}; //612F00h is a special case. it just displays the ME FW assertion.

  //handle the special case
  if ( memcmp(event_data, (uint8_t *)&ME_FW_ASSERTION, 3) == 0 ) {
    strcat(error_log, "Management Engine FW");
    return PAL_EOK;
  }

  switch(code) {
    case THROTTLING_RELEASED:
      strcat(error_log, "Throttling released, ");
      break;
    case THROTTLING_ENFORCED:
      strcat(error_log, "Throttling enforced, ");
      break;
    default:
      strcat(error_log, "Undefined data, ");
      break;
  }

  switch (severity) {
    case TRANS_TO_OK:
      strcat(error_log, "Transition to OK");
      break;
    case TRANS_TO_NON_CRIT_FROM_OK:
      strcat(error_log, "Transition to noncritical from OK");
      break;
    case TRANS_TO_CRIT_FROM_LESS_SEVERE:
      strcat(error_log, "Transition to critical from less severe");
      break;
    case TRANS_TO_UNR_FROM_LESS_SEVERE:
      strcat(error_log, "Transition to unrecoverable from less severe");
      break;
    case TRANS_TO_NCR_FROM_MORE_SEVERE:
      strcat(error_log, "Transition to noncritical from more severe");
      break;
    case TRANS_TO_CRIT_FROM_UNR:
      strcat(error_log, "Transition to critical from unrecoverable");
      break;
    case TRANS_TO_UNR:
      strcat(error_log, "Transition to unrecoverable");
      break;
    case MONITOR:
      strcat(error_log, "Monitor");
      break;
    case INFORMATIONAL:
      strcat(error_log, "Informational");
      break;
    default:
      strcat(error_log, "Undefined severity");
      break;
  }

  return PAL_EOK;
}

static int
pal_parse_vr_event(uint8_t fru, uint8_t *event_data, char *error_log) {
  enum {
    VCCIN_VRHOT    = 0x00,
    VCCIO_VRHOT    = 0x01,
    DIMM_ABC_VRHOT = 0x02,
    DIMM_DEF_VRHOT = 0x03,
  };
  uint8_t event = event_data[0];

  switch (event) {
    case VCCIN_VRHOT:
      strcat(error_log, "CPU VCCIN VR HOT Warning");
      break;
    case VCCIO_VRHOT:
      strcat(error_log, "CPU VCCIO VR HOT Warning");
      break;
    case DIMM_ABC_VRHOT:
      strcat(error_log, "DIMM ABC Memory VR HOT Warning");
      break;
    case DIMM_DEF_VRHOT:
      strcat(error_log, "DIMM DEF Memory VR HOT Warning");
      break;
    default:
      strcat(error_log, "Undefined VR event");
      break;
  }

  return PAL_EOK;
}

uint8_t
pal_get_gpv3_cfg() {
  char value[MAX_VALUE_LEN] = {0};
  int ret = PAL_ENOTSUP;
  static uint8_t cfg = 0;

  if ( cfg == 0 ) {
    ret = kv_get("gpv3_cfg", value, NULL, 0);
    if ( ret < 0 ) {
      syslog(LOG_WARNING, "Failed to get gpv3_cfg\n");
    } else cfg = atoi(value);
  }

  // return 0 by default
  return cfg;
}

static void
pal_sel_root_port_mapping_tbl(uint8_t fru, uint8_t *bmc_location, MAPTOSTRING **tbl, uint8_t *cnt) {
  enum {
    GPV3_84CH_DUAL  = 0x4,
    GPV3_100CH_DUAL = 0x6,
  };
  uint8_t board_1u = M2_BOARD;
  uint8_t board_2u = M2_BOARD;
  uint8_t config_status = CONFIG_UNKNOWN;
  int ret = 0;
  size_t tbl_size = 0;

  do {
    ret = fby3_common_get_bmc_location(bmc_location);
    if ( ret < 0 ) {
      syslog(LOG_WARNING, "%s() Cannot get the location of BMC\n", __func__);
      break;
    }

    ret = bic_is_m2_exp_prsnt(fru);
    if ( ret < 0 ) {
      syslog(LOG_WARNING, "%s() Cannot get bic_is_m2_exp_prsnt\n", __func__);
      break;
    } else config_status = (uint8_t)ret;

    // For Config C and D, there are EDSFF_1U, E1S_BOARD and GPv3 architecture
    // BMC should select the corresponding table.
    // For Config B and A, root_port_mapping should be selected.
    // only check it when 1OU is present
    if ( *bmc_location != NIC_BMC && ((config_status & PRESENT_1OU) == PRESENT_1OU) ) {
      ret = bic_get_1ou_type(fru, &board_1u);
      if (ret < 0) {
        syslog(LOG_ERR, "%s() Cannot get 1ou_board_type\n", __func__);
        break;
      }
    }
    // only check it when 2OU is present
    if ( ((config_status & PRESENT_2OU) == PRESENT_2OU) ) {
      ret = fby3_common_get_2ou_board_type(fru, &board_2u);
      if (ret < 0) {
        syslog(LOG_ERR, "%s() Cannot get 2ou_board_type\n", __func__);
        break;
      }

      // get the GPv3 config and provide the corresponding table
      if ( board_2u == GPV3_MCHP_BOARD || board_2u == CWC_MCHP_BOARD ) {
        // re-use it
        config_status = pal_get_gpv3_cfg();
      }
    }
  } while(0);

  if ( ret < 0 ) {
    syslog(LOG_ERR, "%s() Use the default root_port_mapping\n", __func__);
    board_1u = M2_BOARD; //make sure the default is used
    board_2u = M2_BOARD;
  }

  if ( board_1u == EDSFF_1U || board_2u == E1S_BOARD ) {
    // case 1/2OU E1S
    *tbl = root_port_mapping_e1s;
    tbl_size = sizeof(root_port_mapping_e1s);
  } else if ( board_2u == GPV3_MCHP_BOARD ) {
    // case Config C and Config D GPv3
    if ( config_status == GPV3_84CH_DUAL || config_status == GPV3_100CH_DUAL ) {
      *tbl = root_port_mapping_2u_dual_m2_gpv3;
      tbl_size = sizeof(root_port_mapping_2u_dual_m2_gpv3);
    } else { // single by default
      *tbl = root_port_mapping_2u_sgl_m2_gpv3;
      tbl_size = sizeof(root_port_mapping_2u_sgl_m2_gpv3);
    }
  } else if ( board_2u == GPV3_BRCM_BOARD) {
    // case BRCM GPv3
    *tbl = root_port_mapping_2u_sgl_m2_gpv3_brcm;
    tbl_size = sizeof(root_port_mapping_2u_sgl_m2_gpv3_brcm);
  } else if ( board_2u == CWC_MCHP_BOARD ) {
    if ( config_status == GPV3_84CH_DUAL ||config_status == GPV3_100CH_DUAL ) {
      *tbl = root_port_mapping_4u_dual_m2_gpv3;
      tbl_size = sizeof(root_port_mapping_4u_dual_m2_gpv3);
    } else { // single by default
      *tbl = root_port_mapping_4u_sgl_m2_gpv3;
      tbl_size = sizeof(root_port_mapping_4u_sgl_m2_gpv3);
    }
  } else {
    *tbl = root_port_mapping;
    tbl_size = sizeof(root_port_mapping);
  }

  *cnt = tbl_size / sizeof(MAPTOSTRING);

  return;
}

static void
pal_search_pcie_err(uint8_t err1_id, uint8_t err2_id, char **err1_desc, char **err2_desc) {
  int i = 0;
  int size = (sizeof(pcie_err_tab)/sizeof(PCIE_ERR_DECODE));

  for ( i = 0; i < size; i++ ) {
    if ( err2_id == pcie_err_tab[i].err_id ) {
      *err2_desc = malloc(strlen(pcie_err_tab[i].err_descr)+ERROR_ID_LOG_LEN+2);
      sprintf(*err2_desc, ", ErrID2: 0x%02X(%s)",err2_id, pcie_err_tab[i].err_descr);
      continue;
    } else if ( err1_id == pcie_err_tab[i].err_id ) {
      *err1_desc = malloc(strlen(pcie_err_tab[i].err_descr)+ERROR_ID_LOG_LEN+2);
      sprintf(*err1_desc, ", ErrID1: 0x%02X(%s)",err1_id, pcie_err_tab[i].err_descr);
      continue;
    }

    if ( *err1_desc != NULL && *err2_desc != NULL ) {
      break;
    }
  }

  if (*err2_desc == NULL) {
    *err2_desc = malloc(ERROR_ID_LOG_LEN);
    sprintf(*err2_desc,", ErrID2: 0x%02X", err2_id);
  }

  if (*err1_desc == NULL) {
    *err1_desc = malloc(ERROR_ID_LOG_LEN);
    sprintf(*err1_desc,", ErrID1: 0x%02X", err1_id);
  }

  return;
}

static bool
pal_search_pcie_dev(MAPTOSTRING *tbl, int size, uint8_t bmc_location, uint8_t dev, uint8_t bus, char **sil, char **location) {
  int i = 0;
  for ( i = 0; i < size; i++ ) {
    // check bus and dev are match
    if ( (bus == tbl[i].bus_value) && \
         (dev == tbl[i].dev_value) ) {
      *location = tbl[i].location;
      // 1OU is not expected on class 2, skip
      if ( !strcmp(*location, "1OU") && bmc_location == NIC_BMC ) {
        continue;
      }
      *sil = tbl[i].silk_screen;
      return true;
    }
  }
  return false;
}

static void
pal_get_pcie_err_string(uint8_t fru, uint8_t *pdata, char **sil, char **location, char **err1_str, char **err2_str) {
  enum {
    DMI_DEV = 0x00,
    DMI_BUS = 0x00,
    DMI_FUN = 0x00,
  };
  uint8_t bmc_location = 0;
  uint8_t dev = pdata[0] >> 3;
  uint8_t fun = pdata[0] & 0x7;
  uint8_t bus = pdata[1];
  uint8_t err1_id = pdata[5];
  uint8_t err2_id = pdata[4];
  uint8_t size = 0;
  MAPTOSTRING *mapping_table = NULL;

  // DMI case bus = 0, dev = 0, fun = 0
  if ( dev == DMI_DEV && bus == DMI_BUS && fun == DMI_FUN ) {
    *location = "SB";
    *sil = "DMI";
  } else {
    // get the table first
    pal_sel_root_port_mapping_tbl(fru, &bmc_location, &mapping_table, &size);

    // search for the device table first
    if ( pal_search_pcie_dev(mapping_table, size, bmc_location, dev, bus, sil, location) == false ) {
      // if dev is not found in the device table, search for the common table
      size = sizeof(root_port_common_mapping)/sizeof(MAPTOSTRING);
      pal_search_pcie_dev(root_port_common_mapping, size, bmc_location, dev, bus, sil, location);
    }
  }

  // parse err
  pal_search_pcie_err(err1_id, err2_id, err1_str, err2_str);
  return;
}

static void
pal_get_m2vpp_str_name(uint8_t fru, uint8_t comp, uint8_t root_port, char *error_log) {
  int i = 0;
  uint8_t size = 0;
  uint8_t bmc_location = 0;
  MAPTOSTRING *mapping_table = NULL;

  // select root port mapping tbl first
  pal_sel_root_port_mapping_tbl(fru, &bmc_location, &mapping_table, &size);

  for ( i = 0 ; i < size; i++ ) {
    if ( mapping_table[i].root_port == root_port ) {
      char *silk_screen = mapping_table[i].silk_screen;
      char *location = mapping_table[i].location;
      snprintf(error_log, 256, "%s/%s ", location, silk_screen);
      return;
    }
  }

  if ( i == size ) {
    snprintf(error_log, 256, "Undefined M2 RootPort %X ", root_port);
  }
  return;
}

static const char*
pal_get_board_name(uint8_t comp) {
  const char *comp_str[8] = {"ServerBoard", "1OU", "2OU", "SPE", "GPv3", "CWC", "TOP GPv3", "BOT GPv3"};
  const uint8_t comp_size = ARRAY_SIZE(comp_str);
  if ( comp < comp_size ) {
    return comp_str[comp];
  }

  return "Undefined board";
}

static const char*
pal_get_cwc_dev_str(uint8_t dev) {
  const char *dev_str[5] = {"P5V", "P3V3_STBY", "P3V3", "P1V8", "P0V84"};
  const uint8_t dev_size = ARRAY_SIZE(dev_str);
  if ( dev < dev_size ) {
    return dev_str[dev];
  }

  return "Undefined device";
}

static void
pal_get_m2_str_name(uint8_t event, uint8_t comp, uint8_t device_num, char *error_log) {
  if ((event == 0x0c || event == 0x15) && comp == 5) {
    snprintf(error_log, 256, "%s/%s ", pal_get_board_name(comp), pal_get_cwc_dev_str(device_num));
  } else {
    snprintf(error_log, 256, "%s/Num %d ", pal_get_board_name(comp), device_num);
  }
  return;
}

static void
pal_get_2ou_pesw_str_name(uint8_t board_id, char *board_name_str) {
  snprintf(board_name_str, 256, "%s ", pal_get_board_name(board_id));
  return;
}

static void
pal_get_2ou_vr_str_name(uint8_t comp, uint8_t vr_num, char *error_log) {
  const char *vr_list_str[5] = {"P3V3_STBY1", "P3V3_STBY2", "P3V3_STBY3", "P1V8", "PESW VR"};
  const char *cwc_vr_list[] = {"PESW VR"};
  const char *board = pal_get_board_name(comp);
  const uint8_t vr_list_size = ARRAY_SIZE(vr_list_str);
  const uint8_t cwc_vr_list_size = ARRAY_SIZE(cwc_vr_list);

  if (strcmp(board, "CWC") == 0) {
    snprintf(error_log, 256, "%s/%s ", board, (vr_num < cwc_vr_list_size)?cwc_vr_list[vr_num]:"Undefined VR");
  } else {
    snprintf(error_log, 256, "%s/%s ", board, (vr_num < vr_list_size)?vr_list_str[vr_num]:"Undefined VR");
  }
  return;
}

static void
pal_get_gpv3_not_present_str_name(uint8_t comp, uint8_t gpv3_name, char *error_log) {
  const char *gpv3_present_list_str[5] = {"TOP_GPv3_PRESENT1", "TOP_GPv3_PRESENT2", "BOT_GPv3_PRESENT1", "BOT_GPv3_PRESENT2"};
  const uint8_t gpv3_present_list_size = ARRAY_SIZE(gpv3_present_list_str);
  snprintf(error_log, 256, "%s/%s ", pal_get_board_name(comp), (gpv3_name < gpv3_present_list_size)?gpv3_present_list_str[gpv3_name]:"Undefined GPv3");
  return;
}

static void
pal_get_pesw_config_str_name(uint8_t board_id, uint8_t pesw_config, char *error_log) {
  const char *pesw_config_list_str[8] = {"2U GPv3 SINGLE M.2", "2U GPv3 DUAL M.2", "4U CWC", "4U TOP GPv3 DUAL M.2", "4U BOT GPv3 DUAL M.2", "4U TOP GPv3 SINGLE M.2", "4U BOT GPv3 SINGLE M.2","GPv3_HALF_BW_M.2"};
  const uint8_t pesw_config_list_size = ARRAY_SIZE(pesw_config_list_str);

  if (pesw_config == BIC_READ_EEPROM_FAILED ) {
    snprintf(error_log, 256, "%s/%s ", pal_get_board_name(board_id),"Read EEprom Failed");
  } else {
    snprintf(error_log, 256, "%s ", (pesw_config < pesw_config_list_size)?pesw_config_list_str[pesw_config]:"Undefined" );
  }
  return;
}

static void
pal_get_tlp_timeout_str_name(uint8_t board_id, uint8_t pesw_port_id, char *error_log) {
  const char *single_gpv3_pesw_port_list_str[16] = {"GPv3 USP0", "Dev0", "Dev1", "Dev2", "Dev3", "Dev4", "Dev5", "Dev6", "Dev7", \
                                              "E1.S 0", "GPv3 USP1", "Dev8", "Dev9", "Dev10", "Dev11", "E1.S 1"};
  const char *dual_gpv3_pesw_port_list_str[10] = {"GPv3 USP0", "Dev0_1", "Dev2_3", "Dev4_5", "Dev6_7", "E1.S 0", "GPv3 USP1", "Dev8_9", \
                                              "Dev10_11", "E1.S 1"};
  const char *cwc_pesw_port_list_str[6] = {"CWC USP0", "TOP GPv3 USP0", "BOT GPv3 USP0", "CWC USP1", "TOP GPv3 USP1", "BOT GPv3 USP1"};
  const uint8_t single_gpv3_pesw_port_list_size = ARRAY_SIZE(single_gpv3_pesw_port_list_str);
  const uint8_t dual_gpv3_pesw_port_list_size = ARRAY_SIZE(dual_gpv3_pesw_port_list_str);
  const uint8_t cwc_pesw_port_list_size = ARRAY_SIZE(cwc_pesw_port_list_str);
  enum {
    GPv3 = 0x04,
    CWC = 0x05,
    TOP_GPv3 = 0x06,
    BOT_GPv3 = 0x07,
  };
  uint8_t pcie_config = 0xff;

  switch(board_id) {
    case GPv3:
      if (pal_get_m2_config(FRU_2U, &pcie_config) != 0) {
        return;
      }
      break;
    case TOP_GPv3:
      if (pal_get_m2_config(FRU_2U_TOP, &pcie_config) != 0) {
        return;
      }
      break;
    case BOT_GPv3:
      if (pal_get_m2_config(FRU_2U_BOT, &pcie_config) != 0) {
        return;
      }
      break;
    case CWC:
      snprintf(error_log, MAX_ERR_LOG_SIZE, "%s/%s ", pal_get_board_name(board_id), \
          (pesw_port_id < cwc_pesw_port_list_size)?cwc_pesw_port_list_str[pesw_port_id]:"Undefined");
      return;
    default:
      syslog(LOG_WARNING, "%s() Undefined board id:%u",__func__, board_id);
      return;
  }

  switch(pcie_config) {
    case CONFIG_C_CWC_SINGLE:
      snprintf(error_log, MAX_ERR_LOG_SIZE, "%s/%s ", pal_get_board_name(board_id), \
          (pesw_port_id < single_gpv3_pesw_port_list_size)?single_gpv3_pesw_port_list_str[pesw_port_id]:"Undefined");
      return;
    case CONFIG_C_CWC_DUAL:
      snprintf(error_log, MAX_ERR_LOG_SIZE, "%s/%s ", pal_get_board_name(board_id), \
          (pesw_port_id < dual_gpv3_pesw_port_list_size)?dual_gpv3_pesw_port_list_str[pesw_port_id]:"Undefined");
      return;
    default:
      return;
  }
  return;
}

static int
pal_parse_sys_sts_event(uint8_t fru, uint8_t *sel, char *error_log) {
  enum {
    SYS_THERM_TRIP     = 0x00,
    SYS_FIVR_FAULT     = 0x01,
    SYS_SURGE_CURR     = 0x02,
    SYS_PCH_PROCHOT    = 0x03,
    SYS_UV_DETECT      = 0x04,
    SYS_OC_DETECT      = 0x05,
    SYS_OCP_FAULT_WARN = 0x06,
    SYS_FW_TRIGGER     = 0x07,
    SYS_HSC_FAULT      = 0x08,
    SYS_RSVD           = 0x09,
    SYS_VR_WDT_TIMEOUT = 0x0A,
    SYS_M2_VPP         = 0x0B,
    SYS_M2_PGOOD       = 0x0C,
    SYS_VCCIO_FAULT    = 0x0D,
    SYS_SMI_STUCK_LOW  = 0x0E,
    SYS_OV_DETECT      = 0x0F,
    SYS_M2_OCP_DETECT  = 0x10,
    SYS_SLOT_PRSNT     = 0x11,
    SYS_PESW_ERR       = 0x12,
    SYS_2OU_VR_FAULT   = 0x13,
    SYS_GPV3_NOT_PRESENT  = 0x14,
    SYS_PWRGOOD_TIMEOUT   = 0x15,
    SYS_DP_X8_PWR_FAULT   = 0x16,
    SYS_DP_X16_PWR_FAULT  = 0x17,
    SYS_PESW_CONFIG       = 0x18,
    SYS_TLP_TIMEOUT       = 0x19,
    E1S_1OU_M2_PRESENT         = 0x80,
    E1S_1OU_INA230_PWR_ALERT   = 0x81,
    E1S_1OU_HSC_PWR_ALERT      = 0x82,
    E1S_1OU_E1S_P12V_FAULT     = 0x83,
    E1S_1OU_E1S_P3V3_FAULT     = 0x84,
    E1S_1OU_P12V_EDGE_FAULT    = 0x85,
  };
  uint8_t event_dir = sel[12] & 0x80;
  uint8_t *event_data = &sel[13];
  uint8_t event = event_data[0];
  char prsnt_str[32] = {0};
  char log_msg[MAX_ERR_LOG_SIZE] = {0};
  uint8_t type_2ou = UNKNOWN_BOARD;
  char cri_sel[128];

  switch (event) {
    case SYS_THERM_TRIP:
      strcat(error_log, "System thermal trip");
      sprintf(cri_sel, "%s - %s", error_log, ((event_dir & 0x80) == 0)?"Assert":"Deassert" );
      pal_add_cri_sel(cri_sel);
      break;
    case SYS_FIVR_FAULT:
      strcat(error_log, "System FIVR fault");
      break;
    case SYS_SURGE_CURR:
      strcat(error_log, "Surge Current Warning");
      break;
    case SYS_PCH_PROCHOT:
      strcat(error_log, "PCH prochot");
      break;
    case SYS_UV_DETECT:
      strcat(error_log, "Under Voltage Warning");
      sprintf(cri_sel, "CPU FPH by UV - %s",((event_dir & 0x80) == 0)?"Assert":"Deassert" );
      pal_add_cri_sel(cri_sel);
      break;
    case SYS_OC_DETECT:
      strcat(error_log, "OC Warning");
      sprintf(cri_sel, "CPU FPH by OC - %s",((event_dir & 0x80) == 0)?"Assert":"Deassert" );
      pal_add_cri_sel(cri_sel);
      break;
    case SYS_OCP_FAULT_WARN:
      strcat(error_log, "OCP Fault Warning");
      sprintf(cri_sel, "CPU FPH by OCP Fault - %s",((event_dir & 0x80) == 0)?"Assert":"Deassert" );
      pal_add_cri_sel(cri_sel);
      break;
    case SYS_FW_TRIGGER:
      strcat(error_log, "Firmware");
      break;
    case SYS_HSC_FAULT:
      strcat(error_log, "HSC fault");
      break;
    case SYS_VR_WDT_TIMEOUT:
      strcat(error_log, "VR WDT");
      break;
    case SYS_M2_VPP:
      pal_get_m2vpp_str_name(fru, event_data[1], event_data[2], error_log);
      strcat(error_log, "VPP Power Control");
      break;
    case SYS_M2_PGOOD:
      pal_get_m2_str_name(event, event_data[1], event_data[2], error_log);
      strcat(error_log, "Power Good Fault");
      break;
    case SYS_VCCIO_FAULT:
      strcat(error_log, "VCCIO fault");
      break;
    case SYS_SMI_STUCK_LOW:
      strcat(error_log, "SMI stuck low over 90s");
      break;
    case SYS_OV_DETECT:
      strcat(error_log, "VCCIO Over Voltage Fault");
      break;
    case SYS_M2_OCP_DETECT:
      pal_get_m2_str_name(event, event_data[1], event_data[2], error_log);
      strcat(error_log, "INA233 Alert");
      break;
    case SYS_SLOT_PRSNT:
      snprintf(prsnt_str, sizeof(prsnt_str), "Slot%d present", event_data[1]);
      strcat(error_log, prsnt_str);
      break;
    case SYS_PESW_ERR:
      pal_get_2ou_pesw_str_name(event_data[1], error_log);
      strcat(error_log, "2OU PESW error");
      break;
    case SYS_2OU_VR_FAULT:
      pal_get_2ou_vr_str_name(event_data[1], event_data[2], error_log);
      strcat(error_log, "2OU VR fault");
      break;
    case SYS_GPV3_NOT_PRESENT:
      pal_get_gpv3_not_present_str_name(event_data[1], event_data[2], error_log);
      strcat(error_log, "GPv3 Board Not Present Warning");
      break;
    case SYS_PWRGOOD_TIMEOUT:
      pal_get_m2_str_name(event, event_data[1], event_data[2], error_log);
      strcat(error_log, "Power Good Time Out");
      break;
    case SYS_DP_X8_PWR_FAULT:
      fby3_common_get_2ou_board_type(fru, &type_2ou);
      if (type_2ou == M2_BOARD) {
        strcat(error_log, "M.2 Board Power Fault");
      } else if (type_2ou == E1S_BOARD) {
        strcat(error_log, "Sierra Point Board Power Fault");
      } else if ((type_2ou == GPV3_MCHP_BOARD) || (type_2ou == GPV3_BRCM_BOARD)) {
        strcat(error_log, "GPv3 Board Power Fault");
      } else if (type_2ou == DP_RISER_BOARD) {
        strcat(error_log, "DP x8 Riser Power Fault");
      } else if (type_2ou == CWC_MCHP_BOARD) {
        strcat(error_log, "CWC Board Power Fault");
      } else {
        strcat(error_log, "Unknown 2OU Board Power Fault");
      }
      break;
    case SYS_DP_X16_PWR_FAULT:
      strcat(error_log, "DP x16 Riser Power Fault");
      break;
    case SYS_PESW_CONFIG:
      pal_get_pesw_config_str_name(event_data[1], event_data[2], error_log);
      strcat(error_log, "PESW Config Setting");
      break;
    case SYS_TLP_TIMEOUT:
      pal_get_tlp_timeout_str_name(event_data[1], event_data[2], error_log);
      strcat(error_log, "TLP Credit Time Out");
      break;
    case E1S_1OU_M2_PRESENT:
      snprintf(log_msg, sizeof(log_msg), "E1S 1OU M.2 dev%d present", event_data[2]);
      strcat(error_log, log_msg);
      break;
    case E1S_1OU_INA230_PWR_ALERT:
      snprintf(log_msg, sizeof(log_msg), "E1S 1OU INA230 PWR ALERT, alert type:%d", event_data[2]);
      strcat(error_log, log_msg);
      break;
    case E1S_1OU_HSC_PWR_ALERT:
      strcat(error_log, "E1S 1OU HSC Power");
      break;
    case E1S_1OU_E1S_P12V_FAULT:
      snprintf(log_msg, sizeof(log_msg), "E1S 1OU M.2 dev%d P12V fault", event_data[2]);
      strcat(error_log, log_msg);
      break;
    case E1S_1OU_E1S_P3V3_FAULT:
      snprintf(log_msg, sizeof(log_msg), "E1S 1OU M.2 dev%d P3V3 fault", event_data[2]);
      strcat(error_log, log_msg);
      break;
    case E1S_1OU_P12V_EDGE_FAULT:
      strcat(error_log, "E1S 1OU P12V edge fault");
      break;
    case SYS_EVENT_HOST_STALL:
      strcat(error_log, "BIOS stalled");
      break;
    default:
      strcat(error_log, "Undefined system event");
      break;
  }

  return PAL_EOK;
}

static int
pal_parse_ssd_hot_plug_event(uint8_t fru, uint8_t *event_data, char *error_log) {
  enum {
    SSD0 = 0x00,
    SSD1 = 0x01,
    SSD2 = 0x02,
    SSD3 = 0x03,
    SSD4 = 0x04,
    SSD5 = 0x05,
  };
  uint8_t event = event_data[0];

  switch (event) {
    case SSD0:
      strcat(error_log, "SSD0");
      break;
    case SSD1:
      strcat(error_log, "SSD1");
      break;
    case SSD2:
      strcat(error_log, "SSD2");
      break;
    case SSD3:
      strcat(error_log, "SSD3");
      break;
    case SSD4:
      strcat(error_log, "SSD4");
      break;
    case SSD5:
      strcat(error_log, "SSD5");
      break;
    default:
      strcat(error_log, "Undefined hot plug event");
      break;
  }

  return PAL_EOK;
}

static int
pal_parse_pwr_detect_event(uint8_t fru, uint8_t *event_data, char *error_log) {
  enum {
    SLED_CYCLE = 0x00,
    SLOT = 0x01,
  };
  enum {
    CYCLE_12V = 0x00,
    ON_12V = 0x01,
    OFF_12V = 0x02,
    HOST_RESET = 0x03,
  };

  switch (event_data[0]) {
    case SLED_CYCLE:
      strcat(error_log, "SLED_CYCLE by BB BIC");
      break;
    case SLOT:
      strcat(error_log, "SERVER ");
      switch (event_data[1]) {
        case CYCLE_12V:
          strcat(error_log, "12V CYCLE by BB BIC");
          break;
        case ON_12V:
          strcat(error_log, "12V ON by BB BIC");
          break;
        case OFF_12V:
          strcat(error_log, "12V OFF by BB BIC");
          break;
        case HOST_RESET:
          strcat(error_log, "HOST RESET by LCD DEBUG CARD");
          break;
        default:
          strcat(error_log, "Undefined Baseboard BIC event");
          break;
      }
      break;
    default:
      strcat(error_log, "Undefined Baseboard BIC event");
      break;
  }

  return PAL_EOK;
}

static int
pal_parse_button_detect_event(uint8_t fru, uint8_t *event_data, char *error_log) {
  enum {
    ADAPTER_BUTTON_BMC_CO_N_R = 0x01,
    AC_ON_OFF_BTN_SLOT1_N = 0x02,
    AC_ON_OFF_BTN_SLOT3_N = 0x03,
  };
  switch (event_data[0]) {
    case ADAPTER_BUTTON_BMC_CO_N_R:
      strcat(error_log, "ADAPTER_BUTTON_BMC_CO_N_R");
      break;
    case AC_ON_OFF_BTN_SLOT1_N:
      strcat(error_log, "AC_ON_OFF_BTN_SLOT1_N");
      break;
    case AC_ON_OFF_BTN_SLOT3_N:
      strcat(error_log, "AC_ON_OFF_BTN_SLOT3_N");
      break;
    default:
      strcat(error_log, "Undefined Baseboard BIC event");
      break;
  }

  return PAL_EOK;
}

int
pal_parse_sel(uint8_t fru, uint8_t *sel, char *error_log) {
  enum {
    EVENT_TYPE_NOTIF = 0x77, /*IPMI-Table 42-1, Event/Reading Type Code Ranges - OEM specific*/
  };
  uint8_t snr_num = sel[11];
  uint8_t event_dir = sel[12] & 0x80;
  uint8_t event_type = sel[12] & 0x7f;
  uint8_t *event_data = &sel[13];
  bool unknown_snr = false;
  error_log[0] = '\0';

  switch (snr_num) {
    case BIC_SENSOR_VRHOT:
      pal_parse_vr_event(fru, event_data, error_log);
      break;
    case BIC_SENSOR_SYSTEM_STATUS:
      pal_parse_sys_sts_event(fru, sel, error_log);
      break;
    case ME_SENSOR_SMART_CLST:
      pal_parse_smart_clst_event(fru, event_data, error_log);
      break;
    case BIC_SENSOR_PROC_FAIL:
      pal_parse_proc_fail(fru, event_data, error_log);
      break;
    case BIC_SENSOR_SSD_HOT_PLUG:
      pal_parse_ssd_hot_plug_event(fru, event_data, error_log);
      break;
    case BB_BIC_SENSOR_POWER_DETECT:
      pal_parse_pwr_detect_event(fru, event_data, error_log);
      break;
    case BB_BIC_SENSOR_BUTTON_DETECT:
      pal_parse_button_detect_event(fru, event_data, error_log);
      break;
    default:
      unknown_snr = true;
      break;
  }

  if ( unknown_snr == false ) {
    if ( event_type == EVENT_TYPE_NOTIF ) {
      strcat(error_log, " Triggered");
    } else {
      strcat(error_log, ((event_dir & 0x80) == 0)?" Assertion":" Deassertion");
    }
  } else {
    pal_parse_sel_helper(fru, sel, error_log);
  }
  return PAL_EOK;
}

int
pal_parse_oem_unified_sel(uint8_t fru, uint8_t *sel, char *error_log)
{
#define ERROR_LOG_LEN 256

  uint8_t general_info = (uint8_t) sel[3];
  uint8_t error_type = general_info & 0x0f;
  uint8_t plat = 0;
  char temp_log[128] = {0};
  error_log[0] = '\0';
  char *sil = "NA";
  char *location = "NA";
  char *err1_descript = NULL, *err2_descript = NULL;

  switch (error_type) {
    case UNIFIED_PCIE_ERR:
      plat = (general_info & 0x10) >> 4;
      if (plat == 0) {  //x86
        pal_get_pcie_err_string(fru, &sel[10], &sil, &location, &err1_descript, &err2_descript);

        snprintf(error_log, ERROR_LOG_LEN, "GeneralInfo: x86/PCIeErr(0x%02X), Bus %02X/Dev %02X/Fun %02X, %s/%s,"
                            "TotalErrID1Cnt: 0x%04X",
                general_info, sel[11], sel[10] >> 3, sel[10] & 0x7, location, sil, ((sel[13]<<8)|sel[12]));
        if (err2_descript != NULL) {
          strcat(error_log,err2_descript);
          free(err2_descript);
        }
        if (err1_descript != NULL) {
          strcat(error_log,err1_descript);
          free(err1_descript);
        }
      } else {
        snprintf(error_log, ERROR_LOG_LEN, "GeneralInfo: ARM/PCIeErr(0x%02X), Aux. Info: 0x%04X, Bus %02X/Dev %02X/Fun %02X,"
                            "TotalErrID1Cnt: 0x%04X, ErrID2: 0x%02X, ErrID1: 0x%02X",
                general_info, ((sel[9]<<8)|sel[8]),sel[11], sel[10] >> 3, sel[10] & 0x7, ((sel[13]<<8)|sel[12]), sel[14], sel[15]);
      }
      sprintf(temp_log, "PCIe Error ,FRU:%u", fru);
      pal_add_cri_sel(temp_log);

      return PAL_EOK;
  }

  pal_parse_oem_unified_sel_common(fru, sel, error_log);

  return PAL_EOK;
}

int
pal_oem_unified_sel_handler(uint8_t fru, uint8_t general_info, uint8_t *sel) {
  char key[MAX_KEY_LEN] = {0};
  snprintf(key, MAX_KEY_LEN, SEL_ERROR_STR, fru);
  sel_error_record[fru-1]++;
  return pal_set_key_value(key, "0");
}

void
pal_log_clear(char *fru) {
  char key[MAX_KEY_LEN] = {0};
  uint8_t fru_id = 0;
  uint8_t fru_cnt = 0;
  int ret = 0;
  int i = 0;

  if (pal_is_cwc() == PAL_EOK) {
    if ( strcmp(fru, "all") == 0 ) {
      for (fru_id = FRU_CWC; fru_id <= FRU_2U_BOT; fru_id ++) {
        snprintf(key, MAX_KEY_LEN, "cwc_fru%d_sel_error", fru_id);
        pal_set_key_value(key, "1");
        snprintf(key, MAX_KEY_LEN, "cwc_fru%d_sensor_health", fru_id);
        pal_set_key_value(key, "1");
      }
    } else if (strcmp(fru, FRU_NAME_2U_CWC) == 0 || strcmp(fru, FRU_NAME_2U_TOP) == 0 || strcmp(fru, FRU_NAME_2U_BOT) == 0 ) {
      ret = pal_get_fru_id(fru, &fru_id);
      if ( ret == 0 ) {
        snprintf(key, MAX_KEY_LEN, "cwc_fru%d_sel_error", fru_id);
        pal_set_key_value(key, "1");
        snprintf(key, MAX_KEY_LEN, "cwc_fru%d_sensor_health", fru_id);
        pal_set_key_value(key, "1");
      }
    }
  }

  if ( strncmp(fru, "slot", 4) == 0 && strlen(fru) == 5 ) {
    fru_cnt = fru[4] - 0x30;
    i = fru_cnt;
  } else if ( strcmp(fru, "all") == 0 ) {
    fru_cnt = 4;
    i = 1;
  }

  for ( ; ((i <= fru_cnt) && (i != 0)); i++ ) {
    snprintf(key, MAX_KEY_LEN, SEL_ERROR_STR, i);
    pal_set_key_value(key, "1");
    snprintf(key, MAX_KEY_LEN, SNR_HEALTH_STR, i);
    pal_set_key_value(key, "1");
    sel_error_record[i-1] = 0;
  }
}

int
pal_is_debug_card_prsnt(uint8_t *status, uint8_t read_flag) {
  int ret = -1;
  uint8_t bmc_location = 0;
  int retry = 0;
  char key[MAX_KEY_LEN] = {0};
  char value[MAX_VALUE_LEN] = {0};

  ret = fby3_common_get_bmc_location(&bmc_location);
  if ( ret < 0 ) {
    syslog(LOG_WARNING, "%s() Cannot get the location of BMC", __func__);
    return -1;
  }

  if(bmc_location == NIC_BMC) {
    // when updating firmware, front-paneld should pend to avoid the invalid access
    if ( pal_is_fw_update_ongoing(FRU_SLOT1) == true || \
         bic_is_crit_act_ongoing(FRU_SLOT1) == true ) return PAL_ENOTSUP;

    snprintf(key, sizeof(key), DEBUG_CARD_PRSNT_KEY);
    if (read_flag == READ_FROM_CACHE) {
      if (kv_get(key, value, NULL, 0) == 0) {
        *status = atoi(value);
        return 0;
      } else {
        return pal_is_debug_card_prsnt(status, READ_FROM_BIC);
      }
    }

    uint8_t tbuf[3] = {0x9c, 0x9c, 0x00};
    uint8_t rbuf[16] = {0x00};
    uint8_t tlen = 3;
    uint8_t rlen = 1;

    while (retry < 3) {
      ret = bic_ipmb_send(FRU_SLOT1, NETFN_OEM_1S_REQ, BIC_CMD_OEM_GET_DBG_PRSNT, tbuf, tlen, rbuf, &rlen, BB_BIC_INTF);
      if (ret == 0) {
        break;
      }
      retry++;
    }

    if (ret != 0) {
      return -1;
    }
    if(rbuf[3] == 0x0) {
       *status = 1;
    } else {
      *status = 0;
    }
    snprintf(value, sizeof(value), "%d", *status);
    kv_set(key, value, 0, 0);
  }
  else {
    gpio_value_t value;
    gpio_desc_t *gpio = gpio_open_by_shadow(GPIO_OCP_DEBUG_BMC_PRSNT_N);
    if (!gpio) {
      return -1;
    }

    ret = gpio_get_value(gpio, &value);
    gpio_close(gpio);

    if (ret != 0) {
      return -1;
    }
    if (value == 0x0) {
      *status = 1;
    } else {
      *status = 0;
    }
  }

  return 0;
}

int
pal_set_uart_IO_sts(uint8_t slot_id, uint8_t io_sts) {
  const uint8_t UART_POS_BMC = 0x00;
  const uint8_t MAX_RETRY = 3;
  int i2cfd = -1;
  int ret = PAL_EOK;
  int retry = MAX_RETRY;
  int st_idx = slot_id, end_idx = slot_id;
  uint8_t tbuf[5] = {0x00};
  uint8_t rbuf[1] = {0x00};
  uint8_t tlen = 2;
  uint8_t rlen = 0;
  uint8_t bmc_location = 0; //the value of bmc_location is board id.
  uint8_t index = 0;

  ret = fby3_common_get_bmc_location(&bmc_location);
  if ( ret < 0 ) {
    syslog(LOG_WARNING, "%s() Cannot get the location of BMC", __func__);
    return -1;
  }

  if (bmc_location == NIC_BMC) {
    uint8_t bus = 0;
    //class 2 might have slot1 or slot3
    if(bic_get_mb_index(&index) != 0) {
      return -1;
    }

    if(slot_id == UART_POS_BMC) {
      index =  index == FRU_SLOT3 ? FRU_SLOT3 : FRU_SLOT1;
    }

    tbuf[0] = (bus << 1) + 1;
    tbuf[1] = CPLD_ADDRESS;
    tbuf[2] = 0x00; //read 0 byte
    tbuf[3] = BB_CPLD_IO_BASE_OFFSET + index;
    tbuf[4] = io_sts;
    tlen = 5;
    rlen = 0;
    ret = bic_ipmb_send(FRU_SLOT1, NETFN_APP_REQ, CMD_APP_MASTER_WRITE_READ, tbuf, tlen, rbuf, &rlen, BB_BIC_INTF);
    if (ret != 0) {
      syslog(LOG_WARNING, "Failed to update IO sts. reg:%02X, data: %02X\n", tbuf[3], tbuf[1]);
      return ret;
    }
  }
  else {
    i2cfd = i2c_cdev_slave_open(BB_CPLD_BUS, CPLD_ADDRESS >> 1, I2C_SLAVE_FORCE_CLAIM);
    if ( i2cfd < 0 ) {
      syslog(LOG_WARNING, "Failed to open bus 12\n");
      return i2cfd;
    }

    // adjust the st_idx and end_idx, we need to reset all reg values
    // when uart_pos is at BMC position
    if ( slot_id == UART_POS_BMC ) {
      st_idx = FRU_SLOT1;
      end_idx = FRU_SLOT4;
    }

    do {
       tbuf[0] = BB_CPLD_IO_BASE_OFFSET + st_idx; //get the corresponding reg
       tbuf[1] = io_sts; // data to be written
       ret = i2c_rdwr_msg_transfer(i2cfd, CPLD_ADDRESS, tbuf, tlen, NULL, 0);
       if ( ret < 0 ) {
         retry--;
       } else {
         st_idx++; //move on
         retry = MAX_RETRY; //reset it
       }
    } while ( retry > 0 && st_idx <= end_idx );

    if ( retry == 0 ) {
      syslog(LOG_WARNING, "Failed to update IO sts after performed 3 time attempts. reg:%02X, data: %02X\n", tbuf[0], tbuf[1]);
    }

    if ( i2cfd > 0 ) close(i2cfd);
  }
  return ret;
}

int
pal_get_uart_select_from_kv(uint8_t *pos) {
  char value[MAX_VALUE_LEN] = {0};
  uint8_t loc;
  int ret = -1;

  ret = kv_get("debug_card_uart_select", value, NULL, 0);
  if (!ret) {
    loc = atoi(value);
    *pos = loc;
  }

  return ret;
}

int
pal_get_uart_select_from_cpld(uint8_t *uart_select) {
  int fd = 0;
  int retry = 3;
  int ret = -1;
  uint8_t tbuf[3] = {0x00};
  uint8_t rbuf[4] = {0x00};
  uint8_t tlen = 0;
  uint8_t rlen = 0;
  uint8_t bmc_location = 0; //the value of bmc_location is board id.

  ret = fby3_common_get_bmc_location(&bmc_location);
  if ( ret < 0 ) {
    syslog(LOG_WARNING, "%s() Cannot get the location of BMC", __func__);
    return -1;
  }

  if(bmc_location == NIC_BMC) {
    tbuf[0] = 0x9c;
    tbuf[1] = 0x9c;
    tbuf[2] = 0x00;
    tlen = 3;
    rlen = 1;

    retry = 0;

    while (retry < 3) {
      ret = bic_ipmb_send(FRU_SLOT1, NETFN_OEM_1S_REQ, BIC_CMD_OEM_GET_DBG_UART, tbuf, tlen, rbuf, &rlen, BB_BIC_INTF);
      if (ret == 0) {
        break;
      }
      retry++;
    }
    if (ret != 0) {
      return -1;
    }

    if((rbuf[3] == 0) || (rbuf[3] == 1) || (rbuf[3] == 4))
      rbuf[0] = 0;
    else if((rbuf[3] == 2) || (rbuf[3] == 3) || (rbuf[3] == 5) || (rbuf[3] == 6))
      rbuf[0] = 1;
    else
      return -1;
  } else {
    fd = open("/dev/i2c-12", O_RDWR);
    if (fd < 0) {
      syslog(LOG_WARNING, "Failed to open bus 12");
      return -1;
    }

    tbuf[0] = DEBUG_CARD_UART_MUX;
    tlen = 1;
    rlen = 1;

    while (retry-- > 0) {
      ret = i2c_rdwr_msg_transfer(fd, CPLD_ADDRESS, tbuf, tlen, rbuf, rlen);
      if (ret == 0) {
        break;
      }
    }
    if (fd > 0) {
      close(fd);
    }
    if (ret < 0) {
      return -1;
    }
  }

  *uart_select = rbuf[0];
  return 0;
}

int
pal_post_display(uint8_t uart_select, uint8_t postcode) {
  int fd = 0;
  int retry = 3;
  int ret = -1;
  uint8_t tlen = 0;
  uint8_t rlen = 0;
  uint8_t tbuf[5] = {0x00};
  uint8_t rbuf[1] = {0x00};
  uint8_t offset = 0;
  uint8_t bmc_location = 0; //the value of bmc_location is board id.
  uint8_t index = 0;

  ret = fby3_common_get_bmc_location(&bmc_location);
  if ( ret < 0 ) {
    syslog(LOG_WARNING, "%s() Cannot get the location of BMC", __func__);
    return -1;
  }

  if(bmc_location == NIC_BMC) {
    uint8_t bus = 0;

    if(bic_get_mb_index(&index) !=0)
      return -1;

    if((uart_select == FRU_SLOT1) && (index == FRU_SLOT3))
      offset = SLOT3_POSTCODE_OFFSET;
    else
      offset = SLOT1_POSTCODE_OFFSET;

    tbuf[0] = (bus << 1) + 1;
    tbuf[1] = CPLD_ADDRESS;
    tbuf[2] = 0x00;
    tbuf[3] = offset; // offset 00
    tbuf[4] = postcode;
    tlen = 5;
    rlen = 0;
    ret = bic_ipmb_send(FRU_SLOT1, NETFN_APP_REQ, CMD_APP_MASTER_WRITE_READ, tbuf, tlen, rbuf, &rlen, BB_BIC_INTF);
    if (ret != 0) {
      return -1;
    }
  }
  else {
    fd = open("/dev/i2c-12", O_RDWR);
    if (fd < 0) {
      syslog(LOG_WARNING, "Failed to open bus 12");
      return -1;
    }

    switch (uart_select) {
      case 1:
        offset = SLOT1_POSTCODE_OFFSET;
        break;
      case 2:
        offset = SLOT2_POSTCODE_OFFSET;
        break;
      case 3:
        offset = SLOT3_POSTCODE_OFFSET;
        break;
      case 4:
        offset = SLOT4_POSTCODE_OFFSET;
        break;
    }

    tbuf[0] = offset;
    tbuf[1] = postcode;
    tlen = 2;
    rlen = 0;
    do {
      ret = i2c_rdwr_msg_transfer(fd, CPLD_ADDRESS, tbuf, tlen, rbuf, rlen);
    } while (ret < 0 && retry-- > 0);

    if (fd > 0) {
      close(fd);
    }
  }

  return ret;
}

// Handle the received post code, display it on debug card
int
pal_post_handle(uint8_t slot, uint8_t postcode) {
  uint8_t prsnt = 0;
  uint8_t uart_select = 0;
  int ret = -1;

  // Check for debug card presence
  ret = pal_is_debug_card_prsnt(&prsnt, READ_FROM_CACHE);
  if (ret) {
    return ret;
  }

  // No debug card  present, return
  if (prsnt == 0) {
    return 0;
  }

  // Get the UART SELECT from kv, avoid large access CPLD in a short time
  ret = pal_get_uart_select_from_kv(&uart_select);
  if (ret) {
    return ret;
  }

  // If the give server is not selected, return
  if (uart_select != slot) {
    return 0;
  }

  // Display the post code in the debug card
  ret = pal_post_display(uart_select, postcode);

  return ret;
}

static int
pal_caterr_handler(uint8_t fru, bool ierr) {
  uint8_t status;
  long time_cpu_pwrgd = 0;
  long time_caterr = 0;
  char key[MAX_KEY_LEN] = {0};
  char value[MAX_VALUE_LEN] = {0};
  struct timespec ts = {0};
  uint8_t fail_count = 0;

  if (!pal_get_server_power(fru, &status) && !status) {
    syslog(LOG_WARNING, "%s() fru %u is OFF", __func__, fru);
    return PAL_ENOTSUP;
  }

  // save CATERR timestamp
  clock_gettime(CLOCK_REALTIME, &ts);
  time_caterr = ts.tv_sec;
  snprintf(key, sizeof(key), KEY_CATERR_TIMESTAMP, fru);
  snprintf(value, sizeof(value), "%ld", time_caterr);
  if (kv_set(key, value, 0, 0) < 0) {
    syslog(LOG_WARNING, "%s() cache_set key = %s, value = %s failed.\n", __func__, key, value);
  }

  // get last pwrgd time
  snprintf(key, sizeof(key), KEY_CPU_PWRGD_TIMESTAMP, fru);
  if(kv_get(key, value, NULL, 0) == 0) {
    time_cpu_pwrgd = strtol(value, NULL, 10);
  }

  // get fail count
  snprintf(key, sizeof(key), KEY_HOST_FAILURE_COUNT, fru);
  if(kv_get(key, value, NULL, 0) == 0) {
    fail_count = atoi(value);
  }

  if (time_cpu_pwrgd == 0 || (time_cpu_pwrgd > time_caterr)) { // abnormal case, bypass recovery
    return fby3_common_crashdump(fru, ierr, false, CRASHDUMP_NO_POWER_CONTROL);
  }

  if ((time_caterr - time_cpu_pwrgd) < 15) { // CATERR within 15 sec
    if (fail_count < 5) {
      fail_count++;
      snprintf(value, sizeof(value), "%d", fail_count);
      if (kv_set(key, value, 0, 0) < 0) {
        syslog(LOG_WARNING, "%s() cache_set key = %s, value = %s failed.\n", __func__, key, value);
      }
      syslog(LOG_CRIT, "%s slot%u boot failed, recover by power cycle: retry %u", __func__, fru, fail_count);
      pal_set_server_power(fru, SERVER_POWER_CYCLE);
      return 0;
    } else {
      syslog(LOG_CRIT, "%s slot%u boot failed and unrecoverable", __func__, fru);
      return fby3_common_crashdump(fru, ierr, false, CRASHDUMP_POWER_OFF);
    }
  } else { // reset failure count and run original dump process
    snprintf(key, sizeof(key), KEY_HOST_FAILURE_COUNT, fru);
    if (kv_set(key, "0", 0, 0) < 0) {
      syslog(LOG_WARNING, "%s() cache_set key = %s, value = %s failed.\n", __func__, key, value);
    }
  }

  return fby3_common_crashdump(fru, ierr, false, CRASHDUMP_NO_POWER_CONTROL);
}

static void *
pal_cycle_host_after_acd_dump(void *arg) {
    char path[PATH_MAX] = {0};
    uint8_t fru = (uint8_t)(uintptr_t)arg;

    cycle_thread_is_running[fru] = true;
    syslog(LOG_CRIT, "%s: FRU %u", __func__, fru);
    snprintf(path, sizeof(path), "/var/run/autodump%d.pid", fru);

    // wait crash dump finished
    while(access(path, F_OK) == 0) {
        sleep(1);
    }

    syslog(LOG_CRIT, "%s: power cycle FRU %u", __func__, fru);
    pal_set_server_power(fru, SERVER_POWER_CYCLE);

    pthread_detach(pthread_self());
    cycle_thread_is_running[fru] = false;
    pthread_exit(NULL);
}

static void
pal_host_stall_handler(uint8_t fru) {
  pthread_t tid;

  syslog(LOG_CRIT, "%s: FRU %u", __func__, fru);

  if (cycle_thread_is_running[fru] == false) {
    if (pthread_create(&tid, NULL, pal_cycle_host_after_acd_dump, (void*)(uintptr_t)fru) < 0) {
      syslog(LOG_WARNING, "%s Create thread failed!\n", __func__);
    }
  }

  return;
}

static int
pal_bic_sel_handler(uint8_t fru, uint8_t snr_num, uint8_t *event_data) {
  int ret = PAL_EOK;
  bool is_cri_sel = false;
  enum {  //event_data[3]
    SLOT = 0x01,
  };
  enum {  //event_data[4]
    HOST_RESET = 0x03,
  };

  switch (snr_num) {
    case CATERR_B:
      is_cri_sel = true;
      pal_caterr_handler(fru, (event_data[3] == 0x00));  // 00h:IERR, 0Bh:MCERR
      break;
    case CPU_DIMM_HOT:
    case PWR_ERR:
      is_cri_sel = true;
      break;
    case BIC_SENSOR_SYSTEM_STATUS:
      switch(event_data[3]) {
        // it's not the fault event, filter it
        // or the amber LED will blink
        case 0x80: //E1S_1OU_M2_PRESENT
        case 0x11: //SYS_SLOT_PRSNT
        case 0x0B: //SYS_M2_VPP
        case 0x07: //SYS_FW_TRIGGER
          break;
        case SYS_EVENT_HOST_STALL:
          pal_host_stall_handler(fru);
          break;
        default:
          is_cri_sel = true;
          break;
      }

      if (event_data[3] == 0x11) { // when another blade insert/remove, start/stop fscd
        ret = system("/etc/init.d/setup-fan.sh reload &");
        if (ret != 0) {
          syslog(LOG_WARNING, "%s() can not reload setup-fan.sh", __func__);
          return -1;
        }
      }
      break;
    case BB_BIC_SENSOR_POWER_DETECT:
      switch(event_data[3]) {
        case SLOT:
          switch(event_data[4]) {
            case HOST_RESET: //host reset from LCD debug card
              if (pal_can_change_power(FRU_SLOT1)) {
                ret = pal_set_server_power(FRU_SLOT1, SERVER_POWER_RESET);
                if (ret < 0) {
                  syslog(LOG_WARNING, "power_util: pal_set_rst_btn failed for"
                    " fru %u", FRU_SLOT1);
                } else {
                  syslog(LOG_CRIT, "SERVER_POWER_RESET successful for FRU: %d", FRU_SLOT1);
                  pal_set_restart_cause(FRU_SLOT1, RESTART_CAUSE_RESET_PUSH_BUTTON);
                }
              } else {
                syslog(LOG_WARNING, "Server power can not be reset for FRU: %d", FRU_SLOT1);
              }
              break;
            default:
              break;
          }
          break;
        default:
          break;
      }
      break;
  }

  if ( is_cri_sel == true ) {
    char key[MAX_KEY_LEN] = {0};
    if ( (event_data[2] & 0x80) == 0 ) sel_error_record[fru-1]++;
    else sel_error_record[fru-1]--;

    snprintf(key, MAX_KEY_LEN, SEL_ERROR_STR, fru);
    pal_set_key_value(key, (sel_error_record[fru-1] > 0)?"0":"1"); // 0: Assertion,  1: Deassertion
  }

  return PAL_EOK;
}

int
pal_sel_handler(uint8_t fru, uint8_t snr_num, uint8_t *event_data) {
  int ret = PAL_EOK;
  switch(fru) {
    case FRU_SLOT1:
    case FRU_SLOT2:
    case FRU_SLOT3:
    case FRU_SLOT4:
      ret = pal_bic_sel_handler(fru, snr_num, event_data);
      break;
    default:
      ret = PAL_ENOTSUP;
      break;
  }

  return ret;
}

int
pal_get_dev_id(char *str, uint8_t *dev) {
  return fby3_common_dev_id(str, dev);
}

int
pal_get_num_devs(uint8_t slot, uint8_t *num) {

  if (fby3_common_check_slot_id(slot) == 0) {
    *num = MAX_NUM_DEVS - 1;
  } else if (pal_is_cwc() == PAL_EOK && (slot == FRU_2U_TOP || slot == FRU_2U_BOT)) {
    return fby3_common_exp_get_num_devs(slot, num);
  } else {
    *num = 0;
  }

  return 0;
}

int
pal_get_dev_fruid_name(uint8_t fru, uint8_t dev, char *name)
{
  char temp[64] = {0};
  char dev_name[32] = {0};
  int ret = pal_get_fruid_name(fru, name);
  if (ret < 0) {
    return ret;
  }

  switch(fru) {
    case FRU_SLOT1:
    case FRU_SLOT2:
    case FRU_SLOT3:
    case FRU_SLOT4:
      fby3_common_dev_name(dev, dev_name);
      break;
    case FRU_2U_TOP:
    case FRU_2U_BOT:
      fby3_common_exp_dev_name(dev, dev_name);
      break;
    default:
      return -1;
  }

  snprintf(temp, sizeof(temp), "%s %s", name, dev_name);
  strcpy(name, temp);
  return 0;
}

int
pal_get_dev_name(uint8_t fru, uint8_t dev, char *name)
{
  switch(fru) {
    case FRU_SLOT1:
    case FRU_SLOT2:
    case FRU_SLOT3:
    case FRU_SLOT4:
      fby3_common_dev_name(dev, name);
      break;
    case FRU_2U_TOP:
    case FRU_2U_BOT:
      if (pal_is_cwc() == PAL_EOK) {
        fby3_common_exp_dev_name(dev, name);
      } else {
        return -1;
      }
      break;
    default:
      return -1;
  }
  return 0;
}

int
pal_get_dev_fruid_path(uint8_t fru, uint8_t dev_id, char *path) {
  return fby3_get_fruid_path(fru, dev_id, path);
}

int
pal_handle_dcmi(uint8_t fru, uint8_t *request, uint8_t req_len, uint8_t *response, uint8_t *rlen) {
  int ret;
  uint8_t rbuf[256] = {0x00}, len = 0;

  ret = bic_me_xmit(fru, request, req_len, rbuf, &len);
  if (ret || (len < 1)) {
    return -1;
  }

  if (rbuf[0] != 0x00) {
    return -1;
  }

  *rlen = len - 1;
  memcpy(response, &rbuf[1], *rlen);

  return 0;
}

// only for class 2
static int
pal_init_fan_mode(uint8_t ctrl_mode) {
#define FAN_MANUAL 3
#define FAN_AUTO   6
  int ret = PAL_ENOTSUP;
  uint8_t low_time = 0;

  // assign low_time
  if ( AUTO_MODE == ctrl_mode ) low_time = FAN_AUTO;
  else if ( MANUAL_MODE == ctrl_mode ) low_time = FAN_MANUAL;
  else {
    printf("it's not supported, ctrl_mode = %02X\n", ctrl_mode);
    return ret;
  }

  // set crit_act bit and then sleep
  ret = bic_set_crit_act_flag(SEL_ASSERT);
  if ( ret < 0 ) {
    // if we cant notify the other BMC, force to run the mode
    // because it's just a notification
    syslog(LOG_WARNING, "Failed to assert crit_act bit");
  } else {
    printf("Setup fan mode control...\n");
    // if we want to do something before sleeping, put here
    sleep (low_time);
  }

  // recover the flag
  if ( ret == PAL_EOK ) {
    ret = bic_set_crit_act_flag(SEL_DEASSERT);
    if ( ret < 0 ) {
      syslog(LOG_WARNING, "Failed to deassert crit_act bit");
    }
    sleep(2); // need to wait for gpiod to be ready
  }

  return ret;
}

int
pal_set_fan_ctrl (char *ctrl_opt) {
  FILE* fp = NULL;
  uint8_t bmc_location = 0;
  uint8_t ctrl_mode, status;
  int ret = 0;
  char cmd[64] = {0};
  char buf[32];

  ret = fby3_common_get_bmc_location(&bmc_location);
  if ( ret < 0 ) {
    syslog(LOG_WARNING, "%s() Cannot get the location of BMC", __func__);
    return ret;
  }

  // get the option
  if (!strcmp(ctrl_opt, ENABLE_STR)) ctrl_mode = AUTO_MODE;
  else if (!strcmp(ctrl_opt, DISABLE_STR)) ctrl_mode = MANUAL_MODE;
  else if (!strcmp(ctrl_opt, STATUS_STR)) ctrl_mode = GET_FAN_MODE;
  else if (!strcmp(ctrl_opt, WAKEUP_STR)) ctrl_mode = WAKEUP_MODE;
  else if (!strcmp(ctrl_opt, SLEEP_STR))  ctrl_mode = SLEEP_MODE;
  else {
    syslog(LOG_WARNING, "%s() it's not supported, ctrl_opt=%s\n", __func__, ctrl_opt);
    return -1;
  }

  // assign the command if needed
  switch (ctrl_mode) {
    case AUTO_MODE:
    case WAKEUP_MODE:
      snprintf(cmd, sizeof(cmd), "sv start fscd > /dev/null 2>&1");
      break;
    case MANUAL_MODE:
    case SLEEP_MODE:
      snprintf(cmd, sizeof(cmd), "sv force-stop fscd > /dev/null 2>&1");
      break;
  }

  // special fan case for class 2
  if (bmc_location == NIC_BMC) {
    // use WAKEUP_MODE/SLEEP_MODE to skip the above code, adjust it back
    // so we can use the original logic to start fscd
    if ( ctrl_mode == WAKEUP_MODE || ctrl_mode == SLEEP_MODE ) {
      ctrl_mode = (ctrl_mode == WAKEUP_MODE)?AUTO_MODE:MANUAL_MODE;
    } else {
      // notify the other BMC except for getting fan mode
      if ( (pal_is_cwc() != PAL_EOK) && (ctrl_mode != GET_FAN_MODE) && (pal_init_fan_mode(ctrl_mode) < 0) ) {
        syslog(LOG_WARNING, "%s() Failed to call pal_init_fan_mode. ctrl_mode=%02X", __func__, ctrl_mode);
        return -1;
      }

      // get/set/ fan mode
      if ( bic_set_fan_auto_mode(ctrl_mode, &status) < 0 ) {
        syslog(LOG_WARNING, "%s() Failed to call bic_set_fan_auto_mode. ctrl_mode=%02X", __func__, ctrl_mode);
        return -1;
      } else {
        // set the flag by itself, so it can handle requests from the other BMC
        kv_set("fan_manual", (ctrl_mode == AUTO_MODE)?"0":"1", 0, 0);
      }
    }
  }

  if (ctrl_mode == GET_FAN_MODE) {
    if (bmc_location == NIC_BMC) {
      if (status == AUTO_MODE) {
        printf("Auto Mode: Normal\n");
      } else if (status == MANUAL_MODE) {
        printf("Auto Mode: Manual\n");
      } else {
        printf("Auto Mode: Unknown\n");
      }
    } else {
      snprintf(cmd, sizeof(cmd), "fan-util --get | grep \"Fan Mode:\" | cut -d: -f2-");
      if((fp = popen(cmd, "r")) == NULL) {
        printf("Auto Mode: Unknown\n");
        return -1;
      }

      if(fgets(buf, sizeof(buf), fp) != NULL) {
        printf("Auto Mode:%s",buf);
      }
      if(fp != NULL) pclose(fp);
    }
  } else {  // AUTO_MODE or MANUAL_MODE
    if(ctrl_mode == AUTO_MODE) {
      if (system(cmd) != 0)
        return -1;
    } else if (ctrl_mode == MANUAL_MODE){
      if (system(cmd) != 0) {
        // Although sv force-stop sends kill (-9) signal after timeout,
        // it still returns an error code.
        // we will check status here to ensure that fscd has stopped completely.
        syslog(LOG_WARNING, "%s() force-stop timeout", __func__);
        snprintf(cmd, sizeof(cmd), "sv status fscd 2>/dev/null | cut -d: -f1");
        if((fp = popen(cmd, "r")) == NULL) {
          syslog(LOG_WARNING, "%s() popen failed, cmd: %s", __func__, cmd);
          ret = -1;
        } else if(fgets(buf, sizeof(buf), fp) == NULL) {
          syslog(LOG_WARNING, "%s() read popen failed, cmd: %s", __func__, cmd);
          ret = -1;
        } else if(strncmp(buf, "down", 4) != 0) {
          syslog(LOG_WARNING, "%s() failed to terminate fscd", __func__);
          ret = -1;
        }

        if(fp != NULL) pclose(fp);
        if(ret != 0) return ret;
      }
    }
  }

  return ret;
}

int
pal_get_pfr_address(uint8_t fru, uint8_t *bus, uint8_t *addr, bool *bridged) {
  int ret;
  uint8_t bmc_location = 0;

  if ((ret = fby3_common_get_bmc_location(&bmc_location))) {
    syslog(LOG_WARNING, "%s() Cannot get the location of BMC", __func__);
    return ret;
  }

  switch (fru) {
    case FRU_BMC:
      *bus = (bmc_location == NIC_BMC) ? PFR_NICEXP_BUS : PFR_BB_BUS;
      *addr = PFR_MAILBOX_ADDR;
      *bridged = false;
      break;
    case FRU_SLOT1:
    case FRU_SLOT2:
    case FRU_SLOT3:
    case FRU_SLOT4:
      if ((bmc_location == NIC_BMC) && (fru != FRU_SLOT1)) {
        ret = -1;
        break;
      }

      if ((ret = fby3_common_get_bus_id(fru)) < 0) {
        syslog(LOG_WARNING, "%s() get bus failed, fru %d\n", __func__, fru);
        break;
      }

      *bus = ret + 4;  // I2C_4 ~ I2C_7
      *addr = PFR_MAILBOX_ADDR;
      *bridged = false;
      ret = 0;
      break;
    default:
      ret = -1;
      break;
  }
  return ret;
}

int
pal_is_pfr_active(void) {
  int pfr_active = PFR_NONE;
  int ifd, retry = 3;
  uint8_t tbuf[8], rbuf[8];
  char dev_i2c[16];
  uint8_t bus, addr;
  bool bridged;

  if (pal_get_pfr_address(FRU_BMC, &bus, &addr, &bridged)) {
    return pfr_active;
  }

  sprintf(dev_i2c, "/dev/i2c-%d", bus);
  ifd = open(dev_i2c, O_RDWR);
  if (ifd < 0) {
    return pfr_active;
  }

  tbuf[0] = 0x0A;
  do {
    if (!i2c_rdwr_msg_transfer(ifd, addr, tbuf, 1, rbuf, 1)) {
      pfr_active = (rbuf[0] & 0x20) ? PFR_ACTIVE : PFR_UNPROVISIONED;
      break;
    }

#ifdef DEBUG
    syslog(LOG_WARNING, "i2c%u xfer failed, cmd: %02x", 4, tbuf[0]);
#endif
    if (--retry > 0)
      msleep(20);
  } while (retry > 0);
  close(ifd);

  return pfr_active;
}

int
pal_is_slot_pfr_active(uint8_t fru) {
  int pfr_active = PFR_NONE;
  int ifd, retry = 3;
  uint8_t tbuf[8], rbuf[8];
  uint8_t bus, addr;
  bool bridged;

  if (pal_get_pfr_address(fru, &bus, &addr, &bridged)) {
    return pfr_active;
  }

  ifd = i2c_cdev_slave_open(bus, addr >> 1, I2C_SLAVE_FORCE_CLAIM);
  if (ifd < 0) {
    return pfr_active;
  }

  tbuf[0] = 0x0A;
  do {
    if (!i2c_rdwr_msg_transfer(ifd, addr, tbuf, 1, rbuf, 1)) {
      pfr_active = (rbuf[0] & 0x20) ? PFR_ACTIVE : PFR_UNPROVISIONED;
      break;
    }

#ifdef DEBUG
    syslog(LOG_WARNING, "i2c%u xfer failed, cmd: %02x", 4, tbuf[0]);
#endif
    if (--retry > 0)
      msleep(20);
  } while (retry > 0);
  close(ifd);

  return pfr_active;
}

int
pal_fw_update_finished(uint8_t fru, const char *comp, int status) {
  int ret = 0;
  int ifd, retry = 3;
  uint8_t buf[16];
  uint8_t rbuf[16];
  char dev_i2c[16];
  uint8_t bus, addr;
  bool bridged;

  if (pal_get_pfr_address(FRU_BMC, &bus, &addr, &bridged)) {
    return -1;
  }

  ret = status;
  if (ret == 0) {
    sprintf(dev_i2c, "/dev/i2c-%d", bus);
    ifd = open(dev_i2c, O_RDWR);
    if (ifd < 0) {
      return -1;
    }

    buf[0] = 0x13;  // BMC update intent
    if (!strcmp(comp, "bmc")) {
      buf[1] = UPDATE_BMC_ACTIVE;
      buf[1] |= UPDATE_AT_RESET;
    } else if (!strcmp(comp, "pfr_cpld")) {
      buf[1] = UPDATE_CPLD_ACTIVE;
    } else {
      close(ifd);
      return -1;
    }

    sync();
    printf("sending update intent to CPLD...\n");
    fflush(stdout);
    sleep(1);
    do {
      ret = i2c_rdwr_msg_transfer(ifd, addr, buf, 2, rbuf, 1);
      if (ret) {
        syslog(LOG_WARNING, "i2c%u xfer failed, cmd: %02x", addr, buf[0]);
        if (--retry > 0) {
          msleep(100);
        }
      }
    } while (ret && retry > 0);

    buf[1] |= rbuf[0];

    do {
      ret = i2c_rdwr_msg_transfer(ifd, addr, buf, 2, NULL, 0);
      if (ret) {
        syslog(LOG_WARNING, "i2c%u xfer failed, cmd: %02x %02x", addr, buf[0], buf[1]);
        if (--retry > 0) {
          msleep(100);
        }
      }
    } while (ret && retry > 0);
    close(ifd);
  }

  return ret;
}

int
pal_get_pfr_update_address(uint8_t fru, uint8_t *bus, uint8_t *addr, bool *bridged) {
  int ret;
  uint8_t bmc_location = 0;

  ret = fby3_common_get_bmc_location(&bmc_location);
  if (ret < 0) {
    syslog(LOG_ERR, "%s() Cannot get the location of BMC", __func__);
  }

  switch (fru) {
    case FRU_BB:
    case FRU_BMC:
      *bus = (bmc_location == NIC_BMC) ? PFR_NICEXP_BUS : PFR_BB_BUS;
      *addr = CPLD_UPDATE_ADDR;
      *bridged = false;
      break;
    case FRU_SLOT1:
    case FRU_SLOT2:
    case FRU_SLOT3:
    case FRU_SLOT4:
      if ((bmc_location == NIC_BMC) && (fru != FRU_SLOT1)) {
        ret = -1;
        break;
      }

      if ((ret = fby3_common_get_bus_id(fru)) < 0) {
        syslog(LOG_WARNING, "%s() get bus failed, fru %d\n", __func__, fru);
        break;
      }

      *bus = ret + 4;  // I2C_4 ~ I2C_7
      *addr = CPLD_UPDATE_ADDR;
      *bridged = false;
      ret = 0;
      break;
    default:
      ret = -1;
      break;
  }

  return ret;
}

// OEM Command "CMD_OEM_BYPASS_CMD" 0x34
int pal_bypass_cmd(uint8_t slot, uint8_t *req_data, uint8_t req_len, uint8_t *res_data, uint8_t *res_len){
  int ret;
  int completion_code = CC_UNSPECIFIED_ERROR;
  uint8_t netfn, cmd, select;
  uint8_t tlen, rlen;
  uint8_t tbuf[256] = {0x00};
  uint8_t rbuf[256] = {0x00};
  uint8_t status;
  NCSI_NL_MSG_T *msg = NULL;
  NCSI_NL_RSP_T *rsp = NULL;
  uint8_t channel = 0;
  uint8_t netdev = 0;
  uint8_t netenable = 0;
  char sendcmd[128] = {0};
  int i;

  *res_len = 0;

  if (slot < FRU_SLOT1 || slot > FRU_SLOT4) {
    return CC_PARAM_OUT_OF_RANGE;
  }

  ret = pal_is_fru_prsnt(slot, &status);
  if (ret < 0) {
    return -1;
  }
  if (status == 0) {
    return CC_UNSPECIFIED_ERROR;
  }

  ret = pal_get_server_12v_power(slot, &status);
  if(ret < 0 || status == SERVER_12V_OFF) {
    return CC_NOT_SUPP_IN_CURR_STATE;
  }

  if(!pal_is_slot_server(slot)) {
    return CC_UNSPECIFIED_ERROR;
  }

  select = req_data[0];

  switch (select) {
    case BYPASS_BIC:
      tlen = req_len - 6; // payload_id, netfn, cmd, data[0] (select), data[1] (bypass netfn), data[2] (bypass cmd)
      if (tlen < 0) {
        completion_code = CC_INVALID_LENGTH;
        break;
      }

      netfn = req_data[1];
      cmd = req_data[2];

      // Bypass command to Bridge IC
      if (tlen != 0) {
        ret = bic_ipmb_wrapper(slot, netfn, cmd, &req_data[3], tlen, res_data, res_len);
      } else {
        ret = bic_ipmb_wrapper(slot, netfn, cmd, NULL, 0, res_data, res_len);
      }

      if (0 == ret) {
        completion_code = CC_SUCCESS;
      }
      break;
    case BYPASS_ME:
      tlen = req_len - 6; // payload_id, netfn, cmd, data[0] (select), data[1] (bypass netfn), data[2] (bypass cmd)
      if (tlen < 0) {
        completion_code = CC_INVALID_LENGTH;
        break;
      }

      netfn = req_data[1];
      cmd = req_data[2];

      tlen += 2;
      memcpy(tbuf, &req_data[1], tlen);
      tbuf[0] = tbuf[0] << 2;

      // Bypass command to ME
      ret = bic_me_xmit(slot, tbuf, tlen, rbuf, &rlen);
      if (0 == ret) {
        completion_code = rbuf[0];
        memcpy(&res_data[0], &rbuf[1], (rlen - 1));
        *res_len = rlen - 1;
      }
      break;
    case BYPASS_NCSI:
      tlen = req_len - 7; // payload_id, netfn, cmd, data[0] (select), netdev, channel, cmd
      if (tlen < 0) {
        completion_code = CC_INVALID_LENGTH;
        break;
      }

      netdev = req_data[1];
      channel = req_data[2];
      cmd = req_data[3];

      msg = calloc(1, sizeof(NCSI_NL_MSG_T));
      if (!msg) {
        syslog(LOG_ERR, "%s Error: failed msg buffer allocation", __func__);
        break;
      }

      memset(msg, 0, sizeof(NCSI_NL_MSG_T));

      sprintf(msg->dev_name, "eth%d", netdev);
      msg->channel_id = channel;
      msg->cmd = cmd;
      msg->payload_length = tlen;

      for (i=0; i<msg->payload_length; i++) {
        msg->msg_payload[i] = req_data[4+i];
      }
      //send_nl_msg_libnl
      rsp = send_nl_msg_libnl(msg);
      if (rsp) {
        memcpy(&res_data[0], &rsp->msg_payload[0], rsp->hdr.payload_length);
        *res_len = rsp->hdr.payload_length;
        completion_code = CC_SUCCESS;
      } else {
        completion_code = CC_UNSPECIFIED_ERROR;
      }

      free(msg);
      if (rsp)
        free(rsp);

      break;
    case BYPASS_NETWORK:
      tlen = req_len - 6; // payload_id, netfn, cmd, data[0] (select), netdev, netenable
      if (tlen != 0) {
        completion_code = CC_INVALID_LENGTH;
        break;
      }

      netdev = req_data[1];
      netenable = req_data[2];

      if (netenable) {
        if (netenable > 1) {
          completion_code = CC_INVALID_PARAM;
          break;
        }

        sprintf(sendcmd, "ifup eth%d", netdev);
      } else {
        sprintf(sendcmd, "ifdown eth%d", netdev);
      }
      ret = system(sendcmd);
      completion_code = CC_SUCCESS;
      break;
    default:
      return completion_code;
  }

  return completion_code;
}

int
pal_check_pfr_mailbox(uint8_t fru) {
  int ret = 0, i2cfd = 0, retry=0, index = 0;
  uint8_t tbuf[1] = {0}, rbuf[1] = {0};
  uint8_t tlen = 1, rlen = 1;
  uint8_t major_err = 0, minor_err = 0;
  char *major_str = "NA", *minor_str = "NA";
  char fru_str[32] = {0};
  uint8_t bus, addr;
  bool bridged;

  if (pal_get_pfr_address(fru, &bus, &addr, &bridged)) {
    syslog(LOG_WARNING, "%s() Failed to do pal_get_pfr_address(), FRU%d", __func__, fru);
    return -1;
  }

  i2cfd = i2c_cdev_slave_open(bus, addr >> 1, I2C_SLAVE_FORCE_CLAIM);
  if ( i2cfd < 0 ) {
    syslog(LOG_WARNING, "%s() Failed to open bus %d. Err: %s", __func__, bus, strerror(errno));
    return -1;
  }

  tbuf[0] = MAJOR_ERR_OFFSET;
  tlen = 1;
  retry = 0;
  while (retry < MAX_READ_RETRY) {
    ret = i2c_rdwr_msg_transfer(i2cfd, addr, tbuf, tlen, rbuf, rlen);
    if ( ret < 0 ) {
      retry++;
      sleep(1);
    } else {
      major_err = rbuf[0];
      break;
    }
  }
  if (retry == MAX_READ_RETRY) {
    syslog(LOG_WARNING, "%s() Failed to do i2c_rdwr_msg_transfer, tlen=%d", __func__, tlen);
  }

  tbuf[0] = MINOR_ERR_OFFSET;
  tlen = 1;
  retry = 0;
  while (retry < MAX_READ_RETRY) {
    ret = i2c_rdwr_msg_transfer(i2cfd, addr, tbuf, tlen, rbuf, rlen);
    if ( ret < 0 ) {
      retry++;
      sleep(1);
    } else {
      minor_err = rbuf[0];
      break;
    }
  }

  if ( i2cfd > 0 ) close(i2cfd);

  if ( (major_err != 0) || (minor_err != 0) ) {
    if ( major_err == MAJOR_ERROR_PCH_AUTH_FAILED ) {
      major_str = "MAJOR_ERROR_BMC_AUTH_FAILED";
      for (index = 0; index < minor_auth_size; index++) {
        if (minor_err == minor_auth_error[index].err_id) {
          minor_str = minor_auth_error[index].err_des;
          break;
        }
      }
    } else if ( major_err == MAJOR_ERROR_PCH_AUTH_FAILED ) {
      major_str = "MAJOR_ERROR_PCH_AUTH_FAILED";
      for (index = 0; index < minor_auth_size; index++) {
        if (minor_err == minor_auth_error[index].err_id) {
          minor_str = minor_auth_error[index].err_des;
          break;
        }
      }
    } else if ( major_err == MAJOR_ERROR_UPDATE_FROM_PCH_FAILED ) {
      major_str = "MAJOR_ERROR_UPDATE_FROM_PCH_FAILED";
      for (index = 0; index < minor_update_size; index++) {
        if (minor_err == minor_update_error[index].err_id) {
          minor_str = minor_update_error[index].err_des;
          break;
        }
      }
    } else if ( major_err == MAJOR_ERROR_UPDATE_FROM_BMC_FAILED ) {
      major_str = "MAJOR_ERROR_UPDATE_FROM_BMC_FAILED";
      for (index = 0; index < minor_update_size; index++) {
        if (minor_err == minor_update_error[index].err_id) {
          minor_str = minor_update_error[index].err_des;
          break;
        }
      }
    } else {
      major_str = "unknown major error";
    }

    switch (fru) {
      case FRU_BMC:
        snprintf(fru_str, sizeof(fru_str), "BMC");
        break;
      case FRU_SLOT1:
      case FRU_SLOT2:
      case FRU_SLOT3:
      case FRU_SLOT4:
        snprintf(fru_str, sizeof(fru_str), "FRU: %d", fru);
        break;
      default:
        break;
    }

    syslog(LOG_CRIT, "%s, PFR - Major error: %s (0x%02X), Minor error: %s (0x%02X)", fru_str, major_str, major_err, minor_str, minor_err);
    return -1;
  }

  return 0;
}

int
pal_parse_oem_sel(uint8_t fru, uint8_t *sel, char *error_log)
{
  uint8_t mfg_id[] = {0x9c, 0x9c, 0x00};
  char temp_log[MAX_ERR_LOG_SIZE];

  error_log[0] = '\0';
  // Record Type: 0xC0 (OEM)
  if ((sel[2] == 0xC0) && !memcmp(&sel[7], mfg_id, sizeof(mfg_id))) {
    snprintf(temp_log, MAX_ERR_LOG_SIZE, "%s: Can not control SSD%d alert LED", sel[10] ? "Assert" : "Deassert", sel[11]);
    strcat(error_log, temp_log);
  }

  return 0;
}


int
set_pfr_i2c_filter(uint8_t slot_id, uint8_t value) {
  int ret;
  uint8_t tbuf[2] = {0};
  uint8_t tlen = 2;
  char path[128];
  int i2cfd = 0, retry=0;

  snprintf(path, sizeof(path), "/dev/i2c-%d", (slot_id + SLOT_BUS_BASE));
  i2cfd = open(path, O_RDWR);
  if ( i2cfd < 0 ) {
    syslog(LOG_WARNING, "%s() Failed to open %s", __func__, path);
    return -1;
  }
  retry = 0;
  tbuf[0] = PFR_I2C_FILTER_OFFSET;
  tbuf[1] = value;
  while (retry < RETRY_TIME) {
    ret = i2c_rdwr_msg_transfer(i2cfd, CPLD_ADDRESS, tbuf, tlen, NULL, 0);
    if ( ret < 0 ) {
      retry++;
      msleep(100);
    } else {
      break;
    }
  }
  if ( i2cfd > 0 ) close(i2cfd);

  if ( retry == RETRY_TIME ) return -1;

  return 0;

}

int
pal_check_sled_mgmt_cbl_id(uint8_t slot_id, uint8_t *cbl_val, bool log_evnt, uint8_t bmc_location) {
  enum {
    SLOT1_CBL = 0x03,
    SLOT2_CBL = 0x02,
    SLOT3_CBL = 0x01,
    SLOT4_CBL = 0x00,
  };
  enum {
    SLOT1_ID0_DETECT_BMC_N = 33,
    SLOT1_ID1_DETECT_BMC_N = 34,
    SLOT3_ID0_DETECT_BMC_N = 37,
    SLOT3_ID1_DETECT_BMC_N = 38,
  };
  const uint8_t mapping_tbl[4] = {SLOT1_CBL, SLOT2_CBL, SLOT3_CBL, SLOT4_CBL};
  const char *gpio_mgmt_cbl_tbl[] = {"SLOT%d_ID1_DETECT_BMC_N", "SLOT%d_ID0_DETECT_BMC_N"};
  const int num_of_mgmt_pins = ARRAY_SIZE(gpio_mgmt_cbl_tbl);
  int i = 0;
  int ret = 0;
  char dev[32] = {0};
  uint8_t val = 0;
  gpio_value_t gval;
  uint8_t gpio_vals = 0;
  bic_gpio_t gpio = {0};
  int i2cfd = 0;
  uint8_t bus = 0;
  uint8_t tbuf[1] = {0x06};
  uint8_t rbuf[1] = {0};
  uint8_t tlen = 1;
  uint8_t rlen = 1;
  uint8_t cpld_slot_cbl_val = 0;
  uint8_t slot_id_tmp = slot_id;

  if ( bmc_location == BB_BMC ) {
    // EVT BB_BMC HW not support cable management
    // just return present to skip checking
    cbl_val[0] = STATUS_PRSNT;
    return 0;
  } else if ( bmc_location == DVT_BB_BMC ) {
    //read GPIO vals
    for ( i = 0; i < num_of_mgmt_pins; i++ ) {
      snprintf(dev, sizeof(dev), gpio_mgmt_cbl_tbl[i], slot_id);
      if ( (gval = gpio_get_value_by_shadow(dev)) == GPIO_VALUE_INVALID ) {
        syslog(LOG_WARNING, "%s() Failed to read %s", __func__, dev);
      }
      val = (uint8_t)gval;
      gpio_vals |= (val << i);
    }
  } else {
    //NIC EXP
    //a bus starts from 4
    ret = fby3_common_get_bus_id(slot_id) + 4;
    if ( ret < 0 ) {
      syslog(LOG_WARNING, "%s() Cannot get the bus with fru%d", __func__, slot_id);
      return -1;
    }

    bus = (uint8_t)ret;
    i2cfd = i2c_cdev_slave_open(bus, CPLD_ADDRESS >> 1, I2C_SLAVE_FORCE_CLAIM);
    if ( i2cfd < 0 ) {
      syslog(LOG_WARNING, "%s() Failed to open bus %d. Err: %s", __func__, bus, strerror(errno));
      return -1;
    }

    //read 06h from SB CPLD
    ret = i2c_rdwr_msg_transfer(i2cfd, CPLD_ADDRESS, tbuf, tlen, rbuf, rlen);
    if ( i2cfd > 0 ) close(i2cfd);
    if ( ret < 0 ) {
      syslog(LOG_WARNING, "%s() Failed to do i2c_rdwr_msg_transfer, tlen=%d", __func__, tlen);
      return -1;
    }

    cpld_slot_cbl_val = rbuf[0];

    //read GPIO from BB BIC
    ret = bic_get_gpio(slot_id, &gpio, BB_BIC_INTF);
    if ( ret < 0 ) {
      printf("%s() bic_get_gpio returns %d\n", __func__, ret);
      return ret;
    }
    if (cpld_slot_cbl_val == SLOT1_CBL) {
      val = BIT_VALUE(gpio, SLOT1_ID1_DETECT_BMC_N);
      gpio_vals |= (val << 0);
      val = BIT_VALUE(gpio, SLOT1_ID0_DETECT_BMC_N);
      gpio_vals |= (val << 1);
    } else {
      val = BIT_VALUE(gpio, SLOT3_ID1_DETECT_BMC_N);
      gpio_vals |= (val << 0);
      val = BIT_VALUE(gpio, SLOT3_ID0_DETECT_BMC_N);
      gpio_vals |= (val << 1);
      slot_id_tmp = 3;
    }
  }

  bool vals_match = (bmc_location == DVT_BB_BMC) ? (gpio_vals == mapping_tbl[slot_id-1]):(gpio_vals == cpld_slot_cbl_val);
  if (vals_match == false) {
    for ( i = 0; i < (sizeof(mapping_tbl)/sizeof(uint8_t)); i++ ) {
      if(mapping_tbl[i] == gpio_vals) {
        break;
      }
    }
    if (log_evnt == true) {
      syslog(LOG_CRIT, "Abnormal - slot%d instead of slot%d", slot_id_tmp, (i+1));
    }
  }

  if ( cbl_val != NULL ) {
    cbl_val[0] = (vals_match == false)?STATUS_ABNORMAL:STATUS_PRSNT;
    if (cbl_val[0] == STATUS_ABNORMAL) {
      cbl_val[1] = slot_id_tmp << 4 | (i+1);
    } else {
      cbl_val[1] = 0x00;
    }
  }
  return ret;
}

int pal_set_bios_cap_fw_ver(uint8_t slot, uint8_t *req_data, uint8_t req_len, uint8_t *res_data, uint8_t *res_len) {
  int i2cfd = 0, ret = 0;
  uint8_t status;
  uint8_t bus;
  uint8_t retry = 0;
  uint32_t ver_reg = BIOS_CAP_STAG_MAILBOX;
  uint8_t tbuf[18] = {0};
  uint8_t tlen = BIOS_CAP_VER_LEN;

  ret = fby3_common_check_slot_id(slot);
  if (ret < 0 ) {
    ret = PAL_ENOTSUP;
    goto error_exit;
  }

  ret = pal_is_fru_prsnt(slot, &status);
  if ( ret < 0 || status == 0 ) {
    ret = PAL_ENOTREADY;
    goto error_exit;
  }

  ret = fby3_common_get_bus_id(slot) + 4;
  if ( ret < 0 ) {
    syslog(LOG_WARNING, "%s() Cannot get the bus with fru%d", __func__, slot);
    goto error_exit;
  }

  bus = (uint8_t)ret;
  i2cfd = i2c_cdev_slave_open(bus, CPLD_INTENT_CTRL_ADDR >> 1, I2C_SLAVE_FORCE_CLAIM);
  if ( i2cfd < 0) {
    printf("Failed to open bus %d. Err: %s\n", bus, strerror(errno));
    goto error_exit;
  }

  memcpy(tbuf, (uint8_t *)&ver_reg, 1);
  memcpy(&tbuf[1], req_data, tlen);
  tlen = tlen + 1;

  while (retry < MAX_READ_RETRY) {
    ret = i2c_rdwr_msg_transfer(i2cfd, CPLD_INTENT_CTRL_ADDR, tbuf, tlen, NULL, 0);
    if ( ret < 0 ) {
      retry++;
      sleep(1);
    } else {
      break;
    }
  }
  if (retry == MAX_READ_RETRY) {
    syslog(LOG_WARNING, "%s() Failed to do i2c_rdwr_msg_transfer, tlen=%d", __func__, tlen);
  }

error_exit:
  if ( i2cfd > 0 ) close(i2cfd);
  *res_len = 0;
  return ret;
}

static int
pal_get_cpld_ver(uint8_t fru, uint8_t* data_buf, uint8_t* data_len) {
  int ret = 0;
  const uint8_t cpld_addr = 0x80;
  uint8_t tbuf[4] = {0x00, 0x20, 0x00, 0x28};
  uint8_t rbuf[4] = {0x00};
  uint8_t tlen = 4;
  uint8_t rlen = 4;
  uint8_t i2c_bus = 0;
  int i2cfd = 0;

  switch (fru){
  case FRU_SLOT1:
  case FRU_SLOT2:
  case FRU_SLOT3:
  case FRU_SLOT4:
    i2c_bus = fby3_common_get_bus_id(fru) + 4;
    break;
  case FRU_BB:
    i2c_bus = 12;
    break;
  default:
    return -1;
  }

  ret = i2c_cdev_slave_open(i2c_bus, cpld_addr >> 1, I2C_SLAVE_FORCE_CLAIM);
  if (ret < 0) {
    syslog(LOG_WARNING, "Failed to open bus 12");
    return -1;
  }
  i2cfd = ret;
  ret = i2c_rdwr_msg_transfer(i2cfd, cpld_addr, tbuf, tlen, rbuf, rlen);
  if ( i2cfd > 0 )
    close(i2cfd);
  if (ret < 0) {
    syslog(LOG_WARNING, "%s() Failed to do i2c_rdwr_msg_transfer to slave@0x%02X on bus %d", __func__, cpld_addr, i2c_bus);
    return -1;
  }

  memcpy(data_buf, rbuf, rlen);
  if (data_len != NULL) {
    *data_len = rlen;
  }

  return 0;
}

int
pal_get_bb_fw_info(unsigned char target, char* ver_str) {
  char key[MAX_KEY_LEN] = {0};
  char value[MAX_VALUE_LEN] = {0};

  if (target == FW_CPLD) {
    uint8_t data_buf[4] = {0};
    uint8_t data_len;

    sprintf(key, "bb_cpld_ver");
    // kv cache exist
    if (kv_get(key, value, NULL, 0) == 0) {
      memcpy(ver_str, value, 8);
      return 0;
    }

    // get version by i2c
    if (pal_get_cpld_ver(FRU_BB, data_buf, &data_len) < 0) {
      return -1;
    }

    // write cache
    sprintf(value, "%02X%02X%02X%02X", data_buf[3], data_buf[2], data_buf[1], data_buf[0]);
    kv_set(key, value, 8, 0);
    memcpy(ver_str, value, 8);
    return 0;
  }

  // unsupported target
  return -1;
}

int
pal_get_fw_info(uint8_t fru, unsigned char target, unsigned char* res, unsigned char* res_len)
{
  uint8_t bmc_location = 0;
  uint8_t config_status;
  int ret = PAL_ENOTSUP;
  uint8_t tmp_cpld_swap[4] = {0};
  uint8_t type_2ou = UNKNOWN_BOARD;

  ret = fby3_common_get_bmc_location(&bmc_location);
  if (ret < 0) {
    syslog(LOG_ERR, "%s() Cannot get the location of BMC", __func__);
    goto error_exit;
  }

  if (target == FW_CPLD) {
    ret = pal_get_cpld_ver(fru, res, res_len);
    if (ret < 0) {
      goto error_exit;
    }
  } else if(target == FW_BIOS) {
    ret = pal_get_sysfw_ver(fru, res);
    if ( ret < 0 ) {
      syslog(LOG_WARNING, "%s() Failed to get sysfw ver", __func__);
      goto error_exit;
    }
  } else if(target == FW_VR) {
    // TODO
    goto not_support;
  } else {
    switch(target) {
    case FW_1OU_BIC:
    case FW_1OU_BIC_BOOTLOADER:
    case FW_1OU_CPLD:
      ret = (pal_is_fw_update_ongoing(fru) == false) ? bic_is_m2_exp_prsnt(fru):bic_is_m2_exp_prsnt_cache(fru);
      if (ret < 0) {
        syslog(LOG_WARNING, "%s() Failed to get 1ou & 2ou present status", __func__);
        goto error_exit;
      }
      config_status = (uint8_t) ret;
      if (!((bmc_location == BB_BMC || bmc_location == DVT_BB_BMC) && ((config_status & PRESENT_1OU) == PRESENT_1OU))) {
        goto not_support;
      }
      break;
    case FW_2OU_BIC:
    case FW_2OU_BIC_BOOTLOADER:
    case FW_2OU_CPLD:
      ret = (pal_is_fw_update_ongoing(fru) == false) ? bic_is_m2_exp_prsnt(fru):bic_is_m2_exp_prsnt_cache(fru);
      if (ret < 0) {
        syslog(LOG_WARNING, "%s() Failed to get 1ou & 2ou present status", __func__);
        goto error_exit;
      }
      config_status = (uint8_t) ret;
      if ( fby3_common_get_2ou_board_type(fru, &type_2ou) < 0 ) {
        syslog(LOG_WARNING, "%s() Failed to get 2OU board type\n", __func__);
      }
      if (!((config_status & PRESENT_2OU) == PRESENT_2OU)) {
        goto not_support;
      } else if (type_2ou == DP_RISER_BOARD) {
        goto not_support;
      }
      break;
    case FW_BB_BIC:
    case FW_BB_BIC_BOOTLOADER:
    case FW_BB_CPLD:
      if(bmc_location != NIC_BMC) {
        goto not_support;
      }
      break;
    case FW_BIOS_CAPSULE:
    case FW_CPLD_CAPSULE:
    case FW_BIOS_RCVY_CAPSULE:
    case FW_CPLD_RCVY_CAPSULE:
      // TODO
      goto not_support;
    default:
      if (target >= FW_COMPONENT_LAST_ID)
        goto not_support;
      break;
    }

    ret = bic_get_fw_ver(fru, target, res);
    if (ret != BIC_STATUS_SUCCESS) {
      syslog(LOG_WARNING, "%s() bic_get_fw_ver returns %d\n", __func__, ret);
      goto error_exit;
    }
  }

  switch(target) {
  case FW_CPLD:
  case FW_1OU_CPLD:
  case FW_2OU_CPLD:
  case FW_BB_CPLD:
    tmp_cpld_swap[0] = res[3];
    tmp_cpld_swap[1] = res[2];
    tmp_cpld_swap[2] = res[1];
    tmp_cpld_swap[3] = res[0];
    memcpy(res, tmp_cpld_swap, 4);
    *res_len = 4;
    break;
  case FW_ME:
    *res_len = 5;
    break;
  case FW_BIC:
  case FW_1OU_BIC:
  case FW_2OU_BIC:
  case FW_BB_BIC:
  case FW_BIC_BOOTLOADER:
  case FW_1OU_BIC_BOOTLOADER:
  case FW_2OU_BIC_BOOTLOADER:
  case FW_BB_BIC_BOOTLOADER:
    *res_len = 2;
    break;
  case FW_BIOS:
    *res_len = 16;
    break;
  default:
    goto not_support;
  }

  return PAL_EOK;

not_support:
  return PAL_ENOTSUP;
error_exit:
  return ret;
}

int
pal_set_nic_perst(uint8_t fru, uint8_t val) {
  int i2cfd = 0;
  int ret = 0;
  char path[32] = {0};
  uint8_t bmc_location = 0;
  uint8_t bus = 0;
  uint8_t tbuf[2] = {NIC_CARD_PERST_CTRL, val};
  uint8_t tlen = 2;

  ret = fby3_common_get_bmc_location(&bmc_location);
  if ( ret < 0 ) {
    syslog(LOG_WARNING, "%s() Cannot get the location of BMC", __func__);
    goto error_exit;
  }

  if ( bmc_location == BB_BMC || bmc_location == DVT_BB_BMC ) {
    return 0;
  }

  bus= (uint8_t)NIC_CPLD_BUS;
  snprintf(path, sizeof(path), "/dev/i2c-%d", bus);

  i2cfd = open(path, O_RDWR);
  if ( i2cfd < 0 ) {
    syslog(LOG_WARNING, "%s() Failed to open %s", __func__, path);
    goto error_exit;
  }

  ret = i2c_rdwr_msg_transfer(i2cfd, CPLD_ADDRESS, tbuf, tlen, NULL, 0);
  if ( ret < 0 ) {
    syslog(LOG_WARNING, "%s() Failed to do i2c_rdwr_msg_transfer, tlen=%d", __func__, tlen);
    goto error_exit;
  }

error_exit:
  if ( i2cfd > 0 ) {
    close(i2cfd);
  }

  return ret;
}

static const char *sock_path_asd_bic[MAX_NODES+1] = {
  "",
  SOCK_PATH_ASD_BIC "_1",
  SOCK_PATH_ASD_BIC "_2",
  SOCK_PATH_ASD_BIC "_3",
  SOCK_PATH_ASD_BIC "_4"
};

static const char *sock_path_jtag_msg[MAX_NODES+1] = {
  "",
  SOCK_PATH_JTAG_MSG "_1",
  SOCK_PATH_JTAG_MSG "_2",
  SOCK_PATH_JTAG_MSG "_3",
  SOCK_PATH_JTAG_MSG "_4"
};

static void
cpu_pwrgd_handler(uint8_t slot)
{
  long current_time = 0;
  long time_cpu_pwrgd = 0;
  long time_caterr = 0;
  char key[MAX_KEY_LEN] = {0};
  char value[MAX_VALUE_LEN] = {0};
  struct timespec ts = {0};

  // get last pwrgd time and update it with current time
  clock_gettime(CLOCK_REALTIME, &ts);
  current_time = ts.tv_sec;

  snprintf(key, sizeof(key), KEY_CPU_PWRGD_TIMESTAMP, slot);
  if(kv_get(key, value, NULL, 0) == 0) {
    time_cpu_pwrgd = strtol(value, NULL, 10);
  }
  snprintf(value, sizeof(value), "%lld", ts.tv_sec);
  if (kv_set(key, value, 0, 0) < 0) {
    syslog(LOG_WARNING, "%s() cache_set key = %s, value = %s failed.\n", __func__, key, value);
  }

  // get last caterr time
  snprintf(key, sizeof(key), KEY_CATERR_TIMESTAMP, slot);
  if(kv_get(key, value, NULL, 0) == 0) {
    time_caterr = strtol(value, NULL, 10);
  }

  // reset failure count if no CATERR within 15 seccond
  if (time_cpu_pwrgd && time_caterr && (time_cpu_pwrgd > time_caterr)) {
    if ((current_time - time_cpu_pwrgd > 15)) {
      snprintf(key, sizeof(key), KEY_HOST_FAILURE_COUNT, slot);
      if (kv_set(key, "0", 0, 0) < 0) {
        syslog(LOG_WARNING, "%s() cache_set key = %s, value = %s failed.\n", __func__, key, value);
      }
    }
  }
}

int
pal_handle_oem_1s_intr(uint8_t slot, uint8_t *data)
{
  int sock;
  int err;
  struct sockaddr_un server;

  if (*data == PWRGD_CPU_LVC3_R) {
    cpu_pwrgd_handler(slot);
  }

  if (access(sock_path_asd_bic[slot], F_OK) == -1) {
    // SOCK_PATH_ASD_BIC doesn't exist, means ASD daemon for this
    // slot is not running, exit
    return 0;
  }

  sock = socket(AF_UNIX, SOCK_STREAM, 0);
  if (sock < 0) {
    err = errno;
    syslog(LOG_ERR, "%s failed open socket (errno=%d)", __FUNCTION__, err);
    return -1;
  }

  server.sun_family = AF_UNIX;
  strcpy(server.sun_path, sock_path_asd_bic[slot]);

  if (connect(sock, (struct sockaddr *) &server, sizeof(struct sockaddr_un)) < 0) {
    err = errno;
    close(sock);
    syslog(LOG_ERR, "%s failed connecting stream socket (errno=%d), %s",
           __FUNCTION__, err, server.sun_path);
    return -1;
  }
  if (write(sock, data, 2) < 0) {
    err = errno;
    syslog(LOG_ERR, "%s error writing on stream sockets (errno=%d)",
           __FUNCTION__, err);
  }
  close(sock);

  return 0;
}

int
pal_handle_oem_1s_asd_msg_in(uint8_t slot, uint8_t *data, uint8_t data_len)
{
  int sock;
  int err;
  struct sockaddr_un server;

  if (access(sock_path_jtag_msg[slot], F_OK) == -1) {
    // SOCK_PATH_JTAG_MSG doesn't exist, means ASD daemon for this
    // slot is not running, exit
    return 0;
  }

  sock = socket(AF_UNIX, SOCK_STREAM, 0);
  if (sock < 0) {
    err = errno;
    syslog(LOG_ERR, "%s failed open socket (errno=%d)", __FUNCTION__, err);
    return -1;
  }

  server.sun_family = AF_UNIX;
  strcpy(server.sun_path, sock_path_jtag_msg[slot]);

  if (connect(sock, (struct sockaddr *)&server, sizeof(struct sockaddr_un)) < 0) {
    err = errno;
    close(sock);
    syslog(LOG_ERR, "%s failed connecting stream socket (errno=%d), %s",
           __FUNCTION__, err, server.sun_path);
    return -1;
  }

  if (write(sock, data, data_len) < 0) {
    err = errno;
    syslog(LOG_ERR, "%s error writing on stream sockets (errno=%d)", __FUNCTION__, err);
  }

  close(sock);
  return 0;
}

// It's called by fpc-util and front-paneld
int
pal_sb_set_amber_led(uint8_t fru, bool led_on, uint8_t led_mode) {
  int ret = 0;
  int i2cfd = -1;
  uint8_t bus = 0;

  ret = fby3_common_get_bus_id(fru);
  if ( ret < 0 ) {
    printf("%s() Couldn't get the bus id of fru%d\n", __func__, fru);
    goto err_exit;
  }
  bus = (uint8_t)ret + 4;

  i2cfd = i2c_cdev_slave_open(bus, CPLD_ADDRESS >> 1, I2C_SLAVE_FORCE_CLAIM);
  if ( i2cfd < 0 ) {
    printf("%s() Couldn't open i2c bus%d, err: %s\n", __func__, bus, strerror(errno));
    goto err_exit;
  }

  uint8_t tbuf[2] = {0x0, (led_on == true)?0x01:0x00};
  if ( led_mode == LED_LOCATE_MODE ) {
    /* LOCATE_MODE */
    // 0x0f 01h: off
    //      00h: on
    tbuf[0] = 0x0f;
    tbuf[1] = (led_on == true)?0x01:0x00;
  } else if ( led_mode == LED_CRIT_PWR_OFF_MODE || led_mode == LED_CRIT_PWR_ON_MODE ) {
    /* CRIT_MODE */
    // 0x12 02h: 900ms_on/100ms_off
    //      01h: 900ms_off/100ms_on
    //      00h: off
    tbuf[0] = 0x12;
    tbuf[1] = (led_on == false)?0x00:(led_mode == LED_CRIT_PWR_OFF_MODE)?0x01:0x02;
  } else {
    syslog(LOG_WARNING, "%s() fru:%d, led_on:%d, led_mode:%d\n", __func__, fru, led_on, led_mode);
  }

  ret = i2c_rdwr_msg_transfer(i2cfd, CPLD_ADDRESS, tbuf, 2, NULL, 0);
  if ( ret < 0 ) {
    printf("%s() Couldn't write data to addr %02X, err: %s\n",  __func__, CPLD_ADDRESS, strerror(errno));
  }

err_exit:
  if ( i2cfd > 0 ) close(i2cfd);
  return ret;
}

// IPMI chassis identification LED command
// ipmitool chassis identify [force|0]
//   force: turn on LED indefinitely
//       0: turn off LED
int
pal_set_slot_led(uint8_t slot, uint8_t *req_data, uint8_t req_len, uint8_t *res_data, uint8_t *res_len) {
  int rsp_cc = CC_UNSPECIFIED_ERROR;
  uint8_t *data = req_data;
  *res_len = 0;

   /* There are 2 option bytes for Chassis Identify Command
    * Byte 1 : Identify Interval in seconds. (Not support, OpenBMC only support turn off action)
    *          00h = Turn off Identify
    * Byte 2 : Force Identify On
    *          BIT0 : 1b = Turn on Identify indefinitely. This overrides the values in byte 1.
    *                 0b = Identify state driven according to byte 1.
    */
  if ( 5 == req_len ) {
    if ( GETBIT(*(data+1), 0) ) {
      //turn on
      rsp_cc = pal_sb_set_amber_led(slot, true, LED_LOCATE_MODE);
    } else if ( 0 == *data ) {
      //turn off
      rsp_cc = pal_sb_set_amber_led(slot, false, LED_LOCATE_MODE);
    } else {
      rsp_cc = CC_INVALID_PARAM;
    }
  } else if ( 4  == req_len ) {
    if (0 == *data) {
      //turn off
      rsp_cc = pal_sb_set_amber_led(slot, false, LED_LOCATE_MODE);
    } else {
      rsp_cc = CC_INVALID_PARAM;
    }
  }

  if ( rsp_cc < 0 ) rsp_cc = CC_UNSPECIFIED_ERROR;
  return rsp_cc;
}

int
pal_get_dev_info(uint8_t slot_id, uint8_t dev_id, uint8_t *nvme_ready, uint8_t *status, uint8_t *type) {
  return bic_get_dev_info(slot_id, dev_id, nvme_ready, status, type);
}

int
pal_check_slot_cpu_present(uint8_t slot_id) {
  int ret = 0;
  bic_gpio_t gpio = {0};

  ret = bic_get_gpio(slot_id, &gpio, NONE_INTF);
  if ( ret < 0 ) {
    printf("%s() bic_get_gpio returns %d\n", __func__, ret);
    return ret;
  }

  if (BIT_VALUE(gpio, FM_CPU_SKTOCC_LVT3_N)) {
    syslog(LOG_CRIT, "FRU: %d, CPU absence", slot_id);
  } else {
    syslog(LOG_CRIT, "FRU: %d, CPU presence", slot_id);
  }

  return ret;
}

int
pal_get_sensor_util_timeout(uint8_t fru) {
  switch (fru) {
    case FRU_SLOT1:
    case FRU_SLOT2:
    case FRU_SLOT3:
    case FRU_SLOT4:
    case FRU_2U:
    case FRU_CWC:
    case FRU_2U_TOP:
    case FRU_2U_BOT:
      return 10;
    case FRU_BMC:
      return 15;
    case FRU_NIC:
    default:
      return 4;
  }
}

// IPMI OEM Command
// netfn: NETFN_OEM_1S_REQ (0x38)
// command code: CMD_OEM_1S_GET_SYS_FW_VER (0x40)
int
pal_get_fw_ver(uint8_t slot, uint8_t *req_data, uint8_t *res_data, uint8_t *res_len) {
  int ret = 0;
  uint8_t type_2ou = 0;
  uint8_t fru = 0;
  uint8_t comp = 0;
  FILE* fp = NULL;
  char buf[MAX_FW_VER_LEN] = {0};
  // To keep the format consistent with fw-util, get version from fw-util directly.
  static const char* cmd_table[IPMI_GET_VER_FRU_NUM][IPMI_GET_VER_MAX_COMP] = {
    // BMC
    {
      "/usr/bin/fw-util bmc --version bmc | grep 'Version:' | awk '{print $NF}'",
      "/usr/bin/fw-util bmc --version rom | grep 'Version:' | awk '{print $NF}'",
      "/usr/bin/fw-util bmc --version cpld | awk '{print $NF}'",
      "/usr/bin/fw-util bmc --version fscd | awk '{print $NF}'",
      "/usr/bin/fw-util bmc --version tpm | awk '{print $NF}'",
      NULL,
      NULL,
      NULL,
      NULL
    },
    // NIC
    {
      "/usr/bin/fw-util nic --version | awk '{print $NF}'",
      NULL,
      NULL,
      NULL,
      NULL,
      NULL,
      NULL,
      NULL,
      NULL
    },
    // Base board
    {
      "/usr/bin/fw-util slot1 --version bb_bic | awk '{print $NF}'",
      "/usr/bin/fw-util slot1 --version bb_bicbl | awk '{print $NF}'",
      "/usr/bin/fw-util slot1 --version bb_cpld | awk '{print $NF}'",
      NULL,
      NULL,
      NULL,
      NULL,
      NULL,
      NULL
    },
    // Server board
    {
      "/usr/bin/fw-util slot1 --version bic | awk '{print $NF}'",
      "/usr/bin/fw-util slot1 --version bicbl | awk '{print $NF}'",
      "/usr/bin/fw-util slot1 --version bios | awk '{print $NF}'",
      "/usr/bin/fw-util slot1 --version cpld | awk '{print $NF}'",
      "/usr/bin/fw-util slot1 --version me | awk '{print $NF}'",
      "/usr/bin/fw-util slot1 --version vr | grep 'VCCIN_VSA' | awk '{print $5}' | cut -c -8",
      "/usr/bin/fw-util slot1 --version vr | grep 'VCCIO' | awk '{print $5}' | cut -c -8",
      "/usr/bin/fw-util slot1 --version vr | grep 'VDDQ_ABC' | awk '{print $5}' | cut -c -8",
      "/usr/bin/fw-util slot1 --version vr | grep 'VDDQ_DEF' | awk '{print $5}' | cut -c -8"
    },
    // 2OU
    {
      "/usr/bin/fw-util slot1 --version 2ou_bic | awk '{print $NF}'",
      "/usr/bin/fw-util slot1 --version 2ou_bicbl | awk '{print $NF}'",
      NULL,
      NULL,
      NULL,
      NULL,
      NULL,
      NULL,
      NULL
    }
  };

  *res_len = 0;
  if (req_data == NULL) {
    syslog(LOG_ERR, "%s(): IPMI request failed due to NULL parameter: *req_data", __func__);
    return CC_INVALID_PARAM;
  }
  if (res_data == NULL) {
    syslog(LOG_ERR, "%s(): IPMI request failed due to NULL parameter: *res_data", __func__);
    return CC_INVALID_PARAM;
  }
  if (res_len == NULL) {
    syslog(LOG_ERR, "%s(): IPMI request failed due to NULL parameter: *res_len", __func__);
    return CC_INVALID_PARAM;
  }

  ret = fby3_common_get_2ou_board_type(slot, &type_2ou);
  if (ret < 0) {
    syslog(LOG_ERR, "%s(): Not support CMD_OEM_1S_GET_SYS_FW_VER IPMI command due to get 2ou board type failed", __func__);
    return CC_UNSPECIFIED_ERROR;
  }
  if (type_2ou != E1S_BOARD) {
    syslog(LOG_ERR, "%s(): CMD_OEM_1S_GET_SYS_FW_VER IPMI command only support by Sierra Point system", __func__);
    return CC_NOT_SUPP_IN_CURR_STATE;
  }

  fru = ((GET_FW_VER_REQ*)req_data)->fru;
  comp = ((GET_FW_VER_REQ*)req_data)->component;
  if ((fru >= IPMI_GET_VER_FRU_NUM) || (comp >= IPMI_GET_VER_MAX_COMP) || (cmd_table[fru][comp] == NULL)) {
    syslog(LOG_ERR, "%s(): wrong FRU or component, fru = %x, comp = %x", __func__, fru, comp);
    return CC_PARAM_OUT_OF_RANGE;
  }

  if((fp = popen(cmd_table[fru][comp], "r")) == NULL) {
    syslog(LOG_ERR, "%s(): fail to send command: %s, errno: %s", __func__, cmd_table[fru][comp], strerror(errno));
    return CC_UNSPECIFIED_ERROR;
  }

  memset(buf, 0, sizeof(buf));
  if(fgets(buf, sizeof(buf), fp) != NULL) {
    *res_len = strlen(buf);
    strncpy((char*)res_data, buf, MAX_FW_VER_LEN);
  }

  if (fp != NULL) {
    pclose(fp);
  }

  return CC_SUCCESS;
}

int
pal_gpv3_mux_select(uint8_t slot_id, uint8_t dev_id) {
  if ( slot_id == FRU_2U_TOP ) {
    if ( bic_mux_select(FRU_SLOT1, get_gpv3_bus_number(dev_id), dev_id, RREXP_BIC_INTF1) < 0 ) {
      printf("* Failed to select MUX from top GPV3\n");
      return BIC_STATUS_FAILURE;
    }
  } else if ( slot_id == FRU_2U_BOT ) {
    if ( bic_mux_select(FRU_SLOT1, get_gpv3_bus_number(dev_id), dev_id, RREXP_BIC_INTF2) < 0 ) {
      printf("* Failed to select MUX from bot GPV3\n");
      return BIC_STATUS_FAILURE;
    }
  } else {
    if ( bic_mux_select(slot_id, get_gpv3_bus_number(dev_id), dev_id, REXP_BIC_INTF) < 0 ) {
      printf("* Failed to select MUX\n");
      return BIC_STATUS_FAILURE;
    }
  }
  return BIC_STATUS_SUCCESS;
}

bool
pal_is_aggregate_snr_valid(uint8_t snr_num) {
  char sys_conf[MAX_VALUE_LEN] = {0};

  switch(snr_num) {
    // In type 8 system, if one fan fail, show NA in airflow reading.
    case AGGREGATE_SENSOR_SYSTEM_AIRFLOW:
      memset(sys_conf, 0, sizeof(sys_conf));
      if (kv_get("sled_system_conf", sys_conf, NULL, KV_FPERSIST) < 0) {
        syslog(LOG_WARNING, "%s() Failed to read sled_system_conf", __func__);
        return true;
      }
      if (strcmp(sys_conf, "Type_8") != 0) {
        return true;
      }
      if (access(FAN_FAIL_RECORD_PATH, F_OK) == 0) {
        return false;
      }
      break;
    default:
      return true;
  }

  return true;
}

bool pal_check_fava_ssd_exist(uint8_t ssd_num) {
  uint8_t slot = FRU_SLOT1;
  uint8_t tbuf[16] = {0xb8, 0x40, 0x57, 0x01 ,0x00 ,0x30 ,0x06 ,0x05 ,0x61 ,0x00 ,0x00 ,0x00 ,0x80 ,0x01};
  uint8_t rbuf[10] = {0x00};
  uint8_t tlen = 14;
  uint8_t rlen = 10;
  uint16_t read_vid = 0x0, read_did = 0x0;
  int ret = 0;
  if (ssd_num == 1) {
    tbuf[12] = 0x80; //Bus 0x18
    tbuf[13] = 0x01;
  } else if (ssd_num == 2) {
    tbuf[12] = 0x90; //Bus 0x19
    tbuf[13] = 0x01;
  } else {
    return false;
  }
  memset(rbuf,0,rlen);
  ret = bic_me_xmit(slot, tbuf, tlen, rbuf, &rlen);
  if (ret == 0) {
    read_vid = rbuf[6] << 8 | rbuf[5];
    read_did = rbuf[8] << 8 | rbuf[7];
    if ((read_vid == FAVA_PM9A3_VID) && (read_did == FAVA_PM9A3_DID)) {
      syslog(LOG_WARNING, "%s() FAVA SSD%u PM9A3 vid:0x%X did:0x%X", __func__,ssd_num,read_vid,read_did);
      return true;
    } else {
      syslog(LOG_WARNING, "%s() FAVA SSD%u vid:0x%X did:0x%X", __func__,ssd_num,read_vid,read_did);
    }
  } else {
    syslog(LOG_WARNING, "%s() FAVA SSD%u no response", __func__,ssd_num);
  }
  return false;
}

static int reload_fan_table(const char* sysconfig, const char* fan_table) {
  int ret = -1;
  char cmd[MAX_CMD_LEN] = {0};
  char value[MAX_VALUE_LEN] = {0};

  if (sysconfig == NULL || fan_table == NULL) {
    syslog(LOG_WARNING, "%s() Invalid input parameter", __func__);
    return ret;
  }

  ret = kv_get("sled_system_conf", value, NULL, KV_FPERSIST);
  if (ret == 0 && (strncmp(value, sysconfig, sizeof(value)) == 0)) {
    return 0;
  }

  syslog(LOG_WARNING, "%s() Reload fan table: %s", __func__, fan_table);

  snprintf(cmd, sizeof(cmd), "ln -sf %s %s", fan_table, DEFAULT_FSC_CFG_PATH);
  ret = system(cmd);
  if (ret != 0) {
    syslog(LOG_WARNING, "%s() can not import table: %s", __func__, fan_table);
    return ret;
  }

  if (kv_set("sled_system_conf", sysconfig, 0, KV_FPERSIST) < 0) {
    syslog(LOG_WARNING, "%s() Failed to set sled_system_conf\n", __func__);
  }

  ret = system("/usr/bin/sv restart fscd");
  if (ret != 0) {
    syslog(LOG_WARNING, "%s() can not restart fscd", __func__);
    return ret;
  }

  ret = system("/usr/bin/sv restart sensord");
  if (ret != 0) {
    syslog(LOG_WARNING, "%s() can not restart sensord", __func__);
    return ret;
  }

  return ret;
}

int pal_dp_fan_table_check(void) {
  //DP only slot1
  uint8_t slot = FRU_SLOT1;
  int ret = 0, config_status = 0;
  uint8_t bmc_location = 0;
  uint8_t type_2ou = UNKNOWN_BOARD;
  uint8_t tbuf[16] = {0xb8, 0x40, 0x57, 0x01 ,0x00 ,0x30 ,0x06 ,0x05 ,0x61 ,0x00 ,0x00 ,0x00 ,0x60 ,0x01};
  uint8_t rbuf[10] = {0x00};
  uint8_t tlen = 14;
  uint8_t rlen = 10;
  uint16_t read_vid = 0x0, read_did = 0x0;
  bool fava_ssd1 = false, fava_ssd2 = false;
  uint8_t ssd_num = 1;

  ret = fby3_common_get_bmc_location(&bmc_location);
  if (ret < 0) {
    syslog(LOG_ERR, "%s() Cannot get the location of BMC", __func__);
    return -1;
  }

  config_status = bic_is_m2_exp_prsnt(slot);
  if ( (config_status & PRESENT_2OU) == PRESENT_2OU ) {
    //check if GPv3 is installed
    if ( fby3_common_get_2ou_board_type(slot, &type_2ou) < 0 ) {
      syslog(LOG_WARNING, "%s() Failed to get 2OU board type\n", __func__);
    }
  }

  if ( (bmc_location == BB_BMC) || (bmc_location == DVT_BB_BMC) ) {
    if (type_2ou == DP_RISER_BOARD) {
      ret = bic_me_xmit(slot, tbuf, tlen, rbuf, &rlen);
      if (ret == 0) {
        read_vid = rbuf[6] << 8 | rbuf[5];
        read_did = rbuf[8] << 8 | rbuf[7];
        if (((read_vid == HBA_M_VID) && (read_did == HBA_M_DID)) || ((read_vid == HBA_B_VID) && (read_did == HBA_B_DID))) {
          if ((ret = reload_fan_table("Type_DPB", DP_HBA_FAN_TBL_PATH)) != 0) {
            return ret;
          }
        } else {
          if ((ret = reload_fan_table("Type_DP", DP_FAN_TBL_PATH)) != 0) {
            return ret;
          }
        }
      }

      ssd_num = 1;
      fava_ssd1 = pal_check_fava_ssd_exist (ssd_num);

      ssd_num = 2;
      fava_ssd2 = pal_check_fava_ssd_exist (ssd_num);

      if (fava_ssd1 || fava_ssd2) { // If one of FAVA SSD exists
        syslog(LOG_WARNING, "%s() start to import DP-FAVA fan table", __func__);
        if ((ret = reload_fan_table("Type_DPF", DP_FAVA_FAN_TBL_PATH)) != 0) {
          return ret;
        }
      }
    }
  }

  return ret;
}

int pal_is_cwc(void) {
  static uint8_t bmc_location = UNKNOWN_BOARD;
  static uint8_t board_type = UNKNOWN_BOARD;

  if (board_type == UNKNOWN_BOARD) {
    if (fby3_common_get_bmc_location(&bmc_location) < 0) {
      syslog(LOG_ERR, "%s() Cannot get the location of BMC", __func__);
      return PAL_ENOTSUP;
    }

    if ( bmc_location == NIC_BMC ) {
      if (fby3_common_get_2ou_board_type(FRU_SLOT1, &board_type) < 0) {
        syslog(LOG_WARNING, "%s() Failed to get 2OU board type\n", __func__);
        return PAL_ENOTSUP;
      }
    }
  }

  return board_type == CWC_MCHP_BOARD ? PAL_EOK : PAL_ENOTSUP;
}

int pal_get_asd_sw_status(uint8_t fru) {
  int intf = 0, slot = 0;
  bic_gpio_t gpio = {0};

  switch (fru) {
    case FRU_2U_TOP:
      intf = RREXP_BIC_INTF1;
      slot = FRU_SLOT1;
      break;
    case FRU_2U_BOT:
      intf = RREXP_BIC_INTF2;
      slot = FRU_SLOT1;
      break;
    default:
      return 0;
  }

  if (bic_get_gpio(slot, &gpio, intf) == 0) {
    if (gpio.gpio[3] & 0x01) {
      return 1;
    }
  }
  return 0;
}

int pal_is_exp(void) {
  return pal_is_cwc();
}

int pal_get_fru_slot(uint8_t fru, uint8_t *slot) {
  switch (fru) {
    case FRU_2U:
    case FRU_CWC:
    case FRU_2U_TOP:
    case FRU_2U_BOT:
      *slot = FRU_SLOT1;
      break;
    case FRU_2U_SLOT3:
      *slot = FRU_SLOT3;
      break;
    default:
      *slot = fru;
      break;
  }
  return PAL_EOK;
}

int pal_get_root_fru(uint8_t fru, uint8_t *root) {
  return pal_get_fru_slot(fru, root);
}

int pal_get_2ou_board_type(uint8_t fru, uint8_t *type_2ou) {
  int ret = 0;
  uint8_t bmc_location = 0;
  uint8_t slot = 0;

  ret = fby3_common_get_bmc_location(&bmc_location);
  if ( ret < 0 ) {
    syslog(LOG_WARNING, "%s() Cannot get the location of BMC\n", __func__);
    return ret;
  }
  if ( bmc_location == NIC_BMC ) {
    ret = pal_get_fru_slot(fru, &slot);
    if ( ret < 0 ) {
      syslog(LOG_WARNING, "%s() Failed to get slot of fru\n",__func__);
      return ret;
    }

    ret = fby3_common_get_2ou_board_type(slot, type_2ou);
    if ( ret < 0 ) {
      syslog(LOG_WARNING, "%s() Failed to get 2ou board type\n",__func__);
      return ret;
    }
  } else {
    *type_2ou = UNKNOWN_BOARD;
    return ret;
  }
  return ret;
}

int pal_is_sensor_num_exceed(uint8_t sensor_num) {
  if (sensor_num > MAX_SENSOR_NUM) {
    syslog(LOG_CRIT, "Amount of sensors is more than Maximum value");
    return PAL_ENOTSUP;
  } else {
    return PAL_EOK;
  }
  return PAL_EOK;
}

int pal_is_pesw_power_on(uint8_t fru, uint8_t *status) {
  enum {
    GP3_GPIO_PWRGD_P0V84 = 43,
    GP3_GPIO_PWRGD_P1V8  = 46,
    CWC_GPIO_PWRGD_P0V84 = 47,
    CWC_GPIO_PWRGD_P1V8  = 48,
  };
  int ret = 0;
  uint8_t slot = 0;
  uint8_t intf = 0;
  bic_gpio_t gpio = {0};

  ret = pal_get_fru_slot(fru, &slot);
  if ( ret < 0 ) {
    syslog(LOG_WARNING, "%s() Failed to get slot of fru\n",__func__);
    return ret;
  }

  switch (fru) {
    case FRU_CWC:
      intf = REXP_BIC_INTF;
      break;
    case FRU_2U_TOP:
      intf = RREXP_BIC_INTF1;
      break;
    case FRU_2U_BOT:
      intf = RREXP_BIC_INTF2;
      break;
    default:
      return -1;
  }

  ret = bic_get_gpio(slot, &gpio, intf);
  if ( ret < 0 ) {
    printf("%s() bic_get_gpio returns %d\n", __func__, ret);
    return ret;
  }

  switch (fru) {
    case FRU_CWC:
      if (BIT_VALUE(gpio, CWC_GPIO_PWRGD_P0V84) == VALUE_HIGH && BIT_VALUE(gpio, CWC_GPIO_PWRGD_P1V8) == VALUE_HIGH) {
        *status = 1;
      } else {
        *status = 0;
      }
      break;
    case FRU_2U_TOP:
    case FRU_2U_BOT:
      if (BIT_VALUE(gpio, GP3_GPIO_PWRGD_P0V84) == VALUE_HIGH && BIT_VALUE(gpio, GP3_GPIO_PWRGD_P1V8) == VALUE_HIGH) {
        *status = 1;
      } else {
        *status = 0;
      }
      break;
    default:
      return -1;
  }
  return ret;
}

int
pal_ipmb_processing(int bus, void *buf, uint16_t size) {
  char key[MAX_KEY_LEN];
  char value[MAX_VALUE_LEN];
  struct timespec ts;
  static time_t last_time = 0;

  if ((bus == 9) && (((uint8_t *)buf)[0] == (BMC_SLAVE_ADDR<<1))) {  // OCP LCD debug card
    clock_gettime(CLOCK_MONOTONIC, &ts);
    if (ts.tv_sec >= (last_time + 5)) {
      last_time = ts.tv_sec;
      ts.tv_sec += 20;

      sprintf(key, "ocpdbg_lcd");
      sprintf(value, "%lld", ts.tv_sec);
      kv_set(key, value, 0, 0);
    }
  }

  return 0;
}

int
pal_is_mcu_ready(uint8_t bus) {
  char key[MAX_KEY_LEN];
  char value[MAX_VALUE_LEN] = {0};
  struct timespec ts;

  sprintf(key, "ocpdbg_lcd");
  if (kv_get(key, value, NULL, 0)) {
    return false;
  }

  clock_gettime(CLOCK_MONOTONIC, &ts);
  if (strtoul(value, NULL, 10) > ts.tv_sec) {
     return true;
  }

  return false;
}

void
pal_get_fru_intf(uint8_t fru, uint8_t *intf) {
  switch (fru) {
    case FRU_BB:
      *intf = BB_BIC_INTF;
      break;
    case FRU_2U:
    case FRU_2U_SLOT3:
    case FRU_CWC:
      *intf = REXP_BIC_INTF;
      break;
    case FRU_2U_TOP:
      *intf = RREXP_BIC_INTF1;
      break;
    case FRU_2U_BOT:
      *intf = RREXP_BIC_INTF2;
      break;
    // Add yourself
    default:
      *intf = NONE_INTF;
      break;
  }
  return;
}
