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
  printf("Usage: guid-util <slot1|slot2|slot3|slot4|spb> <dev|sys> <--get>\n");
  printf("       guid-util <slot1|slot2|slot3|slot4|spb> <dev|sys> <--set> <uid>\n");
}

int
main(int argc, char **argv) {
  int i = 0;
  char guid[16] = {0};
  uint8_t slot_id;
  uint8_t status;
  uint8_t ret;

  // Check for border conditions
  if ((argc != 4) && (argc != 5)) {
    goto err_exit;
  }

  // Derive slot_id from first parameter
  if (!strcmp(argv[1], "slot1")) {
    slot_id = FRU_SLOT1;
  } else if (!strcmp(argv[1] , "slot2")) {
    slot_id = FRU_SLOT2;
  } else if (!strcmp(argv[1] , "slot3")) {
    slot_id = FRU_SLOT3;
  } else if (!strcmp(argv[1] , "slot4")) {
    slot_id = FRU_SLOT4;
  } else if (!strcmp(argv[1] , "spb")) {
    slot_id = FRU_SPB;
  } else {
    goto err_exit;
  }

  switch (slot_id) {
    case FRU_SLOT1:
    case FRU_SLOT2:
    case FRU_SLOT3:
    case FRU_SLOT4:
      // Check SLOT present and SLOT type
      ret = pal_is_fru_prsnt(slot_id, &status);
      if (ret < 0) {
        printf("pal_is_fru_prsnt failed for fru: %d\n", slot_id);
        goto err_exit;
      }
      if (status == 0) {
        printf("slot%d is empty!\n", slot_id);
        goto err_exit;
      }

      if(!pal_is_slot_server(slot_id)) {
        printf("slot%d is not server\n", slot_id);
        goto err_exit;
      }

      if (!strcmp(argv[2], "dev")) {
        printf("There is only system GUID on each slots.\n");
        goto err_exit;
      }      
      break;
    case FRU_SPB:
      if (!strcmp(argv[2], "sys")) {
        printf("There is only device GUID on spb.\n");
        goto err_exit;
      }
      break;
  }

  // Handle GUID get condition
  if (!strcmp(argv[3], "--get")) {
    if (!strcmp(argv[2], "dev")) {
      if (pal_get_dev_guid(slot_id, guid)) {
        goto err_exit;
      }
    } else if (!strcmp(argv[2], "sys")) {
      if (pal_get_sys_guid(slot_id, guid)) {
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
  if (!strcmp(argv[3], "--set")) {
    // Check if all 5 arguments are present
    if (argc != 5) {
      goto err_exit;
    }

    if (!strcmp(argv[2], "dev")) {
      // Handle dev GUID
      if (pal_set_dev_guid(slot_id, argv[4])) {
        goto err_exit;
      }
    } else if (!strcmp(argv[2], "sys")) {
      // Handle Sys GUID
      if (pal_set_sys_guid(slot_id, argv[4])) {
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
