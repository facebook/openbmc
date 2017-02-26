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
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <fcntl.h>
#include <errno.h>
#include <syslog.h>
#include <sys/mman.h>
#include <sys/file.h>
#include <string.h>
#include <pthread.h>
#include <facebook/i2c-dev.h>
#include "pal.h"

#define BIT(value, index) ((value >> index) & 1)

#define LIGHTNING_PLATFORM_NAME "Lightning"
#define LAST_KEY "last_key"
#define LIGHTNING_MAX_NUM_SLOTS 0
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
#define GPIO_RESET_SSD_SWITCH 134

#define GPIO_HB_LED 115
#define GPIO_BMC_SELF_TRAY 108
#define GPIO_BMC_PEER_TRAY 0
#define GPIO_PEER_BMC_HB 117

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

/* NVMe-MI Temperature Definition Code */
#define TEMP_HIGHER_THAN_127 0x7F
#define TEPM_LOWER_THAN_n60 0xC4
#define TEMP_NO_UPDATE 0x80
#define TEMP_SENSOR_FAIL 0x81

/* NVMe-MI SSD Status Flag bit mask */
#define NVME_SFLGS_MASK_BIT 0x2B  //Just check bit 0,1,3,5
#define NVME_SFLGS_CHECK_VALUE 0x0B // normal - bit0,1,3=1, bit5 = 0

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
  0x2C, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, // 0xF0 - 0xFF
};

char * key_list[] = {
"peb_sensor_health",
"pdpb_sensor_health",
"fcb_sensor_health",
"system_identify",
/* Add more Keys here */
LAST_KEY /* This is the last key of the list */
};

char * def_val_list[] = {
  "1", /* peb_sensor_health */
  "1", /* pdbb_sensor_health */
  "1", /* fcb_sensor_health */
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
  *num = LIGHTNING_MAX_NUM_SLOTS;

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

int
pal_is_debug_card_prsnt(uint8_t *status) {
  return 0;
}

int
pal_get_server_power(uint8_t slot_id, uint8_t *status) {

  return 0;
}

// Power Off, Power On, or Power Reset the server in given slot
int
pal_set_server_power(uint8_t slot_id, uint8_t cmd) {

  return 0;
}

int
pal_sled_cycle(void) {

  return 0;
}

// Read the Front Panel Hand Switch and return the position
int
pal_get_hand_sw(uint8_t *pos) {

  return 0;
}

// Return the Front panel Power Button
int
pal_get_pwr_btn(uint8_t *status) {

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

// Update the Reset button input to the server at given slot
int
pal_set_rst_btn(uint8_t slot, uint8_t status) {

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

// Update the Identification LED for the given slot with the status
int
pal_set_id_led(uint8_t slot, uint8_t status) {
  return 0;
}

// Update the USB Mux to the server at given slot
int
pal_switch_usb_mux(uint8_t slot) {

  return 0;
}

// Get the last post code of the given slot
int
pal_post_get_last(uint8_t slot, uint8_t *status) {

  return 0;
}

// Handle the received post code, for now display it on debug card
int
pal_post_handle(uint8_t slot, uint8_t status) {

  return 0;
}


static int
read_kv(char *key, char *value) {

  FILE *fp;
  int rc;

  fp = fopen(key, "r");
  if (!fp) {
    int err = errno;
#ifdef DEBUG
    syslog(LOG_WARNING, "read_kv: failed to open %s", key);
#endif
    return err;
  }

  rc = (int) fread(value, 1, MAX_VALUE_LEN, fp);
  fclose(fp);
  if (rc <= 0) {
#ifdef DEBUG
    syslog(LOG_INFO, "read_kv: failed to read %s", key);
#endif
    return ENOENT;
  } else {
    return 0;
  }
}

static int
write_kv(char *key, char *value) {

  FILE *fp;
  int rc;

  fp = fopen(key, "w");
  if (!fp) {
    int err = errno;
#ifdef DEBUG
    syslog(LOG_WARNING, "write_kv: failed to open %s", key);
#endif
    return err;
  }

  rc = fwrite(value, 1, strlen(value), fp);
  fclose(fp);

  if (rc < 0) {
#ifdef DEBUG
    syslog(LOG_WARNING, "write_kv: failed to write to %s", key);
#endif
    return ENOENT;
  } else {
    return 0;
  }
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
pal_get_fru_sdr_path(uint8_t fru, char *path) {
  return lightning_sensor_sdr_path(fru, path);
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
pal_sensor_read(uint8_t fru, uint8_t sensor_num, void *value) {

  char key[MAX_KEY_LEN] = {0};
  char str[MAX_VALUE_LEN] = {0};
  int ret;

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
  ret = edb_cache_get(key, str);
  if(ret < 0) {
#ifdef DEBUG
    syslog(LOG_WARNING, "pal_sensor_read: cache_get %s failed.", key);
#endif
    return ret;
  }
  if(strcmp(str, "NA") == 0)
    return -1;
  *((float*)value) = atof(str);
  return ret;
}

int
pal_sensor_read_raw(uint8_t fru, uint8_t sensor_num, void *value) {

  char key[MAX_KEY_LEN] = {0};
  char str[MAX_VALUE_LEN] = {0};
  int ret;
  FILE *fp = NULL;
  int tmp = 0;

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
  
  ret = lightning_sensor_read(fru, sensor_num, value);

  if(ret < 0) {
    strcpy(str, "NA");
  }
  else if (ret == 1) { //case: skip monitoring due to enclosure-util running. but not return NA
    return -1;
  }
  else {
    // On successful sensor read
    sprintf(str, "%.2f", *((float*)value));
  }

  if(edb_cache_set(key, str) < 0) {
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

int
pal_fruid_write(uint8_t slot, char *path) {

  return 0;
}

static int
get_key_value(char* key, char *value) {

  char kpath[64] = {0};

  sprintf(kpath, KV_STORE, key);

  if (access(KV_STORE_PATH, F_OK) == -1) {
    mkdir(KV_STORE_PATH, 0777);
  }

  return read_kv(kpath, value);
}

static int
set_key_value(char *key, char *value) {

  char kpath[64] = {0};

  sprintf(kpath, KV_STORE, key);

  if (access(KV_STORE_PATH, F_OK) == -1) {
    mkdir(KV_STORE_PATH, 0777);
  }

  return write_kv(kpath, value);
}

int
pal_set_def_key_value() {

  int ret;
  int i;
  int fru;
  char key[MAX_KEY_LEN] = {0};
  char kpath[MAX_KEY_PATH_LEN] = {0};

  i = 0;
  while(strcmp(key_list[i], LAST_KEY)) {

    memset(key, 0, MAX_KEY_LEN);
    memset(kpath, 0, MAX_KEY_PATH_LEN);

    sprintf(kpath, KV_STORE, key_list[i]);

    if (access(kpath, F_OK) == -1) {

      if ((ret = kv_set(key_list[i], def_val_list[i])) < 0) {
#ifdef DEBUG
          syslog(LOG_WARNING, "pal_set_def_key_value: kv_set failed. %d", ret);
#endif
      }
    }

    i++;
  }

  return 0;
}

static int
pal_key_check(char *key) {
  int ret;
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

  // Check is key is defined and valid
  if (pal_key_check(key))
    return -1;

  return kv_set(key, value);
}

int
pal_get_key_value(char *key, char *value) {

  // Check is key is defined and valid
  if (pal_key_check(key))
    return -1;

  return kv_get(key, value);
}

int
pal_get_fru_devtty(uint8_t fru, char *devtty) {

  return 0;
}

void
pal_dump_key_value(void) {
  int i;
  int ret;

  char value[MAX_VALUE_LEN] = {0x0};

  while (strcmp(key_list[i], LAST_KEY)) {
    printf("%s:", key_list[i]);
    if (ret = get_key_value(key_list[i], value) < 0) {
      printf("\n");
    } else {
      printf("%s\n",  value);
    }
    i++;
    memset(value, 0, MAX_VALUE_LEN);
  }
}

int
pal_set_last_pwr_state(uint8_t fru, char *state) {

  return 0;
}

int
pal_get_last_pwr_state(uint8_t fru, char *state) {

  return 0;
}

int
pal_get_sys_guid(uint8_t slot, char *guid) {
  int ret;

  return 0;
}

int
pal_set_sysfw_ver(uint8_t slot, uint8_t *ver) {

  return 0;
}

int
pal_get_sysfw_ver(uint8_t slot, uint8_t *ver) {

  return 0;
}

int
pal_is_bmc_por(void) {

  return 0;
}

int
pal_get_fru_discrete_list(uint8_t fru, uint8_t **sensor_list, int *cnt) {

  return 0;
}

int
pal_sensor_discrete_check(uint8_t fru, uint8_t snr_num, char *snr_name,
    uint8_t o_val, uint8_t n_val) {

  return 0;
}
int
pal_sel_handler(uint8_t fru, uint8_t snr_num, uint8_t *event_data) {

  return 0;
}

int
pal_get_event_sensor_name(uint8_t fru, uint8_t snr_num, char *name) {

  return 0;
}

int
pal_parse_sel(uint8_t fru, uint8_t snr_num, uint8_t *event_data, char *error_log) {

  return 0;
}

// Helper function for msleep
void
msleep(int msec) {
  struct timespec req;

  req.tv_sec = 0;
  req.tv_nsec = msec * 1000 * 1000;

  while(nanosleep(&req, &req) == -1 && errno == EINTR) {
    continue;
  }
}

int
pal_set_sensor_health(uint8_t fru, uint8_t value) {

  char key[MAX_KEY_LEN] = {0};
  char cvalue[MAX_VALUE_LEN] = {0};

  switch(fru) {
    case FRU_PEB:
      sprintf(key, "peb_sensor_health");
      break;
    case FRU_PDPB:
      sprintf(key, "pdpb_sensor_health");
      break;
    case FRU_FCB:
      sprintf(key, "fcb_sensor_health");
      break;

    default:
      return -1;
  }

  sprintf(cvalue, (value > 0) ? "1": "0");

  return pal_set_key_value(key, cvalue);

  return 0;
}

int
pal_get_fru_health(uint8_t fru, uint8_t *value) {

  char cvalue[MAX_VALUE_LEN] = {0};
  char key[MAX_KEY_LEN] = {0};
  int ret;

  switch(fru) {
    case FRU_PEB:
      sprintf(key, "peb_sensor_health");
      break;
    case FRU_PDPB:
      sprintf(key, "pdpb_sensor_health");
      break;
    case FRU_FCB:
      sprintf(key, "fcb_sensor_health");
      break;

    default:
      return -1;
  }

  ret = pal_get_key_value(key, cvalue);
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
pal_sensor_threshold_flag(uint8_t fru, uint8_t snr_num, uint16_t *flag) {

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
  int ret;

  if (fan >= pal_pwm_cnt) {
    syslog(LOG_INFO, "pal_set_fan_speed: fan number is invalid - %d", fan);
    return -1;
  }

  // Convert the percentage to our 1/96th unit.
  unit = pwm * PWM_UNIT_MAX / 100;

  // For 0%, turn off the PWM entirely
  if (unit == 0) {
    write_fan_value(fan, "pwm%d_en", 0);
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

  ret = pal_sensor_read(FRU_FCB, FCB_SENSOR_FAN1_FRONT_SPEED + fan, &value);
  if (ret == 0)
    *rpm = (int) value;

  return ret;
}

void
pal_inform_bic_mode(uint8_t fru, uint8_t mode) {
}

void
pal_update_ts_sled() {
}

int
pal_handle_dcmi(uint8_t fru, uint8_t *tbuf, uint8_t tlen, uint8_t *rbuf, uint8_t *rlen) {
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
  ret = pal_set_fan_led( (fan_num / 2), FAN_LED_OFF);

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
  ret = pal_set_fan_led( (fan_num / 2), FAN_LED_ON);

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
pal_peer_tray_detection(uint8_t *value) {

  char path[64] = {0};
  int val;

  sprintf(path, GPIO_VAL, GPIO_PEER_BMC_HB);
  if (read_device(path, &val))
    return -1;

  *value = (uint8_t) val;

  return 0;
}

int
pal_self_tray_location(uint8_t *value) {

  char path[64] = {0};
  int val;

  sprintf(path, GPIO_VAL, GPIO_BMC_SELF_TRAY);
  if (read_device(path, &val))
    return -1;

  *value = (uint8_t) val;

  return 0;
}

int
pal_peer_tray_location(uint8_t *value) {

  char path[64] = {0};
  int val;

  sprintf(path, GPIO_VAL, GPIO_BMC_PEER_TRAY);
  if (read_device(path, &val))
    return -1;

  *value = (uint8_t) val;

  return 0;
}

int
pal_is_crashdump_ongoing(uint8_t slot)
{
  return 0;
}

// Reset SSD Switch
int
pal_reset_ssd_switch() {

  char path[64] = {0};
  sprintf(path, GPIO_VAL, GPIO_RESET_SSD_SWITCH);

  if (write_device(path, "0"))
    return -1;

  msleep(100);

  if (write_device(path, "1"))
    return -1;

  msleep(100);

  return 0;
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
  }
}

//For Merge Yosemite and TP
int
pal_get_platform_id(uint8_t *id) {
  // dummy definition
  return 0;
}
int
pal_get_board_rev_id(uint8_t *id) {
  // dummy definition
  return 0;
}
int
pal_get_mb_slot_id(uint8_t *id) {
  // dummy definition
  return 0;
}
int
pal_get_slot_cfg_id(uint8_t *id) {
  // dummy definition
  return 0;
}
int
pal_get_boot_order(uint8_t fru, uint8_t *boot) {
  // dummy definition
  return 0;
}
int
pal_set_boot_order(uint8_t fru, uint8_t *boot) {
  // dummy definition
  return 0;
}

int
pal_get_dev_guid(uint8_t fru, char *guid) {
  // dummy definition
  return 0;
}

int pal_get_poss_pcie_config(uint8_t *pcie_config){
   // dummy definition
   return 0;
}

int pal_get_plat_sku_id(void){
   // dummy definition
   return 0;
}

void
pal_sensor_assert_handle(uint8_t snr_num, float val) {
  return;
}
void
pal_sensor_deassert_handle(uint8_t snr_num, float val) {
  return;
}

void
pal_set_post_end(void) {
  return;
}

void
pal_post_end_chk(uint8_t *post_end_chk) {
  return;
}

int
pal_get_fw_info(unsigned char target, unsigned char* res,
    unsigned char* res_len) {

    return 0;
}

void
pal_err_code_enable(error_code *update) {
  // error code distributed over 0~99
  if (update->code < (ERROR_CODE_NUM * 8)) {
    update->status = 1;
    pal_write_error_code_file(update);
  }
  else
    syslog(LOG_WARNING, "%s(): wrong error code number", __func__); 
}

void
pal_err_code_disable(error_code *update) {
  // error code distributed over 0~99
  if (update->code < (ERROR_CODE_NUM * 8)) {
    update->status = 0;
    pal_write_error_code_file(update);
  }
  else
    syslog(LOG_WARNING, "%s(): wrong error code number", __func__);
}

/* init error code status about sensors*/
void
pal_error_code_array_init(const int count, const uint8_t *list, error_code *updateArray) {
  int i;
  updateArray[count].code = 0xFE;
  for (i = 0; i < count; i++) {
    updateArray[i].code = list[i];
    updateArray[i].status = 0;
  }
}

void
pal_err_code_enable_by_sensor_num(uint8_t snr_num, error_code *updateArray) {
  uint8_t error_num = sennum2errcode_mapping[snr_num];
  int i, j, k;
  error_code errorCode = {error_num, 1};

  for (i = 0; i < MAX_SENSOR_NUM; i++) {
    if (sennum2errcode_mapping[i] == error_num) {
      for (j = 0; updateArray[j].code != 0xFE; j++) {
        if (updateArray[j].code == snr_num)
          k = j;
        else if (updateArray[j].code == sennum2errcode_mapping[i])
          errorCode.status |= updateArray[j].status;
      }
    }
  }

  updateArray[k].status = 1;

  if (errorCode.status == updateArray[k].status)
    pal_write_error_code_file(&errorCode);
}

void
pal_err_code_disable_by_sensor_num(uint8_t snr_num, error_code *updateArray) {
  uint8_t error_num = sennum2errcode_mapping[snr_num];
  int i, j, k;
  error_code errorCode = {error_num, 0};

  for (i = 0; i < MAX_SENSOR_NUM; i++) {
    if (sennum2errcode_mapping[i] == error_num) {
      for (j = 0; updateArray[j].code != 0xFE; j++) {
        if (updateArray[j].code == snr_num)
          k = j;
        else if (updateArray[j].code == sennum2errcode_mapping[i])
          errorCode.status |= updateArray[j].status;
      }
    }
  }

  updateArray[k].status = 0;

  if (errorCode.status == updateArray[k].status)
    pal_write_error_code_file(&errorCode);
}

uint8_t
pal_read_error_code_file(uint8_t *error_code_array) {
  FILE *fp = NULL;
  uint8_t ret = 0;
  int i = 0;
  int tmp = 0;
  int retry_count = 0;

  if (access(ERR_CODE_FILE, F_OK) == -1) {
    for (i = 0; i < ERROR_CODE_NUM; i++)
      error_code_array[i] = 0;
    return 0;
  }
  else
    fp = fopen(ERR_CODE_FILE, "r");
  if (!fp)
      return -1;

  ret = flock(fileno(fp), LOCK_EX | LOCK_NB);
  while (ret && (retry_count < 3)) {
    retry_count++;
    msleep(100);
    ret = flock(fileno(fp), LOCK_EX | LOCK_NB);
  }
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

uint8_t
pal_write_error_code_file(error_code *update) {
  FILE *fp = NULL;
  uint8_t ret = 0;
  int retry_count = 0;
  int i = 0;
  int stat = 0;
  int bit_stat = 0;
  uint8_t error_code_array[ERROR_CODE_NUM] = {0};

  pal_read_error_code_file(error_code_array);

  if (access(ERR_CODE_FILE, F_OK) == -1)
    fp = fopen(ERR_CODE_FILE, "w");
  else
    fp = fopen(ERR_CODE_FILE, "r+");
  if (!fp)
      return -1;

  ret = flock(fileno(fp), LOCK_EX | LOCK_NB);
  while (ret && (retry_count < 3)) {
    retry_count++;
    msleep(100);
    ret = flock(fileno(fp), LOCK_EX | LOCK_NB);
  }
  if (ret) {
    int err = errno;
    syslog(LOG_WARNING, "%s(): failed to flock on %s, err %d", __func__, ERR_CODE_FILE, err);
    fclose(fp);
    return -1;
  }

  stat = update->code / 8;
  bit_stat = update->code % 8;

  if (update->status)
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
pal_drive_status(const char* dev) {
  ssd_data ssd;
  int i;
  char tmp[MAX_SERIAL_NUM + 1];

  if (nvme_serial_num_read(dev, ssd.serial_num, MAX_SERIAL_NUM))
    printf("Fail on reading Serial Number\n");
  else{
    memcpy(tmp, ssd.serial_num, MAX_SERIAL_NUM);
    tmp[MAX_SERIAL_NUM] = '\0';
    printf("Serial Number: %s\n", tmp); 
  }

  if (nvme_temp_read(dev, &ssd.temp))
    printf("Fail on reading Composite Temperature\n");
  else {
    if (ssd.temp <= TEMP_HIGHER_THAN_127)
      printf("Composite Temperature: %d C\n", ssd.temp);
    else if (ssd.temp >= TEPM_LOWER_THAN_n60)
      printf("Composite Temperature: %d C\n", ssd.temp - 0x100);
    else if (ssd.temp == TEMP_NO_UPDATE)
      printf("Composite Temperature: No data or data is too old\n");
    else if (ssd.temp == TEMP_SENSOR_FAIL)
      printf("Composite Temperature: Sensor failure\n");
  }

  if (nvme_pdlu_read(dev, &ssd.pdlu))
    printf("Fail on reading Percentage Drive Life Used\n");
  else
    printf("Percentage Drive Life Used: %d\n", ssd.pdlu);

  if (nvme_sflgs_read(dev, &ssd.sflgs))
    printf("Fail on reading Status Flags\n");
  else {
    printf("Status Flags: 0x%2X\n", ssd.sflgs);
    if ((ssd.sflgs & 0x80) == 0)
      printf("    SMBUS block read complete: FAIL\n");
    else
      printf("    SMBUS block read complete: OK\n");

    if ((ssd.sflgs & 0x40) == 0)
      printf("    Drive Ready: Ready\n");
    else
      printf("    Drive Ready: Not ready\n");

    if ((ssd.sflgs & 0x20) == 0)
      printf("    Drive Functional: Unrecoverable Failure\n");
    else
      printf("    Drive Functional: Functional\n");

    if ((ssd.sflgs & 0x10) == 0)
      printf("    Reset Required: Required\n");
    else
      printf("    Reset Required: No\n");

    if ((ssd.sflgs & 0x08) == 0)
      printf("    Port 0 PCIe Link Active: Down\n");
    else
      printf("    Port 0 PCIe Link Active: Up\n");

    if ((ssd.sflgs & 0x04) == 0)
      printf("    Port 1 PCIe Link Active: Down\n");
    else
      printf("    Port 1 PCIe Link Active: Up\n");
  }

  if (nvme_smart_warning_read(dev, &ssd.warning))
    printf("Fail on reading SMART Critical Warning\n");
  else {
    printf("SMART Critical Warning: 0x%2X\n", ssd.warning);
    if ((ssd.warning & 0x01) == 0)
      printf("    Spare Space: Low\n");
    else
      printf("    Spare Space: Normal\n");

    if ((ssd.warning & 0x02) == 0)
      printf("    Temperature Warning: Abnormal\n");
    else
      printf("    Temperature Warning: Normal\n");

    if ((ssd.warning & 0x04) == 0)
      printf("    NVM Subsystem Reliability: Degraded\n");
    else
      printf("    NVM Subsystem Reliability: Normal\n");

    if ((ssd.warning & 0x08) == 0)
      printf("    Media Status: Read Only mode\n");
    else
      printf("    Media Status: Normal\n");

    if ((ssd.warning & 0x10) == 0)
      printf("    Volatile Memory Backup Device: Failed\n");
    else
      printf("    Volatile Memory Backup Device: Normal\n");
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
    if (ret == 1)
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
  uint8_t vendor;
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
  }

  else if (cmd == CMD_DRIVE_HEALTH) {
    ret = pal_drive_health(bus);
    return ret;
  }

  else {
    syslog(LOG_DEBUG, "%s(): unknown cmd", __func__);
    return -1;
  }

  if(ret < 0) {
    syslog(LOG_DEBUG, "%s(): pal_drive_status failed", __func__);
    return -1;
  }

  return 1;
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
  }

  else if (cmd == CMD_DRIVE_HEALTH) {
    ret = pal_drive_health(bus);
    return ret;
  }

  else {
    syslog(LOG_DEBUG, "%s(): unknown cmd", __func__);
    return -1;
  }
  
  if(ret < 0) {
    syslog(LOG_DEBUG, "%s(): pal_drive_status failed", __func__);
    return -1;
  }

  return 0;
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
