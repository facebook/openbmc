/*
 *
 * Copyright 2015-present Facebook. All Rights Reserved.
 *
 * This file contains code to support IPMI2.0 Specificaton available @
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
#include <stdint.h>
#include <stdbool.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <syslog.h>
#include <sys/mman.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <pthread.h>
#include <openbmc/obmc-i2c.h>
#include <openbmc/obmc-sensor.h>
#include <openbmc/kv.h>
#include "pal.h"
#include <string.h>

#define BIT(value, index) ((value >> index) & 1)

#define LIGHTNING_PLATFORM_NAME "Lightning"
#define LAST_KEY "last_key"

#define GPIO_VAL "/sys/class/gpio/gpio%d/value"
#define GPIO_DIR "/sys/class/gpio/gpio%d/direction"

#define GPIO_HAND_SW_ID1 138
#define GPIO_HAND_SW_ID2 139
#define GPIO_HAND_SW_ID4 140
#define GPIO_HAND_SW_ID8 141

#define GPIO_DEBUG_RST_BTN 54
#define GPIO_DEBUG_UART_COUNT 125
#define GPIO_BMC_UART_SWITCH 123

#define GPIO_RESET_PCIE_SWITCH 8

#define GPIO_HB_LED 115
#define GPIO_BMC_SELF_TRAY_INTRU 108 // 0: tray pull-in, 1: tray pull-out
#define GPIO_BMC_PEER_TRAY_INTRU 0   // 0: tray pull-in, 1: tray pull-out
#define GPIO_TRAY_LOCATION_ID 55 // 0: lower tray, 1: upper tray

#define GPIO_DEBUG_CARD_HIGH_HEX_0 48
#define GPIO_DEBUG_CARD_HIGH_HEX_1 49
#define GPIO_DEBUG_CARD_HIGH_HEX_2 50
#define GPIO_DEBUG_CARD_HIGH_HEX_3 51
#define GPIO_DEBUG_CARD_LOW_HEX_0 72
#define GPIO_DEBUG_CARD_LOW_HEX_1 73
#define GPIO_DEBUG_CARD_LOW_HEX_2 74
#define GPIO_DEBUG_CARD_LOW_HEX_3 75

#define I2C_DEV_FAN "/dev/i2c-5"
#define I2C_ADDR_FAN 0x2d
#define FAN_REGISTER_H 0x80
#define FAN_REGISTER_L 0x81
#define I2C_ADDR_FAN_LED 0x60

#define LARGEST_DEVICE_NAME 120
#define PWM_DIR "/sys/devices/platform/ast_pwm_tacho.0"
#define PWM_UNIT_MAX 96

#define I2C_MUX_FLASH1 0
#define I2C_MUX_FLASH2 1
#define I2C_DEV_FLASH1 "/dev/i2c-7"
#define I2C_DEV_FLASH2 "/dev/i2c-8"
#define MAX_SERIAL_NUM 20

/* NVMe-MI SSD Status Flag bit mask */
#define NVME_SFLGS_MASK_BIT 0x28  //Just check bit 3,5
#define NVME_SFLGS_CHECK_VALUE 0x28 // normal - bit 3,5 = 1

/* NVMe-MI SSD SMART Critical Warning */
#define NVME_SMART_WARNING_MASK_BIT 0x1F // Check bit 0~4

const char pal_fru_list[] = "all, peb, pdpb, fcb";
const char pal_fru_list_wo_all[] = "peb, pdpb, fcb";
size_t pal_pwm_cnt = 1;
size_t pal_tach_cnt = 12;
const char pal_pwm_list[] = "0";
const char pal_tach_list[] = "0...11";
/* A mapping tabel for fan id to pwm id.  */
uint8_t fanid2pwmid_mapping[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

/* sensornum-errorcode mapping table */
const uint8_t sennum2errcode_mapping[MAX_SENSOR_NUM] = {
//   0     1     2     3     4     5     6     7     8     9     A     B     C     D     E     F
  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, // 0x00 - 0x0F
  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, // 0x10 - 0x1F
  0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0xFF, 0xFF, 0xFF, 0xFF, // 0x20 - 0x2F
  0xFF, 0xFF, 0xFF, 0xFF, 0x2F, 0x2F, 0x2F, 0x2F, 0x2F, 0x30, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, // 0x30 - 0x3F
  0xFF, 0xFF, 0x2C, 0x2C, 0x2C, 0x2C, 0x2C, 0x2C, 0x2C, 0x2C, 0x2D, 0x2E, 0x2E, 0xFF, 0x2E, 0x20, // 0x40 - 0x4F
  0x1F, 0x22, 0x21, 0x28, 0x27, 0xFF, 0x25, 0x26, 0x24, 0x23, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, // 0x50 - 0x5F
  0xFF, 0xFF, 0xFF, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3A, 0x3B, 0x3C, 0x3D, 0x3E, // 0x60 - 0x6F
  0x3F, 0x40, 0xFF, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3A, 0x3B, 0x3C, 0x3D, 0x3E, // 0x70 - 0x7F
  0x3F, 0x40, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, // 0x80 - 0x8F
  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, // 0x90 - 0x9F
  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, // 0xA0 - 0xAF
  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, // 0xB0 - 0xBF
  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, // 0xC0 - 0xCF
  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, // 0xD0 - 0xDF
  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, // 0xE0 - 0xEF
  0x2C, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF  // 0xF0 - 0xFF
};

char * key_list[] = {
"peb_sensor_health",
"pdpb_sensor_health",
"fcb_sensor_health",
"bmc_health",
"system_identify",
/* Add more Keys here */
LAST_KEY /* This is the last key of the list */
};

char * def_val_list[] = {
  "1", /* peb_sensor_health */
  "1", /* pdbb_sensor_health */
  "1", /* fcb_sensor_health */
  "1", /* bmc_health */
  "off", /* system_identify */
  /* Add more def values for the correspoding keys*/
  LAST_KEY /* Same as last entry of the key_list */
};

// Helper Functions
static int
read_device(const char *device, int *value) {
  FILE *fp;
  int rc;

  fp = fopen(device, "r");
  if (!fp) {
    int err = errno;
#ifdef DEBUG
    syslog(LOG_INFO, "failed to open device %s", device);
#endif
    return err;
  }

  rc = fscanf(fp, "%d", value);
  fclose(fp);
  if (rc != 1) {
#ifdef DEBUG
    syslog(LOG_INFO, "failed to read device %s", device);
#endif
    return ENOENT;
  } else {
    return 0;
  }
}

static int
write_device(const char *device, const char *value) {
  FILE *fp;
  int rc;

  fp = fopen(device, "w");
  if (!fp) {
    int err = errno;
#ifdef DEBUG
    syslog(LOG_INFO, "failed to open device for write %s", device);
#endif
    return err;
  }

  rc = fputs(value, fp);
  fclose(fp);

  if (rc < 0) {
#ifdef DEBUG
    syslog(LOG_INFO, "failed to write device %s", device);
#endif
    return ENOENT;
  } else {
    return 0;
  }
}

static int
read_device_hex(const char *device, int *value) {
  FILE *fp;
  int rc;

  fp = fopen(device, "r");
  if (!fp) {
#ifdef DEBUG
    syslog(LOG_INFO, "failed to open device %s", device);
#endif
    return errno;
  }

  rc = fscanf(fp, "%x", value);
  fclose(fp);
  if (rc != 1) {
#ifdef DEBUG
    syslog(LOG_INFO, "failed to read device %s", device);
#endif
    return ENOENT;
  } else {
    return 0;
  }
}

// Platform Abstraction Layer (PAL) Functions
int
pal_get_platform_name(char *name) {
  strcpy(name, LIGHTNING_PLATFORM_NAME);

  return 0;
}

int
pal_get_num_slots(uint8_t *num) {
  *num = MAX_NUM_SLOTS;

  return 0;
}

int
pal_is_fru_prsnt(uint8_t fru, uint8_t *status) {

  switch(fru) {
    case FRU_PEB:
    case FRU_PDPB:
    case FRU_FCB:
      *status = 1;
      break;
    default:
      return -1;
  }
  return 0;
}

// Return the DEBUGCARD's UART Channel Button Status
int
pal_get_uart_chan_btn(uint8_t *status) {

  char path[64] = {0};
  int val;

  sprintf(path, GPIO_VAL, GPIO_DEBUG_UART_COUNT);
  if (read_device(path, &val))
    return -1;

  *status = (uint8_t) val;

  return 0;
}

// Return the current uart position
int
pal_get_uart_chan(uint8_t *status) {

  char path[64] = {0};
  int val;

  sprintf(path, GPIO_VAL, GPIO_BMC_UART_SWITCH);
  if (read_device(path, &val))
    return -1;

  *status = (uint8_t) val;

  return 0;
}

// Set the UART Channel
int
pal_set_uart_chan(uint8_t status) {

  char path[64] = {0};
  char *val;

  val = (status == 0) ? "0": "1";

  sprintf(path, GPIO_VAL, GPIO_BMC_UART_SWITCH);
  if (write_device(path, val))
    return -1;

  return 0;
}

// Return the DEBUGCARD's Reset Button status
int
pal_get_rst_btn(uint8_t *status) {

  char path[64] = {0};
  int val;

  sprintf(path, GPIO_VAL, GPIO_DEBUG_RST_BTN);
  if (read_device(path, &val))
    return -1;

  *status = (uint8_t) val;

  return 0;
}

// Update the LED for the given slot with the status
int
pal_set_led(uint8_t led, uint8_t status) {

  char path[64] = {0};
  char *val;

  if (status) {
    val = "1";
  } else {
    val = "0";
  }

  sprintf(path, GPIO_VAL, led);
  if (write_device(path, val)) {
    return -1;
  }

  return 0;
}

// Update Heartbeat LED
int
pal_set_hb_led(uint8_t status) {

  return pal_set_led(LED_HB, status);
}

int
pal_get_fru_id(char *str, uint8_t *fru) {

  return lightning_common_fru_id(str, fru);
}

int
pal_get_fru_name(uint8_t fru, char *name) {

  return lightning_common_fru_name(fru, name);
}

int
pal_get_fru_sensor_list(uint8_t fru, uint8_t **sensor_list, int *cnt) {

  uint8_t sw = 0;
  uint8_t sku = 0;

  switch(fru) {
    case FRU_PEB:
      while (lightning_pcie_switch(fru, &sw) < 0);

      if (sw == PCIE_SW_PMC) {
        *sensor_list = (uint8_t *) peb_sensor_pmc_list;
        *cnt = peb_sensor_pmc_cnt;
      } else if (sw == PCIE_SW_PLX) {
        *sensor_list = (uint8_t *) peb_sensor_plx_list;
        *cnt = peb_sensor_plx_cnt;
      } else
        return -1;
      break;
    case FRU_PDPB:
      while (lightning_ssd_sku(&sku) < 0);

      if (sku == U2_SKU) {
        *sensor_list = (uint8_t *) pdpb_u2_sensor_list;
        *cnt = pdpb_u2_sensor_cnt;
      } else if (sku == M2_SKU) {
        *sensor_list = (uint8_t *) pdpb_m2_sensor_list;
        *cnt = pdpb_m2_sensor_cnt;
      } else
        return -1;
      break;
    case FRU_FCB:
      *sensor_list = (uint8_t *) fcb_sensor_list;
      *cnt = fcb_sensor_cnt;
      break;
    default:
#ifdef DEBUG
      syslog(LOG_WARNING, "pal_get_fru_sensor_list: Wrong fru id %u", fru);
#endif
      return -1;
  }
    return 0;
}


int
pal_sensor_sdr_init(uint8_t fru, sensor_info_t *sinfo) {
  return lightning_sensor_sdr_init(fru, sinfo);
}

int
pal_sensor_read_raw(uint8_t fru, uint8_t sensor_num, void *value) {

  char key[MAX_KEY_LEN] = {0};
  char str[MAX_VALUE_LEN] = {0};
  int ret, retry = 0;
  FILE *fp = NULL;
  int tmp = 0;
  bool isNegative = false;
  int round_value_tmp = 0;

  switch(fru) {
    case FRU_PEB:
      sprintf(key, "peb_sensor%d", sensor_num);
      break;
    case FRU_PDPB:
      sprintf(key, "pdpb_sensor%d", sensor_num);
      break;
    case FRU_FCB:
      sprintf(key, "fcb_sensor%d", sensor_num);
      break;
  }

  //if enclosure-util read SSD, skip SSD monitor
  if ((access(SKIP_READ_SSD_TEMP, F_OK) == 0) &&
      (((sensor_num >= PDPB_SENSOR_FLASH_TEMP_0) && (sensor_num <= PDPB_SENSOR_FLASH_TEMP_14)) ||
      ((sensor_num >= PDPB_SENSOR_AMB_TEMP_0) && (sensor_num <= PDPB_SENSOR_AMB_TEMP_14)))) {
    fp = fopen(SKIP_READ_SSD_TEMP, "r+");
    if (!fp) {
      return -1;
    }

    /* To avoid the flag file is not removed by enclosure-util */
    fscanf(fp, "%d ", &tmp);
    if (tmp < 1) {
      fclose(fp);
      remove(SKIP_READ_SSD_TEMP);
      return -1;
    }
    else {
      rewind(fp);
      fprintf(fp, "%d ", tmp-1);
    }

    fclose(fp);
    return -1;
  }

  // Add retry to avoid N/A which caused by sesnor reading collision
  while (retry < MAX_RETRY) {
    ret = lightning_sensor_read(fru, sensor_num, value);

    if (!ret)
      break;
    retry++;
  }

  if(ret < 0) {
    syslog(LOG_WARNING,  "%s(): lightning_sensor_read() failed", __func__);
    strcpy(str, "NA");
  }
  else if (ret == 1) { //case: skip monitoring due to enclosure-util running. but not return NA
    return -1;
  }
  else {

    // Round the current sensor value to the nearest hundredth before checking the threshold and cached key/val
    if((*((float*)value)) < 0) {
        isNegative = true;
        *((float*)value) = -(*((float*)value));
    }

    round_value_tmp = ((*((float*)value)) * 100) + 0.5;
    *((float*)value) = round_value_tmp / (100*1.0);

    if(isNegative) {
        *((float*)value) = -(*((float*)value));
    }

    // On successful sensor read
    sprintf(str, "%.2f", *((float*)value));
  }

  if(kv_set(key, str, 0, 0) < 0) {
#ifdef DEBUG
      syslog(LOG_WARNING, "pal_sensor_read_raw: cache_set key = %s, str = %s failed.", key, str);
#endif
    return -1;
  }
  else {
    return ret;
  }
}

int
pal_get_sensor_threshold(uint8_t fru, uint8_t sensor_num, uint8_t thresh, void *value) {
  return lightning_sensor_threshold(fru, sensor_num, thresh, value);
}

int
pal_get_sensor_name(uint8_t fru, uint8_t sensor_num, char *name) {
  return lightning_sensor_name(fru, sensor_num, name);
}

int
pal_get_sensor_units(uint8_t fru, uint8_t sensor_num, char *units) {
  return lightning_sensor_units(fru, sensor_num, units);
}

int
pal_get_sensor_poll_interval(uint8_t fru, uint8_t sensor_num, uint32_t *value) {
  return lightning_sensor_poll_interval(fru, sensor_num, value);
}

int
pal_get_fruid_path(uint8_t fru, char *path) {
  return lightning_get_fruid_path(fru, path);
}

int
pal_get_fruid_eeprom_path(uint8_t fru, char *path) {
  return lightning_get_fruid_eeprom_path(fru, path);
}

int
pal_get_fruid_name(uint8_t fru, char *name) {
  return lightning_get_fruid_name(fru, name);
}

static int
get_key_value(char* key, char *value) {
  int ret;

  if (!strcmp(key, "system_identify")) {
    ret = kv_get(key, value, NULL, KV_FPERSIST);
  } else {
    ret = kv_get(key, value, NULL, 0);
  }
  return ret;
}

static int
set_key_value(char *key, char *value) {
  int ret;

  if (!strcmp(key, "system_identify")) {
    ret = kv_set(key, value, 0, KV_FPERSIST);
  } else {
    ret = kv_set(key, value, 0, 0);
  }
  return ret;
}

int
pal_set_def_key_value() {
  int i;

  for (i = 0; strcmp(key_list[i], LAST_KEY) != 0; i++) {
    unsigned int flag = KV_FCREATE;
    if (!strcmp(key_list[i], "system_identify")) {
      flag |= KV_FPERSIST;
    }
    if (kv_set(key_list[i], def_val_list[i], 0, flag) != 0) {
#ifdef DEBUG
          syslog(LOG_WARNING, "pal_set_def_key_value: kv_set failed. %d", ret);
#endif
    }
  }
  return 0;
}

static int
pal_key_check(char *key) {
  int i;

  i = 0;
  while(strcmp(key_list[i], LAST_KEY)) {

    // If Key is valid, return success
    if (!strcmp(key, key_list[i]))
      return 0;

    i++;
  }

#ifdef DEBUG
  syslog(LOG_WARNING, "pal_key_check: invalid key - %s", key);
#endif
  return -1;
}

int
pal_set_key_value(char *key, char *value) {
  int i = 0;
  int max_retry = 5;
  int ret = 0;

  // Check is key is defined and valid
  if (pal_key_check(key))
    return -1;

  //Retry for max_retry Times
  for (i = 0; i < max_retry; i++) {
    ret = 0;
    ret = set_key_value(key, value);
    if (ret != 0) {
      syslog(LOG_ERR, "%s, failed to write device (%s), ret: %d, retry: %d", __func__, key, ret, i);
    }
    else {
      break;
    }
    msleep(100);
  }

  return ret;
}

int
pal_get_key_value(char *key, char *value) {
  int i = 0;
  int max_retry = 5;
  int ret = 0;

  // Check is key is defined and valid
  if (pal_key_check(key))
    return -1;

  //Retry for max_retry Times
  for (i = 0; i < max_retry; i++) {
    ret = get_key_value(key, value);
    if (ret != 0) {
      syslog(LOG_ERR, "%s, failed to read device (%s), ret: %d, retry: %d", __func__, key, ret, i);
    }
    else {
      break;
    }
    msleep(100);
  }

  return ret;
}

void
pal_dump_key_value(void) {
  int i = 0;
  int ret = 0;

  char value[MAX_VALUE_LEN] = {0x0};

  while (strcmp(key_list[i], LAST_KEY)) {
    printf("%s:", key_list[i]);
    if ((ret = get_key_value(key_list[i], value)) < 0) {
      printf("\n");
    } else {
      printf("%s\n",  value);
    }
    i++;
    memset(value, 0, MAX_VALUE_LEN);
  }
}

int
pal_set_sensor_health(uint8_t fru, uint8_t value) {

  char key[MAX_KEY_LEN] = {0};
  char cvalue[MAX_VALUE_LEN] = {0};

  switch(fru) {
    case FRU_PEB:
      strcpy(key, "peb_sensor_health");
      break;
    case FRU_PDPB:
      strcpy(key, "pdpb_sensor_health");
      break;
    case FRU_FCB:
      strcpy(key, "fcb_sensor_health");
      break;
    case BMC_HEALTH:
      strcpy(key, "bmc_health");
      break;

    default:
      return -1;
  }

  sprintf(cvalue, (value > 0) ? "1": "0");

  return kv_set(key, cvalue, 0, 0);
}

int
pal_get_fru_health(uint8_t fru, uint8_t *value) {

  char cvalue[MAX_VALUE_LEN] = {0};
  char key[MAX_KEY_LEN] = {0};
  int ret;

  switch(fru) {
    case FRU_PEB:
      strcpy(key, "peb_sensor_health");
      break;
    case FRU_PDPB:
      strcpy(key, "pdpb_sensor_health");
      break;
    case FRU_FCB:
      strcpy(key, "fcb_sensor_health");
      break;
    case BMC_HEALTH:
      strcpy(key, "bmc_health");
      break;

    default:
      return -1;
  }

  ret = kv_get(key, cvalue, NULL, 0);
  if (ret) {
    return ret;
  }

  *value = atoi(cvalue);

  return 0;
}

int
pal_get_fru_list(char *list) {

  strcpy(list, pal_fru_list);
  return 0;
}

int
pal_get_fan_name(uint8_t num, char *name) {

  switch(num) {

    case FAN_1_FRONT:
      sprintf(name, "Fan 1 Front");
      break;

    case FAN_1_REAR:
      sprintf(name, "Fan 1 Rear");
      break;

    case FAN_2_FRONT:
      sprintf(name, "Fan 2 Front");
      break;

    case FAN_2_REAR:
      sprintf(name, "Fan 2 Rear");
      break;

    case FAN_3_FRONT:
      sprintf(name, "Fan 3 Front");
      break;

    case FAN_3_REAR:
      sprintf(name, "Fan 3 Rear");
      break;

    case FAN_4_FRONT:
      sprintf(name, "Fan 4 Front");
      break;

    case FAN_4_REAR:
      sprintf(name, "Fan 4 Rear");
      break;

    case FAN_5_FRONT:
      sprintf(name, "Fan 5 Front");
      break;

    case FAN_5_REAR:
      sprintf(name, "Fan 5 Rear");
      break;

    case FAN_6_FRONT:
      sprintf(name, "Fan 6 Front");
      break;

    case FAN_6_REAR:
      sprintf(name, "Fan 6 Rear");
      break;

    default:
      return -1;
  }

  return 0;
}

static int
write_fan_value(const int fan, const char *device, const int value) {
  char full_name[LARGEST_DEVICE_NAME];
  char device_name[LARGEST_DEVICE_NAME];
  char output_value[LARGEST_DEVICE_NAME];

  snprintf(device_name, LARGEST_DEVICE_NAME, device, fan);
  snprintf(full_name, LARGEST_DEVICE_NAME, "%s/%s", PWM_DIR, device_name);
  snprintf(output_value, LARGEST_DEVICE_NAME, "%d", value);
  return write_device(full_name, output_value);
}

int
pal_set_fan_speed(uint8_t fan, uint8_t pwm) {
  int unit;
  int ret = 0;

  if (fan >= pal_pwm_cnt) {
    syslog(LOG_INFO, "pal_set_fan_speed: fan number is invalid - %d", fan);
    return -1;
  }

  // Convert the percentage to our 1/96th unit.
  unit = pwm * PWM_UNIT_MAX / 100;

  // For 0%, turn off the PWM entirely
  if (unit == 0) {
    ret = write_fan_value(fan, "pwm%d_en", 0);
    if (ret < 0) {
      syslog(LOG_INFO, "set_fan_speed: write_fan_value failed");
      return -1;
    }

    return 0;

  // For 100%, set falling and rising to the same value
  } else if (unit == PWM_UNIT_MAX) {
    unit = 0;
  }

  ret = write_fan_value(fan, "pwm%d_type", 0);
  if (ret < 0) {
    syslog(LOG_INFO, "set_fan_speed: write_fan_value failed");
    return -1;
  }

  ret = write_fan_value(fan, "pwm%d_rising", 0);
  if (ret < 0) {
    syslog(LOG_INFO, "set_fan_speed: write_fan_value failed");
    return -1;
  }

  ret = write_fan_value(fan, "pwm%d_falling", unit);
  if (ret < 0) {
    syslog(LOG_INFO, "set_fan_speed: write_fan_value failed");
    return -1;
  }

  ret = write_fan_value(fan, "pwm%d_en", 1);
  if (ret < 0) {
    syslog(LOG_INFO, "set_fan_speed: write_fan_value failed");
    return -1;
  }

  return 0;
}

int
pal_get_fan_speed(uint8_t fan, int *rpm) {

  int ret;
  float value;

  ret = sensor_cache_read(FRU_FCB, FCB_SENSOR_FAN1_FRONT_SPEED + fan, &value);
  if (ret == 0)
    *rpm = (int) value;

  return ret;
}

int
pal_is_fru_ready(uint8_t fru, uint8_t *status) {

  *status = 1;

  return 0;
}

int
pal_get_pwm_value(uint8_t fan_num, uint8_t *value) {
  char path[64] = {0};
  char device_name[64] = {0};
  int val = 0;
  int pwm_enable = 0;

  snprintf(device_name, LARGEST_DEVICE_NAME, "pwm%d_en", fanid2pwmid_mapping[fan_num]);
  snprintf(path, LARGEST_DEVICE_NAME, "%s/%s", PWM_DIR, device_name);
  if (read_device(path, &pwm_enable)) {
    syslog(LOG_INFO, "pal_get_pwm_value: read %s failed", path);
    return -1;
  }

  // Check the PWM is enable or not
  if(pwm_enable) {
    // fan number should in this range
    if(fan_num >= 0 && fan_num <= 11)
      snprintf(device_name, LARGEST_DEVICE_NAME, "pwm%d_falling", fanid2pwmid_mapping[fan_num]);
    else {
      syslog(LOG_INFO, "pal_get_pwm_value: fan number is invalid - %d", fan_num);
      return -1;
    }

    snprintf(path, LARGEST_DEVICE_NAME, "%s/%s", PWM_DIR, device_name);

    if (read_device_hex(path, &val)) {
      syslog(LOG_INFO, "pal_get_pwm_value: read %s failed", path);
      return -1;
    }
    if(val)
      *value = (100 * val) / PWM_UNIT_MAX;
    else
      // 0 means duty cycle is 100%
      *value = 100;
  }
  else
    //PWM is disable
    *value = 0;


  return 0;
}

int
pal_set_fan_led(uint8_t num, uint8_t operation) {

  int dev, ret, res;
  int led_offset;
  uint8_t reg, data;

  if(num > MAX_FAN_LED_NUM) {
    syslog(LOG_ERR, "%s: Wrong LED ID\n", __func__);
    return -1;
  }

  dev = open(I2C_DEV_FAN, O_RDWR);
  if(dev < 0) {
    syslog(LOG_ERR, "%s: open() failed\n", __func__);
    return -1;
  }

  ret = ioctl(dev, I2C_SLAVE, I2C_ADDR_FAN_LED);
  if(ret < 0) {
    syslog(LOG_ERR, "%s: ioctl() assigned i2c addr failed\n", __func__);
    close(dev);
    return -1;
  }

  led_offset = num;
  if(num < 4) {
    reg = REG_LS0;
  } else {
    reg = REG_LS1;
    led_offset -= 4;
  }

  //Read the input register
  res = i2c_smbus_read_byte_data(dev, reg);
  if(res < 0) {
    close(dev);
    syslog(LOG_ERR, "%s: i2c_smbus_read_byte_data failed\n", __func__);
    return -1;
  }

  data = res & ~(3 << (led_offset << 1));

  switch(operation) {
    case FAN_LED_ON:
      break;
    case FAN_LED_OFF:
      data |= (1 << (led_offset << 1));
      break;
    case FAN_LED_BLINK_PWM0_RATE:
      data |= (2 << (led_offset << 1));
      break;
    case FAN_LED_BLINK_PWM1_RATE:
      data |= (3 << (led_offset << 1));
      break;
    default:
      break;
  }

  res = i2c_smbus_write_byte_data(dev, reg, data);
  if(res < 0) {
    close(dev);
    syslog(LOG_ERR, "%s: i2c_smbus_write_byte_data failed\n", __func__);
    return -1;
  }
  close(dev);
  return 0;
}

int
pal_fan_dead_handle(int fan_num) {
  int ret;

  /* Because two fans map to one LED, and fan ID start at 1 */
  ret = pal_set_fan_led( ((fan_num - FAN_INDEX_BASE) / 2), FAN_LED_OFF);

  if(ret < 0) {
    syslog(LOG_ERR, "%s: pal_set_fan_led failed\n", __func__);
    return -1;
  }

  return 0;
}

int
pal_fan_recovered_handle(int fan_num) {
  int ret;

  /* Because two fans map to one LED, and fan ID start at 1 */
  ret = pal_set_fan_led( ((fan_num - FAN_INDEX_BASE) / 2), FAN_LED_ON);

  if(ret < 0) {
    syslog(LOG_ERR, "%s: pal_set_fan_led failed\n", __func__);
    return -1;
  }

  return 0;
}

// Reset PCIe Switch
int
pal_reset_pcie_switch() {

  char path[64] = {0};

  sprintf(path, GPIO_VAL, GPIO_RESET_PCIE_SWITCH);

  if (write_device(path, "0"))
    return -1;

  msleep(100);

  if (write_device(path, "1"))
    return -1;

  return 0;
}

int
pal_self_tray_location(uint8_t *value) {

  char path[64] = {0};
  int val;

  sprintf(path, GPIO_VAL, GPIO_TRAY_LOCATION_ID);
  if (read_device(path, &val))
    return -1;

  *value = (uint8_t) val;

  return 0;
}

int
pal_self_tray_insertion(uint8_t *value) {

  char path[64] = {0};
  int val;

  sprintf(path, GPIO_VAL, GPIO_BMC_SELF_TRAY_INTRU);
  if (read_device(path, &val))
    return -1;

  *value = (uint8_t) val;

  return 0;
}

int
pal_peer_tray_insertion(uint8_t *value) {

  char path[64] = {0};
  int val;

  sprintf(path, GPIO_VAL, GPIO_BMC_PEER_TRAY_INTRU);
  if (read_device(path, &val))
    return -1;

  *value = (uint8_t) val;

  return 0;
}

int
pal_get_tray_location(char *self_tray_name, uint8_t self_len,
                      char *peer_tray_name, uint8_t peer_len,
                      uint8_t *peer_tray_pwr) {
  uint8_t val = 0;
  char cvalue[MAX_VALUE_LEN];

  if ((self_tray_name == NULL) || (peer_tray_name == NULL)){
    syslog(LOG_ERR, "%s(): Error - Null Pointer", __func__);
  }

  // We already have this information
  if (kv_get("tray_location", cvalue, NULL, 0) == 0) {
    return 0;
  }

  if (pal_self_tray_location(&val) == -1)
    return -1;

  if (val == 1) {
    snprintf(self_tray_name, self_len, "Upper Tray");
    snprintf(peer_tray_name, peer_len, "Lower Tray");
    *peer_tray_pwr = FCB_SENSOR_P12VL;
  } else if (val == 0) {
    snprintf(self_tray_name, self_len, "Lower Tray");
    snprintf(peer_tray_name, peer_len, "Upper Tray");
    *peer_tray_pwr = FCB_SENSOR_P12VU;
  } else {
    syslog(LOG_WARNING, "%s(): invalid tray_location_id: %d", __func__, val);
    return -1;
  }

  if (self_len > MAX_VALUE_LEN){
    syslog(LOG_ERR, "%s(): Invalid Parameter. self_len:%d, max len:%d",
                     __func__, self_len, MAX_VALUE_LEN);
    return -1;
  }

  return kv_set("tray_location", self_tray_name, 0, 0);
}


void
pal_log_clear(char *fru) {
  if (!strcmp(fru, "peb")) {
    pal_set_key_value("peb_sensor_health", "1");
  } else if (!strcmp(fru, "pdpb")) {
    pal_set_key_value("pdpb_sensor_health", "1");
  } else if (!strcmp(fru, "fcb")) {
    pal_set_key_value("fcb_sensor_health", "1");
  } else if (!strcmp(fru, "all")) {
    pal_set_key_value("peb_sensor_health", "1");
    pal_set_key_value("pdpb_sensor_health", "1");
    pal_set_key_value("fcb_sensor_health", "1");
    pal_set_key_value("bmc_health", "1");
  }
}

void
pal_sensor_assert_handle(uint8_t fru, uint8_t snr_num, float val, uint8_t thresh) {
  uint8_t *sensorStatus = NULL;
  uint8_t error_code_num = sennum2errcode_mapping[snr_num];
  int fd, ret;

  if (access(ERR_CODE_FILE, F_OK) == -1)
    fd = open("/tmp/share_sensor_status", O_CREAT | O_RDWR | O_TRUNC, 00777);
  else
    fd = open("/tmp/share_sensor_status", O_CREAT | O_RDWR, 00777);
  if (fd == -1) {
    syslog(LOG_ERR, "%s(): Error opening file for reading", __func__);
    return;
  }

  ret = pal_flock_retry(fd);
  if (ret) {
    syslog(LOG_WARNING, "%s(): failed to flock on %s. %s", __func__, ERR_CODE_FILE, strerror(errno));
    close(fd);
    return;
  }

  lseek(fd, (sizeof(uint8_t) * MAX_SENSOR_NUM) - 1, SEEK_SET);
  write(fd, "", 1);

  sensorStatus = (uint8_t*) mmap(NULL, (sizeof(uint8_t) * MAX_SENSOR_NUM),
                              PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

  if (sensorStatus == MAP_FAILED){
    syslog(LOG_ERR, "%s(): Error mmapping the file. %s", __func__, strerror(errno));
    flock(fd, LOCK_UN);
    close(fd);
    return;
  }

  switch (thresh) {
  case UNC_THRESH:
    sensorStatus[snr_num] = SETBIT(sensorStatus[snr_num], UNC_THRESH);
    break;
  case UCR_THRESH:
    sensorStatus[snr_num] = SETBIT(sensorStatus[snr_num], UCR_THRESH);
    break;
  case UNR_THRESH:
    sensorStatus[snr_num] = SETBIT(sensorStatus[snr_num], UNR_THRESH);
    break;
  case LNC_THRESH:
    sensorStatus[snr_num] = SETBIT(sensorStatus[snr_num], LNC_THRESH);
    break;
  case LCR_THRESH:
    sensorStatus[snr_num] = SETBIT(sensorStatus[snr_num], LCR_THRESH);
    break;
  case LNR_THRESH:
    sensorStatus[snr_num] = SETBIT(sensorStatus[snr_num], LNR_THRESH);
    break;
  default:
    syslog(LOG_ERR, "%s(): wrong threshold value", __func__);
    return;
  }

  pal_write_error_code_file(error_code_num, ERR_ASSERT);

  munmap(sensorStatus, sizeof(bool) * MAX_SENSOR_NUM);
  flock(fd, LOCK_UN);
  close(fd);
}

void
pal_sensor_deassert_handle(uint8_t fru, uint8_t snr_num, float val, uint8_t thresh) {
  uint8_t *sensorStatus = NULL;
  uint8_t error_code_num = sennum2errcode_mapping[snr_num];
  int i, fd, ret;
  bool tmp = 0;

  if (access(ERR_CODE_FILE, F_OK) == -1)
    fd = open("/tmp/share_sensor_status", O_CREAT | O_RDWR | O_TRUNC, 00777);
  else
    fd = open("/tmp/share_sensor_status", O_CREAT | O_RDWR, 00777);
  if (fd == -1) {
    syslog(LOG_ERR, "%s(): Error opening file for reading", __func__);
    return;
  }

  ret = pal_flock_retry(fd);
  if (ret) {
    syslog(LOG_WARNING, "%s(): failed to flock on %s. %s", __func__, ERR_CODE_FILE, strerror(errno));
    close(fd);
    return;
  }

  lseek(fd, (sizeof(uint8_t) * MAX_SENSOR_NUM) - 1, SEEK_SET);
  write(fd, "", 1);

  sensorStatus = (uint8_t*) mmap(NULL, (sizeof(uint8_t) * MAX_SENSOR_NUM),
                              PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

  if (sensorStatus == MAP_FAILED){
    syslog(LOG_ERR, "%s(): Error mmapping the file. %s", __func__, strerror(errno));
    flock(fd, LOCK_UN);
    close(fd);
    return;
  }

  switch (thresh) {
  case UNC_THRESH:
    sensorStatus[snr_num] = CLEARBIT(sensorStatus[snr_num], UNC_THRESH);
  case UCR_THRESH:
    sensorStatus[snr_num] = CLEARBIT(sensorStatus[snr_num], UCR_THRESH);
  case UNR_THRESH:
    sensorStatus[snr_num] = CLEARBIT(sensorStatus[snr_num], UNR_THRESH);
    break;

  case LNC_THRESH:
    sensorStatus[snr_num] = CLEARBIT(sensorStatus[snr_num], LNC_THRESH);
  case LCR_THRESH:
    sensorStatus[snr_num] = CLEARBIT(sensorStatus[snr_num], LCR_THRESH);
  case LNR_THRESH:
    sensorStatus[snr_num] = CLEARBIT(sensorStatus[snr_num], LNR_THRESH);
    break;

  default:
    syslog(LOG_ERR, "%s(): wrong threshold value", __func__);
    return;
  }

  for (i = 0; i < MAX_SENSOR_NUM; i++) {
    if (sennum2errcode_mapping[i] == error_code_num) {
      tmp |= sensorStatus[i];
    }
  }

  if (tmp == ERR_DEASSERT){
    pal_write_error_code_file(error_code_num, ERR_DEASSERT);
  }

  munmap(sensorStatus, sizeof(bool) * MAX_SENSOR_NUM);
  flock(fd, LOCK_UN);
  close(fd);
}

void
pal_err_code_enable(const uint8_t error_num) {
  // error code distributed over 0~99
  if (error_num  < (ERROR_CODE_NUM * 8))
    pal_write_error_code_file(error_num, ERR_ASSERT);
  else
    syslog(LOG_WARNING, "%s(): wrong error code number", __func__);
}

void
pal_err_code_disable(const uint8_t error_num) {
  // error code distributed over 0~99
  if (error_num < (ERROR_CODE_NUM * 8))
    pal_write_error_code_file(error_num, ERR_DEASSERT);
  else
    syslog(LOG_WARNING, "%s(): wrong error code number", __func__);
}

int
pal_read_error_code_file(uint8_t *error_code_array) {
  FILE *fp = NULL;
  uint8_t ret = 0;
  int i = 0;
  int tmp = 0;

  if (access(ERR_CODE_FILE, F_OK) == -1) {
    memset(error_code_array, 0, ERROR_CODE_NUM);
    return 0;
  }
  else
    fp = fopen(ERR_CODE_FILE, "r");

  if (!fp)
      return -1;

  ret = pal_flock_retry(fileno(fp));
  if (ret) {
    int err = errno;
    syslog(LOG_WARNING, "%s(): failed to flock on %s, err %d", __func__, ERR_CODE_FILE, err);
    fclose(fp);
    return -1;
  }

  for (i = 0; fscanf(fp, "%X", &tmp) != EOF && i < ERROR_CODE_NUM; i++)
    error_code_array[i] = (uint8_t) tmp;

  flock(fileno(fp), LOCK_UN);
  fclose(fp);
  return 0;
}

int
pal_write_error_code_file(const uint8_t error_num, const bool status) {
  FILE *fp = NULL;
  uint8_t ret = 0;
  int i = 0;
  int stat = 0;
  int bit_stat = 0;
  uint8_t error_code_array[ERROR_CODE_NUM] = {0};

  if (pal_read_error_code_file(error_code_array) != 0){
    syslog(LOG_ERR, "%s(): pal_read_error_code_file() failed", __func__);
    return -1;
  }

  if (access(ERR_CODE_FILE, F_OK) == -1)
    fp = fopen(ERR_CODE_FILE, "w");
  else
    fp = fopen(ERR_CODE_FILE, "r+");
  if (!fp){
    syslog(LOG_ERR, "%s(): open %s failed. %s", __func__, ERR_CODE_FILE, strerror(errno));
    return -1;
  }

  ret = pal_flock_retry(fileno(fp));
  if (ret) {
    syslog(LOG_WARNING, "%s(): failed to flock on %s. %s", __func__, ERR_CODE_FILE, strerror(errno));
    fclose(fp);
    return -1;
  }

  stat = error_num / 8;
  bit_stat = error_num % 8;

  if (status)
    error_code_array[stat] |= 1 << bit_stat;
  else
    error_code_array[stat] &= ~(1 << bit_stat);

  for(i = 0; i < ERROR_CODE_NUM; i++) {
    fprintf(fp, "%X ", error_code_array[i]);
    if(error_code_array[i] != 0) {
      ret = 1;
    }
  }

  fprintf(fp, "\n");
  flock(fileno(fp), LOCK_UN);
  fclose(fp);

  return ret;
}

int
pal_drive_status(const char* i2c_bus) {
  ssd_data ssd;
  t_status_flags status_flag_decoding;
  t_smart_warning smart_warning_decoding;
  t_key_value_pair temp_decoding;
  t_key_value_pair pdlu_decoding;
  t_key_value_pair vendor_decoding;
  t_key_value_pair sn_decoding;

  if (nvme_vendor_read_decode(i2c_bus, &ssd.vendor, &vendor_decoding))
    printf("Fail on reading Vendor ID\n");
  else
    printf("%s: %s\n", vendor_decoding.key, vendor_decoding.value);

  if (nvme_serial_num_read_decode(i2c_bus, ssd.serial_num, MAX_SERIAL_NUM, &sn_decoding))
    printf("Fail on reading Serial Number\n");
  else
    printf("%s: %s\n", sn_decoding.key, sn_decoding.value);

  if (nvme_temp_read_decode(i2c_bus, &ssd.temp, &temp_decoding))
    printf("Fail on reading Composite Temperature\n");
  else
    printf("%s: %s\n", temp_decoding.key, temp_decoding.value);

  if (nvme_pdlu_read_decode(i2c_bus, &ssd.pdlu, &pdlu_decoding))
    printf("Fail on reading Percentage Drive Life Used\n");
  else
    printf("%s: %s\n", pdlu_decoding.key, pdlu_decoding.value);

  if (nvme_sflgs_read_decode(i2c_bus, &ssd.sflgs, &status_flag_decoding))
    printf("Fail on reading Status Flags\n");
  else {
    printf("%s: %s\n", status_flag_decoding.self.key, status_flag_decoding.self.value);
    printf("    %s: %s\n", status_flag_decoding.read_complete.key, status_flag_decoding.read_complete.value);
    printf("    %s: %s\n", status_flag_decoding.ready.key, status_flag_decoding.ready.value);
    printf("    %s: %s\n", status_flag_decoding.functional.key, status_flag_decoding.functional.value);
    printf("    %s: %s\n", status_flag_decoding.reset_required.key, status_flag_decoding.reset_required.value);
    printf("    %s: %s\n", status_flag_decoding.port0_link.key, status_flag_decoding.port0_link.value);
    printf("    %s: %s\n", status_flag_decoding.port1_link.key, status_flag_decoding.port1_link.value);
  }

  if (nvme_smart_warning_read_decode(i2c_bus, &ssd.warning, &smart_warning_decoding))
    printf("Fail on reading SMART Critical Warning\n");
  else {
    printf("%s: %s\n", smart_warning_decoding.self.key, smart_warning_decoding.self.value);
    printf("    %s: %s\n", smart_warning_decoding.spare_space.key, smart_warning_decoding.spare_space.value);
    printf("    %s: %s\n", smart_warning_decoding.temp_warning.key, smart_warning_decoding.temp_warning.value);
    printf("    %s: %s\n", smart_warning_decoding.reliability.key, smart_warning_decoding.reliability.value);
    printf("    %s: %s\n", smart_warning_decoding.media_status.key, smart_warning_decoding.media_status.value);
    printf("    %s: %s\n", smart_warning_decoding.backup_device.key, smart_warning_decoding.backup_device.value);
  }

  printf("\n");
  return 0;
}

int
pal_read_nvme_data(uint8_t slot_num, uint8_t cmd) {
  uint8_t sku;
  int ret;

  ret = lightning_ssd_sku(&sku);

  if (ret < 0) {
    syslog(LOG_DEBUG, "%s(): lightning_ssd_sku failed", __func__);
    return -1;
  }
  if (sku == U2_SKU) {
    ret = pal_u2_flash_read_nvme_data(slot_num, cmd);
    /*Drive status report
    0: drive is not health
    1: drive is health*/
    if (ret == 0)
      return 1;
    else
      return 0;
  }

  else if (sku == M2_SKU) {
    ret = pal_m2_flash_read_nvme_data(slot_num, cmd);
    return ret;
  }

  else {
    syslog(LOG_DEBUG, "%s(): unknown ssd sku", __func__);
    return -1;
  }
}

int
pal_u2_flash_read_nvme_data(uint8_t slot_num, uint8_t cmd) {

  int ret;
  uint8_t mux;
  uint8_t chan;
  char bus[32];

  mux = lightning_flash_list[slot_num] / 10;
  chan = lightning_flash_list[slot_num] % 10;

  /* Set 1-level mux */
  ret = lightning_flash_mux_sel_chan(mux, chan);
  if(ret < 0) {
    syslog(LOG_DEBUG, "%s(): lightning_flash_mux_sel_chan on Mux %d failed", __func__, mux);
    return -1;
  }

  if (mux == I2C_MUX_FLASH1)
    sprintf(bus, "%s", I2C_DEV_FLASH1);
  else if (mux == I2C_MUX_FLASH2)
    sprintf(bus, "%s", I2C_DEV_FLASH2);
  else {
    syslog(LOG_DEBUG, "%s(): unknown mux", __func__);
    return -1;
  }

  if (cmd == CMD_DRIVE_STATUS) {
    printf("Slot%d:\n", slot_num);
    ret = pal_drive_status(bus);
    if(ret < 0) {
      syslog(LOG_DEBUG, "%s(): pal_drive_status failed", __func__);
      return -1;
    }

    ret = pal_drive_health(bus);
    return ret;
  }

  else if (cmd == CMD_DRIVE_HEALTH) {
    ret = pal_drive_health(bus);
    return ret;
  }

  else {
    syslog(LOG_DEBUG, "%s(): unknown cmd", __func__);
    return -1;
  }
}

int
pal_m2_flash_read_nvme_data(uint8_t slot_num, uint8_t cmd) {
  int ret1;
  int ret2;

  /* read the M.2 NVMe-MI data on channel 0 */
  ret1 = pal_m2_read_nvme_data(slot_num, M2_MUX_CHANNEL_0, cmd);
  if (ret1 < 0)
    syslog(LOG_DEBUG, "%s(): pal_m2_read_nvme_data on channel 0 failed", __func__);

  /* read the M.2 NVMe-MI data on channel 1 */
  ret2 = pal_m2_read_nvme_data(slot_num, M2_MUX_CHANNEL_1, cmd);
  if (ret2 < 0)
    syslog(LOG_DEBUG, "%s(): pal_m2_read_nvme_data on channel 1 failed", __func__);

  /*Drive status report
    2: two drives are not health
    3: only first drive is health
    4: only second drive is health
    5: two drives are health*/
  if ((ret1 == 0) && (ret2 == 0))
    return 5;
  else if ((ret1 == 0) && (ret2 != 0))
    return 3;
  else if ((ret1 != 0) && (ret2 == 0))
    return 4;
  else
    return 2;
}

int
pal_m2_read_nvme_data(uint8_t slot_num, uint8_t m2_mux_chan, uint8_t cmd) {

  int ret;
  uint8_t mux;
  uint8_t chan;
  char bus[32];

  mux = lightning_flash_list[slot_num] / 10;
  chan = lightning_flash_list[slot_num] % 10;

  /* Set 1-level mux */
  ret = lightning_flash_mux_sel_chan(mux, chan);
  if(ret < 0) {
    syslog(LOG_DEBUG, "%s(): lightning_flash_mux_sel_chan on Mux %d failed", __func__, mux);
    return -1;
  }

  /* Set 2-level mux */
  ret = lightning_flash_sec_mux_sel_chan(mux, m2_mux_chan);
  if(ret < 0) {
    syslog(LOG_DEBUG, "%s(): lightning_flash_sec_mux_sel_chan on Mux %d failed", __func__, mux);
    return -1;
  }

  if (mux == I2C_MUX_FLASH1)
    sprintf(bus, "%s", I2C_DEV_FLASH1);
  else if (mux == I2C_MUX_FLASH2)
    sprintf(bus, "%s", I2C_DEV_FLASH2);
  else {
    syslog(LOG_DEBUG, "%s(): unknown mux", __func__);
    return -1;
  }

  if (cmd == CMD_DRIVE_STATUS) {
    printf("Slot%d Drive%d\n", slot_num, m2_mux_chan);
    ret = pal_drive_status(bus);
    if(ret < 0) {
      syslog(LOG_DEBUG, "%s(): pal_drive_status failed", __func__);
      return ret;
    }

    ret = pal_drive_health(bus);
    return ret;
  }

  else if (cmd == CMD_DRIVE_HEALTH) {
    ret = pal_drive_health(bus);
    return ret;
  }

  else {
    syslog(LOG_DEBUG, "%s(): unknown cmd", __func__);
    return -1;
  }
}

int
pal_drive_health(const char* dev) {
  uint8_t sflgs;
  uint8_t warning;

  if (nvme_smart_warning_read(dev, &warning))
    return -1;
  else
    if ((warning & NVME_SMART_WARNING_MASK_BIT) != NVME_SMART_WARNING_MASK_BIT)
      return -1;

  if (nvme_sflgs_read(dev, &sflgs))
    return -1;
  else
    if ((sflgs & NVME_SFLGS_MASK_BIT) != NVME_SFLGS_CHECK_VALUE)
      return -1;

  return 0;
}

void
pal_i2c_crash_assert_handle(int i2c_bus_num) {
  // I2C bus number: 0~13
  if (i2c_bus_num < I2C_BUS_MAX_NUMBER)
    pal_err_code_enable(ERR_CODE_I2C_CRASH_BASE + i2c_bus_num);
  else
    syslog(LOG_WARNING, "%s(): wrong I2C bus number", __func__);
}

void
pal_i2c_crash_deassert_handle(int i2c_bus_num) {
  // I2C bus number: 0~13
  if (i2c_bus_num < I2C_BUS_MAX_NUMBER)
    pal_err_code_disable(ERR_CODE_I2C_CRASH_BASE + i2c_bus_num);
  else
    syslog(LOG_WARNING, "%s(): wrong I2C bus number", __func__);
}

int
pal_bmc_err_enable(const char *error_item) {
  if (strcasestr(error_item, "CPU") != 0ULL) {
    pal_err_code_enable(ERR_CODE_CPU);
  } else if (strcasestr(error_item, "Memory") != 0ULL) {
    pal_err_code_enable(ERR_CODE_MEM);
  } else if (strcasestr(error_item, "ECC Unrecoverable") != 0ULL) {
    // Skip reporting ECC error code
  } else if (strcasestr(error_item, "ECC Recoverable") != 0ULL) {
    // Skip reporting ECC error code
  } else {
    syslog(LOG_WARNING, "%s: invalid bmc health item: %s", __func__, error_item);
    return -1;
  }
  return 0;
}

int
pal_bmc_err_disable(const char *error_item) {
  if (strcasestr(error_item, "CPU") != 0ULL) {
    pal_err_code_disable(ERR_CODE_CPU);
  } else if (strcasestr(error_item, "Memory") != 0ULL) {
    pal_err_code_disable(ERR_CODE_MEM);
  } else if (strcasestr(error_item, "ECC Unrecoverable") != 0ULL) {
    // Skip reporting ECC error code
  } else if (strcasestr(error_item, "ECC Recoverable") != 0ULL) {
    // Skip reporting ECC error code
  } else {
    syslog(LOG_WARNING, "%s: invalid bmc health item: %s", __func__, error_item);
    return -1;
  }
  return 0;
}

int
pal_get_boot_option(unsigned char para,unsigned char* pbuff)
{
  unsigned char option_size[]   = {1,1,1,1,2,5,9,17,127};
  unsigned char size = option_size[para];
  memset(pbuff, 0, size);
  return size;
}

int
pal_set_debug_card_led(const int display_num) {
  int val;
  int high_hex = display_num / 16;
  int low_hex = display_num % 16;

  val = low_hex % 2;
  if (pal_set_gpio_value(GPIO_DEBUG_CARD_LOW_HEX_0, val))
    return -1;
  val = (low_hex / 2) % 2;
  if (pal_set_gpio_value(GPIO_DEBUG_CARD_LOW_HEX_1, val))
    return -1;
  val = (low_hex / 4) % 2;
  if (pal_set_gpio_value(GPIO_DEBUG_CARD_LOW_HEX_2, val))
    return -1;
  val = (low_hex / 8) % 2;
  if (pal_set_gpio_value(GPIO_DEBUG_CARD_LOW_HEX_3, val))
    return -1;

  val = high_hex % 2;
  if (pal_set_gpio_value(GPIO_DEBUG_CARD_HIGH_HEX_0, val))
    return -1;
  val = (high_hex / 2) % 2;
  if (pal_set_gpio_value(GPIO_DEBUG_CARD_HIGH_HEX_1, val))
    return -1;
  val = (high_hex / 4) % 2;
  if (pal_set_gpio_value(GPIO_DEBUG_CARD_HIGH_HEX_2, val))
    return -1;
  val = (high_hex / 8) % 2;
  if (pal_set_gpio_value(GPIO_DEBUG_CARD_HIGH_HEX_3, val))
    return -1;

  return 0;
}
