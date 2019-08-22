/*
 * threshold-util
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
#include <stdint.h>
#include <pthread.h>
#include <string.h>
#include <openbmc/pal.h>
#include <openbmc/sdr.h>

enum {
  UCR = 0x01,
  UNC,
  UNR,
  LCR,
  LNC,
  LNR,
};

static void
print_usage_help(void) {
  printf("Usage: threshold-util [fru] <--set> <snr_num> [thresh_type] <threshold_value>\n");
  printf("       threshold-util [fru] <--clear>\n");
  printf("       [fru]           : %s\n", pal_fru_list);
  printf("       <snr_num>    : 0xXX\n");
  printf("       [thresh_type]   : UCR, UNC, UNR, LCR, LNC, LNR\n");
}

static int
is_fru_valid(uint8_t fru) {
  int ret;
  uint8_t status;
  char fruname[16] = {0};

  if (fru == FRU_ALL)
    return 0;

  ret = pal_get_fru_name(fru, fruname);
  if (ret < 0) {
    printf("%s: Fail to get fru%d name\n",__func__,fru);    
    return ret;
  }

  ret = pal_is_fru_prsnt(fru, &status);
  if (ret < 0) {
    printf("pal_is_fru_prsnt failed for fru: %s\n", fruname);
    return ret;
  }
  if (status == 0) {
    printf("%s is empty!\n", fruname);
    return -1;
  }

  ret = pal_is_fru_ready(fru, &status);
  if ((ret < 0) || (status == 0)) {
    printf("%s is unavailable!\n", fruname);
    return ret;
  }

  return 0;
}

static int
clear_thresh_value_setting(uint8_t fru) {
  int ret;
  char fpath[64] = {0};
  char cmd[128] = {0};
  char fruname[16] = {0};

  ret = pal_get_fru_name(fru, fruname);
  if (ret < 0) {
    printf("%s: Fail to get fru%d name\n",__func__,fru);
    return ret;
  }
  
  memset(fpath, 0, sizeof(fpath));
  sprintf(fpath, THRESHOLD_BIN, fruname);
  sprintf(cmd,"rm -rf %s",fpath);
  system(cmd);

  memset(fpath, 0, sizeof(fpath));
  sprintf(fpath, THRESHOLD_RE_FLAG, fruname);
  sprintf(cmd,"touch %s",fpath);
  system(cmd); 

  return 0;
}

int
main(int argc, char **argv) {
  uint8_t fru;
  uint8_t snr_num = 0;
  uint8_t thresh_type = 0;
  float threshold_value;
  int ret = -1;
  char *end = NULL;

  // Check for border conditions
  if ((argc != 3) && (argc != 6)) {
    print_usage_help();
    return ret;
  }

  ret = pal_get_fru_id(argv[1], &fru);
  if (ret < 0) {
    print_usage_help();
    return ret;
  }

  ret = is_fru_valid(fru);
  if (ret < 0) {
    print_usage_help();
    return ret;
  }

  if (!(strcmp(argv[2], "--set"))) {
    if (!strcmp(argv[4], "UCR")) {
      thresh_type = UCR;
    } else if (!strcmp(argv[4] , "UNC")) {
      thresh_type = UNC;
    } else if (!strcmp(argv[4] , "UNR")) {
      thresh_type = UNR;
    } else if (!strcmp(argv[4] , "LCR")) {
      thresh_type = LCR;
    } else if (!strcmp(argv[4] , "LNC")) {
      thresh_type = LNC;
    } else if (!strcmp(argv[4] , "LNR")) {
      thresh_type = LNR;
    } else {
      print_usage_help();
      return -1;
    }

    errno = 0;
    ret = strtol(argv[3], &end, 0);
    if (errno || *end || (ret < 0) || (ret > 0xFF)) {
      print_usage_help();
      return -1;
    }
    snr_num = (uint8_t)ret;

    errno = 0;
    threshold_value = strtof(argv[5], &end);
    if (errno || *end) {
      print_usage_help();
      return -1;
    }

    if (FRU_ALL == fru) { // For FRU ALL
      for (fru = FRU_ALL+1; fru <= MAX_NUM_FRUS; fru++) {
        if (!pal_is_sensor_existing(fru, snr_num)) {
          printf("Could not find sensor 0x%x for fru%d\n", snr_num, fru);
          continue;
        }

        ret = pal_sensor_thresh_modify(fru, snr_num, thresh_type, threshold_value);
        if (ret < 0)
          printf("Fail to set sensor 0x%x threshold for fru%d\n", snr_num, fru);
      }
    } else {
      if (!pal_is_sensor_existing(fru, snr_num)) {
        printf("Could not find sensor 0x%x for fru%d\n", snr_num, fru);
        return 0;
      }
      ret = pal_sensor_thresh_modify(fru, snr_num, thresh_type, threshold_value);
      if (ret < 0)
        printf("Fail to set sensor 0x%x threshold for fru%d\n", snr_num, fru);
    }
  } else if (!(strcmp(argv[2], "--clear"))) {
    if (FRU_ALL == fru) { // For FRU ALL
      ret = 0;
      for (fru = FRU_ALL+1; fru <= MAX_NUM_FRUS; fru++) {
        ret |= clear_thresh_value_setting(fru);
        if (ret < 0) {
          printf("Fail to clear threshold for fru%d\n", fru);
        }
      }
    } else {
      ret = clear_thresh_value_setting(fru);
      if (ret < 0) {
        printf("Fail to clear threshold for fru%d\n", fru);
      }
    }
  } else {
    print_usage_help();
    return -1;
  }

  return 0;
}
