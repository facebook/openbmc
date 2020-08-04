/*
 *
 * Copyright 2020-present Facebook. All Rights Reserved.
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

//  #define DEBUG

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <syslog.h>
#include <time.h>
#include <unistd.h>
#include <pthread.h>
#include <ctype.h>
#include <linux/limits.h>
#include <linux/version.h>
#include "pal.h"
#include <math.h>
#include <facebook/bic.h>
#include <openbmc/kv.h>
#include <openbmc/obmc-i2c.h>
#include <openbmc/sensor-correction.h>
#include <openbmc/misc-utils.h>
#include <openbmc/log.h>

#define RUN_SHELL_CMD(_cmd)                              \
  do {                                                   \
    int _ret = system(_cmd);                             \
    if (_ret != 0)                                       \
      OBMC_WARN("'%s' command returned %d", _cmd, _ret); \
  } while (0)

struct threadinfo {
  uint8_t is_running;
  uint8_t fru;
  pthread_t pt;
};

static struct threadinfo t_dump[MAX_NUM_FRUS] = {0, };

const char pal_fru_list[] = "all, scm, smb, pim1, pim2, pim3, \
pim4, pim5, pim6, pim7, pim8, psu1, psu2, psu3, psu4";

char * key_list[] = {
"pwr_server_last_state",
"sysfw_ver_server",
"timestamp_sled",
"server_por_cfg",
"server_sel_error",
"scm_sensor_health",
"smb_sensor_health",
"pim1_sensor_health",
"pim2_sensor_health",
"pim3_sensor_health",
"pim4_sensor_health",
"pim5_sensor_health",
"pim6_sensor_health",
"pim7_sensor_health",
"pim8_sensor_health",
"psu1_sensor_health",
"psu2_sensor_health",
"psu3_sensor_health",
"psu4_sensor_health",
"server_boot_order",
"server_restart_cause",
"server_cpu_ppin",
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
  "1", /* pim1_sensor_health */
  "1", /* pim2_sensor_health */
  "1", /* pim3_sensor_health */
  "1", /* pim4_sensor_health */
  "1", /* pim5_sensor_health */
  "1", /* pim6_sensor_health */
  "1", /* pim7_sensor_health */
  "1", /* pim8_sensor_health */
  "1", /* psu1_sensor_health */
  "1", /* psu2_sensor_health */
  "1", /* psu3_sensor_health */
  "1", /* psu4_sensor_health */
  "0000000", /* scm_boot_order */
  "3", /* scm_restart_cause */
  "0", /* scm_cpu_ppin */
  /* Add more def values for the correspoding keys*/
  LAST_KEY /* Same as last entry of the key_list */
};

sensor_info_t g_sinfo[MAX_NUM_FRUS][MAX_SENSOR_NUM] = {0};

// Helper Functions
int read_device(const char *device, int *value) {
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

  rc = fscanf(fp, "%i", value);
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

// Helper Functions
static int
read_device_float(const char *device, float *value) {
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

  rc = fscanf(fp, "%f", value);
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

int
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

int
pal_detect_i2c_device(uint8_t bus, uint8_t addr, uint8_t mode, uint8_t force) {

  int fd = -1, rc = -1;
  char fn[32];
  uint32_t funcs;

  snprintf(fn, sizeof(fn), "/dev/i2c-%d", bus);
  fd = open(fn, O_RDWR);
  if (fd == -1) {
    syslog(LOG_WARNING, "Failed to open i2c device %s", fn);
    return I2C_BUS_ERROR;
  }

  if (ioctl(fd, I2C_FUNCS, &funcs) < 0) {
    syslog(LOG_WARNING, "Failed to get %s functionality matrix", fn);
    close(fd);
    return I2C_FUNC_ERROR;
  }

  if (force) {
    if (ioctl(fd, I2C_SLAVE_FORCE, addr)) {
      syslog(LOG_WARNING, "Failed to open slave @ address 0x%x", addr);
      close(fd);
      return I2C_DEVICE_ERROR;
    }
   } else {
    if (ioctl(fd, I2C_SLAVE, addr) < 0) {
      syslog(LOG_WARNING, "Failed to open slave @ address 0x%x", addr);
      close(fd);
      return I2c_DRIVER_EXIST;
    }
  }

  /* Probe this address */
  switch (mode) {
    case MODE_QUICK:
      /* This is known to corrupt the Atmel AT24RF08 EEPROM */
      rc = i2c_smbus_write_quick(fd, I2C_SMBUS_WRITE);
      break;
    case MODE_READ:
      /* This is known to lock SMBus on various
         write-only chips (mainly clock chips) */
      rc = i2c_smbus_read_byte(fd);
      break;
    default:
      if ((addr >= 0x30 && addr <= 0x37) || (addr >= 0x50 && addr <= 0x5F))
        rc = i2c_smbus_read_byte(fd);
      else
        rc = i2c_smbus_write_quick(fd, I2C_SMBUS_WRITE);
  }
  close(fd);

  if (rc < 0) {
    return I2C_DEVICE_ERROR;
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

#if DEBUG
  syslog(LOG_WARNING, "[%s] Cmd: %s", __func__, cmd);
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

#if DEBUG
  syslog(LOG_WARNING, "[%s] Cmd: %s", __func__, cmd);
#endif

  ret = run_command(cmd);

  return ret;
}

int
pal_get_pim_type(uint8_t fru, int retry) {
  int ret = -1, val;
  char path[LARGEST_DEVICE_NAME + 1];
  uint8_t bus = ((fru - FRU_PIM1) * 8) + 80;

  snprintf(path, LARGEST_DEVICE_NAME,
           I2C_SYSFS_DEVICES"/%d-0060/board_ver", bus);

  if (retry < 0) {
    retry = 0;
  }

  while ((ret = read_device(path, &val)) != 0 && retry--) {
    msleep(500);
  }
  if (ret) {
    return -1;
  }

#if DEBUG
  syslog(LOG_WARNING, "[%s] val: 0x%x", __func__, val);
#endif

  if (val == 0x0) {
    ret = PIM_TYPE_16Q;
  } else if ((val & 0xf0) == 0xf0) {
    ret = PIM_TYPE_16O;
  } else if (val == 0x10) {
    ret = PIM_TYPE_4DD;
  } else {
    return -1;
  }

  return ret;
}

int
pal_set_pim_type_to_file(uint8_t fru, char *type) {
  char fru_name[16];
  char key[MAX_KEY_LEN];

  pal_get_fru_name(fru, fru_name);
  sprintf(key, "%s_type", fru_name);

  return kv_set(key, type, 0, 0);
}

int
pal_get_pim_type_from_file(uint8_t fru) {
  char fru_name[16];
  char key[MAX_KEY_LEN];
  char type[12] = {0};

  pal_get_fru_name(fru, fru_name);
  sprintf(key, "%s_type", fru_name);

  if (kv_get(key, type, NULL, 0)) {
#if DEBUG
    syslog(LOG_WARNING,
            "pal_get_pim_type_from_file: %s get tpye fail", fru_name);
#endif
    return -1;
  }

  if (!strncmp(type, "16q", sizeof("16q"))) {
    return PIM_TYPE_16Q;
  } else if (!strncmp(type, "16o", sizeof("16o"))) {
    return PIM_TYPE_16O;
  } else if (!strncmp(type, "4dd", sizeof("4dd"))) {
    return PIM_TYPE_4DD;
  } else if (!strncmp(type, "unplug", sizeof("unplug"))) {
    return PIM_TYPE_UNPLUG;
  } else {
    return PIM_TYPE_NONE;
  }
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
  } else if (!strcmp(str, "smb") || !strcmp(str, "bmc")) {
    *fru = FRU_SMB;
  } else if (!strcmp(str, "scm")) {
    *fru = FRU_SCM;
  } else if (!strcmp(str, "pim1")) {
    *fru = FRU_PIM1;
  } else if (!strcmp(str, "pim2")) {
    *fru = FRU_PIM2;
  } else if (!strcmp(str, "pim3")) {
    *fru = FRU_PIM3;
  } else if (!strcmp(str, "pim4")) {
    *fru = FRU_PIM4;
  } else if (!strcmp(str, "pim5")) {
    *fru = FRU_PIM5;
  } else if (!strcmp(str, "pim6")) {
    *fru = FRU_PIM6;
  } else if (!strcmp(str, "pim7")) {
    *fru = FRU_PIM7;
  } else if (!strcmp(str, "pim8")) {
    *fru = FRU_PIM8;
  } else if (!strcmp(str, "psu1")) {
    *fru = FRU_PSU1;
  } else if (!strcmp(str, "psu2")) {
    *fru = FRU_PSU2;
  } else if (!strcmp(str, "psu3")) {
    *fru = FRU_PSU3;
  } else if (!strcmp(str, "psu4")) {
    *fru = FRU_PSU4;
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
  } else if (!strcmp(str, "fan7")) {
    *fru = FRU_FAN7;
  } else if (!strcmp(str, "fan8")) {
    *fru = FRU_FAN8;
  } else if (!strcmp(str, "bmc")) {
    *fru = FRU_BMC;
  } else if (!strcmp(str, "cpld")) {
    *fru = FRU_CPLD;
  } else if (!strcmp(str, "fpga")) {
    *fru = FRU_FPGA;
  } else {
    syslog(LOG_WARNING, "pal_get_fru_id: Wrong fru#%s", str);
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
    case FRU_SCM:
      strcpy(name, "scm");
      break;
    case FRU_PIM1:
      strcpy(name, "pim1");
      break;
    case FRU_PIM2:
      strcpy(name, "pim2");
      break;
    case FRU_PIM3:
      strcpy(name, "pim3");
      break;
    case FRU_PIM4:
      strcpy(name, "pim4");
      break;
    case FRU_PIM5:
      strcpy(name, "pim5");
      break;
    case FRU_PIM6:
      strcpy(name, "pim6");
      break;
    case FRU_PIM7:
      strcpy(name, "pim7");
      break;
    case FRU_PIM8:
      strcpy(name, "pim8");
      break;
    case FRU_PSU1:
      strcpy(name, "psu1");
      break;
    case FRU_PSU2:
      strcpy(name, "psu2");
      break;
    case FRU_PSU3:
      strcpy(name, "psu3");
      break;
    case FRU_PSU4:
      strcpy(name, "psu4");
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
    case FRU_FAN7:
      strcpy(name, "fan7");
      break;
    case FRU_FAN8:
      strcpy(name, "fan8");
      break;
    default:
      if (fru > MAX_NUM_FRUS)
        return -1;
      sprintf(name, "fru%d", fru);
      break;
  }
  return 0;
}

// Platform Abstraction Layer (PAL) Functions
int
pal_get_platform_name(char *name) {
  strcpy(name, PLATFORM_NAME);

  return 0;
}

int
pal_is_fru_prsnt(uint8_t fru, uint8_t *status) {
  int val;
  char tmp[LARGEST_DEVICE_NAME];
  char path[LARGEST_DEVICE_NAME + 1];
  *status = 0;

  switch (fru) {
    case FRU_SMB:
      *status = 1;
      return 0;
    case FRU_SCM:
      snprintf(path, LARGEST_DEVICE_NAME, SMBCPLD_PATH_FMT, SCM_PRSNT_STATUS);
      break;
    case FRU_PIM1:
    case FRU_PIM2:
    case FRU_PIM3:
    case FRU_PIM4:
    case FRU_PIM5:
    case FRU_PIM6:
    case FRU_PIM7:
    case FRU_PIM8:
      snprintf(tmp, LARGEST_DEVICE_NAME, SMBCPLD_PATH_FMT, PIM_PRSNT_STATUS);
      snprintf(path, LARGEST_DEVICE_NAME, tmp, fru - 2);
      break;
    case FRU_PSU1:
      snprintf(tmp, LARGEST_DEVICE_NAME, SMBCPLD_PATH_FMT, PSU_PRSNT_STATUS);
      snprintf(path, LARGEST_DEVICE_NAME, tmp, 1);
      break;
    case FRU_PSU2:
      snprintf(tmp, LARGEST_DEVICE_NAME, SMBCPLD_PATH_FMT, PSU_PRSNT_STATUS);
      snprintf(path, LARGEST_DEVICE_NAME, tmp, 2);
      break;
    case FRU_PSU3:
      snprintf(tmp, LARGEST_DEVICE_NAME, SMBCPLD_PATH_FMT, PSU_PRSNT_STATUS);
      snprintf(path, LARGEST_DEVICE_NAME, tmp, 3);
      break;
    case FRU_PSU4:
      snprintf(tmp, LARGEST_DEVICE_NAME, SMBCPLD_PATH_FMT, PSU_PRSNT_STATUS);
      snprintf(path, LARGEST_DEVICE_NAME, tmp, 4);
      break;
    case FRU_FAN1:
    case FRU_FAN3:
    case FRU_FAN5:
    case FRU_FAN7:
      snprintf(tmp, LARGEST_DEVICE_NAME,
               TOP_FCMCPLD_PATH_FMT, FAN_PRSNT_STATUS);
      snprintf(path, LARGEST_DEVICE_NAME, tmp, ((fru - 14) / 2) + 1);
      break;
    case FRU_FAN2:
    case FRU_FAN4:
    case FRU_FAN6:
    case FRU_FAN8:
      snprintf(tmp, LARGEST_DEVICE_NAME,
               BOTTOM_FCMCPLD_PATH_FMT, FAN_PRSNT_STATUS);
      snprintf(path, LARGEST_DEVICE_NAME, tmp, (fru - 14) / 2);
      break;
    default:
      return -1;
    }

    if (read_device(path, &val)) {
      return -1;
    }

    if (val == 0x0) {
      *status = 1;
    } else {
      *status = 0;
    }
  return 0;
}

int
pal_is_fru_ready(uint8_t fru, uint8_t *status) {
  *status = 1;

  return 0;
}

void
pal_update_ts_sled(void)
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
  syslog(LOG_ERR, "Board rev: %d\n", *rev);

  return 0;
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
  char min_ver_path[PATH_MAX];
  char sub_ver_path[PATH_MAX];
  
  switch(fru) {
    case FRU_CPLD:
      if (!(strncmp(device, SCM_CPLD, strlen(SCM_CPLD)))) {
        snprintf(ver_path, sizeof(ver_path), SCM_SYSFS, "cpld_ver");
        snprintf(min_ver_path, sizeof(min_ver_path),
                 SCM_SYSFS, "cpld_min_ver");
        snprintf(sub_ver_path, sizeof(sub_ver_path),
                 SCM_SYSFS, "cpld_sub_ver");
      } else if (!(strncmp(device, SMB_CPLD, strlen(SMB_CPLD)))) {
        snprintf(ver_path, sizeof(ver_path), SMB_SYSFS, "cpld_ver");
        snprintf(min_ver_path, sizeof(min_ver_path),
                 SMB_SYSFS, "cpld_minor_ver");
        snprintf(sub_ver_path, sizeof(sub_ver_path),
                 SMB_SYSFS, "cpld_sub_ver");
      } else if (!(strncmp(device, PWR_CPLD_L, strlen(PWR_CPLD_L)))) {
        snprintf(ver_path, sizeof(ver_path), PWR_L_SYSFS, "cpld_ver");
        snprintf(min_ver_path, sizeof(min_ver_path),
                 PWR_L_SYSFS, "cpld_minor_ver");
        snprintf(sub_ver_path, sizeof(sub_ver_path),
                 PWR_L_SYSFS, "cpld_sub_ver");
      } else if (!(strncmp(device, PWR_CPLD_R, strlen(PWR_CPLD_R)))) {
        snprintf(ver_path, sizeof(ver_path), PWR_R_SYSFS, "cpld_ver");
        snprintf(min_ver_path, sizeof(min_ver_path),
                 PWR_R_SYSFS, "cpld_minor_ver");
        snprintf(sub_ver_path, sizeof(sub_ver_path),
                 PWR_R_SYSFS, "cpld_sub_ver");
      }else if (!(strncmp(device, FCM_CPLD_T, strlen(FCM_CPLD_T)))) {
        snprintf(ver_path, sizeof(ver_path), FCM_T_SYSFS, "cpld_ver");
        snprintf(min_ver_path, sizeof(min_ver_path),
                 FCM_T_SYSFS, "cpld_minor_ver");
        snprintf(sub_ver_path, sizeof(sub_ver_path),
                 FCM_T_SYSFS, "cpld_sub_ver");
      } else if (!(strncmp(device, FCM_CPLD_B, strlen(FCM_CPLD_B)))) {
        snprintf(ver_path, sizeof(ver_path), FCM_B_SYSFS, "cpld_ver");
        snprintf(min_ver_path, sizeof(min_ver_path),
                 FCM_B_SYSFS, "cpld_minor_ver");
        snprintf(sub_ver_path, sizeof(sub_ver_path),
                 FCM_B_SYSFS, "cpld_sub_ver");
      } else {
        return -1;
      }
      break;
    case FRU_FPGA:
      if (!(strncmp(device, PIM1_DOM_FPGA, strlen(PIM1_DOM_FPGA)))) {
        snprintf(ver_path, sizeof(ver_path), PIM1_DOMFPGA_SYSFS, "fpga_ver");
        snprintf(sub_ver_path, sizeof(sub_ver_path),
                 PIM1_DOMFPGA_SYSFS, "fpga_sub_ver");
      } else if (!(strncmp(device, PIM2_DOM_FPGA, strlen(PIM2_DOM_FPGA)))) {
        snprintf(ver_path, sizeof(ver_path), PIM2_DOMFPGA_SYSFS, "fpga_ver");
        snprintf(sub_ver_path, sizeof(sub_ver_path),
                 PIM2_DOMFPGA_SYSFS, "fpga_sub_ver");
      } else if (!(strncmp(device, PIM3_DOM_FPGA, strlen(PIM3_DOM_FPGA)))) {
        snprintf(ver_path, sizeof(ver_path), PIM3_DOMFPGA_SYSFS, "fpga_ver");
        snprintf(sub_ver_path, sizeof(sub_ver_path),
                 PIM3_DOMFPGA_SYSFS, "fpga_sub_ver");
      } else if (!(strncmp(device, PIM4_DOM_FPGA, strlen(PIM4_DOM_FPGA)))) {
        snprintf(ver_path, sizeof(ver_path), PIM4_DOMFPGA_SYSFS, "fpga_ver");
        snprintf(sub_ver_path, sizeof(sub_ver_path),
                 PIM4_DOMFPGA_SYSFS, "fpga_sub_ver");
      } else if (!(strncmp(device, PIM5_DOM_FPGA, strlen(PIM5_DOM_FPGA)))) {
        snprintf(ver_path, sizeof(ver_path), PIM5_DOMFPGA_SYSFS, "fpga_ver");
        snprintf(sub_ver_path, sizeof(sub_ver_path),
                 PIM5_DOMFPGA_SYSFS, "fpga_sub_ver");
      } else if (!(strncmp(device, PIM6_DOM_FPGA, strlen(PIM6_DOM_FPGA)))) {
        snprintf(ver_path, sizeof(ver_path), PIM6_DOMFPGA_SYSFS, "fpga_ver");
        snprintf(sub_ver_path, sizeof(sub_ver_path),
                 PIM6_DOMFPGA_SYSFS, "fpga_sub_ver");
      } else if (!(strncmp(device, PIM7_DOM_FPGA, strlen(PIM7_DOM_FPGA)))) {
        snprintf(ver_path, sizeof(ver_path), PIM7_DOMFPGA_SYSFS, "fpga_ver");
        snprintf(sub_ver_path, sizeof(sub_ver_path),
                 PIM7_DOMFPGA_SYSFS, "fpga_sub_ver");
      } else if (!(strncmp(device, PIM8_DOM_FPGA, strlen(PIM8_DOM_FPGA)))) {
        snprintf(ver_path, sizeof(ver_path), PIM8_DOMFPGA_SYSFS, "fpga_ver");
        snprintf(sub_ver_path, sizeof(sub_ver_path),
                 PIM8_DOMFPGA_SYSFS, "fpga_sub_ver");
      } else if (!(strncmp(device, IOB_FPGA, strlen(IOB_FPGA)))) {
        snprintf(ver_path, sizeof(ver_path), IOBFPGA_SYSFS, "fpga_ver");
        snprintf(sub_ver_path, sizeof(sub_ver_path),
                 IOBFPGA_SYSFS, "fpga_sub_ver");
      } else {
        return -1;
      }
      break;
    default:
      return -1;
  }

  if (!read_device(ver_path, &val)) {
    ver[0] = (uint8_t)val;
  } else {
    return -1;
  }

  if (fru == FRU_CPLD) {
    if (!read_device(min_ver_path, &val)) {
      ver[1] = (uint8_t)val;
    } else {
      return -1;
    }
  }

  if (!read_device(sub_ver_path, &val)) {
    if(fru == FRU_CPLD) {
      ver[2] = (uint8_t)val;
    } else {
      ver[1] = (uint8_t)val;
    }
  } else {
    return -1;
  }

  return 0;
}

int
pal_get_fw_info(uint8_t fru, unsigned char target,
                unsigned char* res, unsigned char* res_len) {
  return -1;
}

static int
_set_pim_sts_led(uint8_t fru, uint8_t color)
{
  char path[LARGEST_DEVICE_NAME];
  uint8_t bus = 80 + ((fru - 3) * 8);

  snprintf(path, LARGEST_DEVICE_NAME,
           I2C_SYSFS_DEVICES"/%d-0060/system_led", bus);

  if(color == FPGA_STS_CLR_BLUE)
    write_device(path, "1");
  else if(color == FPGA_STS_CLR_YELLOW)
    write_device(path, "0");

  return 0;
}

void
pal_set_pim_sts_led(uint8_t fru)
{
  int val;
  char tmp[LARGEST_DEVICE_NAME];
  char path[LARGEST_DEVICE_NAME + 1];
  snprintf(tmp, LARGEST_DEVICE_NAME, KV_PATH, KV_PIM_HEALTH);
  /* FRU_PIM1 = 3, FRU_PIM2 = 4, ...., FRU_PIM8 = 10 */
  /* KV_PIM1 = 1, KV_PIM2 = 2, ...., KV_PIM8 = 8 */
  snprintf(path, LARGEST_DEVICE_NAME, tmp, fru - 2);
  if(read_device(path, &val)) {
    syslog(LOG_ERR, "%s cannot get value from %s", __func__, path);
    return;
  }
  if(val)
    _set_pim_sts_led(fru, FPGA_STS_CLR_BLUE);
  else
    _set_pim_sts_led(fru, FPGA_STS_CLR_YELLOW);

  return;
}

void *
generate_dump(void *arg) {

  uint8_t fru = *(uint8_t *) arg;
  char cmd[128];
  char fname[128];
  char fruname[16];

  // Usually the pthread cancel state are enable by default but
  // here we explicitly would like to enable them
  pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
  pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);

  pal_get_fru_name(fru, fruname);//scm

  snprintf(fname, sizeof(fname), "/var/run/autodump%d.pid", fru);
  if (access(fname, F_OK) == 0) {
    if (unlink(fname) != 0) {
      OBMC_ERROR(errno, "failed to delete %s", fname);
    }
  }

  // Execute automatic crashdump
  snprintf(cmd, sizeof(cmd), "%s %s", CRASHDUMP_BIN, fruname);
  RUN_SHELL_CMD(cmd);

  syslog(LOG_CRIT, "Crashdump for FRU: %d is generated.", fru);

  t_dump[fru-1].is_running = 0;
  return 0;
}

static int
pal_store_crashdump(uint8_t fru) {

  int ret;
  char cmd[100];

  // Check if the crashdump script exist
  if (access(CRASHDUMP_BIN, F_OK) == -1) {
    syslog(LOG_CRIT, "Crashdump for FRU: %d failed : "
        "auto crashdump binary is not preset", fru);
    return 0;
  }

  // Check if a crashdump for that fru is already running.
  // If yes, kill that thread and start a new one.
  if (t_dump[fru-1].is_running) {
    ret = pthread_cancel(t_dump[fru-1].pt);
    if (ret == ESRCH) {
      syslog(LOG_INFO,
             "pal_store_crashdump: No Crashdump pthread exists");
    } else {
      pthread_join(t_dump[fru-1].pt, NULL);
      snprintf(cmd, sizeof(cmd),
              "ps | grep '{dump.sh}' | grep 'scm' "
              "| awk '{print $1}'| xargs kill");
      RUN_SHELL_CMD(cmd);
      snprintf(cmd, sizeof(cmd),
              "ps | grep 'bic-util' | grep 'scm' "
              "| awk '{print $1}'| xargs kill");
      RUN_SHELL_CMD(cmd);
#ifdef DEBUG
      syslog(LOG_INFO, "pal_store_crashdump:"
                       " Previous crashdump thread is cancelled");
#endif
    }
  }

  // Start a thread to generate the crashdump
  t_dump[fru-1].fru = fru;
  if (pthread_create(&(t_dump[fru-1].pt), NULL, generate_dump,
      (void*) &t_dump[fru-1].fru) < 0) {
    syslog(LOG_WARNING, "pal_store_crashdump: pthread_create for"
        " FRU %d failed\n", fru);
    return -1;
  }

  t_dump[fru-1].is_running = 1;

  syslog(LOG_INFO, "Crashdump for FRU: %d is being generated.", fru);

  return 0;
}

int
pal_sel_handler(uint8_t fru, uint8_t snr_num, uint8_t *event_data) {

  char key[MAX_KEY_LEN] = {0};
  char cvalue[MAX_VALUE_LEN] = {0};
  static int assert_cnt[FUJI_MAX_NUM_SLOTS] = {0};
  switch(fru) {
    case FRU_SCM:
      switch(snr_num) {
        case CATERR_B:
          pal_store_crashdump(fru);
          break;

        case 0x00:  // don't care sensor number 00h
          return 0;
      }
      sprintf(key, "server_sel_error");

      if ((event_data[2] & 0x80) == 0) {  // 0: Assertion,  1: Deassertion
         assert_cnt[fru-1]++;
      } else {
        if (--assert_cnt[fru-1] < 0)
           assert_cnt[fru-1] = 0;
      }
      sprintf(cvalue, "%s", (assert_cnt[fru-1] > 0) ? "0" : "1");
      break;

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
  dev = open("/dev/i2c-50", O_RDWR);
  if(dev < 0) {
    syslog(LOG_ERR, "%s: open() failed\n", __func__);
    return;
  }

  ret = ioctl(dev, I2C_SLAVE, I2C_ADDR_SIM_LED);
  if(ret < 0) {
    syslog(LOG_ERR, "%s: ioctl() assigned i2c addr failed\n", __func__);
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
  dev = open("/dev/i2c-50", O_RDWR);
  if(dev < 0) {
    syslog(LOG_ERR, "%s: open() failed\n", __func__);
    return -1;
  }

  ret = ioctl(dev, I2C_SLAVE, I2C_ADDR_SIM_LED);
  if(ret < 0) {
    syslog(LOG_ERR, "%s: ioctl() assigned i2c addr failed\n", __func__);
    close(dev);
    return -1;
  }
  val_io0 = i2c_smbus_read_byte_data(dev, 0x02);
  if(val_io0 < 0) {
    close(dev);
    syslog(LOG_ERR, "%s: i2c_smbus_read_byte_data failed\n", __func__);
    return -1;
  }

  val_io1 = i2c_smbus_read_byte_data(dev, 0x03);
  if(val_io1 < 0) {
    close(dev);
    syslog(LOG_ERR, "%s: i2c_smbus_read_byte_data failed\n", __func__);
    return -1;
  }

  clr_val = color;

  if(brd_rev == 0 || brd_rev == 4) {
    if(led_name == SLED_SMB || led_name == SLED_PSU) {
      clr_val = clr_val << 3;
      val_io0 = (val_io0 & 0x7) | clr_val;
      val_io1 = (val_io1 & 0x7) | clr_val;
    }
    else if(led_name == SLED_SYS || led_name == SLED_FAN) {
      val_io0 = (val_io0 & 0x38) | clr_val;
      val_io1 = (val_io1 & 0x38) | clr_val;
    }
    else
      syslog(LOG_WARNING, "%s: unknown led name\n", __func__);

    if(led_name == SLED_PSU || led_name == SLED_FAN) {
      i2c_smbus_write_byte_data(dev, io0_reg, val_io0);
    } else {
      i2c_smbus_write_byte_data(dev, io1_reg, val_io1);
    }
  }
  else {
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
      syslog(LOG_WARNING, "%s: unknown led name\n", __func__);
    }

    if(led_name == SLED_SYS || led_name == SLED_FAN) {
      i2c_smbus_write_byte_data(dev, io0_reg, val_io0);
    } else {
      i2c_smbus_write_byte_data(dev, io1_reg, val_io1);
    }
  }

  close(dev);
  return 0;
}


static void
upgrade_led_blink(int brd_rev,
                uint8_t sys_ug, uint8_t fan_ug, uint8_t psu_ug, uint8_t smb_ug)
{
  static uint8_t sys_alter = 0, fan_alter = 0, psu_alter = 0, smb_alter = 0;

  if(sys_ug) {
    if(sys_alter == 0) {
      set_sled(brd_rev, SLED_CLR_BLUE, SLED_SYS);
      sys_alter = 1;
    } else {
      set_sled(brd_rev, SLED_CLR_YELLOW, SLED_SYS);
      sys_alter = 0;
    }
  }
  if(fan_ug) {
    if(fan_alter == 0) {
      set_sled(brd_rev, SLED_CLR_BLUE, SLED_FAN);
      fan_alter = 1;
    } else {
      set_sled(brd_rev, SLED_CLR_YELLOW, SLED_FAN);
      fan_alter = 0;
    }
  }
  if(psu_ug) {
    if(psu_alter == 0) {
      set_sled(brd_rev, SLED_CLR_BLUE, SLED_PSU);
      psu_alter = 1;
    } else {
      set_sled(brd_rev, SLED_CLR_YELLOW, SLED_PSU);
      psu_alter = 0;
    }
  }
  if(smb_ug) {
    if(smb_alter == 0) {
      set_sled(brd_rev, SLED_CLR_BLUE, SLED_SMB);
      smb_alter = 1;
    } else {
      set_sled(brd_rev, SLED_CLR_YELLOW, SLED_SMB);
      smb_alter = 0;
    }
  }
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
      syslog(LOG_ERR,
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

  *sys_ug = (strstr(buf_ptr, "pimcpld_update") != NULL) ? 1 : 0;
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

  *smb_ug = (strstr(buf_ptr, "pdbcpld_update") != NULL) ? 1 : 0;
  if(*smb_ug) goto close_fp;

  *smb_ug = (strstr(buf_ptr, "flashcp") != NULL) ? 1 : 0;
  if(*smb_ug) goto close_fp;

  *smb_ug = strstr(buf_ptr, "write spi1 IOB_FPGA_FLASH") != NULL ? 1 : 0;
  if(*smb_ug) goto close_fp;

  *smb_ug = strstr(buf_ptr, "write spi1 TH3_FLASH") != NULL ? 1 : 0;
  if(*smb_ug) goto close_fp;

  *smb_ug = strstr(buf_ptr, "write spi1 BCM5396_EE") != NULL ? 1 : 0;
  if(*smb_ug) goto close_fp;

close_fp:
  ret = pclose(fp);
  if(-1 == ret)
     syslog(LOG_ERR, "%s pclose() fail ", __func__);

  upgrade_led_blink(brd_rev, *sys_ug, *fan_ug, *psu_ug, *smb_ug);

free_buf:
  free(buf_ptr);
  return 0;
}



void set_sys_led(int brd_rev)
{
  uint8_t fru;
  uint8_t ret = 0;
  uint8_t prsnt = 0;
  ret = pal_is_fru_prsnt(FRU_SCM, &prsnt);
  if (ret) {
    set_sled(brd_rev, SLED_CLR_YELLOW, SLED_SYS);
    return;
  }
  if (!prsnt) {
    set_sled(brd_rev, SLED_CLR_YELLOW, SLED_SYS);
    return;
  }

  for(fru = FRU_PIM1; fru <= FRU_PIM8; fru++){
    ret = pal_is_fru_prsnt(fru, &prsnt);
    if (ret) {
      set_sled(brd_rev, SLED_CLR_YELLOW, SLED_SYS);
      return;
    }
    if (!prsnt) {
      set_sled(brd_rev, SLED_CLR_YELLOW, SLED_SYS);
      return;
    }
  }
  set_sled(brd_rev, SLED_CLR_BLUE, SLED_SYS);
  return;
}

void set_fan_led(int brd_rev)
{
  int i, val;
  uint8_t fan_num = 16;//rear:8 && front:8
  char path[LARGEST_DEVICE_NAME + 1];
  int sensor_num[] = {42, 43, 44, 45, 46, 47, 48, 49,
                             50, 51, 52, 53, 54, 55, 56, 57};

  for(i = 0; i < fan_num; i++) {
    snprintf(path, LARGEST_DEVICE_NAME, SENSORD_FILE_SMB, sensor_num[i]);
    if(read_device(path, &val)) {
      set_sled(brd_rev, SLED_CLR_YELLOW, SLED_FAN);
      return;
    }
    if(val <= 800) {
      set_sled(brd_rev, SLED_CLR_YELLOW, SLED_FAN);
      return;
    }
  }
  set_sled(brd_rev, SLED_CLR_BLUE, SLED_FAN);
  return;
}

void set_psu_led(int brd_rev)
{
  int i, val_in, val_out12;
  float val_out3;
  int vin_min = 92;
  int vin_max = 310;
  int vout_12_min = 11;
  int vout_12_max = 13;
  float vout_3_min = 3.00;
  float vout_3_max = 3.60;
  uint8_t psu_num = 4;
  uint8_t prsnt;
  int sensor_num[] = {1, 14, 27, 40};
  char path[LARGEST_DEVICE_NAME + 1];

  for(i = FRU_PSU1; i <= FRU_PSU4; i++) {
    pal_is_fru_prsnt(i, &prsnt);
    if(!prsnt) {
      set_sled(brd_rev, SLED_CLR_YELLOW, SLED_PSU);
      return;
    }
  }

  for(i = 0; i < psu_num; i++) {

    snprintf(path, LARGEST_DEVICE_NAME, SENSORD_FILE_PSU, i+1, sensor_num[i]);
    if(read_device(path, &val_in)) {
      set_sled(brd_rev, SLED_CLR_YELLOW, SLED_PSU);
      return;
    }

    if(val_in > vin_max || val_in < vin_min) {
      set_sled(brd_rev, SLED_CLR_YELLOW, SLED_PSU);
      return;
    }

    snprintf(path, LARGEST_DEVICE_NAME, SENSORD_FILE_PSU,
             i+1, sensor_num[i] + 1);
    if(read_device(path, &val_out12)) {
      set_sled(brd_rev, SLED_CLR_YELLOW, SLED_PSU);
      return;
    }

    if(val_out12 > vout_12_max || val_out12 < vout_12_min) {
      set_sled(brd_rev, SLED_CLR_YELLOW, SLED_PSU);
      return;
    }

    snprintf(path, LARGEST_DEVICE_NAME, SENSORD_FILE_PSU,
             i+1, sensor_num[i] + 2);
    if(read_device_float(path, &val_out3)) {
      set_sled(brd_rev, SLED_CLR_YELLOW, SLED_PSU);
      return;
    }

    if(val_out3 > vout_3_max || val_out3 < vout_3_min) {
      set_sled(brd_rev, SLED_CLR_YELLOW, SLED_PSU);
      return;
    }

  }

  set_sled(brd_rev, SLED_CLR_BLUE, SLED_PSU);

  return;
}

void set_smb_led(int brd_rev)
{
  set_sled(brd_rev, SLED_CLR_BLUE, SLED_SMB);
  return;
}

int
pal_light_scm_led(uint8_t led_color)
{
  int ret;
  char *val;

  if(led_color == SCM_LED_BLUE)
    val = "0";
  else
    val = "1";
  ret = write_device(SCM_SYS_LED_COLOR, val);
  if (ret) {
#ifdef DEBUG
  syslog(LOG_WARNING, "write_device failed for %s\n", SCM_SYS_LED_COLOR);
#endif
    return -1;
  }

  return 0;
}

int
pal_set_def_key_value(void) {

  int i, ret;
  char path[LARGEST_DEVICE_NAME + 1];

  for (i = 0; strcmp(key_list[i], LAST_KEY) != 0; i++) {
    snprintf(path, LARGEST_DEVICE_NAME, KV_PATH, key_list[i]);
    if ((ret = kv_set(key_list[i], def_val_list[i],
	                  0, KV_FPERSIST | KV_FCREATE)) < 0) {
#ifdef DEBUG
      syslog(LOG_WARNING, "pal_set_def_key_value: kv_set failed. %d", ret);
#endif
    }
  }
  return 0;
 }

 void
pal_inform_bic_mode(uint8_t fru, uint8_t mode) {
  switch(mode) {
  case BIC_MODE_NORMAL:
    // Bridge IC entered normal mode
    // Inform BIOS that BMC is ready
    bic_set_gpio(fru, BMC_READY_N, 0);
    break;
  case BIC_MODE_UPDATE:
    // Bridge IC entered update mode
    // TODO: Might need to handle in future
    break;
  default:
    break;
  }
}

//For OEM command "CMD_OEM_GET_PLAT_INFO" 0x7e
int
pal_get_plat_sku_id(void){
  return 0x06; // Fuji
}


//Use part of the function for OEM Command "CMD_OEM_GET_POSS_PCIE_CONFIG" 0xF4
int
pal_get_poss_pcie_config(uint8_t slot, uint8_t *req_data, uint8_t req_len,
                         uint8_t *res_data, uint8_t *res_len) {

  uint8_t completion_code = CC_SUCCESS;
  uint8_t pcie_conf = 0x02;//Fuji
  uint8_t *data = res_data;

  *data++ = pcie_conf;
  *res_len = data - res_data;
  return completion_code;
}

// Enable POST buffer for the server in given slot
int
pal_post_enable(uint8_t slot) {
  int ret;

  bic_config_t config = {0};
  bic_config_u *t = (bic_config_u *) &config;

  ret = bic_get_config(IPMB_BUS, &config);
  if (ret) {
#ifdef DEBUG
    syslog(LOG_WARNING,
           "post_enable: bic_get_config failed for fru: %d\n", slot);
#endif
    return ret;
  }

  if (0 == t->bits.post) {
    t->bits.post = 1;
    ret = bic_set_config(IPMB_BUS, &config);
    if (ret) {
#ifdef DEBUG
      syslog(LOG_WARNING, "post_enable: bic_set_config failed\n");
#endif
      return ret;
    }
  }

  return 0;
}

// Disable POST buffer for the server in given slot
int
pal_post_disable(uint8_t slot) {
  int ret;

  bic_config_t config = {0};
  bic_config_u *t = (bic_config_u *) &config;

  ret = bic_get_config(IPMB_BUS, &config);
  if (ret) {
    return ret;
  }

  t->bits.post = 0;

  ret = bic_set_config(IPMB_BUS, &config);
  if (ret) {
    return ret;
  }

  return 0;
}

// Get the last post code of the given slot
int
pal_post_get_last(uint8_t slot, uint8_t *status) {
  int ret;
  uint8_t buf[MAX_IPMB_RES_LEN] = {0x0};
  uint8_t len;


  ret = bic_get_post_buf(IPMB_BUS, buf, &len);
  if (ret) {
    return ret;
  }

  *status = buf[0];

  return 0;
}

int
pal_is_slot_server(uint8_t fru) {
  if (fru == FRU_SCM) {
    return 1;
  }
  return 0;
}

int
pal_get_pfr_address(uint8_t fru, uint8_t *bus, uint8_t *addr, bool *bridged) {

  *addr = PFR_MAILBOX_ADDR;
  *bus = PFR_MAILBOX_BUS;
  *bridged = false;

  return 0;
}

int
pal_get_pfr_update_address(uint8_t fru, uint8_t *bus, uint8_t *addr, bool *bridged) {

  *addr = PFR_UPDATE_ADDR;
  *bus = PFR_MAILBOX_BUS;
  *bridged = false;

  return 0;
}

