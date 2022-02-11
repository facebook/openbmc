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
#include <openbmc/me.h>

#define LAST_RECORD_ID 0xFFFF
#define MAX_SENSOR_NUM 0xFF
#define BYTES_ENTIRE_RECORD 0xFF

void
fruid_cache_init() {
  int ret;
  int i;
  char fruid_temp_path[64] = {0};
  char fruid_path[64] = {0};

  sprintf(fruid_temp_path, "/tmp/tfruid_me.bin");
  sprintf(fruid_path, "/tmp/fruid_me.bin");

  ret = me_read_fruid(0, fruid_temp_path);
  if (ret) {
    syslog(LOG_WARNING, "fruid_cache_init: me_read_fruid returns %d\n", ret);
  }

  rename(fruid_temp_path, fruid_path);

  return;
}

void
sdr_cache_init() {
  int ret;
  int fd;
  uint8_t rlen;
  uint8_t rbuf[MAX_IPMB_RES_LEN] = {0};
  char *path = NULL;
  char sdr_temp_path[64] = {0};
  char sdr_path[64] = {0};

  sprintf(sdr_temp_path, "/tmp/tsdr_me.bin");
  sprintf(sdr_path, "/tmp/sdr_me.bin");

  ipmi_sel_sdr_req_t req;
  ipmi_sel_sdr_res_t *res = (ipmi_sel_sdr_res_t *) rbuf;

  req.rsv_id = 0;
  req.rec_id = 0;
  req.offset = 0;
  req.nbytes = BYTES_ENTIRE_RECORD;

  // Read Slot0's SDR records and store
  path = sdr_temp_path;
  unlink(path);
  fd = open(path, O_WRONLY | O_CREAT | O_EXCL, 0666);
  if (fd < 0) {
    syslog(LOG_WARNING, "sdr_cache_init: open fails for path: %s\n", path);
    return;
  }

  while (1) {
    ret = me_get_sdr(&req, res, &rlen);
    if (ret) {
      syslog(LOG_WARNING, "sdr_cache_init:me_get_sdr returns %d\n", ret);
      continue;
    }

    sdr_full_t *sdr = res->data;

    write(fd, sdr, sizeof(sdr_full_t));

    req.rec_id = res->next_rec_id;
    if (req.rec_id == LAST_RECORD_ID) {
      // syslog(LOG_INFO, "This record is LAST record\n");
      break;
    }
  }

  rename(sdr_temp_path, sdr_path);
}

int
main (void)
{
  int ret;
  ipmi_dev_id_t id = {0};

  do {
    ret = me_get_dev_id(&id);
    sleep(5);
  } while (ret != 0);

  fruid_cache_init();
  sdr_cache_init();

  return 0;
}
