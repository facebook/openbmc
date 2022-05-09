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
#include <sys/un.h>
#include <sys/file.h>
#include <openbmc/ipmi.h>
#include <openbmc/ipmb.h>
#include <openbmc/pal.h>
#include <facebook/bic.h>


#define LAST_RECORD_ID      0xFFFF
#define BYTES_ENTIRE_RECORD 0xFF
#define BIC_SERVER_FRUID    0x0

int
fruid_cache_init(void) {
  // Initialize server's fruid
  int ret = 0;
  int fru_size = 0;
  char fruid_temp_path[MAX_PATH_LEN] = {0};
  char fruid_path[MAX_PATH_LEN] = {0};
  int retry = 0;

  snprintf(fruid_temp_path, MAX_PATH_LEN, "/tmp/tfruid_server.bin");
  snprintf(fruid_path, MAX_PATH_LEN, "/tmp/fruid_server.bin");

  do {
    ret = bic_read_fruid(BIC_SERVER_FRUID, fruid_temp_path, &fru_size);
    retry++;
    sleep(1);
  } while ((ret != 0) && (retry < BIC_MAX_RETRY));

  if (retry == BIC_MAX_RETRY) {
    syslog(LOG_CRIT, "Fail on getting Server FRU.\n");
  } else {
    syslog(LOG_INFO, "Server FRU initial is done.\n");
  }

  rename(fruid_temp_path, fruid_path);

  if ((ret = pal_check_fru_is_valid(fruid_path, LOG_CRIT)) < 0) {
    syslog(LOG_WARNING, "%s() The FRU %s is wrong.", __func__, fruid_path);
  }

  return ret;
}

int
sdr_cache_init() {
  int ret = 0;
  int fd = 0;
  int retry = 0;
  uint8_t rlen = 0;
  uint8_t rbuf[MAX_IPMB_RES_LEN] = {0};
  char *path = NULL;
  char sdr_temp_path[MAX_PATH_LEN] = {0};
  char sdr_path[MAX_PATH_LEN] = {0};
  ssize_t bytes_wr = 0;

  snprintf(sdr_temp_path, MAX_PATH_LEN, "/tmp/tsdr_server.bin");
  snprintf(sdr_path, MAX_PATH_LEN, "/tmp/sdr_server.bin");

  ipmi_sel_sdr_req_t req = {0};
  ipmi_sel_sdr_res_t *res = (ipmi_sel_sdr_res_t *) rbuf;

  req.rsv_id = 0;
  req.rec_id = 0;
  req.offset = 0;
  req.nbytes = BYTES_ENTIRE_RECORD;

  path = sdr_temp_path;
  unlink(path);
  fd = open(path, O_WRONLY | O_CREAT | O_EXCL, 0666);
  if (fd < 0) {
    syslog(LOG_WARNING, "%s: open fails for path: %s\n", __func__, path);
    return -1;
  }

  ret = pal_flock_retry(fd);
  if (ret == -1) {
   syslog(LOG_WARNING, "%s: failed to flock on %s", __func__, path);
   close(fd);
   return ret;
  }

  retry = 0;
  do {
    ret = bic_get_sdr(&req, res, &rlen);
    if (ret != 0) {
      syslog(LOG_WARNING, "%s: bic_get_sdr returns %d, rsv_id: 0x%x, record_id: 0x%x, sdr_size: %d\n", __func__, ret, req.rsv_id, req.rec_id, rlen);
      retry++;
      sleep(1);
      continue;
    }

    sdr_full_t *sdr = (sdr_full_t *)res->data;

    bytes_wr = write(fd, sdr, sizeof(sdr_full_t));
    if (bytes_wr != sizeof(sdr_full_t)) {
      syslog(LOG_ERR, "%s: write sdr failed\n", __func__);
      return -1;
    }

    req.rec_id = res->next_rec_id;
    if (req.rec_id == LAST_RECORD_ID) {
      break;
    }
    retry++;
    sleep(1);
  } while (retry < BIC_MAX_RETRY);

  if (retry == BIC_MAX_RETRY) {   // if exceed 3 mins, exit this step
    syslog(LOG_CRIT, "Fail on getting Server SDR.\n");
  } else {
    syslog(LOG_INFO, "Get Server SDR success.\n");   
  }

  ret = pal_unflock_retry(fd);
  if (ret == -1) {
   syslog(LOG_WARNING, "%s: failed to unflock on %s", __func__, path);
   close(fd);
   return ret;
  }

  close(fd);
  rename(sdr_temp_path, sdr_path);

  return ret;
}

int
main (int argc, char * const argv[])
{
  int ret = 0, retry = 0;
  uint8_t self_test_result[2] = {0};

  do {
    ret = bic_get_self_test_result((uint8_t *)&self_test_result);
    if (ret == 0) {
      syslog(LOG_INFO, "bic_get_self_test_result: %X %X", self_test_result[0], self_test_result[1]);
      break;
    }
    retry++;
    sleep(5);
  } while ((ret != 0) && (retry < MAX_RETRY)); // each time wait 5 second, max wait 3 times
  
  if (retry == MAX_RETRY) {
    syslog(LOG_WARNING, "bic is wrong because bic get self test result is fail.");
  }

  fruid_cache_init();
  sdr_cache_init();

  return 0;
}
