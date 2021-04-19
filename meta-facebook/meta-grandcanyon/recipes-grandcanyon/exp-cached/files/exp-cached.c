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
}

void
fruid_cache_init(void) {
  int i = 0;
  for (i = 0; i < ARRAY_SIZE(expander_fruid_list); i++) {
    exp_read_fruid_wrapper(expander_fruid_list[i]);
  }
  return;
}

int
main (int argc, char * const argv[])
{
  fruid_cache_init();
  return 0;
}
