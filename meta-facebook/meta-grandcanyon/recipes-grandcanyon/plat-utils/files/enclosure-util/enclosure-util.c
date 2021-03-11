/*
 * enclosure-util
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
#include <stdint.h>
#include <string.h>
#include <facebook/fbgc_common.h>
#include <openbmc/pal.h>
#include <facebook/exp.h>
#include "enclosure.h"

static void
print_usage_help(void) {
  printf("Usage: enclosure-util --error\n");
  printf("         Expander error code range: 1~100\n");
  printf("         BMC error code range:      0x64~0xFF\n");
}


void
show_error_code() {
  uint8_t error[MAX_NUM_ERR_CODES] = {0}, error_count = 0;
  int i = 0, ret = 0;
  
  memset(error, 0, sizeof(error));
  
  ret = pal_get_error_code(error, &error_count);
  if (ret < 0) {
    printf("enclosure-util: fail to get error code\n");
    return;
  }
  
  if (error_count == 0){
    printf("Error Counter: 0 (No Error)\n");
  } else {
    printf("Error Counter: %d\n", error_count);

    for (i = 0; i < error_count; i++) {
      // Expander error: 0~99
      if (error[i] < MAX_NUM_EXP_ERR_CODES) {
        printf("Error Code %0d: ", error[i]);
        printf("%s", exp_error_code_description[error[i]]);
      
      // BMC error: 0x64(100)~0xFF(255)
      } else {
        printf("Error Code 0x%0X: ", error[i]);
        printf("%s", bmc_error_code_description[error[i] - BMC_ERR_CODE_START_NUM]);
      }
      printf("\n");
    }
  }
}

int
main(int argc, char **argv) {
  if (argv == NULL) {
    syslog(LOG_WARNING, "fail to execute enclosure-util because NULL parameter: **argv");
    return -1;
  }
  
  if (argc == 2) {
    if (strcmp(argv[1], "--error") == 0) {
      show_error_code();
    }
    return 0;
  }
  
  print_usage_help();
  return -1;
}
