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
#include <openbmc/ipmi.h>
#include <openbmc/ipmb.h>
#include <openbmc/pal.h>

#define HB_SLEEP_TIME (5 * 60)
#define HB_TIMESTAMP_COUNT (60 * 60 / HB_SLEEP_TIME)

#define ID_LED_ON 0
#define ID_LED_OFF 1

#define LED_ON_TIME_IDENTIFY 100
#define LED_OFF_TIME_IDENTIFY 900

//SLED Time Sync Timeout
#define SLED_TS_TIMEOUT 100

// Thread for monitoring debug card hotswap
static void *
debug_card_handler() {
  int curr = -1;
  int prev = -1;
  uint8_t prsnt;
  int ret;

  while (1) {
    // Check if debug card present or not
    ret = pal_is_debug_card_prsnt(&prsnt);
    if (ret) {
      goto debug_card_out;
    }

    curr = prsnt;

    // Check if Debug Card was either inserted or removed
    if (curr == prev) {
      goto debug_card_out;
    }

    if (!curr) {
    // Debug Card was removed
      syslog(LOG_WARNING, "Debug Card Extraction\n");
      // Switch UART mux to BMC
      ret = pal_switch_uart_mux(UART_TO_BMC);
      if (ret) {
        goto debug_card_out;
      }
    } else {
    // Debug Card was inserted
      syslog(LOG_WARNING, "Debug Card Insertion\n");
      // Switch UART mux to Debug card
      ret = pal_switch_uart_mux(UART_TO_DEBUG);
      if (ret) {
        goto debug_card_out;
      }
    }
    prev = curr;
debug_card_out:
      sleep(1);
  }
  return NULL;
}

// Thread to handle LED state of the SLED
static void *
led_sync_handler() {
  int ret;
  char identify[MAX_VALUE_LEN] = {0};

  while (1) {
    // Handle Slot IDENTIFY condition
    memset(identify, 0x0, sizeof(identify));
    ret = pal_get_key_value("identify_sled", identify);
    if (ret == 0 && !strcmp(identify, "on")) {
      // Start blinking the ID LED
      pal_set_id_led(FRU_MB, ID_LED_ON);

      msleep(LED_ON_TIME_IDENTIFY);

      pal_set_id_led(FRU_MB, ID_LED_OFF);

      msleep(LED_OFF_TIME_IDENTIFY);
      continue;
    }

    sleep(1);
  }
  return NULL;
}

int
main (int argc, char * const argv[]) {
  pthread_t tid_debug_card;
  pthread_t tid_sync_led;
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
    if (daemon(0, 1) != 0) {
      syslog(LOG_ERR, "Cannot daemonize front-paneld\n");
    }
   openlog("front-paneld", LOG_CONS, LOG_DAEMON);
  }


  if (pthread_create(&tid_debug_card, NULL, debug_card_handler, NULL) < 0) {
    syslog(LOG_WARNING, "pthread_create for debug card error\n");
    exit(1);
  }

  if (pthread_create(&tid_sync_led, NULL, led_sync_handler, NULL) < 0) {
    syslog(LOG_WARNING, "pthread_create for led sync error\n");
    exit(1);
  }

  pthread_join(tid_debug_card, NULL);
  pthread_join(tid_sync_led, NULL);
  return 0;
}
