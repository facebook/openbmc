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

#define PLATFORM_NAME "yosemitev35"
#define LAST_KEY "last_key"

#define OFFSET_SYS_GUID 0x17F0
#define OFFSET_DEV_GUID 0x1800

#define NUM_SERVER_FRU  4
#define NUM_NIC_FRU     1
#define NUM_BMC_FRU     1

#define FAN_FAIL_RECORD_PATH "/tmp/cache_store/fan_fail_boost"

const char pal_fru_list_print[] = "all, slot1, slot2, slot3, slot4, bmc, nic, bb, nicexp";
const char pal_fru_list_rw[] = "slot1, slot2, slot3, slot4, bmc, bb, nicexp";
const char pal_fru_list_sensor_history[] = "all, slot1, slot2, slot3, slot4, bmc nic";
const char pal_fru_list[] = "all, slot1, slot2, slot3, slot4, bmc, nic";
const char pal_guid_fru_list[] = "slot1, slot2, slot3, slot4, bmc";
const char pal_server_list[] = "slot1, slot2, slot3, slot4";
const char pal_dev_fru_list[] = "all, 1U, 2U, 1U-dev0, 1U-dev1, 1U-dev2, 1U-dev3, 2U-dev0, 2U-dev1, 2U-dev2, 2U-dev3, 2U-dev4, 2U-dev5, " \
                            "2U-dev6, 2U-dev7, 2U-dev8, 2U-dev9, 2U-dev10, 2U-dev11, 2U-dev12, 2U-dev13, 2U-X8, 2U-X16";
const char pal_dev_pwr_list[] = "all, 1U-dev0, 1U-dev1, 1U-dev2, 1U-dev3, 2U-dev0, 2U-dev1, 2U-dev2, 2U-dev3, 2U-dev4, 2U-dev5, " \
                            "2U-dev6, 2U-dev7, 2U-dev8, 2U-dev9, 2U-dev10, 2U-dev11, 2U-dev12, 2U-dev13";
const char pal_dev_pwr_option_list[] = "status, off, on, cycle";
const char *pal_server_fru_list[NUM_SERVER_FRU] = {"slot1", "slot2", "slot3", "slot4"};
const char *pal_nic_fru_list[NUM_NIC_FRU] = {"nic"};
const char *pal_bmc_fru_list[NUM_BMC_FRU] = {"bmc"};

static char sel_error_record[NUM_SERVER_FRU] = {0};

size_t server_fru_cnt = NUM_SERVER_FRU;
size_t nic_fru_cnt  = NUM_NIC_FRU;
size_t bmc_fru_cnt  = NUM_BMC_FRU;

#define SYSFW_VER "sysfw_ver_slot"
#define SYSFW_VER_STR SYSFW_VER "%d"
#define BOOR_ORDER_STR "slot%d_boot_order"
#define SEL_ERROR_STR  "slot%d_sel_error"
#define SNR_HEALTH_STR "slot%d_sensor_health"
#define GPIO_OCP_DEBUG_BMC_PRSNT_N "OCP_DEBUG_BMC_PRSNT_N"

#define SLOT1_POSTCODE_OFFSET 0x02
#define SLOT2_POSTCODE_OFFSET 0x03
#define SLOT3_POSTCODE_OFFSET 0x04
#define SLOT4_POSTCODE_OFFSET 0x05
#define DEBUG_CARD_UART_MUX 0x06

#define BB_CPLD_IO_BASE_OFFSET 0x16

#define ENABLE_STR "enable"
#define DISABLE_STR "disable"
#define STATUS_STR "status"
#define FAN_MODE_FILE "/tmp/cache_store/fan_mode"
#define FAN_MODE_STR_LEN 8 // include the string terminal

#define IPMI_GET_VER_FRU_NUM  5
#define IPMI_GET_VER_MAX_COMP 9
#define MAX_FW_VER_LEN        32  //include the string terminal

#define MAX_COMPONENT_LEN 32 //include the string terminal

#define BMC_CPLD_BUS     (12)
#define NIC_EXP_CPLD_BUS (9)
#define CPLD_FW_VER_ADDR (0x80)
#define BMC_CPLD_VER_REG (0x28002000)
#define SB_CPLD_VER_REG  (0x000000c0)
#define KEY_BMC_CPLD_VER "bmc_cpld_ver"

#define ERROR_LOG_LEN 256
#define ERR_DESC_LEN 64

static int key_func_pwr_last_state(int event, void *arg);
static int key_func_por_cfg(int event, void *arg);

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
  {LAST_KEY, LAST_KEY, NULL} /* This is the last key of the list */
};

MAPTOSTRING root_port_common_mapping[] = {
    // XCC
    { 0xB3, 1, 0x5A, "Num 0", "SB" },   // root_port=0x5A, Boot Drive
    { 0xB3, 5, 0x5E, "Class 1", "NIC"}, // root_port=0x5E, Class 1 NIC
    // MCC
    { 0xBB, 7, 0x57, "Num 0", "SB" },   // root_port=0x5G, Boot Drive
    { 0xBB, 5, 0x5E, "Num 0", "SB" },   // root_port=0x5E, Boot Drive
    { 0xBB, 1, 0x5A, "Class 1", "NIC"}, // root_port=0x5A, Class 1 NIC
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
    {0x53, "DPC triggered by uncorrectable error"},
    {0x54, "DPC triggered by ERR_NONFATAL"},
    {0x55, "DPC triggered by ERR_FATAL"},
    {0x59, "LER was triggered by ERR_NONFATAL"},
    {0x5A, "LER was triggered by ERR_FATAL"},
    {0xA0, "PERR (non-AER)"},
    {0xA1, "SERR (non-AER)"},
    {0xFF, "None"}
};

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
      *caps = FRU_CAPABILITY_SENSOR_HISTORY;
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
    default:
      ret = -1;
      break;
  }
  return ret;
}

int
pal_get_dev_capability(uint8_t fru, uint8_t dev, unsigned int *caps)
{
  if (fru < FRU_SLOT1 || fru > FRU_SLOT4)
    return -1;
  if (dev >= DEV_ID0_1OU && dev <= DEV_ID13_2OU) {
    *caps = FRU_CAPABILITY_FRUID_ALL | FRU_CAPABILITY_SENSOR_ALL |
      (FRU_CAPABILITY_POWER_ALL & (~FRU_CAPABILITY_POWER_RESET));
  } else if (dev >= BOARD_1OU && dev <= BOARD_2OU_X16) {
    *caps = FRU_CAPABILITY_FRUID_ALL | FRU_CAPABILITY_SENSOR_ALL;
  } else {
    *caps = 0;
  }
  return 0;
}

int
pal_get_board_id(uint8_t slot, uint8_t *req_data, uint8_t req_len, uint8_t *res_data, uint8_t *res_len) {
  int ret = CC_UNSPECIFIED_ERROR;
  uint8_t *data = res_data;
  uint8_t bmc_location = 0; //the value of bmc_location is board id.
  *res_len = 0;

  ret = fby35_common_get_bmc_location(&bmc_location);
  if ( ret < 0 ) {
    syslog(LOG_WARNING, "%s() Cannot get the location of BMC", __func__);
    goto error_exit;
  }

  *data++ = bmc_location;

  if ( bmc_location == BB_BMC ) {
    int dev, retry = 3;
    uint8_t tbuf[4] = {0};
    uint8_t rbuf[4] = {0};

    dev = open("/dev/i2c-12", O_RDWR);
    if ( dev < 0 ) {
      return -1;
    }

    while ((--retry) > 0) {
      tbuf[0] = 8;
      ret = i2c_rdwr_msg_transfer(dev, 0x1E, tbuf, 1, rbuf, 1);
      if (!ret)
        break;
      if (retry)
        msleep(10);
    }

    close(dev);
    if (ret) {
      *data++ = 0x00; //board rev id
    } else {
      *data++ = rbuf[0]; //board rev id
    }

  } else {
    // Config C can not get rev id form NIC EXP CPLD so far
    *data++ = 0x00; //board rev id
  }

  *data++ = slot; //slot id
  *data++ = 0x00; //slot type. server = 0x00
  *res_len = data - res_data;
  ret = CC_SUCCESS;

error_exit:

  return ret;
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
  uint8_t bmc_location = 0;

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
      if ( bmc_location == BB_BMC ) {
        ret = fby35_common_is_fru_prsnt(fru, status);
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
        snprintf(path, path_len, EEPROM_PATH, FRU_DPV2_X8_BUS(fru), DPV2_FRU_ADDR);
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

  ret = bic_is_m2_exp_prsnt(fru);
  if ( ret < 0 ) {
    printf("%s() Couldn't get the status of 1OU/2OU\n", __func__);
    return ret;
  }

  config_status = (uint8_t) ret;

  if ( (dev_id == BOARD_1OU) && ((config_status & PRESENT_1OU) == PRESENT_1OU) && (bmc_location != NIC_BMC) ) { // 1U
    return bic_write_fruid(fru, 0, path, FEXP_BIC_INTF);
  } else if ( (config_status & PRESENT_2OU) == PRESENT_2OU ) {
    if ( fby35_common_get_2ou_board_type(fru, &type_2ou) < 0 ) {
      syslog(LOG_WARNING, "%s() Failed to get 2OU board type\n", __func__);
    } else if ( dev_id == BOARD_2OU_X16 && ((type_2ou & DPV2_X16_BOARD) == DPV2_X16_BOARD)) {
      return bic_write_fruid(fru, 1, path, NONE_INTF);
    } else if ( dev_id == BOARD_2OU ) {
      return bic_write_fruid(fru, 0, path, REXP_BIC_INTF);
    } else if ( dev_id >= DEV_ID0_2OU && dev_id <= DEV_ID11_2OU ) {
      return bic_write_fruid(fru, dev_id - DEV_ID0_2OU + 1, path, REXP_BIC_INTF);
    } else {
      printf("Dev%d is not supported on 2OU!\n", dev_id);
    }
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

int pal_get_poss_pcie_config(uint8_t slot, uint8_t *req_data, uint8_t req_len, uint8_t *res_data, uint8_t *res_len) {
  uint8_t pcie_conf = 0xff;
  uint8_t *data = res_data;
  int ret = 0, config_status = 0;
  uint8_t bmc_location = 0;
  uint8_t type_1ou = 0;
  uint8_t type_2ou = UNKNOWN_BOARD;

  ret = fby35_common_get_bmc_location(&bmc_location);
  if ( ret < 0 ) {
    syslog(LOG_ERR, "%s() Cannot get the location of BMC", __func__);
    return -1;
  }

  config_status = bic_is_m2_exp_prsnt(slot);
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
    switch (config_status & (PRESENT_2OU|PRESENT_1OU)) {
      case 0:
        pcie_conf = CONFIG_A;
        break;
      case PRESENT_1OU:
        if ( bic_get_1ou_type(slot, &type_1ou) != 0 ) {
          pcie_conf = CONFIG_C;
          break;
        }
        switch (type_1ou) {
          case EDSFF_1U:
          case M2_1U:
            pcie_conf = CONFIG_MFG;
            break;
          case WF_1U:
            pcie_conf = CONFIG_C_WF;
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

  *data++ = pcie_conf;
  *res_len = data - res_data;
  return ret;
}

int
pal_is_slot_server(uint8_t fru)
{
  if ( SERVER_TYPE_DL == fby35_common_get_slot_type(fru) ) {
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

static void
pal_sel_root_port_mapping_tbl(uint8_t fru, uint8_t *bmc_location, MAPTOSTRING **tbl, uint8_t *cnt) {
  uint8_t board_1u = M2_BOARD;
  uint8_t board_2u = M2_BOARD;
  uint8_t config_status = CONFIG_UNKNOWN;
  int ret = 0;

  do {
    ret = fby35_common_get_bmc_location(bmc_location);
    if ( ret < 0 ) {
      syslog(LOG_WARNING, "%s() Cannot get the location of BMC\n", __func__);
      break;
    }

    ret = bic_is_m2_exp_prsnt(fru);
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
    board_1u = M2_BOARD; //make sure the default is used
    board_2u = M2_BOARD;
  }

  if ( board_1u == EDSFF_1U || board_2u == E1S_BOARD ) {
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
    SYS_HSC_THROTTLE   = 0x05,
    SYS_OC_DETECT      = 0x06,
    SYS_MB_THROTTLE    = 0x07,
    SYS_HSC_FAULT      = 0x08,
    SYS_RSVD           = 0x09,
    SYS_WDT_TIMEOUT    = 0x0A,
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
    E1S_1OU_HSC_PWR_ALERT = 0x82,
  };
  uint8_t event = event_data[0];
  char log_msg[MAX_ERR_LOG_SIZE] = {0};
  char fan_mode_str[FAN_MODE_STR_LEN] = {0};
  char component_str[MAX_COMPONENT_LEN] = {0};

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
    case SYS_WDT_TIMEOUT:
      strcat(error_log, "VR Watchdog timeout");
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
    case E1S_1OU_HSC_PWR_ALERT:
      strcat(error_log, "E1S 1OU HSC power alert");
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
  const uint8_t MAX_RETRY = 3;
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
  int i = 0;
  bool is_cri_sel = false;

  switch (snr_num) {
    case CATERR_B:
      is_cri_sel = true;
      pal_store_crashdump(fru, (event_data[3] == 0x00));  // 00h:IERR, 0Bh:MCERR
      if (event_data[3] == 0x00) { // IERR
        fby35_common_fscd_ctrl((event_data[2] == SEL_ASSERT) ? FAN_MANUAL_MODE : FAN_AUTO_MODE);
        if (event_data[2] == SEL_ASSERT) {
          for (i = 0; i < pal_pwm_cnt; i++) {
            pal_set_fan_speed(i, 100);
          }
        }
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
        ret = bic_ipmb_wrapper(slot, netfn, cmd, &req_data[3], tlen, res_data, res_len);
      } else {
        ret = bic_ipmb_wrapper(slot, netfn, cmd, NULL, 0, res_data, res_len);
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
  const char *gpio_mgmt_cbl_tbl[] = {"SLOT%d_ID0_DETECT_BMC_N", "SLOT%d_ID1_DETECT_BMC_N"};
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
    ret = fby35_common_get_bus_id(slot_id) + 4;
    if ( ret < 0 ) {
      syslog(LOG_WARNING, "%s() Cannot get the bus with fru%d", __func__, slot_id);
      return -1;
    }

    bus = (uint8_t)ret;
    i2cfd = i2c_cdev_slave_open(bus, SB_CPLD_ADDR, I2C_SLAVE_FORCE_CLAIM);
    if ( i2cfd < 0 ) {
      syslog(LOG_WARNING, "%s() Failed to open bus %d. Err: %s", __func__, bus, strerror(errno));
      return -1;
    }

    //read 06h from SB CPLD
    ret = i2c_rdwr_msg_transfer(i2cfd, (SB_CPLD_ADDR << 1), tbuf, tlen, rbuf, rlen);
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

  bool vals_match = (bmc_location == BB_BMC) ? (gpio_vals == mapping_tbl[slot_id-1]):(gpio_vals == cpld_slot_cbl_val);
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
      ret = bic_is_m2_exp_prsnt(fru);
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
      ret = bic_is_m2_exp_prsnt(fru);
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
  case FW_BIC:
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

  ret = bic_get_gpio(slot_id, &gpio, NONE_INTF);
  if ( ret < 0 ) {
    printf("%s() bic_get_gpio returns %d\n", __func__, ret);
    return ret;
  }

  if (BIT_VALUE(gpio, FM_CPU_SKTOCC_LVT3_PLD_N)) {
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
      "/usr/bin/fw-util slot1 --version vr | grep 'VCCIN' | awk '{print $5}' | cut -c -8",
      "/usr/bin/fw-util slot1 --version vr | grep 'VCCD' | awk '{print $5}' | cut -c -8",
      "/usr/bin/fw-util slot1 --version vr | grep 'VCCINFAON' | awk '{print $5}' | cut -c -8"
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

  ret = fby35_common_get_2ou_board_type(slot, &type_2ou);
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

int
pal_check_slot_fru(uint8_t slot_id) {
  int ret = 0, i2cfd = 0 ,retry = 0;
  uint8_t bus = slot_id + SLOT_BUS_BASE;
  uint8_t tbuf[1] = {0x11};
  uint8_t rbuf[1] = {0xff};
  uint8_t tlen = 1;
  uint8_t rlen = 1;

  i2cfd = i2c_cdev_slave_open(bus, CPLD_ADDRESS >> 1, I2C_SLAVE_FORCE_CLAIM);
  if ( i2cfd < 0) {
    printf("%s(): Failed to open bus %d. Err: %s\n", __func__, bus, strerror(errno));
    goto error_exit;
  }

  while ( retry < MAX_READ_RETRY ) {
    ret = i2c_rdwr_msg_transfer(i2cfd, CPLD_ADDRESS, tbuf, tlen, rbuf, rlen);
    if ( ret < 0 ) {
      retry++;
      sleep(1);
    } else {
      break;
    }
  }
  if ( retry == MAX_READ_RETRY ) {
    syslog(LOG_WARNING, "%s() Failed to do i2c_rdwr_msg_transfer, tlen=%d", __func__, tlen);
    goto error_exit;
  }

  if ( rbuf[0] != 0x0 ) {
    syslog(LOG_CRIT, "Slot%d plugged in a wrong FRU", slot_id);
  }

error_exit:
  if ( i2cfd > 0 ) close(i2cfd);
  return ret;
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
