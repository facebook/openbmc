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
get_error_code() {
  uint8_t tbuf[256] = {0x00};
  uint8_t rbuf[256] = {0x00};
  uint8_t p_exp_error[256];
  uint8_t rlen = 0;
  uint8_t tlen = 0;
  int p_ret = 0;
  FILE *fp;
  uint8_t exp_error[13];
  uint8_t error[32];
  char index_tmp[8];
  int ret, i, count = 0;

  ret = expander_ipmb_wrapper(NETFN_OEM_REQ, EXPANDER_ERROR_CODE, tbuf, tlen, rbuf, &rlen);
  if (ret) {
    printf("IPMB Query Error...\n");
    #ifdef DEBUG
       syslog(LOG_WARNING, "enclosure-util: get_error_code, expander_ipmb_wrapper failed.");
    #endif
  }

  memcpy(exp_error, rbuf, rlen);

  fp = fopen("/tmp/error_code.bin", "r");
    if (!fp) {
      printf("fopen Fail: %s,  Error Code: %d\n", strerror(errno), errno);
      #ifdef DEBUG
        syslog(LOG_WARNING, "enclosure-util get_error_code, BMC error code File open failed...\n");
	    #endif
  }
  else {
    while (fscanf(fp, "%d", error+count) != EOF && count!=32) {    
      count++; 
    }
  }

  if(!ret && fp) {
    memcpy(error, exp_error, rlen - 1); //Not the last one (12th)
    error[12] = ((error[12] & 0xF0) + (exp_error[12] & 0xF));
    memset(p_exp_error,256,0);
    p_ret = OEM_BITMAP_FUN(error, p_exp_error);
    printf("Error Counter: %d\n", p_ret);
    for (i = 0; i < p_ret; i++) {
      printf("Error Code %d: ", p_exp_error[i]);
      if ( p_exp_error[i] < 100 )
        printf("%s", Error_Code_Description[p_exp_error[i]]);
      printf("\n");
    }
    printf("\n");
  }
  else if(ret && fp) {
    #ifdef DEBUG
      syslog(LOG_WARNING,"Only showing BMC Error Code 100~255, there is something wrong with Expander Error Code...\n");
    #endif
    memset(p_exp_error,256,0);
    p_ret = OEM_BITMAP_FUN(error, p_exp_error);
    printf("Error Counter: %d\n", p_ret);
    for (i = 0; i < p_ret; i++) {
      printf("Error Code %d: ", p_exp_error[i]);
      if ( p_exp_error[i] < 100 )
        printf("%s", Error_Code_Description[p_exp_error[i]]);
      printf("\n");
    }
    printf("\n");
  }
  else if(!fp && !ret) {
    #ifdef DEBUG
      syslog(LOG_WARNING, "Only showing Expander Error Code 0~99, there is something wrong with BMC Error Code...\n");
    #endif
    memset(p_exp_error,256,0);
    p_ret = OEM_BITMAP_FUN(exp_error, p_exp_error);
    printf("Error Counter: %d\n", p_ret);
    for (i = 0; i < p_ret; i++) {
      printf("Error Code %d: ", p_exp_error[i]);
      if ( p_exp_error[i] < 100 )
        printf("%s", Error_Code_Description[p_exp_error[i]]);
      printf("\n");
    }
    printf("\n");
  }
  else
     syslog(LOG_WARNING, "There is something wrong with the Error Codes...\n");
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
    get_error_code();
  else
    print_usage_help();

  return 0;
}

static int OEM_BITMAP_FUN(unsigned char* in_error,unsigned char* data)
{
  int ret = 0;
  int ii=0;
  int kk=0;
  int NUM = 0;
  if( data == NULL)
  {
    return 0;
  }
  for( ii = 0; ii < 32; ii++ )
  {

    for( kk = 0; kk < 8; kk++ )
    {
        if((( in_error[ii] >> kk )&0x01 ) == 1)
        {
          if( (data + ret) == NULL)
            return ret;
          NUM = ii*8 + kk;
            *(data + ret) = NUM;
          ret++;
        }
    }
  }
  return ret;
}
