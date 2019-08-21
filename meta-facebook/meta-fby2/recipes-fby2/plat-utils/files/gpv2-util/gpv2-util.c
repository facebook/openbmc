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
#include <facebook/bic.h>
#include <openbmc/pal.h>

#define MAX_READ_RETRY 10
#define MAX_GPV2_DRIVE_NUM 12

static void
print_usage_help(void) {
  printf("Usage: gpv2-util <slot1|slot3> --device-list\n");
}

int
main(int argc, char **argv) {

  uint8_t slot_id;
  uint8_t dev_id;
  uint8_t status;
  uint8_t type;
  uint8_t nvme_ready;
  uint8_t ffi;
  uint8_t meff;
  uint16_t vendor_id = 0;
  uint8_t major_ver;
  uint8_t minor_ver;
  uint8_t retry = MAX_READ_RETRY;
  int ret;

  if (argc < 3) {
    goto err_exit;
  }

  if (!strcmp(argv[1], "slot1")) {
    slot_id = 1;
  } else if (!strcmp(argv[1] , "slot3")) {
    slot_id = 3;
  } else {
    goto err_exit;
  }

  if (!strcmp(argv[2], "--device-list")) {
    if (argc != 3) {
      goto err_exit;
    }
    for (dev_id = 1; dev_id <= MAX_GPV2_DRIVE_NUM; dev_id++) {
      status = 0;
      while (retry) {
        ret = bic_get_dev_power_status(slot_id, dev_id, &nvme_ready, &status, &ffi, &meff, &vendor_id, &major_ver, &minor_ver);
        if (!ret)
          break;
        msleep(50);
        retry--;
      }
      if (ret)
        printf("device%d: Unknown\n",dev_id-1);
      else if (status)
        printf("device%d: Present\n",dev_id-1);
      else
        printf("device%d: Not Present\n",dev_id-1);
    }
  } else {
    goto err_exit;
  }

  return 0;
err_exit:
  print_usage_help();
  return -1;
}
