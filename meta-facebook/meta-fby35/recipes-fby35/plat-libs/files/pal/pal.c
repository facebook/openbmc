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
#include <openbmc/misc-utils.h>
#include <openbmc/obmc-i2c.h>
#include <openbmc/ncsi.h>
#include <openbmc/nl-wrapper.h>
#include <sys/socket.h>
#include <linux/netlink.h>
#include <sys/un.h>
#include "pal.h"

#define PLATFORM_NAME "yosemite35"
#define LAST_KEY "last_key"

#define OFFSET_SYS_GUID 0x17F0
#define OFFSET_DEV_GUID 0x1800

#define NUM_SERVER_FRU  4
#define NUM_NIC_FRU     1
#define NUM_BMC_FRU     1

#define MRC_CODE_MATCH 4

#define NUM_CABLES 4
#define NUM_MANAGEMENT_PINS 2

const char pal_fru_list_print[] = "all, slot1, slot2, slot3, slot4, bmc, nic, bb, nicexp";
const char pal_fru_list_rw[] = "slot1, slot2, slot3, slot4, bmc, bb, nicexp";
const char pal_fru_list_sensor_history[] = "all, slot1, slot2, slot3, slot4, bmc nic";
const char pal_fru_list[] = "all, slot1, slot2, slot3, slot4, bmc, nic";
const char pal_guid_fru_list[] = "slot1, slot2, slot3, slot4, bmc";
const char pal_server_list[] = "slot1, slot2, slot3, slot4";
const char pal_dev_pwr_list[] = "all, 1U-dev0, 1U-dev1, 1U-dev2, 1U-dev3, 2U-dev0, 2U-dev1, 2U-dev2, 2U-dev3, 2U-dev4, 3U-dev0, " \
                            "3U-dev1, 3U-dev2, 4U-dev0, 4U-dev1, 4U-dev2, 4U-dev3, 4U-dev4";
const char pal_dev_pwr_option_list[] = "status, off, on, cycle";
const char *pal_vr_addr_list[] = {"c0h", "c4h", "ech", "c2h", "c6h", "c8h", "cch", "d0h", "96h", "9ch", "9eh", "8ah", "8ch", "8eh", "d4h", "dch", "c0h"};
const char *pal_vr_1ou_addr_list[] = {"b0h", "b4h", "c8h"};
const char *pal_server_fru_list[NUM_SERVER_FRU] = {"slot1", "slot2", "slot3", "slot4"};
const char *pal_nic_fru_list[NUM_NIC_FRU] = {"nic"};
const char *pal_bmc_fru_list[NUM_BMC_FRU] = {"bmc"};

static char sel_error_record[NUM_SERVER_FRU] = {0};

size_t server_fru_cnt = NUM_SERVER_FRU;
size_t nic_fru_cnt  = NUM_NIC_FRU;
size_t bmc_fru_cnt  = NUM_BMC_FRU;

#define BOOR_ORDER_STR "slot%d_boot_order"
#define SEL_ERROR_STR  "slot%d_sel_error"
#define SNR_HEALTH_STR "slot%d_sensor_health"
#define GPIO_OCP_DEBUG_BMC_PRSNT_N "OCP_DEBUG_BMC_PRSNT_N"

#define KEY_POWER_LIMIT "fru%d_power_limit_status"
#define UNINITIAL_POWER_LIMIT 0x02

#define SLOT1_POSTCODE_OFFSET 0x02
#define SLOT2_POSTCODE_OFFSET 0x03
#define SLOT3_POSTCODE_OFFSET 0x04
#define SLOT4_POSTCODE_OFFSET 0x05
#define DEBUG_CARD_UART_MUX 0x06

#define BB_CPLD_IO_BASE_OFFSET 0x16

#define ENABLE_STR "enable"
#define DISABLE_STR "disable"
#define STATUS_STR "status"
#define FAN_MODE_STR_LEN 8 // include the string terminal

#define IPMI_GET_VER_FRU_NUM  5
#define IPMI_GET_VER_MAX_COMP 9
#define MAX_FW_VER_LEN        32  //include the string terminal
#define MAX_CMD_LEN           128

#define MAX_COMPONENT_LEN 32 //include the string terminal

#define BMC_CPLD_BUS     (12)
#define NIC_EXP_CPLD_BUS (9)
#define CPLD_FW_VER_ADDR (0x80)
#define HD_CPLD_FW_VER_ADDR (0x88)
#define BMC_CPLD_VER_REG (0x28002000)
#define SB_CPLD_VER_REG  (0x000000c0)
#define KEY_BMC_CPLD_VER "bmc_cpld_ver"
#define CABLE_PRSNT_REG (0x30)

#define MAX_RETRY 3

#define ERROR_LOG_LEN 256
#define ERR_DESC_LEN 64

#define KEY_FAN_MODE_EVENT "fan_mode_event"

static int key_func_pwr_last_state(int event, void *arg);
static int key_func_por_cfg(int event, void *arg);
static int key_func_end_of_post(int event, void *arg);

static uint8_t memory_record_code[4][4] = {0};

enum key_event {
  KEY_BEFORE_SET,
  KEY_AFTER_INI,
};

enum sel_event_data_index {
  DATA_INDEX_0 = 3,
  DATA_INDEX_1 = 4,
  DATA_INDEX_2 = 5,
};

enum get_fw_ver_board_type {
  FW_VER_BMC = 0,
  FW_VER_NIC,
  FW_VER_BB,
  FW_VER_SB,
  FW_VER_2OU,
};

enum cable_connection_status {
  CABLE_DISCONNECT = 0,
  CABLE_CONNECT = 1,
};

enum fscd_fan_mode {
  FAN_MODE_NORMAL = 0,
  FAN_MODE_TRANSITION,
  FAN_MODE_BOOST,
  FAN_MODE_PROGRESSIVE,
};

struct pal_key_cfg {
  char *name;
  char *def_val;
  int (*function)(int, void*);
} key_cfg[] = {
  /* name, default value, function */
  {"fru1_sysfw_ver", "0", NULL},
  {"fru2_sysfw_ver", "0", NULL},
  {"fru3_sysfw_ver", "0", NULL},
  {"fru4_sysfw_ver", "0", NULL},
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
  {"slot1_enable_pxe_sel", "0", NULL},
  {"slot2_enable_pxe_sel", "0", NULL},
  {"slot3_enable_pxe_sel", "0", NULL},
  {"slot4_enable_pxe_sel", "0", NULL},
  {"slot1_end_of_post", "1", key_func_end_of_post},
  {"slot2_end_of_post", "1", key_func_end_of_post},
  {"slot3_end_of_post", "1", key_func_end_of_post},
  {"slot4_end_of_post", "1", key_func_end_of_post},
  /* Add more Keys here */
  {LAST_KEY, LAST_KEY, NULL} /* This is the last key of the list */
};

MAPTOSTRING root_port_common_mapping[] = {
    // XCC
    { 0xB3, 1, 0x5A, "Num 0", "SB" },   // root_port=0x5A, Boot Drive
    { 0xB3, 5, 0x5E, "Class 1", "NIC"}, // root_port=0x5E, Class 1 NIC
    // MCC
    { 0xBB, 7, 0x57, "Num 0", "SB" },   // root_port=0x5G, Boot Drive
    // QS
    { 0xBB, 1, 0x5A, "Num 0", "SB" },   // root_port=0x5A, Boot Drive
    { 0xBB, 5, 0x5E, "Class 1", "NIC"}, // root_port=0x5E, Class 1 NIC
    // Halfdome
    { 0x00, 3, 0xFF, "Class 1", "NIC"}, //  Root port of Class 1 NIC
    { 0x01, 0, 0xFF, "Class 1", "NIC"}, //  Endpoint of Class 1 NIC
    { 0x00, 5, 0xFF, "Num 0", "SB" },   //  Root port of Boot Drive
    { 0x02, 0, 0xFF, "Num 0", "SB" },   //  Endpoint of Boot Drive
    { 0xC0, 1, 0xFF, "CXL", "1OU" },   //  Root port of RBF CXL
    { 0xFF, 0, 0xFF, "CXL", "1OU" },   //  Endpoint of RBF CXL
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

MAPTOSTRING root_port_mapping_gpv3[] = {
    // bus, device, port, silk screen, location
    { 0x17, 0, 0x01, "Num 0", "2OU"},
    { 0x17, 1, 0x02, "Num 1", "2OU"},
    { 0x17, 2, 0x03, "Num 2", "2OU"},
    { 0x17, 3, 0x04, "Num 3", "2OU"},
    { 0x17, 4, 0x05, "Num 4", "2OU"},
    { 0x17, 5, 0x06, "Num 5", "2OU"},
    { 0x17, 6, 0x07, "Num 6", "2OU"},
    { 0x17, 7, 0x08, "Num 7", "2OU"},
    { 0x65, 0, 0x09, "Num 8", "2OU"},
    { 0x65, 1, 0x0A, "Num 9", "2OU"},
    { 0x65, 2, 0x0B, "Num 10", "2OU"},
    { 0x65, 3, 0x0C, "Num 11", "2OU"},
    { 0x17, 8, 0x0D, "E1S 0", "2OU"},
    { 0x65, 4, 0x0E, "E1S 1", "2OU"},
};

MAPTOSTRING root_port_mapping_e1s[] = {
    // XCC
    { 0x7F, 1, 0x3A, "Num 0", "1OU" }, // root_port=0x3A, 1OU E1S
    { 0x7F, 3, 0x3C, "Num 1", "1OU" }, // root_port=0x3C, 1OU E1S
    { 0x7F, 5, 0x3E, "Num 2", "1OU" }, // root_port=0x3E, 1OU E1S
    { 0x7F, 7, 0x37, "Num 3", "1OU" }, // root_port=0x3G, 1OU E1S
    // MCC
    { 0x84, 1, 0x3A, "Num 0", "1OU" }, // root_port=0x3A, 1OU E1S
    { 0x84, 3, 0x3C, "Num 1", "1OU" }, // root_port=0x3C, 1OU E1S
    { 0x84, 5, 0x3E, "Num 2", "1OU" }, // root_port=0x3E, 1OU E1S
    { 0x84, 7, 0x37, "Num 3", "1OU" }, // root_port=0x3G, 1OU E1S
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
    {0x53, "DPC triggered by uncorrectable error"},
    {0x54, "DPC triggered by ERR_NONFATAL"},
    {0x55, "DPC triggered by ERR_FATAL"},
    {0x59, "LER was triggered by ERR_NONFATAL"},
    {0x5A, "LER was triggered by ERR_FATAL"},
    {0xA0, "PERR (non-AER)"},
    {0xA1, "SERR (non-AER)"},
    {0xFF, "None"}
};

static mrc_desc_t mrc_warning_code[] = {
//MRC warning code
  { "WARN_MEMORY_BOOT_HEALTH_CHECK", 0x8C, 0x0 },
  { "WARN_MEMORY_BOOT_HEALTH_CHECK_MASK_FAIL", 0x8C, 0x01 },
  { "WARN_RDIMM_ON_UDIMM", 0x01, 0x0 },
  { "WARN_MINOR_CONFIG_NOT_SUPPORTED", 0x01, 0x01 },
  { "WARN_UDIMM_ON_RDIMM", 0x02, 0x0 },
  { "WARN_SODIMM_ON_RDIMM", 0x03, 0x0 },
  { "WARN_4Gb_FUSE", 0x04, 0x0 },
  { "WARN_8Gb_FUSE", 0x05, 0x0 },
  { "WARN_IMC_DISABLED", 0x06, 0x0 },
  { "WARN_DIMM_COMPAT", 0x07, 0x0 },
  { "WARN_DIMM_COMPAT_MINOR_X16_C0MBO", 0x07, 0x01 },
  { "WARN_DIMM_COMPAT_MINOR_MAX_RANKS", 0x07, 0x02 },
  { "WARN_DIMM_COMPAT_MINOR_QR", 0x07, 0x03 },
  { "WARN_DIMM_COMPAT_MINOR_NOT_SUPPORTED", 0x07, 0x04 },
  { "WARN_RANK_NUM", 0x07, 0x05 },
  { "WARN_TOO_SLOW", 0x07, 0x06 },
  { "WARN_DIMM_COMPAT_MINOR_ROW_ADDR_ORDER", 0x07, 0x07 },
  { "WARN_CHANNEL_CONFIG_NOT_SUPPORTED", 0x07, 0x08 },
  { "WARN_CHANNEL_MIX_ECC_NONECC", 0x07, 0x09 },
  { "WARN_DIMM_VOLTAGE_NOT_SUPPORTED", 0x07, 0x0A },
  { "WARN_DIMM_COMPAT_TRP_NOT_SUPPORTED", 0x07, 0x0B },
  { "WARN_DIMM_NONECC", 0x07, 0x0C },
  { "WARN_DIMM_COMPAT_3DS_RDIMM_NOT_SUPPORTED", 0x07, 0x0D },
  { "WARN_RANK_COUNT_MISMATCH", 0x07, 0x0E },
  { "WARN_DIMM_SKU_MISMATCH", 0x07, 0x0F },
  { "WARN_DIMM_3DS_NOT_SUPPORTED", 0x07, 0x10 },
  { "WARN_DIMM_NVMDIMM_NOT_SUPPORTED", 0x07, 0x11 },
  { "WARN_DIMM_16GB_SUPPORTED", 0x07, 0x12 },
  { "WARN_CHANNEL_SKU_NOT_SUPPORTED", 0x07, 0x13 },
  { "WARN_DIMM_SKU_NOT_SUPPORTED", 0x07, 0x14 },
  { "WARN_CHANNEL_CONFIG_FREQ_NOT_SUPPORTED", 0x07, 0x15 },
  { "WARN_DIMM_SPEED_NOT_SUP", 0x07, 0x16 },
  { "WARN_NO_DDR4_ON_SOCKET", 0x07, 0x17 },
  { "WARN_NO_DDR4_ON_S0C0D0", 0x07, 0x18 },
  { "WARN_DIMM_NOT_IN_DDRT_POR_DDR_TABLE", 0x07, 0x19 },
  { "WARN_SOCKET_CPU_DOES_NOT_SUPPORT_NVMDIMM", 0x07, 0x1A },
  { "WARN_SOCKET_SKU_DOES_NOT_SUPPORT_NVMDIMM", 0x07, 0x1B },
  { "WARN_DIMM_IN_DDRT_POR_DDR_TABLE_NOT_VALIDATED", 0x07, 0x1C },
  { "WARN_DDRT_DIMM_NOT_SUPPORTED", 0x07, 0x1D },
  { "WARN_CHANNEL_WIDTH_MIX_NOT_SUPPORTED", 0x07, 0x1E },
  { "WARN_DIMM_24GB_SUPPORTED", 0x07, 0x1F },
  { "WARN_DIMM_32GB_SUPPORTED", 0x07, 0x20 },
  { "WARN_LRDIMM_NOT_SUPPORTED", 0x07, 0x21 },
  { "WARN_LOCKSTEP_DISABLE", 0x09, 0x0 },
  { "WARN_LOCKSTEP_DISABLE_MINOR_RAS_MODE", 0x09, 0x01 },
  { "WARN_LOCKSTEP_DISABLE_MINOR_MISMATCHED", 0x09, 0x02 },
  { "WARN_LOCKSTEP_DISABLE_MINOR_MEMTEST_FAILED", 0x09, 0x03 },
  { "WARN_USER_DIMM_DISABLE", 0x0a, 0x0 },
  { "WARN_USER_DIMM_DISABLE_QUAD_AND_3DPC", 0x0a, 0x01 },
  { "WARN_USER_DIMM_DISABLE_MEMTEST", 0x0a, 0x02 },
  { "WARN_USER_DIMM_DISABLE_RANK_DISABLED", 0x0a, 0x03 },
  { "WARN_USER_DIMM_DISABLE_MAPPED_OUT", 0x0a, 0x04 },
  { "WARN_MEMTEST_DIMM_DISABLE", 0x0b, 0x0 },
  { "WARN_MIRROR_DISABLE", 0x0c, 0x0 },
  { "WARN_MIRROR_DISABLE_MINOR_RAS_DISABLED", 0x0c, 0x01 },
  { "WARN_MIRROR_DISABLE_MINOR_MISMATCH", 0x0c, 0x02 },
  { "WARN_MIRROR_DISABLE_MINOR_MEMTEST", 0x0c, 0x03 },
  { "WARN_PMIRROR_DISABLE", 0x0d, 0x0 },
  { "WARN_PMIRROR_DISABLE_MINOR_RAS_DISABLED", 0x0d, 0x01 },
  { "WARN_INTERLEAVE_FAILURE", 0x0e, 0x0 },
  { "WARN_SAD_RULES_EXCEEDED", 0x0e, 0x01 },
  { "WARN_TAD_RULES_EXCEEDED", 0x0e, 0x02 },
  { "WARN_RIR_RULES_EXCEEDED", 0x0e, 0x03 },
  { "WARN_TAD_OFFSET_NEGATIVE", 0x0e, 0x04 },
  { "WARN_TAD_LIMIT_ERROR", 0x0e, 0x05 },
  { "WARN_INTERLEAVE_3WAY", 0x0e, 0x06 },
  { "WARN_A7_MODE_AND_3WAY_CH_INTRLV", 0x0e, 0x07 },
  { "WARN_INTERLEAVE_EXCEEDED", 0x0e, 0x08 },
  { "WARN_DIMM_CAPACITY_MISMATCH", 0x0e, 0x09 },
  { "WARN_DIMM_POPULATION_MISMATCH", 0x0e, 0x0A },
  { "WARN_NM_MAX_SIZE_EXCEEDED", 0x0e, 0x0B },
  { "WARN_NM_SIZE_BELOW_MIN_LIMIT", 0x0e, 0x0C },
  { "WARN_NM_SIZE_NOT_POWER_OF_TWO", 0x0e, 0x0D },
  { "WARN_MAX_INTERLEAVE_SETS_EXCEEDED", 0x0e, 0x0E },
  { "WARN_NGN_DIMM_COMM_FAILED", 0x0e, 0x0F },
  { "WARN_DIMM_COMM_FAILED", 0x0F, 0x0 },
  { "WARN_MINOR_DIMM_COMM_START_DOORBELL_TIMEOUT", 0x0F, 0x01 },
  { "WARN_MINOR_DIMM_COMM_FAILED_STATUS", 0x0F, 0x02 },
  { "WARN_MINOR_DIMM_MAILBOX_FAILED", 0x0F, 0x03 },
  { "WARN_MINOR_DIMM_COMM_FINISH_DOORBELL_TIMEOUT", 0x0F, 0x04 },
  { "WARN_MINOR_DIMM_COMM_FINISH_COMPLETE_TIMEOUT", 0x0F, 0x05 },
  { "WARN_SPARE_DISABLE", 0x10, 0x0 },
  { "WARN_SPARE_DISABLE_MINOR_RK_SPARE", 0x10, 0x01 },
  { "WARN_PTRLSCRB_DISABLE", 0x11, 0x0 },
  { "WARN_PTRLSCRB_MINOR_DISABLE", 0x11, 0x00 },
  { "WARN_UNUSED_MEMORY", 0x12, 0x0 },
  { "WARN_UNUSED_MEMORY_MINOR_MIRROR", 0x12, 0x01 },
  { "WARN_UNUSED_MEMORY_MINOR_LOCKSTEP", 0x12, 0x02 },
  { "WARN_RD_DQ_DQS", 0x13, 0x0 },
  { "WARN_RD_RCVEN", 0x14, 0x0 },
  { "WARN_ROUNDTRIP_EXCEEDED", 0x14, 0x01 },
  { "WARN_RCVEN_PI_DELAY_EXCEEDED", 0x14, 0x02 },
  { "WARN_WR_LEVEL", 0x15, 0x0 },
  { "WARN_WR_FLYBY_CORR", 0x15, 0x00 },
  { "WARN_WR_FLYBY_UNCORR", 0x15, 0x01 },
  { "WARN_WR_FLYBY_DELAY", 0x15, 0x02 },
  { "WARN_WR_DQ_DQS", 0x16, 0x0 },
  { "WARN_DIMM_POP_RUL", 0x17, 0x0 },
  { "WARN_DIMM_POP_RUL_MINOR_OUT_OF_ORDER", 0x17, 0x01 },
  { "WARN_DIMM_POP_RUL_MINOR_INDEPENDENT_MODE", 0x17, 0x02 },
  { "WARN_DIMM_POP_RUL_2_AEP_FOUND_ON_SAME_CH", 0x17, 0x03 },
  { "WARN_DIMM_POP_RUL_MINOR_MIXED_RANKS_FOUND", 0x17, 0x04 },
  { "WARN_DIMM_POP_RUL_NVMDIMM_OUT_OF_ORDER", 0x17, 0x05 },
  { "WARN_DIMM_POP_RUL_UDIMM_POPULATION", 0x17, 0x10 },
  { "WARN_DIMM_POP_RUL_RDIMM_POPULATION", 0x17, 0x20 },
  { "WARN_DIMM_POP_RUL_LRDIMM_DUAL_DIE_POPULATION", 0x17, 0x40 },
  { "WARN_DIMM_POP_RUL_MINOR_NO_DDRT_ON_MC0", 0x17, 0x06 },
  { "WARN_DIMM_POP_RUL_MINOR_NO_DDRT_ON_SOCKET0", 0x17, 0x07 },
  { "WARN_DIMM_POP_RUL_DDR4_CAP_NOT_POR_FOR_MEM_POP", 0x17, 0x08 },
  { "WARN_DIMM_POP_RUL_DDR4_TYPE_NOT_POR_FOR_MM", 0x17, 0x09 },
  { "WARN_DIMM_POP_RUL_MEM_TOPOLOGY_NOT_SYMMETRICAL", 0x17, 0x0a },
  { "WARN_DIMM_POP_RUL_NM_FM_RATIO_VIOLATION", 0x17, 0x0b },
  { "WARN_DIMM_POP_RUL_2LM_IMC_MEM_MISMATCH", 0x17, 0x0c },
  { "WARN_DIMM_POP_RUL_DDRT_CAPACITY_MISMATCH", 0x17, 0x0d },
  { "WARN_DIMM_POP_RUL_DDRT_PARTITION_MISMATCH", 0x17, 0x0e },
  { "WARN_DIMM_POP_RUL_AEP_VDD_CHANGED", 0x17, 0x0f },
  { "WARN_DIMM_POP_RUL_SOCKET_MODE_MISMATCH", 0x17, 0x10 },
  { "WARN_DIMM_POP_RUL_DCPDIMM_MIXING", 0x17, 0x11 },
  { "WARN_DIMM_POP_RUL_DDR_CAPACITY_MISMATCH", 0x17, 0x12 },
  { "WARN_DIMM_POP_RUL_POPULATION_POR_MISMATCH", 0x17, 0x13 },
  { "WARN_DIMM_POP_RUL_MEM_POP_POR_TBL_INVALID", 0x17, 0x14 },
  { "WARN_DIMM_POP_RUL_2LM_FM_CH_NOT_PWR_OF_TWO", 0x17, 0x15 },
  { "WARN_DIMM_POP_RUL_POPULATION_POR_NOT_FOUND", 0x17, 0x16 },
  { "WARN_DIMM_POP_RUL_PMEM_X1_POPULATION_INVALID", 0x17, 0x17 },
  { "WARN_DIMM_POP_RUL_NO_DIMM_ON_SOCKET", 0x17, 0x18 },
  { "WARN_DIMM_POP_RUL_UMA_POR_COMPATABILITY", 0x17, 0x19 },
  { "WARN_DIMM_POP_RUL_MCR_POP_INVALID", 0x17, 0x1A },
  { "WARN_DIMM_POP_RUL_9X4_CONFIG_INVALID", 0x17, 0x1B },
  { "WARN_DIMM_POP_RUL_EXTENDED", 0x3e, 0x0 },
  { "WARN_DIMM_POP_RUL_LRDIMM_3DS_POPULATION", 0x3e, 0x1 },
  { "WARN_DIMM_POP_RUL_RDIMM_3DS_POPULATION", 0x3e, 0x2 },
  { "WARN_CLTT_DISABLE", 0x18, 0x0 },
  { "WARN_CLTT_MINOR_NO_TEMP_SENSOR", 0x18, 0x01 },
  { "WARN_CLTT_MINOR_CIRCUIT_TST_FAILED", 0x18, 0x02 },
  { "WARN_THROT_INSUFFICIENT", 0x19, 0x0 },
  { "WARN_2X_REFRESH_TEMPLO_DISABLED", 0x19, 0x01 },
  { "WARN_CUSTOM_REFRESH_RATE_REVERTED", 0x19, 0x02 },
  { "WARN_CLTT_DIMM_UNKNOWN", 0x1a, 0x0 },
  { "WARN_DQS_TEST", 0x1b, 0x0 },
  { "WARN_MEM_TEST", 0x1c, 0x0 },
  { "WARN_CLOSED_PAGE_OVERRIDE", 0x1d, 0x0 },
  { "WARN_DIMM_VREF_NOT_PRESENT", 0x1e, 0x0 },
  { "WARN_EARLY_RID", 0x1f, 0x0 },
  { "WARN_EARLY_RID_UNCORR", 0x1f, 0x01 },
  { "WARN_EARLY_RID_CYCLE_FAIL", 0x1f, 0x02 },
  { "WARN_LV_STD_DIMM_MIX", 0x20, 0x0 },
  { "WARN_LV_2QR_DIMM", 0x21, 0x0 },
  { "WARN_LV_3DPC", 0x22, 0x0 },
  { "WARN_CMD_ADDR_PARITY_ERR", 0x23, 0x0 },
  { "WARN_DQ_SWIZZLE_DISC", 0x24, 0x0 },
  { "WARN_DQ_SWIZZLE_DISC_UNCORR", 0x24, 0x01 },
  { "WARN_COD_HA_NOT_ACTIVE", 0x25, 0x0 },
  { "WARN_CMD_CLK_TRAINING", 0x26, 0x0 },
  { "WARN_CMD_PI_GROUP_SMALL_EYE", 0x26, 0x01 },
  { "WARN_CMD_PI_GROUP_NO_EYE", 0x26, 0x02 },
  { "WARN_INVALID_BUS", 0x27, 0x0 },
  { "WARN_INVALID_FNV_SUBOPCODE", 0x28, 0x00 },
  { "WARN_MEMORY_TRAINING", 0x29, 0x0 },
  { "WARN_CTL_CLK_LOOPBACK_TRAINING", 0x29, 0x02 },
  { "WARN_ODT_TIMING_OVERFLOW", 0x29, 0x03 },
  { "WARN_CS_CLK_LOOPBACK_TRAINING", 0x29, 0x04 },
  { "WARN_CA_CLK_LOOPBACK_TRAINING", 0x29, 0x05 },
  { "WARN_REQ_CLK_TRAINING", 0x29, 0x06 },
  { "WARN_NO_MEMORY", 0x2a, 0x0 },
  { "WARN_NO_MEMORY_MINOR_NO_MEMORY", 0x2a, 0x01 },
  { "WARN_NO_MEMORY_MINOR_ALL_CH_DISABLED", 0x2a, 0x02 },
  { "WARN_NO_MEMORY_MINOR_ALL_CH_DISABLED_MIXED", 0x2a, 0x03 },
  { "WARN_NO_MEMORY_MINOR_NO_DDR", 0x2a, 0x04 },
  { "WARN_ROUNDTRIP_ERROR", 0x2b, 0x0 },
  { "WARN_RCVNTAP_CMDDELAY_EXCEEDED", 0x2b, 0x01 },
  { "WARN_MEMORY_MODEL_ERROR", 0x2c, 0x0 },
  { "WARN_SNC24_MODEL_ERROR", 0x2c, 0x01 },
  { "WARN_QUAD_HEMI_MODEL_ERROR", 0x2c, 0x02 },
  { "WARN_SNC24_DIMM_POPULATION_MISMATCH", 0x2c, 0x03 },
  { "WARN_SNC24_INCOMPATIBLE_DDR_CAPACITY", 0x2c, 0x04 },
  { "WARN_SNC24_INCOMPATIBLE_MCDRAM_CAPACITY", 0x2c, 0x05 },
  { "WARN_MCDRAM_CONFIG_NOT_SUPPORTED", 0x2c, 0x06 },
  { "WARN_SNC24_TILE_POPULATION_MISMATCH", 0x2c, 0x07 },
  { "WARN_OVERRIDE_MEMORY_MODE", 0x2d, 0x0 },
  { "WARN_OVERRIDE_TO_FLAT_NOT_ENOUGH_NEAR_MEMORY", 0x2d, 0x01 },
  { "WARN_OVERRIDE_TO_FLAT_NOT_ENOUGH_FAR_MEMORY", 0x2d, 0x02 },
  { "WARN_OVERRIDE_TO_HYBRID_50_PERCENT", 0x2d, 0x03 },
  { "WARN_MEM_INIT", 0x2e, 0x0 },
  { "WARN_SENS_AMP_TRAINING", 0x2f, 0x0 },
  { "WARN_SENS_AMP_CH_FAILIURE", 0x2f, 0x01 },
  { "WARN_FPT_CORRECTABLE_ERROR", 0x30, 0x0 },
  { "WARN_FPT_UNCORRECTABLE_ERROR", 0x31, 0x0 },
  { "WARN_FPT_MINOR_RD_DQ_DQS", 0x31, 0x13 },
  { "WARN_FPT_MINOR_RD_RCVEN", 0x31, 0x14 },
  { "WARN_FPT_MINOR_WR_LEVEL", 0x31, 0x15 },
  { "WARN_FPT_MINOR_WR_FLYBY", 0x31, 0x00 },
  { "WARN_FPT_MINOR_WR_DQ_DQS", 0x31, 0x16 },
  { "WARN_FPT_MINOR_DQS_TEST", 0x31, 0x1b },
  { "WARN_FPT_MINOR_MEM_TEST", 0x31, 0x1c },
  { "WARN_FPT_MINOR_LRDIMM_DWL_EXT_COARSE_TRAINING", 0x31, 0x20 },
  { "WARN_FPT_MINOR_LRDIMM_DWL_EXT_FINE_TRAINING", 0x31, 0x21 },
  { "WARN_FPT_MINOR_LRDIMM_DWL_INT_COARSE_TRAINING", 0x31, 0x22 },
  { "WARN_FPT_MINOR_LRDIMM_DWL_INT_FINE_TRAINING", 0x31, 0x23 },
  { "WARN_FPT_MINOR_LRDIMM_TRAINING", 0x31, 0x24 },
  { "WARN_FPT_MINOR_VREF_TRAINING", 0x31, 0x25 },
  { "WARN_FPT_MINOR_LRDIMM_RCVEN_PHASE_TRAINING", 0x31, 0x26 },
  { "WARN_FPT_MINOR_LRDIMM_RCVEN_CYCLE_TRAINING", 0x31, 0x27 },
  { "WARN_FPT_MINOR_LRDIMM_READ_DELAY_TRAINING", 0x31, 0x28 },
  { "WARN_FPT_MINOR_LRDIMM_WL_TRAINING", 0x31, 0x29 },
  { "WARN_FPT_MINOR_LRDIMM_COARSE_WL_TRAINING", 0x31, 0x2A },
  { "WARN_FPT_MINOR_LRDIMM_WRITE_DELAY_TRAINING", 0x31, 0x2B },
  { "WARN_QxCA_CLK_NO_EYE_FOUND", 0x31, 0x2C },
  { "WARN_FPT_ROW_FAILURE", 0x31, 0x2D },
  { "WARN_FPT_PPR_ROW_REPAIR", 0x31, 0x2E },
  { "WARN_FPT_MINOR_WL_EXTERNAL_COARSE", 0x31, 0x30 },
  { "WARN_FPT_MINOR_WL_EXTERNAL_FINE", 0x31, 0x31 },
  { "WARN_FPT_MINOR_WL_INTERNAL_COARSE", 0x31, 0x32 },
  { "WARN_FPT_MINOR_WL_INTERNAL_FINE", 0x31, 0x33 },
  { "WARN_FPT_MINOR_WL_INTERNAL_OUT_OF_CYCLE", 0x31, 0x34 },
  { "WARN_FPT_MINOR_RD_DQ_DQS_JITTER", 0x31, 0x35 },
  { "WARN_CH_DISABLED", 0x32, 0x0 },
  { "WARN_TWR_LIMIT_REACHED", 0x32, 0x01 },
  { "WARN_TWR_LIMIT_ON_LOCKSTEP_CH", 0x32, 0x02 },
  { "WARN_SPD_BLOCK_UNLOCKED", 0x32, 0x03 },
  { "WARN_MEM_LIMIT", 0x33, 0x0 },
  { "WARN_RT_DIFF_EXCEED", 0x34, 0x0 },
  { "WARN_RT_DIFF_MINOR_EXCEED", 0x34, 0x01 },
  { "WARN_PPR_FAILED", 0x35, 0x0 },
  { "WARN_REGISTER_OVERFLOW", 0x36, 0x0 },
  { "WARN_MINOR_REGISTER_OVERFLOW", 0x36, 0x01 },
  { "WARN_MINOR_REGISTER_UNDERFLOW", 0x36, 0x02 },
  { "WARN_SWIZZLE_DISCOVERY_TRAINING", 0x37, 0x0 },
  { "WARN_SWIZZLE_PATTERN_MISMATCH", 0x37, 0x01 },
  { "WARN_WRCRC_DISABLE", 0x38, 0x0 },
  { "WARN_TRAIL_ODT_LIMIT_REACHED", 0x38, 0x01 },
  { "WARN_FNV_BSR", 0x39, 0x0 },
  { "WARN_DT_ERROR", 0x39, 0x01 },
  { "WARN_MEDIA_READY_ERROR", 0x39, 0x02 },
  { "WARN_POLLING_LOOP_TIMEOUT", 0x39, 0x03 },
  { "WARN_OPCODE_INDEX_LOOKUP", 0x39, 0x04 },
  { "WARN_DR_READY_ERROR", 0x05 },
  { "WARN_FADR_ERROR", 0x39, 0x06 },
  { "WARN_WDB_ERROR", 0x39, 0x07 },
  { "WARN_ADDDC_DISABLE", 0x3a, 0x0 },
  { "WARN_ADDDC_MINOR_DISABLE", 0x3a, 0x01 },
  { "WARN_SDDC_DISABLE", 0x3b, 0x0 },
  { "WARN_SDDC_MINOR_DISABLE", 0x3b, 0x02 },
  { "WARN_FW_CLK_MOVEMENT", 0x3c, 0x00 },
  { "WARN_FW_BCOM_MARGINING", 0x3c, 0x01 },
  { "WARN_SMBUS_FAULT", 0x3d, 0x00 },
  { "WARN_SMBUS_WRITE", 0x3d, 0x01 },
  { "WARN_SMBUS_READ", 0x3d, 0x02 },
  { "WARN_COMPLETION_DELAY_ERROR", 0x3f, 0x00 },
  { "WARN_CMPL_DELAY_BELOW_MIN", 0x3f, 0x02 },
  { "WARN_CMPL_DELAY_MAX_EXCEEDED", 0x3f, 0x03 },
  { "WARN_MEM_CONFIG_CHANGED", 0x40, 0x00 },
  { "WARN_MEM_OVERRIDE_DISABLED", 0x40, 0x01 },
  { "WARN_DIMM_MEM_MODE", 0x41, 0x00 },
  { "WARN_DIMM_MEM_MODE_NEW_DIMM_2LM_NOT_SUPPORTED", 0x41, 0x1 },
  { "WARN_DIMM_MEM_MODE_2LM_NOT_SUPPORTED", 0x41, 0x2 },
  { "WARN_DFE_TAP_MARGIN_STATUS", 0x42, 0x00 },
  { "WARN_DFE_TAP_ZERO_MARGIN_ERROR", 0x42, 0x01 },
  { "WARN_DFE_MARGIN_STATUS", 0x43, 0x00 },
  { "WARN_DFE_ZERO_MARGIN_ERROR", 0x43, 0x01 },
  { "WARN_FREQUENCY_POR_CHECK", 0x44, 0x00 },
  { "WARN_PLATFORM_SEGMENT_NOT_FOUND", 0x44, 0x00 },
  { "WARN_INVALID_NUMBER_OF_SOCKETS", 0x44, 0x01 },
  { "WARN_UNKNOWN_PLATFORM_SEGMENT", 0x44, 0x02 },
  { "WARN_INVALID_POR_SOCKET_CONFIG", 0x44, 0x03 },
  { "WARN_BOUNDS_ERROR", 0x45, 0x00 },
  { "WARN_INVALID_DIMM_INDEX", 0x45, 0x01 },
  { "WARN_POWER_FAILURE", 0x46, 0x00 },
  { "WARN_DDR5_POWER_FAILURE_PHASE_1", 0x46, 0x00 },
  { "WARN_DDR5_POWER_FAILURE_PHASE_2", 0x46, 0x01 },
  { "WARN_DDRT_POWER_FAILURE", 0x46, 0x02 },
  { "WARN_DDRIO_POWER_FAILURE", 0x46, 0x03 },
  { "WARN_TRAINING_RESULT_CHECK", 0x47, 0x00 },
  { "WARN_TRAINING_RESULT_BEYOND_LIMIT", 0x47, 0x01 },
  { "WARN_MEMTEST_ROW_FAIL_RANGE_ERROR", 0x48, 0x00 },
  { "WARN_MEMTEST_DDR_ROW_FAIL_RANGE_LIST_FULL", 0x48, 0x01 },
  { "WARN_MEMTEST_HBM_ROW_FAIL_RANGE_LIST_FULL", 0x48, 0x02 },
  { "WARN_MCA_UNCORRECTABLE_ERROR", 0x50, 0x00 },
  { "WARN_PREVIOUS_BOOT_MCA_MINOR_CODE", 0x50, 0x01 },
  { "WARN_DM_TEST_ERROR_CODE", 0x60, 0x00 },
  { "WARN_DM_TEST_PARSE_ERROR_MINOR_CODE", 0x60, 0x01 },
  { "WARN_DM_TEST_CONFIGURATION_ERROR_MINOR_CODE", 0x60, 0x02 },
  { "WARN_DM_TEST_EXECUTION_ERROR_MINOR_CODE", 0x60, 0x03 },
  { "WARN_FPGA_NOT_DETECTED", 0x75, 0x00 },
  //HBM Warning Codes
  { "WARN_HBM_POR_LIMITATION", 0x79, 0x00 },
  { "WARN_HBM_UNSUPPORTED_SOCKET_POPULATION", 0x79, 0x01 },
  { "WARN_HBM_UNSUPPORTED_IO_STACK_POPULATION", 0x79, 0x02 },
  { "WARN_FSM_ERROR_CODE", 0x80, 0x00 },
  { "WARN_FSM_TIMEOUT_ERROR", 0x80, 0x01 },
  { "WARN_IEEE_1500_ERROR_CODE", 0x81, 0x00 },
  { "WARN_IEEE_1500_TIMEOUT_ERROR", 0x81, 0x01 },
  { "WARN_HBM_CORRECTABLE_ERROR", 0x82, 0x00 },
  { "WARN_HBM_UNCORRECTABLE_ERROR", 0x83, 0x00 },
  { "WARN_HBM_TRAINING_FAILURE", 0x83, 0x01 },
  { "WARN_HBM_MBIST_FAILURE", 0x83, 0x02 },
  { "WARN_HBM_INVALID_PPR_ADDRESS", 0x83, 0x03 },
  { "WARN_HBM_PPR_FAILURE", 0x83, 0x04 },
  { "WARN_HBM_BASIC_MEMTEST_FAILURE", 0x83, 0x05 },
  { "WARN_HBM_ADVANCED_MEMTEST_FAILURE", 0x83, 0x06 },
  //NVDIMM Status Warning Codes
  { "WARN_NVMCTRL_MEDIA_STATUS", 0x84, 0x00 },
  { "WARN_NVMCTRL_MEDIA_NOTREADY", 0x84, 0x02 },
  { "WARN_NVMCTRL_MEDIA_INERROR", 0x84, 0x03 },
  { "WARN_NVMCTRL_MEDIA_RESERVED", 0x84, 0x04 },
  { "WARN_NVMCTRL_MEDIA_DWR", 0x84, 0x05 },
  { "WARN_NVMCTRL_MEDIA_AIT_NOTREADY", 0x84, 0x06 },
  //Current Configuration Warning Codes
  { "WARN_CFGCUR_STATUS", 0x85, 0x00 },
  { "WARN_CFGCUR_SIGNATURE_MISMATCH", 0x85, 0x01 },
  { "WARN_CFGCUR_CHECKSUM_MISMATCH", 0x85, 0x02 },
  { "WARN_CFGCUR_REVISION_MISMATCH", 0x85, 0x03 },
  { "WARN_CFGCUR_DATASIZE_EXCEEDED", 0x85, 0x04 },
  { "WARN_CFGCUR_DATASIZE_OFFSET_EXCEEDED", 0x85, 0x05 },
  //Input Configuration Warning Codes
  { "WARN_CFGIN_STATUS", 0x86, 0x00 },
  { "WARN_CFGIN_SIGNATURE_MISMATCH", 0x86, 0x01 },
  { "WARN_CFGIN_CHECKSUM_MISMATCH", 0x86, 0x02 },
  { "WARN_CFGIN_REVISION_MISMATCH", 0x86, 0x03 },
  { "WARN_CFGIN_DATASIZE_EXCEEDED", 0x86, 0x04 },
  //Output Configuration Warning Codes
  { "WARN_CFGOUT_STATUS", 0x87, 0x00 },
  { "WARN_CFGOUT_SIGNATURE_MISMATCH", 0x87, 0X01 },
  { "WARN_CFGOUT_CHECKSUM_MISMATCH", 0x87, 0X02 },
  { "WARN_CFGOUT_REVISION_MISMATCH", 0x87, 0X03 },
  { "WARN_CFGOUT_DATASIZE_EXCEEDED", 0x87, 0x04 },
  //BIOS Configuration Header Warning Codes
  { "WARN_BIOS_CONFIG_HEADER_STATUS", 0x88, 0x00 },
  { "WARN_BIOS_CONFIG_HEADER_CHECKSUM_MISMATCH", 0x88, 0x01 },
  { "WARN_BIOS_CONFIG_HEADER_REVISION_MISMATCH", 0x88, 0x02 },
  { "WARN_BIOS_CONFIG_HEADER_SIGNATURE_MISMATCH", 0x88, 0x03 },
  //OS Configuration Header Warning Codes
  { "WARN_OS_CONFIG_HEADER_STATUS", 0x89, 0x00 },
  { "WARN_OS_CONFIG_HEADER_CHECKSUM_MISMATCH", 0x89, 0x01 },
  { "WARN_OS_CONFIG_HEADER_REVISION_MISMATCH", 0x89, 0x02 },
  { "WARN_OS_CONFIG_HEADER_LENGTH_MISMATCH", 0x89, 0x03 },
  { "WARN_OS_CONFIG_HEADER_SIGNATURE_MISMATCH", 0x89, 0x04 },
  //NVMDIMM Surprise Clock Warning codes
  { "WARN_NVDIMM_SURPRISE_CLOCK_STOP", 0x8A, 0x00 },
  { "WARN_POWER_CYCLE_POLICY_NOT_APPLY", 0x8A, 0x01 },
  { "WARN_POWER_CYCLE_POLICY_APPLY", 0x8A, 0x02 },
  //NVMDIMM FNV Access Codes
  { "WARN_FNV_ACCESS", 0x8B, 0x00 },
  { "WARN_INVALID_FNV_ACCESS_MODE", 0x8B, 0x01 },
  //MEMORY Boot Health log Warnings
  { "WARN_MEMORY_BOOT_HEALTH_CHECK", 0x8C, 0x00 },
  { "WARN_MEMORY_BOOT_HEALTH_CHECK_MASK_FAIL", 0x8C, 0x01 },
  { "WARN_MEMORY_BOOT_HEALTH_CHECK_CHANNEL_MAP_OUT", 0x8C, 0x02 },
  //Memory Thermal Management Error
  { "WARN_MEMORY_POWER_MANAGEMENT", 0x8D, 0x00 },
  { "WARN_MEMORY_PM_THERMAL_PMON_VR_NOT_FOUND", 0x8D, 0x01 },
  { "WARN_MEMORY_PM_THERMAL_TABLE_NOT_FOUND", 0x8D, 0x02 },
  //Total Memory Encryption Error
  { "WARN_TME_MKTME_FAILURE", 0xE0, 0x00 },
  { "MINOR_ERR_SECURITY_POLICY_NOT_FOUND", 0xE0, 0x01 },
  { "MINOR_ERR_PHYSICAL_ADDRESS_BITS_EXCEEDED_MAX", 0xE0, 0x02 },
  //Silicon capability limitation warning
  { "WARN_SILICON_CAPABILITY_LIMITATION", 0xE1, 0x00 },
  { "WARN_TME_ENABLED_CRYSTAL_RIDGE_NOT_SUPPORTED", 0xE1, 0x01 },
  { "WARN_SGX_ENABLED_CRYSTAL_RIDGE_NOT_SUPPORTED", 0xE1, 0x02 },
  { "WARN_TME_INTEGRITY_ENABLED_CRYSTAL_RIDGE_NOT_SUPPORTED", 0xE1, 0x03 },
  //Round trip calculation for DDRT exceeded valid range
  { "WARN_ROUNDTRIP_CALCULATION_RANGE_ERROR", 0xE2, 0x00 },
  { "WARN_DDRT_TO_DDR4_ROUNDTRIP_RANGE_ERROR", 0xE2, 0x01 },
  { "WARN_DDR4_TO_DDRT_ROUNDTRIP_RANGE_ERROR", 0xE2, 0x02 },
  { "WARN_MINOR_WILDCARD", 0xff, 0x0 },

  //MRC fatal error cdes
  { "ERR_SPD_DECODE", 0xE0, 0x0 },
  { "ERR_STAGGERED_SYNC", 0xE1, 0x0 },
  { "ERR_RC_INTERNAL2", 0xE2, 0x0 },
  { "ERR_RC_DCA_DFE", 0xE6, 0x0 },
  { "ERR_INVALID_SIGNAL", 0xE6, 0x01 },
  { "ERR_RC_SWEEP_LIB_INTERNAL", 0xE7, 0x01 },
  { "ERR_NO_MEMORY", 0xE8, 0x0 },
  { "ERR_NO_MEMORY_MINOR_NO_MEMORY", 0xE8, 0x01 },
  { "ERR_NO_MEMORY_MINOR_ALL_CH_DISABLED", 0xE8, 0x02 },
  { "ERR_NO_MEMORY_MINOR_ALL_CH_DISABLED_MIXED", 0xE8, 0x03 },
  { "ERR_LT_LOCK", 0xE9, 0x0 },
  { "ERR_DDR_INIT", 0xEA, 0x0 },
  { "ERR_RD_DQ_DQS", 0xEA, 0x01 },
  { "ERR_RC_EN", 0xEA, 0x02 },
  { "ERR_WR_LEVEL", 0xEA, 0x03 },
  { "ERR_WR_DQ_DQS", 0xEA, 0x04 },
  { "ERR_TX_RETRAIN_CAP", 0xEA, 0x05 },
  { "ERR_MEM_TEST", 0xEB, 0x0 },
  { "ERR_MEM_TEST_MINOR_SOFTWARE", 0xEB, 0x01 },
  { "ERR_MEM_TEST_MINOR_HARDTWARE", 0xEB, 0x02 },
  { "ERR_MEM_TEST_MINOR_LOCKSTEP_MODE", 0xEB, 0x03 },
  { "ERR_VENDOR_SPECIFIC", 0xEC, 0x0 },
  { "ERR_DIMM_PLL_LOCK_ERROR", 0xEC, 0x01 },
  { "ERR_DIMM_COMPAT", 0xED, 0x0 },
  { "ERR_MIXED_MEM_TYPE", 0xED, 0x01 },
  { "ERR_INVALID_POP", 0xED, 0x02 },
  { "ERR_INVALID_POP_MINOR_QR_AND_3RD_SLOT", 0xED, 0x03 },
  { "ERR_INVALID_POP_MINOR_UDIMM_AND_ 3RD_SLOT", 0xED, 0x04 },
  { "ERR_INVALID_POP_MINOR_UNSUPPORTED_VOLTAGE", 0xED, 0x05 },
  { "ERR_DDR3_DDR4_MIXED", 0xED, 0x06 },
  { "ERR_MIXED_SPD_TYPE", 0xED, 0x07 },
  { "ERR_MISMATCH_DIMM_TYPE", 0xED, 0x08 },
  { "ERR_INVALID_DDR5_SPD_CONTENT", 0xED, 0x09 },
  { "ERR_SMBUS_READ_FAILURE", 0xED, 0x0A },
  { "ERR_MIXED_MEM_AEP_AND_UDIMM", 0xED, 0x0B },
  { "ERR_NVMDIMM_NOT_SUPPORTED", 0xED, 0x0C },
  { "ERR_CPU_CAP_NVMDIMM_NOT_SUPPORTED", 0xED, 0x0D },
  { "ERR_MRC_COMPATIBILITY", 0xEE, 0x0 },
  { "ERR_MRC_DIRE_NONECC", 0xEE, 0x01 },
  { "ERR_MRC_STRUCT", 0xEF, 0x0 },
  { "ERR_INVALID_BOOT_MODE", 0xEF, 0x01 },
  { "ERR_INVALID_SUB_BOOT_MODE", 0xEF, 0x02 },
  { "ERR_INVALID_HOST_ADDR", 0xEF, 0x03 },
  { "ERR_ARRAY_OUT_OF_BOUNDS", 0xEF, 0x04 },
  { "ERR_IMC_NUMBER_EXCEEDED", 0xEF, 0x05 },
  { "ERR_ODT_STRUCT", 0xEF, 0x06 },
  { "ERR_SET_VDD", 0xF0, 0x0 },
  { "ERR_UNKNOWN_VR_MODE", 0xF0, 0x01 },
  { "ERR_BEYOND_MAX_VDD_OFFSET", 0xF0, 0x02 },
  { "ERR_IOT_MEM_BUFFER", 0xF1, 0x0 },
  { "ERR_RC_INTERNAL", 0xF2, 0x0 },
  { "ERR_RC_INTERNAL_HMB", 0xF2, 0x01 },
  { "ERR_INVALID_REG_ACCESS", 0xF3, 0x0 },
  { "ERR_INVALID_WRITE_REG_BDF", 0xF3, 0x01 },
  { "ERR_INVALID_WRITE_REG_OFFSET", 0xF3, 0x02 },
  { "ERR_INVALID_READ_REG_BDF", 0xF3, 0x03 },
  { "ERR_INVALID_READ_REG_OFFSET", 0xF3, 0x04 },
  { "ERR_INVALID_WRITE_REG_SIZE", 0xF3, 0x05 },
  { "ERR_INVALID_READ_REG_SIZE", 0xF3, 0x06 },
  { "ERR_UNKNOWN_REG_TYPE", 0xF3, 0x07 },
  { "ERR_INVALID_ACCESS_METHOD", 0xF3, 0x08 },
  { "ERR_INVALID_BIT_ACCESS", 0xF3, 0x09 },
  { "ERR_SET_MC_FREQ", 0xF4, 0x0 },
  { "ERR_UNSUPPORTED_MC_FREQ", 0xF4, 0x01 },
  { "ERR_UNSPECIFIED_MC_FREQ_SETTING_ERROR", 0xF4, 0x02 },
  { "ERR_READ_MC_FREQ", 0xF5, 0x0 },
  { "ERR_NOT_ABLE_READ_MC_FREQ", 0xF5, 0x01 },
  { "ERR_DIMM_CHANNEL", 0x70, 0x0 },
  { "ERR_BIST_CHECK", 0x74, 0x0 },
  { "ERR_DDR_FREQ_NOT_FOUND", 0x75, 0x0 },
  { "ERR_PIPE", 0x76, 0x0 },
  { "ERR_SMBUS", 0xF6, 0x0 },
  { "TSOD_POLLING_ENABLED_READ", 0xF6, 0x01 },
  { "TSOD_POLLING_ENABLED_WRITE", 0xF6, 0x02 },
  { "ERR_LRDIMM_SMBUS_READ_FAILURE", 0xF6, 0x03 },
  { "ERR_PCU", 0xF7, 0x0 },
  { "PCU_NOT_RESPONDING", 0xF7, 0x01 },
  { "FUSE_ERROR", 0xF7, 0x02 },
  { "ERR_PCU_COMMAND_NOT_SUPPORTED", 0xF7, 0x03 },
  { "ERR_NGN", 0xF8, 0x0 },
  { "NGN_DRIVER_NOT_RESPONSIBLE", 0xF8, 0x01 },
  { "NGN_ARRAY_OUT_OF_BOUNDS", 0xF8, 0x02 },
  { "NGN_PMEM_CONFIG_ERROR", 0xF8, 0x03 },
  { "NGN_INTERLEAVE_EXCEEDED", 0xF8, 0x04 },
  { "NGN_BYTES_MISMATCH", 0xF8, 0x05 },
  { "NGN_SKU_MISMATCH", 0xF8, 0x06 },
  { "ERR_INTERLEAVE_FAILURE", 0xF9, 0x0 },
  { "ERR_RIR_RULES_EXCEEDED", 0xF9, 0x01 },
  { "ERR_CPGC_TIMEOUTT", 0xFA, 0x0 },
  { "ERR_CAR_LIMIT", 0xFB, 0x0 },
  { "ERR_OUT_OF_CAR_RESOURCES", 0xFB, 0x01 },
  { "PRINTF_OUTOF_SYNC_ERR_MAJOR", 0xCF, 0x0 },
  { "PRINTF_OUTOF_SYNC_ERR_MINOR", 0xCF, 0x01 },
  { "ERR_CMI_FAILURE", 0xFC, 0x0  },
  { "ERR_CMI_INIT_FAILED", 0xFC, 0x01 },
  { "ERR_VALUE_OUT_OF_RANGE", 0xFD, 0x0 },
  { "ERR_VALUE_BELOW_MIN", 0xFD, 0x01 },
  { "ERR_VALUE_ABOVE_MAX", 0xFD, 0x02 },
  { "ERR_DDRIO_HWFSM", 0xFE, 0x0  },
  { "ERR_XOVER_HWFSM_TIMEOUT", 0xFE, 0x01 },
  { "ERR_XOVER_HWFSM_FAILURE", 0xFE, 0x02 },
  { "ERR_SENSEAMP_HWFSM_TIMEOUT", 0xFE, 0x03 },
  { "ERR_SENSEAMP_HWFSM_FAILURE", 0xFE, 0x04 },
  { "ERR_SENSEAMP_HWFSM_SIGNAL_ERROR", 0xFE, 0x05 },
  { "ERR_MRC_POINTER", 0xFF, 0x0 },
  { "ERR_TEMP_THRESHOLD_INVALID", 0xFF, 0x02 },
};

typedef struct
{
  uint8_t base_1ou;
  uint8_t base_2ou;
  uint8_t base_3ou;
  uint8_t base_4ou;
} SENSOR_BASE_NUM;

SENSOR_BASE_NUM sensor_base_cl = {0x50, 0x80, 0xA0, 0xD0};
SENSOR_BASE_NUM sensor_base_hd = {0x50, 0x80, 0xA0, 0xD0};
SENSOR_BASE_NUM sensor_base_hd_op = {0x40, 0x70, 0xA0, 0xD0};
SENSOR_BASE_NUM sensor_base_gl = {0x60, 0x90, 0xA0, 0xD0};

static int
pal_key_index(char *key) {

  int i;

  i = 0;
  while(strcmp(key_cfg[i].name, LAST_KEY)) {

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

static int
key_func_end_of_post(int event, void *arg) {
  int i = 0, ret = 0;
  uint8_t slot_id = 0, value = 0, direction = 0;
  char key[MAX_KEY_LEN] = {0}, value_str[MAX_VALUE_LEN] = {0};

  if (event == KEY_AFTER_INI) {
    // Get slot ID
    for (i = 0; i < NUM_SERVER_FRU; i++) {
      if (strstr((char *)arg, pal_server_fru_list[i]) != NULL) {
        slot_id = i + 1;
        break;
      }
    }

    if (fby35_common_get_slot_type(slot_id) == SERVER_TYPE_GL) {
      // Get post complete from BIC after initializing key value.
      snprintf(key, sizeof(key), POST_COMPLETE_STR, slot_id);
      ret = bic_get_virtual_gpio(slot_id, GL_BIOS_POST_COMPLETE, &value, &direction);
      // GL BIC virtual gpio post complete: 1, post not complete: 0
      if (ret < 0) {
        snprintf(value_str, sizeof(value_str), "%d", POST_COMPLETE_UNKNOWN);
      } else if (value == 0) {
        snprintf(value_str, sizeof(value_str), "%d", POST_NOT_COMPLETE);
      } else {
        snprintf(value_str, sizeof(value_str), "%d", POST_COMPLETE);
      }
      kv_set(key, value_str, 0, KV_FPERSIST);
    }
  }

  return ret;
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

  while (strcmp(key_cfg[i].name, LAST_KEY)) {
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
  //char key[MAX_KEY_LEN] = {0};

  for(i = 0; strcmp(key_cfg[i].name, LAST_KEY) != 0; i++) {
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
pal_set_power_limit(uint8_t slot_id, uint8_t *req_data, uint8_t *res_data, uint8_t *res_len) {
  int ret = CC_UNSPECIFIED_ERROR;
  char key[MAX_KEY_LEN] = {0};
  char value[MAX_VALUE_LEN] = {0};

  if ((req_data == NULL) || (res_data == NULL) ||(res_len == NULL)) {
    syslog(LOG_WARNING, "%s() Fail to set power limit due to null pointer check", __func__);
    return -1;
  }

  *res_len = 0;

  snprintf(key, sizeof(key), KEY_POWER_LIMIT, slot_id);
  snprintf(value, sizeof(value), "0x%02X", req_data[0]);
  ret = kv_set(key, value, 0, KV_FPERSIST);
  if (ret < 0) {
    syslog(LOG_WARNING, "%s() Fail to set the key \"%s\"", __func__, key);
  }

  return ret;
}

int
pal_get_power_limit(uint8_t slot_id, uint8_t *req_data, uint8_t *res_data, uint8_t *res_len) {
  int ret = 0;
  char key[MAX_KEY_LEN] = {0};
  char value[MAX_VALUE_LEN] = {0};

  if ((req_data == NULL) || (res_data == NULL) ||(res_len == NULL)) {
    syslog(LOG_WARNING, "%s() Fail to get power limit due to null pointer check", __func__);
    return -1;
  }

  *res_len = 0;

  snprintf(key, sizeof(key), KEY_POWER_LIMIT, slot_id);
  ret = kv_get(key, value, NULL, KV_FPERSIST);
  if (ret < 0) {
    res_data[*res_len] = UNINITIAL_POWER_LIMIT;
    ret = CC_SUCCESS;
  } else {
    res_data[*res_len] = (int)strtol(value, NULL, 16);
  }
  *res_len = SIZE_CPU_POWER_LIMIT_DATA;

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

  ret = fby35_common_check_slot_id(slot_id);
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

bool
pal_is_fw_update_ongoing(uint8_t fruid) {
  char key[MAX_KEY_LEN];
  char value[MAX_VALUE_LEN] = {0};
  int ret;
  struct timespec ts;

  sprintf(key, "fru%d_fwupd", fruid);
  ret = kv_get(key, value, NULL, 0);
  if (ret < 0) {
     return false;
  }

  clock_gettime(CLOCK_MONOTONIC, &ts);
  if (strtoul(value, NULL, 10) > ts.tv_sec)
     return true;

  return false;
}

void
pal_update_ts_sled()
{
  char key[MAX_KEY_LEN] = {0};
  char tstr[MAX_VALUE_LEN] = {0};
  struct timespec ts;

  clock_gettime(CLOCK_REALTIME, &ts);
  sprintf(tstr, "%ld", ts.tv_sec);

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
pal_get_fru_capability(uint8_t fru, unsigned int *caps)
{
  uint8_t bmc_location = 0, prsnt = 0;

  if (caps == NULL) {
    return -1;
  }

  *caps = 0;
  switch (fru) {
    case FRU_ALL:
      *caps = FRU_CAPABILITY_SENSOR_READ | FRU_CAPABILITY_SENSOR_HISTORY;
      break;
    case FRU_SLOT1:
    case FRU_SLOT2:
    case FRU_SLOT3:
    case FRU_SLOT4:
      if (fby35_common_is_fru_prsnt(fru, &prsnt)) {
        break;
      }

      if (prsnt == SLOT_PRESENT) {
        *caps = FRU_CAPABILITY_FRUID_ALL | FRU_CAPABILITY_SENSOR_ALL |
          FRU_CAPABILITY_SERVER | FRU_CAPABILITY_POWER_ALL |
          FRU_CAPABILITY_POWER_12V_ALL | FRU_CAPABILITY_HAS_DEVICE;
      }
      break;
    case FRU_BMC:
      *caps = FRU_CAPABILITY_FRUID_ALL | FRU_CAPABILITY_SENSOR_ALL |
        FRU_CAPABILITY_MANAGEMENT_CONTROLLER;
      break;
    case FRU_NIC:
      *caps = FRU_CAPABILITY_FRUID_READ | FRU_CAPABILITY_SENSOR_ALL |
        FRU_CAPABILITY_NETWORK_CARD;
      break;
    case FRU_BB:
      *caps = FRU_CAPABILITY_FRUID_ALL;
    case FRU_NICEXP:
      if (fby35_common_get_bmc_location(&bmc_location)) {
        break;
      }

      if (bmc_location == NIC_BMC) {
        *caps |= FRU_CAPABILITY_SENSOR_ALL;
      }
      break;
  }

  return 0;
}

int
pal_get_dev_capability(uint8_t fru, uint8_t dev, unsigned int *caps)
{
  int config_status = 0;
  uint8_t type = 0;

  if (caps == NULL) {
    return -1;
  }
  if (fru < FRU_SLOT1 || fru > FRU_SLOT4) {
    return -1;
  }

  *caps = 0;
  switch (dev) {
    case BOARD_2OU_X8:
    case BOARD_2OU_X16:
      if ((config_status = bic_is_exp_prsnt(fru)) <= 0) {
        break;
      }
      if ((config_status & PRESENT_2OU) != PRESENT_2OU) {
        break;
      }
      if (fby35_common_get_2ou_board_type(fru, &type) != 0) {
        break;
      }

      if ((dev == BOARD_2OU_X8 && (type & DPV2_X8_BOARD) == DPV2_X8_BOARD) ||
          (dev == BOARD_2OU_X16 && (type & DPV2_X16_BOARD) == DPV2_X16_BOARD)) {
        *caps = FRU_CAPABILITY_FRUID_ALL | FRU_CAPABILITY_SENSOR_ALL;
      }
      break;
    case BOARD_PROT:
      if (fby35_common_is_prot_card_prsnt(fru)) {
        *caps = FRU_CAPABILITY_FRUID_ALL;
      }
      break;
    default:
      if ((config_status = bic_is_exp_prsnt(fru)) < 0) {
        config_status = 0;
      }
      if ((config_status & PRESENT_1OU) == PRESENT_1OU) {
        if (dev >= DEV_ID0_1OU && dev <= DEV_ID4_4OU) {
          *caps = FRU_CAPABILITY_SENSOR_ALL |
            (FRU_CAPABILITY_POWER_ALL & ~FRU_CAPABILITY_POWER_RESET);
        } else if (dev == BOARD_1OU) {
          *caps = FRU_CAPABILITY_FRUID_ALL | FRU_CAPABILITY_SENSOR_ALL;
        } else {
          if (bic_get_1ou_type(fru, &type)) {
            type = TYPE_1OU_UNKNOWN;
          }
          if (type == TYPE_1OU_OLMSTEAD_POINT &&
              dev >= BOARD_1OU && dev <= BOARD_4OU) {
            *caps = FRU_CAPABILITY_FRUID_ALL | FRU_CAPABILITY_SENSOR_ALL;
          }
        }
      }
      break;
  }

  return 0;
}

int
pal_get_board_id(uint8_t slot, uint8_t *req_data, uint8_t req_len, uint8_t *res_data, uint8_t *res_len) {
  int ret = CC_UNSPECIFIED_ERROR;
  uint8_t *data = res_data;
  uint8_t tbuf[8] = {0};
  uint8_t rbuf[8] = {0};
  uint8_t rlen = sizeof(rbuf);
  uint8_t bmc_location = 0;  // the value of bmc_location is board id.
  *res_len = 0;

  ret = fby35_common_get_bmc_location(&bmc_location);
  if (ret < 0) {
    syslog(LOG_WARNING, "%s() Cannot get the location of BMC", __func__);
    return CC_UNSPECIFIED_ERROR;
  }

  *data++ = bmc_location;

  // board rev id
  if (bmc_location == BB_BMC) {
    ret = fby35_common_get_bb_rev();
    if (ret < 0) {
      *data++ = 0x00;
    } else {
      *data++ = ret;
    }
  } else {
    // Config D can not get rev id form NIC EXP CPLD so far
    *data++ = 0x00;
  }

  *data++ = slot;  // slot id
  *data++ = 0x00;  // slot type. server = 0x00

  if (fby35_common_check_slot_id(slot) != 0) {
    *res_len = data - res_data;
    return CC_SUCCESS;
  }

  // Get SB board ID
  tbuf[0] = 0x01;  // cpld bus id
  tbuf[1] = 0x42;  // slave addr
  tbuf[2] = 0x01;  // read 1 byte
  tbuf[3] = 0x05;  // register offset
  ret = retry_cond(!bic_data_wrapper(slot, NETFN_APP_REQ, CMD_APP_MASTER_WRITE_READ,
                                     tbuf, 4, rbuf, &rlen), 3, 10);
  if (ret < 0) {
    *data++ = 0;             // dummy data to match the response format
  } else {
    *data++ = rbuf[0] >> 4;  // bit[7:4]: SB board ID
  }
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

  ret = fby35_common_get_bmc_location(&bmc_location);
  if ( ret < 0 ) {
    syslog(LOG_WARNING, "%s() Cannot get the location of BMC", __func__);
    return payload_id;
  }

  if ( bmc_location == NIC_BMC ) {
    ret = bic_data_send(payload_id, NETFN_OEM_REQ, 0xF0, tbuf, tlen, rbuf, &rlen, BB_BIC_INTF);
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
  uint8_t bmc_location = 0;

  if (status == NULL) {
    return -1;
  }

  switch (fru) {
    case FRU_SLOT1:
    case FRU_SLOT2:
    case FRU_SLOT3:
    case FRU_SLOT4:
      if (fby35_common_is_fru_prsnt(fru, status) < 0) {
        return -1;
      }
      break;
    case FRU_BB:
      *status = 1;
      break;
    case FRU_NICEXP:
      if (fby35_common_get_bmc_location(&bmc_location) < 0) {
        syslog(LOG_WARNING, "%s() Cannot get the location of BMC", __func__);
        return -1;
      }
      *status = (bmc_location == NIC_BMC) ? 1 : 0;
      break;
    case FRU_NIC:
      *status = 1;
      break;
    case FRU_BMC:
      *status = 1;
      break;
    default:
      *status = 0;
      syslog(LOG_WARNING, "%s() wrong fru id 0x%02x", __func__, fru);
      return PAL_ENOTSUP;
  }

  return 0;
}

int
pal_get_fru_id(char *str, uint8_t *fru) {
  int ret = 0;

  ret = fby35_common_get_fru_id(str, fru);
  if ( ret < 0 ) {
    syslog(LOG_WARNING, "%s() wrong fru %s", __func__, str);
    return -1;
  }

  return ret;
}

int
pal_get_fruid_name(uint8_t fru, char *name) {
  return fby35_get_fruid_name(fru, name);
}

int
pal_get_fru_name(uint8_t fru, char *name) {
  int ret = 0;
  uint8_t bmc_location = 0;

  ret = fby35_common_get_bmc_location(&bmc_location);
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
    case FRU_OCPDBG:
      snprintf(name, MAX_COMPONENT_LEN, "ocpdbg");
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
  ret = fby35_common_get_bmc_location(&bmc_location);
  if ( ret < 0 ) {
    syslog(LOG_WARNING, "%s() Cannot get the location of BMC", __func__);
    return ret;
  }

  switch(fru) {
  case FRU_BMC:
    if ( bmc_location == BB_BMC ) {
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
pal_get_dev_fruid_eeprom_path(uint8_t fru, uint8_t dev_id, char *path, uint8_t path_len) {
  switch(fru) {
    case FRU_SLOT1:
    case FRU_SLOT2:
    case FRU_SLOT3:
    case FRU_SLOT4:
      if (dev_id == BOARD_2OU_X8) {
        snprintf(path, path_len, EEPROM_PATH, FRU_DEVICE_BUS(fru), DPV2_FRU_ADDR);
      } else if (dev_id == BOARD_PROT) {
        snprintf(path, path_len, EEPROM_PATH, FRU_DEVICE_BUS(fru), PROT_FRU_ADDR);
      } else {
        return PAL_ENOTSUP;
      }
      break;
    default:
      return PAL_ENOTSUP;
  }
  return PAL_EOK;
}

int
pal_get_fruid_path(uint8_t fru, char *path) {
  char fname[16] = {0};
  int ret = 0;
  uint8_t bmc_location = 0;

  ret = fby35_common_get_bmc_location(&bmc_location);
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
  }

  return bic_write_fruid(fru, 0, path, NONE_INTF);
}

int
pal_dev_fruid_write(uint8_t fru, uint8_t dev_id, char *path) {
  int ret = PAL_ENOTSUP;
  uint8_t config_status = 0;
  uint8_t bmc_location = 0;
  uint8_t type_2ou = UNKNOWN_BOARD;

  ret = fby35_common_get_bmc_location(&bmc_location);
  if ( ret < 0 ) {
    printf("%s() Couldn't get the location of BMC\n", __func__);
    return ret;
  }

  ret = bic_is_exp_prsnt(fru);
  if ( ret < 0 ) {
    printf("%s() Couldn't get the status of 1OU/2OU\n", __func__);
    return ret;
  }

  config_status = (uint8_t) ret;

  if ( (dev_id == BOARD_1OU) && ((config_status & PRESENT_1OU) == PRESENT_1OU) && (bmc_location != NIC_BMC) ) { // 1U
    return bic_write_fruid(fru, 0, path, FEXP_BIC_INTF);
  } else if ( (config_status & PRESENT_2OU) == PRESENT_2OU &&
    (dev_id == BOARD_2OU || dev_id == DPV2_X16_BOARD)) {
    if ( fby35_common_get_2ou_board_type(fru, &type_2ou) == 0 ) {
      if ( dev_id == BOARD_2OU_X16 && ((type_2ou & DPV2_X16_BOARD) == DPV2_X16_BOARD)) {
        return bic_write_fruid(fru, 1, path, NONE_INTF);
      }
    }
    if ( dev_id == BOARD_2OU ) {
      return bic_write_fruid(fru, 0, path, REXP_BIC_INTF);
    } else {
      printf("Dev%d is not supported on 2OU!\n", dev_id);
      ret = PAL_ENOTSUP;
    }
  } else if ((config_status & PRESENT_3OU) == PRESENT_3OU && (dev_id == BOARD_3OU)) {
    return bic_write_fruid(fru, 0, path, EXP3_BIC_INTF);
  } else if ((config_status & PRESENT_4OU) == PRESENT_4OU && (dev_id == BOARD_4OU)) {
    return bic_write_fruid(fru, 0, path, EXP4_BIC_INTF);
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

/*
 *  Transfer key ASCII code to character
 *  e.g. 593335434c443039540000000000000000 -> Y35CLD09T
 */
void
pal_sysfw_key_hex_to_char(char *str, uint8_t *ver) {
  char *pstr, tstr[8] = {0};
  int j = 0;

  for (int blk = 0; blk < BLK_SYSFW_VER; blk++) {
    pstr = str + blk * SIZE_SYSFW_VER*2;
    for (int i = 0; i < SIZE_SYSFW_VER*2; i += 2) {
      if (blk > 0 && i == 0) {
        continue;
      }
      memcpy(tstr, &pstr[i], 2);
      ver[j++] = strtoul(tstr, NULL, 16);
    }
  }
}

int pal_clear_bios_delay_activate_ver(int slot) {
  char ver_key[MAX_KEY_LEN] = {0};
  snprintf(ver_key, sizeof(ver_key), BIOS_NEW_VER_STR, slot);

  return kv_del(ver_key, KV_FPERSIST);
}

int
pal_get_sysfw_ver_from_bic(uint8_t slot_id, uint8_t *ver) {
  int ret = 0;
  int i, offs = 0;
  uint8_t bios_post_complete = 0;

  if (ver == NULL) {
    syslog(LOG_ERR, "%s: failed to get system firmware version due to NULL pointer\n", __func__);
    return -1;
  }

  ret = pal_get_post_complete(slot_id, &bios_post_complete);
  if ( ret < 0 ) {
    syslog(LOG_ERR, "%s() pal_get_post_complete returns %d\n", __func__, ret);
    return ret;
  }

  if (bios_post_complete != POST_COMPLETE) {
    syslog(LOG_WARNING, "%s() Failed to get BIOS firmware version because BIOS is not ready", __func__);
    return -1;
  }

  // Get BIOS firmware version from BIC if key: sysfw_ver_server is not set
  if (bic_get_sys_fw_ver(slot_id, ver) < 0) {
    syslog(LOG_WARNING, "%s() failed to get system firmware version from BIC", __func__);
    return -1;
  }

  // Set BIOS firmware version to key: sysfw_ver_server
  for (i = 0; i < BLK_SYSFW_VER; i++) {
    if (pal_set_sysfw_ver(slot_id, &ver[offs]) < 0) {
      syslog(LOG_WARNING, "%s() failed to set key value of system firmware version", __func__);
      return -1;
    }
    offs += SIZE_SYSFW_VER;

    if (ver[2] <= (14 + 16*i)) {  // data length of 1st block: 14
      i++;                        //                2nd block: 16
      break;
    }
  }
  for (; i < BLK_SYSFW_VER; i++, offs += SIZE_SYSFW_VER) {
    memset(&ver[offs], 0, SIZE_SYSFW_VER);
  }

  return ret;
}

int
pal_get_sysfw_ver(uint8_t slot, uint8_t *ver) {
  char key[MAX_KEY_LEN];
  char str[MAX_VALUE_LEN] = {0};

  if (ver == NULL) {
    syslog(LOG_ERR, "%s: failed to get system firmware version due to NULL pointer\n", __func__);
    return -1;
  }

  sprintf(key, "fru%u_sysfw_ver", slot);
  if (kv_get(key, str, NULL, KV_FPERSIST) || (strcmp(str, "0") == 0)) {
    if (pal_get_sysfw_ver_from_bic(slot, ver)) {
      memset(ver, 0, SIZE_SYSFW_VER);
    }
    return PAL_EOK;
  }

  pal_sysfw_key_hex_to_char(str, ver);

  return PAL_EOK;
}

int pal_get_delay_activate_sysfw_ver(uint8_t slot_id, uint8_t *ver) {
  char key[MAX_KEY_LEN] = {0};
  char str[MAX_VALUE_LEN] = {0};

  if (ver == NULL) {
    syslog(LOG_ERR, "%s: failed to get system firmware version due to NULL pointer\n", __func__);
    return -1;
  }

  snprintf(key, sizeof(key), "fru%u_delay_activate_sysfw_ver", slot_id);
  if (kv_get(key, str, NULL, KV_FPERSIST) || (strcmp(str, "0") == 0)) {
    memset(ver, 0, SIZE_SYSFW_VER);
    return -1;
  }

  pal_sysfw_key_hex_to_char(str, ver);

  return PAL_EOK;
}

int
pal_is_fru_ready(uint8_t fru, uint8_t *status) {
  int ret = PAL_EOK;
  uint8_t bmc_location = 0;
  uint8_t status_12v = 1;

  ret = fby35_common_get_bmc_location(&bmc_location);
  if ( ret < 0 ) {
    syslog(LOG_WARNING, "%s() Cannot get the location of BMC", __func__);
    return ret;
  }

  switch (fru) {
    case FRU_SLOT1:
    case FRU_SLOT2:
    case FRU_SLOT3:
    case FRU_SLOT4:
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

  if ( fby35_common_get_bmc_location(&bmc_location) < 0 ) {
    syslog(LOG_ERR, "%s() Cannot get the location of BMC", __func__);
    return -1;
  }

  if ( bmc_location == BB_BMC ) {
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

  if ( fby35_common_get_bmc_location(&bmc_location) < 0 ) {
    syslog(LOG_ERR, "%s() Cannot get the location of BMC", __func__);
    return -1;
  }

  if ( bmc_location == BB_BMC ) {
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
DPV2 Riser bifurcation table
-------------------------------------------------
              | P3  P2  P1  P0 |  Speed
-------------------------------------------------
Retimer 1x16  | 1   0   0   0  |  x16 (0x08)
Retimer 2x8   | 0   0   1   0  |  x8  (0x09)
Retimer 4x4   | 0   0   0   0  |  x4  (0x0A)
Others Cards  | 0   1   1   1  |  x16 (0x08)
*/
static int pal_get_dpv2_pcie_config(uint8_t slot_id, uint8_t *pcie_config) {
  const uint8_t dp_pcie_card_mask = 0x01;
  uint8_t dp_pcie_conf;
  if (bic_get_dp_pcie_config(slot_id, &dp_pcie_conf)) {
    syslog(LOG_ERR, "%s() Cannot get DPV2 PCIE configuration\n", __func__);
    return -1;
  }

  syslog(LOG_INFO, "%s() DPV2 PCIE config: %u\n", __func__, dp_pcie_conf);

  if (dp_pcie_conf & dp_pcie_card_mask) {
    // PCIE Card
    (*pcie_config) = CONFIG_B_DPV2_X16;
  } else {
    // Retimer Card
    switch(dp_pcie_conf) {
      case DPV2_RETIMER_X16:
        (*pcie_config) = CONFIG_B_DPV2_X16;
        break;
      case DPV2_RETIMER_X8:
        (*pcie_config) = CONFIG_B_DPV2_X8;
        break;
      case DPV2_RETIMER_X4:
        (*pcie_config) = CONFIG_B_DPV2_X4;
        break;
      default:
        syslog(LOG_ERR, "%s() Unable to get correct DP PCIE configuration\n", __func__);
        return -1;
    }
  }

  return 0;
}

static int pal_get_vf2_pcie_config(uint8_t slot, uint8_t *pcie_config) {
  int prsnt;

  if (pcie_config == NULL) {
    return -1;
  }

  if ((prsnt = fby35_common_get_1ou_m2_prsnt(slot)) < 0) {
    return -1;
  }

  // T3: SSD 0, 1, X, 3
  *pcie_config = ((prsnt & 0xB) == 0x0) ? CONFIG_C_VF_T3 : CONFIG_C_VF_T10;

  return 0;
}

int pal_get_poss_pcie_config(uint8_t slot, uint8_t *req_data, uint8_t req_len, uint8_t *res_data, uint8_t *res_len) {
  uint8_t pcie_conf = 0xff;
  uint8_t *data = res_data;
  int ret = 0, config_status = 0;
  uint8_t bmc_location = 0;
  uint8_t type_1ou = TYPE_1OU_UNKNOWN;
  uint8_t type_2ou = UNKNOWN_BOARD;

  ret = fby35_common_get_bmc_location(&bmc_location);
  if ( ret < 0 ) {
    syslog(LOG_ERR, "%s() Cannot get the location of BMC", __func__);
    return -1;
  }

  config_status = bic_is_exp_prsnt(slot);
  if ( config_status < 0 ) {
    syslog(LOG_ERR, "%s() Cannot get the status of 1OU/2OU", __func__);
    config_status = 0;
  }
  if ( (config_status & PRESENT_2OU) == PRESENT_2OU ) {
    if ( fby35_common_get_2ou_board_type(slot, &type_2ou) < 0 ) {
      syslog(LOG_WARNING, "%s() Failed to get 2OU board type", __func__);
    }
  }

  if ( bmc_location == BB_BMC ) {
    if ((config_status & PRESENT_1OU) == PRESENT_1OU) {
      if ( bic_get_1ou_type(slot, &type_1ou) != 0 ) {
        pcie_conf = CONFIG_C;
        goto done;
      }
      if (type_1ou == TYPE_1OU_OLMSTEAD_POINT) {
        pcie_conf = CONFIG_OP;
        goto done;
      }
    }
    switch (config_status & (PRESENT_2OU|PRESENT_1OU)) {
      case 0:
        pcie_conf = CONFIG_A;
        break;
      case PRESENT_1OU:
        switch (type_1ou) {
          case TYPE_1OU_VERNAL_FALLS_WITH_AST:
            if (pal_get_vf2_pcie_config(slot, &pcie_conf) != 0) {
              pcie_conf = CONFIG_C_VF_T10;
            }
            break;
          case TYPE_1OU_EXP_WITH_6_M2:
            pcie_conf = CONFIG_MFG;
            break;
          default:
            pcie_conf = CONFIG_C;
            break;
        }
        break;
      case PRESENT_2OU:
        pcie_conf = CONFIG_B;
        if ( type_2ou == DPV2_BOARD ) {
          // To be defined on fby35
          pal_get_dpv2_pcie_config(slot, &pcie_conf);
        }
        break;
      case (PRESENT_2OU|PRESENT_1OU):
        // MFG Test: 1OU=M2, 2OU=DPV2
        pcie_conf = CONFIG_MFG;
        break;
    }
  } else {
    pcie_conf = CONFIG_D;
  }

done:
  *data++ = pcie_conf;
  *res_len = data - res_data;
  return ret;
}

int
pal_is_slot_server(uint8_t fru)
{
  int ret = fby35_common_get_slot_type(fru);

  if ((ret == SERVER_TYPE_CL) || (ret == SERVER_TYPE_HD) || (ret == SERVER_TYPE_GL)) {
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
  if (address == 0x40 && bus_num <= 3) {
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
         case BIOS_SENSOR_PMIC_ERR:
          snprintf(name, MAX_SNR_NAME, "PMIC_ERR");
          break;
        case BIC_SENSOR_PROC_FAIL:
          sprintf(name, "PROC_FAIL");
          break;
        case BIC_SENSOR_SSD_HOT_PLUG:
          if (fby35_common_get_2ou_board_type(fru, &board_type) < 0) {
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
        case BB_BIC_SENSOR_SLOT_PRESENT:
          sprintf(name, "SLOT_PRESENT");
          break;
        case BIOS_END_OF_POST:
          snprintf(name, MAX_SNR_NAME, "END_OF_POST");
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
      strcat(error_log, "FRB3/Processor Startup/Initialization Failure, ");
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
    SOC_VRHOT    = 0x00,
  };
  uint8_t event = event_data[0];

  switch (event) {
    case SOC_VRHOT:
      strcat(error_log, "SOC VR HOT warning");
      break;
    default:
      strcat(error_log, "Undefined VR event");
      break;
  }

  return PAL_EOK;
}

/* For VR_OCP/VR_ALERT SEL */
static int
pal_get_vr_name(uint8_t fru, uint8_t vr_num, char *name) {
  enum {
    PVDDCR_CPU0 = 0x00,
    PVDDCR_CPU1 = 0x01,
    PVDD11_S3 = 0x02,
    RF_P0V9_V8_ASICA = 0x03,
    RF_VDDQAB = 0x04,
    RF_VDDQCD = 0x05,
  };

  switch (vr_num) {
    case PVDDCR_CPU0:
      snprintf(name, 32, "PVDDCR_CPU0");
      break;
    case PVDDCR_CPU1:
      snprintf(name, 32, "PVDDCR_CPU1");
      break;
    case PVDD11_S3:
      snprintf(name, 32, "PVDD11_S3");
      break;
    case RF_P0V9_V8_ASICA:
      snprintf(name, 32, "RF_P0V9/V8_ASICA");
      break;
    case RF_VDDQAB:
      snprintf(name, 32, "RF_VDDQAB");
      break;
    case RF_VDDQCD:
      snprintf(name, 32, "RF_VDDQCD");
      break;
    default:
      snprintf(name, 32, "Undefined VR");
      break;
  }

  return PAL_EOK;
}

static int
pal_parse_vr_ocp_event(uint8_t fru, uint8_t *event_data, char *error_log) {
  uint8_t vr_num = event_data[0];
  char vr_name[32] ;

  pal_get_vr_name(fru, vr_num, vr_name);
  strcat(error_log, vr_name);

  return PAL_EOK;
}

static int
pal_parse_vr_alert_event(uint8_t fru, uint8_t *event_data, char *error_log) {
  uint8_t vr_num = event_data[0] >> 1;
  uint8_t page = event_data[0] & 1;
  char tmp_log[128] = {0};
  char vr_name[32] = {0};

  pal_get_vr_name(fru, vr_num, vr_name);
  snprintf(tmp_log, 128, "%s page%d status(0x%02X%02X)", vr_name, page, event_data[2] ,event_data[1]);
  strcat(error_log, tmp_log);

  return PAL_EOK;
}

static int
pal_parse_end_of_post_event(uint8_t fru, uint8_t *event_data, char *error_log) {
  strcat(error_log, "OEM System boot event");
  return PAL_EOK;
}

static int
parse_bank_mapping_name(uint8_t bank_num, char *error_log) {

  switch (bank_num) {
    case 0:
      strcpy(error_log, "LS");
      break;
    case 1:
      strcpy(error_log, "IF");
      break;
    case 2:
      strcpy(error_log, "L2");
      break;
    case 3:
      strcpy(error_log, "DE");
      break;
    case 4:
      strcpy(error_log, "RAZ");
      break;
    case 5:
      strcpy(error_log, "EX");
      break;
    case 6:
      strcpy(error_log, "FP");
      break;
    case 7:
    case 8:
    case 9:
    case 10:
    case 11:
    case 12:
    case 13:
    case 14:
      strcpy(error_log, "L3");
      break;
    case 15:
      strcpy(error_log, "MP5");
      break;
    case 16:
      strcpy(error_log, "PB");
      break;
    case 17:
    case 18:
      strcpy(error_log, "PCS_GMI");
      break;
    case 19:
    case 20:
      strcpy(error_log, "KPX_GMI");
      break;
    case 21:
      strcpy(error_log, "UMC/PB");
      break;
    case 22:
      strcpy(error_log, "UMC/PCIE");
      break;
    case 23:
    case 24:
      strcpy(error_log, "CS");
      break;
    case 25:
      strcpy(error_log, "NBIO/SHUB");
      break;
    case 26:
      strcpy(error_log, "PCIE/SATA");
      break;
    case 27:
      strcpy(error_log, "PCIE/NBIF");
      break;
    case 28:
      strcpy(error_log, "PIE/PSP/KPX_WAFL/NBIF/USB");
      break;
    case 29:
      strcpy(error_log, "SMU/MPDMA");
      break;
    case 30:
      strcpy(error_log, "PCS_XGMI");
      break;
    case 31:
      strcpy(error_log, "KPX_SERDES");
      break;
    default:
      strcpy(error_log, "UNKNOWN");
      break;
  }

  return 0;
}

static int
pal_parse_mce_error_sel(uint8_t fru, uint8_t *event_data, char *error_log) {
  uint8_t bank_num;
  uint8_t error_type = ((event_data[1] & 0x60) >> 5);
  char temp_log[512] = {0};
  char bank_mapping_name[32] = {0};

  switch (event_data[0] & 0x0F)
  {
    case 0x0B: //Uncorrectable
    {
      switch (error_type) {
        case 0x00:
          strcat(error_log, "Uncorrected Recoverable Error, ");
          break;
        case 0x01:
          strcat(error_log, "Uncorrected Thread Fatal Error, ");
          break;
        case 0x02:
          strcat(error_log, "Uncorrected System Fatal Error, ");
          break;
        default:
          strcat(error_log, "Unknown (Uncorrectable Type Event) ");
          break;
      }
      break;
    }

    case 0x0C: //Correctable
    {
      switch (error_type) {
        case 0x00:
          strcat(error_log, "Correctable Error, ");
          break;
        case 0x01:
          strcat(error_log, "Deferred Error, ");
          break;
        default:
          strcat(error_log, "Unknown (Correctable Type Event), ");
          break;
      }
      break;
    }

    default:
    {
      strcat(error_log, "Unknown Event Type, ");
      break;
    }
  }
  bank_num = event_data[1] & 0x1F;
  parse_bank_mapping_name(bank_num, bank_mapping_name);
  snprintf(temp_log, sizeof(temp_log), "Bank Number %d (%s), ", bank_num, bank_mapping_name);
  strcat(error_log, temp_log);

  snprintf(temp_log, sizeof(temp_log), "CPU %d, Core %d", ((event_data[2] & 0xF0) >> 4), (event_data[2] & 0x0F));
  strcat(error_log, temp_log);

  return 0;
}

static void
pal_sel_root_port_mapping_tbl(uint8_t fru, uint8_t *bmc_location, MAPTOSTRING **tbl, uint8_t *cnt) {
  uint8_t board_1u = TYPE_1OU_UNKNOWN;
  uint8_t board_2u = UNKNOWN_BOARD;
  uint8_t config_status = CONFIG_UNKNOWN;
  int ret = 0;

  do {
    ret = fby35_common_get_bmc_location(bmc_location);
    if ( ret < 0 ) {
      syslog(LOG_WARNING, "%s() Cannot get the location of BMC\n", __func__);
      break;
    }

    ret = bic_is_exp_prsnt(fru);
    if ( ret < 0 ) {
      syslog(LOG_WARNING, "%s() Cannot get the status of 1OU/2OU", __func__);
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
      ret = fby35_common_get_2ou_board_type(fru, &board_2u);
      if (ret < 0) {
        syslog(LOG_ERR, "%s() Cannot get 2ou_board_type\n", __func__);
        break;
      }
    }
  } while(0);

  if ( ret < 0 ) {
    syslog(LOG_ERR, "%s() Use the default root_port_mapping\n", __func__);
  }

  if ( board_1u == TYPE_1OU_VERNAL_FALLS_WITH_AST || board_2u == E1S_BOARD ) {
    // case 1/2OU E1S
    *tbl = root_port_mapping_e1s;
    *cnt = sizeof(root_port_mapping_e1s)/sizeof(MAPTOSTRING);
  } else if ( (board_2u == GPV3_MCHP_BOARD || board_2u == GPV3_BRCM_BOARD) && \
              (*bmc_location == NIC_BMC) ) {
    *tbl = root_port_mapping_gpv3;
    *cnt = sizeof(root_port_mapping_gpv3)/sizeof(MAPTOSTRING);
  } else {
    *tbl = root_port_common_mapping;
    *cnt = sizeof(root_port_common_mapping)/sizeof(MAPTOSTRING);
  }
  return;
}

static void
pal_search_pcie_err(uint8_t err1_id, uint8_t err2_id, char *err1_desc, char *err2_desc) {
  int i;
  int size = (sizeof(pcie_err_tab)/sizeof(PCIE_ERR_DECODE));

  for ( i = 0; i < size; i++ ) {
    if ( err2_id == pcie_err_tab[i].err_id ) {
      snprintf(err2_desc, ERR_DESC_LEN, "(%s)", pcie_err_tab[i].err_descr);
      continue;
    } else if ( err1_id == pcie_err_tab[i].err_id ) {
      snprintf(err1_desc, ERR_DESC_LEN, "(%s)", pcie_err_tab[i].err_descr);
      continue;
    }

    if ( err1_desc[0] && err2_desc[0] ) {
      break;
    }
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
pal_get_pcie_err_string(uint8_t fru, uint8_t *pdata, char **sil, char **location, char *err1_str, char *err2_str) {
  uint8_t bmc_location = 0;
  uint8_t dev = pdata[0] >> 3;
  uint8_t bus = pdata[1];
  uint8_t err1_id = pdata[5];
  uint8_t err2_id = pdata[4];
  uint8_t size = 0;
  MAPTOSTRING *mapping_table = NULL;

  // get the table first
  pal_sel_root_port_mapping_tbl(fru, &bmc_location, &mapping_table, &size);

  // search for the device table first
  if ( pal_search_pcie_dev(mapping_table, size, bmc_location, dev, bus, sil, location) == false ) {
    // if dev is not found in the device table, search for the common table
    size = sizeof(root_port_common_mapping)/sizeof(MAPTOSTRING);
    pal_search_pcie_dev(root_port_common_mapping, size, bmc_location, dev, bus, sil, location);
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
  const char *comp_str[5] = {"ServerBoard", "1OU", "2OU", "SPE", "GPv3"};
  const uint8_t comp_size = ARRAY_SIZE(comp_str);
  if ( comp < comp_size ) {
    return comp_str[comp];
  }

  return "Undefined board";
}

static void
pal_get_m2_str_name(uint8_t comp, uint8_t device_num, char *error_log) {
  snprintf(error_log, 256, "%s/Num %d ", pal_get_board_name(comp), device_num);
  return;
}

static void
pal_get_2ou_vr_str_name(uint8_t comp, uint8_t vr_num, char *error_log) {
  const char *vr_list_str[5] = {"P3V3_STBY1", "P3V3_STBY2", "P3V3_STBY3", "P1V8", "PESW VR"};
  const uint8_t vr_list_size = ARRAY_SIZE(vr_list_str);
  snprintf(error_log, 256, "%s/%s ", pal_get_board_name(comp), (vr_num < vr_list_size)?vr_list_str[vr_num]:"Undefined VR");
  return;
}

static int
pal_parse_sys_sts_event(uint8_t fru, uint8_t *event_data, char *error_log) {
  enum {
    SYS_SOC_THERM_TRIP = 0x00,
    SYS_THROTTLE       = 0x02,
    SYS_PCH_THERM_TRIP = 0x03,
    SYS_UV_DETECT      = 0x04,
    SYS_HSC_THROTTLE   = 0x05,
    SYS_OC_DETECT      = 0x06,
    SYS_MB_THROTTLE    = 0x07,
    SYS_HSC_FAULT      = 0x08,
    SYS_RSVD           = 0x09,
    SYS_S0_PWR_FAILURE = 0x0A,
    SYS_M2_VPP         = 0x0B,
    SYS_M2_PGOOD       = 0x0C,
    SYS_VCCIO_FAULT    = 0x0D,
    SYS_SMI_STUCK_LOW  = 0x0E,
    SYS_OV_DETECT      = 0x0F,
    SYS_FM_THROTTLE    = 0x10,
    SYS_CPU_MEM_THERM_TRIP     = 0x11,
    SYS_PESW_ERR       = 0x12,
    SYS_2OU_VR_FAULT   = 0x13,
    SYS_FAN_SERVICE    = 0x14,
    SYS_BB_FW_EVENT    = 0x15,
    E1S_1OU_M2_PRESENT = 0x80,
    E1S_1OU_INA230_PWR_ALERT   = 0x81,
    E1S_1OU_HSC_PWR_ALERT      = 0x82,
    E1S_1OU_P12V_FAULT = 0x83,
    E1S_1OU_P3V3_FAULT = 0x84,
    P12V_EDGE_FAULT    = 0x85,
    E1S_INA233_ALERT   = 0x86,
  };
  uint8_t event = event_data[0];
  char log_msg[MAX_ERR_LOG_SIZE] = {0};
  char fan_mode_str[FAN_MODE_STR_LEN] = {0};
  char component_str[MAX_COMPONENT_LEN] = {0};
  uint8_t type_1ou = 0;

  if (bic_get_1ou_type(fru, &type_1ou)) {
    type_1ou = TYPE_1OU_UNKNOWN;
  }
  switch (event) {
    case SYS_SOC_THERM_TRIP:
      strcat(error_log, "SOC thermal trip");
      break;
    case SYS_THROTTLE:
      strcat(error_log, "SYS_Throttle throttle");
      break;
    case SYS_PCH_THERM_TRIP:
      strcat(error_log, "PCH thermal trip");
      break;
    case SYS_UV_DETECT:
      strcat(error_log, "SYS_UV");
      break;
    case SYS_FM_THROTTLE:
      strcat(error_log, "FM_Throttle throttle");
      break;
    case SYS_HSC_THROTTLE:
      strcat(error_log, "HSC_Throttle throttle");
      break;
    case SYS_OC_DETECT:
      strcat(error_log, "HSC_OC warning");
      break;
    case SYS_MB_THROTTLE:
      strcat(error_log, "MB_Throttle throttle");
      break;
    case SYS_HSC_FAULT:
      strcat(error_log, "HSC fault");
      break;
    case SYS_S0_PWR_FAILURE:
      strcat(error_log, "S0 power on failure");
      break;
    case SYS_M2_VPP:
      pal_get_m2vpp_str_name(fru, event_data[1], event_data[2], error_log);
      strcat(error_log, "VPP power control");
      break;
    case SYS_M2_PGOOD:
      pal_get_m2_str_name(event_data[1], event_data[2], error_log);
      strcat(error_log, "Power Good fault");
      break;
    case SYS_VCCIO_FAULT:
      strcat(error_log, "VCCIO fault");
      break;
    case SYS_SMI_STUCK_LOW:
      strcat(error_log, "SMI stuck low over 90s");
      break;
    case SYS_OV_DETECT:
      strcat(error_log, "VCCIO over voltage fault");
      break;
    case SYS_CPU_MEM_THERM_TRIP:
      strcat(error_log, "CPU/Memory thermal trip");
      break;
    case SYS_PESW_ERR:
      strcat(error_log, "2OU PESW error");
      break;
    case SYS_2OU_VR_FAULT:
      pal_get_2ou_vr_str_name(event_data[1], event_data[2], error_log);
      strcat(error_log, "2OU VR fault");
      break;
    case SYS_FAN_SERVICE:
      if (event_data[2] == FAN_MANUAL_MODE) {
        snprintf(fan_mode_str, sizeof(fan_mode_str), "manual");
      } else {
        snprintf(fan_mode_str, sizeof(fan_mode_str), "auto");
      }
      if ((event_data[1] == FRU_SLOT1) || (event_data[1] == FRU_SLOT3)) {
        snprintf(log_msg, sizeof(log_msg), "Fan mode changed to %s mode by slot%d", fan_mode_str, event_data[1]);
      } else {
        snprintf(log_msg, sizeof(log_msg), "Fan mode changed to %s mode by unknown slot", fan_mode_str);
      }

      strcat(error_log, log_msg);
      break;
    case SYS_BB_FW_EVENT:
      if (event_data[1] == FW_BB_BIC) {
        strncpy(component_str, "BIC", sizeof(component_str));
      } else if (event_data[1] == FW_BB_CPLD) {
        strncpy(component_str, "CPLD", sizeof(component_str));
      } else {
        strncpy(component_str, "unknown component", sizeof(component_str));
      }
      snprintf(log_msg, sizeof(log_msg), "Baseboard firmware %s update is ongoing", component_str);
      strcat(error_log, log_msg);
      break;
    case E1S_1OU_M2_PRESENT:
      if (type_1ou == TYPE_1OU_OLMSTEAD_POINT) {
        snprintf(log_msg, sizeof(log_msg), "E1S %dOU E1.S dev%d present", event_data[1] + 1, event_data[2]);
      } else {
        snprintf(log_msg, sizeof(log_msg), "E1S 1OU M.2 dev%d present", event_data[2]);
      }
      strcat(error_log, log_msg);
      break;
    case E1S_1OU_INA230_PWR_ALERT:
      snprintf(log_msg, sizeof(log_msg), "E1S 1OU dev%d INA230 power alert", event_data[2]);
      strcat(error_log, log_msg);
      break;
    case E1S_1OU_HSC_PWR_ALERT:
      strcat(error_log, "E1S 1OU HSC power alert");
      break;
    case E1S_1OU_P12V_FAULT:
      if (type_1ou == TYPE_1OU_OLMSTEAD_POINT) {
        snprintf(log_msg, sizeof(log_msg), "E1S %dOU dev%d P12V fault", event_data[1] + 1, event_data[2]);
      } else {
        snprintf(log_msg, sizeof(log_msg), "E1S 1OU dev%d P12V fault", event_data[2]);
      }
      strcat(error_log, log_msg);
      break;
    case E1S_1OU_P3V3_FAULT:
      if (type_1ou == TYPE_1OU_OLMSTEAD_POINT) {
        snprintf(log_msg, sizeof(log_msg), "E1S %dOU dev%d P3V3 fault", event_data[1] + 1, event_data[2]);
      } else {
        snprintf(log_msg, sizeof(log_msg), "E1S 1OU dev%d P3V3 fault", event_data[2]);
      }
      strcat(error_log, log_msg);
      break;
    case P12V_EDGE_FAULT:
      strcat(error_log, "P12V Edge fault");
      break;
    case E1S_INA233_ALERT:
      if (event_data[2] == 0x5) {
        snprintf(log_msg, sizeof(log_msg), "E1S %dOU P12V EDGE fault", event_data[1] + 1);
      } else if (event_data[2] == 0x6) {
        snprintf(log_msg, sizeof(log_msg), "E1S %dOU P12V MAIN fault", event_data[1] + 1);
      } else {
        snprintf(log_msg, sizeof(log_msg), "E1S %dOU dev%d INA233 alert", event_data[1] + 1, event_data[2]);
      }
      strcat(error_log, log_msg);
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
    BB_BUTTON_BMC_BIC_N_R = 0x01,
    AC_ON_OFF_BTN_SLOT1_N = 0x02,
    AC_ON_OFF_BTN_SLOT3_N = 0x03,
  };
  switch (event_data[0]) {
    case BB_BUTTON_BMC_BIC_N_R:
      strcat(error_log, "BB_BUTTON_BMC_BIC_N_R");
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

static int
pal_parse_slot_present_event(uint8_t fru, uint8_t *event_data, char *error_log) {
  enum {
    SLOT1_SYSTEM_PRESENT = 0x01,
    SLOT3_SYSTEM_PRESENT = 0x03,
    SLOT1_SYSTEM_ABSENT = 0x11,
    SLOT3_SYSTEM_ABSENT = 0x13,
    SLOT1_CABLE_ABSENT = 0x21,
    SLOT3_CABLE_ABSENT = 0x23,
    SLOT1_INSERT_SLOT3 = 0x31,
    SLOT3_INSERT_SLOT1 = 0x33,
  };
  if ((event_data != NULL) && (error_log != NULL)) {
    switch (event_data[0]) {
      case SLOT1_SYSTEM_PRESENT:
        strcat(error_log, "slot1(peer slot) present");
        break;
      case SLOT3_SYSTEM_PRESENT:
        strcat(error_log, "slot3(peer slot) present");
        break;
      case SLOT1_SYSTEM_ABSENT:
        strcat(error_log, "Abnormal - slot1(peer slot) not detected");
        break;
      case SLOT3_SYSTEM_ABSENT:
        strcat(error_log, "Abnormal - slot3(peer slot) not detected");
        break;
      case SLOT1_CABLE_ABSENT:
        strcat(error_log, "Slot1 cable is not connected to the baseboard");
        break;
      case SLOT3_CABLE_ABSENT:
        strcat(error_log, "Slot3 cable is not connected to the baseboard");
        break;
      case SLOT1_INSERT_SLOT3:
        strcat(error_log, "Abnormal - slot3 instead of slot1");
        break;
      case SLOT3_INSERT_SLOT1:
        strcat(error_log, "Abnormal - slot1 instead of slot3");
        break;
      default:
        strcat(error_log, "Undefined Baseboard BIC event");
        break;
    }
  }
  else {
    return -1;
  }

  return PAL_EOK;
}

static int
pal_parse_pmic_err_event(uint8_t fru, uint8_t *event_data, char *error_log) {
  static const char *cl_dimm_label[] = {"A0", "A2", "A3", "A4", "A6", "A7", "Unknown"};
  static const char *hd_dimm_label[] = {"A0", "A1", "A2", "A4", "A6", "A7", "A8", "A10", "Unknown"};
  const char **dimm_label = cl_dimm_label;
  uint8_t arr_size = ARRAY_SIZE(cl_dimm_label);
  uint8_t dimm_num = 0, err_type = 0;
  char tmp_log[128] = {0};
  char err_str[32] = {0};

  if ((event_data == NULL) || (error_log == NULL)) {
    syslog(LOG_WARNING, "%s(): NULL error log", __func__);
    return -1;
  }

  if (fby35_common_get_slot_type(fru) == SERVER_TYPE_HD) {
    dimm_label = hd_dimm_label;
    arr_size = ARRAY_SIZE(hd_dimm_label);
  }
  dimm_num = (event_data[0] < arr_size) ? event_data[0] : (arr_size - 1);
  err_type = event_data[1];
  get_pmic_err_str(err_type, err_str, sizeof(err_str));
  snprintf(tmp_log, sizeof(tmp_log), "DIMM %s %s", dimm_label[dimm_num], err_str);
  strcat(error_log, tmp_log);

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
      pal_parse_sys_sts_event(fru, event_data, error_log);
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
    case BB_BIC_SENSOR_SLOT_PRESENT:
      pal_parse_slot_present_event(fru, event_data, error_log);
      break;
    case BIOS_SENSOR_PMIC_ERR:
      pal_parse_pmic_err_event(fru, event_data, error_log);
      break;
    case MACHINE_CHK_ERR:
      pal_parse_mce_error_sel(fru, event_data, error_log);
      break;
    case VR_OCP:
      pal_parse_vr_ocp_event(fru, event_data, error_log);
      break;
    case VR_ALERT:
      pal_parse_vr_alert_event(fru, event_data, error_log);
      break;
    case BIOS_END_OF_POST:
      pal_parse_end_of_post_event(fru, event_data, error_log);
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
  uint8_t general_info = (uint8_t) sel[3];
  uint8_t error_type = general_info & 0x0f;
  uint8_t plat = 0;
  char temp_log[128] = {0};
  error_log[0] = '\0';
  char *sil = "NA";
  char *location = "NA";
  char err1_desc[ERR_DESC_LEN] = {0}, err2_desc[ERR_DESC_LEN] = {0};

  switch (error_type) {
    case UNIFIED_PCIE_ERR:
      plat = (general_info & 0x10) >> 4;
      if (plat == 0) {  //x86
        pal_get_pcie_err_string(fru, &sel[10], &sil, &location, err1_desc, err2_desc);

        snprintf(error_log, ERROR_LOG_LEN, "GeneralInfo: x86/PCIeErr(0x%02X), Bus %02X/Dev %02X/Fun %02X, %s/%s,"
                            "TotalErrID1Cnt: 0x%04X, ErrID2: 0x%02X%s, ErrID1: 0x%02X%s",
                general_info, sel[11], sel[10] >> 3, sel[10] & 0x7, location, sil, ((sel[13]<<8)|sel[12]), sel[14], err2_desc, sel[15], err1_desc);
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

int
pal_convert_to_dimm_str(uint8_t cpu, uint8_t channel, uint8_t slot, char *str) {
  if (!str) {
    return -1;
  }

  sprintf(str, "A%u", channel);
  return 0;
}

void
pal_log_clear(char *fru) {
  char key[MAX_KEY_LEN] = {0};
  uint8_t fru_cnt = 0;
  int i = 0;

  if ( strncmp(fru, "slot", 4) == 0 ) {
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
pal_ipmb_processing(int bus, void *buf, uint16_t size) {
  char key[MAX_KEY_LEN];
  char value[MAX_VALUE_LEN];
  struct timespec ts;
  static time_t last_time = 0;

  if ((bus == OCP_DBG_I2C_BUS) && (((uint8_t *)buf)[0] == (BMC_SLAVE_ADDR << 1))) {
    // sent from OCP debug card
    clock_gettime(CLOCK_MONOTONIC, &ts);
    if (ts.tv_sec >= (last_time + 5)) {
      last_time = ts.tv_sec;
      ts.tv_sec += 20;

      snprintf(key, sizeof(key), "ocpdbg_lcd");
      snprintf(value, sizeof(value), "%ld", ts.tv_sec);
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

  if (bus == OCP_DBG_I2C_BUS) {
    snprintf(key, sizeof(key), "ocpdbg_lcd");
    if (kv_get(key, value, NULL, 0) == 0) {
      clock_gettime(CLOCK_MONOTONIC, &ts);
      if (strtoul(value, NULL, 10) > ts.tv_sec)
        return 1;
    }
  }

  return 0;
}

int
pal_is_debug_card_prsnt(uint8_t *status) {
  int ret = -1;
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

  return 0;
}

int
pal_set_uart_IO_sts(uint8_t slot_id, uint8_t io_sts) {
  const uint8_t UART_POS_BMC = 0x00;
  int i2cfd = -1;
  int ret = PAL_EOK;
  int retry = MAX_RETRY;
  int st_idx = slot_id, end_idx = slot_id;
  uint8_t tbuf[2] = {0x00};
  uint8_t tlen = 2;

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
     tbuf[0] = BB_CPLD_IO_BASE_OFFSET + st_idx; //get the correspoding reg
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
  return ret;
}

int
pal_get_uart_select_from_kv(uint8_t *uart_select) {
  char value[MAX_VALUE_LEN] = {0};
  uint8_t loc;
  int ret = -1;

  ret = kv_get("debug_card_uart_select", value, NULL, 0);
  if (!ret) {
    loc = atoi(value);
    *uart_select = loc;
  }

  return ret;
}

int
pal_clear_vr_crc(uint8_t fru) {
  char ver_key[MAX_KEY_LEN] = {0};

  for (int j = 0; j < ARRAY_SIZE(pal_vr_addr_list); j++) {
    snprintf(ver_key, sizeof(ver_key), VR_NEW_CRC_STR, fru, pal_vr_addr_list[j]);
    kv_del(ver_key, KV_FPERSIST);

    snprintf(ver_key, sizeof(ver_key), VR_CRC_STR, fru, pal_vr_addr_list[j]);
    kv_del(ver_key, 0);
  }

  for (int j = 0; j < ARRAY_SIZE(pal_vr_1ou_addr_list); j++) {
    snprintf(ver_key, sizeof(ver_key), VR_1OU_NEW_CRC_STR, fru, pal_vr_1ou_addr_list[j]);
    kv_del(ver_key, KV_FPERSIST);

    snprintf(ver_key, sizeof(ver_key), VR_1OU_CRC_STR, fru, pal_vr_1ou_addr_list[j]);
    kv_del(ver_key, 0);
  }
  return 0;
}

int
pal_get_uart_select_from_cpld(uint8_t *uart_select) {
  int fd = 0;
  int retry = 3;
  int ret = -1;
  uint8_t tbuf[1] = {0x00};
  uint8_t rbuf[1] = {0x00};
  uint8_t tlen = 0;
  uint8_t rlen = 0;

  fd = open("/dev/i2c-12", O_RDWR);
  if (fd < 0) {
    syslog(LOG_WARNING, "Failed to open bus 12");
    return -1;
  }

  tbuf[0] = DEBUG_CARD_UART_MUX;
  tlen = 1;
  rlen = 1;

  while (ret < 0 && retry-- > 0) {
    ret = i2c_rdwr_msg_transfer(fd, CPLD_ADDRESS, tbuf, tlen, rbuf, rlen);
  }
  if (fd > 0) {
    close(fd);
  }
  if (ret < 0) {
    return -1;
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
  uint8_t tbuf[2] = {0x00};
  uint8_t rbuf[1] = {0x00};
  uint8_t offset = 0;

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
  while (ret < 0 && retry-- > 0) {
    ret = i2c_rdwr_msg_transfer(fd, CPLD_ADDRESS, tbuf, tlen, rbuf, rlen);
  }
  if (fd > 0) {
    close(fd);
  }

  return ret;
}

int
pal_mrc_warning_detect(uint8_t slot, uint8_t postcode) {
  static uint8_t mrc_position = 0;
  static uint8_t post_match_count = 0;
  static bool pattern_in_rule = false;
  int ret = 0;
  char key[MAX_KEY_LEN] = "";
  char value[MAX_VALUE_LEN] = {0};
  snprintf(key, sizeof(key), "slot%d_mrc_warning", slot);

  if (mrc_position == 4) {
    mrc_position = 0;
    pattern_in_rule = false;
  }

// DIMM loop pattern format = 00 -> DIMM location -> major code -> minor code ...... repeat
  if(((mrc_position == 0) && (postcode == 0)) || pattern_in_rule) { //MRC warning code always start with 00
    pattern_in_rule = true;
    if (memory_record_code[slot][mrc_position] == postcode) { //If the pattern is repeat, match_count++
      memory_record_code[slot][mrc_position++] = postcode;
      post_match_count++;
    } else {
      memory_record_code[slot][mrc_position] = postcode; //Else match count and position reset to zero
      post_match_count = 0;
      mrc_position = 0;
      pattern_in_rule = false;
    }
  } else {
    mrc_position = 0;
    post_match_count = 0;
  }

  if (post_match_count > MRC_CODE_MATCH) { //Repeat 4 times means the post code is in the loop.
    snprintf(value, sizeof(value), "%d", KEY_SET);
    ret = kv_set(key, value, 0, 0);
    if (ret < 0) {
      syslog(LOG_WARNING, "%s() Fail to set the key \"%s\"", __func__, key);
      return ret;
    }
    return ret;
  }

  return -1;
}

bool
pal_is_mrc_warning_occur(uint8_t slot) {
  int ret = 0;
  char key[MAX_KEY_LEN] = "";
  char value[MAX_VALUE_LEN] = {0};
  uint8_t mrc_status = 0;
  snprintf(key, sizeof(key), "slot%d_mrc_warning", slot);

  ret = kv_get(key, value, NULL, 0);
  if (ret == 0) {
    mrc_status = (uint8_t)strtol(value, NULL, 10); // convert string to number
    if (mrc_status == 1) { // if the key is set 1, no need to check the rest process.
      return true;
    }
  }

  return false;
}

int
pal_clear_mrc_warning(uint8_t slot) {
  int ret = 0;
  char key[MAX_KEY_LEN] = "";
  char value[MAX_VALUE_LEN] = {0};
  snprintf(key, sizeof(key), "slot%d_mrc_warning", slot);

  snprintf(value, sizeof(value), "%d", KEY_CLEAR);
  ret = kv_set(key, value, 0, 0);
  if (ret < 0) {
    syslog(LOG_WARNING, "%s() Fail to set the key \"%s\"", __func__, key);
    return ret;
  }

  return ret;
}

int
pal_get_dimm_loop_pattern(uint8_t slot, DIMM_PATTERN *dimm_loop_pattern) {
  int ret = 0;

  if (!dimm_loop_pattern) {
    return -1;
  }

  dimm_loop_pattern->start_code = memory_record_code[slot][0];
  snprintf(dimm_loop_pattern->dimm_location, sizeof(dimm_loop_pattern->dimm_location), "A%d", memory_record_code[slot][1]);
  dimm_loop_pattern->major_code = memory_record_code[slot][2];
  dimm_loop_pattern->minor_code = memory_record_code[slot][3];

  return ret;
}

// Handle the received post code, display it on debug card
int
pal_post_handle(uint8_t slot, uint8_t postcode) {
  uint8_t prsnt = 0;
  uint8_t uart_select = 0;
  int ret = -1;

  // Check for debug card presence
  ret = pal_is_debug_card_prsnt(&prsnt);
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

  if (pal_is_mrc_warning_occur(slot) == false) {
    pal_mrc_warning_detect(slot, postcode);
  }

  // Display the post code in the debug card
  ret = pal_post_display(uart_select, postcode);

  return ret;
}

static int
pal_store_crashdump(uint8_t fru, bool ierr) {
  uint8_t status;

  if (!pal_get_server_power(fru, &status) && !status) {
    syslog(LOG_WARNING, "%s() fru %u is OFF", __func__, fru);
    return PAL_ENOTSUP;
  }

  return fby35_common_crashdump(fru, ierr, false);
}

static int
pal_bic_sel_handler(uint8_t fru, uint8_t snr_num, uint8_t *event_data) {
  int ret = PAL_EOK;
  bool is_cri_sel = false;
  uint8_t tbuf[16] = {0};
  uint8_t rbuf[16] = {0};
  uint8_t rlen = 0, tlen = 0;
  uint8_t fan_mode = 0;
  char value[MAX_VALUE_LEN] = {0};
  char key[MAX_KEY_LEN] = {0};

  switch (snr_num) {
    case CATERR_B:
      is_cri_sel = true;
      pal_store_crashdump(fru, (event_data[3] == 0x00));  // 00h:IERR, 0Bh:MCERR
      if (event_data[3] == 0x00) { // IERR
        fan_mode = (event_data[2] == SEL_ASSERT) ? FAN_MODE_BOOST : FAN_MODE_NORMAL;
        snprintf(value, sizeof(value), "%d", fan_mode);
        kv_set(KEY_FAN_MODE_EVENT, value, 0, 0);
      }
      break;
    case CPU_DIMM_HOT:
    case PWR_ERR:
      is_cri_sel = true;
      break;
    case BIC_SENSOR_SYSTEM_STATUS:
      switch(event_data[3]) {
        case 0x14: //SYS_FAN_SERVICE
        case 0x11: //SYS_SLOT_PRSNT
        case 0x0B: //SYS_M2_VPP
        case 0x07: //SYS_FW_TRIGGER
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
      } else if (event_data[3] == SYS_FAN_EVENT) {
        if (pal_is_fw_update_ongoing(fru) == true) {
          return PAL_EOK;
        }
        // start/stop fscd according fan mode change
        fby35_common_fscd_ctrl(event_data[DATA_INDEX_2]);
      } else if (event_data[3] == SYS_BB_FW_UPDATE) {
        if (event_data[2] == SEL_ASSERT) {
          if (event_data[4] == FW_BB_BIC) {
          kv_set("bb_fw_update", "bic", 0, 0);
          } else if (event_data[4] == FW_BB_CPLD) {
            kv_set("bb_fw_update", "cpld", 0, 0);
          } else {
            kv_set("bb_fw_update", "unknown", 0, 0);
          }
        } else if (event_data[2] == SEL_DEASSERT) {
          // if BB fw update complete, delete the key
          kv_del("bb_fw_update", 0);
        }

        return PAL_EOK;
      }
      break;
    case BIOS_SENSOR_PMIC_ERR:
      memcpy(tbuf, (uint8_t *)&IANA_ID, IANA_ID_SIZE);
      memcpy(&tbuf[3], &event_data[3], 2); // dimm location, error type
      tlen = IANA_ID_SIZE + 2;
      if (bic_data_send(fru, NETFN_OEM_1S_REQ, BIC_CMD_OEM_NOTIFY_PMIC_ERR, tbuf, tlen, rbuf, &rlen, NONE_INTF) < 0) {
        return -1;
      }
      break;
    case BIOS_END_OF_POST:
      snprintf(key, MAX_KEY_LEN, POST_COMPLETE_STR, fru);
      if (event_data[2] == SEL_ASSERT) {
        // post complete
        pal_set_key_value(key, STR_VALUE_0);
      } else {
        // post not complete
        pal_set_key_value(key, STR_VALUE_1);
      }
      break;
    default:
      return PAL_EOK;
  }

  if ( is_cri_sel == true ) {
    if ( (event_data[2] & 0x80) == 0 ) sel_error_record[fru-1]++;
    else sel_error_record[fru-1]--;
    memset(key, 0, sizeof(key));
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
  return fby35_common_dev_id(str, dev);
}

int
pal_get_num_devs(uint8_t slot, uint8_t *num) {

  if (fby35_common_check_slot_id(slot) == 0) {
      *num = MAX_NUM_DEVS - 1;
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
      fby35_common_dev_name(dev, dev_name);
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
      fby35_common_dev_name(dev, name);
      break;
    default:
      return -1;
  }
  return 0;
}

int
pal_get_dev_fruid_path(uint8_t fru, uint8_t dev_id, char *path) {
  return fby35_get_fruid_path(fru, dev_id, path);
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

int
pal_set_fan_ctrl (char *ctrl_opt) {
  FILE* fp = NULL;
  uint8_t bmc_location = 0;
  uint8_t ctrl_mode, status;
  int ret = 0;
  char cmd[64] = {0};
  char buf[32];

  ret = fby35_common_get_bmc_location(&bmc_location);
  if ( ret < 0 ) {
    syslog(LOG_WARNING, "%s() Cannot get the location of BMC", __func__);
    return ret;
  }

  if (!strcmp(ctrl_opt, ENABLE_STR)) {
    ctrl_mode = AUTO_MODE;
    snprintf(cmd, sizeof(cmd), "sv start fscd > /dev/null 2>&1");
  } else if (!strcmp(ctrl_opt, DISABLE_STR)) {
    ctrl_mode = MANUAL_MODE;
    snprintf(cmd, sizeof(cmd), "sv force-stop fscd > /dev/null 2>&1");
  } else if (!strcmp(ctrl_opt, STATUS_STR)) {
    ctrl_mode = GET_FAN_MODE;
  } else {
    return -1;
  }

  // notify baseboard bic and another slot BMC (Class 2)
  if (bmc_location == NIC_BMC) {
    // get/set fan status
    if ( bic_set_fan_auto_mode(ctrl_mode, &status) < 0 ) {
      syslog(LOG_WARNING, "%s() Failed to call bic_set_fan_auto_mode. ctrl_mode=%02X", __func__, ctrl_mode);
      return -1;
    }

    // notify the other BMC except for getting fan mode
    if ( ctrl_mode != GET_FAN_MODE && (bic_notify_fan_mode(ctrl_mode) < 0) ) {
      syslog(LOG_WARNING, "%s() Failed to call bic_notify_fan_mode. ctrl_mode=%02X", __func__, ctrl_mode);
      return -1;
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
      pclose(fp);
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
      if (req_len < 6) {
        completion_code = CC_INVALID_LENGTH;
        break;
      }
      tlen = req_len - 6; // payload_id, netfn, cmd, data[0] (select), data[1] (bypass netfn), data[2] (bypass cmd)

      netfn = req_data[1];
      cmd = req_data[2];

      // Bypass command to Bridge IC
      if (tlen != 0) {
        ret = bic_data_send(slot, netfn, cmd, &req_data[3], tlen, res_data, res_len, NONE_INTF);
      } else {
        ret = bic_data_send(slot, netfn, cmd, NULL, 0, res_data, res_len, NONE_INTF);
      }

      if (0 == ret) {
        completion_code = CC_SUCCESS;
      }
      break;
    case BYPASS_ME:
      if (req_len < 6) {
        completion_code = CC_INVALID_LENGTH;
        break;
      }
      tlen = req_len - 6; // payload_id, netfn, cmd, data[0] (select), data[1] (bypass netfn), data[2] (bypass cmd)

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
      if (req_len < 7) {
        completion_code = CC_INVALID_LENGTH;
        break;
      }
      tlen = req_len - 7; // payload_id, netfn, cmd, data[0] (select), netdev, channel, cmd

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
      if (req_len != 6) {
        completion_code = CC_INVALID_LENGTH;
        break;
      }
      tlen = req_len - 6; // payload_id, netfn, cmd, data[0] (select), netdev, netenable

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
pal_parse_oem_sel(uint8_t fru, uint8_t *sel, char *error_log)
{
  uint8_t mfg_id[] = {0x15, 0xa0, 0x00};
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
pal_check_sled_managment_cable_id(uint8_t slot_id, bool log, uint8_t *cbl_val, uint8_t bmc_location) {
  enum {
    SLOT1_CABLE = 0x03,
    SLOT2_CABLE = 0x02,
    SLOT3_CABLE = 0x01,
    SLOT4_CABLE = 0x00,
  };

  const uint8_t mapping_table[NUM_CABLES] = {SLOT1_CABLE, SLOT2_CABLE, SLOT3_CABLE, SLOT4_CABLE};
  const char *gpio_management_cable_table[] = {"SLOT%d_ID0_DETECT_BMC_N", "SLOT%d_ID1_DETECT_BMC_N"};
  int i = 0;
  int ret = 0;
  char dev[32] = {0};
  uint8_t val = 0;
  gpio_value_t gval;
  uint8_t gpio_vals = 0;
  uint8_t slot_id_tmp = slot_id;
  bool vals_match = false;

  if(bmc_location == BB_BMC){
    //read GPIO vals
    for(i = 0; i < NUM_MANAGEMENT_PINS; i++) {
      snprintf(dev, sizeof(dev), gpio_management_cable_table[i], slot_id);
      gval = gpio_get_value_by_shadow(dev);
      if(gval == GPIO_VALUE_INVALID) {
        ret = -1;
        syslog(LOG_WARNING, "%s() Failed to read %s", __func__, dev);
        return ret;
      }
      val = (uint8_t)gval;
      gpio_vals |= (val << i);
    }

    if(gpio_vals == mapping_table[slot_id-1]) {
      vals_match = true;
    } else {
      vals_match = false;
    }

    if(vals_match == false) {
      for(i = 0; i < NUM_CABLES; i++) {
        if(mapping_table[i] == gpio_vals) {
          break;
        }
      }
      if (log) {
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
  }
  else {
    bic_check_cable_status();
  }
  return ret;
}

int
pal_get_cpld_ver(uint8_t fru, uint8_t *ver) {
  int ret, i2cfd;
  uint8_t rbuf[4] = {0};
  uint8_t i2c_bus = BMC_CPLD_BUS;
  uint8_t cpld_addr = CPLD_FW_VER_ADDR;
  uint32_t ver_reg = BMC_CPLD_VER_REG;
  char value[MAX_VALUE_LEN] = {0};

  uint8_t bmc_location = 0;

  if (fby35_common_get_bmc_location(&bmc_location) < 0) {
    printf("Failed to get BMC location\n");
    return -1;
  }

  switch (fru) {
    case FRU_SLOT1:
    case FRU_SLOT2:
    case FRU_SLOT3:
    case FRU_SLOT4:
      if (fby35_common_get_slot_type(fru) == SERVER_TYPE_HD){
        cpld_addr = HD_CPLD_FW_VER_ADDR;
      }
      i2c_bus = fby35_common_get_bus_id(fru) + 4;
      ver_reg = SB_CPLD_VER_REG;
      break;
    case FRU_BMC:
      if(bmc_location == NIC_BMC) {
        i2c_bus = NIC_EXP_CPLD_BUS;
      }
      if (!kv_get(KEY_BMC_CPLD_VER, value, NULL, 0)) {
        *(uint32_t *)rbuf = strtol(value, NULL, 16);
        memcpy(ver, rbuf, sizeof(rbuf));
        return 0;
      }
      break;
    default:
      return -1;
  }

  i2cfd = i2c_cdev_slave_open(i2c_bus, cpld_addr >> 1, I2C_SLAVE_FORCE_CLAIM);
  if (i2cfd < 0) {
    syslog(LOG_WARNING, "Failed to open bus %u", i2c_bus);
    return -1;
  }

  ret = i2c_rdwr_msg_transfer(i2cfd, cpld_addr, (uint8_t *)&ver_reg, sizeof(ver_reg), rbuf, sizeof(rbuf));
  close(i2cfd);
  if (ret < 0) {
    syslog(LOG_WARNING, "%s() i2c_rdwr_msg_transfer to slave@0x%02X on bus %u failed", __func__, cpld_addr, i2c_bus);
    return -1;
  }

  if (fru == FRU_BMC) {
    snprintf(value, sizeof(value), "%02X%02X%02X%02X", rbuf[3], rbuf[2], rbuf[1], rbuf[0]);
    kv_set(KEY_BMC_CPLD_VER, value, 0, 0);
  }

  memcpy(ver, rbuf, sizeof(rbuf));

  return 0;
}

int
pal_get_fw_info(uint8_t fru, unsigned char target, unsigned char* res, unsigned char* res_len)
{
  uint8_t bmc_location = 0;
  uint8_t config_status = CONFIG_UNKNOWN;
  int ret = PAL_ENOTSUP;
  uint8_t tmp_cpld_swap[4] = {0};
  uint8_t type_2ou = UNKNOWN_BOARD;

  ret = fby35_common_get_bmc_location(&bmc_location);
  if (ret < 0) {
    syslog(LOG_ERR, "%s() Cannot get the location of BMC", __func__);
    goto error_exit;
  }

  if (target == FW_CPLD) {
    if (pal_get_cpld_ver(fru, res)) {
      goto error_exit;
    }
  } else if(target == FW_BIOS) {
    ret = pal_get_sysfw_ver(fru, res);
    if ( ret < 0 ) {
      syslog(LOG_WARNING, "%s() Failed to get sysfw ver", __func__);
      goto error_exit;
    }
  } else {
    switch(target) {
    case FW_1OU_BIC:
    case FW_1OU_CPLD:
      ret = bic_is_exp_prsnt(fru);
      if (ret < 0) {
        syslog(LOG_ERR, "%s() Couldn't get the status of 1OU/2OU", __func__);
        goto error_exit;
      }
      config_status = ret;
      if (!((bmc_location == BB_BMC) && ((config_status & PRESENT_1OU) == PRESENT_1OU))) {
        goto not_support;
      }
      break;
    case FW_2OU_BIC:
    case FW_2OU_CPLD:
      ret = bic_is_exp_prsnt(fru);
      if (ret < 0) {
        syslog(LOG_ERR, "%s() Couldn't get the status of 1OU/2OU", __func__);
        goto error_exit;
      }
      config_status = ret;
      if ( fby35_common_get_2ou_board_type(fru, &type_2ou) < 0 ) {
        syslog(LOG_WARNING, "%s() Failed to get 2OU board type\n", __func__);
      }
      if ((config_status & PRESENT_2OU) != PRESENT_2OU || type_2ou == DPV2_BOARD) {
        goto not_support;
      }
      break;
    case FW_BB_BIC:
    case FW_BB_CPLD:
      if (bmc_location != NIC_BMC) {
        goto not_support;
      }
      break;
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
    *res_len = 4;
    break;
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
  case FW_SB_BIC:
    *res_len = strlen((char*)res);
    if (*res_len == 2) { // old version format

    } else if (*res_len >= 4){ // new version format
      *res_len = 7; //check BIC code, keep 7 bytes.
    } else {
      syslog(LOG_WARNING, "%s() Format not supported, length invalid %d", __func__, *res_len);
      ret = -1;
      goto error_exit;
    }
    break;
  case FW_1OU_BIC:
  case FW_2OU_BIC:
  case FW_BB_BIC:
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

  ret = fby35_common_get_bmc_location(&bmc_location);
  if ( ret < 0 ) {
    syslog(LOG_WARNING, "%s() Cannot get the location of BMC", __func__);
    goto error_exit;
  }

  if ( bmc_location == BB_BMC ) {
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

int
pal_handle_oem_1s_intr(uint8_t slot, uint8_t *data)
{
  int sock;
  int err;
  struct sockaddr_un server;

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

  ret = fby35_common_get_bus_id(fru);
  if ( ret < 0 ) {
    printf("%s() Couldn't get the bus id of fru%d\n", __func__, fru);
    goto err_exit;
  }
  bus = (uint8_t)ret + 4;

  i2cfd = i2c_cdev_slave_open(bus, SB_CPLD_ADDR, I2C_SLAVE_FORCE_CLAIM);
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

  ret = i2c_rdwr_msg_transfer(i2cfd, (SB_CPLD_ADDR << 1), tbuf, 2, NULL, 0);
  if ( ret < 0 ) {
    printf("%s() Couldn't write data to addr %02X, err: %s\n",  __func__, SB_CPLD_ADDR, strerror(errno));
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
  uint8_t cpu_prsnt_pin = 0;

  ret = bic_get_gpio(slot_id, &gpio, NONE_INTF);
  if ( ret < 0 ) {
    printf("%s() bic_get_gpio returns %d\n", __func__, ret);
    return ret;
  }

  switch (fby35_common_get_slot_type(slot_id)) {
    case SERVER_TYPE_CL:
      cpu_prsnt_pin = FM_CPU_SKTOCC_LVT3_PLD_N;
      break;
    case SERVER_TYPE_HD:
      cpu_prsnt_pin = HD_FM_PRSNT_CPU_BIC_N;
      break;
    case SERVER_TYPE_GL:
      cpu_prsnt_pin = GL_FM_CPU_SKTOCC_LVT3_PLD_N;
      break;
    default:
      //CPU Present pin default for Crater Lake
      cpu_prsnt_pin = FM_CPU_SKTOCC_LVT3_PLD_N;
      break;
  }

  if (BIT_VALUE(gpio, cpu_prsnt_pin)) {
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
    case FRU_BMC:
      return 10;
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
  uint8_t fru = 0;
  uint8_t comp = 0;
  int ret = 0;
  uint8_t bmc_location = 0;
  FILE* fp = NULL;
  char cmd[MAX_CMD_LEN] = {0};
  char buf[MAX_FW_VER_LEN] = {0};
  // To keep the format consistent with fw-util, get version from fw-util directly.
  static const char* cmd_table[IPMI_GET_VER_FRU_NUM][IPMI_GET_VER_MAX_COMP] = {
    // BMC
    {
      "/usr/bin/fw-util bmc --version bmc | awk '{print $NF}'",
      "/usr/bin/fw-util bmc --version rom | awk '{print $NF}'",
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
      "/usr/bin/fw-util slot%d --version bb_bic | awk '{print $NF}'",
      "/usr/bin/fw-util slot%d --version bb_cpld | awk '{print $NF}'",
      NULL,
      NULL,
      NULL,
      NULL,
      NULL,
      NULL,
      NULL
    },
    // Server board
    {
      "/usr/bin/fw-util slot%d --version bic | awk '{print $NF}'",
      "/usr/bin/fw-util slot%d --version bios | awk '{print $NF}'",
      "/usr/bin/fw-util slot%d --version cpld | awk '{print $NF}'",
      "/usr/bin/fw-util slot%d --version me | awk '{print $NF}'",
      "/usr/bin/fw-util slot%d --version vr_vccin | awk '{print $4}' | cut -c -8",
      "/usr/bin/fw-util slot%d --version vr_vccd | awk '{print $4}' | cut -c -8",
      "/usr/bin/fw-util slot%d --version vr_vccinfaon | awk '{print $4}' | cut -c -8"
    },
    // 2OU
    {
      "/usr/bin/fw-util slot%d --version 2ou_bic | awk '{print $NF}'",
      NULL,
      NULL,
      NULL,
      NULL,
      NULL,
      NULL,
      NULL,
      NULL
    }
  };

  if (res_len == NULL) {
    syslog(LOG_ERR, "%s(): IPMI request failed due to NULL parameter: *res_len", __func__);
    return CC_INVALID_PARAM;
  }
  *res_len = 0;

  if (req_data == NULL) {
    syslog(LOG_ERR, "%s(): IPMI request failed due to NULL parameter: *req_data", __func__);
    return CC_INVALID_PARAM;
  }
  if (res_data == NULL) {
    syslog(LOG_ERR, "%s(): IPMI request failed due to NULL parameter: *res_data", __func__);
    return CC_INVALID_PARAM;
  }

  fru = ((GET_FW_VER_REQ*)req_data)->fru;
  comp = ((GET_FW_VER_REQ*)req_data)->component;
  if ((fru >= IPMI_GET_VER_FRU_NUM) || (comp >= IPMI_GET_VER_MAX_COMP) || (cmd_table[fru][comp] == NULL)) {
    syslog(LOG_ERR, "%s(): wrong FRU or component, fru = %x, comp = %x", __func__, fru, comp);
    return CC_PARAM_OUT_OF_RANGE;
  }
  ret = fby35_common_get_bmc_location(&bmc_location);
  if (ret < 0) {
    syslog(LOG_ERR, "%s() Cannot get the location of BMC", __func__);
    return CC_UNSPECIFIED_ERROR;
  }
  if (bmc_location == BB_BMC) {
    if (fru == FW_VER_BB) {
      return CC_INVALID_PARAM;
    }
  }

  switch (fru) {
    case FW_VER_BB:
    case FW_VER_SB:
    case FW_VER_2OU:
       snprintf(cmd, sizeof(cmd), cmd_table[fru][comp], slot);
       break;
    default:
       memcpy(cmd, cmd_table[fru][comp], sizeof(cmd));
  }
  if((fp = popen(cmd, "r")) == NULL) {
    syslog(LOG_ERR, "%s(): fail to send command: %s, errno: %s", __func__, cmd_table[fru][comp], strerror(errno));
    return CC_UNSPECIFIED_ERROR;
  }

  memset(buf, 0, sizeof(buf));
  if(fgets(buf, sizeof(buf), fp) != NULL) {
    *res_len = strlen(buf) - 1; //ignore "\n"
    memcpy(res_data, buf, *res_len);
  }
  pclose(fp);

  return CC_SUCCESS;
}

int
pal_gpv3_mux_select(uint8_t slot_id, uint8_t dev_id) {
  if ( bic_mux_select(slot_id, get_gpv3_bus_number(dev_id), dev_id, REXP_BIC_INTF) < 0 ) {
    printf("* Failed to select MUX\n");
    return BIC_STATUS_FAILURE;
  }
  return BIC_STATUS_SUCCESS;
}

bool
pal_is_aggregate_snr_valid(uint8_t snr_num) {
  char sys_conf[MAX_VALUE_LEN] = {0};
  char key[MAX_KEY_LEN] = {0};
  char value[MAX_VALUE_LEN] = {0};
  snprintf(key, sizeof(key), "fan_fail_boost");

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
      if (kv_get(key, value, NULL, 0) == 0) {
        return false;
      }
      break;
    default:
      return true;
  }

  return true;
}

int
pal_check_slot_fru(uint8_t slot_id) {
  int slot_type = fby35_common_get_slot_type(slot_id);

#ifdef CONFIG_HALFDOME
  if ( slot_type != SERVER_TYPE_HD ) {
#elif CONFIG_GREATLAKES
  if ( slot_type != SERVER_TYPE_GL ) {
#else
  if ( slot_type != SERVER_TYPE_CL ) {
#endif
    syslog(LOG_CRIT, "Slot%d plugged in a wrong FRU", slot_id);
  }
  return 0;
}

int
pal_clear_cmos(uint8_t slot_id) {
  int ret = 0, i2cfd = 0, retry = 0;
  uint8_t rtc_rst_reg = 0x2C + (slot_id - 1);
  uint8_t tbuf[2] = {rtc_rst_reg, 0x00};
  uint8_t tlen = 2;
  uint8_t bmc_location = 0, status = 0;

  ret = fby35_common_get_bmc_location(&bmc_location);
  if (ret < 0) {
    syslog(LOG_ERR, "%s() Cannot get the location of BMC", __func__);
    return ret;
  }

  if ( bmc_location != BB_BMC ) {
    // TODO: Class 2
    printf("Not supported");
    return -1;
  }

  ret = pal_set_server_power(slot_id, SERVER_12V_OFF);
  if (ret < 0) {
    printf("Failed to set server power 12V-off\n");
    return ret;
  }
  sleep(DELAY_12V_CYCLE);

  printf("Performing CMOS clear\n");
  i2cfd = i2c_cdev_slave_open(BB_CPLD_BUS, CPLD_ADDRESS >> 1, I2C_SLAVE_FORCE_CLAIM);
  if ( i2cfd < 0) {
    printf("%s(): Failed to open bus %d. Err: %s\n", __func__, BB_CPLD_BUS, strerror(errno));
    return -1;
  }

  while ( retry < MAX_READ_RETRY ) {
    // to generate 200ms high pulse to clear CMOS
    ret = i2c_rdwr_msg_transfer(i2cfd, CPLD_ADDRESS, tbuf, tlen, NULL, 0);
    if ( ret < 0 ) {
      retry++;
      sleep(1);
    } else {
      break;
    }
  }
  close(i2cfd);
  if ( retry == MAX_READ_RETRY ) {
    syslog(LOG_WARNING, "%s() Failed to do i2c_rdwr_msg_transfer, tlen=%d", __func__, tlen);
    return -1;
  }
  sleep(1);

  ret = pal_set_server_power(slot_id, SERVER_12V_ON);
  if (ret < 0) {
    printf("Failed to set server power 12V-on\n");
    return ret;
  }
  if ( (pal_get_server_power(slot_id, &status) == 0) && (status == SERVER_POWER_OFF) ) {
    ret = pal_set_server_power(slot_id, SERVER_POWER_ON);
    if (ret < 0) {
      printf("Failed to set server power on\n");
      return ret;
    }
  }

  return ret;
}

int
pal_is_cable_connect_baseborad(uint8_t slot_id, uint16_t curr) {
  uint8_t tbuf[4] = {CABLE_PRSNT_REG};
  uint8_t rbuf[4] = {0};
  uint8_t mask = 1 << (slot_id - 1);
  int i2cfd = -1;
  int ret = 0;

  if (curr == GPIO_VALUE_LOW) {
    return 0;
  }

  i2cfd = i2c_cdev_slave_open(BB_CPLD_BUS, CPLD_ADDRESS >> 1, I2C_SLAVE_FORCE_CLAIM);
  if (i2cfd < 0) {
    syslog(LOG_WARNING, "Failed to open bus %d\n", BB_CPLD_BUS);
    return i2cfd;
  }

  ret = retry_cond(!i2c_rdwr_msg_transfer(i2cfd, CPLD_ADDRESS, tbuf, 1, rbuf, 1), MAX_RETRY, 50);
  if (ret < 0) {
    syslog(LOG_WARNING, "%s() Failed to read BB_CPLD, reg=0x%02x", __func__, tbuf[0]);
    goto exit;
  }

  /*
   *  rbuf[0] = 0x0~0xf
   *  if slot1 is present, bit 0 is 0 else 1.
   *  if slot2 is present, bit 1 is 0 else 1.
   *  if slot3 is present, bit 2 is 0 else 1.
   *  if slot4 is present, bit 3 is 0 else 1.
   */
  if ((rbuf[0] & mask) != 0) {
    syslog(LOG_CRIT, "Slot%u cable is not connected to the baseboard", slot_id);
  } else {
    syslog(LOG_CRIT, "Slot%u cable is not connected to the serverboard", slot_id);
  }

exit:
  if (i2cfd >= 0) {
    close(i2cfd);
  }

  return ret;
}


int
pal_set_sdr_update_flag(uint8_t slot, uint8_t update) {
  char key[MAX_KEY_LEN] = {0};
  char str[MAX_VALUE_LEN] = {0};

  snprintf(key,MAX_KEY_LEN, "slot%u_sdr_thresh_update", slot);
  snprintf(str,MAX_VALUE_LEN, "%u", update);
  return kv_set(key, str, 0, 0);
}

int
pal_get_sdr_update_flag(uint8_t slot) {
  int ret = 0;
  char key[MAX_KEY_LEN] = {0};
  char cvalue[MAX_VALUE_LEN] = {0};
  sprintf(key, "slot%u_sdr_thresh_update", slot);

  ret = kv_get(key, cvalue, NULL, 0);
  if (ret) {
    return 0;
  }
  return atoi(cvalue);
}

bool
pal_can_change_power(uint8_t fru) {
  char fruname[32] = {0};
  uint8_t bmc_location = 0;
  int ret = 0;

  ret = fby35_common_get_bmc_location(&bmc_location);
  if ( ret < 0 ) {
    syslog(LOG_WARNING, "%s() Cannot get the location of BMC", __func__);
    return false;
  }

  //For class1 and class2, BMC will check the corresponding fru that power changing may affect
  if (pal_get_fru_name(fru, fruname)) {
    sprintf(fruname, "fru%d", fru);
  }
  if (pal_is_fw_update_ongoing(fru)) {
    printf("FW update for %s is ongoing, block the power controlling.\n", fruname);
    return false;
  }
  if (pal_is_crashdump_ongoing(fru)) {
    printf("Crashdump for %s is ongoing, block the power controlling.\n", fruname);
    return false;
  }

  // For class2, doing 12V-cycle on slot will also affect NIC expansion, so we need to additionally check the fru of NIC expansion
  if ((bmc_location == NIC_BMC) && (fru == FRU_SLOT1)) {
    if (pal_get_fru_name(FRU_BMC, fruname)) {
      sprintf(fruname, "fru%d", FRU_BMC);
    }
    if (pal_is_fw_update_ongoing(FRU_BMC)) {
      printf("FW update for %s is ongoing, block the power controlling.\n", fruname);
      return false;
    }
  }

  return true;
}

/*
 When hw jumper nonpop, jtag switch will pass jtag signal between BIC and CPU/PCH.
*/
int
pal_is_jumper_enable(uint8_t slot_id, uint8_t *status) {
  int i2cfd = 0;
  uint8_t tbuf[1] = {SB_CPLD_BOARD_ASD_STATUS_REG};
  uint8_t rbuf[1] = {0};
  uint8_t tlen = 1;
  uint8_t rlen = 1;
  uint8_t val = 0;
  int ret = -1;

  if (status == NULL) {
    syslog(LOG_WARNING, "%s() fail to get jumper status due to getting NULL input *status.\n", __func__);
    return -1;
  }

  if (bic_get_one_gpio_status(slot_id, BMC_JTAG_SEL_R, &val)) {
    syslog(LOG_WARNING, "%s() get BMC_JTAG_SEL_R gpio value fail.\n", __func__);
    return -1;
  }

  if (val == ASD_DISABLE) {
    ret = bic_set_gpio(slot_id, BMC_JTAG_SEL_R, ASD_ENABLE);
    if (ret < 0) {
      syslog(LOG_WARNING, "%s() set bic_set_gpio to HIGH fail, returns %d\n", __func__, ret);
      return ret;
    }
  }

  i2cfd = i2c_cdev_slave_open(slot_id + SLOT_BUS_BASE, SB_CPLD_ADDR, I2C_SLAVE_FORCE_CLAIM);
  if ( i2cfd < 0 ) {
    syslog(LOG_WARNING, "%s() Failed to open bus %d. Err: %s", __func__, (slot_id + SLOT_BUS_BASE), strerror(errno));
    goto error_exit;
  }

  ret = i2c_rdwr_msg_transfer(i2cfd, (SB_CPLD_ADDR << 1), tbuf, tlen, rbuf, rlen);
  if ( ret < 0 ) {
    syslog(LOG_WARNING, "%s() Failed to do i2c_rdwr_msg_transfer, tlen=%d", __func__, tlen);
    goto error_exit;
  }

  *status = rbuf[0];

error_exit:
  if ( i2cfd >= 0 ) {
    close(i2cfd);
  }

  if (val == ASD_DISABLE) { // set back to the original value
    ret = bic_set_gpio(slot_id, BMC_JTAG_SEL_R, ASD_DISABLE);
    if (ret < 0) {
      syslog(LOG_WARNING, "%s() set bic_set_gpio to LOW fail, returns %d\n", __func__, ret);
      return ret;
    }
  }

  return ret;
}

int
pal_udbg_get_frame_total_num() {
  return 4;
}

bool
pal_is_support_vr_delay_activate(void){
  return true;
}

int
pal_get_mrc_desc(uint8_t fru, mrc_desc_t **desc, size_t *desc_count)
{
  if (!desc) {
    syslog(LOG_ERR, "%s() Variable: desc NULL pointer ERROR", __func__);
    return -1;
  }

  if (!desc_count) {
    syslog(LOG_ERR, "%s() Variable: desc_count NULL pointer ERROR", __func__);
    return -1;
  }

  *desc = mrc_warning_code;
  *desc_count = sizeof(mrc_warning_code) / sizeof(mrc_warning_code[0]);

  return 0;
}

bool
pal_is_prot_card_prsnt(uint8_t fru)
{
  return fby35_common_is_prot_card_prsnt(fru);
}

bool
pal_is_prot_bypass(uint8_t fru) {
  return bic_is_prot_bypass(fru);
}

int
pal_get_prot_address(uint8_t fru, uint8_t *bus, uint8_t *addr) {
  int ret = fby35_common_get_bus_id(fru);
  if (ret < 0) {
    syslog(LOG_WARNING, "%s() Cannot get the bus with fru%d", __func__, fru);
    return -1;
  }

  static const int PROT_PLATFIRE_ADDR = 0x51;
  *bus = (uint8_t)(ret + 4);
  *addr = PROT_PLATFIRE_ADDR;

  return 0;
}

int
pal_set_last_postcode(uint8_t slot, uint32_t postcode) {
  char key[MAX_KEY_LEN] = {0};
  char str[MAX_VALUE_LEN] = {0};

  snprintf(key,MAX_KEY_LEN, "slot%u_last_postcode", slot);
  snprintf(str,MAX_VALUE_LEN, "%08X", postcode);
  return kv_set(key, str, 0, 0);
}

int
pal_get_last_postcode(uint8_t slot, char* postcode) {
  int ret;
  char key[MAX_KEY_LEN] = {0};
  sprintf(key, "slot%u_last_postcode", slot);

  ret = kv_get(key, postcode,NULL,0);
  if (ret) {
    syslog(LOG_WARNING,"pal_get_last_postcode failed");
    return -1;
  }
  return 0;
}

#ifdef CONFIG_HALFDOME
int
pal_get_80port_page_record(uint8_t slot, uint8_t page_num, uint8_t *res_data, size_t max_len, size_t *res_len) {

  int ret;
  uint8_t status;
  uint8_t len;

  if (slot < FRU_SLOT1 || slot > FRU_SLOT4) {
    return PAL_ENOTSUP;
  }

  ret = pal_is_fru_prsnt(slot, &status);
  if (ret < 0) {
     return -1;
  }
  if (status == 0) {
    return PAL_ENOTREADY;
  }

  ret = pal_get_server_12v_power(slot, &status);
  if(ret < 0 || status == SERVER_12V_OFF) {
    return PAL_ENOTREADY;
  }

  if(!pal_is_slot_server(slot)) {
    return PAL_ENOTSUP;
  }

  // Send command to get 80 port record from Bridge IC
  ret = bic_request_post_buffer_page_data(slot, page_num, res_data, &len);
  if (ret == 0)
    *res_len = (size_t)len;

  return ret;
}

int
pal_display_4byte_post_code(uint8_t slot, uint32_t postcode_dw) {

  // update current post code to debug card's SYS Info page
  pal_set_last_postcode(slot, postcode_dw);

  return 0;
}
#endif

int
pal_read_bic_sensor(uint8_t fru, uint8_t sensor_num, ipmi_extend_sensor_reading_t *sensor, uint8_t bmc_location, const uint8_t config_status) {
  static uint8_t board_type[MAX_NODES] = {UNKNOWN_BOARD, UNKNOWN_BOARD, UNKNOWN_BOARD, UNKNOWN_BOARD};
  int ret = 0;
  uint8_t server_type = 0;
  uint8_t card_type = TYPE_1OU_UNKNOWN;
  static SENSOR_BASE_NUM *sensor_base = NULL;

  if(sensor == NULL) {
    syslog(LOG_ERR, "%s: failed to read bic sensor due to NULL pointer\n", __func__);
    return READING_NA;
  }

  if (((config_status & PRESENT_2OU) == PRESENT_2OU) && (board_type[fru-1] == UNKNOWN_BOARD)) {
    ret = fby35_common_get_2ou_board_type(fru, &board_type[fru-1]);
    if (ret < 0) {
      syslog(LOG_ERR, "%s() Cannot get board_type", __func__);
    }
  }
  if((config_status & PRESENT_1OU) == PRESENT_1OU) {
    ret = bic_get_1ou_type(fru, &card_type);
    if (ret < 0) {
      syslog(LOG_ERR, "%s() Cannot get 1OU card_type", __func__);
    }
  }

  server_type = fby35_common_get_slot_type(fru);
  if (sensor_base == NULL) {
    switch (server_type) {
      case SERVER_TYPE_HD:
        if (card_type == TYPE_1OU_OLMSTEAD_POINT) {
          sensor_base = &sensor_base_hd_op;
        } else {
          sensor_base = &sensor_base_hd;
        }
        break;
      case SERVER_TYPE_CL:
        sensor_base = &sensor_base_cl;
        break;
      case SERVER_TYPE_GL:
        sensor_base = &sensor_base_gl;
        break;
      default:
        syslog(LOG_WARNING, "%s(): unknown board type", __func__);
        return READING_NA;
    }
  }

  //check snr number first. If it not holds, it will move on
  if (sensor_num < sensor_base->base_1ou || (((board_type[fru-1] & DPV2_X16_BOARD) == DPV2_X16_BOARD) && (board_type[fru-1] != UNKNOWN_BOARD) &&
      (sensor_num >= BIC_DPV2_SENSOR_DPV2_2_12V_VIN && sensor_num <= BIC_DPV2_SENSOR_DPV2_2_EFUSE_PWR))) { //server board
    ret = bic_get_sensor_reading(fru, sensor_num, sensor, NONE_INTF);
  } else if ( (sensor_num >= sensor_base->base_1ou && sensor_num < sensor_base->base_2ou) && (bmc_location != NIC_BMC) && //1OU
       ((config_status & PRESENT_1OU) == PRESENT_1OU) ) { // 1OU
    ret = bic_get_sensor_reading(fru, sensor_num, sensor, FEXP_BIC_INTF);
  } else if ( (sensor_num >= sensor_base->base_2ou && sensor_num < sensor_base->base_3ou) &&
              ((config_status & PRESENT_2OU) == PRESENT_2OU) ) { //2OU
    ret = bic_get_sensor_reading(fru, sensor_num, sensor, REXP_BIC_INTF);
  } else if ( (sensor_num >= sensor_base->base_3ou && sensor_num < sensor_base->base_4ou) &&
              ((config_status & PRESENT_3OU) == PRESENT_3OU) ) { //3OU
    ret = bic_get_sensor_reading(fru, sensor_num, sensor, EXP3_BIC_INTF);
  } else if ( (sensor_num >= sensor_base->base_4ou ) &&
              ((config_status & PRESENT_4OU) == PRESENT_4OU) ) { //4OU
    ret = bic_get_sensor_reading(fru, sensor_num, sensor, EXP4_BIC_INTF);
  } else if ( (sensor_num >= 0xD1 && sensor_num <= 0xF1) && (bmc_location == NIC_BMC)) { //BB
    ret = bic_get_sensor_reading(fru, sensor_num, sensor, BB_BIC_INTF);
  } else {
    return READING_NA;
  }

  if ( ret < 0 ) {
    syslog(LOG_WARNING, "%s() Failed to run bic_get_sensor_reading(). fru: %x, snr#0x%x", __func__, fru, sensor_num);
  }

  return ret;
}

int
pal_get_board_type(uint8_t slot_id, int *config_status, uint8_t *board_type) {
  int ret = 0;

  if((config_status == NULL) || (board_type == NULL)) {
    syslog(LOG_ERR, "%s: failed to get board type due to NULL pointer\n", __func__);
    return POWER_STATUS_ERR;
  }

  *config_status = bic_is_exp_prsnt(slot_id);
  if (*config_status < 0) {
    return POWER_STATUS_FRU_ERR;
  }

  if ((*config_status & PRESENT_1OU) == PRESENT_1OU) {
    ret = bic_get_1ou_type(slot_id, board_type);
    if (ret < 0) {
      syslog(LOG_ERR, "%s() Cannot get 1ou board_type", __func__);
      *board_type = M2_BOARD;
    }
  } else if ((*config_status & PRESENT_2OU) == PRESENT_2OU){
    if ( fby35_common_get_2ou_board_type(slot_id, board_type) < 0 ) {
      return POWER_STATUS_FRU_ERR;
    }
  } else {
    // dev not found
    return POWER_STATUS_FRU_ERR;
  }

  return PAL_EOK;
}

int
pal_oem_bios_extra_setup(uint8_t slot, uint8_t *req_data, uint8_t req_len, uint8_t *res_data, uint8_t *res_len) {
  char key[MAX_KEY_LEN] = {0};
  char cvalue[MAX_VALUE_LEN] = {0};
  int ret;
  uint8_t fun, cmd;
  uint8_t value;

  if (req_len < 5) { // at least byte[0] function byte[1] cmommand
    syslog(LOG_WARNING, "%s: slot%d req_len:%d < 5", __func__, slot,req_len);
    return CC_UNSPECIFIED_ERROR;
  }

  fun = req_data[0];

  if (fun == 0x1) { // PXE SEL ENABLE/DISABLE
    switch(slot) {
      case FRU_SLOT1:
      case FRU_SLOT2:
      case FRU_SLOT3:
      case FRU_SLOT4:
        sprintf(key, "slot%d_enable_pxe_sel", slot);
        break;

      default:
        syslog(LOG_WARNING, "%s: invalid slot id %d", __func__, slot);
        return CC_PARAM_OUT_OF_RANGE;
    }

    cmd = req_data[1];
    if (cmd == 0x1) { // GET
      *res_len = 1;
      ret = pal_get_key_value(key, cvalue);
      if (ret) {
        syslog(LOG_WARNING, "%s: slot%d get %s failed", __func__, slot,key);
        return CC_UNSPECIFIED_ERROR;
      }
      res_data[0] = strtol(cvalue,NULL,10);
      syslog(LOG_WARNING, "%s: slot%d GET PXE SEL ENABLE/DISABLE %d", __func__, slot, res_data[0]);
    } else if (cmd == 0x2) { // SET
      if (req_len < 6) {
        syslog(LOG_WARNING, "%s: slot%d SET no value to PXE SEL ENABLE/DISABLE", __func__, slot);
      }
      *res_len = 0;
      value = req_data[2];
      syslog(LOG_WARNING, "%s: slot%d SET %d to PXE SEL ENABLE/DISABLE", __func__, slot,value);
      sprintf(cvalue, (value > 0) ? "1": "0");
      ret = pal_set_key_value(key, cvalue);
      if (ret) {
        syslog(LOG_WARNING, "%s: slot%d set %s failed", __func__, slot,key);
        return CC_UNSPECIFIED_ERROR;
      }
    } else {
      syslog(LOG_WARNING, "%s: slot%d wrong command:%d", __func__, slot,cmd);
    }
    return CC_SUCCESS;
  } else {
    syslog(LOG_WARNING, "%s: slot%d wrong function:%d", __func__, slot,fun);
    return CC_UNSPECIFIED_ERROR;
  }
}

int
pal_get_post_complete(uint8_t slot_id, uint8_t *bios_post_complete) {
  uint8_t post_cmplt_pin = 0;
  int ret = -1, slot_type = SERVER_TYPE_NONE;
  char key[MAX_KEY_LEN] = {0}, value[MAX_VALUE_LEN] = {0};
  bic_gpio_t gpio = {0};

  if (bios_post_complete == NULL) {
    syslog(LOG_ERR, "%s: failed to get post complete value due to NULL pointer\n", __func__);
    return ret;
  }

  slot_type = fby35_common_get_slot_type(slot_id);

  switch (slot_type) {
    case SERVER_TYPE_CL:
    case SERVER_TYPE_HD:
      // Get BIOS complete pin
      if (slot_type == SERVER_TYPE_CL) {
        post_cmplt_pin = FM_BIOS_POST_CMPLT_BMC_N;
      } else {
        post_cmplt_pin = HD_FM_BIOS_POST_CMPLT_BIC_N;
      }

      // Get BIOS complete from BIC GPIO
      ret = bic_get_gpio(slot_id, &gpio, NONE_INTF);
      if ( ret < 0 ) {
        syslog(LOG_ERR, "%s() bic_get_gpio returns %d\n", __func__, ret);
        return ret;
      }

      *bios_post_complete = BIT_VALUE(gpio, post_cmplt_pin);
      break;
    case SERVER_TYPE_GL:
      // Get BIOS complete from key value
      snprintf(key, sizeof(key), POST_COMPLETE_STR, slot_id);
      ret = pal_get_key_value(key, value);
      if (ret < 0) {
        syslog(LOG_ERR, "%s() failed to get key value of %s\n", __func__, key);
        return ret;
      }

      *bios_post_complete = atoi(value);
      break;
    default:
      break;
  }

  return ret;
}

int
pal_add_apml_crashdump_record(uint8_t fru, uint8_t ras_status, uint8_t num_of_proc, uint8_t target_cpu, const uint32_t* cpuid) {
  int ret = 0;
  char cmd[128] = "\0";
  int cmd_len = sizeof(cmd);

  // Check if the crashdump script exist
  if (access(APMLDUMP_BIN, F_OK) == -1) {
    syslog(LOG_CRIT, "%s() Try to run apmldump for %d but %s is not existed", __func__, fru, APMLDUMP_BIN);
    return ret;
  }

  if (cpuid == NULL) {
    return -1;
  }

  snprintf(cmd, cmd_len, "%s slot%d %d %d %d %08X %08X %08X %08X &", APMLDUMP_BIN, fru, ras_status, num_of_proc, target_cpu, cpuid[0], cpuid[1], cpuid[2], cpuid[3]);

  if (system(cmd) != 0) {
    syslog(LOG_WARNING, "[%s] %s failed\n", __func__, cmd);
    return -1;
  }

  return 0;
}
