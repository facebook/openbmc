/*
 * expander-util
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
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <syslog.h>
#include <stdint.h>
#include <ctype.h>
#include <sys/time.h>
#include <time.h>

#include <facebook/expander.h>
#include <openbmc/pal.h>


static void
print_usage_help(void) {
  printf("Usage: expander-util scb <[0..n]data_bytes_to_send> \n");
}


static int
process_command(uint8_t fru_id, int argc, char **argv) {
  int i, ret, retry = 2;
  uint8_t tbuf[256] = {0x00};
  uint8_t rbuf[256] = {0x00};
  uint8_t tlen = 0;
  uint8_t rlen = 0;
  long val = 0;
  char *endptr;

  for (i = 0; i < argc; i++) {
    errno = 0;
    val = strtol(argv[i], &endptr, 0);
    if(*endptr != '\0') {
      printf("invalid raw data: %s \n", endptr);
      print_usage_help();
      return -1;
    }
    if((val < 0) || (val > 255)) {
      printf("invalid raw data \n");
      print_usage_help();
      return -1;
    }
    tbuf[tlen++] = (uint8_t)val;
  }

  while (retry >= 0) {
    ret = exp_ipmb_wrapper(fru_id, tbuf[0]>>2, tbuf[1], &tbuf[2], tlen-2, rbuf, &rlen);
    if (ret == 0)
      break;

    retry--;
  }
  if (ret) {
    printf("Expander no response!\n");
    return ret;
  }

  for (i = 0; i < rlen; i++) {
    if (!(i % 16) && i)
      printf("\n");

    printf("%02X ", rbuf[i]);
  }
  printf("\n");

  return 0;
}


int
main(int argc, char **argv) {
  uint8_t fru_id = 0;
  int ret = 0;

  if (argc < 3) {
    print_usage_help();
    return -1;
  }
  
  ret = pal_get_fru_id(argv[1], &fru_id);
  if (ret < 0) {
    printf("%s is invalid!\n", argv[1]);
    goto err_exit;
  }

  if (argc >= 4) {
    return process_command(fru_id, (argc - 2), (argv + 2));
  } else {
    goto err_exit;
  }

err_exit:
  print_usage_help();
  return -1;
}
