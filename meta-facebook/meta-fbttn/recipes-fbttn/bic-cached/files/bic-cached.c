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
#include <openbmc/ipmi.h>
#include <openbmc/ipmb.h>
#include <openbmc/pal.h>
#include <facebook/bic.h>

#define LAST_RECORD_ID 0xFFFF
#define MAX_SENSOR_NUM 0xFF
#define BYTES_ENTIRE_RECORD 0xFF
#define MAX_RETRY 180         // 1 s * 180 = 3 mins
#define MAX_RETRY_CNT 9000    // A senond can run about 50 times, 3 mins = 180 * 50
#define SERVER_SDR_DONE_FILE "/tmp/sdr_server.done"

int
fruid_cache_init(uint8_t slot_id) {
  // Initialize server's fruid
  int ret = 0;
  int fru_size = 0;
  char fruid_temp_path[64] = {0};
  char fruid_path[64] = {0};

  sprintf(fruid_temp_path, "/tmp/tfruid_server.bin");
  sprintf(fruid_path, "/tmp/fruid_server.bin");

  ret = bic_read_fruid(slot_id, 0, fruid_temp_path, &fru_size);
  if (ret) {
    syslog(LOG_WARNING, "fruid_cache_init: bic_read_fruid returns %d, fru_size: %d\n", ret, fru_size);
  }

  rename(fruid_temp_path, fruid_path);

  return ret;
}

int
sdr_cache_init(uint8_t slot_id) {
  int ret = 0;
  int fd;
  int retry = 0;
  uint8_t rlen;
  uint8_t rbuf[MAX_IPMB_RES_LEN] = {0};
  char *path = NULL;
  char sdr_temp_path[64] = {0};
  char sdr_path[64] = {0};

  sprintf(sdr_temp_path, "/tmp/tsdr_server.bin");
  sprintf(sdr_path, "/tmp/sdr_server.bin");

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
    syslog(LOG_WARNING, "%s: open fails for path: %s\n", __func__, path);
    ret = -2;
    return ret;
  }

  ret = pal_flock_retry(fd);
  if (ret == -1) {
   syslog(LOG_WARNING, "%s: failed to flock on %s", __func__, path);
   close(fd);
   return ret;
  }

  retry = 0;
  do {
    ret = bic_get_sdr(slot_id, &req, res, &rlen);
    if (ret) {
      syslog(LOG_WARNING, "%s: bic_get_sdr returns %d, rsv_id: 0x%x, record_id: 0x%x, sdr_size: %d\n", __func__, ret, req.rsv_id, req.rec_id, rlen);
      retry++;
      continue;
    }

    sdr_full_t *sdr = res->data;

    write(fd, sdr, sizeof(sdr_full_t));

    req.rec_id = res->next_rec_id;
    if (req.rec_id == LAST_RECORD_ID) {
      // syslog(LOG_INFO, "This record is LAST record\n");
      break;
    }
    retry++;
  } while (retry < MAX_RETRY_CNT);
  if (retry == MAX_RETRY_CNT) {   // if exceed 3 mins, exit this step
    syslog(LOG_CRIT, "Fail on getting Server SDR via BIC.\n");
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
  int ret = -1;
  int retry = 0;
  uint8_t slot_id;
  uint8_t self_test_result[2] = {0};
  uint8_t bic_ready_val = BIC_NOT_READY;
  char cmd[128];

  if (argc != 2) {
    return -1;
  }

  slot_id = atoi(argv[1]);

  // Check BIC Self Test Result
  while (1) {
    ret = bic_get_self_test_result(slot_id, &self_test_result);
    if (ret == 0) {

      // Check BIC is ready
      ret = pal_is_bic_ready(slot_id, &bic_ready_val);
      if ((ret == 0) && (bic_ready_val == BIC_READY)) {
        syslog(LOG_INFO, "bic_get_self_test_result: %02X %02X, "
            "BIC is ready: %d\n", self_test_result[0],
            self_test_result[1], bic_ready_val);
        break;
      }
    }
    sleep(5);
  }

  // Get Server SDR
  retry = 0;
  do {
    ret = sdr_cache_init(slot_id);
    retry++;
    sleep(1);
  } while ((ret != 0) && (retry < MAX_RETRY));
  if (retry == MAX_RETRY) {   // if exceed 3 mins, exit this step
    syslog(LOG_CRIT, "Fail on getting Server SDR.\n");
  } else {
    syslog(LOG_INFO, "Server SDR initial is done.\n");
    memset(cmd, 0, sizeof(cmd));
    sprintf(cmd, "touch %s", SERVER_SDR_DONE_FILE);
    system(cmd);
  }

  // Get Server FRU
  retry = 0;
  do {
    ret = fruid_cache_init(slot_id);
    retry++;
    sleep(1);
  } while ((ret != 0) && (retry < MAX_RETRY));
  if (retry == MAX_RETRY) {
    syslog(LOG_CRIT, "Fail on getting Server FRU.\n");
  } else {
    syslog(LOG_INFO, "Server FRU initial is done.\n");
  }

  return 0;
}
