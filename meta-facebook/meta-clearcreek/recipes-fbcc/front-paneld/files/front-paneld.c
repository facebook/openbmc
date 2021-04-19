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
#include <unistd.h>
#include <errno.h>
#include <syslog.h>
#include <string.h>
#include <pthread.h>
#include <fcntl.h>
#include <openbmc/kv.h>
#include <openbmc/pal.h>

#define BTN_MAX_SAMPLES   200
#define FW_UPDATE_ONGOING 1
#define CRASHDUMP_ONGOING 2

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

    for (fru = FRU_MB; fru <= FRU_CARRIER2; fru++) {
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

static int
is_btn_blocked(uint8_t fru) {

  if (pal_is_fw_update_ongoing(fru)) {
    return FW_UPDATE_ONGOING;
  }

  if (pal_is_crashdump_ongoing(fru)) {
    return CRASHDUMP_ONGOING;
  }
  return 0;
}

static void *
rst_btn_handler() {
  int ret;
  uint8_t pos = FRU_MB;
  int i;
  uint8_t btn;
  uint8_t last_btn = 0;

  ret = pal_get_rst_btn(&btn);
  if (0 == ret) {
    last_btn = btn;
  }

  while (1) {
    // Check if reset button is pressed
    ret = pal_get_rst_btn(&btn);
    if (ret || !btn) {
      if (last_btn != btn) {
        pal_set_rst_btn(pos, 1);
      }
      goto rst_btn_out;
    }
    ret = is_btn_blocked(pos);

    if (ret) {
      if (!last_btn) {
        switch (ret) {
          case FW_UPDATE_ONGOING:
            syslog(LOG_CRIT, "Reset Button blocked due to FW update is ongoing\n");
            break;
          case CRASHDUMP_ONGOING:
            syslog(LOG_CRIT, "Reset Button blocked due to crashdump is ongoing");
            break;
        }
      }
      goto rst_btn_out;
    }

    // Pass the reset button to the selected slot
    syslog(LOG_WARNING, "Reset button pressed\n");
    ret = pal_set_rst_btn(pos, 0);
    if (ret) {
      goto rst_btn_out;
    }

    // Wait for the button to be released
    for (i = 0; i < BTN_MAX_SAMPLES; i++) {
      ret = pal_get_rst_btn(&btn);
      if (ret || btn) {
        msleep(100);
        continue;
      }
      pal_update_ts_sled();
      syslog(LOG_WARNING, "Reset button released\n");
      syslog(LOG_CRIT, "Reset Button pressed for FRU: %d\n", pos);
      ret = pal_set_rst_btn(pos, 1);
      if (!ret) {
        pal_set_restart_cause(pos, RESTART_CAUSE_RESET_PUSH_BUTTON);
      }
      goto rst_btn_out;
    }

    // handle error case
    if (i == BTN_MAX_SAMPLES) {
      pal_update_ts_sled();
      syslog(LOG_WARNING, "Reset button seems to stuck for long time\n");
      goto rst_btn_out;
    }

rst_btn_out:
    last_btn = btn;
    msleep(100);
  }

  return 0;
}

int
main (int argc, char * const argv[]) {
  int pid_file, rc;
  pthread_t tid_rst_btn;
  pthread_t tid_sync_led;

  pid_file = open("/var/run/front-paneld.pid", O_CREAT | O_RDWR, 0666);
  rc = flock(pid_file, LOCK_EX | LOCK_NB);
  if (rc) {
    if (EWOULDBLOCK == errno) {
      printf("Another front-paneld instance is running...\n");
      exit(-1);
    }
  } else {
    openlog("front-paneld", LOG_CONS, LOG_DAEMON);
  }

  if (pthread_create(&tid_sync_led, NULL, led_sync_handler, NULL) < 0) {
    syslog(LOG_WARNING, "pthread_create for led sync error\n");
    exit(1);
  }

  if (pthread_create(&tid_rst_btn, NULL, rst_btn_handler, NULL) < 0) {
    syslog(LOG_WARNING, "pthread_create for reset button error\n");
    exit(1);
  }

  pthread_join(tid_sync_led, NULL);
  pthread_join(tid_rst_btn, NULL);

  return 0;
}
