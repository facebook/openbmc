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
#include <time.h>
#include <sys/mman.h>
#include <sys/stat.h>
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
#define UNINITIAL_POWER_LIMIT 0x02
#define MAX_MCE_ERROR_STR_LEN 256

const char pal_fru_list[] = "all, server, bmc, pdb, fio, nic";

// export to sensor-util
const char pal_fru_list_sensor_history[] = "all, server, bmc, pdb, fio, nic";
// fru name list for pal_get_fru_id()
const char *fru_str_list[] = {"all", "server", "bmc", "pdb", "fio", "nic"};

size_t pal_pwm_cnt = 1;
size_t pal_tach_cnt = 4;
const int fan_map[] = {1, 3, 5, 7};
const int fanIdToPwmIdMapping[] = {0, 0, 0, 0};

const static pmbus_dev_info pmbus_dev_list[] = {
  [MP29608B] = {
  {{CMD_TEMP1, READ_WORD, LINEAR11},
  {CMD_TEMP2, READ_WORD, LINEAR11},
  {CMD_VOUT, READ_WORD, VOUT_MODE},
  {CMD_VIN, READ_WORD, LINEAR11},
  {CMD_IOUT, READ_WORD, LINEAR11},
  {CMD_IIN, READ_WORD, LINEAR11},
  {CMD_POUT, READ_WORD, LINEAR11},
  {CMD_PIN, READ_WORD, LINEAR11},}},
  [XDPE19283D] = {
  {{CMD_TEMP1, READ_WORD, LINEAR11},
  {CMD_TEMP2, READ_WORD, LINEAR11},
  {CMD_VOUT, READ_WORD, VOUT_MODE},
  {CMD_VIN, READ_WORD, LINEAR11},
  {CMD_IOUT, READ_WORD, LINEAR11},
  {CMD_IIN, READ_WORD, LINEAR11},
  {CMD_POUT, READ_WORD, LINEAR11},
  {CMD_PIN, READ_WORD, LINEAR11},}},
  [RAA229641] = {
  {{CMD_TEMP1, READ_WORD, DIRECT},
  {CMD_TEMP2, READ_WORD, DIRECT},
  {CMD_VOUT, READ_WORD, DIRECT},
  {CMD_VIN, READ_WORD, DIRECT},
  {CMD_IOUT, READ_WORD, DIRECT},
  {CMD_IIN, READ_WORD, DIRECT},
  {CMD_POUT, READ_WORD, DIRECT},
  {CMD_PIN, READ_WORD, DIRECT},}},
};

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
  {"nic_sensor_health", "1", NULL},
  {"sysfw_ver_server", "0", NULL},
  {"system_identify_led", "off", NULL},
  {"server_power_on_reset_cfg", "lps", NULL},
  {"server_last_power_state", "on", NULL},
  {"ntp_server", "", NULL},
  {"server_boot_order", "0100090203ff", NULL},
  {"timestamp_sled", "0", NULL},
  /* Add more Keys here */
  {"server_power_limit_status", "enable (invalid)", NULL},
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
    case FRU_NIC:
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
  return netlakenext_get_fruid_name(fru, name);
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
  return netlakenext_get_fruid_path(fru, path);
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
  return netlakenext_get_fruid_eeprom_path(fru, path);
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
      *caps = (FRU_CAPABILITY_SENSOR_ALL | FRU_CAPABILITY_POWER_ALL); // MB FRU uses Meta FBOSS EEPROM format; managed via weutil
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
    case FRU_NIC:
      *caps = (FRU_CAPABILITY_FRUID_READ | FRU_CAPABILITY_SENSOR_ALL | FRU_CAPABILITY_NETWORK_CARD);
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
    case FRU_NIC:
      snprintf(name, MAX_FRU_CMD_STR, "nic");
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

  return netlakenext_check_fru_is_valid(fruid_path);
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
    case FRU_NIC:
      snprintf(key, sizeof(key), "nic_sensor_health");
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
    case FRU_NIC:
      snprintf(key, sizeof(key), "nic_sensor_health");
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
  snprintf(fan_controller, sizeof(fan_controller), "%s%d-%s", "max31790-i2c-",
           FAN_CTL_BUS, FAN_CTL_ADDR_STR);
  ret = sensors_read(fan_controller, fan_label, &value);
  if (ret == 0 ) {
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
pal_parse_sel(uint8_t fru, uint8_t *sel, char *error_log)
{
  uint8_t snr_num = sel[11];
  uint8_t *event_data = &sel[10];
  bool parsed = true;

  strcpy(error_log, "");
  switch(snr_num) {
    case MACHINE_CHK_ERR:
      parse_mce_error_sel(event_data, error_log);
      parsed = true;
      break;
    default:
      parsed = false;
      break;
  }

  if (parsed == true) {
    return 0;
  }

  pal_parse_sel_helper(fru, sel, error_log);
  return 0;
}

void
parse_mce_error_sel(uint8_t *event_data, char *error_log) {
  uint8_t *ed = &event_data[3];
  uint8_t error_type = ((ed[1] & 0x60) >> 5);
  char cri_sel[512] = {0};

  const char *err_type_str = "";

  if ((ed[0] & 0x0F) == 0x0B) { // Uncorrectable
    switch (error_type) {
      case 0x00:
        err_type_str = "Uncorrected Recoverable Error, ";
        break;
      case 0x01:
        err_type_str = "Uncorrected Thread Fatal Error, ";
        break;
      case 0x02:
        err_type_str = "Uncorrected System Fatal Error, ";
        break;
      default:
        err_type_str = "Unknown, ";
        break;
    }
  } else if ((ed[0] & 0x0F) == 0x0C) { // Correctable
    switch (error_type) {
      case 0x00:
        err_type_str = "Correctable Error, ";
        break;
      case 0x01:
        err_type_str = "Deferred Error, ";
        break;
      default:
        err_type_str = "Unknown, ";
        break;
    }
  }

  snprintf(error_log,
           MAX_MCE_ERROR_STR_LEN,
           "%sBank Number %u, CPU %u, Core %u",
           err_type_str,
           ed[1] & 0x1F,
           (ed[2] & 0xE0) >> 5,
           ed[2] & 0x1F);

  snprintf(cri_sel,
           sizeof(cri_sel),
           "MACHINE_CHK_ERR, %s",
           error_log);

  pal_add_cri_sel(cri_sel);
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

  for (i = 0; i < (int)sizeof(error_code_array); i++) {
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
  uint8_t sku = 0;
  bool change_vr_bus = false;

  ret = netlakenext_common_get_vr_sku(&sku, &change_vr_bus);
  if (ret < 0) {
    syslog(LOG_ERR, "%s() Failed to get vr sku, use main source (MPS) setting as default", __func__);
  }

  extern PAL_PMBUS_INFO pmbus_dev_table[];
  extern size_t pmbus_dev_cnt;

  for (uint8_t i = 0; i < pmbus_dev_cnt; i++) {
    pmbus_type = pmbus_dev_table[i].sku_pmbus_type[sku].type;
    if (change_vr_bus) {
      if (pmbus_dev_table[i].slv_addr == VR_PVDDCR_ADDR) {
        pmbus_dev_table[i].bus = VR_PVDDCR_BUS;
      }
      else if (pmbus_dev_table[i].slv_addr == VR_PVDDCR_SOC_ADDR) {
        pmbus_dev_table[i].bus = VR_PVDDCR_SOC_BUS;
      }
      else if (pmbus_dev_table[i].slv_addr == VR_PVDD_MISC_ADDR) {
        pmbus_dev_table[i].bus = VR_PVDD_MISC_BUS;
      }
    }
    for (int j = 0; j < MAX_PMBUS_SUP_CMD_CNT; j++) {
      if (pmbus_dev_table[i].sku_pmbus_type[sku].offset == pmbus_dev_list[pmbus_type].pmbus_cmd_list[j].read_cmd) {
        char key_with_cmd[MAX_KEY_LEN];
        char val[MAX_VALUE_LEN];
        snprintf(key_with_cmd, MAX_KEY_LEN, "pmbus-sensor%02x%c", i, '\0');
        snprintf(val, MAX_VALUE_LEN, "%02d-%02x-%02d-%02x-%02x%c", pmbus_dev_table[i].sku_pmbus_type[sku].type,
                  pmbus_dev_table[i].sku_pmbus_type[sku].page,
                  pmbus_dev_table[i].sku_pmbus_type[sku].offset,
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
  extern size_t hsc_dev_cnt;

  for (uint8_t i = 0; i < hsc_dev_cnt; i++) {
    type = hsc_dev_table[i].sku_pmbus_type[sku].type;
    for (int j = 0; j < MAX_HSC_SUP_CMD_CNT; j++) {
      if (hsc_dev_table[i].sku_pmbus_type[sku].offset == hsc_dev_list[type].hsc_cmd_list[j].read_cmd) {
        char key_with_cmd[MAX_KEY_LEN];
        char val[MAX_VALUE_LEN];
        snprintf(key_with_cmd, MAX_KEY_LEN, "hsc-sensor%02x%c", i, '\0');
        snprintf(val, MAX_VALUE_LEN, "%02d-%02x-%02x%c", hsc_dev_table[i].sku_pmbus_type[sku].offset, type, j,'\0');

        ret = kv_set(key_with_cmd, val, 0, 0);
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
pal_hwmon_probe(char *path, char *dev) {
  FILE *fp;
  int rc = 0;

  fp = fopen(path, "w");
  if (fp == NULL) {
    syslog(LOG_WARNING, "%s() failed to open file, path=%s", __func__, path);
    return -1;
  }

  rc = fputs(dev, fp);
  fclose(fp);

  if (rc < 0) {
    syslog(LOG_WARNING, "%s() device %s bind/unbind failed\n", __func__, dev);
    return -1;
  }

  return 0;
}

int
pal_max31790_init(void) {
  int fd, ret = 0;
  struct stat buf;
  uint8_t tlen = 2;
  uint8_t bus = FAN_CTL_BUS;
  uint8_t addr = FAN_CTL_ADDR;

  fd = i2c_cdev_slave_open(bus, addr >> 1, I2C_SLAVE_FORCE_CLAIM);
  if (fd < 0) {
    syslog(LOG_WARNING, "Failed to open i2c bus %u", bus);
    return -1;
  }

  for (int i = 0; i < FAN_CHANNEL; i++) {
    uint8_t tbuf[] = {MAX31790_REG_FAN_CONFIG(i), FAN_CONF_DATA};

    ret = i2c_rdwr_msg_transfer(fd, addr, tbuf, tlen, NULL, 0);
    if (ret < 0) {
      syslog(LOG_WARNING,
	           "%s() i2c_rdwr_msg_transfer to slave@0x%02X on bus %u failed",
	           __func__, addr, bus);
    }
  }

  close(fd);

  for (int j = 0; j < MAX31790_PROBE_RETRY; j++) {
    pal_hwmon_probe(MAX31790_UNBIND_PATH, MAX31790_BUS_ADDR);
    pal_hwmon_probe(MAX31790_BIND_PATH, MAX31790_BUS_ADDR);

    ret = stat(MAX31790_STAT_PATH, &buf);
    if (ret != 0) {
      syslog(LOG_INFO, "check max31790 stat, errno=%s", strerror(errno));
      sleep(1);
    } else {
      break;
    }
  }

  sensors_reinit();

  return ret;
}

int pal_udbg_get_frame_total_num() {
  return 5;
}

int
pal_get_80port_record(uint8_t slot, uint8_t *buf, size_t max_len, size_t *len) {
  if (!pal_is_slot_server(slot)) {
    syslog(LOG_WARNING, "pal_get_80port_record: slot %d is not supported", slot);
    return PAL_ENOTSUP;
  }

  return pal_get_lpc_pcc_record(slot, buf, max_len, len);
}

int
pal_get_80port_page_record(uint8_t slot, uint8_t page_num, uint8_t *res_data, size_t max_len, size_t *res_len) {
  char key[MAX_KEY_LEN] = {0};
  char value[MAX_VALUE_LEN] = {0};
  size_t len = 0;

  if ((res_data == NULL) || (res_len == NULL)) {
    return -1;
  }

  if (!pal_is_slot_server(slot)) {
    return PAL_ENOTSUP;
  }

  snprintf(key, sizeof(key), "pcc_postcode_%u", page_num);
  if (kv_get(key, value, &len, 0) != 0) {
    *res_len = 0;
    return 0;
  }

  if (len > max_len) {
    len = max_len;
  }

  if (len > 0) {
    memcpy(res_data, value, len);
  }
  *res_len = len;

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
  pal_max31790_init();

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

// Debug Card's and SOL port share UART port and need to enable only one
int8_t
pal_set_uart_routing(uint8_t routing) {
  int lpc_fd;
  uint32_t ctrl;
  void *lpc_reg;
  void *lpc_hicr;

  lpc_fd = open("/dev/mem", O_RDWR | O_SYNC );
  if (lpc_fd < 0) {
    return -1;
  }

  lpc_reg = mmap(NULL, PAGE_SIZE, PROT_READ|PROT_WRITE, MAP_SHARED, lpc_fd,
             AST_LPC_BASE);
  lpc_hicr = (char*)lpc_reg + HICRA_OFFSET;

  // Read HICRA register
  ctrl = *(volatile uint32_t*) lpc_hicr;

  // Clear bits for UART1, UART3 and UART4 routing
  ctrl &= (~HICRA_MASK_UART1);
  ctrl &= (~HICRA_MASK_UART3);
  ctrl &= (~HICRA_MASK_UART4);

  if (routing == DEBUG_CARD_ABSENT) {
    // Route UART3 to UART4 for SoL purpose
    ctrl |= (UART3_TO_UART4 << 25);

    // Route UART4 to UART3 for SoL purpose
    ctrl |= (UART4_TO_UART3 << 22);

  }

  *(volatile uint32_t*) lpc_hicr = ctrl;

  munmap(lpc_reg, PAGE_SIZE);
  close(lpc_fd);

  return 0;
}

int
pal_get_sensor_util_timeout(uint8_t fru) {
  return SENSOR_LIST_TIMEOUT_DEFAULT;
}

void pal_update_ts_sled() {
  char key[MAX_KEY_LEN] = {0};
  char timestamp_str[MAX_VALUE_LEN] = {0};
  struct timespec timestamp;
  int ret = 0;

  memset(key, 0, sizeof(key));
  memset(timestamp_str, 0, sizeof(timestamp_str));
  memset(&timestamp, 0, sizeof(timestamp));

  clock_gettime(CLOCK_REALTIME, &timestamp);

  snprintf(key, sizeof(key), "timestamp_sled");
  snprintf(timestamp_str, sizeof(timestamp_str), "%ld", timestamp.tv_sec);

  ret = pal_set_key_value(key, timestamp_str);
  if (ret < 0) {
    syslog(LOG_ERR, "%s(): failed to set key: %s value: %s", __func__, key, timestamp_str);
  }
}

int pal_lpc_pcc_read(uint8_t *buf, size_t max_len, size_t *rlen)
{
  const char *dev_path = "/dev/aspeed-lpc-pcc";
  int fd, offs;
  char key[MAX_KEY_LEN];
  char value[MAX_VALUE_LEN];
  bool new_data = false;
  size_t len, half1, half2;
  uint8_t page, port, index = 0;
  uint16_t one_code;
  uint32_t post_code = 0;
  static uint32_t post_fifo[PCC_FIFO_SIZE] = {0};
  static int data_in = 0, data_out = 0;
  static bool init_fifo = true;

  if (init_fifo) {
    init_fifo = false;
    // initialize fifo from existing cache store
    for (page = 1; page <= PCC_PAGE; ++page) {
      snprintf(key, sizeof(key), "pcc_postcode_%u", page);
      if (kv_get(key, value, &len, 0) != 0) {
        break;
      }
      if (len > sizeof(value)) {
        syslog(LOG_WARNING, "%s: unexpected len %zu", __func__, len);
        break;
      }
      memcpy(&post_fifo[data_in], value, len);
      data_in += (len / sizeof(uint32_t));
    }
  }

  fd = open(dev_path, O_RDONLY | O_NONBLOCK);
  if (fd < 0) {
    return PAL_ENOTREADY;
  }

  // read postcodes from the FIFO of lpc-pcc driver, and put in cache store
  while (read(fd, &one_code, sizeof(one_code)) == sizeof(one_code)) {
    port = one_code >> 8;
    if ((index == 0 && (port & PCC_PORT1) == PCC_PORT1) ||
        (index == 1 && (port & PCC_PORT2) == PCC_PORT2) ||
        (index == 2 && (port & PCC_PORT3) == PCC_PORT3) ||
        (index == 3 && (port & PCC_PORT4) == PCC_PORT4)) {
      post_code |= (one_code & 0xFF) << (index * 8);
    } else {
      // discard incomplete remnants
      index = 0;
      post_code = 0;
      continue;
    }

    if (index == 3) {
      // use a simple FIFO (ring buffer) for easier drop old postcodes
      post_fifo[data_in] = post_code;
      data_in = (data_in + 1) % PCC_FIFO_SIZE;
      if (data_in == data_out) {
        data_out = (data_out + 1) % PCC_FIFO_SIZE;
      }
      pal_check_psb_error(post_code);
      pal_check_abl_error(post_code);
      pal_mrc_warning_detect(0, post_code);
      index = 0;
      post_code = 0;
      new_data = true;
    } else {
      index++;
    }
  }
  close(fd);

  if (new_data) {
    // store up to 64 postcodes per page (kv)
    for (page = 1, offs = data_out; page <= PCC_PAGE;
         ++page, offs = (offs + PCC_SIZE)%PCC_FIFO_SIZE) {
      snprintf(key, sizeof(key), "pcc_postcode_%u", page);
      if ((offs + PCC_SIZE) <= data_in) {
        len = PCC_SIZE * sizeof(uint32_t);
        memcpy(value, &post_fifo[offs], len);
      } else {
        if (offs <= data_in) {
          len = (data_in - offs) * sizeof(uint32_t);
          memcpy(value, &post_fifo[offs], len);
        } else {
          len = (data_in < data_out) ? (PCC_SIZE * sizeof(uint32_t)) : 0;
          if (len == 0) {  // no more data
            break;
          }
          half1 = (PCC_FIFO_SIZE - offs) * sizeof(uint32_t);
          if (half1 > len) {
            half1 = len;
          }
          half2 = len - half1;
          memcpy(value, &post_fifo[offs], half1);
          if (half2 > 0) {
            memcpy(&value[half1], &post_fifo[0], half2);
          }
        }
      }
      kv_set(key, value, len, 0);
      if (len < (PCC_SIZE * sizeof(uint32_t))) {  // no more data
        break;
      }
    }
  }

  // reply the latest postcodes to caller's buffer
  len = (data_in + PCC_FIFO_SIZE - data_out)%PCC_FIFO_SIZE;
  max_len /= sizeof(uint32_t);  // truncate if max_len is not multiple of 4 bytes
  if (len > max_len) {
    len = max_len;
  }
  if (len > 0) {
    offs = (data_in + PCC_FIFO_SIZE - len)%PCC_FIFO_SIZE;
    len *= sizeof(uint32_t);
    if ((offs + len/sizeof(uint32_t)) <= data_in) {
      memcpy(buf, &post_fifo[offs], len);
    } else {
      half1 = (PCC_FIFO_SIZE - offs) * sizeof(uint32_t);
      if (half1 > len) {
        half1 = len;
      }
      half2 = len - half1;
      memcpy(buf, &post_fifo[offs], half1);
      if (half2 > 0) {
        memcpy(&buf[half1], &post_fifo[0], half2);
      }
    }
  }
  *rlen = len;

  return PAL_EOK;
}

int pal_dimm_page_init()
{
  int ret = 0, fd = 0;
  uint8_t retry = SENSOR_RETRY_TIME;
  const uint8_t dimm_addr_list[] = {
    DIMMA_ADDR,
    DIMMB_ADDR,
  };
  uint8_t rbuf = 0;
  uint8_t rlen = DIMM_TEMP_LEN;

  for (size_t id = 0; id < sizeof(dimm_addr_list); id++) {
    fd = i2c_cdev_slave_open(I2C_BUS5, dimm_addr_list[id] >> 1,
                            I2C_SLAVE_FORCE_CLAIM);
    if (fd < 0) {
      syslog(LOG_ERR, "Failed to open DIMM 0x%x\n", dimm_addr_list[id]);
      return -1;
    }

    // set page 0 for DIMM temp sensor
    uint8_t setpage_data[2];
    setpage_data[0] = DIMM_PAGE_OFFSET;
    setpage_data[1] = DIMM_PAGE0;
    do {
      ret = i2c_rdwr_msg_transfer(fd, dimm_addr_list[id],
                                  setpage_data, sizeof(setpage_data), &rbuf, rlen);
      if (ret != 0) {
        usleep(SENSOR_RETRY_INTERVAL_USEC);
      }
    } while ((ret < 0) && ((retry--) > 0));

    if (ret < 0) {
      syslog(LOG_ERR, "%s() Failed to set 2-byte mode %x-%x", __func__,
            I2C_BUS5, dimm_addr_list[id]);
      close(fd);
      return -1;
    }
    retry = SENSOR_RETRY_TIME;
  }

  close(fd);
  return 0;
}

int
pal_set_power_limit(uint8_t slot_id, uint8_t *req_data, uint8_t *res_data, uint8_t *res_len) {
  int ret = CC_UNSPECIFIED_ERROR;
  char key[MAX_KEY_LEN] = {0};
  char value[MAX_VALUE_LEN] = {0};

  if ((req_data == NULL) || (res_data == NULL) || (res_len == NULL)) {
    syslog(LOG_WARNING, "%s() Fail to set power limit due to null pointer check", __func__);
    return -1;
  }

  *res_len = 0;

  snprintf(key, sizeof(key), "server_power_limit_status");
  uint8_t s = req_data[0];

  // bit[0]: CPU package power limit status (0: disable, 1: enable), bit[7]: valid bit (0: invalid, 1: valid)
  snprintf(value, sizeof(value), "%s%s",
         (s & 0x01) ? "enable" : "disable",
         (s & 0x80) ? "" : " (invalid)");
  
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

  if ((req_data == NULL) || (res_data == NULL) || (res_len == NULL)) {
    syslog(LOG_WARNING, "%s() Fail to get power limit due to null pointer check", __func__);
    return -1;
  }

  *res_len = 0;

  snprintf(key, sizeof(key), "server_power_limit_status");
  ret = kv_get(key, value, NULL, KV_FPERSIST);
  if (ret < 0) {
    res_data[*res_len] = UNINITIAL_POWER_LIMIT;
    ret = CC_SUCCESS;
  } else {
    bool enabled = false;
    bool valid = true;

    if (strncmp(value, "enable", 6) == 0) {
        enabled = true;
    } else if (strncmp(value, "disable", 7) == 0) {
        enabled = false;
    } else {
        syslog(LOG_WARNING, "%s() Invalid power limit status value: %s", __func__, value);
        res_data[*res_len] = UNINITIAL_POWER_LIMIT;
        return -1;
    }

    if (strstr(value, "(invalid)") != NULL) {
        valid = false;
    }

    uint8_t status = 0;
    // bit[0]: CPU package power limit status (0: disable, 1: enable), bit[7]: valid bit (0: invalid, 1: valid)
    if (enabled) status |= 0x01;
    if (valid)   status |= 0x80;
    res_data[*res_len] = status;
  }
  *res_len = SIZE_CPU_POWER_LIMIT_DATA;

  return ret;
}
