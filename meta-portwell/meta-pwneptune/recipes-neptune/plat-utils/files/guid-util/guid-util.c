/*
 * guid-util
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

static void
print_usage_help(void) {
  printf("Usage: guid-util <dev|sys> <--get>\n");
  printf("       guid-util <dev|sys> <--set> <uid>\n");
}

int
main(int argc, char **argv) {
  int i = 0;
  char guid[16] = {0};

  // Check for border conditions
  if ((argc != 3) && (argc != 4)) {
    goto err_exit;
  }

  // Handle GUID get condition
  if (!strcmp(argv[2], "--get")) {
    if (!strcmp(argv[1], "dev")) {
      if (pal_get_dev_guid(1, guid)) {
        goto err_exit;
      }
    } else if (!strcmp(argv[1], "sys")) {
      if (pal_get_sys_guid(1, guid)) {
        goto err_exit;
      }
    } else {
        goto err_exit;
    }

    for (i = 0; i < 16; i++) {
      printf("0x%0X:", guid[i]);
    }
    printf("\n");
    return 0;
  }

  // Handle set case
  if (!strcmp(argv[2], "--set")) {
    // Check if all 4 arguments are present
    if (argc != 4) {
      goto err_exit;
    }

    if (!strcmp(argv[1], "dev")) {
      // Handle dev GUID
      if (pal_set_dev_guid(1, argv[3])) {
        goto err_exit;
      }
    } else if (!strcmp(argv[1], "sys")) {
      // Handle Sys GUID
      if (pal_set_sys_guid(1, argv[3])) {
        goto err_exit;
      }
    } else {
        goto err_exit;
    }

    return 0;
  }
err_exit:
  print_usage_help();
  return -1;
}
