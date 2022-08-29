/*
 * fpc-util
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

#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <unistd.h>
#include <openbmc/pal.h>

static void
print_usage_help(void) {
  printf("Usage: fpc-util <sled> --identify <on/off>\n");
}

int
main(int argc, char **argv) {
  char tstr[64] = {0};

  if (argc < 3) {
    goto err_exit;
  }

  if (strcmp(argv[1], "sled") || strcmp(argv[2], "--identify")) {
    goto err_exit;
  }

  if (strcmp(argv[3], "on") != 0 && strcmp(argv[3], "off") != 0) {
    goto err_exit;
  }

  if (argc != 4) {
    goto err_exit;
  }

  printf("fpc-util: identification for %s is %s\n", argv[1], argv[3]);

  sprintf(tstr, "identify_sled");

  return pal_set_key_value(tstr, argv[3]);

err_exit:
  print_usage_help();
  return -1;
}
