/*
 * yosemite-sensors
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
#include <syslog.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <stdbool.h>
#include <openbmc/pal.h>
#include <openbmc/sdr.h>

#define STATUS_OK   "ok"
#define STATUS_NS   "ns"
#define STATUS_UNC  "unc"
#define STATUS_UCR  "ucr"
#define STATUS_UNR  "unr"
#define STATUS_LNC  "lnc"
#define STATUS_LCR  "lcr"
#define STATUS_LNR  "lnr"

static int
print_usage() {
  printf("Usage: sensor-util [ %s ] <--threshold> <sensor num>\n",
      pal_fru_list);
}

static void
print_sensor_reading(float fvalue, uint8_t snr_num, thresh_sensor_t *thresh,
    bool threshold, char *status) {

  printf("%-28s (0x%X) : %7.2f %-5s | (%s)",
      thresh->name, snr_num, fvalue, thresh->units, status);
  if (threshold) {

    printf(" | UCR: ");
    thresh->flag & GETMASK(UCR_THRESH) ?
      printf("%.2f", thresh->ucr_thresh) : printf("NA");

    printf(" | UNC: ");
    thresh->flag & GETMASK(UNC_THRESH) ?
      printf("%.2f", thresh->unc_thresh) : printf("NA");

    printf(" | UNR: ");
    thresh->flag & GETMASK(UNR_THRESH) ?
      printf("%.2f", thresh->unr_thresh) : printf("NA");

    printf(" | LCR: ");
    thresh->flag & GETMASK(LCR_THRESH) ?
      printf("%.2f", thresh->lcr_thresh) : printf("NA");

    printf(" | LNC: ");
    thresh->flag & GETMASK(LNC_THRESH) ?
      printf("%.2f", thresh->lnc_thresh) : printf("NA");

    printf(" | LNR: ");
    thresh->flag & GETMASK(LNR_THRESH) ?
      printf("%.2f", thresh->lnr_thresh) : printf("NA");

  }

  printf("\n");
}

static void
get_sensor_status(float fvalue, thresh_sensor_t *thresh, char *status) {

  if (thresh->flag == 0)
    sprintf(status, STATUS_NS);
  else
    sprintf(status, STATUS_OK);

  if (GETBIT(thresh->flag, UNC_THRESH) && (fvalue >= thresh->unc_thresh))
    sprintf(status, STATUS_UNC);
  if (GETBIT(thresh->flag, UCR_THRESH) && (fvalue >= thresh->ucr_thresh))
    sprintf(status, STATUS_UCR);
  if (GETBIT(thresh->flag, UNR_THRESH) && (fvalue >= thresh->unr_thresh))
    sprintf(status, STATUS_UNR);
  if (GETBIT(thresh->flag, LNC_THRESH) && (fvalue <= thresh->lnc_thresh))
    sprintf(status, STATUS_LNC);
  if (GETBIT(thresh->flag, LCR_THRESH) && (fvalue <= thresh->lcr_thresh))
    sprintf(status, STATUS_LCR);
  if (GETBIT(thresh->flag, LNR_THRESH) && (fvalue <= thresh->lnr_thresh))
    sprintf(status, STATUS_LNR);
}

static void
get_sensor_reading(uint8_t fru, uint8_t *sensor_list, int sensor_cnt, int num,
    bool threshold) {

  int i;
  uint8_t snr_num;
  float fvalue;
  char path[64];
  char status[8];
  thresh_sensor_t thresh;

  for (i = 0; i < sensor_cnt; i++) {

    snr_num = sensor_list[i];

    /* If calculation is for a single sensor, ignore all others. */
    if (num && snr_num != num) {
      continue;
    }

    if (sdr_get_snr_thresh(fru, snr_num, &thresh))
      syslog(LOG_ERR, "sdr_init_snr_thresh failed for FRU %d num: 0x%X", fru, snr_num);

    usleep(50);

    if (pal_sensor_read(fru, snr_num, &fvalue) < 0) {
      printf("%-28s (0x%X) : NA | (na)\n", thresh.name, sensor_list[i]);
      continue;
    } else {
      get_sensor_status(fvalue, &thresh, status);
      print_sensor_reading(fvalue, snr_num, &thresh, threshold, status);
    }
  }
}

int
main(int argc, char **argv) {

  int i;
  int ret;
  int sensor_cnt;
  uint8_t status;
  uint8_t *sensor_list;
  uint8_t fru;
  uint8_t num = 0;
  char fruname[16];
  bool threshold = false;

  if (argc < 2 || argc > 4) {
    print_usage();
    exit(-1);
  }

  i = 3; /* Starting at argument 3*/
  while (argc > 2 && i <= argc) {
    if (!(strcmp(argv[i-1], "--threshold"))) {
      threshold = true;
    } else {
      errno = 0;
      num = (uint8_t) strtol(argv[i-1], NULL, 0);
      if (errno) {
        print_usage();
        exit(-1);
      }
    }
    i++;
  }


  ret = pal_get_fru_id(argv[1], &fru);
  if (ret < 0) {
    print_usage();
    return ret;
  }

  if (fru == 0) {
    fru = 1;
    while (fru <= MAX_NUM_FRUS) {

      ret = pal_is_fru_prsnt(fru, &status);
      if (ret < 0) {
        printf("pal_is_server_fru failed for fru: %d\n", fru);
      }

      if (status == 0) {
        if (pal_get_fru_name(fru, fruname))
          fruname[0] = '\0';
        printf("%s is empty!\n\n", fruname);
        fru++;
        continue;
      }

      ret = pal_is_fru_ready(fru, &status);
      if ((ret < 0) || (status == 0)) {
        if (pal_get_fru_name(fru, fruname))
          fruname[0] = '\0';
        printf("%s is unavailable!\n", fruname);
        fru++;
        continue;
      }

      ret = pal_get_fru_sensor_list(fru, &sensor_list, &sensor_cnt);
      if (ret < 0) {
        return ret;
      }

      get_sensor_reading(fru, sensor_list, sensor_cnt, num, threshold);

      fru++;
      printf("\n");
    }
  } else {

    ret = pal_is_fru_prsnt(fru, &status);
    if (ret < 0) {
      printf("pal_is_fru_prsnt failed for fru: %d\n", fru);
      print_usage();
      exit(-1);
    }
    if (status == 0) {
      printf("%s is empty!\n", argv[1]);
      print_usage();
      exit(-1);
    }

    ret = pal_is_fru_ready(fru, &status);
    if ((ret < 0) || (status == 0)) {
      printf("%s is unavailable!\n", argv[1]);
      return ret;
    }

    ret = pal_get_fru_sensor_list(fru, &sensor_list, &sensor_cnt);
    if (ret < 0) {
      return ret;
    }

    get_sensor_reading(fru, sensor_list, sensor_cnt, num, threshold);
  }

  return 0;
}
