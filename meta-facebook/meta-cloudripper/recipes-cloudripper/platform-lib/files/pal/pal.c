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
  * This file contains functions and logics that depends on Cloudripper specific
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
#include <facebook/bic.h>
#include <facebook/wedge_eeprom.h>
#include "pal.h"

uint8_t g_dev_guid[GUID_SIZE] = {0};

struct threadinfo {
  uint8_t is_running;
  uint8_t fru;
  pthread_t pt;
};

static struct threadinfo t_dump[MAX_NUM_FRUS] = {0};

const char pal_fru_list[] = "all, scm, smb, "\
                            "psu1, psu2, fan1, fan2, fan3, fan4 ";

char *key_list[] = {
  "pwr_server_last_state",
  "sysfw_ver_server",
  "timestamp_sled",
  "server_por_cfg",
  "server_sel_error",
  "scm_sensor_health",
  "smb_sensor_health",
  "psu1_sensor_health",
  "psu2_sensor_health",
  "fan1_sensor_health",
  "fan2_sensor_health",
  "fan3_sensor_health",
  "fan4_sensor_health",
  "server_boot_order",
  /* Add more Keys here */
  LAST_KEY /* This is the last key of the list */
};

char *def_val_list[] = {
  "on", /* pwr_server_last_state */
  "0", /* sysfw_ver_server */
  "0", /* timestamp_sled */
  "lps", /* server_por_cfg */
  "1", /* server_sel_error */
  "1", /* scm_sensor_health */
  "1", /* smb_sensor_health */
  "1", /* psu1_sensor_health */
  "1", /* psu2_sensor_health */
  "1", /* fan1_sensor_health */
  "1", /* fan2_sensor_health */
  "1", /* fan3_sensor_health */
  "1", /* fan4_sensor_health */
  "0000000",/* server_boot_order */
  /* Add more def values for the correspoding keys*/
  LAST_KEY /* Same as last entry of the key_list */
};

void pal_inform_bic_mode(uint8_t fru, uint8_t mode) {
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

static int pal_key_check(char *key) {
  int i = 0;

  while(strcmp(key_list[i], LAST_KEY)) {
    // If Key is valid, return success
    if (!strcmp(key, key_list[i]))
      return 0;

    i++;
  }

  PAL_DEBUG("pal_key_check: invalid key - %s", key);

  return -1;
}

int pal_get_key_value(char *key, char *value) {
  int ret;
  // Check is key is defined and valid
  if (pal_key_check(key))
    return -1;

  ret = kv_get(key, value, NULL, KV_FPERSIST);
  return ret;
}

int pal_set_key_value(char *key, char *value) {
  // Check is key is defined and valid
  if (pal_key_check(key))
    return -1;

  return kv_set(key, value, 0, KV_FPERSIST);
}

int pal_get_fru_list(char *list) {
  strcpy(list, pal_fru_list);
  return 0;
}

int pal_get_fru_capability(uint8_t fru, unsigned int *caps)
{
  int ret = 0;
  switch(fru) {
    case FRU_SMB:
    case FRU_PSU1:
    case FRU_PSU2:
    case FRU_FAN1:
    case FRU_FAN2:
    case FRU_FAN3:
    case FRU_FAN4:
    case FRU_CPLD:
    case FRU_FPGA:
      *caps = FRU_CAPABILITY_SENSOR_ALL;
      break;
    case FRU_SCM:
      *caps = FRU_CAPABILITY_SENSOR_ALL |
        FRU_CAPABILITY_POWER_ALL;
      break;
    case FRU_BMC:
      *caps = FRU_CAPABILITY_FRUID_ALL | FRU_CAPABILITY_SENSOR_ALL |
        FRU_CAPABILITY_MANAGEMENT_CONTROLLER;
      break;
    default:
      ret = -1;
      break;
  }
  return ret;
}

int pal_get_fru_id(char *str, uint8_t *fru) {
  if (!strcmp(str, "all")) {
    *fru = FRU_ALL;
  } else if (!strcmp(str, "smb")) {
    *fru = FRU_SMB;
  } else if (!strcmp(str, "scm")) {
    *fru = FRU_SCM;
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

int pal_get_fru_name(uint8_t fru, char *name) {
  switch(fru) {
    case FRU_SMB:
      strcpy(name, "smb");
      break;
    case FRU_SCM:
      strcpy(name, "scm");
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
    default:
      if (fru > MAX_NUM_FRUS)
        return -1;
      sprintf(name, "fru%d", fru);
      break;
  }
  return 0;
}

// Platform Abstraction Layer (PAL) Functions
int pal_get_platform_name(char *name) {
  strcpy(name, PLATFORM_NAME);

  return 0;
}

int pal_is_fru_prsnt(uint8_t fru, uint8_t *status) {
  int val;
  char tmp[LARGEST_DEVICE_NAME];
  char path[LARGEST_DEVICE_NAME + 1];
  *status = 0;

  switch (fru) {
    case FRU_SMB:
      *status = 1;
      return 0;
    case FRU_SCM:
      snprintf(path, LARGEST_DEVICE_NAME, SMB_SYSFS, SCM_PRSNT_STATUS);
      break;
    case FRU_PSU1:
    case FRU_PSU2:
      snprintf(tmp, LARGEST_DEVICE_NAME, SMB_SYSFS, PSU_PRSNT_STATUS);
      snprintf(path, LARGEST_DEVICE_NAME, tmp, fru - FRU_PSU1 + 1);
      break;
    case FRU_FAN1:
    case FRU_FAN2:
    case FRU_FAN3:
    case FRU_FAN4:
      snprintf(tmp, LARGEST_DEVICE_NAME, FCM_SYSFS, FAN_PRSNT_STATUS);
      snprintf(path, LARGEST_DEVICE_NAME, tmp, fru - FRU_FAN1 + 1);
      break;
    default:
      OBMC_INFO("unsupported fru id %d", fru);
      return -1;
    }

    if (device_read(path, &val)) {
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

int pal_is_fru_ready(uint8_t fru, uint8_t *status) {
  int ret = 0;

  switch(fru) {
    default:
      *status = 1;

      break;
  }

  return ret;
}

void pal_update_ts_sled() {
  char key[MAX_KEY_LEN];
  char tstr[MAX_VALUE_LEN] = {0};
  struct timespec ts;

  clock_gettime(CLOCK_REALTIME, &ts);
  sprintf(tstr, "%ld", ts.tv_sec);

  sprintf(key, "timestamp_sled");

  pal_set_key_value(key, tstr);
}

int pal_is_debug_card_prsnt(uint8_t *status) {
  int val;
  char path[LARGEST_DEVICE_NAME + 1];

  snprintf(path, LARGEST_DEVICE_NAME, GPIO_DEBUG_PRSNT_N, "value");

  if (device_read(path, &val)) {
    return -1;
  }

  if (val) {
    *status = 1;
  } else {
    *status = 0;
  }

  return 0;
}

// Return the Front panel Power Button
int pal_get_board_rev(int *rev) {
  char path[LARGEST_DEVICE_NAME + 1];

  snprintf(path, sizeof(path), SMB_SYSFS, "board_ver");
  if (device_read(path, rev)) {
    return -1;
  }

  return 0;
}

int pal_get_board_type(uint8_t *brd_type) {
  *brd_type = BOARD_TYPE_CLOUDRIPPER;
  return CC_SUCCESS;
}

int pal_get_board_type_rev(uint8_t *brd_type_rev) {
  *brd_type_rev = BOARD_CLOUDRIPPER;
  return 0;
}

int pal_get_cpld_board_rev(int *rev, const char *device) {
  char full_name[LARGEST_DEVICE_NAME + 1];

  snprintf(full_name, LARGEST_DEVICE_NAME, device, "board_ver");
  if (device_read(full_name, rev)) {
    return -1;
  }

  return 0;
}

int pal_get_cpld_fpga_fw_ver(uint8_t fru, const char *device, uint8_t* ver) {
  int val = -1;
  char ver_path[PATH_MAX];
  char sub_ver_path[PATH_MAX];

  switch(fru) {
    case FRU_CPLD:
      if (!(strncmp(device, SCM_CPLD, strlen(SCM_CPLD)))) {
        snprintf(ver_path, sizeof(ver_path), SCM_SYSFS, "cpld_ver");
        snprintf(sub_ver_path, sizeof(sub_ver_path),
                 SCM_SYSFS, "cpld_sub_ver");
      } else if (!(strncmp(device, SMB_CPLD, strlen(SMB_CPLD)))) {
        snprintf(ver_path, sizeof(ver_path), SMB_SYSFS, "cpld_ver");
        snprintf(sub_ver_path, sizeof(sub_ver_path),
                 SMB_SYSFS, "cpld_sub_ver");
      } else if (!(strncmp(device, PWR_CPLD, strlen(PWR_CPLD)))) {
        snprintf(ver_path, sizeof(ver_path), PWR_SYSFS, "cpld_ver");
        snprintf(sub_ver_path, sizeof(sub_ver_path),
                 PWR_SYSFS, "cpld_sub_ver");
      } else if (!(strncmp(device, FCM_CPLD, strlen(FCM_CPLD)))) {
        snprintf(ver_path, sizeof(ver_path), FCM_SYSFS, "cpld_ver");
        snprintf(sub_ver_path, sizeof(sub_ver_path),
                 FCM_SYSFS, "cpld_sub_ver");
      } else {
        return -1;
      }
      break;
    case FRU_FPGA:
      if (!(strncmp(device, DOM_FPGA1, strlen(DOM_FPGA1)))) {
        snprintf(ver_path, sizeof(ver_path), DOMFPGA1_SYSFS, "fpga_ver");
        snprintf(sub_ver_path, sizeof(sub_ver_path),
                 DOMFPGA1_SYSFS, "fpga_sub_ver");
      } else if (!(strncmp(device, DOM_FPGA2, strlen(DOM_FPGA2)))) {
        snprintf(ver_path, sizeof(ver_path), DOMFPGA2_SYSFS, "fpga_ver");
        snprintf(sub_ver_path, sizeof(sub_ver_path),
                 DOMFPGA2_SYSFS, "fpga_sub_ver");
      } else {
        return -1;
      }
      break;
    default:
      return -1;
  }

  if (!device_read(ver_path, &val)) {
    ver[0] = (uint8_t)val;
  } else {
    return -1;
  }

  if (!device_read(sub_ver_path, &val)) {
    ver[1] = (uint8_t)val;
  } else {
    printf("[debug][ver:%s]\n", ver_path);
    printf("[debug][sub_ver:%s]\n", sub_ver_path);
    OBMC_INFO("[debug][ver:%s]\n", ver_path);
    OBMC_INFO("[debug][sub_ver:%s]\n", sub_ver_path);
    return -1;
  }

  return 0;
}

int pal_get_num_slots(uint8_t *num)
{
  *num = MAX_NUM_SCM;
  return PAL_EOK;
}

void *generate_dump(void *arg) {
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

static int pal_store_crashdump(uint8_t fru) {
  int ret;
  char cmd[100];

  // Check if the crashdump script exist
  if (access(CRASHDUMP_BIN, F_OK) == -1) {
    OBMC_CRIT("Crashdump for FRU: %d failed : "
        "auto crashdump binary is not preset", fru);
    return 0;
  }

  // Check if a crashdump for that fru is already running.
  // If yes, kill that thread and start a new one.
  if (t_dump[fru-1].is_running) {
    ret = pthread_cancel(t_dump[fru-1].pt);
    if (ret == ESRCH) {
      OBMC_INFO("pal_store_crashdump: No Crashdump pthread exists");
    } else {
      pthread_join(t_dump[fru-1].pt, NULL);
      sprintf(cmd,
              "ps | grep '{dump.sh}' | grep 'scm' "
              "| awk '{print $1}'| xargs kill");
      if (system(cmd)) {
        OBMC_INFO("Detection of existing crashdump failed!\n");
      }
      sprintf(cmd,
              "ps | grep 'bic-util' | grep 'scm' "
              "| awk '{print $1}'| xargs kill");
      if (system(cmd)) {
        OBMC_INFO("Detection of existing bic-util scm failed!\n");
      }

      PAL_DEBUG("pal_store_crashdump: Previous crashdump thread is cancelled");

    }
  }

  // Start a thread to generate the crashdump
  t_dump[fru-1].fru = fru;
  if (pthread_create(&(t_dump[fru-1].pt), NULL, generate_dump,
      (void*) &t_dump[fru-1].fru) < 0) {
    OBMC_WARN("pal_store_crashdump: pthread_create for"
        " FRU %d failed\n", fru);
    return -1;
  }

  t_dump[fru-1].is_running = 1;

  OBMC_INFO("Crashdump for FRU: %d is being generated.", fru);

  return 0;
}

int pal_sel_handler(uint8_t fru, uint8_t snr_num, uint8_t *event_data) {
  char key[MAX_KEY_LEN] = {0};
  char cvalue[MAX_VALUE_LEN] = {0};
  static int assert_cnt[MAX_NUM_SLOTS] = {0};

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

int pal_mon_fw_upgrade(uint8_t *status)
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
  if (NULL == fp)
     return -1;

  buf_ptr = (char *)malloc(buf_size * sizeof(char) + sizeof(char));
  memset(buf_ptr, 0, sizeof(char));
  tmp_size = str_size;
  while(fgets(str, str_size, fp) != NULL) {
    tmp_size = tmp_size + str_size;
    if (tmp_size + str_size >= buf_size) {
      buf_ptr = realloc(buf_ptr, sizeof(char) * buf_size * 2 + sizeof(char));
      buf_size *= 2;
    }
    if (!buf_ptr) {
      OBMC_ERROR(-1,
             "%s realloc() fail, please check memory remaining", __func__);
      goto free_buf;
    }
    strncat(buf_ptr, str, str_size);
  }

  *status = strstr(buf_ptr, "spi_util.sh") != NULL ? 1 : 0;
  if (*status) goto close_fp;

  *status = (strstr(buf_ptr, "fw-util") != NULL) ?
          ((strstr(buf_ptr, "--update") != NULL) ? 1 : 0) : 0;
  if (*status) goto close_fp;

  *status = (strstr(buf_ptr, "psu-util") != NULL) ?
          ((strstr(buf_ptr, "--update") != NULL) ? 1 : 0) : 0;
  if (*status) goto close_fp;

  *status = (strstr(buf_ptr, "cpld_update.sh") != NULL) ? 1 : 0;
  if (*status) goto close_fp;

  *status = (strstr(buf_ptr, "flashcp") != NULL) ? 1 : 0;
  if (*status) goto close_fp;

close_fp:
  ret = pclose(fp);
  if (-1 == ret)
     OBMC_ERROR(-1, "%s pclose() fail ", __func__);

free_buf:
  free(buf_ptr);
  return 0;
}

int pal_set_def_key_value(void) {
  int i, ret;
  char path[LARGEST_DEVICE_NAME + 1];

  for (i = 0; strcmp(key_list[i], LAST_KEY) != 0; i++) {
    snprintf(path, LARGEST_DEVICE_NAME, KV_PATH, key_list[i]);
    if ((ret = kv_set(key_list[i], def_val_list[i],
                    0, KV_FPERSIST | KV_FCREATE)) < 0) {
      PAL_DEBUG("pal_set_def_key_value: kv_set failed. %d", ret);
    }
  }
  return 0;
 }

int pal_init_sensor_check(uint8_t fru, uint8_t snr_num, void *snr) {
  pal_set_def_key_value();
  return 0;
}

int pal_get_fru_health(uint8_t fru, uint8_t *value) {
  char cvalue[MAX_VALUE_LEN] = {0};
  char key[MAX_KEY_LEN] = {0};
  int ret;

  switch(fru) {
  case FRU_SCM:
    sprintf(key, "scm_sensor_health");
    break;
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

int pal_set_sensor_health(uint8_t fru, uint8_t value) {
  char key[MAX_KEY_LEN] = {0};
  char cvalue[MAX_VALUE_LEN] = {0};

  switch(fru) {
  case FRU_SCM:
    sprintf(key, "scm_sensor_health");
    break;
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
  default:
    return -1;
  }

  sprintf(cvalue, (value > 0) ? "1": "0");

  return pal_set_key_value(key, cvalue);
}

int pal_parse_sel(uint8_t fru, uint8_t *sel, char *error_log)
{
  uint8_t snr_num = sel[11];
  uint8_t *event_data = &sel[10];
  uint8_t *ed = &event_data[3];
  char temp_log[512] = {0};
  uint8_t sen_type = event_data[0];
  uint8_t chn_num, dimm_num;
  bool parsed = false;

  switch(snr_num) {
    case BIC_SENSOR_SYSTEM_STATUS:
      strcpy(error_log, "");
      switch (ed[0] & 0x0F) {
        case 0x00:
          strcat(error_log, "SOC_Thermal_Trip");
          break;
        case 0x01:
          strcat(error_log, "SOC_FIVR_Fault");
          break;
        case 0x02:
          strcat(error_log, "SOC_Throttle");
          break;
        case 0x03:
          strcat(error_log, "PCH_HOT");
          break;
      }
      parsed = true;
      break;

    case BIC_SENSOR_CPU_DIMM_HOT:
      strcpy(error_log, "");
      switch (ed[0] & 0x0F) {
        case 0x01:
          strcat(error_log, "SOC_MEMHOT");
          break;
      }
      parsed = true;
      break;

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

static int platform_sensor_name(uint8_t fru, uint8_t sensor_num, char *name) {
  switch(fru) {
    case FRU_SCM:
      switch(sensor_num) {
        case BIC_SENSOR_SYSTEM_STATUS:
          sprintf(name, "SYSTEM_STATUS");
          break;
        case BIC_SENSOR_SYS_BOOT_STAT:
          sprintf(name, "SYS_BOOT_STAT");
          break;
        case BIC_SENSOR_CPU_DIMM_HOT:
          sprintf(name, "CPU_DIMM_HOT");
          break;
        case BIC_SENSOR_PROC_FAIL:
          sprintf(name, "PROC_FAIL");
          break;
        case BIC_SENSOR_VR_HOT:
          sprintf(name, "VR_HOT");
          break;
        default:
          return -1;
      }
      break;
  }
  return 0;
}

int pal_get_event_sensor_name(uint8_t fru, uint8_t *sel, char *name) {
  uint8_t snr_type = sel[10];
  uint8_t snr_num = sel[11];

  // If SNR_TYPE is OS_BOOT, sensor name is OS
  switch (snr_type) {
    case OS_BOOT:
      // OS_BOOT used by OS
      sprintf(name, "OS");
      return 0;
    default:
      if (platform_sensor_name(fru, snr_num, name) != 0) {
        break;
      }
      return 0;
  }
  // Otherwise, translate it based on snr_num
  return pal_get_x86_event_sensor_name(fru, snr_num, name);
}

// Write GUID into EEPROM
static int pal_set_guid(uint16_t offset, char *guid) {
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
static int pal_get_guid(uint16_t offset, char *guid) {
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

int pal_get_dev_guid(uint8_t fru, char *guid) {
  pal_get_guid(OFFSET_DEV_GUID, (char *)g_dev_guid);
  memcpy(guid, g_dev_guid, GUID_SIZE);

  return 0;
}

int pal_set_dev_guid(uint8_t slot, char *guid) {
  pal_populate_guid(g_dev_guid, guid);

  return pal_set_guid(OFFSET_DEV_GUID, (char *)g_dev_guid);
}

int pal_get_boot_order(uint8_t slot, uint8_t *req_data,
                       uint8_t *boot, uint8_t *res_len) {
  int ret, msb, lsb, i, j = 0;
  char str[MAX_VALUE_LEN] = {0};
  char tstr[4] = {0};

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

int pal_get_bmc_ipmb_slave_addr(uint16_t *slave_addr, uint8_t bus_id)
{
  *slave_addr = 0x10;
  return 0;
}

int pal_ipmb_processing(int bus, void *buf, uint16_t size) {
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
