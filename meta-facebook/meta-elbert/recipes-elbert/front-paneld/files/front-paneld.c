/*
 *
 * Copyright 2021-present Facebook. All Rights Reserved.
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
#include <signal.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <linux/limits.h>
#include <openbmc/kv.h>
#include <openbmc/ipmi.h>
#include <openbmc/pal.h>
#include <openbmc/pal_sensors.h>
#include <openbmc/misc-utils.h>
#include <openbmc/sdr.h>
#include <openbmc/obmc-i2c.h>

#define INTERVAL_MAX  5
#define PIM_RETRY     10

static int
pim_thresh_init_file_check(uint8_t fru) {
  char fru_name[32];
  char fpath[64];

  pal_get_fru_name(fru, fru_name);
  snprintf(fpath, sizeof(fpath), INIT_THRESHOLD_BIN, fru_name);

  return access(fpath, F_OK);
}

int
pim_thresh_init(uint8_t fru) {
  int i, sensor_cnt;
  uint8_t snr_num;
  uint8_t *sensor_list;
  thresh_sensor_t snr[MAX_NUM_FRUS][MAX_SENSOR_NUM] = {0};

  pal_get_fru_sensor_list(fru, &sensor_list, &sensor_cnt);
  for (i = 0; i < sensor_cnt; i++) {
    snr_num = sensor_list[i];
    if (sdr_get_snr_thresh(fru, snr_num, &snr[fru - 1][snr_num]) < 0) {
#ifdef DEBUG
      syslog(LOG_WARNING, "pim_thresh_init: sdr_get_snr_thresh for FRU: %d",
             fru);
#endif
      continue;
    }
  }

  if (access(THRESHOLD_PATH, F_OK) == -1) {
    mkdir(THRESHOLD_PATH, 0777);
  }

  if (pal_copy_all_thresh_to_file(fru, snr[fru - 1]) < 0) {
    syslog(LOG_WARNING, "%s: Fail to copy thresh to file for FRU: %d",
           __func__, fru);
    return -1;
  }

  return 0;
}

// Thread for monitoring pim plug
static void *
pim_monitor_handler(void *unused) {
  uint8_t fru;
  uint8_t num;
  uint8_t ret;
  uint8_t prsnt = 0;
  uint8_t prsnt_ori = 0;
  uint8_t curr_state = 0x00;
  uint8_t pim_type;
  uint8_t pim_type_old[10] = {PIM_TYPE_UNPLUG};
  uint8_t interval[10];
  bool thresh_first[10];

  memset(interval, INTERVAL_MAX, sizeof(interval));
  memset(thresh_first, true, sizeof(thresh_first));
  while (1) {
    for (fru = FRU_PIM2; fru <= FRU_PIM9; fru++) {
      ret = pal_is_fru_prsnt(fru, &prsnt);
      if (ret) {
        goto pim_mon_out;
      }
      /* Get original prsnt state PIM2 @bit0, PIM3 @bit1, ..., PIM9 @bit7 */
      num = fru - FRU_PIM2 + 2;
      prsnt_ori = GETBIT(curr_state, (num - 2));
      /* 1 is prsnt, 0 is not prsnt. */
      if (prsnt != prsnt_ori) {
        if (prsnt) {
          syslog(LOG_WARNING, "PIM %d is plugged in.", num);
          pim_type = pal_get_pim_type(fru, PIM_RETRY);

          if (pim_type != pim_type_old[num]) {
            if (pim_type == PIM_TYPE_16Q) {
              if (!pal_set_pim_type_to_file(fru, "16q")) {
                syslog(LOG_INFO, "PIM %d type is 16Q", num);
                pim_type_old[num] = PIM_TYPE_16Q;
              } else {
                syslog(LOG_WARNING,
                       "pal_set_pim_type_to_file: PIM %d set 16Q failed", num);
              }
            } else if (pim_type == PIM_TYPE_8DDM) {
              if (!pal_set_pim_type_to_file(fru, "8ddm")) {
                syslog(LOG_INFO, "PIM %d type is 8DDM", num);
                pim_type_old[num] = PIM_TYPE_8DDM;
              } else {
                syslog(LOG_WARNING,
                       "pal_set_pim_type_to_file: PIM %d set 8DDM failed", num);
              }
            } else {
              if (!pal_set_pim_type_to_file(fru, "none")) {
                syslog(LOG_CRIT,
                        "PIM %d type cannot detect, EEPROM get fail", num);
                pim_type_old[num] = PIM_TYPE_NONE;
              } else {
                syslog(LOG_WARNING,
                      "pal_set_pim_type_to_file: PIM %d set none failed", num);
              }
            }

            pal_set_sdr_update_flag(fru,1);
            if (thresh_first[num] == true) {
              thresh_first[num] = false;
            } else {
              if (pim_thresh_init_file_check(fru)) {
                pim_thresh_init(fru);
              } else {
                pal_set_pim_thresh(fru);
              }
            }
          }
        } else {
          syslog(LOG_WARNING, "PIM %d is unplugged", num);
          pal_clear_thresh_value(fru);
          pal_set_sdr_update_flag(fru,1);
          pim_type_old[num] = PIM_TYPE_UNPLUG;
          if (pal_set_pim_type_to_file(fru, "unplug")) {
            syslog(LOG_WARNING,
                  "pal_set_pim_type_to_file: PIM %d set unplug failed", num);
          }
        }
        /* Set PIM2 prsnt state @bit0, PIM3 @bit1, ..., PIM9 @bit7 */
        curr_state = prsnt ? SETBIT(curr_state, (num - 2))
                           : CLEARBIT(curr_state, (num - 2));
      }

      /* 1 is prsnt, 0 is not prsnt. */
      if (prsnt) {
        if (interval[num] == 0) {
          interval[num] = INTERVAL_MAX;
          msleep(50);
        } else {
          interval[num]--;
        }
      }
    }
pim_mon_out:
    sleep(1);
  }
  return NULL;
}

int
main (int argc, char * const argv[]) {
  pthread_t tid_pim_monitor;
  int rc;
  int pid_file;

  pid_file = open("/var/run/front-paneld.pid", O_CREAT | O_RDWR, 0666);
  rc = flock(pid_file, LOCK_EX | LOCK_NB);
  if (rc) {
    if (EWOULDBLOCK == errno) {
      printf("Another front-paneld instance is running...\n");
      exit(-1);
    }
  } else {
    openlog("front-paneld", LOG_CONS, LOG_DAEMON);
  }

  if ((rc = pthread_create(&tid_pim_monitor, NULL,
                           pim_monitor_handler, NULL))) {
    syslog(LOG_WARNING, "failed to create pim monitor thread: %s",
           strerror(rc));
    exit(1);
  }

  pthread_join(tid_pim_monitor, NULL);

  return 0;
}
