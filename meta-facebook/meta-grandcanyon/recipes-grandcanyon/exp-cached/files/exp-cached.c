/*
 *
 * Copyright 2020-present Facebook. All Rights Reserved.
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
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <syslog.h>
#include <string.h>
#include <pthread.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <openbmc/ipmi.h>
#include <openbmc/ipmb.h>
#include <openbmc/pal.h>
#include <openbmc/kv.h>
#include <facebook/exp.h>

#define MAX_FRU_PATH 64
#define FW_VERSION_LENS 4
#define MAX_RETRY_TO_GET_EXP_FRU 5

uint8_t expander_fruid_list[] = {FRU_SCC, FRU_DPB};
uint8_t expander_fan_fruid_list[] = {FRU_FAN0, FRU_FAN1, FRU_FAN2, FRU_FAN3};

static void
exp_read_fruid_wrapper(uint8_t fru, char* fru_path) {
  int ret = 0, log_level = LOG_WARNING;
  char fruid_temp_path[MAX_FRU_PATH] = {0};
  char fruid_path[MAX_FRU_PATH] = {0};
  char fru_name[MAX_FRU_CMD_STR] = {0};
  char cmd[MAX_SYS_CMD_REQ_LEN + MAX_FRU_PATH] = {0};
  int retry = 0;
  uint8_t present_status = 0;

  if (fru_path == NULL) {
    syslog(LOG_WARNING, "%s() Fail to get fru%d due to parameter fru_path is NULL\n", __func__, fru);
    return;
  }

  if (pal_get_fru_name(fru, fru_name) < 0) {
    syslog(LOG_WARNING, "%s() Fail to get fru%d name\n", __func__, fru);
    return;
  }

  if (pal_is_fru_prsnt(fru, &present_status) < 0) {
    syslog(LOG_WARNING, "%s()  pal_is_fru_prsnt error\n", __func__);
    return;
  }

  if (present_status == FRU_ABSENT) {
    syslog(LOG_WARNING, "%s()  FRU: %s is not present, skip fruid reading\n", __func__, fru_name);
    return;
  }

  snprintf(fruid_temp_path, sizeof(fruid_temp_path), COMMON_TMP_FRU_PATH, fru_name);
  snprintf(fruid_path, sizeof(fruid_path), fru_path, fru_name);

  while(retry < MAX_RETRY_TO_GET_EXP_FRU) {
    ret = exp_read_fruid(fruid_temp_path, fru);
    if (ret != 0) {
      retry++;
      sleep(1);
    } else {
      if (retry == (MAX_RETRY_TO_GET_EXP_FRU - 1)) {
        log_level = LOG_CRIT;
      }
      // rename() can only handle src and dst path both within same mounted file system
      // Use light weight rename() to handle where the fruid_temp_path and fruid_path
      // within same mounted file system. If both path are on the different mounted filesystem
      // i.e. one is tempfs and one on persistent storage /mnt/data0 we have to use "mv" command.
      if (rename(fruid_temp_path, fruid_path) < 0) {
        snprintf(cmd, sizeof(cmd), "mv %s %s", fruid_temp_path, fruid_path);
        run_command(cmd);
      }

      // Check checksum of FRU after dump from expander.
      if (pal_check_fru_is_valid(fruid_path, log_level) < 0) {
        retry++;
        syslog(LOG_WARNING, "%s()  FRU: %s is invalid, retry: %d\n", __func__, fru_name, retry);
        sleep(1);
      } else {
        break;
      }
    }
  }

  if(retry == MAX_RETRY_TO_GET_EXP_FRU) {
    syslog(LOG_CRIT, "%s: exp_read_fruid failed with FRU: %s\n", __func__, fru_name);
  } else {
    syslog(LOG_INFO, "FRU: %s initial is done.\n", fru_name);
  }


}

void
fruid_cache_init(void) {
  int i = 0, ret = 0, retry = 0;
  uint8_t ver[FW_VERSION_LENS] = {0};
  uint8_t tbuf[MAX_IPMB_BUFFER] = {0x00};
  uint8_t rbuf[MAX_IPMB_BUFFER] = {0x00};
  uint8_t rlen = 0 , tlen = 0;
  char key[MAX_KEY_LEN] = {0};

  // Auto dump SCC and DPB
  for (i = 0; i < ARRAY_SIZE(expander_fruid_list); i++) {
    exp_read_fruid_wrapper(expander_fruid_list[i], COMMON_FRU_PATH);
  }

  // If use new EXP f/w, version > 17 (0x11)
  // Ask EXP about FAN FRU state before dump
  ret = expander_get_fw_ver(ver, sizeof(ver));
  if ((ret == 0) && (ver[3] >= 0x11)) {
    // If FAN FRU is ready, EXP will send SEL include FAN checksum
    ret = expander_ipmb_wrapper(NETFN_OEM_REQ, CMD_OEM_EXP_GET_FAN_FRU_STATUS, tbuf, tlen, rbuf, &rlen);
    if ((ret < 0) || (rbuf[0] != EXP_FAN_FRU_READY)) {
      syslog(LOG_WARNING, "%s(): Checking of FAN FRU is pending until EXP FAN FRU cache is ready.\n", __func__);
    }

  } else {
    for (i = 0; i < ARRAY_SIZE(expander_fan_fruid_list); i++) {
      exp_read_fruid_wrapper(expander_fan_fruid_list[i], COMMON_FAN_FRU_PATH);
      memset(key, 0, sizeof(key));
      snprintf(key, sizeof(key), "fan%d_dumped", i);
      kv_set(key, STR_VALUE_1, 0, 0);
    }
  }

  return;
}

void
sensor_timestamp_init(void) {
  // Initialize SCC and DPB sensor timestamp to 0
  char key[MAX_KEY_LEN] = {0};
  int ret = 0;

  snprintf(key, sizeof(key), "dpb_sensor_timestamp");
  ret = pal_set_cached_value(key, "0");
  if (ret != 0) {
    syslog(LOG_CRIT, "%s, failed to init %s, ret: %d", __func__, key, ret);
  }

  memset(key, 0, sizeof(key));
  snprintf(key, sizeof(key), "scc_sensor_timestamp");
  ret = pal_set_cached_value(key, "0");
  if (ret != 0) {
    syslog(LOG_CRIT, "%s, failed to init %s, ret: %d", __func__, key, ret);
  }
}

void
sensor_threshold_init(void) {
  int i = 0, ret = 0;
  uint8_t ver[FW_VERSION_LENS] = {0};

  // Support to get SCC/DPB threshold with expander f/w version 18
  ret = expander_get_fw_ver(ver, sizeof(ver));
  if ((ret == 0) && (ver[3] >= 0x12)) {
    // Get sensors' threshold of SCC and DPB
    for (i = 0; i < ARRAY_SIZE(expander_fruid_list); i++) {
      ret = pal_exp_sensor_threshold_init(expander_fruid_list[i]);
      if (ret < 0) {
        syslog(LOG_CRIT, "%s() failed to initialize sensors' threshold of FRU:%d \n", __func__, expander_fruid_list[i]);
      }
    }
  } else {
    syslog(LOG_CRIT, "%s() Not support to get SCC/DPB sensors' threshold from expander\n", __func__);
  }

  return;
}

int
main (int argc, char * const argv[])
{
  uint8_t fru_id = -1;

  if ((argc < 2) || (argc > 3)) {
    return -1;
  }

  if (strcmp(argv[1], "--booting") == 0) {
    sensor_timestamp_init();
    fruid_cache_init();
    sensor_threshold_init();

  } else if (strcmp(argv[1], "--update_fan") == 0) {
    if (strcmp(argv[2], "fan0") == 0) {
      fru_id = FRU_FAN0;
    } else if (strcmp(argv[2], "fan1") == 0) {
      fru_id = FRU_FAN1;
    } else if (strcmp(argv[2], "fan2") == 0) {
      fru_id = FRU_FAN2;
    } else if (strcmp(argv[2], "fan3") == 0) {
      fru_id = FRU_FAN3;
    } else {
      return -1;
    }
    exp_read_fruid_wrapper(fru_id, COMMON_FAN_FRU_PATH);
  }

  return 0;
}
