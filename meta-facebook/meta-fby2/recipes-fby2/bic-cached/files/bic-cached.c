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
#include <getopt.h>
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
#include <facebook/bic.h>
#include <openbmc/pal.h>
#include <facebook/fby2_common.h>


#define LAST_RECORD_ID 0xFFFF
#define MAX_SENSOR_NUM 0xFF
#define BYTES_ENTIRE_RECORD 0xFF

#define MAX_RETRY 6           // 180 secs = 1500 * 6
#define MAX_RETRY_CNT 1500    // A senond can run about 50 times, 30 secs = 30 * 50

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
    syslog(LOG_WARNING, "fruid_cache_init: bic_read_fruid slot%d returns %d, fru_size: %d\n", slot_id, ret, fru_size);
  }

  rename(fruid_temp_path, fruid_path);

  return ret;
}

bool
is_nvme_temp_dev(uint8_t sensor_num) {
  uint8_t NVMe_Temp_dev[] = {GPV2_SENSOR_DEV0_Temp, GPV2_SENSOR_DEV1_Temp, GPV2_SENSOR_DEV2_Temp, GPV2_SENSOR_DEV3_Temp, 
                             GPV2_SENSOR_DEV4_Temp, GPV2_SENSOR_DEV5_Temp, GPV2_SENSOR_DEV6_Temp, GPV2_SENSOR_DEV7_Temp, 
                             GPV2_SENSOR_DEV8_Temp, GPV2_SENSOR_DEV9_Temp, GPV2_SENSOR_DEV10_Temp, GPV2_SENSOR_DEV11_Temp};
  int i = 0;
  int sensor_cnt = 0;

  sensor_cnt = sizeof(NVMe_Temp_dev) / sizeof(NVMe_Temp_dev[0]);
  for (i = 0; i < sensor_cnt; i++) {
    if (sensor_num == NVMe_Temp_dev[i]) {
      return true;
    }
  }
  return false;
}

int
sdr_cache_init(uint8_t slot_id) {
  int ret = 0, rc;
  int fd;
  int retry = 0;
  int spb_type = 0;
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
    syslog(LOG_WARNING, "%s: open fails for path: %s\n", __func__, path);
    ret = -2;
    return ret;
  }

  rc = pal_flock_retry(fd);
  if (rc == -1) {
   syslog(LOG_WARNING, "%s: failed to flock on %s", __func__, path);
   close(fd);
   return rc;
  }

  retry = 0;
  do {
    ret = bic_get_sdr(slot_id, &req, res, &rlen);
    if (ret) {
      syslog(LOG_WARNING, "%s: bic_get_sdr returns %d, rsv_id: 0x%x, record_id: 0x%x, sdr_size: %d\n", __func__, ret, req.rsv_id, req.rec_id, rlen);
      retry++;
      continue;
    }

    sdr_full_t *sdr = (sdr_full_t *)res->data;

    spb_type = fby2_common_get_spb_type();
    if ((spb_type == TYPE_SPB_YV250) && ((slot_id == FRU_SLOT1) || (slot_id == FRU_SLOT3))) {
      if (is_nvme_temp_dev(sdr->sensor_num) == true) {
        sdr->uc_thresh = YV250_NVMe_Temp_Dev_UCR;
      }
    }

    if (write(fd, sdr, sizeof(sdr_full_t)) != sizeof(sdr_full_t)){
      syslog(LOG_WARNING, "%s: write file failed, file: %s", __func__, path);
      close(fd);
      return -1;
    }

    req.rec_id = res->next_rec_id;
    if (req.rec_id == LAST_RECORD_ID) {
      // syslog(LOG_INFO, "This record is LAST record\n");
      break;
    }
    retry++;
  } while (retry < MAX_RETRY_CNT);
  if (retry == MAX_RETRY_CNT) {   // if exceed 30 secs, exit this step
    syslog(LOG_WARNING, "Fail on getting Slot%u SDR via BIC", slot_id);
  }

  rc = pal_unflock_retry(fd);
  if (rc == -1) {
   syslog(LOG_WARNING, "%s: failed to unflock on %s", __func__, path);
   close(fd);
   return rc;
  }

  close(fd);
  rename(sdr_temp_path, sdr_path);

  return ret;
}

int
parse_args(int argc, char * const argv[], bool *do_fru, bool *do_sdr) {
  int opt;
  int optind_long;
  uint8_t fru = 0, sdr = 0;
  static const char *optstring = "fs";
  static const struct option long_options[] = {
    {"fruid", no_argument, 0, 'f'},
    {"sdr", no_argument, 0, 's'},
    {0, 0, 0, 0}
  };

  while ((opt = getopt_long(argc, argv, optstring, long_options, &optind_long)) != -1) {
    switch (opt) {
      case 'f':
        fru = 1;
        break;
      case 's':
        sdr = 1;
        break;
      default:
        return -1;
    }
  }

  if (argc < optind + 1) {
    return -1;
  }

  if (fru ^ sdr) {
    if (fru) {
      *do_fru = true;
      *do_sdr = false;
    } else {
      *do_fru = false;
      *do_sdr = true;
    }
  } else {
    *do_fru = true;
    *do_sdr = true;
  }

  return optind;
}

int
main (int argc, char * const argv[])
{
  int ret;
  int pid_file;
  uint8_t slot_id;
  uint8_t self_test_result[2]={0};
  int retry = 0;
  int max_retry = 3;
  char path[128];
  bool fruid_dump = true, sdr_dump = true;

  ret = parse_args(argc, argv, &fruid_dump, &sdr_dump);
  if ((ret < 0) || (ret >= argc)) {
    printf("Usage: bic-cached [Option] <SLOT_NUM>\n");
    printf("\n");
    printf("           Option:\n");
    printf("                 -f update FRU cache\n");
    printf("                 -s update SDR cache\n");
    return -1;
  }

  slot_id = atoi(argv[ret]);
  if ((slot_id < FRU_SLOT1) || (slot_id > FRU_SLOT4)) {
    return -1;
  }

  sprintf(path, BIC_CACHED_PID, slot_id);
  pid_file = open(path, O_WRONLY | O_CREAT | O_EXCL, 0666);
  if (pid_file < 0) {
    syslog(LOG_WARNING, "%s: fails to open path: %s\n", __func__, path);
  }

  ret = pal_flock_retry(pid_file);
  if (ret) {
   syslog(LOG_WARNING, "%s: failed to flock on %s", __func__, path);
  }

  // Check BIC Self Test Result
  do {
    ret = bic_get_self_test_result(slot_id, (uint8_t *)&self_test_result);
    if (ret == 0) {
      syslog(LOG_INFO, "bic_get_self_test_result of slot%u: %X %X", slot_id, self_test_result[0], self_test_result[1]);
      break;
    }
    sleep(5);
  } while (ret != 0);

  if (sdr_dump == true) {
    pal_set_update_sdr_flag(slot_id,1);
    retry = 0;
    do {
      ret = sdr_cache_init(slot_id);
      if (ret == 0)
        break;

      retry++;
      sleep(1);
    } while ((ret != 0) && (retry < MAX_RETRY));

    if (ret != 0) {   // if exceed 3 mins, exit this step
      syslog(LOG_CRIT, "Fail on getting Slot%u SDR", slot_id);
    }
    pal_set_update_sdr_flag(slot_id,0);
  }

  // Get Server FRU
  if (fruid_dump == true) {
    retry = 0;
    do {
      ret = fruid_cache_init(slot_id);
      if (ret == 0)
        break;

      retry++;
      sleep(1);
    } while (retry < max_retry);

    if (ret != 0) {
      syslog(LOG_CRIT, "Fail on getting Slot%u FRU", slot_id);
    }
  }

  ret = pal_unflock_retry(pid_file);
  if (ret == -1) {
   syslog(LOG_WARNING, "%s: failed to unflock on %s", __func__, path);
  }

  close(pid_file);
  remove(path);
  return 0;
}
