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
#include <facebook/fby2_sensor.h>
#include <openbmc/pal.h>
#include <jansson.h>

#define MAX_READ_RETRY 10
#define MAX_GPV2_DRIVE_NUM 12
#define MAX_DEVICE_LEN 16
#define MAX_DEVICE_SLOT_NUM 2

static void
print_usage_help(void) {
  printf("Usage: gpv2-util <slot1|slot3> --device-list [--json]\n");
}

int
main(int argc, char **argv) {

  uint8_t slot_id;
  uint8_t dev_id;
  uint8_t dev_slot = 0;
  uint8_t status;
  uint8_t type;
  uint8_t nvme_ready;
  uint8_t ffi;
  uint8_t meff;
  uint16_t vendor_id = 0;
  uint8_t major_ver;
  uint8_t minor_ver;
  uint8_t retry = MAX_READ_RETRY;
  uint8_t output_json = 0;
  json_t *device_object = NULL;
  char device[MAX_DEVICE_LEN] = {};
  int ret = 0;
  bool is_dual_m2 = false;
  uint8_t dev_status[MAX_GPV2_DRIVE_NUM] = {0};
  uint8_t dev_ret[MAX_GPV2_DRIVE_NUM] = {0};

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

  if (fby2_get_slot_type(slot_id) != SLOT_TYPE_GPV2) {
    printf("slot%d is not GPV2\n",slot_id);
    goto err_exit;
  }

  if (!strcmp(argv[2], "--device-list")) {

    // check --json option
    if (argc > 3) {
      if (argc == 4 && !strcmp(argv[3], "--json")) {
        output_json = 1;
        //Returns a new JSON object, or NULL on error
        device_object = json_object();
        if (!device_object)
          goto err_exit;
      } else
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
      retry = MAX_READ_RETRY;

      dev_status[dev_id - 1] = status;
      dev_ret[dev_id - 1] = ret;

      if (is_dual_m2 == false && meff == MEFF_DUAL_M2) {
          is_dual_m2 = true;
      }
    }

    for (dev_id = 1; dev_id <= MAX_GPV2_DRIVE_NUM; dev_id+=2) {
      for (dev_slot = 0; dev_slot < MAX_DEVICE_SLOT_NUM; dev_slot++) {
        if (is_dual_m2 == true) {
          snprintf(device, MAX_DEVICE_LEN, "device%d/%d", dev_id - 1, dev_id);
          ret = dev_ret[dev_id - 1] | dev_ret[dev_id];
          status = dev_status[dev_id - 1] | dev_status[dev_id];
          dev_slot++;
        } else {
          snprintf(device, MAX_DEVICE_LEN, "device%d", (dev_id + dev_slot - 1));
          ret = dev_ret[dev_id + dev_slot - 1];
          status = dev_status[dev_id + dev_slot - 1];
        }

        if (ret) {
          if (output_json) {
            json_object_set_new(device_object, device, json_string("Unknown"));
          }
          else {
            printf("%s: Unknown\n", device, dev_id - 1);
          }
        } else if (status) {
          if (output_json) {
            json_object_set_new(device_object, device, json_string("Present"));
          }
          else {
            printf("%s: Present\n", device, dev_id - 1);
          }
        } else {
          if (output_json) {
            json_object_set_new(device_object, device, json_string("Not Present"));
          }
          else {
            printf("%s: Not Present\n", device, dev_id - 1);
          }
        }
      }
    }
    if (output_json) {
      json_dumpf(device_object, stdout, 4);
      json_decref(device_object);
    }
    printf("\n");
  } else {
    goto err_exit;
  }

  return 0;
err_exit:
  print_usage_help();
  return -1;
}
