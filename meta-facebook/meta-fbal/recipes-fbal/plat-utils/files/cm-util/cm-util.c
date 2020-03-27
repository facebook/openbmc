/*
 * cm-util
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
  printf("Usage : cm-util --set --mode <slot> <do_cycle>\n");
  printf("      : cm-util sled-cycle\n");
  printf("<slot>     : the slot to be master\n");
  printf("             [0..3] if 8S mode, 4 if 2S mode\n");
  printf("<do_cycle> : 1 if do power-cycle to apply configuration\n");
  printf("             0 otherwise\n");
}

int
main(int argc, char **argv) {
  bool do_cycle;
  uint8_t slot;
  uint8_t mode;

  if (argc != 2 && argc != 5)
    goto err_exit;

  if (!strcmp(argv[1], "--set") && !strcmp(argv[2], "--mode")) {
    slot = (uint8_t)atoi(argv[3]);
    if (slot < 0 || slot > ALL_2S_MODE)
      goto err_exit;

    do_cycle = atoi(argv[4])? true: false;

    if (slot == ALL_2S_MODE) {
      printf("set system mode to 2S\n");
      mode = MB_2S_MODE;
    } else {
      printf("set system mode to 8S and master is slot %d\n", (int)slot);
      mode = MB_8S_MODE;
      // Reverse the index
      slot = ~slot & 0x3;
    }

    if (pal_ep_set_system_mode(mode) < 0)
      printf("Set JBOG system mode failed\n");
    if (cmd_cmc_set_system_mode(slot, false) < 0)
      printf("Set CM system mode failed\n");

    if (do_cycle) {
      if (pal_ep_sled_cycle() < 0)
	printf("Request JBOG sled-cycle failed\n");

      cmd_cmc_sled_cycle();
    } else {
      printf("Do \"cm-util sled-cycle\" to enable the new configuration\n");
    }
  } else if (!strcmp(argv[1], "sled-cycle")) {
    if (pal_ep_sled_cycle() < 0)
      printf("Request JBOG sled-cycle failed\n");

    cmd_cmc_sled_cycle();
  } else {
    goto err_exit;
  }

  return 0;
err_exit:
  print_usage_help();
  return -1;
}
