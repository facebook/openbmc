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

#define FRU_TO_FAN_ID(fru)   (((fru) - FRU_FAN1 + 1) / 2)

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

  while ((ret = device_read(path, &val)) != 0 && retry--) {
    msleep(500);
  }
  if (ret) {
    return -1;
  }

#if DEBUG
  syslog(LOG_WARNING, "[%s] val: 0x%x", __func__, val);
#endif

  if ((val & 0x80) == 0x0) {
    ret = PIM_TYPE_16Q;
  } else if ((val & 0x80) == 0x80) {
    ret = PIM_TYPE_16O;
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
    return -1;
  }

  if (!strncmp(type, "16q", sizeof("16q"))) {
    return PIM_TYPE_16Q;
  } else if (!strncmp(type, "16o", sizeof("16o"))) {
    return PIM_TYPE_16O;
  } else if (!strncmp(type, "unplug", sizeof("unplug"))) {
    return PIM_TYPE_UNPLUG;
  } else {
    return PIM_TYPE_NONE;
  }
}

int pal_get_pim_pedigree(uint8_t fru, int retry){
  int val;
  int ret = -1;
  char path[PATH_MAX];
  uint8_t bus;

  if (fru < FRU_PIM1 || fru > FRU_PIM8)
    return -1;

  if (retry < 0) {
    retry = 0;
  }

  bus = ((fru - FRU_PIM1) * 8) + 80;
  snprintf(path, sizeof(path), I2C_SYSFS_DEVICES"/%d-0060/board_ver", bus);

  while ((ret = device_read(path, &val)) != 0 && retry--) {
    msleep(500);
  }
  if (ret) {
    return -1;
  }

  if (val == 0xF0) {
    ret = PIM_16O_SIMULATE;
  } else if (val == 0xF1) {
    ret = PIM_16O_ALPHA1;
  } else {
    ret = PIM_16O_NONE_VERSION;
  }
  // TODO: need to implement get for ALPHA2,BETA and PILOT

  return ret;
}


int
pal_set_pim_pedigree_to_file(uint8_t fru, char *type) {
  char fru_name[16];
  char key[MAX_KEY_LEN];

  pal_get_fru_name(fru, fru_name);
  snprintf(key, sizeof(key), "%s_pedigree", fru_name);

  return kv_set(key, type, 0, 0);
}

int
pal_get_pim_pedigree_from_file(uint8_t fru) {
  char fru_name[16];
  char key[MAX_KEY_LEN];
  char type[12] = {0};

  pal_get_fru_name(fru, fru_name);
  snprintf(key, sizeof(key), "%s_pedigree", fru_name);

  if (kv_get(key, type, NULL, 0)) {
#ifdef DEBUG
    syslog(LOG_WARNING,
            "pal_get_pim_pedigree_from_file: %s get type fail", fru_name);
#endif
    return -1;
  }

  if (!strncmp(type, "simulate", sizeof("simulate"))) {
    return PIM_16O_SIMULATE;
  } else if (!strncmp(type, "alpha1", sizeof("alpha1"))) {
    return PIM_16O_ALPHA1;
  } else if (!strncmp(type, "alpha2", sizeof("alpha2"))) {
    return PIM_16O_ALPHA2;
  } else {
    return PIM_TYPE_NONE;
  }
}

int
pal_get_pim_phy_type(uint8_t fru, int retry) {
  int ret = -1, val;
  char path[LARGEST_DEVICE_NAME];
  uint8_t bus = ((fru - FRU_PIM1) * PIM_I2C_OFFSET) + PIM_I2C_BASE;

  snprintf(path, sizeof(path), I2C_SYSFS_DEVICES"/%d-0060/board_ver", bus);

  if (retry < 0) {
    retry = 0;
  }

  while ((ret = device_read(path, &val)) != 0 && retry--) {
    msleep(500);
  }
  if (ret) {
    return -1;
  }

  /* DVT or later is 7nm */
  if (val > PIM_16Q_EVT3) {
    ret = PIM_16Q_PHY_7NM;
  /* EVT1 or EVT2 is 16nm */
  } else if (val < PIM_16Q_EVT3) {
    ret = PIM_16Q_PHY_16NM;
  } else {
    /* When PIM board rev is EVT3, some PIM is 16nm and some is 7nm,
     * need to read Phy ID to identify 7nm or 16nm */
    if (pal_pim_is_7nm(fru))
      ret = PIM_16Q_PHY_7NM;
    else
      ret = PIM_16Q_PHY_16NM;
  }

  return ret;
}

int
pal_set_pim_phy_type_to_file(uint8_t fru, char *type) {
  char fru_name[16];
  char key[MAX_KEY_LEN];

  pal_get_fru_name(fru, fru_name);
  snprintf(key, sizeof(key), "%s_phy_type", fru_name);

  return kv_set(key, type, 0, 0);
}

int
pal_get_pim_phy_type_from_file(uint8_t fru) {
  char fru_name[16];
  char key[MAX_KEY_LEN];
  char type[12] = {0};

  pal_get_fru_name(fru, fru_name);
  snprintf(key, sizeof(key), "%s_phy_type", fru_name);

  if (kv_get(key, type, NULL, 0)) {
    return -1;
  }

  if (!strncmp(type, "7nm", sizeof("7nm"))) {
    return PIM_16Q_PHY_7NM;
  } else if (!strncmp(type, "16nm", sizeof("16nm"))) {
    return PIM_16Q_PHY_16NM;
  } else {
    return PIM_16Q_PHY_UNKNOWN;
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
pal_get_num_slots(uint8_t *num)
{
  *num = FUJI_MAX_NUM_SLOTS;
  return PAL_EOK;
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
      snprintf(path, sizeof(path), SMBCPLD_PATH_FMT, SCM_PRSNT_STATUS);
      break;
    case FRU_PIM1:
    case FRU_PIM2:
    case FRU_PIM3:
    case FRU_PIM4:
    case FRU_PIM5:
    case FRU_PIM6:
    case FRU_PIM7:
    case FRU_PIM8:
      snprintf(tmp, sizeof(tmp), SMBCPLD_PATH_FMT, PIM_PRSNT_STATUS);
      snprintf(path, sizeof(path), tmp, fru - 2);
      break;
    case FRU_PSU1:
      snprintf(tmp, sizeof(tmp), SMBCPLD_PATH_FMT, PSU_PRSNT_STATUS);
      snprintf(path, sizeof(path), tmp, 1);
      break;
    case FRU_PSU2:
      snprintf(tmp, sizeof(tmp), SMBCPLD_PATH_FMT, PSU_PRSNT_STATUS);
      snprintf(path, sizeof(path), tmp, 2);
      break;
    case FRU_PSU3:
      snprintf(tmp, sizeof(tmp), SMBCPLD_PATH_FMT, PSU_PRSNT_STATUS);
      snprintf(path, sizeof(path), tmp, 3);
      break;
    case FRU_PSU4:
      snprintf(tmp, sizeof(tmp), SMBCPLD_PATH_FMT, PSU_PRSNT_STATUS);
      snprintf(path, sizeof(path), tmp, 4);
      break;
    case FRU_FAN1:
    case FRU_FAN3:
    case FRU_FAN5:
    case FRU_FAN7:
      snprintf(tmp, sizeof(tmp),
               TOP_FCMCPLD_PATH_FMT, FAN_PRSNT_STATUS);
      snprintf(path, sizeof(path), tmp, FRU_TO_FAN_ID(fru) + 1);
      break;
    case FRU_FAN2:
    case FRU_FAN4:
    case FRU_FAN6:
    case FRU_FAN8:
      snprintf(tmp, sizeof(tmp),
               BOTTOM_FCMCPLD_PATH_FMT, FAN_PRSNT_STATUS);
      snprintf(path, sizeof(path), tmp, FRU_TO_FAN_ID(fru));
      break;
    default:
      return -1;
    }

    if (device_read(path, &val)) {
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
  int val;
  int ret = -1;

  snprintf(path, sizeof(path), IOBFPGA_PATH_FMT, "board_ver");
  ret = device_read(path, &val);
  if (ret) {
    return -1;
  }
  *rev = val;

  return 0;
}

int
pal_get_cpld_board_rev(int *rev, const char *device) {
  char full_name[LARGEST_DEVICE_NAME + 1];

  snprintf(full_name, LARGEST_DEVICE_NAME, device, "board_ver");
  if (device_read(full_name, rev)) {
    return -1;
  }

  return 0;
}

int cpld_name_to_path(const char *name, char *ver_path,char *min_ver_path,
                      char *sub_ver_path, size_t size)
{
  int i, j;
  static struct {
  const char *name;
  const char *sysfs_dir[2];
  } cpld_sysfs_map[] = {
    {
      SCM_CPLD,
      {SCM_SYSFS, NULL},
    },
    {
      SMB_CPLD,
      {SMB_SYSFS, NULL},
    },
    {
      PWR_CPLD_L,
      {PWR_L_SYSFS_DVT, PWR_L_SYSFS_EVT},
    },
    {
      PWR_CPLD_R,
      {PWR_R_SYSFS_DVT, PWR_R_SYSFS_EVT},
    },
    {
      FCM_CPLD_T,
      {FCM_T_SYSFS, NULL},
    },
    {
      FCM_CPLD_B,
      {FCM_B_SYSFS, NULL},
    },
    {NULL, {NULL, NULL}}, /* always the last entry. */
  };

  for (i = 0; cpld_sysfs_map[i].name != NULL; i++) {
    if (strcmp(name, cpld_sysfs_map[i].name) != 0)
      continue;

    for (j = 0; j < ARRAY_SIZE(cpld_sysfs_map->sysfs_dir); j++) {
      if (cpld_sysfs_map[i].sysfs_dir[j] != NULL &&
            path_exists(cpld_sysfs_map[i].sysfs_dir[j])) {
            snprintf(ver_path, size, "%scpld_ver", cpld_sysfs_map[i].sysfs_dir[j]);
            snprintf(min_ver_path, size, "%scpld_minor_ver", cpld_sysfs_map[i].sysfs_dir[j]);
            snprintf(sub_ver_path, size, "%scpld_sub_ver", cpld_sysfs_map[i].sysfs_dir[j]);
            return 0;
      }
    }
  }
  return -1; /* no matched CPLD. */
}

int fpga_name_to_path(const char *name, char *ver_path, char *sub_ver_path, size_t size)
{
  int i, j;
  static struct {
  const char *name;
  const char *sysfs_dir[2];
  } fpga_sysfs_map[] = {
    {
      PIM1_DOM_FPGA,
      {PIM1_DOMFPGA_SYSFS, NULL},
    },
    {
      PIM2_DOM_FPGA,
      {PIM2_DOMFPGA_SYSFS, NULL},
    },
    {
      PIM3_DOM_FPGA,
      {PIM3_DOMFPGA_SYSFS, NULL},
    },
    {
      PIM4_DOM_FPGA,
      {PIM4_DOMFPGA_SYSFS, NULL},
    },
    {
      PIM5_DOM_FPGA,
      {PIM5_DOMFPGA_SYSFS, NULL},
    },
    {
      PIM6_DOM_FPGA,
      {PIM6_DOMFPGA_SYSFS, NULL},
    },
    {
      PIM7_DOM_FPGA,
      {PIM7_DOMFPGA_SYSFS, NULL},
    },
    {
      PIM8_DOM_FPGA,
      {PIM8_DOMFPGA_SYSFS, NULL},
    },
    {
      IOB_FPGA,
      {IOBFPGA_SYSFS, NULL},
    },
    {NULL, {NULL, NULL}}, /* always the last entry. */
  };

  for (i = 0; fpga_sysfs_map[i].name != NULL; i++) {
    if (strcmp(name, fpga_sysfs_map[i].name) != 0)
      continue;

    for (j = 0; j < ARRAY_SIZE(fpga_sysfs_map->sysfs_dir); j++) {
      if (fpga_sysfs_map[i].sysfs_dir[j] != NULL &&
            path_exists(fpga_sysfs_map[i].sysfs_dir[j])) {
            snprintf(ver_path, size, "%sfpga_ver", fpga_sysfs_map[i].sysfs_dir[j]);
            snprintf(sub_ver_path, size, "%sfpga_sub_ver", fpga_sysfs_map[i].sysfs_dir[j]);
            return 0;
      }
    }
  }
  return -1; /* no matched CPLD. */
}

int
pal_get_cpld_fpga_fw_ver(uint8_t fru, const char *device, uint8_t* ver) {
  int val = -1;
  char ver_path[PATH_MAX];
  char min_ver_path[PATH_MAX];
  char sub_ver_path[PATH_MAX];

  memset(ver_path, 0, sizeof(ver_path));
  memset(min_ver_path, 0, sizeof(min_ver_path));
  memset(sub_ver_path, 0, sizeof(sub_ver_path));

  switch (fru) {
    case FRU_CPLD:
      if (cpld_name_to_path(device, ver_path, min_ver_path, sub_ver_path, PATH_MAX) < 0)
        return -1;
      break;

    case FRU_FPGA:
      if (fpga_name_to_path(device, ver_path, sub_ver_path, PATH_MAX) < 0)
        return -1;
      break;

    default:
      return -1;
  }

  if (!device_read(ver_path, &val)) {
    ver[0] = (uint8_t)val;
  } else {
    return -1;
  }

  if (fru == FRU_CPLD) {
    if (!device_read(min_ver_path, &val)) {
      ver[1] = (uint8_t)val;
    } else {
      return -1;
    }
  }

  if (!device_read(sub_ver_path, &val)) {
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
    device_write_buff(path, "1");
  else if(color == FPGA_STS_CLR_YELLOW)
    device_write_buff(path, "0");

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
  if(device_read(path, &val)) {
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
  // TODO, currently there is no definition, but in the future, maybe need action. 
  return;
}

int
set_sled(int brd_rev, uint8_t color, uint8_t name)
{
  char led_path[64];
  struct {
    const char *color;
    const char *brightness;
  } led_colors[SLED_COLOR_MAX] = {
    {"0 0 0",         "0"},   // SLED_COLOR_OFF
    {"0 0 255",       "255"}, // SLED_COLOR_BLUE
    {"0 255 0",       "255"}, // SLED_COLOR_GREEN
    {"255 45 0",      "255"}, // SLED_COLOR_AMBER
    {"255 0 0",       "255"}  // SLED_COLOR_RED
  };

  const char *led_names[SLED_NAME_MAX] = {
    "sys", // SLED_NAME_SYS
    "fan", // SLED_NAME_FAN
    "psu", // SLED_NAME_PSU
    "smb"  // SLED_NAME_SMB
  };

  if ((color >= SLED_COLOR_MAX) || (name >= SLED_NAME_MAX)) {
    syslog(LOG_WARNING, "set_sled: error color %d or error name %d", color, name);
    return -1;
  }
  
  sprintf(led_path,"/sys/class/leds/%s/multi_intensity", led_names[name]);
  device_write_buff(led_path, led_colors[color].color);
  sprintf(led_path,"/sys/class/leds/%s/brightness", led_names[name]);
  device_write_buff(led_path, led_colors[color].brightness);

  return 0;
}

int
pal_mon_fw_upgrade(int brd_rev, uint8_t *status)
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
  *status = strstr(buf_ptr, "spi_util.sh") != NULL ? 1 : 0;
  if(*status) goto close_fp;

  *status = (strstr(buf_ptr, "cpld_update.sh") != NULL) ? 1 : 0;
  if(*status) goto close_fp;

  *status = (strstr(buf_ptr, "fw-util") != NULL) ?
          ((strstr(buf_ptr, "--update") != NULL) ? 1 : 0) : 0;
  if(*status) goto close_fp;

  *status = (strstr(buf_ptr, "psu-util") != NULL) ?
          ((strstr(buf_ptr, "--update") != NULL) ? 1 : 0) : 0;
  if(*status) goto close_fp;

  *status = (strstr(buf_ptr, "flashcp") != NULL) ? 1 : 0;
  if(*status) goto close_fp;


close_fp:
  ret = pclose(fp);
  if(-1 == ret)
     syslog(LOG_ERR, "%s pclose() fail ", __func__);


free_buf:
  free(buf_ptr);
  return 0;
}

int
pal_get_boot_order(uint8_t slot, uint8_t *req_data,
                   uint8_t *boot, uint8_t *res_len) {
  int ret, msb, lsb, i, j = 0;
  char str[MAX_VALUE_LEN] = {0};
  char tstr[4];

  ret = pal_get_key_value("server_boot_order", str);
  if (ret) {
    *res_len = 0;
    return ret;
  }

  for (i = 0; i < 2*SIZE_BOOT_ORDER; i += 2) {
    snprintf(tstr, sizeof(tstr), "%c\n", str[i]);
    msb = strtol(tstr, NULL, 16);

    snprintf(tstr, sizeof(tstr), "%c\n", str[i+1]);
    lsb = strtol(tstr, NULL, 16);
    boot[j++] = (msb << 4) | lsb;
  }
  *res_len = SIZE_BOOT_ORDER;

  return 0;
}

int
pal_set_boot_order(uint8_t slot, uint8_t *boot,
                   uint8_t *res_data, uint8_t *res_len) {
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

int
pal_light_scm_led(uint8_t led_color)
{
  int ret;
  char *val;

  if(led_color == SCM_LED_BLUE)
    val = "0";
  else
    val = "1";
  ret = device_write_buff(SCM_SYS_LED_COLOR, val);
  if (ret) {
#ifdef DEBUG
  syslog(LOG_WARNING, "device_write_buff failed for %s\n", SCM_SYS_LED_COLOR);
#endif
    return -1;
  }

  return 0;
}

int
pal_set_def_key_value(void) {

  int i, ret;
  char path[LARGEST_DEVICE_NAME + 1];

  for (i = 0; strncmp(key_list[i], LAST_KEY, strlen(LAST_KEY)) != 0; i++) {
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

int
pal_get_80port_record(uint8_t slot, uint8_t *res_data, size_t max_len, size_t *res_len)
{
  int ret;
  uint8_t len;

  ret = bic_get_post_buf(IPMB_BUS, res_data, &len);
  if (ret) {
    return CC_NODE_BUSY;
  } else {
    *res_len = len;
  }

  return CC_SUCCESS;
}

int
pal_get_board_id(uint8_t slot, uint8_t *req_data, uint8_t req_len,
                 uint8_t *res_data, uint8_t *res_len)
{
  int board_sku_id = 0, board_rev = 0, val = 0, ret = 0;
  unsigned char *data = res_data;
  char path[PATH_MAX];

  board_sku_id = pal_get_plat_sku_id();

  snprintf(path, sizeof(path), IOBFPGA_PATH_FMT, "board_ver");
  ret = device_read(path, &val);
  if (ret) return CC_NODE_BUSY;
  board_rev = val;

  *data++ = board_sku_id;
  *data++ = board_rev;
  *data++ = slot;
  *data++ = 0x00; // 1S Server.
  *res_len = data - res_data;

  return CC_SUCCESS;
}
