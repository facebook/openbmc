/*
 * ipmi-util
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
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <openbmc/ipmi.h>
#include <openbmc/pal.h>

static void
print_usage_help(void) {
  printf("Usage: ipmi-util <node#> <[0..n]data_bytes_to_send>\n");
}

int
main(int argc, char **argv) {
  uint8_t tbuf[256] = {0x00};
  uint8_t rbuf[256] = {0x00};
  uint8_t tlen = 0;
  uint16_t rlen = 0;
  int i;

  if (argc < 3) {
    goto err_exit;
  }

  if (!pal_is_slot_server((uint8_t)strtoul(argv[1], NULL, 0))) {
    printf("Node: %s is not a valid server\n", argv[1]);
    goto err_exit;
  }

  for (i = 1; i < argc; i++) {
    tbuf[tlen++] = (uint8_t)strtoul(argv[i], NULL, 0);
  }

  // Invoke IPMB library handler
  lib_ipmi_handle(tbuf, tlen, rbuf, &rlen);

  if (rlen == 0) {
    printf("lib_ipmi_handle returned zero bytes\n");
    return -1;
  }

  // Print the response
  for (i = 0; i < rlen; i++) {
    printf("%02X ", rbuf[i]);
  }
  printf("\n");

  return 0;
err_exit:
  print_usage_help();
  return -1;
}
