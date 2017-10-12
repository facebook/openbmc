/*
 *
 * Copyright 2015-present Facebook. All Rights Reserved.
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

void
fruid_cache_init(void) {
  // Initialize Slot0's fruid
  int ret;
  int i;
  char fruid_temp_path[64] = {0};
  char fruid_path[64] = {0};
  int retry = 0;

  sprintf(fruid_temp_path, "/tmp/tfruid_scc.bin");
  sprintf(fruid_path, "/tmp/fruid_scc.bin");

  while(retry < 5) {
    ret = exp_read_fruid(fruid_temp_path, FRU_SCC);
    if (ret != 0) {
      retry++;
      msleep(20);
    }     
    else
      break;
  }

  if(retry == 5)
    syslog(LOG_CRIT, "%s: exp_read_fruid failed with FRU:%d\n", __func__, FRU_SCC);
  else
    syslog(LOG_INFO, "SCC FRU initial is done.\n");

  rename(fruid_temp_path, fruid_path);

  retry = 0;
  sprintf(fruid_temp_path, "/tmp/tfruid_dpb.bin");
  sprintf(fruid_path, "/tmp/fruid_dpb.bin");

  //delay 3 seconds between for two continuous commands
  sleep(3);

  while(retry < 5) {
    ret = exp_read_fruid(fruid_temp_path, FRU_DPB);
    if (ret != 0) {
      retry++;
      msleep(20);
    }     
    else
      break;
  }

  if(retry == 5)
    syslog(LOG_CRIT, "%s: exp_read_fruid failed with FRU:%d\n", __func__, FRU_DPB);
  else
    syslog(LOG_INFO, "DPB FRU initial is done.\n");

  rename(fruid_temp_path, fruid_path);
  return;
}

void
sensor_timestamp_init(void) {
  //Initialize SCC and DPB sensor timestamp to 0
  char key[MAX_KEY_LEN] = {0};
  int ret = 0;

  sprintf(key, "dpb_sensor_timestamp");
  ret = pal_set_edb_value(key, "0");
  if (ret != 0) {
    syslog(LOG_CRIT, "%s, failed to init %s, ret: %d", __func__, key, ret);
  }

  sprintf(key, "scc_sensor_timestamp");
  ret = pal_set_edb_value(key, "0");
  if (ret != 0) {
    syslog(LOG_CRIT, "%s, failed to init %s, ret: %d", __func__, key, ret);
  }
}

int
main (int argc, char * const argv[])
{
  sensor_timestamp_init();
  fruid_cache_init();
  return 0;
}
