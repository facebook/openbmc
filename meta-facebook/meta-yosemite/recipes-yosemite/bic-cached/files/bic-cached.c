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
#include <facebook/bic.h>

#define LAST_RECORD_ID 0xFFFF
#define MAX_SENSOR_NUM 0xFF
#define BYTES_ENTIRE_RECORD 0xFF

static int
_fruid_check(const char *bin) {
  FILE *fruid_fp;
  int fruid_len, ret = -1;
  uint16_t area_offs, area_len;
  uint8_t sum, i, area;
  uint8_t *fruid_data, *p_buf;

  /* Open the FRUID binary file */
  fruid_fp = fopen(bin, "rb");
  if (!fruid_fp) {
    return ENOENT;
  }

  /* Get the size of the binary file */
  fseek(fruid_fp, 0, SEEK_END);
  fruid_len = ftell(fruid_fp);
  if (fruid_len <= 0) {
    fclose(fruid_fp);
    return -1;
  }

  fseek(fruid_fp, 0, SEEK_SET);
  fruid_data = (uint8_t *)malloc(fruid_len);
  if (!fruid_data) {
    fclose(fruid_fp);
    return ENOMEM;
  }

  /* Read the binary file */
  fread(fruid_data, sizeof(uint8_t), fruid_len, fruid_fp);
  fclose(fruid_fp);

  /* Check Common Header */
  sum = 0;
  for (i = 0; i < 8; i++) {
    sum += fruid_data[i];
  }
  if (sum || (fruid_data[0] != 0x01)) {
    free(fruid_data);
    return -1;
  }

  /* Check Chassis/Board/Product Info Area */
  for (area = 2; area <= 4; area++) {
    if (fruid_data[area]) {
      area_offs = fruid_data[area] * 8;
      if (area_offs >= fruid_len) {
        break;
      }

      area_len = fruid_data[area_offs + 1] * 8;
      if ((area_offs + area_len) >= fruid_len) {
        break;
      }

      sum = 0;
      p_buf = fruid_data + area_offs;
      for (i = 0; i < area_len; i++) {
        sum += p_buf[i];
      }
      if (sum || (p_buf[0] != 0x01)) {
        break;
      }
    }
  }
  if (area > 4) {
    ret = 0;
  }
  free(fruid_data);

  return ret;
}

int
fruid_cache_init(uint8_t slot_id) {
  // Initialize Slot0's fruid
  int ret=0;
  int fru_size=0;
  char fruid_temp_path[64] = {0};
  char fruid_path[64] = {0};

  sprintf(fruid_temp_path, "/tmp/tfruid_slot%d.bin", slot_id);
  sprintf(fruid_path, "/tmp/fruid_slot%d.bin", slot_id);

  ret = bic_read_fruid(slot_id, 0, fruid_temp_path, &fru_size);
  if (ret) {
    syslog(LOG_WARNING, "fruid_cache_init: bic_read_fruid returns %d, fru_size: %d\n", ret, fru_size);
  }

  if (!ret) {
    ret = _fruid_check(fruid_temp_path);
    if (ret) {
      syslog(LOG_WARNING, "fruid_cache_init: _fruid_check returns %d, fru_size: %d\n", ret, fru_size);
    }
  }

  rename(fruid_temp_path, fruid_path);

  return ret;
}

void
sdr_cache_init(uint8_t slot_id) {
  int ret;
  int fd;
  uint8_t rlen;
  uint8_t rbuf[MAX_IPMB_RES_LEN] = {0};
  char *path = NULL;
  char sdr_temp_path[64] = {0};
  char sdr_path[64] = {0};

  sprintf(sdr_temp_path, "/tmp/tsdr_slot%d.bin", slot_id);
  sprintf(sdr_path, "/tmp/sdr_slot%d.bin", slot_id);

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
    ret = bic_get_sdr(slot_id, &req, res, &rlen);
    if (ret) {
      syslog(LOG_WARNING, "sdr_cache_init:bic_get_sdr returns %d\n", ret);
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

  // Check BIC Self Test Result
  do {
    ret = bic_get_self_test_result(slot_id, &self_test_result);
    if (ret == 0) {
      syslog(LOG_INFO, "bic_get_self_test_result: %X %X\n", self_test_result[0], self_test_result[1]);
      break;
    }
    sleep(5);
  } while (ret != 0);

  // Get Server FRU
  do {
    if (fruid_cache_init(slot_id) == 0)
      break;

    retry++;
    sleep(1);
  } while (retry < max_retry);

  if (retry == max_retry)
    syslog(LOG_CRIT, "Fail on getting Server FRU.");

  sdr_cache_init(slot_id);

  return 0;
}
