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
#include <fcntl.h>
#include <sys/file.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <time.h>
#include <sys/time.h>
#include <openbmc/kv.h>
#include <openbmc/pal.h>

#define ID_LED_ON 1
#define ID_LED_OFF 0

#define LED_ON_TIME_IDENTIFY 500
#define LED_OFF_TIME_IDENTIFY 500

#define BTN_MAX_SAMPLES   200
#define FW_UPDATE_ONGOING 1
#define CRASHDUMP_ONGOING 2

static int sensor_health = FRU_STATUS_GOOD;

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

// Thread to handle LED state of the SLED
static void *
led_sync_handler() {
  int ret;
  uint8_t id_on = 0;
  char identify[MAX_VALUE_LEN] = {0};

  while (1) {
    // Handle Slot IDENTIFY condition
    memset(identify, 0x0, sizeof(identify));
    ret = pal_get_key_value("identify_sled", identify);
    if (ret == 0 && !strcmp(identify, "on")) {
      id_on = 1;

      // Start blinking the ID LED
      pal_set_id_led(FRU_MB, ID_LED_ON);
      msleep(LED_ON_TIME_IDENTIFY);

      pal_set_id_led(FRU_MB, ID_LED_OFF);
      msleep(LED_OFF_TIME_IDENTIFY);
      continue;
    } else if (id_on) {
      id_on = 0;
      pal_set_id_led(FRU_MB, 0xFF);
    } else if (ret == 0 && !strcmp(identify, "off")) {
      // Turn on the ID LED if sensor health is abnormal.
      if (sensor_health == FRU_STATUS_BAD) { 
        pal_set_id_led(FRU_MB, ID_LED_ON);
      } else {
        pal_set_id_led(FRU_MB, ID_LED_OFF);
      }
    }

    sleep(1);
  }
  return NULL;
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

// Thread to handle LED state of the server at given slot
static void *
led_handler() {
  int ret;
  uint8_t mb_health = -1, nic0_health = -1, nic1_health = -1;

  while (1) {
    sleep(1);
    ret = pal_get_fru_health(FRU_MB, &mb_health);
    if (ret) {
      syslog(LOG_WARNING, "Fail to get MB health\n");
    }

    ret = pal_get_fru_health(FRU_NIC0, &nic0_health);
    if (ret) {
      syslog(LOG_WARNING, "Fail to get NIC0 health\n");
    }

    ret = pal_get_fru_health(FRU_NIC1, &nic1_health);
    if (ret) {
      syslog(LOG_WARNING, "Fail to get NIC1 health\n");
    }

    if (mb_health != FRU_STATUS_GOOD || nic0_health != FRU_STATUS_GOOD
      || nic1_health != FRU_STATUS_GOOD) {
      sensor_health = FRU_STATUS_BAD;
    }
    else {
      sensor_health = FRU_STATUS_GOOD;
    }
  }
  return NULL;
}

int
main (int argc, char * const argv[]) {
  pthread_t tid_sync_led;
  pthread_t tid_rst_btn;
  pthread_t tid_led;
  int rc;
  int pid_file;

  pid_file = open("/var/run/front-paneld.pid", O_CREAT | O_RDWR, 0666);
  rc = flock(pid_file, LOCK_EX | LOCK_NB);
  if(rc) {
    if(EWOULDBLOCK == errno) {
      printf("Another front-paneld instance is running...\n");
      exit(-1);
    }
  } else {
    if (daemon(0, 1)) {
      printf("Daemon failed!\n");
      exit(-1);
    }
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

  if (pthread_create(&tid_led, NULL, led_handler, NULL) < 0) {
    syslog(LOG_WARNING, "pthread_create for led error\n");
    exit(1);
  }

  pthread_join(tid_sync_led, NULL);
  pthread_join(tid_rst_btn, NULL);
  pthread_join(tid_led, NULL);
  return 0;
}
