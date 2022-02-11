/*
 *
 * Copyright 2019-present Facebook. All Rights Reserved.
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

 /*
  * This file contains functions and logics that depends on Wedge100 specific
  * hardware and kernel drivers. Here, some of the empty "default" functions
  * are overridden with simple functions that returns non-zero error code.
  * This is for preventing any potential escape of failures through the
  * default functions that will return 0 no matter what.
  */

// #define DEBUG
#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>
#include <pthread.h>
#include <math.h>
#include <openbmc/log.h>
#include <openbmc/libgpio.h>
#include <openbmc/kv.h>
#include <openbmc/obmc-i2c.h>
#include <openbmc/sensor-correction.h>
#include <openbmc/misc-utils.h>
#include <facebook/wedge_eeprom.h>
#include "pal_sensors.h"
#include "pal.h"

#define DELTA_SKIP 0

#define GUID_SIZE 16
#define OFFSET_DEV_GUID 0x1800

#define GPIO_VAL "/sys/class/gpio/gpio%d/value"
#define SCM_BRD_ID "6-0021"
#define SENSOR_NAME_ERR "---- It should not be show ----"
#define LED_INTERVAL 5
uint8_t g_dev_guid[GUID_SIZE] = {0};

typedef struct {
  char name[32];
} sensor_desc_t;

struct threadinfo {
  uint8_t is_running;
  uint8_t fru;
  pthread_t pt;
};

static struct threadinfo t_dump[MAX_NUM_FRUS] = {0, };

/* List of SCM sensors to be monitored */
const uint8_t scm_sensor_list[] = {
  SCM_SENSOR_OUTLET_TEMP,
  SCM_SENSOR_INLET_TEMP,
  SCM_SENSOR_HSC_VOLT,
  SCM_SENSOR_HSC_CURR,
};

/* List of SCM and BIC sensors to be monitored */
const uint8_t scm_all_sensor_list[] = {
  SCM_SENSOR_OUTLET_TEMP,
  SCM_SENSOR_INLET_TEMP,
  SCM_SENSOR_HSC_VOLT,
  SCM_SENSOR_HSC_CURR
};

/* List of SMB sensors to be monitored */
const uint8_t smb_sensor_list[] = {
  /* Thermal Sensors */
  SMB_SENSOR_TMP75_LF_TEMP,
  SMB_SENSOR_TMP75_RF_TEMP,
  SMB_SENSOR_TMP75_UPPER_MAC_TEMP,
  SMB_SENSOR_TMP75_LOWER_MAC_TEMP,
  /* Sensors FAN Speed */
  SMB_SENSOR_FAN1_FRONT_TACH,
  SMB_SENSOR_FAN1_REAR_TACH,
  SMB_SENSOR_FAN2_FRONT_TACH,
  SMB_SENSOR_FAN2_REAR_TACH,
  SMB_SENSOR_FAN3_FRONT_TACH,
  SMB_SENSOR_FAN3_REAR_TACH,
  SMB_SENSOR_FAN4_FRONT_TACH,
  SMB_SENSOR_FAN4_REAR_TACH,
  SMB_SENSOR_FAN5_FRONT_TACH,
  SMB_SENSOR_FAN5_REAR_TACH,
  SMB_SENSOR_FAN6_FRONT_TACH,
  SMB_SENSOR_FAN6_REAR_TACH,
  /* BMC ADC Sensors  */
  SMB_BMC_ADC0_VSEN,
  SMB_BMC_ADC1_VSEN,
  SMB_BMC_ADC2_VSEN,
  SMB_BMC_ADC3_VSEN,
  SMB_BMC_ADC5_VSEN,
  SMB_BMC_ADC6_VSEN,
  SMB_BMC_ADC7_VSEN,
  SMB_BMC_ADC8_VSEN,
  SMB_BMC_ADC9_VSEN,
  SMB_BMC_ADC13_VSEN,
  SMB_BMC_ADC14_VSEN,
  SMB_BMC_ADC15_VSEN,
};

const uint8_t psu1_sensor_list[] = {
  PSU1_SENSOR_IN_VOLT,
  PSU1_SENSOR_OUT_VOLT,
  PSU1_SENSOR_IN_CURR,
  PSU1_SENSOR_OUT_CURR,
  PSU1_SENSOR_IN_POWER,
  PSU1_SENSOR_OUT_POWER,
  PSU1_SENSOR_FAN_TACH,
  PSU1_SENSOR_TEMP1,
  PSU1_SENSOR_TEMP2,
};

const uint8_t psu2_sensor_list[] = {
  PSU2_SENSOR_IN_VOLT,
  PSU2_SENSOR_OUT_VOLT,
  PSU2_SENSOR_IN_CURR,
  PSU2_SENSOR_OUT_CURR,
  PSU2_SENSOR_IN_POWER,
  PSU2_SENSOR_OUT_POWER,
  PSU2_SENSOR_FAN_TACH,
  PSU2_SENSOR_TEMP1,
  PSU2_SENSOR_TEMP2,
};


float scm_sensor_threshold[MAX_SENSOR_NUM][MAX_SENSOR_THRESHOLD + 1] = {0};
float smb_sensor_threshold[MAX_SENSOR_NUM][MAX_SENSOR_THRESHOLD + 1] = {0};
float psu_sensor_threshold[MAX_SENSOR_NUM][MAX_SENSOR_THRESHOLD + 1] = {0};

size_t scm_sensor_cnt = sizeof(scm_sensor_list)/sizeof(uint8_t);
size_t scm_all_sensor_cnt = sizeof(scm_all_sensor_list)/sizeof(uint8_t);
size_t smb_sensor_cnt = sizeof(smb_sensor_list)/sizeof(uint8_t);
size_t psu1_sensor_cnt = sizeof(psu1_sensor_list)/sizeof(uint8_t);
size_t psu2_sensor_cnt = sizeof(psu2_sensor_list)/sizeof(uint8_t);
size_t pal_pwm_cnt = 12;
size_t pal_tach_cnt = 12;

const char pal_fru_list[] = "all, smb, \
psu1, psu2, fan1, fan2, fan3, fan4, fan5, fan6";

char * key_list[] = {
  "pwr_server_last_state",
  "sysfw_ver_server",
  "timestamp_sled",
  "server_por_cfg",
  "server_sel_error",
  "scm_sensor_health",
  "smb_sensor_health",
  "fcm_sensor_health",
  "psu1_sensor_health",
  "psu2_sensor_health",
  "fan1_sensor_health",
  "fan2_sensor_health",
  "fan3_sensor_health",
  "fan4_sensor_health",
  "fan5_sensor_health",
  "fan6_sensor_health",
  "slot1_boot_order",
  /* Add more Keys here */
  LAST_KEY /* This is the last key of the list */
};

char * def_val_list[] = {
  "on", /* pwr_server_last_state */
  "0", /* sysfw_ver_server */
  "0", /* timestamp_sled */
  "lps", /* server_por_cfg */
  "1", /* server_sel_error */
  "1", /* scm_sensor_health */
  "1", /* smb_sensor_health */
  "1", /* fcm_sensor_health */
  "1", /* psu1_sensor_health */
  "1", /* psu2_sensor_health */
  "1", /* fan1_sensor_health */
  "1", /* fan2_sensor_health */
  "1", /* fan3_sensor_health */
  "1", /* fan4_sensor_health */
  "1", /* fan5_sensor_health */
  "1", /* fan6_sensor_health */
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
    OBMC_INFO("failed to open device %s", device);
#endif
    return err;
  }

  rc = fscanf(fp, "%i", value);
  fclose(fp);
  if (rc != 1) {
#ifdef DEBUG
    OBMC_INFO("failed to read device %s", device);
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
    OBMC_INFO("failed to open device for write %s", device);
#endif
    return err;
  }

  rc = fputs(value, fp);
  fclose(fp);

  if (rc < 0) {
#ifdef DEBUG
    OBMC_INFO("failed to write device %s", device);
#endif
    return ENOENT;
  } else {
    return 0;
  }
}

int
pal_detect_i2c_device(uint8_t bus, uint8_t addr) {

  int fd = -1, rc = -1;
  char fn[32];

  snprintf(fn, sizeof(fn), "/dev/i2c-%d", bus);
  fd = open(fn, O_RDWR);
  if (fd == -1) {
    OBMC_WARN("Failed to open i2c device %s", fn);
    return -1;
  }

  rc = ioctl(fd, I2C_SLAVE_FORCE, addr);
  if (rc < 0) {
    OBMC_WARN("Failed to open slave @ address 0x%x", addr);
    close(fd);
    return -1;
  }

  rc = i2c_smbus_read_byte(fd);
  close(fd);

  if (rc < 0) {
    return -1;
  } else {
    return 0;
  }
}

int
pal_add_i2c_device(uint8_t bus, uint8_t addr, char *device_name) {

  int ret = -1;
  char cmd[64];

  snprintf(cmd, sizeof(cmd),
            "echo %s %d > /sys/bus/i2c/devices/i2c-%d/new_device",
              device_name, addr, bus);

#ifdef DEBUG
  OBMC_WARN("[%s] Cmd: %s", __func__, cmd);
#endif

  ret = run_command(cmd);

  return ret;
}

int
pal_del_i2c_device(uint8_t bus, uint8_t addr) {

  int ret = -1;
  char cmd[64];

  sprintf(cmd, "echo %d > /sys/bus/i2c/devices/i2c-%d/delete_device",
           addr, bus);

#ifdef DEBUG
  OBMC_WARN("[%s] Cmd: %s", __func__, cmd);
#endif

  ret = run_command(cmd);

  return ret;
}
//For OEM command "CMD_OEM_GET_PLAT_INFO" 0x7e

// SW103_TODO
// LE_DEBUG: oem_get_plat_info
// For OEM command "CMD_OEM_GET_PLAT_INFO" 0x7e
int
pal_get_plat_sku_id(void){
  int val;
  char path[LARGEST_DEVICE_NAME + 1];
  snprintf(path, LARGEST_DEVICE_NAME, SWPLD1_SYSFS, SWBD_ID);
  if (read_device(path, &val)) {
    return -1;
  }
  return val; // AGC032A
}


//Use part of the function for OEM Command "CMD_OEM_GET_POSS_PCIE_CONFIG" 0xF4
int
pal_get_poss_pcie_config(uint8_t slot, uint8_t *req_data, uint8_t req_len,
                         uint8_t *res_data, uint8_t *res_len) {

  uint8_t completion_code = CC_SUCCESS;
  uint8_t pcie_conf = 0x02; // Wedge400/agc032a-C
  uint8_t *data = res_data;
  *data++ = pcie_conf;
  *res_len = data - res_data;
  return completion_code;
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
  OBMC_WARN("pal_key_check: invalid key - %s", key);
#endif
  return -1;
}

int
pal_get_key_value(char *key, char *value) {
  int ret;
  // Check is key is defined and valid
  if (pal_key_check(key))
    return -1;

  ret = kv_get(key, value, NULL, KV_FPERSIST);
  return ret;
}

int
pal_set_key_value(char *key, char *value) {

  // Check is key is defined and valid
  if (pal_key_check(key))
    return -1;

  return kv_set(key, value, 0, KV_FPERSIST);
}

int
pal_set_sysfw_ver(uint8_t slot, uint8_t *ver) {
  int i;
  char key[MAX_KEY_LEN];
  char str[MAX_VALUE_LEN] = {0};
  char tstr[10];
  sprintf(key, "%s", "sysfw_ver_server");

  for (i = 0; i < SIZE_SYSFW_VER; i++) {
    sprintf(tstr, "%02x", ver[i]);
    strcat(str, tstr);
  }

  return pal_set_key_value(key, str);
}

int
pal_get_sysfw_ver(uint8_t slot, uint8_t *ver) {
  int i;
  int j = 0;
  int ret;
  int msb, lsb;
  char key[MAX_KEY_LEN];
  char str[MAX_VALUE_LEN] = {0};
  char tstr[4];

  sprintf(key, "%s", "sysfw_ver_server");

  ret = pal_get_key_value(key, str);
  if (ret) {
    return ret;
  }

  for (i = 0; i < 2*SIZE_SYSFW_VER; i += 2) {
    sprintf(tstr, "%c\n", str[i]);
    msb = strtol(tstr, NULL, 16);

    sprintf(tstr, "%c\n", str[i+1]);
    lsb = strtol(tstr, NULL, 16);
    ver[j++] = (msb << 4) | lsb;
  }

  return 0;
}

int
pal_get_fru_list(char *list) {
  strcpy(list, pal_fru_list);
  return 0;
}

int
pal_get_fru_id(char *str, uint8_t *fru) {
  if (!strcmp(str, "all")) {
    *fru = FRU_ALL;
  } else if (!strcmp(str, "smb")) {
    *fru = FRU_SMB;
  } else if (!strcmp(str, "psu1")) {
    *fru = FRU_PSU1;
  } else if (!strcmp(str, "psu2")) {
    *fru = FRU_PSU2;
  } else if (!strcmp(str, "fan1")) {
    *fru = FRU_FAN1;
  } else if (!strcmp(str, "fan2")) {
    *fru = FRU_FAN2;
  } else if (!strcmp(str, "fan3")) {
    *fru = FRU_FAN3;
  } else if (!strcmp(str, "fan4")) {
    *fru = FRU_FAN4;
  } else if (!strcmp(str, "fan5")) {
    *fru = FRU_FAN5;
  } else if (!strcmp(str, "fan6")) {
    *fru = FRU_FAN6;
  } else if (!strcmp(str, "bmc")) {
    *fru = FRU_BMC;
  } else if (!strcmp(str, "cpld")) {
    *fru = FRU_CPLD;
  } else if (!strcmp(str, "fpga")) {
    *fru = FRU_FPGA;
  } else {
    OBMC_WARN("pal_get_fru_id: Wrong fru#%s", str);
    return -1;
  }

  return 0;
}

int
pal_get_fru_name(uint8_t fru, char *name) {
  switch(fru) {
    case FRU_SMB:
      strcpy(name, "smb");
      break;
#if DETLA_SKIP
    case FRU_SCM:
      strcpy(name, "scm");
      break;
#endif
    case FRU_FCM:
      strcpy(name, "fcm");
      break;
    case FRU_PSU1:
      strcpy(name, "psu1");
      break;
    case FRU_PSU2:
      strcpy(name, "psu2");
      break;
    case FRU_FAN1:
      strcpy(name, "fan1");
      break;
    case FRU_FAN2:
      strcpy(name, "fan2");
      break;
    case FRU_FAN3:
      strcpy(name, "fan3");
      break;
    case FRU_FAN4:
      strcpy(name, "fan4");
      break;
    case FRU_FAN5:
      strcpy(name, "fan5");
      break;
    case FRU_FAN6:
      strcpy(name, "fan6");
      break;
    default:
      if (fru > MAX_NUM_FRUS)
        return -1;
      sprintf(name, "fru%d", fru);
      break;
  }
  return 0;
}

int
pal_get_fruid_path(uint8_t fru, char *path) {
  char fname[16] = {0};
  int ret = 0;

  switch(fru) {
  case FRU_SMB:
    strncpy(fname, "smb", strlen("smb") + 1);
    break;
  case FRU_PSU1:
    strncpy(fname, "psu1", strlen("psu1") + 1);
    break;
  case FRU_PSU2:
    strncpy(fname, "psu2", strlen("psu2") + 1);
    break;
  case FRU_FAN1:
    strncpy(fname, "fan1", strlen("fan1") + 1);
    break;
  case FRU_FAN2:
    strncpy(fname, "fan2", strlen("fan2") + 1);
    break;
  case FRU_FAN3:
    strncpy(fname, "fan3", strlen("fan3") + 1);
    break;
  case FRU_FAN4:
    strncpy(fname, "fan4", strlen("fan4") + 1);
    break;
  case FRU_FAN5:
    strncpy(fname, "fan5", strlen("fan5") + 1);
    break;
  case FRU_FAN6:
    strncpy(fname, "fan6", strlen("fan6") + 1);
    break;
  default:
    OBMC_ERROR(-1, "%s() unknown fruid %d", __func__, fru);
    ret = PAL_ENOTSUP;
  }

  if ( ret != PAL_ENOTSUP ) {
    snprintf(path, LARGEST_DEVICE_NAME,"/tmp/fruid_%s.bin", fname);
  }

  return ret;
}

int
pal_get_fruid_name(uint8_t fru, char *name) {
  return pal_get_fru_name(fru, name);
}

// Platform Abstraction Layer (PAL) Functions
int
pal_get_platform_name(char *name) {
  strcpy(name, PLATFORM_NAME);

  return 0;
}

int
pal_is_fru_prsnt(uint8_t fru, uint8_t *status) {
  int val = 0;
  char tmp[LARGEST_DEVICE_NAME];
  char path[LARGEST_DEVICE_NAME + 1];
  *status = 0;

  switch (fru) {
    case FRU_SMB:
      *status = 1;
      return 0;
    case FRU_PSU1:
      snprintf(tmp, LARGEST_DEVICE_NAME, SWPLD1_SYSFS, PSU_PRSNT_STATUS);
      snprintf(path, LARGEST_DEVICE_NAME, tmp, 1);
      break;
    case FRU_PSU2:
      snprintf(tmp, LARGEST_DEVICE_NAME, SWPLD1_SYSFS, PSU_PRSNT_STATUS);
      snprintf(path, LARGEST_DEVICE_NAME, tmp, 2);
      break;
    case FRU_FAN1:
    case FRU_FAN2:
    case FRU_FAN3:
    case FRU_FAN4:
    case FRU_FAN5:
    case FRU_FAN6:
      snprintf(path, LARGEST_DEVICE_NAME, GPIO_VAL, fru - FRU_FAN1 + FAN_PRSNT_GPIO_START_POS);
      break;
    default:
      printf("unsupported fru id %d\n", fru);
      return -1;
    }

    if (read_device(path, &val)) {
      return -1;
    }

    if (val == 0x0) {
      *status = 1;
    } else {
      *status = 0;
      return 0;
    }

    return 0;
}

static int check_dir_exist(const char *device);

int
pal_is_fru_ready(uint8_t fru, uint8_t *status) {
  int ret = 0;

  switch(fru) {
    case FRU_PSU1:
      if(!check_dir_exist(PSU1_DEVICE)) {
        *status = 1;
      }
      break;
    case FRU_PSU2:
      if(!check_dir_exist(PSU2_DEVICE)) {
        *status = 1;
      }
      break;
    default:
      *status = 1;

      break;
  }

  return ret;
}

int
pal_get_sensor_util_timeout(uint8_t fru) {
  uint8_t brd_type;
  uint8_t brd_type_rev;
  size_t cnt = 0;
  pal_get_board_type(&brd_type);
  pal_get_board_type_rev(&brd_type_rev);
  switch(fru) {
    case FRU_SMB:
      cnt = smb_sensor_cnt;
      break;
    case FRU_PSU1:
      cnt = psu1_sensor_cnt;
      break;
    case FRU_PSU2:
      cnt = psu2_sensor_cnt;
      break;
    default:
      if (fru > MAX_NUM_FRUS)
      cnt = 5;
      break;
  }
  return (READ_UNIT_SENSOR_TIMEOUT * cnt);
}

int
pal_get_fru_sensor_list(uint8_t fru, uint8_t **sensor_list, int *cnt) {
  uint8_t brd_type;
  uint8_t brd_type_rev;
  pal_get_board_type(&brd_type);
  pal_get_board_type_rev(&brd_type_rev);
  switch(fru) {
  case FRU_SMB:
    *sensor_list = (uint8_t *) smb_sensor_list;
    *cnt = smb_sensor_cnt;
    break;
  case FRU_PSU1:
    *sensor_list = (uint8_t *) psu1_sensor_list;
    *cnt = psu1_sensor_cnt;
    break;
  case FRU_PSU2:
    *sensor_list = (uint8_t *) psu2_sensor_list;
    *cnt = psu2_sensor_cnt;
    break;
  default:
    if (fru > MAX_NUM_FRUS)
      return -1;
    // Nothing to read yet.
    *sensor_list = NULL;
    *cnt = 0;
  }
  return 0;
}

void
pal_update_ts_sled()
{
  char key[MAX_KEY_LEN];
  char tstr[MAX_VALUE_LEN] = {0};
  struct timespec ts;

  clock_gettime(CLOCK_REALTIME, &ts);
  sprintf(tstr, "%ld", ts.tv_sec);

  sprintf(key, "timestamp_sled");

  pal_set_key_value(key, tstr);
}

int
pal_get_fru_discrete_list(uint8_t fru, uint8_t **sensor_list, int *cnt) {
  switch(fru) {
  default:
    if (fru > MAX_NUM_FRUS)
      return -1;
    // Nothing to read yet.
    *sensor_list = NULL;
    *cnt = 0;
  }
    return 0;
}

int
pal_is_debug_card_prsnt(uint8_t *status) {
  int val;
  char path[LARGEST_DEVICE_NAME + 1];

  snprintf(path, LARGEST_DEVICE_NAME, GPIO_DEBUG_PRSNT_N, "value");

  if (read_device(path, &val)) {
    return -1;
  }

  if (val) {
    *status = 1;
  } else {
    *status = 0;
  }

  return 0;
}

static int
pal_set_post_gpio_out(void) {
  char path[LARGEST_DEVICE_NAME + 1];
  int ret;
  char *val = "out";

  snprintf(path, LARGEST_DEVICE_NAME, GPIO_POSTCODE_0, "direction");
  ret = write_device(path, val);
  if (ret) {
    goto post_exit;
  }

  snprintf(path, LARGEST_DEVICE_NAME, GPIO_POSTCODE_1, "direction");
  ret = write_device(path, val);
  if (ret) {
    goto post_exit;
  }

  snprintf(path, LARGEST_DEVICE_NAME, GPIO_POSTCODE_2, "direction");
  ret = write_device(path, val);
  if (ret) {
    goto post_exit;
  }

  snprintf(path, LARGEST_DEVICE_NAME, GPIO_POSTCODE_3, "direction");
  ret = write_device(path, val);
  if (ret) {
    goto post_exit;
  }

  snprintf(path, LARGEST_DEVICE_NAME, GPIO_POSTCODE_4, "direction");
  ret = write_device(path, val);
  if (ret) {
    goto post_exit;
  }

  snprintf(path, LARGEST_DEVICE_NAME, GPIO_POSTCODE_5, "direction");
  ret = write_device(path, val);
  if (ret) {
    goto post_exit;
  }

  snprintf(path, LARGEST_DEVICE_NAME, GPIO_POSTCODE_6, "direction");
  ret = write_device(path, val);
  if (ret) {
    goto post_exit;
  }

  snprintf(path, LARGEST_DEVICE_NAME, GPIO_POSTCODE_7, "direction");
  ret = write_device(path, val);
  if (ret) {
    goto post_exit;
  }

  post_exit:
  if (ret) {
#ifdef DEBUG
    OBMC_WARN("write_device failed for %s\n", path);
#endif
    return -1;
  } else {
    return 0;
  }
}

// Display the given POST code using GPIO port
static int
pal_post_display(uint8_t status) {
  char path[LARGEST_DEVICE_NAME + 1];
  int ret;
  char *val;

#ifdef DEBUG
  OBMC_WARN("pal_post_display: status is %d\n", status);
#endif

  ret = pal_set_post_gpio_out();
  if (ret) {
    return -1;
  }

  snprintf(path, LARGEST_DEVICE_NAME, GPIO_POSTCODE_0, "value");
  if (BIT(status, 0)) {
    val = "1";
  } else {
    val = "0";
  }

  ret = write_device(path, val);
  if (ret) {
    goto post_exit;
  }

  snprintf(path, LARGEST_DEVICE_NAME, GPIO_POSTCODE_1, "value");
  if (BIT(status, 1)) {
    val = "1";
  } else {
    val = "0";
  }

  ret = write_device(path, val);
  if (ret) {
    goto post_exit;
  }

  snprintf(path, LARGEST_DEVICE_NAME, GPIO_POSTCODE_2, "value");
  if (BIT(status, 2)) {
    val = "1";
  } else {
    val = "0";
  }

  ret = write_device(path, val);
  if (ret) {
    goto post_exit;
  }

  snprintf(path, LARGEST_DEVICE_NAME, GPIO_POSTCODE_3, "value");
  if (BIT(status, 3)) {
    val = "1";
  } else {
    val = "0";
  }

  ret = write_device(path, val);
  if (ret) {
    goto post_exit;
  }

  snprintf(path, LARGEST_DEVICE_NAME, GPIO_POSTCODE_4, "value");
  if (BIT(status, 4)) {
    val = "1";
  } else {
    val = "0";
  }

  ret = write_device(path, val);
  if (ret) {
    goto post_exit;
  }

  snprintf(path, LARGEST_DEVICE_NAME, GPIO_POSTCODE_5, "value");
  if (BIT(status, 5)) {
    val = "1";
  } else {
    val = "0";
  }

  ret = write_device(path, val);
  if (ret) {
    goto post_exit;
  }

  snprintf(path, LARGEST_DEVICE_NAME, GPIO_POSTCODE_6, "value");
  if (BIT(status, 6)) {
    val = "1";
  } else {
    val = "0";
  }

  ret = write_device(path, val);
  if (ret) {
    goto post_exit;
  }

  snprintf(path, LARGEST_DEVICE_NAME, GPIO_POSTCODE_7, "value");
  if (BIT(status, 7)) {
    val = "1";
  } else {
    val = "0";
  }

  ret = write_device(path, val);
  if (ret) {
    goto post_exit;
  }

post_exit:
  if (ret) {
#ifdef DEBUG
    OBMC_WARN("write_device failed for %s\n", path);
#endif
    return -1;
  } else {
    return 0;
  }
}

// Handle the received post code, for now display it on debug card
int
pal_post_handle(uint8_t slot, uint8_t status) {
  uint8_t prsnt;
  int ret;

  // Check for debug card presence
  ret = pal_is_debug_card_prsnt(&prsnt);
  if (ret) {
    return ret;
  }

  // No debug card present, return
  if (!prsnt) {
    return 0;
  }

  // Display the post code in the debug card
  ret = pal_post_display(status);
  if (ret) {
    return ret;
  }

  return 0;
}

// Return the Front panel Power Button
int
pal_get_board_rev(int *rev) {
  char path[LARGEST_DEVICE_NAME + 1];
  int val_id_0, val_id_1, val_id_2;

  snprintf(path, LARGEST_DEVICE_NAME, GPIO_SMB_REV_ID_0, "value");
  if (read_device(path, &val_id_0)) {
    return -1;
  }

  snprintf(path, LARGEST_DEVICE_NAME, GPIO_SMB_REV_ID_1, "value");
  if (read_device(path, &val_id_1)) {
    return -1;
  }

  snprintf(path, LARGEST_DEVICE_NAME, GPIO_SMB_REV_ID_2, "value");
  if (read_device(path, &val_id_2)) {
    return -1;
  }

  *rev = val_id_0 | (val_id_1 << 1) | (val_id_2 << 2);

  return 0;
}

int
pal_get_board_type(uint8_t *brd_type){
  char path[LARGEST_DEVICE_NAME + 1];
  int val;

  snprintf(path, LARGEST_DEVICE_NAME, GPIO_BMC_BRD_TPYE, "value");
  if (read_device(path, &val)) {
    return CC_UNSPECIFIED_ERROR;
  }

  if(val == 0x1){
    *brd_type = BRD_TYPE_WEDGE400;
    return CC_SUCCESS;
  }else if(val == 0x0){
    *brd_type = BRD_TYPE_WEDGE400C;
    return CC_SUCCESS;
  }else{
    return CC_UNSPECIFIED_ERROR;
  }
}

int
pal_get_board_type_rev(uint8_t *brd_type_rev){
  char path[LARGEST_DEVICE_NAME + 1];
  int brd_rev;
  uint8_t brd_type;
  int val;
  if( pal_get_board_rev(&brd_rev) != 0 ||
      pal_get_board_type(&brd_type) != 0 ){
        return CC_UNSPECIFIED_ERROR;
  } else if ( brd_type == BRD_TYPE_WEDGE400 ){
    switch ( brd_rev ) {
      case 0x00:
        /* For WEDGE400 EVT & EVT3 need to detect from SMBCPLD */
        snprintf(path, LARGEST_DEVICE_NAME, SMB_SYSFS, "board_ver");
        if (read_device(path, &val)) {
          return CC_UNSPECIFIED_ERROR;
        }
        if ( val == 0x00 ) {
          *brd_type_rev = BOARD_WEDGE400_EVT;
        } else {
          *brd_type_rev = BOARD_WEDGE400_EVT3;
        }
        break;
      case 0x02: *brd_type_rev = BOARD_WEDGE400_DVT; break;
      case 0x03: *brd_type_rev = BOARD_WEDGE400_DVT2_PVT_PVT2; break;
      case 0x04: *brd_type_rev = BOARD_WEDGE400_PVT3; break;
      default:
        *brd_type_rev = BOARD_UNDEFINED;
        return CC_UNSPECIFIED_ERROR;
    }
  } else if ( brd_type == BRD_TYPE_WEDGE400C ){
    switch ( brd_rev ) {
      case 0x00: *brd_type_rev = BOARD_WEDGE400C_EVT; break;
      case 0x01: *brd_type_rev = BOARD_WEDGE400C_EVT2; break;
      case 0x02: *brd_type_rev = BOARD_WEDGE400C_DVT; break;
      default:
        *brd_type_rev = BOARD_UNDEFINED;
        return CC_UNSPECIFIED_ERROR;
    }
  } else {
    *brd_type_rev = BOARD_UNDEFINED;
    return CC_UNSPECIFIED_ERROR;
  }
  return CC_SUCCESS;
}

int
pal_get_board_id(uint8_t slot, uint8_t *req_data, uint8_t req_len, uint8_t *res_data, uint8_t *res_len)
{
	int board_id = 0, board_rev ;
	unsigned char *data = res_data;
	int completion_code = CC_UNSPECIFIED_ERROR;


  int i = 0, num_chips, num_pins;
  char device[64], path[32];
	gpiochip_desc_t *chips[GPIO_CHIP_MAX];
  gpiochip_desc_t *gcdesc;
  gpio_desc_t *gdesc;
  gpio_value_t value;

	num_chips = gpiochip_list(chips, ARRAY_SIZE(chips));
  if(num_chips < 0){
    *res_len = 0;
		return completion_code;
  }

  gcdesc = gpiochip_lookup(SCM_BRD_ID);
  if (gcdesc == NULL) {
    *res_len = 0;
		return completion_code;
  }

  num_pins = gpiochip_get_ngpio(gcdesc);
  gpiochip_get_device(gcdesc, device, sizeof(device));

  for(i = 0; i < num_pins; i++){
    sprintf(path, "%s%d", "BRD_ID_",i);
    gpio_export_by_offset(device,i,path);
    gdesc = gpio_open_by_shadow(path);
    if (gpio_get_value(gdesc, &value) == 0) {
      board_id  |= (((int)value)<<i);
    }
    gpio_unexport(path);
  }

	if(pal_get_board_rev(&board_rev) == -1){
    *res_len = 0;
		return completion_code;
  }

	*data++ = board_id;
	*data++ = board_rev;
	*data++ = slot;
	*data++ = 0x00; // 1S Server.
	*res_len = data - res_data;
	completion_code = CC_SUCCESS;

	return completion_code;
}

int
pal_get_cpld_board_rev(int *rev, const char *device) {
  char full_name[LARGEST_DEVICE_NAME + 1];

  snprintf(full_name, LARGEST_DEVICE_NAME, device, "board_ver");
  if (read_device(full_name, rev)) {
    return -1;
  }

  return 0;
}

int
pal_get_cpld_fpga_fw_ver(uint8_t fru, const char *device, uint8_t* ver) {
  int val = -1;
  char ver_path[PATH_MAX];
  char sub_ver_path[PATH_MAX];

  switch(fru) {
    case FRU_CPLD:
      if (!(strncmp(device, SWPLD1, strlen(SWPLD1)))) {
        snprintf(ver_path, sizeof(ver_path), SWPLD1_SYSFS, "swpld1_ver");
        snprintf(sub_ver_path, sizeof(sub_ver_path), SWPLD1_SYSFS, "swpld1_ver_type");
      } else if (!(strncmp(device, SWPLD2, strlen(SWPLD2)))) {
        snprintf(ver_path, sizeof(ver_path), SWPLD2_SYSFS, "swpld2_ver");
        snprintf(sub_ver_path, sizeof(sub_ver_path), SWPLD2_SYSFS, "swpld2_ver_type");
      } else if (!(strncmp(device, SWPLD3, strlen(SWPLD3)))) {
        snprintf(ver_path, sizeof(ver_path), SWPLD3_SYSFS, "swpld3_ver");
        snprintf(sub_ver_path, sizeof(sub_ver_path), SWPLD3_SYSFS, "swpld3_ver_type");
      } else {
        return -1;
      }
      break;
    case FRU_FPGA:
      return -1;
      break;
    default:
      return -1;
  }

  if (!read_device(ver_path, &val)) {
    ver[0] = (uint8_t)val;
  } else {
    return -1;
  }

  if (!read_device(sub_ver_path, &val)) {
    ver[1] = (uint8_t)val;
  } else {
    printf("[debug][ver:%s]\n", ver_path);
    printf("[debug][sub_ver:%s]\n", sub_ver_path);
    OBMC_INFO("[debug][ver:%s]\n", ver_path);
    OBMC_INFO("[debug][ver_type:%s]\n", sub_ver_path);
    return -1;
  }

  return 0;
}

int
pal_set_last_pwr_state(uint8_t fru, char *state) {

  int ret;
  char key[MAX_KEY_LEN];

  sprintf(key, "%s", "pwr_server_last_state");

  ret = pal_set_key_value(key, state);
  if (ret < 0) {
#ifdef DEBUG
    OBMC_WARN("pal_set_last_pwr_state: pal_set_key_value failed for "
        "fru %u", fru);
#endif
  }
  return ret;
}

int
pal_get_last_pwr_state(uint8_t fru, char *state) {
  int ret;
  char key[MAX_KEY_LEN];

  sprintf(key, "%s", "pwr_server_last_state");

  ret = pal_get_key_value(key, state);
  if (ret < 0) {
#ifdef DEBUG
    OBMC_WARN("pal_get_last_pwr_state: pal_get_key_value failed for "
      "fru %u", fru);
#endif
  }
  return ret;
}

int
pal_set_com_pwr_btn_n(char *status) {
  char path[64];
  int ret;
  sprintf(path, SCM_SYSFS, COM_PWR_BTN_N);

  ret = write_device(path, status);
  if (ret) {
#ifdef DEBUG
  OBMC_WARN("write_device failed for %s\n", path);
#endif
    return -1;
  }

  return 0;
}

int
pal_get_num_slots(uint8_t *num)
{
  *num = MAX_NUM_SCM;
  return PAL_EOK;
}

// Power On the COM-E
static int
scm_power_on(uint8_t slot_id) {
  int ret;

  ret = run_command("/usr/local/bin/wedge_power.sh on");
  if (ret) {
    OBMC_ERROR(ret, "%s failed", __func__);
    return -1;
  }
  return 0;
}

// Power Off the COM-E
static int
scm_power_off(uint8_t slot_id) {
  int ret;

  ret = run_command("/usr/local/bin/wedge_power.sh off");
  if (ret) {
    OBMC_ERROR(ret, "%s failed", __func__);
    return -1;
  }
  return 0;
}

// Power Button trigger the server in given slot
static int
cpu_power_btn(uint8_t slot_id) {
  int ret;

  ret = pal_set_com_pwr_btn_n("0");
  if (ret)
    return -1;
  sleep(DELAY_POWER_BTN);
  ret = pal_set_com_pwr_btn_n("1");
  if (ret)
    return -1;

  return 0;
}

// set CPU power off with power button
static int
cpu_power_off(uint8_t slot_id) {
  int ret = pal_set_com_pwr_btn_n("0");//set COM_PWR_BTN_N to low
  if(ret){
    OBMC_ERROR(ret, "%s set COM-e power button low failed.\n", __func__);
    return -1;
  }
  sleep(6);
  ret = pal_set_com_pwr_btn_n("1");//set COM_PWR_BTN_N to high
  if(ret){
    OBMC_ERROR(ret, "%s set COM-e power button high failed.\n", __func__);
    return -1;
  }
  return 0;
}

// Power Off, Power On, or Power Reset the server in given slot
int
pal_set_server_power(uint8_t slot_id, uint8_t cmd) {
  int ret = 0;
  uint8_t status;

  if (pal_get_server_power(slot_id, &status) < 0) {
    return -1;
  }

  switch(cmd) {
    case SERVER_POWER_ON:
      if (status == SERVER_POWER_ON)
        return 1;
      else
        return scm_power_on(slot_id);
      break;

    case SERVER_POWER_OFF:
      if (status == SERVER_POWER_OFF)
        return 1;
      else
        return scm_power_off(slot_id);
      break;

    case SERVER_POWER_CYCLE:
      if (status == SERVER_POWER_ON) {
        if (scm_power_off(slot_id))
          return -1;

        sleep(DELAY_POWER_CYCLE);

        return scm_power_on(slot_id);

      } else if (status == SERVER_POWER_OFF) {

        return (scm_power_on(slot_id));
      }
      break;

    case SERVER_POWER_RESET:
    if (status == SERVER_POWER_ON) {
        ret = pal_set_rst_btn(slot_id, 0);
        if (ret < 0) {
          OBMC_CRIT("Micro-server can't power reset");
          return ret;
        }
        msleep(100); //some server miss to detect a quick pulse, so delay 100ms between low high
        ret = pal_set_rst_btn(slot_id, 1);
        if (ret < 0) {
          OBMC_CRIT("Micro-server in reset state, can't go back to normal state");
          return ret;
        }
      } else if (status == SERVER_POWER_OFF) {
          OBMC_CRIT("Micro-server power status is off, ignore power reset action");
        return -2;
      }
      break;

    case SERVER_GRACEFUL_SHUTDOWN:
      ret = cpu_power_off(slot_id);
      break;

    case SERVER_GRACEFUL_POWER_ON:
      ret = cpu_power_btn(slot_id);
      break;

    default:
      ret = -1;
      break;
  }

  return ret;
}

int
pal_set_th3_power(int option) {
  char path[256];
  int ret;
  uint8_t brd_type;
  char sysfs[128];
  if(pal_get_board_type(&brd_type)){
    return -1;
  }

  if(brd_type == BRD_TYPE_WEDGE400){
    sprintf(sysfs,TH3_POWER);
  }else if(brd_type == BRD_TYPE_WEDGE400C){
    sprintf(sysfs,GB_POWER);
  }else{
    return -1;
  }

  switch(option) {
    case TH3_POWER_ON:
      sprintf(path, SMB_SYSFS, sysfs);
      ret = write_device(path, "1");
      break;
    case TH3_POWER_OFF:
      sprintf(path, SMB_SYSFS, sysfs);
      ret = write_device(path, "0");
      break;
    case TH3_RESET:
      sprintf(path, SMB_SYSFS, sysfs);
      ret = write_device(path, "0");
      sleep(1);
      ret = write_device(path, "1");
      break;
    default:
      ret = -1;
  }
  if (ret)
    return -1;
  return 0;
}

static int check_dir_exist(const char *device) {
  char cmd[LARGEST_DEVICE_NAME + 1];
  FILE *fp;
  char cmd_ret;
  int ret=-1;

  // Check if device exists
  snprintf(cmd, LARGEST_DEVICE_NAME, "[ -e %s ];echo $?", device);
  fp = popen(cmd, "r");
  if(NULL == fp)
     return -1;

  cmd_ret = fgetc(fp);
  ret = pclose(fp);
  if(-1 == ret)
    return -1;
  if('0' != cmd_ret) {
    return -1;
  }

  return 0;
}

static int
get_current_dir(const char *device, char *dir_name) {
  char cmd[LARGEST_DEVICE_NAME + 1];
  FILE *fp;
  int ret=-1;
  int size;

  // Get current working directory
  snprintf(
      cmd, LARGEST_DEVICE_NAME, "cd %s;pwd", device);

  fp = popen(cmd, "r");
  if(NULL == fp)
     return -1;
  if (fgets(dir_name, LARGEST_DEVICE_NAME, fp) == NULL) {
    pclose(fp);
    return -1;
  }

  ret = pclose(fp);
  if(-1 == ret)
     OBMC_ERROR(-1, "%s pclose() fail ", __func__);

  // Remove the newline character at the end
  size = strlen(dir_name);
  dir_name[size-1] = '\0';

  return 0;
}

#if DELTA_SKIP
static int
read_attr_integer(const char *device, const char *attr, int *value) {
  char full_name[LARGEST_DEVICE_NAME * 2];
  char dir_name[LARGEST_DEVICE_NAME + 1];

  // Get current working directory
  if (get_current_dir(device, dir_name)) {
    return -1;
  }

  snprintf(
      full_name, sizeof(full_name), "%s/%s", dir_name, attr);

  if (read_device(full_name, value)) {
    return -1;
  }

  return 0;
}
#endif

static int
read_attr(const char *device, const char *attr, float *value) {
  char full_name[LARGEST_DEVICE_NAME * 2];
  char dir_name[LARGEST_DEVICE_NAME + 1];
  int tmp;

  // Get current working directory
  if (get_current_dir(device, dir_name)) {
    return -1;
  }

  snprintf(
      full_name, sizeof(full_name), "%s/%s", dir_name, attr);

  if (read_device(full_name, &tmp)) {
    return -1;
  }

  *value = ((float)tmp)/UNIT_DIV;

  return 0;
}

static int
read_fan_rpm_psu(const char *device, float *value) {
  char full_name[LARGEST_DEVICE_NAME * 2];
  char dir_name[LARGEST_DEVICE_NAME + 1];
  char device_name[13];
  int tmp;

  /* Get current working directory */
  if (get_current_dir(device, dir_name)) {
    return -1;
  }

  snprintf(device_name, 13, "read_fan_spd");
  snprintf(full_name, sizeof(full_name), "%s/%s", dir_name, device_name);
  if (read_device(full_name, &tmp)) {
    return -1;
  }

  *value = (float) tmp;

  return 0;
}

static int
read_fan_rpm_f(const char *device, uint8_t fan, float *value) {
  char full_name[LARGEST_DEVICE_NAME * 2];
  char dir_name[LARGEST_DEVICE_NAME + 1];
  char device_name[12];
  int tmp;

  /* Get current working directory */
  if (get_current_dir(device, dir_name)) {
    return -1;
  }

  snprintf(device_name, 12, "fan%d_input", fan);
  snprintf(full_name, sizeof(full_name), "%s/%s", dir_name, device_name);
  if (read_device(full_name, &tmp)) {
    return -1;
  }

  *value = (float)tmp;

  return 0;
}

static int
read_fan_rpm(const char *device, uint8_t fan, int *value) {
  char full_name[LARGEST_DEVICE_NAME * 2];
  char dir_name[LARGEST_DEVICE_NAME + 1];
  char device_name[12];
  int tmp = 0;
  int ret = 0;

  /* Get current working directory */
  if (get_current_dir(device, dir_name)) {
    return -1;
  }

  ret = snprintf(device_name, sizeof(device_name), "fan%d_input", fan);
  if (ret < 0) {
    return -1;
  }
  snprintf(full_name, sizeof(full_name), "%s/%s", dir_name, device_name);
  if (read_device(full_name, &tmp)) {
    return -1;
  }

  *value = tmp;

  return 0;
}

int
pal_get_fan_speed(uint8_t fan, int *rpm) {
  if (fan >= pal_pwm_cnt) {
    OBMC_INFO("get_fan_speed: invalid fan#:%d", fan);
    return -1;
  }

  if (fan < 2) {
    return read_fan_rpm(SMB_FAN_CONTROLLER1_DEVICE, (fan + 1), rpm);
  } else if ( fan < 7) {
    return read_fan_rpm(SMB_FAN_CONTROLLER2_DEVICE, (fan - 1), rpm);
  }

  return read_fan_rpm(SMB_FAN_CONTROLLER3_DEVICE, (fan - 6), rpm);
}

int
pal_set_fan_speed(uint8_t fan, uint8_t pwm) {
  char path[LARGEST_DEVICE_NAME * 2];
  char device_name[5];
  char pwm_val[4];
  int  val = 0;
  int  ret = 0;

  if (fan >= pal_pwm_cnt) {
    OBMC_INFO("set_fan_speed: invalid fan#:%d", fan);
    return -1;
  }

  if (fan < 2) {
    ret = snprintf(device_name, sizeof(device_name), "pwm%d", fan + 1);
    if (ret < 0) {
      return -1;
    }
    snprintf(path, sizeof(path), "%s/%s", SMB_FAN_CONTROLLER1_DEVICE, device_name);
  } else if ( fan < 7) {
    ret = snprintf(device_name, sizeof(device_name), "pwm%d", fan - 1);
    if (ret < 0) {
      return -1;
    }
    snprintf(path, sizeof(path), "%s/%s", SMB_FAN_CONTROLLER2_DEVICE, device_name);
  } else {
    ret = snprintf(device_name, sizeof(device_name), "pwm%d", fan - 6);
    if (ret < 0) {
      return -1;
    }
    snprintf(path, sizeof(path), "%s/%s", SMB_FAN_CONTROLLER3_DEVICE, device_name);
  }

  val = (pwm * PWM_UNIT_MAX) / 100;
  snprintf(pwm_val, 4, "%d", val);

  return write_device(path, pwm_val);
}

int pal_get_fan_name(uint8_t num, char *name)
{
  if (num > pal_tach_cnt) {
    syslog(LOG_WARNING, "%s: invalid fan#:%d", __func__, num);
    return -1;
  }

  snprintf(name, 32, "Fan %d",num);

  return 0;
}

int pal_get_pwm_value(uint8_t fan, uint8_t *pwm)
{
  char path[LARGEST_DEVICE_NAME * 2] = {};
  char device_name[5] = {};
  int value = 0;
  int ret = 0;

  if (fan >= pal_pwm_cnt){
    syslog(LOG_WARNING, "%s: invalid fan#:%d", __func__, fan);
    return -1;
  }

  if (fan < 2) {
    ret = snprintf(device_name, sizeof(device_name), "pwm%d", fan + 1);
    if (ret < 0) {
      return -1;
    }
    snprintf(path, sizeof(path), "%s/%s", SMB_FAN_CONTROLLER1_DEVICE, device_name);
  } else if ( fan < 7) {
    ret = snprintf(device_name, sizeof(device_name), "pwm%d", fan - 1);
    if (ret < 0) {
      return -1;
    }
    snprintf(path, sizeof(path), "%s/%s", SMB_FAN_CONTROLLER2_DEVICE, device_name);
  } else {
    ret = snprintf(device_name, sizeof(device_name), "pwm%d", fan - 6);
    if (ret < 0) {
      return -1;
    }
    snprintf(path, sizeof(path), "%s/%s", SMB_FAN_CONTROLLER3_DEVICE, device_name);
  }

  if (read_device(path, &value)) {
    return -1;
  }
  *pwm = (100 * value) / PWM_UNIT_MAX;

  return 0;
}

static int
smb_sensor_read(uint8_t sensor_num, float *value) {

  int ret = -1;

  switch(sensor_num) {
    case SMB_SENSOR_TMP75_LF_TEMP:
      ret = read_attr(SMB_TMP75_LF_DEVICE, TEMP(1), value);
      break;
    case SMB_SENSOR_TMP75_RF_TEMP:
      ret = read_attr(SMB_TMP75_RF_DEVICE, TEMP(1), value);
      break;
    case SMB_SENSOR_TMP75_UPPER_MAC_TEMP:
      ret = read_attr(SMB_TMP75_UPPER_MAC_DEVICE, TEMP(1), value);
      break;
    case SMB_SENSOR_TMP75_LOWER_MAC_TEMP:
      ret = read_attr(SMB_TMP75_LOWER_MAC_DEVICE, TEMP(1), value);
      break;
    case SMB_SENSOR_FAN1_FRONT_TACH:
      ret = read_fan_rpm_f(SMB_FAN_CONTROLLER1_DEVICE, 1, value);
      break;
    case SMB_SENSOR_FAN1_REAR_TACH:
      ret = read_fan_rpm_f(SMB_FAN_CONTROLLER1_DEVICE, 2, value);
      break;
    case SMB_SENSOR_FAN2_FRONT_TACH:
      ret = read_fan_rpm_f(SMB_FAN_CONTROLLER2_DEVICE, 1, value);
      break;
    case SMB_SENSOR_FAN2_REAR_TACH:
      ret = read_fan_rpm_f(SMB_FAN_CONTROLLER2_DEVICE, 2, value);
      break;
    case SMB_SENSOR_FAN3_FRONT_TACH:
      ret = read_fan_rpm_f(SMB_FAN_CONTROLLER2_DEVICE, 3, value);
      break;
    case SMB_SENSOR_FAN3_REAR_TACH:
      ret = read_fan_rpm_f(SMB_FAN_CONTROLLER2_DEVICE, 4, value);
      break;
    case SMB_SENSOR_FAN4_FRONT_TACH:
      ret = read_fan_rpm_f(SMB_FAN_CONTROLLER2_DEVICE, 5, value);
      break;
    case SMB_SENSOR_FAN4_REAR_TACH:
      ret = read_fan_rpm_f(SMB_FAN_CONTROLLER3_DEVICE, 1, value);
      break;
    case SMB_SENSOR_FAN5_FRONT_TACH:
      ret = read_fan_rpm_f(SMB_FAN_CONTROLLER3_DEVICE, 2, value);
      break;
    case SMB_SENSOR_FAN5_REAR_TACH:
      ret = read_fan_rpm_f(SMB_FAN_CONTROLLER3_DEVICE, 3, value);
      break;
    case SMB_SENSOR_FAN6_FRONT_TACH:
      ret = read_fan_rpm_f(SMB_FAN_CONTROLLER3_DEVICE, 4, value);
      break;
    case SMB_SENSOR_FAN6_REAR_TACH:
      ret = read_fan_rpm_f(SMB_FAN_CONTROLLER3_DEVICE, 5, value);
      break;
    case SMB_BMC_ADC0_VSEN:
      ret = read_attr(AST_ADC_DEVICE, VOLT(1), value);
      break;
    case SMB_BMC_ADC1_VSEN:
      ret = read_attr(AST_ADC_DEVICE, VOLT(2), value);
      break;
    case SMB_BMC_ADC2_VSEN:
      ret = read_attr(AST_ADC_DEVICE, VOLT(3), value);
      break;
    case SMB_BMC_ADC3_VSEN:
      ret = read_attr(AST_ADC_DEVICE, VOLT(4), value);
      break;
    case SMB_BMC_ADC5_VSEN:
      ret = read_attr(AST_ADC_DEVICE, VOLT(6), value);
      break;
    case SMB_BMC_ADC6_VSEN:
      ret = read_attr(AST_ADC_DEVICE, VOLT(7), value);
      break;
    case SMB_BMC_ADC7_VSEN:
      ret = read_attr(AST_ADC_DEVICE, VOLT(8), value);
      break;
    case SMB_BMC_ADC8_VSEN:
      ret = read_attr(AST_ADC_DEVICE, VOLT(9), value);
      break;
    case SMB_BMC_ADC9_VSEN:
      ret = read_attr(AST_ADC_DEVICE, VOLT(10), value);
      break;
    case SMB_BMC_ADC13_VSEN:
      ret = read_attr(AST_ADC_DEVICE, VOLT(14), value);
      break;
    case SMB_BMC_ADC14_VSEN:
      ret = read_attr(AST_ADC_DEVICE, VOLT(15), value);
      break;
    case SMB_BMC_ADC15_VSEN:
      ret = read_attr(AST_ADC_DEVICE, VOLT(16), value);
      break;
    default:
      ret = READING_NA;
      break;
  }
  return ret;
}

static int
psu_sensor_read(uint8_t sensor_num, float *value) {

  int ret = -1;

  switch(sensor_num) {
    case PSU1_SENSOR_IN_VOLT:
      ret = read_attr(PSU1_DEVICE, "read_vin", value);
      break;
    case PSU1_SENSOR_OUT_VOLT:
      ret = read_attr(PSU1_DEVICE, "read_vout", value);
      break;
    case PSU1_SENSOR_IN_CURR:
      ret = read_attr(PSU1_DEVICE, "read_iin", value);
      break;
    case PSU1_SENSOR_OUT_CURR:
      ret = read_attr(PSU1_DEVICE, "read_iout", value);
      break;
    case PSU1_SENSOR_IN_POWER:
      ret = read_attr(PSU1_DEVICE, "read_pin", value);
      break;
    case PSU1_SENSOR_OUT_POWER:
      ret = read_attr(PSU1_DEVICE, "read_pout", value);
      break;
    case PSU1_SENSOR_FAN_TACH:
      ret = read_fan_rpm_psu(PSU1_DEVICE, value);
      break;
    case PSU1_SENSOR_TEMP1:
      ret = read_attr(PSU1_DEVICE, "read_temp1", value);
      break;
    case PSU1_SENSOR_TEMP2:
      ret = read_attr(PSU1_DEVICE, "read_temp2", value);
      break;
    case PSU2_SENSOR_IN_VOLT:
      ret = read_attr(PSU2_DEVICE, "read_vin", value);
      break;
    case PSU2_SENSOR_OUT_VOLT:
      ret = read_attr(PSU2_DEVICE, "read_vout", value);
      break;
    case PSU2_SENSOR_IN_CURR:
      ret = read_attr(PSU2_DEVICE, "read_iin", value);
      break;
    case PSU2_SENSOR_OUT_CURR:
      ret = read_attr(PSU2_DEVICE, "read_iout", value);
      break;
    case PSU2_SENSOR_IN_POWER:
      ret = read_attr(PSU2_DEVICE, "read_pin", value);
      break;
    case PSU2_SENSOR_OUT_POWER:
      ret = read_attr(PSU2_DEVICE, "read_pout", value);
      break;
    case PSU2_SENSOR_FAN_TACH:
      ret = read_fan_rpm_psu(PSU2_DEVICE, value);
      break;
    case PSU2_SENSOR_TEMP1:
      ret = read_attr(PSU2_DEVICE, "read_temp1", value);
      break;
    case PSU2_SENSOR_TEMP2:
      ret = read_attr(PSU2_DEVICE, "read_temp2", value);
      break;
    default:
      ret = READING_NA;
      break;
  }
  return ret;
}

int
pal_sensor_read_raw(uint8_t fru, uint8_t sensor_num, void *value) {

  char key[MAX_KEY_LEN];
  char fru_name[32];
  int ret, delay = 500;
  uint8_t prsnt = 0;
  uint8_t status = 0;

  pal_get_fru_name(fru, fru_name);

  ret = pal_is_fru_prsnt(fru, &prsnt);
  if (ret) {
    return ret;
  }
  if (!prsnt) {
#ifdef DEBUG
  OBMC_INFO("pal_sensor_read_raw(): %s is not present\n", fru_name);
#endif
    return -1;
  }

  ret = pal_is_fru_ready(fru, &status);
  if (ret) {
    return ret;
  }
  if (!status) {
#ifdef DEBUG
  OBMC_INFO("pal_sensor_read_raw(): %s is not ready\n", fru_name);
#endif
    return -1;
  }

  sprintf(key, "%s_sensor%d", fru_name, sensor_num);
  switch(fru) {
    case FRU_SMB:
      ret = smb_sensor_read(sensor_num, value);
      if (sensor_num >= SMB_SENSOR_FAN1_FRONT_TACH &&
           sensor_num <= SMB_SENSOR_FAN6_REAR_TACH) {
        delay = 100;
      }
      break;
    case FRU_PSU1:
    case FRU_PSU2:
      ret = psu_sensor_read(sensor_num, value);
      break;
    default:
      return -1;
  }

  if (ret == READING_NA || ret == -1) {
    return READING_NA;
  }
  msleep(delay);

  return 0;
}

int
pal_sensor_discrete_read_raw(uint8_t fru, uint8_t sensor_num, void *value) {

  char key[MAX_KEY_LEN];
  char fru_name[32];
  int ret , delay = 500;
  uint8_t prsnt = 0;
  uint8_t status = 0;

  pal_get_fru_name(fru, fru_name);

  ret = pal_is_fru_prsnt(fru, &prsnt);
  if (ret) {
    return ret;
  }
  if (!prsnt) {
#ifdef DEBUG
  OBMC_INFO("pal_sensor_discrete_read_raw(): %s is not present\n", fru_name);
#endif
    return -1;
  }

  ret = pal_is_fru_ready(fru, &status);
  if (ret) {
    return ret;
  }
  if (!status) {
#ifdef DEBUG
  OBMC_INFO("pal_sensor_discrete_read_raw(): %s is not ready\n", fru_name);
#endif
    return -1;
  }

  sprintf(key, "%s_sensor%d", fru_name, sensor_num);
  switch(fru) {
    default:
      return -1;
  }

  if (ret == READING_NA || ret == -1) {
    return READING_NA;
  }
  msleep(delay);

  return 0;
};

static int
get_smb_sensor_name(uint8_t sensor_num, char *name) {
  sprintf(name,SENSOR_NAME_ERR);
  switch(sensor_num) {
    case SMB_SENSOR_TMP75_LF_TEMP:
      sprintf(name, "SMB_SENSOR_TMP75_LF_TEMP");
      break;
    case SMB_SENSOR_TMP75_RF_TEMP:
      sprintf(name, "SMB_SENSOR_TMP75_RF_TEMP");
      break;
    case SMB_SENSOR_TMP75_UPPER_MAC_TEMP:
      sprintf(name, "SMB_SENSOR_TMP75_UPPER_MAC_TEMP");
      break;
    case SMB_SENSOR_TMP75_LOWER_MAC_TEMP:
      sprintf(name, "SMB_SENSOR_TMP75_LOWER_MAC_TEMP");
      break;
    case SMB_SENSOR_FAN1_FRONT_TACH:
      sprintf(name, "FAN1_FRONT_SPEED");
      break;
    case SMB_SENSOR_FAN1_REAR_TACH:
      sprintf(name, "FAN1_REAR_SPEED");
      break;
    case SMB_SENSOR_FAN2_FRONT_TACH:
      sprintf(name, "FAN2_FRONT_SPEED");
      break;
    case SMB_SENSOR_FAN2_REAR_TACH:
      sprintf(name, "FAN2_REAR_SPEED");
      break;
    case SMB_SENSOR_FAN3_FRONT_TACH:
      sprintf(name, "FAN3_FRONT_SPEED");
      break;
    case SMB_SENSOR_FAN3_REAR_TACH:
      sprintf(name, "FAN3_REAR_SPEED");
      break;
    case SMB_SENSOR_FAN4_FRONT_TACH:
      sprintf(name, "FAN4_FRONT_SPEED");
      break;
    case SMB_SENSOR_FAN4_REAR_TACH:
      sprintf(name, "FAN4_REAR_SPEED");
      break;
    case SMB_SENSOR_FAN5_FRONT_TACH:
      sprintf(name, "FAN5_FRONT_SPEED");
      break;
    case SMB_SENSOR_FAN5_REAR_TACH:
      sprintf(name, "FAN5_REAR_SPEED");
      break;
    case SMB_SENSOR_FAN6_FRONT_TACH:
      sprintf(name, "FAN6_FRONT_SPEED");
      break;
    case SMB_SENSOR_FAN6_REAR_TACH:
      sprintf(name, "FAN6_REAR_SPEED");
      break;
    case SMB_BMC_ADC0_VSEN:
      sprintf(name, "ADC_VCC_12V");
      break;
    case SMB_BMC_ADC1_VSEN:
      sprintf(name, "ADC_VCC_5V");
      break;
    case SMB_BMC_ADC2_VSEN:
      sprintf(name, "ADC_VCC_3V3");
      break;
    case SMB_BMC_ADC3_VSEN:
      sprintf(name, "ADC_STBY_5V");
      break;
    case SMB_BMC_ADC5_VSEN:
      sprintf(name, "ADC_VDDC_1V2_JER1");
      break;
    case SMB_BMC_ADC6_VSEN:
      sprintf(name, "ADC_BMC_1V2");
      break;
    case SMB_BMC_ADC7_VSEN:
      sprintf(name, "ADC_VDD_0V91_JER1");
      break;
    case SMB_BMC_ADC8_VSEN:
      sprintf(name, "ADC_BMC_1V15");
      break;
    case SMB_BMC_ADC9_VSEN:
      sprintf(name, "ADC_VDDS_0V8_JER1");
      break;
    case SMB_BMC_ADC13_VSEN:
      sprintf(name, "ADC_MAC_1V8");
      break;
    case SMB_BMC_ADC14_VSEN:
      sprintf(name, "ADC_MB_3V_CT");
      break;
    case SMB_BMC_ADC15_VSEN:
      sprintf(name, "ADC_DDR_2V5");
      break;
    default:
      return -1;
  }
  return 0;
}

static int
get_psu_sensor_name(uint8_t sensor_num, char *name) {

  switch(sensor_num) {
    case PSU1_SENSOR_IN_VOLT:
      sprintf(name, "PSU1_IN_VOLT");
      break;
    case PSU1_SENSOR_OUT_VOLT:
      sprintf(name, "PSU1_OUT_VOLT");
      break;
    case PSU1_SENSOR_IN_CURR:
      sprintf(name, "PSU1_IN_CURR");
      break;
    case PSU1_SENSOR_OUT_CURR:
      sprintf(name, "PSU1_OUT_CURR");
      break;
    case PSU1_SENSOR_IN_POWER:
      sprintf(name, "PSU1_IN_POWER");
      break;
    case PSU1_SENSOR_OUT_POWER:
      sprintf(name, "PSU1_OUT_POWER");
      break;
    case PSU1_SENSOR_FAN_TACH:
      sprintf(name, "PSU1_FAN_SPEED");
      break;
    case PSU1_SENSOR_TEMP1:
      sprintf(name, "PSU1_TEMP1");
      break;
    case PSU1_SENSOR_TEMP2:
      sprintf(name, "PSU1_TEMP2");
      break;
    case PSU2_SENSOR_IN_VOLT:
      sprintf(name, "PSU2_IN_VOLT");
      break;
    case PSU2_SENSOR_OUT_VOLT:
      sprintf(name, "PSU2_OUT_VOLT");
      break;
    case PSU2_SENSOR_IN_CURR:
      sprintf(name, "PSU2_IN_CURR");
      break;
    case PSU2_SENSOR_OUT_CURR:
      sprintf(name, "PSU2_OUT_CURR");
      break;
    case PSU2_SENSOR_IN_POWER:
      sprintf(name, "PSU2_IN_POWER");
      break;
    case PSU2_SENSOR_OUT_POWER:
      sprintf(name, "PSU2_OUT_POWER");
      break;
    case PSU2_SENSOR_FAN_TACH:
      sprintf(name, "PSU2_FAN_SPEED");
      break;
    case PSU2_SENSOR_TEMP1:
      sprintf(name, "PSU2_TEMP1");
      break;
    case PSU2_SENSOR_TEMP2:
      sprintf(name, "PSU2_TEMP2");
      break;
    default:
      return -1;
  }
  return 0;
}

int
pal_get_sensor_name(uint8_t fru, uint8_t sensor_num, char *name) {

  int ret = -1;

  switch(fru) {
    case FRU_SMB:
      ret = get_smb_sensor_name(sensor_num, name);
      break;
    case FRU_PSU1:
    case FRU_PSU2:
      ret = get_psu_sensor_name(sensor_num, name);
      break;
    default:
      return -1;
  }
  return ret;
}

static int
get_smb_sensor_units(uint8_t sensor_num, char *units) {

  switch(sensor_num) {
    case SMB_SENSOR_TMP75_LF_TEMP:
    case SMB_SENSOR_TMP75_RF_TEMP:
    case SMB_SENSOR_TMP75_UPPER_MAC_TEMP:
    case SMB_SENSOR_TMP75_LOWER_MAC_TEMP:
      sprintf(units, "C");
      break;
    case SMB_BMC_ADC0_VSEN:
    case SMB_BMC_ADC1_VSEN:
    case SMB_BMC_ADC2_VSEN:
    case SMB_BMC_ADC3_VSEN:
    case SMB_BMC_ADC5_VSEN:
    case SMB_BMC_ADC6_VSEN:
    case SMB_BMC_ADC7_VSEN:
    case SMB_BMC_ADC8_VSEN:
    case SMB_BMC_ADC9_VSEN:
    case SMB_BMC_ADC13_VSEN:
    case SMB_BMC_ADC14_VSEN:
    case SMB_BMC_ADC15_VSEN:
      sprintf(units, "Volts");
      break;
    case SMB_SENSOR_FAN1_FRONT_TACH:
    case SMB_SENSOR_FAN1_REAR_TACH:
    case SMB_SENSOR_FAN2_FRONT_TACH:
    case SMB_SENSOR_FAN2_REAR_TACH:
    case SMB_SENSOR_FAN3_FRONT_TACH:
    case SMB_SENSOR_FAN3_REAR_TACH:
    case SMB_SENSOR_FAN4_FRONT_TACH:
    case SMB_SENSOR_FAN4_REAR_TACH:
    case SMB_SENSOR_FAN5_FRONT_TACH:
    case SMB_SENSOR_FAN5_REAR_TACH:
    case SMB_SENSOR_FAN6_FRONT_TACH:
    case SMB_SENSOR_FAN6_REAR_TACH:
      sprintf(units, "RPM");
      break;
    default:
      return -1;
  }
  return 0;
}

static int
get_psu_sensor_units(uint8_t sensor_num, char *units) {

  switch(sensor_num) {
    case PSU1_SENSOR_IN_VOLT:
    case PSU1_SENSOR_OUT_VOLT:
    case PSU2_SENSOR_IN_VOLT:
    case PSU2_SENSOR_OUT_VOLT:
      sprintf(units, "Volts");
      break;
    case PSU1_SENSOR_IN_CURR:
    case PSU1_SENSOR_OUT_CURR:
    case PSU2_SENSOR_IN_CURR:
    case PSU2_SENSOR_OUT_CURR:
      sprintf(units, "Amps");
      break;
    case PSU1_SENSOR_IN_POWER:
    case PSU1_SENSOR_OUT_POWER:
    case PSU2_SENSOR_IN_POWER:
    case PSU2_SENSOR_OUT_POWER:
      sprintf(units, "Watts");
      break;
    case PSU1_SENSOR_FAN_TACH:
    case PSU2_SENSOR_FAN_TACH:
      sprintf(units, "RPM");
      break;
    case PSU1_SENSOR_TEMP1:
    case PSU1_SENSOR_TEMP2:
    case PSU2_SENSOR_TEMP1:
    case PSU2_SENSOR_TEMP2:
      sprintf(units, "C");
      break;
     default:
      return -1;
  }
  return 0;
}

int
pal_get_sensor_units(uint8_t fru, uint8_t sensor_num, char *units) {
  int ret = -1;

  switch(fru) {
    case FRU_SMB:
      ret = get_smb_sensor_units(sensor_num, units);
      break;
    case FRU_PSU1:
    case FRU_PSU2:
      ret = get_psu_sensor_units(sensor_num, units);
      break;
    default:
      return -1;
  }
  return ret;
}

static void
sensor_thresh_array_init(uint8_t fru) {
  static bool init_done[MAX_NUM_FRUS] = {false};
  int fru_offset;

  if (init_done[fru]) {
    return;
  }

  switch (fru) {
    case FRU_SMB:
      /* BMC ADC Sensors */
      smb_sensor_threshold[SMB_BMC_ADC0_VSEN][UCR_THRESH] = 1.428;
      smb_sensor_threshold[SMB_BMC_ADC0_VSEN][LCR_THRESH] = 1.169;
      smb_sensor_threshold[SMB_BMC_ADC1_VSEN][UCR_THRESH] = 1.446;
      smb_sensor_threshold[SMB_BMC_ADC1_VSEN][LCR_THRESH] = 1.183;
      smb_sensor_threshold[SMB_BMC_ADC2_VSEN][UCR_THRESH] = 1.401;
      smb_sensor_threshold[SMB_BMC_ADC2_VSEN][LCR_THRESH] = 1.268;
      smb_sensor_threshold[SMB_BMC_ADC3_VSEN][UCR_THRESH] = 1.446;
      smb_sensor_threshold[SMB_BMC_ADC3_VSEN][LCR_THRESH] = 1.183;
      smb_sensor_threshold[SMB_BMC_ADC5_VSEN][UCR_THRESH] = 1.224;
      smb_sensor_threshold[SMB_BMC_ADC5_VSEN][LCR_THRESH] = 1.176;
      smb_sensor_threshold[SMB_BMC_ADC6_VSEN][UCR_THRESH] = 1.224;
      smb_sensor_threshold[SMB_BMC_ADC6_VSEN][LCR_THRESH] = 1.176;
      smb_sensor_threshold[SMB_BMC_ADC7_VSEN][UCR_THRESH] = 0.856;
      smb_sensor_threshold[SMB_BMC_ADC7_VSEN][LCR_THRESH] = 0.823;
      smb_sensor_threshold[SMB_BMC_ADC8_VSEN][UCR_THRESH] = 1.173;
      smb_sensor_threshold[SMB_BMC_ADC8_VSEN][LCR_THRESH] = 1.127;
      smb_sensor_threshold[SMB_BMC_ADC9_VSEN][UCR_THRESH] = 0.816;
      smb_sensor_threshold[SMB_BMC_ADC9_VSEN][LCR_THRESH] = 0.784;
      smb_sensor_threshold[SMB_BMC_ADC13_VSEN][UCR_THRESH] = 1.417;
      smb_sensor_threshold[SMB_BMC_ADC13_VSEN][LCR_THRESH] = 1.362;
      smb_sensor_threshold[SMB_BMC_ADC14_VSEN][UCR_THRESH] = 1.366;
      smb_sensor_threshold[SMB_BMC_ADC14_VSEN][LCR_THRESH] = 1.313;
      smb_sensor_threshold[SMB_BMC_ADC15_VSEN][UCR_THRESH] = 1.377;
      smb_sensor_threshold[SMB_BMC_ADC15_VSEN][LCR_THRESH] = 1.323;
      /* SMB TEMP Sensors */
      smb_sensor_threshold[SMB_SENSOR_TMP75_LF_TEMP][UCR_THRESH] = 80;
      smb_sensor_threshold[SMB_SENSOR_TMP75_RF_TEMP][UCR_THRESH] = 80;
      smb_sensor_threshold[SMB_SENSOR_TMP75_UPPER_MAC_TEMP][UCR_THRESH] = 80;
      smb_sensor_threshold[SMB_SENSOR_TMP75_LOWER_MAC_TEMP][UCR_THRESH] = 80;
      smb_sensor_threshold[SMB_SENSOR_FAN1_FRONT_TACH][UCR_THRESH] = 23000;
      smb_sensor_threshold[SMB_SENSOR_FAN1_FRONT_TACH][LCR_THRESH] = 6600;
      smb_sensor_threshold[SMB_SENSOR_FAN1_REAR_TACH][UCR_THRESH] = 20500;
      smb_sensor_threshold[SMB_SENSOR_FAN1_REAR_TACH][LCR_THRESH] = 6600;
      smb_sensor_threshold[SMB_SENSOR_FAN2_FRONT_TACH][UCR_THRESH] = 23000;
      smb_sensor_threshold[SMB_SENSOR_FAN2_FRONT_TACH][LCR_THRESH] = 6600;
      smb_sensor_threshold[SMB_SENSOR_FAN2_REAR_TACH][UCR_THRESH] = 20500;
      smb_sensor_threshold[SMB_SENSOR_FAN2_REAR_TACH][LCR_THRESH] = 6600;
      smb_sensor_threshold[SMB_SENSOR_FAN3_FRONT_TACH][UCR_THRESH] = 23000;
      smb_sensor_threshold[SMB_SENSOR_FAN3_FRONT_TACH][LCR_THRESH] = 6600;
      smb_sensor_threshold[SMB_SENSOR_FAN3_REAR_TACH][UCR_THRESH] = 20500;
      smb_sensor_threshold[SMB_SENSOR_FAN3_REAR_TACH][LCR_THRESH] = 6600;
      smb_sensor_threshold[SMB_SENSOR_FAN4_FRONT_TACH][UCR_THRESH] = 23000;
      smb_sensor_threshold[SMB_SENSOR_FAN4_FRONT_TACH][LCR_THRESH] = 6600;
      smb_sensor_threshold[SMB_SENSOR_FAN4_REAR_TACH][UCR_THRESH] = 20500;
      smb_sensor_threshold[SMB_SENSOR_FAN4_REAR_TACH][LCR_THRESH] = 6600;
      smb_sensor_threshold[SMB_SENSOR_FAN5_FRONT_TACH][UCR_THRESH] = 23000;
      smb_sensor_threshold[SMB_SENSOR_FAN5_FRONT_TACH][LCR_THRESH] = 6600;
      smb_sensor_threshold[SMB_SENSOR_FAN5_REAR_TACH][UCR_THRESH] = 20500;
      smb_sensor_threshold[SMB_SENSOR_FAN5_REAR_TACH][LCR_THRESH] = 6600;
      smb_sensor_threshold[SMB_SENSOR_FAN6_FRONT_TACH][UCR_THRESH] = 23000;
      smb_sensor_threshold[SMB_SENSOR_FAN6_FRONT_TACH][LCR_THRESH] = 6600;
      smb_sensor_threshold[SMB_SENSOR_FAN6_REAR_TACH][UCR_THRESH] = 20500;
      smb_sensor_threshold[SMB_SENSOR_FAN6_REAR_TACH][LCR_THRESH] = 6600;

      break;
    case FRU_PSU1:
    case FRU_PSU2:
      fru_offset = fru - FRU_PSU1;
      psu_sensor_threshold[PSU1_SENSOR_IN_VOLT + (fru_offset * PSU1_SENSOR_CNT)][UCR_THRESH] = 264;
      psu_sensor_threshold[PSU1_SENSOR_IN_VOLT + (fru_offset * PSU1_SENSOR_CNT)][LCR_THRESH] = 90;
      psu_sensor_threshold[PSU1_SENSOR_OUT_VOLT + (fru_offset * PSU1_SENSOR_CNT)][UCR_THRESH] = 12.6;
      psu_sensor_threshold[PSU1_SENSOR_OUT_VOLT + (fru_offset * PSU1_SENSOR_CNT)][LCR_THRESH] = 11.4;
      psu_sensor_threshold[PSU1_SENSOR_IN_CURR + (fru_offset * PSU1_SENSOR_CNT)][UCR_THRESH] = 9;
      psu_sensor_threshold[PSU1_SENSOR_OUT_CURR + (fru_offset * PSU1_SENSOR_CNT)][UCR_THRESH] = 99;
      psu_sensor_threshold[PSU1_SENSOR_IN_POWER + (fru_offset * PSU1_SENSOR_CNT)][UCR_THRESH] = 1500;
      psu_sensor_threshold[PSU1_SENSOR_OUT_POWER + (fru_offset * PSU1_SENSOR_CNT)][UCR_THRESH] = 1500;
      psu_sensor_threshold[PSU1_SENSOR_FAN_TACH + (fru_offset * PSU1_SENSOR_CNT)][UCR_THRESH] = 29500;
      psu_sensor_threshold[PSU1_SENSOR_FAN_TACH + (fru_offset * PSU1_SENSOR_CNT)][LCR_THRESH] = 1000;
      psu_sensor_threshold[PSU1_SENSOR_TEMP1 + (fru_offset * PSU1_SENSOR_CNT)][UCR_THRESH] = 55;
      psu_sensor_threshold[PSU1_SENSOR_TEMP2 + (fru_offset * PSU1_SENSOR_CNT)][UCR_THRESH] = 55;
      break;
  }
  init_done[fru] = true;
}

int
pal_get_sensor_threshold(uint8_t fru, uint8_t sensor_num,
                                      uint8_t thresh, void *value) {
  float *val = (float*) value;

  sensor_thresh_array_init(fru);

  switch(fru) {
    case FRU_SMB:
      *val = smb_sensor_threshold[sensor_num][thresh];
      break;
    case FRU_PSU1:
    case FRU_PSU2:
      *val = psu_sensor_threshold[sensor_num][thresh];
      break;
    default:
      return -1;
    }
    return 0;
}

int
pal_get_fw_info(uint8_t fru, unsigned char target,
                unsigned char* res, unsigned char* res_len) {
  return -1;
}

void
pal_sensor_assert_handle(uint8_t fru, uint8_t snr_num,
                                      float val, uint8_t thresh) {
  char crisel[128];
  char thresh_name[10];

  switch (thresh) {
    case UNR_THRESH:
        sprintf(thresh_name, "UNR");
      break;
    case UCR_THRESH:
        sprintf(thresh_name, "UCR");
      break;
    case UNC_THRESH:
        sprintf(thresh_name, "UNCR");
      break;
    case LNR_THRESH:
        sprintf(thresh_name, "LNR");
      break;
    case LCR_THRESH:
        sprintf(thresh_name, "LCR");
      break;
    case LNC_THRESH:
        sprintf(thresh_name, "LNCR");
      break;
    default:
      OBMC_WARN("pal_sensor_assert_handle: wrong thresh enum value");
      exit(-1);
  }

  switch(snr_num) {
    default:
      return;
  }
  pal_add_cri_sel(crisel);
  return;
}

void
pal_sensor_deassert_handle(uint8_t fru, uint8_t snr_num,
                                        float val, uint8_t thresh) {
  char crisel[128];
  char thresh_name[8];

  switch (thresh) {
    case UNR_THRESH:
      sprintf(thresh_name, "UNR");
      break;
    case UCR_THRESH:
      sprintf(thresh_name, "UCR");
      break;
    case UNC_THRESH:
      sprintf(thresh_name, "UNCR");
      break;
    case LNR_THRESH:
      sprintf(thresh_name, "LNR");
      break;
    case LCR_THRESH:
      sprintf(thresh_name, "LCR");
      break;
    case LNC_THRESH:
      sprintf(thresh_name, "LNCR");
      break;
    default:
      OBMC_WARN(
             "pal_sensor_deassert_handle: wrong thresh enum value");
      return;
  }
  pal_add_cri_sel(crisel);
  return;
}

int
pal_sensor_threshold_flag(uint8_t fru, uint8_t snr_num, uint16_t *flag) {

  switch(fru) {

    default:
      break;
  }
  return 0;
}

static void
smb_sensor_poll_interval(uint8_t sensor_num, uint32_t *value) {

  switch(sensor_num) {
    case SMB_SENSOR_TMP75_LF_TEMP:
    case SMB_SENSOR_TMP75_RF_TEMP:
    case SMB_SENSOR_TMP75_UPPER_MAC_TEMP:
    case SMB_SENSOR_TMP75_LOWER_MAC_TEMP:
      *value = 2;
      break;
    case SMB_BMC_ADC0_VSEN:
    case SMB_BMC_ADC1_VSEN:
    case SMB_BMC_ADC2_VSEN:
    case SMB_BMC_ADC3_VSEN:
    case SMB_BMC_ADC5_VSEN:
    case SMB_BMC_ADC6_VSEN:
    case SMB_BMC_ADC7_VSEN:
    case SMB_BMC_ADC8_VSEN:
    case SMB_BMC_ADC9_VSEN:
    case SMB_BMC_ADC13_VSEN:
    case SMB_BMC_ADC14_VSEN:
    case SMB_BMC_ADC15_VSEN:
      *value = 30;
      break;
    case SMB_SENSOR_FAN1_FRONT_TACH:
    case SMB_SENSOR_FAN1_REAR_TACH:
    case SMB_SENSOR_FAN2_FRONT_TACH:
    case SMB_SENSOR_FAN2_REAR_TACH:
    case SMB_SENSOR_FAN3_FRONT_TACH:
    case SMB_SENSOR_FAN3_REAR_TACH:
    case SMB_SENSOR_FAN4_FRONT_TACH:
    case SMB_SENSOR_FAN4_REAR_TACH:
    case SMB_SENSOR_FAN5_FRONT_TACH:
    case SMB_SENSOR_FAN5_REAR_TACH:
    case SMB_SENSOR_FAN6_FRONT_TACH:
    case SMB_SENSOR_FAN6_REAR_TACH:
      *value = 2;
      break;
    default:
      *value = 10;
      break;
  }
}

static void
psu_sensor_poll_interval(uint8_t sensor_num, uint32_t *value) {

  switch(sensor_num) {
    case PSU1_SENSOR_IN_VOLT:
    case PSU1_SENSOR_OUT_VOLT:
    case PSU1_SENSOR_IN_CURR:
    case PSU1_SENSOR_OUT_CURR:
    case PSU1_SENSOR_IN_POWER:
    case PSU1_SENSOR_OUT_POWER:
    case PSU1_SENSOR_FAN_TACH:
    case PSU1_SENSOR_TEMP1:
    case PSU1_SENSOR_TEMP2:
    case PSU2_SENSOR_IN_VOLT:
    case PSU2_SENSOR_OUT_VOLT:
    case PSU2_SENSOR_IN_CURR:
    case PSU2_SENSOR_OUT_CURR:
    case PSU2_SENSOR_IN_POWER:
    case PSU2_SENSOR_OUT_POWER:
    case PSU2_SENSOR_FAN_TACH:
    case PSU2_SENSOR_TEMP1:
    case PSU2_SENSOR_TEMP2:
      *value = 30;
      break;
    default:
      *value = 10;
      break;
  }
}

int
pal_get_sensor_poll_interval(uint8_t fru, uint8_t sensor_num, uint32_t *value) {

  switch(fru) {
    case FRU_SMB:
      smb_sensor_poll_interval(sensor_num, value);
      break;
    case FRU_PSU1:
    case FRU_PSU2:
      psu_sensor_poll_interval(sensor_num, value);
      break;
    default:
      *value = 2;
      break;
  }
  return 0;
}

void
pal_get_chassis_status(uint8_t slot, uint8_t *req_data, uint8_t *res_data, uint8_t *res_len) {

  char key[MAX_KEY_LEN];
  char buff[MAX_VALUE_LEN];
  int policy = 3;
  uint8_t status, ret;
  unsigned char *data = res_data;

  /* Platform Power Policy */
  sprintf(key, "%s", "server_por_cfg");
  if (pal_get_key_value(key, buff) == 0)
  {
    if (!memcmp(buff, "off", strlen("off")))
      policy = 0;
    else if (!memcmp(buff, "lps", strlen("lps")))
      policy = 1;
    else if (!memcmp(buff, "on", strlen("on")))
      policy = 2;
    else
      policy = 3;
  }

  /* Current Power State */
  ret = pal_get_server_power(0, &status);
  if (ret >= 0) {
    *data++ = status | (policy << 5);
  } else {
    /* load default */
    OBMC_WARN("ipmid: pal_get_server_power failed for server\n");
    *data++ = 0x00 | (policy << 5);
  }
  *data++ = 0x00;   /* Last Power Event */
  *data++ = 0x40;   /* Misc. Chassis Status */
  *data++ = 0x00;   /* Front Panel Button Disable */
  *res_len = data - res_data;
}

uint8_t
pal_set_power_restore_policy(uint8_t slot, uint8_t *pwr_policy, uint8_t *res_data) {

    uint8_t completion_code;
    char key[MAX_KEY_LEN];
    unsigned char policy = *pwr_policy & 0x07;  /* Power restore policy */

  completion_code = CC_SUCCESS;   /* Fill response with default values */
    sprintf(key, "%s", "server_por_cfg");
    switch (policy)
    {
      case 0:
        if (pal_set_key_value(key, "off") != 0)
          completion_code = CC_UNSPECIFIED_ERROR;
        break;
      case 1:
        if (pal_set_key_value(key, "lps") != 0)
          completion_code = CC_UNSPECIFIED_ERROR;
        break;
      case 2:
        if (pal_set_key_value(key, "on") != 0)
          completion_code = CC_UNSPECIFIED_ERROR;
        break;
      case 3:
        /* no change (just get present policy support) */
        break;
      default:
          completion_code = CC_PARAM_OUT_OF_RANGE;
        break;
    }
    return completion_code;
}

void *
generate_dump(void *arg) {

  uint8_t fru = *(uint8_t *) arg;
  char cmd[256];
  char fname[128];
  char fruname[16];

  // Usually the pthread cancel state are enable by default but
  // here we explicitly would like to enable them
  pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
  pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);

  pal_get_fru_name(fru, fruname);//scm

  memset(fname, 0, sizeof(fname));
  snprintf(fname, 128, "/var/run/autodump%d.pid", fru);
  if (access(fname, F_OK) == 0) {
    memset(cmd, 0, sizeof(cmd));
    snprintf(cmd,sizeof(cmd),"rm %s",fname);
    if (system(cmd)) {
      OBMC_CRIT("Removing old crashdump: %s failed!\n", fname);
    }
  }

  // Execute automatic crashdump
  memset(cmd, 0, 128);
  sprintf(cmd, "%s %s", CRASHDUMP_BIN, fruname);
  if (system(cmd)) {
    OBMC_CRIT("Crashdump for FRU: %d failed.", fru);
  } else {
    OBMC_CRIT("Crashdump for FRU: %d is generated.", fru);
  }

  t_dump[fru-1].is_running = 0;
  return 0;
}

int
pal_sel_handler(uint8_t fru, uint8_t snr_num, uint8_t *event_data) {

  char key[MAX_KEY_LEN] = {0};
  char cvalue[MAX_VALUE_LEN] = {0};

  switch(fru) {
    default:
      return -1;
  }

  /* Write the value "0" which means FRU_STATUS_BAD */
  return pal_set_key_value(key, cvalue);

}

void
init_led(void)
{
  int dev, ret;
  dev = open(LED_DEV, O_RDWR);
  if(dev < 0) {
    OBMC_ERROR(-1, "%s: open() failed\n", __func__);
    return;
  }

  ret = ioctl(dev, I2C_SLAVE, I2C_ADDR_SIM_LED);
  if(ret < 0) {
    OBMC_ERROR(-1, "%s: ioctl() assigned i2c addr failed\n", __func__);
    close(dev);
    return;
  }

  i2c_smbus_write_byte_data(dev, 0x06, 0x00);
  i2c_smbus_write_byte_data(dev, 0x07, 0x00);
  close(dev);

  return;
}

int
set_sled(int brd_rev, uint8_t color, int led_name)
{
  int dev, ret;
  uint8_t io0_reg = 0x02, io1_reg = 0x03;
  uint8_t clr_val, val_io0, val_io1;
  dev = open(LED_DEV, O_RDWR);
  if(dev < 0) {
    OBMC_ERROR(-1, "%s: open() failed\n", __func__);
    return -1;
  }

  ret = ioctl(dev, I2C_SLAVE, I2C_ADDR_SIM_LED);
  if(ret < 0) {
    OBMC_ERROR(-1, "%s: ioctl() assigned i2c addr 0x%x failed\n", __func__, I2C_ADDR_SIM_LED);
    close(dev);
    return -1;
  }
  val_io0 = i2c_smbus_read_byte_data(dev, 0x02);
  if(val_io0 < 0) {
    close(dev);
    OBMC_ERROR(-1, "%s: i2c_smbus_read_byte_data failed\n", __func__);
    return -1;
  }

  val_io1 = i2c_smbus_read_byte_data(dev, 0x03);
  if(val_io1 < 0) {
    close(dev);
    OBMC_ERROR(-1, "%s: i2c_smbus_read_byte_data failed\n", __func__);
    return -1;
  }

  clr_val = color;

  if(led_name == SLED_FAN || led_name == SLED_SMB) {
    clr_val = clr_val << 3;
    val_io0 = (val_io0 & 0x7) | clr_val;
    val_io1 = (val_io1 & 0x7) | clr_val;
  }
  else if(led_name == SLED_SYS || led_name == SLED_PSU) {
    val_io0 = (val_io0 & 0x38) | clr_val;
    val_io1 = (val_io1 & 0x38) | clr_val;
  }
  else {
    OBMC_WARN("%s: unknown led name\n", __func__);
  }

  if(led_name == SLED_SYS || led_name == SLED_FAN) {
    i2c_smbus_write_byte_data(dev, io0_reg, val_io0);
  } else {
    i2c_smbus_write_byte_data(dev, io1_reg, val_io1);
  }

  close(dev);
  return 0;
}

int
pal_mon_fw_upgrade
(int brd_rev, uint8_t *sys_ug, uint8_t *fan_ug,
              uint8_t *psu_ug, uint8_t *smb_ug)
{
  char cmd[5];
  FILE *fp;
  int ret=-1;
  char *buf_ptr;
  int buf_size = 1000;
  int str_size = 200;
  int tmp_size;
  char str[200];
  snprintf(cmd, sizeof(cmd), "ps w");
  fp = popen(cmd, "r");
  if(NULL == fp)
     return -1;

  buf_ptr = (char *)malloc(buf_size * sizeof(char) + sizeof(char));
  memset(buf_ptr, 0, sizeof(char));
  tmp_size = str_size;
  while(fgets(str, str_size, fp) != NULL) {
    tmp_size = tmp_size + str_size;
    if(tmp_size + str_size >= buf_size) {
      buf_ptr = realloc(buf_ptr, sizeof(char) * buf_size * 2 + sizeof(char));
      buf_size *= 2;
    }
    if(!buf_ptr) {
      OBMC_ERROR(-1,
             "%s realloc() fail, please check memory remaining", __func__);
      goto free_buf;
    }
    strncat(buf_ptr, str, str_size);
  }

  //check whether sys led need to blink
  *sys_ug = strstr(buf_ptr, "write spi2") != NULL ? 1 : 0;
  if(*sys_ug) goto fan_state;

  *sys_ug = strstr(buf_ptr, "write spi1 BACKUP_BIOS") != NULL ? 1 : 0;
  if(*sys_ug) goto fan_state;

  *sys_ug = (strstr(buf_ptr, "scmcpld_update") != NULL) ? 1 : 0;
  if(*sys_ug) goto fan_state;

  *sys_ug = (strstr(buf_ptr, "fw-util") != NULL) ?
          ((strstr(buf_ptr, "--update") != NULL) ? 1 : 0) : 0;
  if(*sys_ug) goto fan_state;

  //check whether fan led need to blink
fan_state:
  *fan_ug = (strstr(buf_ptr, "fcmcpld_update") != NULL) ? 1 : 0;

  //check whether fan led need to blink
  *psu_ug = (strstr(buf_ptr, "psu-util") != NULL) ?
          ((strstr(buf_ptr, "--update") != NULL) ? 1 : 0) : 0;

  //check whether smb led need to blink
  *smb_ug = (strstr(buf_ptr, "smbcpld_update") != NULL) ? 1 : 0;
  if(*smb_ug) goto close_fp;

  *smb_ug = (strstr(buf_ptr, "pwrcpld_update") != NULL) ? 1 : 0;
  if(*smb_ug) goto close_fp;

  *smb_ug = (strstr(buf_ptr, "flashcp") != NULL) ? 1 : 0;
  if(*smb_ug) goto close_fp;

  *smb_ug = strstr(buf_ptr, "write spi1 DOM_FPGA_FLASH") != NULL ? 1 : 0;
  if(*smb_ug) goto close_fp;

  *smb_ug = strstr(buf_ptr, "write spi1 TH3_PCIE_FLASH") != NULL ? 1 : 0;
  if(*smb_ug) goto close_fp;

  *smb_ug = strstr(buf_ptr, "write spi1 GB_PCIE_FLASH") != NULL ? 1 : 0;
  if(*smb_ug) goto close_fp;

  *smb_ug = strstr(buf_ptr, "write spi1 BCM5389_EE") != NULL ? 1 : 0;
  if(*smb_ug) goto close_fp;

close_fp:
  ret = pclose(fp);
  if(-1 == ret)
     OBMC_ERROR(-1, "%s pclose() fail ", __func__);

free_buf:
  free(buf_ptr);
  return 0;
}

void set_sys_led(int brd_rev)
{
  uint8_t ret = 0;
  uint8_t prsnt = 0;
  uint8_t sys_ug = 0, fan_ug = 0, psu_ug = 0, smb_ug = 0;
  static uint8_t alter_sys = 0;

  ret = pal_is_fru_prsnt(FRU_SMB, &prsnt);

  if (ret) {
    set_sled(brd_rev, SLED_CLR_YELLOW, SLED_SYS);
    OBMC_WARN("%s: can't get SCM status\n",__func__);
    return;
  }
  if (!prsnt) {
    set_sled(brd_rev, SLED_CLR_YELLOW, SLED_SYS);
    OBMC_WARN("%s: SCM isn't presence\n",__func__);
    return;
  }
  pal_mon_fw_upgrade(brd_rev, &sys_ug, &fan_ug, &psu_ug, &smb_ug);
  if( sys_ug || fan_ug || psu_ug || smb_ug ){
    OBMC_WARN("firmware upgrading in progress\n");
    if(alter_sys==0){
      alter_sys = 1;
      set_sled(brd_rev, SLED_CLR_YELLOW, SLED_SYS);
      return;
    }else{
      alter_sys = 0;
      set_sled(brd_rev, SLED_CLR_BLUE, SLED_SYS);
      return;
    }
  }
  set_sled(brd_rev, SLED_CLR_BLUE, SLED_SYS);
  return;
}

void set_fan_led(int brd_rev)
{
  int i;
  float value, unc, lcr;
  uint8_t prsnt;
  uint8_t fan_num = MAX_NUM_FAN * 2;//rear & front: MAX_NUM_FAN * 2
  char path[LARGEST_DEVICE_NAME + 1];
  char sensor_name[LARGEST_DEVICE_NAME];
  int sensor_num[] = { SMB_SENSOR_FAN1_FRONT_TACH,
                       SMB_SENSOR_FAN1_REAR_TACH ,
                       SMB_SENSOR_FAN2_FRONT_TACH,
                       SMB_SENSOR_FAN2_REAR_TACH ,
                       SMB_SENSOR_FAN3_FRONT_TACH,
                       SMB_SENSOR_FAN3_REAR_TACH ,
                       SMB_SENSOR_FAN4_FRONT_TACH,
                       SMB_SENSOR_FAN4_REAR_TACH };

  for(i = FRU_FAN1; i <= FRU_FAN6; i++) {
    pal_is_fru_prsnt(i, &prsnt);
#ifdef DEBUG
    OBMC_INFO("fan %d : %d\n",i,prsnt);
#endif
    if(!prsnt) {
      OBMC_WARN("%s: fan %d isn't presence\n",__func__,i-FRU_FAN1+1);
      goto cleanup;
    }
  }
  for(i = 0; i < fan_num; i++) {
    if(smb_sensor_read(sensor_num[i],&value)) {
      OBMC_WARN("%s: can't access %s\n",__func__,path);
      goto cleanup;
    }

    pal_get_sensor_threshold(FRU_SMB, sensor_num[i], UCR_THRESH, &unc);
    pal_get_sensor_threshold(FRU_SMB, sensor_num[i], LCR_THRESH, &lcr);
#ifdef DEBUG
    OBMC_INFO("fan %d\n",i);
    OBMC_INFO(" val %d\n",value);
    OBMC_INFO(" ucr %f\n",smb_sensor_threshold[sensor_num[i]][UCR_THRESH]);
    OBMC_INFO(" lcr %f\n",smb_sensor_threshold[sensor_num[i]][LCR_THRESH]);
#endif
    if(value == 0) {
      OBMC_WARN("%s: can't access %s\n",__func__,path);
      goto cleanup;
    }
    else if(value > unc){
      pal_get_sensor_name(FRU_SMB,sensor_num[i],sensor_name);
      OBMC_WARN("%s: %s value is over than UNC ( %.2f > %.2f )\n",
      __func__,sensor_name,value,unc);
      goto cleanup;
    }
    else if(value < lcr){
      pal_get_sensor_name(FRU_SMB,sensor_num[i],sensor_name);
      OBMC_WARN("%s: %s value is under than LCR ( %.2f < %.2f )\n",
      __func__,sensor_name,value,lcr);
      goto cleanup;
    }
  }
  set_sled(brd_rev, SLED_CLR_BLUE, SLED_FAN);
  sleep(LED_INTERVAL);
  return;
cleanup:
  set_sled(brd_rev, SLED_CLR_YELLOW, SLED_FAN);
  sleep(LED_INTERVAL);
  return;
}
#if DELTA_SKIP
void set_psu_led(int brd_rev)
{
  int i;
  float value,ucr,lcr;
  uint8_t prsnt,pem_prsnt,fru,ready[4] = { 0 };
  char path[LARGEST_DEVICE_NAME + 1];
  int psu1_sensor_num[] = { PSU1_SENSOR_IN_VOLT,
                            PSU1_SENSOR_OUT_VOLT,};
  int psu2_sensor_num[] = { PSU2_SENSOR_IN_VOLT,
                            PSU2_SENSOR_OUT_VOLT,};
  int pem1_sensor_num[] = { PEM1_SENSOR_IN_VOLT,
                            PEM1_SENSOR_OUT_VOLT};
  int pem2_sensor_num[] = { PEM2_SENSOR_IN_VOLT,
                            PEM2_SENSOR_OUT_VOLT};
  char sensor_name[LARGEST_DEVICE_NAME];
  int *sensor_num,sensor_cnt;
  char pem1_sensor_cnt,pem2_sensor_cnt;
  char psu1_sensor_cnt,psu2_sensor_cnt;
  pem1_sensor_cnt = sizeof(pem1_sensor_num)/sizeof(pem1_sensor_num[0]);
  pem2_sensor_cnt = sizeof(pem2_sensor_num)/sizeof(pem2_sensor_num[0]);
  psu1_sensor_cnt = sizeof(psu1_sensor_num)/sizeof(psu1_sensor_num[0]);
  psu2_sensor_cnt = sizeof(psu2_sensor_num)/sizeof(psu2_sensor_num[0]);

  pem_prsnt = 0;
  for( fru = FRU_PEM1 ; fru <= FRU_PSU2 ; fru++){
    switch(fru){
      case FRU_PEM1:
        sensor_cnt = pem1_sensor_cnt;
        sensor_num = pem1_sensor_num;  break;
      case FRU_PEM2:
        sensor_cnt = pem2_sensor_cnt;
        sensor_num = pem2_sensor_num;  break;
      case FRU_PSU1:
        sensor_cnt = psu1_sensor_cnt;
        sensor_num = psu1_sensor_num;  break;
      case FRU_PSU2:
        sensor_cnt = psu2_sensor_cnt;
        sensor_num = psu2_sensor_num;  break;
      default :
        OBMC_WARN("%s: skip with invalid fru %d\n",__func__,fru);
        continue;
    }

    pal_is_fru_prsnt(fru,&prsnt);
    if(fru == FRU_PEM1 || fru == FRU_PEM2){    // check PEM present.
      if(prsnt){
        pem_prsnt = prsnt;                     // if PEM present,no need to check PSU.
      }else{
        pem_prsnt = 0;
        continue;                              // if PEM not present, continue to check
      }
    }else if(fru == FRU_PSU1 || fru == FRU_PSU2){
      if(!prsnt){            // PSU not present then check PEM present.
        if(!pem_prsnt)       // PEM not present.
        {
          OBMC_WARN("%s: PSU%d isn't presence\n",__func__,fru-FRU_PSU1+1);
          goto cleanup;
        }
        else{               // PEM present check another PSU or PEM.
          continue;
        }
      }
    }
    pal_is_fru_ready(fru,&ready[fru-FRU_PEM1]);
    if(!ready[fru-FRU_PEM1]){
      if(fru >= FRU_PSU1 && !ready[fru-2]){     // if PSU and PEM aren't ready both
        OBMC_WARN("%s: PSU%d and PEM%d can't access\n",
        __func__,fru-FRU_PSU1+1,fru-FRU_PSU1+1);
        goto cleanup;
      }
      continue;
    }

    for(i = 0; i < sensor_cnt ; i++) {
      pal_get_sensor_threshold(fru, sensor_num[i], UCR_THRESH, &ucr);
      pal_get_sensor_threshold(fru, sensor_num[i], LCR_THRESH, &lcr);
      pal_get_sensor_name(fru, sensor_num[i],sensor_name);

      if(fru >= FRU_PEM1 && fru <= FRU_PEM2){
        if(pem_sensor_read(sensor_num[i], &value)) {
          OBMC_WARN("%s: fru %d sensor id %d (%s) can't access %s\n",
          __func__,fru,sensor_num[i],sensor_name,path);
          goto cleanup;
        }
      }else if(fru >= FRU_PSU1 && fru <= FRU_PSU2){
        if(psu_sensor_read(sensor_num[i], &value)) {
          OBMC_WARN("%s: fru %d sensor id %d (%s) can't access %s\n",
          __func__,fru,sensor_num[i],sensor_name,path);
          goto cleanup;
        }
      }

      if(value > ucr) {
        OBMC_WARN("%s: %s value is over than UCR ( %.2f > %.2f )\n",
        __func__,sensor_name,value,ucr);
        goto cleanup;
      }else if(value < lcr){
        OBMC_WARN("%s: %s value is under than LCR ( %.2f < %.2f )\n",
        __func__,sensor_name,value,lcr);
        goto cleanup;
      }
    }
  }
  set_sled(brd_rev, SLED_CLR_BLUE, SLED_PSU);
  sleep(LED_INTERVAL);
  return;

cleanup:
  set_sled(brd_rev, SLED_CLR_YELLOW, SLED_PSU);
  sleep(LED_INTERVAL);
  return;
}


void set_scm_led(int brd_rev)
{
  #define SCM_IP_USB "fe80::ff:fe00:2%%usb0"
  char path[LARGEST_DEVICE_NAME];
  char cmd[64];
  char buffer[256];
  FILE *fp;
  int power;
  int ret;

  snprintf(path, LARGEST_DEVICE_NAME, GPIO_COME_PWRGD, "value");
  if (read_device(path, &power)) {
    OBMC_WARN("%s: can't get GPIO value '%s'",__func__,path);
    return;
  }

  if(power){
    // -c count = 1 times
    // -W timeout = 1 secound
    sprintf(cmd,"ping -c 1 -W 1 "SCM_IP_USB);
    fp = popen(cmd,"r");
    if (!fp) {
      OBMC_WARN("%s: can't run cmd '%s'",__func__,cmd);
      goto cleanup;
    }

    while (fgets(buffer,256,fp) != NULL){
    }

    ret = pclose(fp);
    if(ret == 0){ // PING OK
      set_sled(brd_rev,SLED_CLR_GREEN,SLED_SMB);
      return;
    }else{
      OBMC_WARN("%s: can't ping to "SCM_IP_USB,__func__);
      goto cleanup;
    }
  }else{
    OBMC_WARN("%s: micro server is power off\n",__func__);
  }
cleanup:
  set_sled(brd_rev,SLED_CLR_YELLOW,SLED_SMB);
  sleep(LED_INTERVAL);
  return;
}

int
pal_light_scm_led(uint8_t led_color)
{
  char path[64];
  int ret;
  char *val;
  sprintf(path, SCM_SYSFS, SYS_LED_COLOR);
  if(led_color == SCM_LED_BLUE)
    val = "0";
  else
    val = "1";
  ret = write_device(path, val);
  if (ret) {
#ifdef DEBUG
  OBMC_WARN("write_device failed for %s\n", path);
#endif
    return -1;
  }

  return 0;
}

#endif

int
pal_set_def_key_value(void) {

  int i, ret;
  char path[LARGEST_DEVICE_NAME + 1];

  for (i = 0; strcmp(key_list[i], LAST_KEY) != 0; i++) {
    snprintf(path, LARGEST_DEVICE_NAME, KV_PATH, key_list[i]);
    if ((ret = kv_set(key_list[i], def_val_list[i],
                    0, KV_FPERSIST | KV_FCREATE)) < 0) {
#ifdef DEBUG
      OBMC_WARN("pal_set_def_key_value: kv_set failed. %d", ret);
#endif
    }
  }
  return 0;
 }

int
pal_init_sensor_check(uint8_t fru, uint8_t snr_num, void *snr) {
  pal_set_def_key_value();
  return 0;
}

int
pal_get_fru_health(uint8_t fru, uint8_t *value) {

  char cvalue[MAX_VALUE_LEN] = {0};
  char key[MAX_KEY_LEN] = {0};
  int ret;

  switch(fru) {
    case FRU_SMB:
      sprintf(key, "smb_sensor_health");
      break;
    case FRU_FCM:
      sprintf(key, "fcm_sensor_health");
      break;

    case FRU_PSU1:
        sprintf(key, "psu1_sensor_health");
        break;
    case FRU_PSU2:
        sprintf(key, "psu2_sensor_health");
        break;

    case FRU_FAN1:
        sprintf(key, "fan1_sensor_health");
        break;
    case FRU_FAN2:
        sprintf(key, "fan2_sensor_health");
        break;
    case FRU_FAN3:
        sprintf(key, "fan3_sensor_health");
        break;
    case FRU_FAN4:
        sprintf(key, "fan4_sensor_health");
        break;
    case FRU_FAN5:
        sprintf(key, "fan5_sensor_health");
        break;
    case FRU_FAN6:
        sprintf(key, "fan6_sensor_health");
        break;

    default:
      return -1;
  }

  ret = pal_get_key_value(key, cvalue);
  if (ret) {
    return ret;
  }

  *value = atoi(cvalue);
  *value = *value & atoi(cvalue);
  return 0;
}

int
pal_set_sensor_health(uint8_t fru, uint8_t value) {

  char key[MAX_KEY_LEN] = {0};
  char cvalue[MAX_VALUE_LEN] = {0};

  switch(fru) {
    case FRU_SMB:
      sprintf(key, "smb_sensor_health");
      break;
    case FRU_PSU1:
      sprintf(key, "psu1_sensor_health");
      break;
    case FRU_PSU2:
      sprintf(key, "psu2_sensor_health");
      break;
    case FRU_FAN1:
      sprintf(key, "fan1_sensor_health");
      break;
    case FRU_FAN2:
      sprintf(key, "fan2_sensor_health");
      break;
    case FRU_FAN3:
      sprintf(key, "fan3_sensor_health");
      break;
    case FRU_FAN4:
      sprintf(key, "fan4_sensor_health");
      break;
    case FRU_FAN5:
      sprintf(key, "fan5_sensor_health");
      break;
    case FRU_FAN6:
      sprintf(key, "fan6_sensor_health");
      break;
    default:
      return -1;
  }

  sprintf(cvalue, (value > 0) ? "1": "0");

  return pal_set_key_value(key, cvalue);
}

int
pal_parse_sel(uint8_t fru, uint8_t *sel, char *error_log)
{
  uint8_t snr_num = sel[11];
  uint8_t *event_data = &sel[10];
  uint8_t *ed = &event_data[3];
  char temp_log[512] = {0};
  uint8_t sen_type = event_data[0];
  uint8_t chn_num, dimm_num;
  bool parsed = false;

  switch(snr_num) {
    case MEMORY_ECC_ERR:
    case MEMORY_ERR_LOG_DIS:
      strcpy(error_log, "");
      if (snr_num == MEMORY_ECC_ERR) {
        // SEL from MEMORY_ECC_ERR Sensor
        if ((ed[0] & 0x0F) == 0x0) {
          if (sen_type == 0x0C) {
            strcat(error_log, "Correctable");
            sprintf(temp_log, "DIMM%02X ECC err", ed[2]);
            pal_add_cri_sel(temp_log);
          } else if (sen_type == 0x10)
            strcat(error_log, "Correctable ECC error Logging Disabled");
        } else if ((ed[0] & 0x0F) == 0x1) {
          strcat(error_log, "Uncorrectable");
          sprintf(temp_log, "DIMM%02X UECC err", ed[2]);
          pal_add_cri_sel(temp_log);
        } else if ((ed[0] & 0x0F) == 0x5)
          strcat(error_log, "Correctable ECC error Logging Limit Reached");
        else
          strcat(error_log, "Unknown");
      } else {
        // SEL from MEMORY_ERR_LOG_DIS Sensor
        if ((ed[0] & 0x0F) == 0x0)
          strcat(error_log, "Correctable Memory Error Logging Disabled");
        else
          strcat(error_log, "Unknown");
      }

      // DIMM number (ed[2]):
      // Bit[7:5]: Socket number  (Range: 0-7)
      // Bit[4:3]: Channel number (Range: 0-3)
      // Bit[2:0]: DIMM number    (Range: 0-7)
      if (((ed[1] & 0xC) >> 2) == 0x0) {
        /* All Info Valid */
        chn_num = (ed[2] & 0x18) >> 3;
        dimm_num = ed[2] & 0x7;

        /* If critical SEL logging is available, do it */
        if (sen_type == 0x0C) {
          if ((ed[0] & 0x0F) == 0x0) {
            sprintf(temp_log, "DIMM%c%d ECC err,FRU:%u", 'A'+chn_num,
                    dimm_num, fru);
            pal_add_cri_sel(temp_log);
          } else if ((ed[0] & 0x0F) == 0x1) {
            sprintf(temp_log, "DIMM%c%d UECC err,FRU:%u", 'A'+chn_num,
                    dimm_num, fru);
            pal_add_cri_sel(temp_log);
          }
        }
        /* Then continue parse the error into a string. */
        /* All Info Valid                               */
        sprintf(temp_log, " DIMM %c%d Logical Rank %d (CPU# %d, CHN# %d, DIMM# %d)",
            'A'+chn_num, dimm_num, ed[1] & 0x03, (ed[2] & 0xE0) >> 5, chn_num, dimm_num);
      } else if (((ed[1] & 0xC) >> 2) == 0x1) {
        /* DIMM info not valid */
        sprintf(temp_log, " (CPU# %d, CHN# %d)",
            (ed[2] & 0xE0) >> 5, (ed[2] & 0x18) >> 3);
      } else if (((ed[1] & 0xC) >> 2) == 0x2) {
        /* CHN info not valid */
        sprintf(temp_log, " (CPU# %d, DIMM# %d)",
            (ed[2] & 0xE0) >> 5, ed[2] & 0x7);
      } else if (((ed[1] & 0xC) >> 2) == 0x3) {
        /* CPU info not valid */
        sprintf(temp_log, " (CHN# %d, DIMM# %d)",
            (ed[2] & 0x18) >> 3, ed[2] & 0x7);
      }
      strcat(error_log, temp_log);
      parsed = true;
      break;
  }

  if (parsed == true) {
    if ((event_data[2] & 0x80) == 0) {
      strcat(error_log, " Assertion");
    } else {
      strcat(error_log, " Deassertion");
    }
    return 0;
  }

  pal_parse_sel_helper(fru, sel, error_log);

  return 0;
}

int
agc032a_sensor_name(uint8_t fru, uint8_t sensor_num, char *name) {
  return 0;
}

int
pal_get_event_sensor_name(uint8_t fru, uint8_t *sel, char *name) {
  uint8_t snr_type = sel[10];
  uint8_t snr_num = sel[11];

  // If SNR_TYPE is OS_BOOT, sensor name is OS
  switch (snr_type) {
    case OS_BOOT:
      // OS_BOOT used by OS
      sprintf(name, "OS");
      return 0;
    default:
      if (agc032a_sensor_name(fru, snr_num, name) != 0) {
        break;
      }
      return 0;
  }
  // Otherwise, translate it based on snr_num
  return pal_get_x86_event_sensor_name(fru, snr_num, name);
}

// Write GUID into EEPROM
static int
pal_set_guid(uint16_t offset, char *guid) {
  int fd = 0;
  ssize_t bytes_wr;
  char eeprom_path[FBW_EEPROM_PATH_SIZE];
  errno = 0;

  wedge_eeprom_path(eeprom_path);

  // Check for file presence
  if (access(eeprom_path, F_OK) == -1) {
      OBMC_ERROR(-1, "pal_set_guid: unable to access the %s file: %s",
          eeprom_path, strerror(errno));
      return errno;
  }

  // Open file
  fd = open(eeprom_path, O_WRONLY);
  if (fd == -1) {
    OBMC_ERROR(-1, "pal_set_guid: unable to open the %s file: %s",
        eeprom_path, strerror(errno));
    return errno;
  }

  // Seek the offset
  lseek(fd, offset, SEEK_SET);

  // Write GUID data
  bytes_wr = write(fd, guid, GUID_SIZE);
  if (bytes_wr != GUID_SIZE) {
    OBMC_ERROR(-1, "pal_set_guid: write to %s file failed: %s",
        eeprom_path, strerror(errno));
    goto err_exit;
  }

err_exit:
  close(fd);
  return errno;
}

// Read GUID from EEPROM
static int
pal_get_guid(uint16_t offset, char *guid) {
  int fd = 0;
  ssize_t bytes_rd;
  char eeprom_path[FBW_EEPROM_PATH_SIZE];
  errno = 0;

  wedge_eeprom_path(eeprom_path);

  // Check if file is present
  if (access(eeprom_path, F_OK) == -1) {
      OBMC_ERROR(-1, "pal_get_guid: unable to access the %s file: %s",
          eeprom_path, strerror(errno));
      return errno;
  }

  // Open the file
  fd = open(eeprom_path, O_RDONLY);
  if (fd == -1) {
    OBMC_ERROR(-1, "pal_get_guid: unable to open the %s file: %s",
        eeprom_path, strerror(errno));
    return errno;
  }

  // seek to the offset
  lseek(fd, offset, SEEK_SET);

  // Read bytes from location
  bytes_rd = read(fd, guid, GUID_SIZE);
  if (bytes_rd != GUID_SIZE) {
    OBMC_ERROR(-1, "pal_get_guid: read to %s file failed: %s",
        eeprom_path, strerror(errno));
    goto err_exit;
  }

err_exit:
  close(fd);
  return errno;
}

// GUID based on RFC4122 format @ https://tools.ietf.org/html/rfc4122
static void
pal_populate_guid(uint8_t *guid, char *str) {
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
  //getrandom(&guid[8], 2, 0);
  srand(time(NULL));
  //memcpy(&guid[8], rand(), 2);
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

}

int
pal_get_dev_guid(uint8_t fru, char *guid) {

  pal_get_guid(OFFSET_DEV_GUID, (char *)g_dev_guid);
  memcpy(guid, g_dev_guid, GUID_SIZE);

  return 0;
}

int
pal_set_dev_guid(uint8_t slot, char *guid) {

  pal_populate_guid(g_dev_guid, guid);

  return pal_set_guid(OFFSET_DEV_GUID, (char *)g_dev_guid);
}

int
pal_get_boot_order(uint8_t slot, uint8_t *req_data, uint8_t *boot, uint8_t *res_len) {
  int i;
  int j = 0;
  int ret;
  int msb, lsb;
  char key[MAX_KEY_LEN] = {0};
  char str[MAX_VALUE_LEN] = {0};
  char tstr[4] = {0};

  sprintf(key, "slot%d_boot_order", slot);
  ret = pal_get_key_value(key, str);
  if (ret) {
    *res_len = 0;
    return ret;
  }

  for (i = 0; i < 2*SIZE_BOOT_ORDER; i += 2) {
    sprintf(tstr, "%c\n", str[i]);
    msb = strtol(tstr, NULL, 16);

    sprintf(tstr, "%c\n", str[i+1]);
    lsb = strtol(tstr, NULL, 16);
    boot[j++] = (msb << 4) | lsb;
  }

  *res_len = SIZE_BOOT_ORDER;
  return 0;
}

int
pal_set_boot_order(uint8_t slot, uint8_t *boot, uint8_t *res_data, uint8_t *res_len) {
  int i, j, offset, network_dev = 0;
  char str[MAX_VALUE_LEN] = {0};
  enum {
    BOOT_DEVICE_IPV4 = 0x1,
    BOOT_DEVICE_IPV6 = 0x9,
  };

  *res_len = 0;

  for (i = offset = 0; i < SIZE_BOOT_ORDER && offset < sizeof(str); i++) {
    if (i > 0) {  // byte[0] is boot mode, byte[1:5] are boot order
      for (j = i+1; j < SIZE_BOOT_ORDER; j++) {
        if (boot[i] == boot[j])
          return CC_INVALID_PARAM;
      }

      // If bit[2:0] is 001b (Network), bit[3] is IPv4/IPv6 order
      // bit[3]=0b: IPv4 first
      // bit[3]=1b: IPv6 first
      if ((boot[i] == BOOT_DEVICE_IPV4) || (boot[i] == BOOT_DEVICE_IPV6))
        network_dev++;
    }

    offset += snprintf(str + offset, sizeof(str) - offset, "%02x", boot[i]);
  }

  // not allow having more than 1 network boot device in the boot order
  if (network_dev > 1)
    return CC_INVALID_PARAM;

  return pal_set_key_value("server_boot_order", str);

}

int pal_get_bmc_ipmb_slave_addr(uint16_t *slave_addr, uint8_t bus_id)
{
  *slave_addr = 0x10;
  return 0;
}

int
pal_ipmb_processing(int bus, void *buf, uint16_t size) {
  char key[MAX_KEY_LEN];
  char value[MAX_VALUE_LEN];
  struct timespec ts;
  static time_t last_time = 0;
  if ((bus == 4) && (((uint8_t *)buf)[0] == 0x20)) {  // OCP LCD debug card
    clock_gettime(CLOCK_MONOTONIC, &ts);
    if (ts.tv_sec >= (last_time + 5)) {
      last_time = ts.tv_sec;
      ts.tv_sec += 30;

      snprintf(key, sizeof(key), "ocpdbg_lcd");
      snprintf(value, sizeof(value), "%ld", ts.tv_sec);
      if (kv_set(key, value, 0, 0) < 0) {
        return -1;
      }
    }
  }
  return 0;
}

bool
pal_is_mcu_working(void) {
  char key[MAX_KEY_LEN] = {0};
  char value[MAX_VALUE_LEN] = {0};
  struct timespec ts;

  snprintf(key, sizeof(key), "ocpdbg_lcd");
  if (kv_get(key, value, NULL, 0)) {
     return false;
  }

  clock_gettime(CLOCK_MONOTONIC, &ts);
  if (strtoul(value, NULL, 10) > ts.tv_sec) {
     return true;
  }

  return false;
}

/* Add button function for Debug Card */
/* Read the Front Panel Hand Switch and return the position */
int
pal_get_hand_sw_physically(uint8_t *pos) {
  char path[LARGEST_DEVICE_NAME + 1];
  int loc;

  snprintf(path, LARGEST_DEVICE_NAME, BMC_UART_SEL);
  if (read_device(path, &loc)) {
    return -1;
  }
  *pos = loc;

  return 0;
}

int
pal_get_hand_sw(uint8_t *pos) {
  char value[MAX_VALUE_LEN];
  uint8_t loc;
  int ret;

  ret = kv_get("scm_hand_sw", value, NULL, 0);
  if (!ret) {
    loc = atoi(value);
    if (loc > HAND_SW_BMC) {
      return pal_get_hand_sw_physically(pos);
    }
    *pos = loc;

    return 0;
  }

  return pal_get_hand_sw_physically(pos);
}

/* Return the Front Panel Power Button */
int
pal_get_dbg_pwr_btn(uint8_t *status) {
   char cmd[MAX_KEY_LEN + 32] = {0};
  char value[MAX_VALUE_LEN];
  char *p;
  FILE *fp;
  int val = 0;

  sprintf(cmd, "/usr/sbin/i2cget -f -y 4 0x27 1");
  fp = popen(cmd, "r");
  if (!fp) {
    return -1;
  }

  if (fgets(value, MAX_VALUE_LEN, fp) == NULL) {
    pclose(fp);
    return -1;
  }

  for (p = value; *p != '\0'; p++) {
    if (*p == '\n' || *p == '\r') {
      *p = '\0';
      break;
    }
  }

  sscanf(value, "%x", &val);
  if ((!(val & 0x2)) && (val & 0x1)) {
    *status = 1;      /* PWR BTN status pressed */
    syslog(LOG_WARNING, "%s PWR pressed  0x%x\n", __FUNCTION__, val);
  } else {
    *status = 0;      /* PWR BTN status cleared */
  }
  pclose(fp);
  return 0;
}

/* Return the Debug Card Reset Button status */
int
pal_get_dbg_rst_btn(uint8_t *status) {
  char cmd[MAX_KEY_LEN + 32] = {0};
  char value[MAX_VALUE_LEN];
  char *p;
  FILE *fp;
  int val = 0;

  sprintf(cmd, "/usr/sbin/i2cget -f -y 4 0x27 1");
  fp = popen(cmd, "r");
  if (!fp) {
    return -1;
  }

  if (fgets(value, MAX_VALUE_LEN, fp) == NULL) {
    pclose(fp);
    return -1;
  }

  for (p = value; *p != '\0'; p++) {
    if (*p == '\n' || *p == '\r') {
      *p = '\0';
      break;
    }
  }

  sscanf(value, "%x", &val);
  if ((!(val & 0x1)) && (val & 0x2)) {
    *status = 1;      /* RST BTN status pressed */
  } else {
    *status = 0;      /* RST BTN status cleared */
  }
  pclose(fp);
  return 0;
}

/* Update the Reset button input to the server at given slot */
int
pal_set_rst_btn(uint8_t slot, uint8_t status) {
  char *val;
  char path[64];
  int ret;

  if (status) {
    val = "1";
  } else {
    val = "0";
  }

  sprintf(path, SCM_SYSFS, SCM_COM_RST_BTN);

  ret = write_device(path, val);
  if (ret) {
    return -1;
  }
  return 0;
}

/* Return the Debug Card UART Sel Button status */
int
pal_get_dbg_uart_btn(uint8_t *status) {
  char cmd[MAX_KEY_LEN + 32] = {0};
  char value[MAX_VALUE_LEN];
  char *p;
  FILE *fp;
  int val = 0;

  sprintf(cmd, "/usr/sbin/i2cget -f -y 4 0x27 3");
  fp = popen(cmd, "r");
  if (!fp) {
    return -1;
  }

  if (fgets(value, MAX_VALUE_LEN, fp) == NULL) {
    pclose(fp);
    return -1;
  }
  pclose(fp);
  for (p = value; *p != '\0'; p++) {
    if (*p == '\n' || *p == '\r') {
      *p = '\0';
      break;
    }
  }
  sscanf(value, "%x", &val);
  if (!(val & 0x80)) {
    *status = 1;      /* UART BTN status pressed */
  } else {
    *status = 0;      /* UART BTN status cleared */
  }
  return 0;
}

/* Clear Debug Card UART Sel Button status */
int
pal_clr_dbg_uart_btn() {
  int ret;
  ret = run_command("/usr/sbin/i2cset -f -y 4 0x27 3 0xff");
  if (ret)
    return -1;
  return 0;
}

/* Switch the UART mux to userver or BMC */
int
pal_switch_uart_mux() {
  char path[LARGEST_DEVICE_NAME + 1];
  char *val;
  int loc;
  /* Refer the UART select status */
  snprintf(path, LARGEST_DEVICE_NAME, BMC_UART_SEL);
  if (read_device(path, &loc)) {
    return -1;
  }

  if (loc == 3) {
    val = "2";
  } else {
    val = "3";
  }

  if (write_device(path, val)) {
    return -1;
  }
  return 0;
}
