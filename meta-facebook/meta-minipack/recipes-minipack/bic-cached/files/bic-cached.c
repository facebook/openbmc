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
#include <sys/file.h>
#include <openbmc/log.h>
#include <openbmc/ipmi.h>
#include <openbmc/ipmb.h>
#include <openbmc/obmc-pal.h>
#include <facebook/bic.h>


#define LAST_RECORD_ID 0xFFFF
#define MAX_SENSOR_NUM 0xFF
#define BYTES_ENTIRE_RECORD 0xFF

int
fruid_cache_init(uint8_t slot_id) {

  int ret = 0;
  int fru_size=0;
  char fruid_temp_path[64];
  char fruid_path[64];
  char fru_name[32];

  pal_get_fru_name(slot_id + 1, fru_name);
  sprintf(fruid_temp_path, "/tmp/tfruid_%s.bin", fru_name);
  sprintf(fruid_path, "/tmp/fruid_%s.bin", fru_name);

  ret = bic_read_fruid(slot_id, 0, fruid_temp_path, &fru_size);
  if (ret) {
    syslog(LOG_WARNING, "fruid_cache_init: bic_read_fruid returns %d, fru_size: %d\n", ret, fru_size);
  }

  rename(fruid_temp_path, fruid_path);

  return ret;
}

void
sdr_cache_init(uint8_t slot_id) {
  int ret;
  int fd;
  uint8_t rlen;
  uint8_t rbuf[MAX_IPMB_RES_LEN];
  char *path = NULL;
  char sdr_temp_path[64];
  char sdr_path[64];
  char fru_name[32];

  pal_get_fru_name(slot_id + 1, fru_name);
  sprintf(sdr_temp_path, "/tmp/tsdr_%s.bin", fru_name);
  sprintf(sdr_path, "/tmp/sdr_%s.bin", fru_name);

  ipmi_sel_sdr_req_t req;
  ipmi_sel_sdr_res_t *res = (ipmi_sel_sdr_res_t *) rbuf;

  req.rsv_id = 0;
  req.rec_id = 0;
  req.offset = 0;
  req.nbytes = BYTES_ENTIRE_RECORD;

  /* Read SCM's SDR records and store */
  path = sdr_temp_path;
  unlink(path);
  fd = open(path, O_WRONLY | O_CREAT | O_EXCL, 0666);
  if (fd < 0) {
    syslog(LOG_WARNING, "%s: open fails for path: %s\n", __func__, path);
    return;
  }

  ret = pal_flock_retry(fd);
  if (ret == -1) {
   syslog(LOG_WARNING, "%s: failed to flock on %s", __func__, path);
   close(fd);
   return;
  }

  while (1) {
    ret = bic_get_sdr(slot_id, &req, res, &rlen);
    if (ret) {
      syslog(LOG_WARNING, "%s: bic_get_sdr returns %d\n", __func__, ret);
      continue;
    }

    sdr_full_t *sdr = (sdr_full_t *)res->data;

    ret = write(fd, sdr, sizeof(sdr_full_t));
    if (ret < 0) {
      OBMC_ERROR(errno, "write %s failed", path);
      break;
    } else if (ret != sizeof(sdr_full_t)) {
      OBMC_WARN("data truncated (write %s): expect %i, actual %d\n",
                path, sizeof(sdr_full_t), ret);
      /*
       * XXX shall we exit or continue??
       */
    }

    req.rec_id = res->next_rec_id;
    if (req.rec_id == LAST_RECORD_ID) {
      // syslog(LOG_INFO, "This record is LAST record\n");
      break;
    }
  }

  ret = pal_unflock_retry(fd);
  if (ret == -1) {
   syslog(LOG_WARNING, "%s: failed to unflock on %s", __func__, path);
   close(fd);
   return;
  }

  close(fd);
  rename(sdr_temp_path, sdr_path);
}

int
main (int argc, char * const argv[])
{
  int ret;
  uint8_t slot_id;
  uint8_t self_test_result[2]={0};
  int retry = 0;
  int max_retry = 3;

  if (argc != 2) {
    return -1;
  }

  slot_id = atoi(argv[1]);

  /* Check BIC Self Test Result */
  do {
    ret = bic_get_self_test_result(slot_id, (uint8_t *)&self_test_result);
    if (ret == 0) {
      syslog(LOG_INFO, "bic_get_self_test_result: %X %X\n", self_test_result[0], self_test_result[1]);
      break;
    }
    sleep(5);
  } while (ret != 0);

  /* Get uServer FRU */
  do {
    if (fruid_cache_init(slot_id) == 0)
      break;

    retry++;
    sleep(1);
  } while (retry < max_retry);

  if (retry == max_retry)
    syslog(LOG_CRIT, "Fail on getting uServer FRU.");

  sdr_cache_init(slot_id);

  return 0;
}
