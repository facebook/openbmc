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

uint8_t expander_fruid_list[] = {FRU_SCC, FRU_DPB, FRU_FAN0, FRU_FAN1, FRU_FAN2, FRU_FAN3};

static void
exp_read_fruid_wrapper(uint8_t fru) {
  int ret = 0;
  char fruid_temp_path[MAX_FRU_PATH] = {0};
  char fruid_path[MAX_FRU_PATH] = {0};
  char fru_name[MAX_FRU_CMD_STR] = {0};
  int retry = 0;
  uint8_t present_status = 0;

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

  snprintf(fruid_temp_path, sizeof(fruid_temp_path), "/tmp/tfruid_%s.bin", fru_name);
  snprintf(fruid_path, sizeof(fruid_path), "/tmp/fruid_%s.bin", fru_name);

  while(retry < MAX_RETRY) {
    ret = exp_read_fruid(fruid_temp_path, fru);
    if (ret != 0) {
      retry++;
      msleep(20);
    } else {
      break;
    }
  }

  if(retry == MAX_RETRY) {
    syslog(LOG_CRIT, "%s: exp_read_fruid failed with FRU: %s\n", __func__, fru_name);
  } else {
    syslog(LOG_INFO, "FRU: %s initial is done.\n", fru_name);
  }

  rename(fruid_temp_path, fruid_path);

  if(pal_check_fru_is_valid(fruid_path) < 0) {
    syslog(LOG_WARNING, "%s() The FRU %s is wrong.", __func__, fruid_path);
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
    exp_read_fruid_wrapper(expander_fruid_list[i]);
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

int
main (int argc, char * const argv[])
{
  fruid_cache_init();
  sensor_timestamp_init();
  return 0;
}
