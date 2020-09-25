/*
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
#include <string.h>
#include <pthread.h>
#include <sys/file.h>
#include <sys/types.h>
#include <openbmc/kv.h>
#include <openbmc/pal.h>
#include <openbmc/libgpio.h>
#include <facebook/asic.h>

#define LED_ON 1
#define LED_OFF 0

#define LED_IDENTIFY_INTERVAL 500

// Thread to handle STATUS/ID LED state of the SLED
static void* led_sync_handler()
{
  int ret;
  uint8_t status, fru;
  char identify[MAX_VALUE_LEN] = {0};

  while (1) {
    // Handle IDENTIFY LED
    memset(identify, 0x0, sizeof(identify));
    ret = pal_get_key_value("identify_sled", identify);
    if (ret == 0 && !strcmp(identify, "on")) {
      pal_set_id_led(LED_ON);
      msleep(LED_IDENTIFY_INTERVAL);
      pal_set_id_led(LED_OFF);
      msleep(LED_IDENTIFY_INTERVAL);
      continue;
    }

    for (fru = FRU_MB; fru <= FRU_ASIC7; fru++) {
      if (fru == FRU_BSM)
        continue;

      ret = pal_get_fru_health(fru, &status);
      if (ret < 0 || status != FRU_STATUS_GOOD) {
        pal_set_id_led(LED_ON);
        break;
      }
    }
    if (ret == 0 && status == FRU_STATUS_GOOD)
      pal_set_id_led(LED_OFF);
    sleep(1);
  }

  return NULL;
}

static void* get_asic_id_handler()
{
  char* mfr_list[MFR_MAX_NUM+1] = {
    [GPU_NVIDIA] = MFR_NVIDIA,
    [GPU_AMD] = MFR_AMD,
    [GPU_UNKNOWN] = MFR_UNKNOWN
  };
  uint8_t asic_id[8] = {
    GPU_UNKNOWN, GPU_UNKNOWN, GPU_UNKNOWN, GPU_UNKNOWN,
    GPU_UNKNOWN, GPU_UNKNOWN, GPU_UNKNOWN, GPU_UNKNOWN
  };
  uint8_t count = 0x0;
  bool id_err = false;
  char curr_mfr[MAX_VALUE_LEN] = {0};

  if (pal_get_key_value("asic_mfr", curr_mfr) < 0)
    strncpy(curr_mfr, MFR_UNKNOWN, sizeof(curr_mfr));

  while (1) {

    sleep(1);

    if (pal_is_server_off())
      continue;

    for (uint8_t slot = 0; slot < 8 && count != 0xFF; slot++) {
      if (!is_asic_prsnt(slot) || asic_id[slot] != GPU_UNKNOWN) {
        // Bypass if not present
        count |= (0x1 << slot);
        continue;
      }

      asic_id[slot] = asic_get_vendor_id(slot);
      if (asic_id[slot] != GPU_UNKNOWN) {
        syslog(LOG_WARNING, "Detected Vendor for OAM%d: %s, FRU:1",
                             (int)slot, mfr_list[asic_id[slot]]);
      }
    }

    if (count == 0xFF) {
      for (int i = 1; i < 8; i++) {
        if (asic_id[0] != asic_id[i]) {
          syslog(LOG_CRIT, "Incompatible OAM Configuration Detected: OAM0-7: %s %s %s %s %s %s %s %s, FRU:1",
                            mfr_list[asic_id[0]], mfr_list[asic_id[1]],
                            mfr_list[asic_id[2]], mfr_list[asic_id[3]],
                            mfr_list[asic_id[4]], mfr_list[asic_id[5]],
                            mfr_list[asic_id[6]], mfr_list[asic_id[7]]);
          id_err = true;
          break;
        }
      }

      if (!id_err) {
        if (strcmp(curr_mfr, mfr_list[asic_id[0]]) != 0 && strcmp(curr_mfr, MFR_UNKNOWN)) {
          syslog(LOG_CRIT, "Switching OAM Configuration from %s to %s, FRU:1",
                            curr_mfr, mfr_list[asic_id[0]]);
          pal_set_key_value("asic_mfr", mfr_list[asic_id[0]]);
          // Reload sensor list
          if (system("/usr/bin/sv restart sensord >> /dev/null") < 0)
            syslog(LOG_CRIT, "%s: Failed to restart sensord", __func__);
          // Reload FSC table
          if (system("/usr/bin/sv restart fscd >> /dev/null") < 0)
            syslog(LOG_CRIT, "%s: Failed to restart fscd", __func__);
        }
      }

      break;
    }

  }

  return NULL;
}

int main (int argc, char * const argv[])
{
  pthread_t tid_sync_led;
  pthread_t tid_asic_id;
  int rc;
  int pid_file;

  pid_file = open("/var/run/front-paneld.pid", O_CREAT | O_RDWR, 0666);
  rc = flock(pid_file, LOCK_EX | LOCK_NB);
  if(rc) {
    if(EWOULDBLOCK == errno) {
      syslog(LOG_ERR, "Another front-paneld instance is running...\n");
      exit(-1);
    }
  } else {
    openlog("front-paneld", LOG_CONS, LOG_DAEMON);
    syslog(LOG_INFO, "front-paneld: daemon started");
  }

  if (pthread_create(&tid_sync_led, NULL, led_sync_handler, NULL) < 0) {
    syslog(LOG_WARNING, "pthread_create for led sync error\n");
    exit(1);
  }

  if (pthread_create(&tid_asic_id, NULL, get_asic_id_handler, NULL) < 0) {
    syslog(LOG_WARNING, "pthread_create for getting asic id error\n");
    exit(1);
  }

  pthread_join(tid_sync_led, NULL);
  pthread_join(tid_asic_id, NULL);
  return 0;
}
