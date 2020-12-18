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
#include <unistd.h>
#include <stdint.h>
#include <stdbool.h>
#include <fcntl.h>
#include <errno.h>
#include <syslog.h>
#include <ctype.h>
#include <time.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <string.h>
#include <pthread.h>
#include <openbmc/obmc-sensors.h>
#include <openbmc/libgpio.h>
#include <facebook/fbgc_gpio.h>
#include "pal.h"

#define NUM_SERVER_FRU       1
#define NUM_NIC_FRU          1
#define NUM_BMC_FRU          1
#define MAX_FAN_NAME_LEN     32 // include the string terminal
#define MAX_PWM_LABEL_LEN    32 // include the string terminal

const char pal_fru_list[] = "all, server, bmc, uic, dpb, scc, nic, e1s_iocm";

// export to sensor-util
const char pal_fru_list_sensor_history[] = "all, server, uic, nic, e1s_iocm";

// export to power-util
const char pal_server_list[] = "server";

// export to name-util
const char *pal_server_fru_list[NUM_SERVER_FRU] = {"server"};
const char *pal_nic_fru_list[NUM_NIC_FRU] = {"nic"};
const char *pal_bmc_fru_list[NUM_BMC_FRU] = {"bmc"};

// export to fruid-util, only support iocm of FRU_E1S_IOCM
const char pal_fru_list_print[] = "all, server, bmc, uic, dpb, scc, nic, iocm";
const char pal_fru_list_rw[] = "server, bmc, uic, nic, iocm";

// fru name list for pal_get_fru_id(), the name of FRU_E1S_IOCM could be "iocm" or "e1s_iocm"
const char *fru_str_list[][2] = {
  { "all"   , "" },
  { "server", "" },
  { "bmc"   , "" },
  { "uic"   , "" },
  { "dpb"   , "" },
  { "scc"   , "" },
  { "nic"   , "" },
  { "iocm"  , "e1s_iocm" }
};

size_t server_fru_cnt = NUM_SERVER_FRU;
size_t nic_fru_cnt  = NUM_NIC_FRU;
size_t bmc_fru_cnt  = NUM_BMC_FRU;

const char pal_pwm_list[] = "0";
const char pal_tach_list[] = "0...7";

size_t pal_pwm_cnt = 1;
size_t pal_tach_cnt = 8;

// TODO: temporary mapping table, will get from fan config after fan table is ready
uint8_t fanid2pwmid_mapping[] = {0, 0, 0, 0, 0, 0, 0, 0};

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
  {"system_identify", "off", NULL},
  {"server_boot_order", "0100090203ff", NULL},
  {"server_por_cfg", "on", NULL},
  {"sysfw_ver", "0", NULL},
  /* Add more Keys here */
  {NULL, NULL, NULL} /* This is the last key of the list */
};

char * cfg_support_key_list[] = {
  "server_por_cfg",
  NULL /* This is the last key of the list */
};

static int
pal_key_index(char *key) {
  int i = 0;

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

// Check what keys can be set by cfg-util
int
pal_cfg_key_check(char *key) {
  int i = 0;

  while(cfg_support_key_list[i] != NULL) {
    // If Key is valid and can be set, return success
    if (!strncmp(key, cfg_support_key_list[i], strlen(cfg_support_key_list[i]))) {
      return 0;
    }
    i++;
  }
  // If Key could not be set, print syslog and return -1.
  syslog(LOG_WARNING, "%s(): invalid key - %s", __func__, key);

  return -1;
}

int
pal_get_key_value(char *key, char *value) {
  int index = 0;

  // Check key is defined and valid
  if ((index = pal_key_index(key)) < 0) {
    return -1;
  }
  return kv_get(key, value, NULL, KV_FPERSIST);
}

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

int
pal_set_def_key_value() {
  int i = 0;
  int ret = 0, failed_count = 0;

  for (i = 0; key_cfg[i].name != NULL; i++) {
    if ((ret = kv_set(key_cfg[i].name, key_cfg[i].def_val, 0, KV_FCREATE | KV_FPERSIST)) < 0) {
      syslog(LOG_WARNING, "%s(): kv_set failed. %d", __func__, ret);
      failed_count ++;
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

int
pal_get_fru_id(char *str, uint8_t *fru) {
  int fru_id = -1;
  bool found_id = false;

  for (fru_id = FRU_ALL; fru_id <= MAX_NUM_FRUS; fru_id++) {
    if ((strncmp(str, fru_str_list[fru_id][0], MAX_FRU_CMD_STR) == 0) ||
        (strncmp(str, fru_str_list[fru_id][1], MAX_FRU_CMD_STR) == 0)) {
      *fru = fru_id;
      found_id = true;
      break;
    }
  }

  return found_id ? 0 : -1;
}

int
pal_is_fru_ready(uint8_t fru, uint8_t *status) {
  switch (fru) {
    case FRU_SERVER:
    // TODO: Get server power status and BIC ready pin
    case FRU_BMC:
    case FRU_UIC:
    case FRU_DPB:
    case FRU_SCC:
    case FRU_NIC:
    case FRU_E1S_IOCM:
      *status = 1;
      break;
    default:
      *status = 0;
      syslog(LOG_WARNING, "%s() wrong fru id 0x%02x", __func__, fru);
      return PAL_ENOTSUP;
  }

  return 0;
}

int pal_check_gpio_prsnt(uint8_t gpio, int presnt_expect) {

  int ret = GPIO_VALUE_INVALID;

  ret = gpio_get_value_by_shadow(fbgc_get_gpio_name(gpio));

  if (ret == GPIO_VALUE_INVALID) {
    syslog(LOG_ERR, "%s: failed to get gpio value: gpio%d\n",__func__, gpio);
    return -1;
  }

  if (ret == presnt_expect) {
    return FRU_PRESENT;
  } else {
    return FRU_ABSENT;
  }
}

int
pal_is_fru_prsnt(uint8_t fru, uint8_t *status) {
  int ret = 0;
  int e1s_0_ret = 0, e1s_1_ret = 0;

  switch (fru) {
    case FRU_SERVER:
      ret = pal_check_gpio_prsnt(GPIO_COMP_PRSNT_N, GPIO_VALUE_LOW);
      if (ret == -1) {
        *status = 0;
        return PAL_ENOTSUP;
      }
      *status = ret;
      break;
    case FRU_SCC:
      ret = pal_check_gpio_prsnt(GPIO_SCC_LOC_INS_N, GPIO_VALUE_LOW);
      if (ret == -1) {
        *status = 0;
        return PAL_ENOTSUP;
      }
      *status = ret;
      break;
    case FRU_NIC:
      ret = pal_check_gpio_prsnt(GPIO_NIC_PRSNTB3_N, GPIO_VALUE_LOW);
      if (ret == -1) {
        *status = 0;
        return PAL_ENOTSUP;
      }
      *status = ret;
      break;
    case FRU_E1S_IOCM:
      e1s_0_ret = pal_check_gpio_prsnt(GPIO_E1S_1_PRSNT_N, GPIO_VALUE_LOW);
      e1s_1_ret = pal_check_gpio_prsnt(GPIO_E1S_2_PRSNT_N, GPIO_VALUE_LOW);
      if (e1s_0_ret == -1 || e1s_1_ret == -1) {
        *status = 0;
        return PAL_ENOTSUP;
      }
      *status = 0;
      if ( e1s_0_ret == FRU_PRESENT) {
        *status |= E1S0_IOCM_PRESENT_BIT;
      }
      if ( e1s_1_ret == FRU_PRESENT) {
        *status |= E1S1_IOCM_PRESENT_BIT;
      }
      break;
    case FRU_BMC:
    case FRU_UIC:
    case FRU_DPB:
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
pal_get_fruid_name(uint8_t fru, char *name) {
  return fbgc_get_fruid_name(fru, name);
}

int
pal_get_fruid_path(uint8_t fru, char *path) {
  return fbgc_get_fruid_path(fru, path);
}

int
pal_get_fruid_eeprom_path(uint8_t fru, char *path) {
  return fbgc_get_fruid_eeprom_path(fru, path);
}

int
pal_fruid_write(uint8_t fru, char *path) {
  return fbgc_fruid_write(0, path);
}

int
pal_get_fru_list(char *list) {
  snprintf(list, sizeof(pal_fru_list), pal_fru_list);
  return 0;
}

int
pal_get_fan_name(uint8_t fan_id, char *name) {
  if (fan_id >= pal_tach_cnt) {
    syslog(LOG_WARNING, "%s: Invalid fan index: %d", __func__, fan_id);
    return -1;
  }
  snprintf(name, MAX_FAN_NAME_LEN, "Fan %d %s", (fan_id / 2), fan_id % 2 == 0 ? "Front" : "Rear");

  return 0;
}

int
pal_get_fru_name(uint8_t fru, char *name) {
  uint8_t type = 0;

  switch(fru) {
    case FRU_SERVER:
      snprintf(name, MAX_FRU_CMD_STR, "server");
      break;
    case FRU_BMC:
      snprintf(name, MAX_FRU_CMD_STR, "bmc");
      break;
    case FRU_UIC:
      snprintf(name, MAX_FRU_CMD_STR, "uic");
      break;
    case FRU_DPB:
      snprintf(name, MAX_FRU_CMD_STR, "dpb");
      break;
    case FRU_SCC:
      snprintf(name, MAX_FRU_CMD_STR, "scc");
      break;
    case FRU_NIC:
      snprintf(name, MAX_FRU_CMD_STR, "nic");
      break;
    case FRU_E1S_IOCM:
      fbgc_common_get_chassis_type(&type);
      if (type == CHASSIS_TYPE7) {
        snprintf(name, MAX_FRU_CMD_STR, "iocm");
      } else {
        snprintf(name, MAX_FRU_CMD_STR, "e1s");
      }
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

int
pal_set_fan_speed(uint8_t fan_id, uint8_t pwm) {
  char label[MAX_PWM_LABEL_LEN] = {0};
  int zone = 0;

  if (fan_id >= pal_pwm_cnt) {
    syslog(LOG_WARNING, "%s: Invalid fan index: %d", __func__, fan_id);
    return -1;
  }
  zone = fanid2pwmid_mapping[fan_id];
  snprintf(label, sizeof(label), "pwm%d", zone);

  return sensors_write_fan(label, (float)pwm);
}

int
pal_get_pwm_value(uint8_t fan_id, uint8_t *pwm) {
  char label[MAX_PWM_LABEL_LEN] = {0};
  float value = 0;
  int ret = 0;
  int zone = 0;

  if (fan_id >= pal_tach_cnt) {
    syslog(LOG_WARNING, "%s: Invalid fan index: %d", __func__, fan_id);
    return -1;
  }
  zone = fanid2pwmid_mapping[fan_id];
  snprintf(label, sizeof(label), "pwm%d", zone);
  ret = sensors_read_fan(label, &value);
  if (ret == 0) {
    *pwm = (uint8_t)value;
  }

  return ret;
}

// GUID based on RFC4122 format @ https://tools.ietf.org/html/rfc4122
static void
pal_populate_guid(char *guid, char *str) {
  unsigned int secs = 0;
  unsigned int usecs = 0;
  struct timeval tv;
  uint8_t count = 0;
  uint8_t lsb = 0, msb = 0;
  int i = 0, clock_seq = 0;

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
  clock_seq = rand();
  guid[8] = clock_seq & 0xFF;
  guid[9] = (clock_seq >> 8) & 0xFF;

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
  char path[MAX_FILE_PATH] = {0};
  int fd = 0;
  int ret = 0;
  ssize_t bytes_rd = 0;

  // Set path for UIC
  snprintf(path, MAX_FILE_PATH, EEPROM_PATH, I2C_UIC_BUS, UIC_FRU_ADDR);

  // check for file presence
  if (access(path, F_OK)) {
    syslog(LOG_ERR, "%s() unable to access %s: %s", __func__, path, strerror(errno));
    return -1;
  }

  fd = open(path, O_RDONLY);
  if (fd < 0) {
    syslog(LOG_ERR, "%s() unable to open %s: %s", __func__, path, strerror(errno));
    return -1;
  }

  lseek(fd, offset, SEEK_SET);

  bytes_rd = read(fd, guid, GUID_SIZE);
  if (bytes_rd != GUID_SIZE) {
    syslog(LOG_ERR, "%s() read from %s failed: %s", __func__, path, strerror(errno));
    ret = -1;
  }

  close(fd);

  return ret;
}

static int
pal_set_guid(uint16_t offset, char *guid) {
  char path[MAX_FILE_PATH] = {0};
  int fd = 0;
  int ret = 0;
  ssize_t bytes_wr = 0;

  // Set path for UIC
  snprintf(path, MAX_FILE_PATH, EEPROM_PATH, I2C_UIC_BUS, UIC_FRU_ADDR);

  // check for file presence
  if (access(path, F_OK)) {
    syslog(LOG_ERR, "%s() unable to access %s: %s", __func__, path, strerror(errno));
    return -1;
  }

  fd = open(path, O_WRONLY);
  if (fd < 0) {
    syslog(LOG_ERR, "%s() unable to open %s: %s", __func__, path, strerror(errno));
    return -1;
  }

  lseek(fd, offset, SEEK_SET);

  bytes_wr = write(fd, guid, GUID_SIZE);
  if (bytes_wr != GUID_SIZE) {
    syslog(LOG_ERR, "%s() write to %s failed: %s", __func__, path, strerror(errno));
    ret = -1;
  }

  close(fd);

  return ret;
}

int
pal_get_sys_guid(uint8_t fru, char *guid) {
  int ret = 0;

  if (guid == NULL) {
    return -1;
  }

  if (fru == FRU_SERVER) {
    ret = bic_get_sys_guid(fru, (uint8_t *)guid, GUID_SIZE);
    if (ret < 0) {
      syslog(LOG_ERR, "%s() Failed to get system GUID\n", __func__);
    }
  } else {
    ret = -1;
  }

  return ret;
}

int
pal_set_sys_guid(uint8_t fru, char *str) {
  int ret = 0;
  char guid[GUID_SIZE] = {0};

  if (str == NULL) {
    return -1;
  }

  if (fru == FRU_SERVER) {
    pal_populate_guid(guid, str);

    ret = bic_set_sys_guid(fru, (uint8_t *)guid, GUID_SIZE);
    if (ret < 0) {
      syslog(LOG_ERR, "%s() Failed to set system GUID\n", __func__);
    }
  } else {
    ret = -1;
  }

  return ret;
}

int
pal_get_dev_guid(uint8_t fru, char *guid) {
  return pal_get_guid(OFFSET_DEV_GUID, guid);
}

int
pal_set_dev_guid(uint8_t fru, char *str) {
  char guid[GUID_SIZE] = {0};

  pal_populate_guid(guid, str);
  return pal_set_guid(OFFSET_DEV_GUID, guid);
}

// Update the Identification LED for the given fru with the status
int
pal_set_id_led(uint8_t fru, enum LED_HIGH_ACTIVE status) {
  int ret = 0;
  gpio_value_t val = 0;

  if (fru != FRU_UIC) {
    return -1;
  }

  val = (status == LED_ON) ? GPIO_VALUE_HIGH : GPIO_VALUE_LOW;
  ret = gpio_set_value_by_shadow(fbgc_get_gpio_name(GPIO_BMC_LED_PWR_BTN_EN_R), val);

  return ret;
}

int
pal_set_hb_led(uint8_t status) {
  int ret = 0;
  gpio_value_t val = 0;

  if (status == LED_ON) {
    val = GPIO_VALUE_HIGH;
  } else {
    val = GPIO_VALUE_LOW;
  }

  ret = gpio_set_value_by_shadow(fbgc_get_gpio_name(GPIO_BMC_LOC_HEARTBEAT_R), val);

  return ret;
}


int
pal_set_status_led(uint8_t fru, status_led_color color) {
  int ret = 0;
  gpio_value_t val_yellow = 0, val_blue = 0;

  if (fru != FRU_UIC) {
    return -1;
  }

  switch (color) {
  case STATUS_LED_OFF:
    val_yellow = GPIO_VALUE_LOW;
    val_blue   = GPIO_VALUE_LOW;
    break;
  case STATUS_LED_BLUE:
    val_yellow = GPIO_VALUE_LOW;
    val_blue   = GPIO_VALUE_HIGH;
    break;
  case STATUS_LED_YELLOW:
    val_yellow = GPIO_VALUE_HIGH;
    val_blue   = GPIO_VALUE_LOW;
    break;
  default:
    syslog(LOG_ERR, "%s() Invalid LED color: %d\n", __func__, color);
    return -1;
  }

  if (0 != (ret = gpio_set_value_by_shadow(fbgc_get_gpio_name(GPIO_BMC_LED_STATUS_YELLOW_EN_R), val_yellow))) {
    syslog(LOG_ERR, "%s() Failed to set GPIO BMC_LED_STATUS_YELLOW_EN_R to %d\n", __func__, val_yellow);
  }
  if (0 != (ret = gpio_set_value_by_shadow(fbgc_get_gpio_name(GPIO_BMC_LED_STATUS_BLUE_EN_R), val_blue))) {
    syslog(LOG_ERR, "%s() Failed to set GPIO BMC_LED_STATUS_BLUE_EN_R to %d\n", __func__, val_blue);
  }

  return ret;
}

int
pal_set_e1s_led(uint8_t fru, e1s_led_id id, enum LED_HIGH_ACTIVE status) {
  int ret = 0;
  gpio_value_t val = 0;

  if (fru != FRU_E1S_IOCM) {
    return -1;
  }

  if (status == LED_ON) {
    val = GPIO_VALUE_HIGH;
  } else {
    val = GPIO_VALUE_LOW;
  }

  if (id == ID_E1S0_LED) {
    ret = gpio_set_value_by_shadow(fbgc_get_gpio_name(GPIO_E1S_1_LED_ACT), val);
  } else if (id == ID_E1S1_LED) {
    ret = gpio_set_value_by_shadow(fbgc_get_gpio_name(GPIO_E1S_2_LED_ACT), val);
  } else {
    return -1;
  }

  return ret;
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

bool
pal_is_fw_update_ongoing_system(void) {
  uint8_t i = 0;

  for (i = FRU_SERVER; i < FRU_CNT; i++) {
    if (pal_is_fw_update_ongoing(i) == true) {
      return true;
    }
  }

  return false;
}

int
pal_get_nic_fru_id(void)
{
  return FRU_NIC;
}

int
pal_is_slot_server(uint8_t fru) {
  return (fru == FRU_SERVER) ? 1 : 0;
}

