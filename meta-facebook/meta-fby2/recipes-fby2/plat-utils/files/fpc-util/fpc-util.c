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

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <syslog.h>
#include <stdint.h>
#include <pthread.h>
#include <facebook/bic.h>
#include <openbmc/pal.h>
#include <openbmc/ipmi.h>

static void
print_usage_help(void) {
  printf("Usage: fpc-util <slot1|slot2|slot3|slot4> --usb\n");
  printf("       fpc-util --usb --reset\n");
  printf("       fpc-util <slot1|slot2|slot3|slot4|sled> --identify <on/off>\n");
}

int
main(int argc, char **argv) {

  uint8_t slot_id;
  uint8_t pos;
  char tstr[64] = {0};

  if (argc < 3) {
    goto err_exit;
  }

  if (!strcmp(argv[1], "slot1")) {
    slot_id = 1;
  } else if (!strcmp(argv[1] , "slot2")) {
    slot_id = 2;
  } else if (!strcmp(argv[1] , "slot3")) {
    slot_id = 3;
  } else if (!strcmp(argv[1] , "slot4")) {
    slot_id = 4;
  } else if (!strcmp(argv[1] , "sled")) {
    slot_id = 0;
  } else if (!strcmp(argv[1] , "--usb")) {
    if (!strcmp(argv[2], "--reset")) {
      if (pal_get_hand_sw_physically(&pos)) {
        goto err_exit;
      }
      printf("fpc-util: switching USB channel to knob position\n");
      return pal_switch_usb_mux(pos);
    }
    else {
      goto err_exit;
    }
  } else {
    goto err_exit;
  }

  if (!strcmp(argv[2], "--usb")) {
    if ((slot_id < 1) || (slot_id > 4))
      goto err_exit;

    printf("fpc-util: switching USB channel to slot%d\n", slot_id);
    return pal_switch_usb_mux(slot_id);
  } else if (!strcmp(argv[2], "--identify")) {
    if (argc != 4) {
      goto err_exit;
    }
    if (strcmp(argv[3] , "on") && strcmp(argv[3] , "off")) {
      goto err_exit;
    }
    printf("fpc-util: identification for %s is %s\n", argv[1], argv[3]);
    if (slot_id == 0) {
      sprintf(tstr, "identify_sled");
    } else {
      sprintf(tstr, "identify_slot%d", slot_id);
    }

    return pal_set_key_value(tstr, argv[3]);
  } else {
    goto err_exit;
  }

  return 0;
err_exit:
  print_usage_help();
  return -1;
}
