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

#define I2C_DEV_FLASH7 "/dev/i2c-7"
#define I2C_DEV_FLASH8 "/dev/i2c-8"

static void
print_usage_help(void) {
  int sku = 0;
  sku = pal_get_iom_type();

  printf("Usage: enclosure-util --hdd-status\n");
  printf("Usage: enclosure-util --hdd-status <number 0~35>\n");
  printf("Usage: enclosure-util --error\n");
  if (sku == IOM_M2) {
    printf("Usage: enclosure-util --flash-health\n");
    printf("Usage: enclosure-util --flash-status\n");
  }
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
    printf("Error: IPMB Query Error...\n");
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
  uint8_t error[MAX_ERROR_CODES], count = 0;
  int i;

  pal_get_error_code(error, &count);
  
  if (count != 0){
    printf("Error Counter: %d\n", count);

    for (i = 0; i < count; i++) {
      if ( error[i] < EXP_ERROR_CODE_NUM ) { //now only support Expander Error String 0~99
        printf("Error Code %d: ", error[i]);
        printf("%s", Expander_Error_Code_Description[error[i]]);
      } else if ( error[i] >= BMC_ERROR_CODE_START ) {
        printf("Error Code %x: ", error[i]);
        printf("%s", BMC_Error_Code_Description[error[i] - BMC_ERROR_CODE_START]);
      }
      printf("\n");
    }
  }
  else
    printf("Error Counter: 0 (No Error)\n");
}

void 
show_drive_health() {
  char bus[32];
  int ret;
  
  /* read NVMe-MI data */  
  ret = pal_drive_health(I2C_DEV_FLASH7);
  if(ret < 0) {
    syslog(LOG_DEBUG, "%s(): bus7, pal_drive_health failed", __func__);
  }
  printf("flash-1: %s\n", (ret == 0) ? "Normal":"Abnormal");

  /* read NVMe-MI data */  
  ret = pal_drive_health(I2C_DEV_FLASH8);
  if(ret < 0) {
    syslog(LOG_DEBUG, "%s(): bus8, pal_drive_health failed 8", __func__);
  }
  printf("flash-2: %s\n", (ret == 0) ? "Normal":"Abnormal");
}

void 
show_drive_status() {
  char bus[32];
  int ret;

  /* read NVMe-MI data */
  printf("flash-1:\n");
  ret = pal_drive_status(I2C_DEV_FLASH7);
  if(ret < 0) {
    syslog(LOG_DEBUG, "%s(): pal_drive_status failed 7", __func__);
  }

  /* read NVMe-MI data */
  printf("flash-2:\n");
  ret = pal_drive_status(I2C_DEV_FLASH8);
  if(ret < 0) {
    syslog(LOG_DEBUG, "%s(): pal_drive_status failed 8", __func__);
  }
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
  else if (!strcmp(argv[1], "--flash-health"))
    show_drive_health();
  else if (!strcmp(argv[1], "--flash-status"))
    show_drive_status();
  else
    print_usage_help();

  return 0;
}
