/*
 * enclosure-util

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
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <facebook/exp.h>
#include <openbmc/ipmb.h>
#include <openbmc/pal.h>
#include "enclosure.h"

#define LOGFILE "/tmp/enclosure-util.log"

static int OEM_BITMAP_FUN(unsigned char* in_error,unsigned char* data);

static void
print_usage_help(void) {
  printf("Usage: enclosure-util --hdd-status\n");
  printf("Usage: enclosure-util --hdd-status <number 0~35>\n");
  printf("Usage: enclosure-util --error\n");
}

void
get_hdd_status(int hdd_number) {
  uint8_t tbuf[256] = {0x00};
  uint8_t rbuf[256] = {0x00};
  uint8_t rlen = 0;
  uint8_t tlen = 0;
  char good[108] = {0};
  char missing[108] = {0};
  char error[108] = {0};
  char index_tmp[8];
  int ret, i;

  ret = expander_ipmb_wrapper(NETFN_OEM_REQ, EXPANDER_HDD_STATUS, tbuf, tlen, rbuf, &rlen);
  if (ret) {
    printf("IPMB Query Error...\n");
    #ifdef DEBUG
       syslog(LOG_WARNING, "enclosure-util: get_hdd_status failed.");
    #endif
    return -1;
  }

  if (hdd_number == -1) {
    for(i = 0; i <36; i++) {
      if ( (rbuf[4*i] & 0xF) == 0x1 ) {
        sprintf(index_tmp, "%d", i);
        strcat(good, index_tmp);
        strcat(good, " ");
      }
      else if ( (rbuf[4*i] & 0xF) == 0x5 ) {
        sprintf(index_tmp, "%d", i);
        strcat(missing, index_tmp);
        strcat(missing, " ");
      }
      else {
        sprintf(index_tmp, "%d", i);
        strcat(error, index_tmp);
        strcat(error, " ");
      }
    }
    printf("Normal Drives: %s\n", good);
    printf("Abnormal Drives: %s\n", error);
    printf("Missing Drives: %s\n", missing);
  }
  else {
      printf("Drive_%d Status: %02X %02X %02X %02X\n", hdd_number, rbuf[hdd_number*4], rbuf[hdd_number*4+1], rbuf[hdd_number*4+2], rbuf[hdd_number*4+3]);
  }
}

void 
show_error_code() {
  uint8_t error[256], count = 0;
  int i;

  pal_get_error_code(error, &count);
  
  if (count != 0){
    printf("Error Counter: %d\n", count);

    for (i = 0; i < count; i++) {
      printf("Error Code %d: ", error[i]);
      if ( error[i] < 100 ) //now only support Expander Error String 0~99
        printf("%s", Error_Code_Description[error[i]]);
      printf("\n");
    }
  }
  else
    printf("No Error!\n");
}

int
main(int argc, char **argv) {
  int hdd_number = -1;

  if (argc < 2) {
    print_usage_help();
    return -1;
  }

  if (argc == 3) {
    hdd_number = atoi(argv[2]);
    if ( (hdd_number > 35) || (hdd_number < 0) ){
      print_usage_help();
      return -1;
    }
  }

  // Get the command parameters.
  if (!strcmp(argv[1], "--hdd-status"))
    get_hdd_status(hdd_number);
  else if (!strcmp(argv[1], "--error"))
    show_error_code();
  else
    print_usage_help();

  return 0;
}