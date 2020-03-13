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
#include <time.h>
#include <stdlib.h>
#include <sys/time.h>
#include <openbmc/kv.h>
#include <openbmc/libgpio.h>
#include <openbmc/obmc-i2c.h>
#include "pal.h"

#define PLATFORM_NAME "yosemitev3"
#define LAST_KEY "last_key"

#define GUID_SIZE 16
#define OFFSET_SYS_GUID 0x17F0
#define OFFSET_DEV_GUID 0x1800

const char pal_fru_list_print[] = "all, slot1, slot2, slot3, slot4, bmc, nic, bb, nicexp";
const char pal_fru_list_rw[] = "slot1, slot2, slot3, slot4, bmc, bb, nicexp";
const char pal_fru_list_sensor_history[] = "all, slot1, slot2, slot3, slot4, bmc";

const char pal_fru_list[] = "all, slot1, slot2, slot3, slot4, bmc, nic";
const char pal_guid_fru_list[] = "slot1, slot2, slot3, slot4, bmc";
const char pal_server_list[] = "slot1, slot2, slot3, slot4";
const char pal_dev_list[] = "all, 1U, 2U, 1U-dev0, 1U-dev1, 1U-dev2, 1U-dev3, 2U-dev0, 2U-dev1, 2U-dev2, 2U-dev3, 2U-dev4, 2U-dev5";
const char pal_dev_pwr_option_list[] = "status, off, on, cycle";

#define MAX_NUM_DEVS 12

#define SYSFW_VER "sysfw_ver_slot"
#define SYSFW_VER_STR SYSFW_VER "%d"
#define BOOR_ORDER_STR "slot%d_boot_order"
#define GPIO_OCP_DEBUG_BMC_PRSNT_N "OCP_DEBUG_BMC_PRSNT_N"

#define CPLD_ADDRESS 0x1E
#define SLOT1_POSTCODE_OFFSET 0x02
#define SLOT2_POSTCODE_OFFSET 0x03
#define SLOT3_POSTCODE_OFFSET 0x04
#define SLOT4_POSTCODE_OFFSET 0x05
#define DEBUG_CARD_UART_MUX 0x06

enum key_event {
  KEY_BEFORE_SET,
  KEY_AFTER_INI,
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
  {"pwr_server1_last_state", "on", NULL},
  {"pwr_server2_last_state", "on", NULL},
  {"pwr_server3_last_state", "on", NULL},
  {"pwr_server4_last_state", "on", NULL},
  {"timestamp_sled", "0", NULL},
  {"slot1_por_cfg", "lps", NULL},
  {"slot2_por_cfg", "lps", NULL},
  {"slot3_por_cfg", "lps", NULL},
  {"slot4_por_cfg", "lps", NULL},
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
  {"ntp_server", "", NULL},
  {"debug_card_uart_select", "0", NULL},
  /* Add more Keys here */
  {LAST_KEY, LAST_KEY, NULL} /* This is the last key of the list */
};


MAPTOSTRING root_port_mapping[] = {
    { 0xB2, 3, "0", "1OU"}, //Port 0x4D
    { 0xB2, 2, "1", "1OU"}, //Port 0x4C
    { 0xB2, 1, "2", "1OU"}, //Port 0x4B
    { 0xB2, 0, "3", "1OU"}, //Port 0x4A
    { 0x15, 0, "0", "2OU"}, //Port 0x1A
    { 0x15, 1, "1", "2OU"}, //Port 0x1B
    { 0x63, 1, "2", "2OU"}, //Port 0x2B
    { 0x63, 0, "3", "2OU"}, //Port 0x2A
    { 0x15, 2, "4", "2OU"}, //Port 0x1C
    { 0x15, 3, "5", "2OU"}, //Port 0x1D
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
    strncat(str, tstr, 3);
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

  ret = fby3_common_is_fru_prsnt(slot_id, &status);
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

#if 0

static int
key_func_por_policy (int event, void *arg) {
  char value[MAX_VALUE_LEN] = {0};
  int ret = -1;

  switch (event) {
    case KEY_BEFORE_SET:
      if (pal_is_fw_update_ongoing(FRU_MB))
        return -1;
      // sync to env
      if ( !strcmp(arg,"lps") || !strcmp(arg,"on") || !strcmp(arg,"off")) {
        ret = fw_setenv("por_policy", (char *)arg);
      }
      else
        return -1;
      break;
    case KEY_AFTER_INI:
      // sync to env
      kv_get("server_por_cfg", value, NULL, KV_FPERSIST);
      ret = fw_setenv("por_policy", value);
      break;
  }

  return ret;
}

static int
key_func_lps (int event, void *arg)
{
  char value[MAX_VALUE_LEN] = {0};

  switch (event) {
    case KEY_BEFORE_SET:
      if (pal_is_fw_update_ongoing(FRU_MB))
        return -1;
      fw_setenv("por_ls", (char *)arg);
      break;
    case KEY_AFTER_INI:
      kv_get("pwr_server_last_state", value, NULL, KV_FPERSIST);
      fw_setenv("por_ls", value);
      break;
  }

  return 0;
}
#endif

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
pal_get_board_id(uint8_t slot, uint8_t *req_data, uint8_t req_len, uint8_t *res_data, uint8_t *res_len) {
  int ret = CC_UNSPECIFIED_ERROR;
  uint8_t *data = res_data;
  uint8_t bmc_location = 0; //the value of bmc_location is board id.
  *res_len = 0;

  ret = fby3_common_get_bmc_location(&bmc_location);
  if ( ret < 0 ) {
    syslog(LOG_WARNING, "%s() Cannot get the location of BMC", __func__);
    goto error_exit;
  }

  *data++ = bmc_location;
  *data++ = 0x00; //board rev id
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
  uint8_t bmc_location = 0;

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
      if ( bmc_location == BB_BMC ) {
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
  int ret = PAL_EOK;

  switch(fru) {
  case FRU_SLOT1:
    sprintf(name, "Server board 1");
    break;
  case FRU_SLOT2:
    sprintf(name, "Server board 2");
    break;
  case FRU_SLOT3:
    sprintf(name, "Server board 3");
    break;
  case FRU_SLOT4:
    sprintf(name, "Server board 4");
    break;
  case FRU_BMC:
    sprintf(name, "BMC");
    break;
  case FRU_BB:
    sprintf(name, "Baseboard");
    break;
  case FRU_NICEXP:
    sprintf(name, "NIC Expansion");
    break;
  case FRU_NIC:
    sprintf(name, "NIC");
    break;
  default:
    syslog(LOG_WARNING, "%s() wrong fru %d", __func__, fru);
    ret = PAL_ENOTSUP;
  }

  return ret;
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

  ret = fby3_common_get_bmc_location(&bmc_location);
  if ( ret < 0 ) {
    syslog(LOG_WARNING, "%s() Cannot get the location of BMC", __func__);
    return ret;
  }

  switch(fru) {
  case FRU_BMC:
    sprintf(path, EEPROM_PATH, (bmc_location == BB_BMC)?CLASS1_FRU_BUS:CLASS2_FRU_BUS, BMC_FRU_ADDR);
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
      *status = 1;
      //ret = fby3_common_is_bic_ready(fru, status);
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

// GUID based on RFC4122 format @ https://tools.ietf.org/html/rfc4122
static void
pal_populate_guid(char *guid, char *str) {
  unsigned int secs;
  unsigned int usecs;
  struct timeval tv;
  uint8_t count;
  uint8_t lsb, msb;
  int i, r;

  // Populate time
  gettimeofday(&tv, NULL);

  secs = tv.tv_sec;
  usecs = tv.tv_usec;
  guid[0] = usecs & 0xFF;
  guid[1] = (usecs >> 8) & 0xFF;
  guid[2] = (usecs >> 16) & 0xFF;
  guid[3] = (usecs >> 24) & 0xFF;
  guid[4] = secs & 0xFF;
  guid[5] = (secs >> 8) & 0xFF;
  guid[6] = (secs >> 16) & 0xFF;
  guid[7] = (secs >> 24) & 0x0F;

  // Populate version
  guid[7] |= 0x10;

  // Populate clock seq with randmom number
  srand(time(NULL));
  r = rand();
  guid[8] = r & 0xFF;
  guid[9] = (r>>8) & 0xFF;

  // Use string to populate 6 bytes unique
  // e.g. LSP62100035 => 'S' 'P' 0x62 0x10 0x00 0x35
  count = 0;
  for (i = strlen(str)-1; i >= 0; i--) {
    if (count == 6) {
      break;
    }

    // If alphabet use the character as is
    if (isalpha(str[i])) {
      guid[15-count] = str[i];
      count++;
      continue;
    }

    // If it is 0-9, use two numbers as BCD
    lsb = str[i] - '0';
    if (i > 0) {
      i--;
      if (isalpha(str[i])) {
        i++;
        msb = 0;
      } else {
        msb = str[i] - '0';
      }
    } else {
      msb = 0;
    }
    guid[15-count] = (msb << 4) | lsb;
    count++;
  }

  // zero the remaining bytes, if any
  if (count != 6) {
    memset(&guid[10], 0, 6-count);
  }

  return;
}

// GUID for System and Device
static int
pal_get_guid(uint16_t offset, char *guid) {
  char path[128] = {0};
  int fd;
  uint8_t bmc_location = 0;
  ssize_t bytes_rd;

  errno = 0;

  if ( fby3_common_get_bmc_location(&bmc_location) < 0 ) {
    syslog(LOG_ERR, "%s() Cannot get the location of BMC", __func__);
    return -1;
  }

  snprintf(path, sizeof(path), EEPROM_PATH, (bmc_location == BB_BMC)?CLASS1_FRU_BUS:CLASS2_FRU_BUS, BB_FRU_ADDR);

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
  ssize_t bytes_wr;

  errno = 0;

  if ( fby3_common_get_bmc_location(&bmc_location) < 0 ) {
    syslog(LOG_ERR, "%s() Cannot get the location of BMC", __func__);
    return -1;
  }

  snprintf(path, sizeof(path), EEPROM_PATH, (bmc_location == BB_BMC)?CLASS1_FRU_BUS:CLASS2_FRU_BUS, BB_FRU_ADDR);

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
  if (fru == FRU_BMC) {
    return pal_get_guid(OFFSET_DEV_GUID, guid);
  } else {
    return -1;
  }
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

int
pal_set_fw_update_state(uint8_t slot, uint8_t *req_data, uint8_t req_len, uint8_t *res_data, uint8_t *res_len) {
  *res_len = 0;
  if (req_len != 2) {
    return CC_INVALID_LENGTH;
  }

  if (pal_set_fw_update_ongoing(slot, (req_data[1]<<8 | req_data[0]))) {
    return CC_UNSPECIFIED_ERROR;
  }

  return CC_SUCCESS;
}

int pal_get_poss_pcie_config(uint8_t slot, uint8_t *req_data, uint8_t req_len, uint8_t *res_data, uint8_t *res_len) {
  uint8_t pcie_conf = 0xff;
  uint8_t *data = res_data;
  int ret = 0, config_status = 0;
  uint8_t bmc_location = 0;

  ret = fby3_common_get_bmc_location(&bmc_location);

  if (ret < 0) {
    syslog(LOG_ERR, "%s() Cannot get the location of BMC", __func__);
    return -1;
  }

  config_status = bic_is_m2_exp_prsnt(slot);

  if (bmc_location == BB_BMC) {
    if (config_status == 0) {
      pcie_conf = CONFIG_A;
    } else if (config_status == 1) {
      pcie_conf = CONFIG_B;
    } else if (config_status == 3) {
      pcie_conf = CONFIG_D;
    } else {
      pcie_conf = CONFIG_B;
    }
  } else {
      pcie_conf = CONFIG_C;
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
pal_parse_oem_unified_sel(uint8_t fru, uint8_t *sel, char *error_log)
{
  uint8_t general_info = (uint8_t) sel[3];
  uint8_t error_type = general_info & 0x0f;
  uint8_t plat;
  uint8_t dimm_failure_event = (uint8_t) sel[12];
  uint8_t mem_error_type;
  uint8_t channel;
  bool support_mem_mapping = false;
  char dimm_fail_event[][64] = {"Memory training failure", "Memory correctable error", "Memory uncorrectable error", "Reserved"};
  char mem_mapping_string[32];
  char temp_log[128] = {0};
  error_log[0] = '\0';
  int index = 0;
  char *sil = "NA";
  char *location = "NA";

  switch (error_type) {
    case UNIFIED_PCIE_ERR:
      plat = (general_info & 0x10) >> 4;
      if (plat == 0) {  //x86
        for (index = 0; index < (sizeof(root_port_mapping)/sizeof(MAPTOSTRING)); index++)
        {
          if ((sel[11] == root_port_mapping[index].bus_value) && ((sel[10] >> 3) == root_port_mapping[index].dev_value)) {
            location = root_port_mapping[index].location;
            sil = root_port_mapping[index].silk_screen;
            break;
          }
        }
        sprintf(error_log, "GeneralInfo: x86/PCIeErr(0x%02X), Bus %02X/Dev %02X/Fun %02X, %s/Num %s,\
                            TotalErrID1Cnt: 0x%04X, ErrID2: 0x%02X, ErrID1: 0x%02X",
                general_info, sel[11], sel[10] >> 3, sel[10] & 0x7, location, sil, ((sel[13]<<8)|sel[12]), sel[14], sel[15]);
      } else {
        sprintf(error_log, "GeneralInfo: ARM/PCIeErr(0x%02X), Aux. Info: 0x%04X, Bus %02X/Dev %02X/Fun %02X, \
                            TotalErrID1Cnt: 0x%04X, ErrID2: 0x%02X, ErrID1: 0x%02X",
                general_info, ((sel[9]<<8)|sel[8]),sel[11], sel[10] >> 3, sel[10] & 0x7, ((sel[13]<<8)|sel[12]), sel[14], sel[15]);
      }
      sprintf(temp_log, "PCIe Error ,FRU:%u", fru);
      pal_add_cri_sel(temp_log);
      break;
    case UNIFIED_MEM_ERR:
      plat = (dimm_failure_event & 0x80) >> 7;
      mem_error_type = (dimm_failure_event & 0x03);
      channel = (sel[9] & 0x0f);
      pal_parse_mem_mapping_string(channel, &support_mem_mapping, mem_mapping_string);

      switch (mem_error_type) {
        case MEMORY_TRAINING_ERR:
          if(support_mem_mapping) {
            if (plat == 0) { //Intel
              sprintf(error_log, "GeneralInfo: MEMORY_ECC_ERR(0x%02X), DIMM Slot Location: Sled %02X/Socket %02X, Channel %02X, Slot %02X, DIMM %s, \
                                DIMM Failure Event: %s, Major Code: 0x%02X, Minor Code: 0x%02X",
                    general_info, ((sel[8]>>4) & 0x03), sel[8] & 0x0f, sel[9] & 0x0f, sel[10] & 0x0f, mem_mapping_string, dimm_fail_event[sel[12]&0x03], sel[13], sel[14]);
            } else { //AMD
              int16_t minor_code = sel[15] << 8 | sel[14];
              sprintf(error_log, "GeneralInfo: MEMORY_ECC_ERR(0x%02X), DIMM Slot Location: Sled %02X/Socket %02X, Channel %02X, Slot %02X, DIMM %s, \
                                DIMM Failure Event: %s, Major Code: 0x%02X, Minor Code: 0x%04X",
                    general_info, ((sel[8]>>4) & 0x03), sel[8] & 0x0f, sel[9] & 0x0f, sel[10] & 0x0f, mem_mapping_string, dimm_fail_event[sel[12]&0x03], sel[13], minor_code);
            }
          } else {
            if (plat == 0) { //Intel
              sprintf(error_log, "GeneralInfo: MEMORY_ECC_ERR(0x%02X), DIMM Slot Location: Sled %02X/Socket %02X, Channel %02X, Slot %02X, \
                                DIMM Failure Event: %s, Major Code: 0x%02X, Minor Code: 0x%02X",
                    general_info, ((sel[8]>>4) & 0x03), sel[8] & 0x0f, sel[9] & 0x0f, sel[10] & 0x0f, dimm_fail_event[sel[12]&0x03], sel[13], sel[14]);
            } else { //AMD
              int16_t minor_code = sel[15] << 8 | sel[14];
              sprintf(error_log, "GeneralInfo: MEMORY_ECC_ERR(0x%02X), DIMM Slot Location: Sled %02X/Socket %02X, Channel %02X, Slot %02X, \
                                DIMM Failure Event: %s, Major Code: 0x%02X, Minor Code: 0x%04X",
                    general_info, ((sel[8]>>4) & 0x03), sel[8] & 0x0f, sel[9] & 0x0f, sel[10] & 0x0f, dimm_fail_event[sel[12]&0x03], sel[13], minor_code);
            }
          }
          break;
        case MEMORY_CORRECTABLE_ERR:
        case MEMORY_UNCORRECTABLE_ERR:
          if(support_mem_mapping) {
            sprintf(error_log, "GeneralInfo: MEMORY_ECC_ERR(0x%02X), DIMM Slot Location: Sled %02X/Socket %02X, Channel %02X, Slot %02X, DIMM %s, \
                          DIMM Failure Event: %s",
                general_info, ((sel[8]>>4) & 0x03), sel[8] & 0x0f, sel[9] & 0x0f, sel[10] & 0x0f, mem_mapping_string, dimm_fail_event[sel[12]&0x03]);
            if(mem_error_type == MEMORY_CORRECTABLE_ERR) {
              sprintf(temp_log, "DIMM%s ECC err,FRU:%u", mem_mapping_string, fru);
              pal_add_cri_sel(temp_log);
            } else {
              sprintf(temp_log, "DIMM%s UECC err,FRU:%u", mem_mapping_string, fru);
              pal_add_cri_sel(temp_log);
            }
          } else {
            sprintf(error_log, "GeneralInfo: MEMORY_ECC_ERR(0x%02X), DIMM Slot Location: Sled %02X/Socket %02X, Channel %02X, Slot %02X, \
                          DIMM Failure Event: %s",
                general_info, ((sel[8]>>4) & 0x03), sel[8] & 0x0f, sel[9] & 0x0f, sel[10] & 0x0f, dimm_fail_event[sel[12]&0x03]);
          }
          break;
        default:
          sprintf(error_log, "GeneralInfo: MEMORY_ECC_ERR(0x%02X), DIMM Slot Location: Sled %02X/Socket %02X, Channel %02X, Slot %02X, \
                        DIMM Failure Event: %s",
              general_info, ((sel[8]>>4) & 0x03), sel[8] & 0x0f, sel[9] & 0x0f, sel[10] & 0x0f, dimm_fail_event[sel[12]&0x03]);
          break;
      }
      break;
    case UNIFIED_POST_ERR:
      {
        char *post_failure_event[] = {"System PXE boot fail", "CMOS/NRAM configuration cleared", "TPM Self-Test Fail"};
        sprintf(error_log, "GeneralInfo: POST(0x%02X), FailureEvent: %s",
              error_type, post_failure_event[(sel[8]&0x0f)]);
      }
      break;
    default:
      sprintf(error_log, "Undefined Error Type(0x%02X), Raw: %02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X",
              error_type, sel[3], sel[4], sel[5], sel[6], sel[7], sel[8], sel[9], sel[10], sel[11], sel[12], sel[13], sel[14], sel[15]);
      break;
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
pal_get_uart_select_from_kv(uint8_t *pos) {
  char value[MAX_VALUE_LEN] = {0};
  uint8_t loc;
  int ret = -1;

  ret = pal_get_key_value("debug_card_uart_select", value);
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

  return fby3_common_crashdump(fru, ierr, false);
}

static int
pal_bic_sel_handler(uint8_t fru, uint8_t snr_num, uint8_t *event_data) {

  switch (snr_num) {
    case CATERR_B:
      pal_store_crashdump(fru, (event_data[3] == 0x00));  // 00h:IERR, 0Bh:MCERR
      break;
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
      *num = MAX_NUM_DEVS;
  }

  return 0;
}

int
pal_get_dev_name(uint8_t fru, uint8_t dev, char *name)
{
  char temp[64];
  int ret = fby3_get_fruid_name(fru, name);
  if (ret < 0) {
    return ret;
  }

  sprintf(temp, "%s Device %u", name, dev-1);
  strcpy(name, temp);
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
