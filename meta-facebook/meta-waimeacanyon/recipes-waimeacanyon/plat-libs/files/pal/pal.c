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

#include <stdio.h>
#include <string.h>
#include <syslog.h>

#include <openbmc/libgpio.h>
#include <openbmc/kv.h>
#include <facebook/bic_ipmi.h>
#include "pal.h"

#define SYSFW_VER_STR "sysfw_ver_server"
#define LAST_KEY "last_key"

#define OFFSET_DEV_GUID 0x1800

const char pal_fru_list[] = "all, iom, mb, scb, scm, hdbp, nic";

enum key_event {
  KEY_BEFORE_SET,
  KEY_AFTER_INI,
};

static int key_func_por_cfg(int event, void *arg);

struct pal_key_cfg {
  char *name;
  char *def_val;
  int (*function)(int, void*);
} key_cfg[] = {
  /* name, default value, function */
  {SYSFW_VER_STR, "0", NULL},
  {"ntp_server", "", NULL},
  {"server_por_cfg", "lps", key_func_por_cfg},
  /* Add more Keys here */
  {LAST_KEY, LAST_KEY, NULL} /* This is the last key of the list */
};

int
pal_get_fru_list(char *list) {
  strcpy(list, pal_fru_list);
  return PAL_EOK;
}

// add for fruid-util
int
pal_get_fru_id(char *fru_name, uint8_t *fru_id) {
  int index = -1;
  int ret = -1;

  for (index = FRU_ALL; index <= MAX_NUM_FRUS; index++) {
    if (strncmp(fru_name, fru_str_list[index], MAX_FRU_NAME_STR) == 0) {
      *fru_id = index;
      ret = 0;
      break;
    }
  }

  return ret;
}

// add for fruid-util usage
int
pal_get_fru_name(uint8_t fru_id, char *fru_name) {
  if (fru_id > MAX_NUM_FRUS) {
    return -1;
  }
  snprintf(fru_name, MAX_FRU_NAME_STR, "%s", fru_str_list[fru_id]);
  
  return 0;
}

int
pal_get_fru_capability(uint8_t fru, unsigned int *caps)
{
  int ret = 0;

  switch (fru) {
    case FRU_ALL:
      *caps = FRU_CAPABILITY_SENSOR_READ | FRU_CAPABILITY_SENSOR_HISTORY;
      break;
    case FRU_IOM:
      *caps = FRU_CAPABILITY_SENSOR_READ | FRU_CAPABILITY_FRUID_ALL;
      break;
    case FRU_MB:
      *caps = FRU_CAPABILITY_SENSOR_READ | FRU_CAPABILITY_FRUID_ALL | FRU_CAPABILITY_POWER_ALL |
        FRU_CAPABILITY_POWER_12V_ALL;
      break;
    case FRU_SCB:
      *caps = FRU_CAPABILITY_SENSOR_READ | FRU_CAPABILITY_FRUID_ALL;
      break;
    case FRU_BMC:
      *caps = FRU_CAPABILITY_SENSOR_READ;
      break;
    case FRU_SCM:
      *caps = FRU_CAPABILITY_SENSOR_READ | FRU_CAPABILITY_FRUID_ALL;
      break;
    case FRU_BSM:
      *caps = FRU_CAPABILITY_FRUID_ALL;
      break;
    case FRU_HDBP:
      *caps = FRU_CAPABILITY_SENSOR_READ | FRU_CAPABILITY_FRUID_ALL;
      break;
    case FRU_PDB:
      *caps = FRU_CAPABILITY_SENSOR_READ | FRU_CAPABILITY_FRUID_ALL;
      break;
    case FRU_NIC:
      *caps = FRU_CAPABILITY_SENSOR_READ | FRU_CAPABILITY_FRUID_READ;
      break;
    case FRU_FAN0:
    case FRU_FAN1:
    case FRU_FAN2:
    case FRU_FAN3:
      *caps = FRU_CAPABILITY_FRUID_READ;
      break;
    default:
      ret = -1;
      syslog(LOG_WARNING, "%s() failed, fru:%d is not expected.", __func__, fru);
      break;
  }
  return ret;
}



int
pal_get_fruid_name(uint8_t fru, char *name) {
  return fbwc_get_fruid_name(fru, name);
}

int
pal_get_fruid_path(uint8_t fru, char *path) {
  return fbwc_get_fruid_path(fru, path);
}

int
pal_get_fruid_eeprom_path(uint8_t fru, char *path) {
  return fbwc_get_fruid_eeprom_path(fru, path);
}

int
pal_fruid_write(uint8_t fru, char *path) {
  return fbwc_fruid_write(fru, path);
}

int
pal_is_slot_server(uint8_t fru)
{
  if (fru == FRU_MB) {
    return 1;
  }
  return 0;
}

int
pal_get_poss_pcie_config(uint8_t slot, uint8_t *req_data, uint8_t req_len,
                         uint8_t *res_data, uint8_t *res_len)
{
  uint8_t pcie_conf = PCIE_CONFIG_IOM_E1S;
  uint8_t *data = res_data;
  gpio_value_t gpio_sku_id0, gpio_sku_id1;
  // 0 0 -> dual compute system
  // 1 1 -> single compute system

  gpio_sku_id0 = gpio_get_value_by_shadow("SYS_SKU_ID0_R");
  if (gpio_sku_id0 == GPIO_VALUE_INVALID) {
    return -1;
  }
  gpio_sku_id1 = gpio_get_value_by_shadow("SYS_SKU_ID1_R");
  if (gpio_sku_id1 == GPIO_VALUE_INVALID) {
    return -1;
  }

  if (gpio_sku_id0 == GPIO_VALUE_HIGH && gpio_sku_id1 == GPIO_VALUE_HIGH) {
    pcie_conf = PCIE_CONFIG_IOM_SAS;
  }

  *data++ = pcie_conf;
  *res_len = data - res_data;
  return 0;
}

//For OEM command "CMD_OEM_GET_PLAT_INFO" 0x7E
int pal_get_plat_sku_id(void){
  return 0x00; //   000b: WaimeaCanyon
}

//For OEM command "CMD_OEM_GET_PLAT_INFO" 0x7E
int
pal_get_slot_index(unsigned char payload_id)
{
  return 1; // WaimeaCanyon only have 1 slot(MB)
}

static int
pal_key_index(char *key) {
  int i = 0;

  while(strcmp(key_cfg[i].name, LAST_KEY)) {
    // If Key is valid, return success
    if (!strcmp(key, key_cfg[i].name))
      return i;

    i++;
  }

  syslog(LOG_WARNING, "pal_key_index: invalid key - %s", key);
  return -1;
}

int
pal_get_key_value(char *key, char *value) {
  // Check is key is defined and valid
  if (pal_key_index(key) < 0)
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

int
pal_set_sysfw_ver(uint8_t slot, uint8_t *ver) {
  int i;
  char str[MAX_VALUE_LEN] = {0};
  char tstr[8] = {0};

  for (i = 0; i < SIZE_SYSFW_VER; i++) {
    sprintf(tstr, "%02x", ver[i]);
    strcat(str, tstr);
  }

  return pal_set_key_value(SYSFW_VER_STR, str);
}

int
pal_get_sysfw_ver(uint8_t slot, uint8_t *ver) {
  int ret = -1;
  int i, j;
  char str[MAX_VALUE_LEN] = {0};
  char tstr[8] = {0};

  ret = pal_get_key_value(SYSFW_VER_STR, str);
  if (ret) {
    return ret;
  }

  for (i = 0, j = 0; i < 2*SIZE_SYSFW_VER; i += 2) {
    sprintf(tstr, "%c%c", str[i], str[i+1]);
    ver[j++] = strtol(tstr, NULL, 16);
  }

  return 0;
}

int
pal_get_sys_guid(uint8_t fru, char *guid) {
  return bic_get_sys_guid(fru, (uint8_t *)guid);
}

int
pal_set_sys_guid(uint8_t fru, char *str) {
  char guid[GUID_SIZE] = {0};

  pal_populate_guid(guid, str);
  return bic_set_sys_guid(fru, (uint8_t *)guid);
}

int
pal_get_dev_guid(uint8_t fru, char *guid) {
  char path[128] = {0};
  int fd;
  ssize_t bytes_rd;
  errno = 0;

  snprintf(path, sizeof(path), EEPROM_PATH, I2C_BUS_SCM, SCM_FRU_ADDR);
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

  lseek(fd, OFFSET_DEV_GUID, SEEK_SET);

  bytes_rd = read(fd, guid, GUID_SIZE);
  if (bytes_rd != GUID_SIZE) {
    syslog(LOG_ERR, "%s() read from %s failed: %s", __func__, path, strerror(errno));
  }

  if (fd > 0 ) {
    close(fd);
  }

  return errno;
}

int
pal_set_dev_guid(uint8_t fru, char *str) {
  char guid[GUID_SIZE] = {0};
  char path[128] = {0};
  int fd;
  uint8_t fru_bus = I2C_BUS_SCM;
  ssize_t bytes_wr;
  errno = 0;

  pal_populate_guid(guid, str);

  snprintf(path, sizeof(path), EEPROM_PATH, fru_bus, SCM_FRU_ADDR);

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

  lseek(fd, OFFSET_DEV_GUID, SEEK_SET);

  bytes_wr = write(fd, guid, GUID_SIZE);
  if (bytes_wr != GUID_SIZE) {
    syslog(LOG_ERR, "%s() write to %s failed: %s", __func__, path, strerror(errno));
  }

  close(fd);
  return errno;
}

int
pal_set_def_key_value() {

  for(int i = 0; strcmp(key_cfg[i].name, LAST_KEY) != 0; i++) {
    if (kv_set(key_cfg[i].name, key_cfg[i].def_val, 0, KV_FCREATE | KV_FPERSIST)) {
      syslog(LOG_WARNING, "pal_set_def_key_value: kv_set %s failed.", key_cfg[i].name);
    }
    if (key_cfg[i].function) {
      key_cfg[i].function(KEY_AFTER_INI, key_cfg[i].name);
    }
  }

  return 0;
}

void
pal_get_chassis_status(uint8_t fru, uint8_t *req_data, uint8_t *res_data, uint8_t *res_len) {
  char buff[MAX_VALUE_LEN] = {0};
  int policy = POWER_CFG_UKNOWN;
  uint8_t status = 0;
  int ret = 0;
  unsigned char *data = res_data;

  if (res_data == NULL) {
    syslog(LOG_WARNING, "%s() NULL pointer: *res_data", __func__);
  }
  
  if (res_len == NULL) {
    syslog(LOG_WARNING, "%s() NULL pointer: *res_len", __func__);
  }
  
  memset(&buff, 0, sizeof(buff));
  if (pal_get_key_value("server_por_cfg", buff) == 0) {
    if (!memcmp(buff, "off", strlen("off"))) {
      policy = POWER_CFG_OFF;
    } else if (!memcmp(buff, "lps", strlen("lps"))) {
      policy = POWER_CFG_LPS;
    } else if (!memcmp(buff, "on", strlen("on"))) {
      policy = POWER_CFG_ON;
    } else {
      policy = POWER_CFG_UKNOWN;
    }
  }

  // Current Power State
  ret = pal_get_server_power(fru, &status);
  if (ret == 0) {
    *data++ = status | (policy << 5);
  } else {
    syslog(LOG_WARNING, "%s: pal_get_server_power failed", __func__);
    *data++ = 0x00 | (policy << 5);
  }

  *data++ = 0x00;   // Last Power Event
  *data++ = 0x40;   // Misc. Chassis Status
  *data++ = 0x00;   // Front Panel Button Disable
  *res_len = data - res_data;
}


static int
key_func_por_cfg(int event, void *arg) {
  if (event == KEY_BEFORE_SET) {
    if (strcmp((char *)arg, "lps") && strcmp((char *)arg, "on") && strcmp((char *)arg, "off"))
      return -1;
  }

  return 0;
}
