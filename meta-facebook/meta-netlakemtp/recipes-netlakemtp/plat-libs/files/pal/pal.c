/*
 *
 * Copyright 2020-present Facebook. All Rights Reserved.
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
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <stdbool.h>
#include <errno.h>
#include <syslog.h>
#include <string.h>
#include <openbmc/libgpio.h>
#include <openbmc/phymem.h>
#include <openbmc/obmc-sensors.h>
#include <openbmc/kv.h>
#include <openbmc/obmc-i2c.h>
#include "pal.h"
#include "pal_sensors.h"

#define GUID_SIZE 16
#define OFFSET_DEV_GUID 0x1800
#define MAX_FAN_NAME_LEN 32
#define MAX_FAN_CONTROLLER_LEN 32
#define KEY_SERVER_CPLD_VER "server_cpld_ver"
#define MAX_NUM_GPIO_LED_POSTCODE 8

const char pal_fru_list[] = "all, server, bmc, pdb, fio";

// export to sensor-util
const char pal_fru_list_sensor_history[] = "all, server, bmc, pdb, fio";
// fru name list for pal_get_fru_id()
const char *fru_str_list[] = {"all", "server", "bmc", "pdb", "fio"};

size_t pal_pwm_cnt = 1;
size_t pal_tach_cnt = 4;
const int fan_map[] = {1, 3, 5, 7};
const int fanIdToPwmIdMapping[] = {0, 0, 0, 0};

enum key_event {
  KEY_BEFORE_SET,
  KEY_AFTER_INI,
};

char* GPIO_LED_POSTCODE_TABLE[MAX_NUM_GPIO_LED_POSTCODE] = {
  "LED_POSTCODE_0",
  "LED_POSTCODE_1",
  "LED_POSTCODE_2",
  "LED_POSTCODE_3",
  "LED_POSTCODE_4",
  "LED_POSTCODE_5",
  "LED_POSTCODE_6",
  "LED_POSTCODE_7",
};

struct pal_key_cfg {
  char *name;
  char *def_val;
  int (*function)(int, void*);
} key_cfg[] = {
  /* name, default value, function */
  {"server_sensor_health", "1", NULL},
  {"bmc_sensor_health", "1", NULL},
  {"pdb_sensor_health", "1", NULL},
  {"fio_sensor_health", "1", NULL},
  {"sysfw_ver_server", "0", NULL},
  {"system_identify_led", "off", NULL},
  {"server_power_on_reset", "lps", NULL},
  {"server_last_power_state", "on", NULL},
  {"ntp_server", "", NULL},
  {"server_boot_order", "0100090203ff", NULL},
  /* Add more Keys here */
  {NULL, NULL, NULL} /* This is the last key of the list */
};

/**
*  @brief Function of getting FRU ID from FRU name list
*
*  @param *str: string which the FRU was called
*  @param *str: return value of FRU ID
*
*  @return Status of getting FRU ID
*  0: Found
*  -1: Not found
**/
int
pal_get_fru_id(char *str, uint8_t *fru) {
  int fru_id = -1;
  bool is_id_exist = false;

  if (fru == NULL)
  {
    syslog(LOG_ERR, "%s() Pointer \"fru\" is NULL.\n", __func__);
    return -1;
  }

  if (str == NULL)
  {
    syslog(LOG_ERR, "%s() Pointer \"str\" is NULL.\n", __func__);
    return -1;
  }

  for (fru_id = FRU_ALL; fru_id <= MAX_NUM_FRUS; fru_id++) {
    if (strncmp(str, fru_str_list[fru_id], MAX_FRU_CMD_STR) == 0) {
      *fru = fru_id;
      is_id_exist = true;
      break;
    }
  }

  return is_id_exist ? 0 : -1;
}

/**
*  @brief Function of checking FRU is ready to print/dump/write/modify
*
*  @param fru: FRU ID
*  @param *status: FRU status
*  0: not ready
*  1: ready
*
*  @return Status of checking FRU is ready to print/dump/write/modify
*  0: Success
*  PAL_ENOTSUP: Wrong FRU ID
**/
int
pal_is_fru_ready(uint8_t fru, uint8_t *status) {

  if(status == NULL)
  {
    syslog(LOG_ERR, "%s() Pointer \"status\" is NULL.\n", __func__);
    return -1;
  }

  switch (fru) {
    case FRU_SERVER:
    case FRU_BMC:
    case FRU_PDB:
    case FRU_FIO:
      *status = 1;
      break;
    default:
      *status = 0;
      syslog(LOG_WARNING, "%s() wrong fru id 0x%02x", __func__, fru);
      return PAL_ENOTSUP;
  }

  return 0;
}

/**
*  @brief Function of checking FRU is presnet
*
*  @param fru: FRU ID
*  @param *status: return variable of FRU status
*  1: present
*
*  @return Status of checking FRU is presnet
*  0: pass
**/
int
pal_is_fru_prsnt(uint8_t fru, uint8_t *status) {

  if(status == NULL)
  {
    syslog(LOG_ERR, "%s() Pointer \"status\" is NULL.\n", __func__);
    return -1;
  }

  *status = 1;
  return 0;
}

/**
*  @brief Function of getting fru ID name by FRU ID
*
*  @param fru: FRU ID
*  @param *name: return variable of FRU ID name
*
*  @return Status of getting FRU ID name
*  0: Success
*  -1: Wrong FRU ID
**/
int
pal_get_fruid_name(uint8_t fru, char *name) {
  return netlakemtp_get_fruid_name(fru, name);
}

/**
*  @brief Function of getting FRU temp binary path by FRU ID
*
*  @param fru: FRU ID
*  @param *path: return variable of FRU temp binary path
*
*  @return Status of getting FRU temp binary path
*  0: Success
*  -1: Wrong FRU ID
**/
int
pal_get_fruid_path(uint8_t fru, char *path) {
  return netlakemtp_get_fruid_path(fru, path);
}

/**
*  @brief Function of getting FRU EEPROM path by FRU ID
*
*  @param fru: FRU ID
*  @param *name: return variable of FRU EEPROM binary path
*
*  @return Status of getting FRU EEPROM path
*  0: Success
*  -1: Wrong FRU ID
**/
int
pal_get_fruid_eeprom_path(uint8_t fru, char *path) {
  return netlakemtp_get_fruid_eeprom_path(fru, path);
}

/**
*  @brief Function of getting FRU's capability by FRU ID
*
*  @param fru: FRU ID
*  @param *caps: return variable of FRU's capability
*  Bit [1:0]: FRU have a FRUID EEPROM to write/read
*  Bit [2:4]: Sensors on this FRU
*  Bit [5]: Server capability
*  Bit [6]: NIC capability
*  Bit [7]: FRU containing the BMC
*  Bit [8:17]: FRU supports power control
*  Bit [18]: FRU/device contains one or more complex device on its board
*
*  @return Status of of getting FRU's capability
*  0: Success
*  -1: Failed
**/
int
pal_get_fru_capability(uint8_t fru, unsigned int *caps)
{
  int ret = 0;

  if(caps == NULL)
  {
    syslog(LOG_ERR, "%s() Pointer \"caps\" is NULL.\n", __func__);
    return -1;
  }

  switch (fru) {
    case FRU_SERVER:
      *caps = (FRU_CAPABILITY_FRUID_ALL | FRU_CAPABILITY_SENSOR_ALL | FRU_CAPABILITY_POWER_ALL);
      break;
    case FRU_BMC:
      *caps = (FRU_CAPABILITY_FRUID_ALL | FRU_CAPABILITY_SENSOR_ALL);
      break;
    case FRU_PDB:
      *caps = FRU_CAPABILITY_SENSOR_ALL;
      break;
    case FRU_FIO:
      *caps = FRU_CAPABILITY_SENSOR_ALL;
      break;
    default:
      ret = -1;
      break;
  }
  return ret;
}

/**
*  @brief Function of getting FRU name for creating FRU lists in fruid-util by FRU ID
*
*  @param fru: FRU ID
*  @param *name: return variable of FRU name
*
*  @return Status of of getting FRU's capability
*  0: Success
*  -1: Failed
**/
int
pal_get_fru_name(uint8_t fru, char *name) {

  if(name == NULL)
  {
    syslog(LOG_ERR, "%s() Pointer \"name\" is NULL.\n", __func__);
    return -1;
  }

  switch(fru) {
    case FRU_SERVER:
      snprintf(name, MAX_FRU_CMD_STR, "server");
      break;
    case FRU_BMC:
      snprintf(name, MAX_FRU_CMD_STR, "bmc");
      break;
    case FRU_PDB:
      snprintf(name, MAX_FRU_CMD_STR, "pdb");
      break;
    case FRU_FIO:
      snprintf(name, MAX_FRU_CMD_STR, "fio");
      break;
   default:
      if (fru > MAX_NUM_FRUS) {
        return -1;
      }
      snprintf(name, MAX_FRU_CMD_STR, "fru%d", fru);
      break;
  }

  return 0;
}

/**
*  @brief Function to copy EEPROM's content to given binary path
*
*  @param *eeprom_file: EEPROM device path
*  @param *bin_file: return variable of given path to save EEPROM content
*
*  @return Status of copying EEPROM's content to given binary path
*  0: Success
*  -1: Failed
**/
int
pal_copy_eeprom_to_bin(const char *eeprom_file, const char *bin_file) {
  int eeprom = 0;
  int bin = 0;
  int ret = 0;
  uint8_t tmp[FRUID_SIZE] = {0};
  ssize_t bytes_rd = 0, bytes_wr = 0;

  errno = 0;

  if (eeprom_file == NULL || bin_file == NULL) {
    syslog(LOG_ERR, "%s: invalid parameter", __func__);
    return -1;
  }

  eeprom = open(eeprom_file, O_RDONLY);
  if (eeprom < 0) {
    syslog(LOG_ERR, "%s: unable to open the %s file: %s", __func__, eeprom_file, strerror(errno));
    return -1;
  }

  bin = open(bin_file, O_WRONLY | O_CREAT, 0644);
  if (bin < 0) {
    syslog(LOG_ERR, "%s: unable to create %s file: %s", __func__, bin_file, strerror(errno));
    ret = -1;
    goto err;
  }

  bytes_rd = read(eeprom, tmp, FRUID_SIZE);
  if (bytes_rd < 0) {
    syslog(LOG_ERR, "%s: read %s file failed: %s", __func__, eeprom_file, strerror(errno));
    ret = -1;
    goto exit;
  } else if (bytes_rd < FRUID_SIZE) {
    syslog(LOG_ERR, "%s: less than %d bytes", __func__, FRUID_SIZE);
    ret = -1;
    goto exit;
  }

  bytes_wr = write(bin, tmp, bytes_rd);
  if (bytes_wr != bytes_rd) {
    syslog(LOG_ERR, "%s: write to %s file failed: %s",
	__func__, bin_file, strerror(errno));
    ret = -1;
  }

exit:
  close(bin);
err:
  close(eeprom);

  return ret;
}

/**
*  @brief Function of checking FRU EEPROM path by FRU ID
*
*  @param * bin_file: binary path
*
*  @return Status of getting FRU EEPROM path
*  0: Success
*  -1: Failed to check/Invalid FRU
**/
int
pal_check_fru_is_valid(const char* fruid_path) {

  if (fruid_path == NULL) {
    syslog(LOG_ERR, "%s: Failed to check FRU header is valid or not because NULL parameter.", __func__);
    return -1;
  }

  return netlakemtp_check_fru_is_valid(fruid_path);
}

int
pal_is_slot_server(uint8_t fru) {
  return (fru == FRU_SERVER) ? 1 : 0;
}

void
pal_dump_key_value(void) {
  int i = 0;
  char value[MAX_VALUE_LEN] = {0x0};

  while(key_cfg[i].name != NULL) {
    memset(value, 0, MAX_VALUE_LEN);

    printf("%s:", key_cfg[i].name);
    if (kv_get(key_cfg[i].name, value, NULL, KV_FPERSIST) < 0) {
      printf("\n");
    } else {
      printf("%s\n",  value);
    }
    i++;
  }
}

/**
*  @brief Function of setting default key and value with key config
*
*  @return Status of setting default key and value with key config
*  0: Success
*  -1: Failed
**/
int
pal_set_def_key_value() {
  int i = 0;
  int ret = 0, failed_count = 0;

  for (i = 0; key_cfg[i].name != NULL; i++) {
    if ((ret = kv_set(key_cfg[i].name, key_cfg[i].def_val, 0, KV_FCREATE | KV_FPERSIST)) < 0) {
      // Ignore the error messages when the kv node already existed.
      if (errno != EEXIST) {
        syslog(LOG_WARNING, "%s(): kv_set failed, errno=%d, ret=%d.", __func__, errno, ret);
        failed_count ++;
      }
    }
    if (key_cfg[i].function) {
      key_cfg[i].function(KEY_AFTER_INI, key_cfg[i].name);
    }
  }

  if (failed_count != 0) {
    return -1;
  }

  return 0;
}

/**
*  @brief Function of finding key from key config list
*
*  @param *key: key
*
*  @return Index number of key in config list
*  i: Index number of key in config list
**/
static int
pal_key_index(char *key) {
  int i = 0;

  if(key == NULL)
  {
    syslog(LOG_ERR, "%s() Pointer \"key\" is NULL.\n", __func__);
    return -1;
  }

  while(key_cfg[i].name != NULL) {
    // If Key is valid, return index
    if (!strncmp(key, key_cfg[i].name, strlen(key_cfg[i].name))) {
      return i;
    }
    i++;
  }

#ifdef DEBUG
  syslog(LOG_WARNING, "%s() invalid key - %s", __func__, key);
#endif
  return -1;
}

/**
*  @brief Function of setting key value in kv_store
*
*  @param *key: key
*  @param *value: value
*
*  @return Status of setting key value in kv_store
*  0: Success
*  -1: Failed
**/
int
pal_set_key_value(char *key, char *value) {
  int index = 0, ret = 0;
  // Check key is defined and valid
  if ((index = pal_key_index(key)) < 0) {
    return -1;
  }
  if (key_cfg[index].function) {
    ret = key_cfg[index].function(KEY_BEFORE_SET, value);
    if (ret < 0) {
      return ret;
    }
  }

  return kv_set(key, value, 0, KV_FPERSIST);
}

/**
*  @brief Function of getting key value in kv_store
*
*  @param *key: key
*  @param *value: return variable of value
*
*  @return Status of getting key value in kv_store
*  0: Success
*  -1: Failed
**/
int
pal_get_key_value(char *key, char *value) {
  int index = 0;

  // Check key is defined and valid
  if ((index = pal_key_index(key)) < 0) {
    return -1;
  }
  return kv_get(key, value, NULL, KV_FPERSIST);
}

/**
*  @brief Function of getting FRU health sensor status
*
*  @param fru: FRU ID
*  @param *value: return value of FRU health sensor kv value
*
*  @return Status of getting FRU health sensor status
*  0: Success
*  -1: Failed
**/
int
pal_get_fru_health(uint8_t fru, uint8_t *value) {
  char val[MAX_VALUE_LEN] = {0};
  char key[MAX_KEY_LEN] = {0};
  int ret = 0;

  if (value == NULL) {
    syslog(LOG_WARNING, "%s(): failed to get fru health because the parameter: *value is NULL", __func__);
  }

  memset(key, 0, sizeof(key));
  memset(val, 0, sizeof(val));

  switch (fru) {
    case FRU_SERVER:
      snprintf(key, sizeof(key), "server_sensor_health");
      break;
    case FRU_BMC:
      snprintf(key, sizeof(key), "bmc_sensor_health");
      break;
    case FRU_PDB:
      snprintf(key, sizeof(key), "pdb_sensor_health");
      break;
    case FRU_FIO:
      snprintf(key, sizeof(key), "fio_sensor_health");
      break;
    default:
      return -1;
  }

  ret = pal_get_key_value(key, val);
  if (ret < 0) {
    syslog(LOG_WARNING, "%s(): failed to get the fru health because get the value of key: %s failed", __func__, key);
    return ret;
  }

  *value = atoi(val);
  return 0;
}

/**
*  @brief Function of setting sensor health status
*
*  @param fru: FRU ID
*  @param value: setting status
*
*  @return Status of setting sensor health status
*  0: Success
*  -1: Failed
**/
int
pal_set_sensor_health(uint8_t fru, uint8_t value) {
  char val[MAX_VALUE_LEN] = {0};
  char key[MAX_KEY_LEN] = {0};
  int ret = 0;

  memset(key, 0, sizeof(key));
  memset(val, 0, sizeof(val));

  switch (fru) {
    case FRU_SERVER:
      snprintf(key, sizeof(key), "server_sensor_health");
      break;
    case FRU_BMC:
      snprintf(key, sizeof(key), "bmc_sensor_health");
      break;
    case FRU_PDB:
      snprintf(key, sizeof(key), "pdb_sensor_health");
      break;
    case FRU_FIO:
      snprintf(key, sizeof(key), "fio_sensor_health");
      break;
    default:
      return -1;
  }

  if (value == FRU_STATUS_BAD) {
    snprintf(val, sizeof(val), "%d", FRU_STATUS_BAD);
  } else if (value == FRU_STATUS_GOOD) {
    snprintf(val, sizeof(val), "%d", FRU_STATUS_GOOD);
  } else {
    syslog(LOG_WARNING, "%s(): failed to set sensor health status because unexpected status: %d", __func__, value);
    return -1;
  }

  ret = pal_set_key_value(key, val);
  if (ret < 0) {
    syslog(LOG_WARNING, "%s(): failed to set sensor health because set key: %s value: %s failed", __func__, key, val);
  }
  return ret;
}

/**
*  @brief Function of get FRU list
*
*  @param list: FRU list
*  @param value: setting sgit difftatus
*
*  @return Status of this function
*  0: Success
**/
int
pal_get_fru_list(char *list) {

  if(list == NULL)
  {
    syslog(LOG_ERR, "%s() Pointer \"list\" is NULL.\n", __func__);
    return -1;
  }

  snprintf(list, sizeof(pal_fru_list), pal_fru_list);

  return 0;
}

int
pal_get_num_slots(uint8_t *num) {

  if(num == NULL)
  {
    syslog(LOG_ERR, "%s() Pointer \"num\" is NULL.\n", __func__);
    return -1;
  }

  *num = MAX_NODES;

  return 0;
}

int
pal_get_fan_name(uint8_t fan_id, char *name) {

  if (name == NULL)
  {
    syslog(LOG_ERR, "%s() Pointer \"name\" is NULL.\n", __func__);
    return -1;
  }

  if (fan_id >= pal_tach_cnt) {
    syslog(LOG_WARNING, "%s: Invalid fan index: %d, fan count: %d", __func__, fan_id, pal_tach_cnt);
    return -1;
  }

  snprintf(name, MAX_FAN_NAME_LEN, "Fan%d", fan_id);
  return 0;
}

int
pal_get_fan_speed(uint8_t fan_id, int *rpm) {
  int ret = 0;
  float value = 0.0;
  char fan_label[8] = {0};
  char fan_controller[MAX_FAN_CONTROLLER_LEN] = {0};

  if (rpm == NULL)
  {
    syslog(LOG_ERR, "%s() Pointer \"rpm\" is NULL.\n", __func__);
    return -1;
  }

  if (fan_id >= pal_tach_cnt) {
    syslog(LOG_WARNING, "%s: Invalid fan index: %d", __func__, fan_id);
    return -1;
  }

  snprintf(fan_label, sizeof(fan_label), "fan%d", fan_map[fan_id]);
  snprintf(fan_controller, sizeof(fan_controller), "%s%d-%d", "max31790-i2c-", FAN_CTL_BUS, FAN_CTL_ADDR);
  ret = sensors_read(fan_controller, fan_label, &value);
  if (ret != 0 ) {
    syslog(LOG_WARNING, "%s: Fan sensor read fail: %s", __func__, fan_label);
  }
  else {
    *rpm = (int)value;
  }

  return ret;
}

int
pal_get_pwm_value(uint8_t fan_id, uint8_t *pwm_value) {
  float value = 0;
  int ret = 0;
  int pwm_id = 0;

  if (pwm_value == NULL)
  {
    syslog(LOG_ERR, "%s() Pointer \"pwm_value\" is NULL.\n", __func__);
    return -1;
  }

  if (fan_id >= pal_tach_cnt) {
    syslog(LOG_WARNING, "%s: Invalid fan index: %d", __func__, fan_id);
    return -1;
  }

  pwm_id = fanIdToPwmIdMapping[fan_id];
  ret = sensors_read_pwmfan(pwm_id, &value);
  if (ret != 0 ) {
    syslog(LOG_WARNING, "%s: Get PWM value fail: pwm id = %d", __func__, pwm_id);
  }
  else {
    *pwm_value = (uint8_t)value;
  }

  return ret;
}

int
pal_set_fan_speed(uint8_t pwm_id, uint8_t pwm_value) {

  if (pwm_id >= pal_pwm_cnt) {
    syslog(LOG_WARNING, "%s: Invalid pwm index: %d", __func__, pwm_id);
    return -1;
  }

  return sensors_write_pwmfan(pwm_id, (float)pwm_value);
}

int
pal_get_cpld_ver(uint8_t fru, char *rbuf) {
  int ret, i2cfd;
  uint8_t rbuf_i2c[CPLD_VER_BYTE] = {0};
  uint8_t i2c_bus = CPLD_FW_REG_BUS;
  uint8_t cpld_addr = CPLD_FW_REG_ADDR;
  uint32_t ver_reg = CPLD_VER_REG;

  if (rbuf == NULL)
  {
    syslog(LOG_ERR, "%s() Pointer \"rbuf\" is NULL.\n", __func__);
    return -1;
  }

  switch (fru) {
    case FRU_SERVER:
      if (!kv_get(KEY_SERVER_CPLD_VER, rbuf, NULL, 0)) {
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

  ret = i2c_rdwr_msg_transfer(i2cfd, cpld_addr, (uint8_t *)&ver_reg, sizeof(ver_reg), rbuf_i2c, sizeof(rbuf_i2c));
  close(i2cfd);
  if (ret < 0) {
    syslog(LOG_WARNING, "%s() i2c_rdwr_msg_transfer to slave@0x%02X on bus %u failed", __func__, cpld_addr, i2c_bus);
    return -1;
  }

  if (fru == FRU_SERVER) {
    for (int i = 0; i < CPLD_VER_BYTE; i++) {
      snprintf(rbuf + (i * sizeof(uint16_t)), sizeof(rbuf), "%02X", rbuf_i2c[(CPLD_VER_BYTE - 1) - i]);
    }
    kv_set(KEY_SERVER_CPLD_VER, rbuf, 0, 0);
  }

  return 0;
}

int
pal_parse_oem_unified_sel(uint8_t fru, uint8_t *sel, char *error_log)
{
  char *post_err[] = {
    "System PXE boot fail",
    "CMOS/NVRAM configuration cleared",
    "TPM Self-Test Fail",
    "Boot Drive failure",
    "Data Drive failure",
    "Reserved"
  };

  if (sel == NULL)
  {
    syslog(LOG_ERR, "%s() Pointer \"sel\" is NULL.\n", __func__);
    return -1;
  }

  if (error_log == NULL)
  {
    syslog(LOG_ERR, "%s() Pointer \"error_log\" is NULL.\n", __func__);
    return -1;
  }

  uint8_t general_info = sel[3];
  uint8_t error_type = general_info & 0xF;
  uint8_t event_type, estr_idx;

  switch (error_type) {
    case UNIFIED_POST_ERR:
      event_type = sel[8] & 0xF;
      estr_idx = (event_type < ARRAY_SIZE(post_err)) ? event_type : (ARRAY_SIZE(post_err) - 1);
      sprintf(error_log, "GeneralInfo: POST(0x%02X), POST Failure Event: %s", general_info, post_err[estr_idx]);
      return PAL_EOK;
    default:
      break;
  }

  pal_parse_oem_unified_sel_common(fru, sel, error_log);

  return PAL_EOK;
}

int
pal_set_sysfw_ver(uint8_t slot, uint8_t* ver) {
  int i;
  char key[MAX_KEY_LEN] = {0};
  char str[MAX_VALUE_LEN] = {0};
  char tstr[10] = {0};

  if (ver == NULL) {
    syslog(LOG_ERR, "%s() Pointer \"ver\" is NULL.\n", __func__);
    return -1;
  }

  snprintf(key, sizeof(key), "sysfw_ver_server");

  for (i = 0; i < SIZE_SYSFW_VER; i++) {
    snprintf(tstr, sizeof(tstr), "%02x", ver[i]);
    strncat(str, tstr, sizeof(tstr));
  }

  return pal_set_key_value(key, str);
}

int
pal_get_sysfw_ver(uint8_t slot, uint8_t* ver) {
  int i = 0, j = 0;
  int ret;
  int msb, lsb;
  char key[MAX_KEY_LEN] = {0};
  char str[MAX_VALUE_LEN] = {0};
  char tstr[4] = {0};

  if (ver == NULL) {
    syslog(LOG_ERR, "%s() Pointer \"ver\" is NULL.\n", __func__);
    return -1;
  }

  snprintf(key, sizeof(key), "sysfw_ver_server");

  ret = pal_get_key_value(key, str);
  if (ret) {
    return ret;
  }

  for (i = 0; i < 2 * SIZE_SYSFW_VER; i += 2) {
    snprintf(tstr, sizeof(tstr), "%c\n", str[i]);
    msb = strtol(tstr, NULL, 16);

    snprintf(tstr, sizeof(tstr), "%c\n", str[i + 1]);
    lsb = strtol(tstr, NULL, 16);
    ver[j++] = (msb << 4) | lsb;
  }

  return 0;
}

// Update the Identification LED for the given fru with the status
int
pal_set_id_led(uint8_t fru, enum LED_HIGH_ACTIVE value) {
  if (fru != FRU_SERVER) {
    return -1;
  }
  return gpio_set_value_by_shadow("PWR_ID_LED", value);
}

int
pal_set_fault_led(uint8_t fru, enum LED_HIGH_ACTIVE value) {
  if (fru != FRU_SERVER) {
    return -1;
  }
  return gpio_set_value_by_shadow("FAULT_LED", value);
}

int
pal_read_error_code_file(uint8_t *error_code_array, uint8_t error_code_array_len) {
  FILE *err_file = NULL;
  int i = 0, ret = 0;
  unsigned int err_tmp = 0;

  if (error_code_array == NULL) {
    syslog(LOG_WARNING, "%s(): fail to read error code because NULL parameter: *error_code_byte", __func__);
    return -1;
  }

  // if no file, create file
  if (access(ERR_CODE_BIN, F_OK) == -1) {
    err_file = fopen(ERR_CODE_BIN, "w");
    if (err_file == NULL) {
      syslog(LOG_WARNING, "%s: fail to open %s file because %s ", __func__, ERR_CODE_BIN, strerror(errno));
      return -1;
    }

    ret = pal_flock_retry(fileno(err_file));
    if (ret < 0) {
      syslog(LOG_WARNING, "%s: fail to flock %s file because %s ", __func__, ERR_CODE_BIN, strerror(errno));
      fclose(err_file);
      return -1;
    }

    memset(error_code_array, 0, error_code_array_len);
    for (i = 0; i < error_code_array_len; i++) {
      fprintf(err_file, "%X ", error_code_array[i]);
    }
    fprintf(err_file, "\n");

    pal_unflock_retry(fileno(err_file));
    fclose(err_file);
    return 0;
  }

  err_file = fopen(ERR_CODE_BIN, "r");
  if (err_file == NULL) {
    syslog(LOG_WARNING, "%s: fail to open %s file because %s ", __func__, ERR_CODE_BIN, strerror(errno));
    return -1;
  }

  for (i = 0; (fscanf(err_file, "%X", &err_tmp) != EOF) && (i < error_code_array_len); i++) {
    error_code_array[i] = (uint8_t) err_tmp;
  }

  fclose(err_file);
  return 0;
}

int
pal_write_error_code_file(unsigned char error_code_update, uint8_t error_code_status) {
  FILE *err_file = NULL;
  int i = 0, ret = 0;
  int byte_site = 0 , bit_site = 0;
  uint8_t error_code_array[MAX_NUM_ERR_CODES_ARRAY] = {0};

  memset(error_code_array, 0, sizeof(error_code_array));

  ret = pal_read_error_code_file(error_code_array, sizeof(error_code_array));
  if (ret < 0) {
    syslog(LOG_WARNING, "%s(): fail to write error code 0x%X because read %s error", __func__, error_code_update, ERR_CODE_BIN);
    return ret;
  }

  err_file = fopen(ERR_CODE_BIN, "r+");

  ret = pal_flock_retry(fileno(err_file));
  if (ret < 0) {
    syslog(LOG_WARNING, "%s: fail to flock %s file because %s ", __func__, ERR_CODE_BIN, strerror(errno));
    fclose(err_file);
    return ret;
  }

  byte_site = error_code_update / 8;
  bit_site = error_code_update % 8;

  if (error_code_status == ERR_CODE_ENABLE) {
    error_code_array[byte_site] = SETBIT(error_code_array[byte_site], bit_site);
  } else {
    error_code_array[byte_site] = CLEARBIT(error_code_array[byte_site], bit_site);
  }

  for (i = 0; i < sizeof(error_code_array); i++) {
    fprintf(err_file, "%X ", error_code_array[i]);
  }
  fprintf(err_file, "\n");

  pal_unflock_retry(fileno(err_file));
  fclose(err_file);
  return 0;
}

int
pal_get_error_code(uint8_t *data, uint8_t *error_count) {
  uint8_t total_error_array[MAX_NUM_ERR_CODES_ARRAY] = {0};
  int ret = 0, i = 0, j = 0;
  int tmp_err_count = 0;

  if (data == NULL) {
    printf("%s: fail to get error code because NULL parameter: *data", __func__);
    return -1;
  }

  if (error_count == NULL) {
    printf("%s: fail to get error code because NULL parameter: *error_count", __func__);
    return -1;
  }

  memset(total_error_array, 0, sizeof(total_error_array));

  // get bmc error code
  ret = pal_read_error_code_file(total_error_array, sizeof(total_error_array));
  if (ret < 0) {
    printf("Failed to get bmc error code\n");
    memset(total_error_array, 0, sizeof(total_error_array));
  }

  // count error and change storage format from byte array to number
  memset(data, 0, MAX_NUM_ERR_CODES);
  for (i = 0; i < MAX_NUM_ERR_CODES_ARRAY; i++) {
    for (j = 0; j < 8; j++) {
      if (GETBIT(total_error_array[i], j) == 1) {
        data[tmp_err_count] = (i * 8) + j;
        tmp_err_count++;
      }
    }
  }
  *error_count = tmp_err_count;

  return 0;
}

void
pal_set_error_code(unsigned char error_num, uint8_t error_code_status) {
  int ret = 0;

  if (error_num < MAX_NUM_ERR_CODES) {
    ret = pal_write_error_code_file(error_num, error_code_status);
    if (ret < 0) {
      syslog(LOG_ERR, "%s(): fail to write error code: 0x%02X", __func__, error_num);
    }
  } else {
    syslog(LOG_WARNING, "%s(): invalid error code number", __func__);
  }
}

int
pal_set_last_pwr_state(uint8_t fru, char *state) {

  int ret;
  char key[MAX_KEY_LEN] = {0};

  if(state == NULL)
  {
    syslog(LOG_ERR, "%s() Pointer \"state\" is NULL.\n", __func__);
    return -1;
  }

  sprintf(key, "%s", "server_last_power_state");

  ret = pal_set_key_value(key, state);
  if (ret < 0) {
#ifdef DEBUG
    syslog(LOG_WARNING, "pal_set_last_pwr_state: pal_set_key_value failed for "
        "fru %u", fru);
#endif
  }

  return ret;
}

int
pal_get_last_pwr_state(uint8_t fru, char *state) {
  int ret;
  char key[MAX_KEY_LEN] = {0};

  if(state == NULL)
  {
    syslog(LOG_ERR, "%s() Pointer \"state\" is NULL.\n", __func__);
    return -1;
  }

  sprintf(key, "%s", "server_last_power_state");

  ret = pal_get_key_value(key, state);
  if (ret < 0) {
#ifdef DEBUG
    syslog(LOG_WARNING, "pal_get_last_pwr_state: pal_get_key_value failed for "
        "fru %u", fru);
#endif
  }

  return ret;
}

int
pal_is_bmc_por(void) {
  FILE *fp;
  int is_power_on_reset = 0;

  fp = fopen(PATH_POWER_ON_RESET, "r");
  if (fp != NULL) {
    if (fscanf(fp, "%d", &is_power_on_reset) != 1) {
      is_power_on_reset = 0;
    }
    fclose(fp);
  }

  return (is_power_on_reset) ? 1 : 0;
}

int
pal_get_boot_order(uint8_t slot, uint8_t *req_data, uint8_t *boot, uint8_t *res_len) {
  int i = 0;
  int j = 0;
  int ret = 0;
  int msb = 0, lsb = 0;
  int tmp_len = 0;
  char key[MAX_KEY_LEN] = {0};
  char str[MAX_VALUE_LEN] = {0};
  char tmp_str[4] = {0};

  tmp_len = sizeof(tmp_str);

  snprintf(key, MAX_KEY_LEN, "server_boot_order");

  ret = pal_get_key_value(key, str);
  if (ret != 0) {
    *res_len = 0;
    return ret;
  }

  for (i = 0; i < 2*SIZE_BOOT_ORDER; i += 2) {
    snprintf(tmp_str, tmp_len, "%c\n", str[i]);
    msb = strtol(tmp_str, NULL, 16);

    snprintf(tmp_str, tmp_len, "%c\n", str[i+1]);
    lsb = strtol(tmp_str, NULL, 16);
    boot[j++] = (msb << 4) | lsb;
  }

  *res_len = SIZE_BOOT_ORDER;

  return 0;
}

int
pal_set_boot_order(uint8_t slot, uint8_t *boot, uint8_t *res_data, uint8_t *res_len) {
  int i = 0;
  int tmp_len = 0;
  char key[MAX_KEY_LEN] = {0};
  char str[MAX_VALUE_LEN] = {0};
  char tmp_str[4] = {0};

  *res_len = 0;
  tmp_len = sizeof(tmp_str);

  snprintf(key, MAX_KEY_LEN, "server_boot_order");

  for (i = 0; i < SIZE_BOOT_ORDER; i++) {
    snprintf(tmp_str, tmp_len, "%02x", boot[i]);
    strncat(str, tmp_str, tmp_len);
  }

  return pal_set_key_value(key, str);
}

int
pal_pmbus_sensor_info_initial(void) {
  int ret = 0;
  uint8_t pmbus_type = 0;
  uint8_t rev_id = 0;

  ret = netlakemtp_common_get_board_rev(&rev_id);
  if (ret < 0) {
    syslog(LOG_ERR, "%s() Failed to get CPLD board revision, use main source setting as default", __func__);
  }

  int sku = ((int)rev_id & 0x08) >> 3;
  extern PAL_PMBUS_INFO pmbus_dev_table[];
  extern size_t pmbus_dev_cnt;

  for (uint8_t i = 0; i < pmbus_dev_cnt; i++) {
    pmbus_type = pmbus_dev_table[i].sku_pmbus_type[sku].type;
    for (int j = 0; j < MAX_PMBUS_SUP_CMD_CNT; j++) {
      if (pmbus_dev_table[i].offset == pmbus_dev_list[pmbus_type].pmbus_cmd_list[j].read_cmd) {
        char key_with_cmd[MAX_KEY_LEN];
        char val[MAX_VALUE_LEN];
        snprintf(key_with_cmd, MAX_KEY_LEN, "pmbus-sensor%02x%c", i, '\0');
        snprintf(val, MAX_VALUE_LEN, "%02x-%02x-%02x%c", pmbus_dev_table[i].sku_pmbus_type[sku].page,
                  pmbus_dev_list[pmbus_type].pmbus_cmd_list[j].read_byte,
                  pmbus_dev_list[pmbus_type].pmbus_cmd_list[j].read_type, '\0');
        ret = kv_set(key_with_cmd, val, 0, 0);
        if (ret < 0) {
          syslog(LOG_ERR, "%s() Failed to set PMBUS info, key=%s, errno=%d", __func__, key_with_cmd, errno);
          return -1;
        }
        break;
      }
    }
  }
  return 0;
}

int
pal_hsc_reading_enable(void) {
  int ret, fd;
  uint8_t rbuf[CPLD_VER_BYTE] = {0};
  uint8_t bus = MTP_HSC_BUS;
  uint8_t addr = MTP_HSC_ADDR;
  uint8_t enable_vout_req[MTP_HSC_EN_VOUT_LENGTH] = {0};
  //enable_vout_req[0]: PMON_CONFIG address, enable_vout_req[1-2]: PMON_CONFIG register

  fd = i2c_cdev_slave_open(bus, addr >> 1, I2C_SLAVE_FORCE_CLAIM);
  if (fd < 0) {
    syslog(LOG_WARNING, "Failed to open bus %u", bus);
    return -1;
  }

  uint8_t pmon_config_addr = MTP_PMON_CONFIG_ADDR;
  ret = i2c_rdwr_msg_transfer(fd, addr, &pmon_config_addr, sizeof(pmon_config_addr), (enable_vout_req + 1), 2);
  if (ret < 0) {
    syslog(LOG_WARNING, "%s() i2c_rdwr_msg_transfer to slave@0x%02X on bus %u failed", __func__, addr, bus);
    close(fd);
    return -1;
  }

  //set bit 1 to 1 for Enabling VOUT sampling
  enable_vout_req[1] |= 0x2;
  enable_vout_req[0] = pmon_config_addr;
  ret = i2c_rdwr_msg_transfer(fd, addr, enable_vout_req, MTP_HSC_EN_VOUT_LENGTH, NULL, 0);
  close(fd);
  if (ret < 0) {
    syslog(LOG_WARNING, "%s() i2c_rdwr_msg_transfer to slave@0x%02X on bus %u failed", __func__, addr, bus);
    return -1;
  }

  return 0;
}

int
pal_hsc_sensor_info_initial(void) {
  int ret = 0;
  uint8_t type;

  int sku = ADM1278;
  extern PAL_PMBUS_INFO hsc_dev_table[];
  extern hsc_dev_info hsc_dev_list[];
  extern size_t hsc_dev_cnt;

  for (uint8_t i = 0; i < hsc_dev_cnt; i++) {
    type = hsc_dev_table[i].sku_pmbus_type[sku].type;
    for (int j = 0; j < MAX_HSC_SUP_CMD_CNT; j++) {
      if (hsc_dev_table[i].offset == hsc_dev_list[type].hsc_cmd_list[j].read_cmd) {
        char key_with_cmd[MAX_KEY_LEN];
        char val[MAX_VALUE_LEN];
        snprintf(key_with_cmd, MAX_KEY_LEN, "hsc-sensor%02x%c", i, '\0');
        snprintf(val, MAX_VALUE_LEN, "%02x-%02x%c", type, j,'\0');

        kv_set(key_with_cmd, val, 0, 0);
        if (ret < 0) {
          syslog(LOG_ERR, "%s() Failed to set HSC info, errno=%d", __func__, errno);
          return -1;
        }
        break;
      }
    }
  }
  return ret;
}

int
pal_adc_clock_control(void) {
  int ret = 0;
  uint32_t reg_value = 0;

  ret = phymem_get_dword(ADC_BASE, REG_ADC0C, &reg_value);
  if (ret < 0) {
    syslog(LOG_ERR, "%s() Failed to get register ADC0C", __func__);
    return -1;
  }

  reg_value = ((reg_value >> 10) << 10 | 0x00000510);
  phymem_set_dword(ADC_BASE, REG_ADC0C, reg_value);
  if (ret < 0) {
    syslog(LOG_ERR, "%s() Failed to set register ADC0C", __func__);
    return -1;
  }

  return 0;
}

int
pal_sensor_monitor_initial(void) {
  int ret = 0;

  ret = pal_pmbus_sensor_info_initial();
  if (ret < 0) {
    syslog(LOG_ERR, "%s() Failed to init VR info", __func__);
  }

  ret = pal_hsc_sensor_info_initial();
  if (ret < 0) {
    syslog(LOG_ERR, "%s() Failed to init HSC info", __func__);
  }

  pal_adc_clock_control();
  pal_hsc_reading_enable();

  return 0;
}

int
pal_post_display(uint8_t status) {
  int ret = 0, i = 0;
  gpio_value_t value = GPIO_VALUE_INVALID;

  for (i = 0; i < MAX_NUM_GPIO_LED_POSTCODE; i++) {
    if (BIT(status, i) != 0) {
      value = GPIO_VALUE_HIGH;
    } else {
      value = GPIO_VALUE_LOW;
    }
    ret = gpio_set_value_by_shadow(GPIO_LED_POSTCODE_TABLE[i], value);

    if (ret < 0) {
      syslog(LOG_WARNING, "%s Failed GPIO: LED_POSTCODE_%d, ret: %d\n", __func__, i, ret);
      break;
    }
  }

  return ret;
}
