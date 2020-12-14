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

void
fruid_cache_init(void) {
  int ret = 0;
  char fruid_temp_path[MAX_FRU_PATH] = {0};
  char fruid_path[MAX_FRU_PATH] = {0};
  int retry = 0;

  snprintf(fruid_temp_path, sizeof(fruid_temp_path), "/tmp/tfruid_scc.bin");
  snprintf(fruid_path, sizeof(fruid_path), "/tmp/fruid_scc.bin");

  while(retry < MAX_RETRY) {
    ret = exp_read_fruid(fruid_temp_path, FRU_SCC);
    if (ret != 0) {
      retry++;
      msleep(20);
    }
    else {
      break;
    }
  }

  if(retry == MAX_RETRY) {
    syslog(LOG_CRIT, "%s: exp_read_fruid failed with FRU:%d\n", __func__, FRU_SCC);
  } else {
    syslog(LOG_INFO, "SCC FRU initial is done.\n");
  }

  rename(fruid_temp_path, fruid_path);

  retry = 0;
  snprintf(fruid_temp_path, sizeof(fruid_temp_path), "/tmp/tfruid_dpb.bin");
  snprintf(fruid_path, sizeof(fruid_path), "/tmp/fruid_dpb.bin");

  //delay 3 seconds between for two continuous commands
  sleep(3);

  while(retry < MAX_RETRY) {
    ret = exp_read_fruid(fruid_temp_path, FRU_DPB);
    if (ret != 0) {
      retry++;
      msleep(20);
    } else {
      break;
    }
  }

  if(retry == MAX_RETRY) {
    syslog(LOG_CRIT, "%s: exp_read_fruid failed with FRU:%d\n", __func__, FRU_DPB);
  } else {
    syslog(LOG_INFO, "DPB FRU initial is done.\n");
  }

  rename(fruid_temp_path, fruid_path);
  return;
}

int
main (int argc, char * const argv[])
{
  fruid_cache_init();
  return 0;
}
