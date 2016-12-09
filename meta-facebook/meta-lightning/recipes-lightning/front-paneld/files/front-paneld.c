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
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <time.h>
#include <sys/time.h>
#include <openbmc/pal.h>

#define BTN_MAX_SAMPLES   200
#define BTN_POWER_OFF     40
#define MAX_NUM_SLOTS 4
#define HB_TIMESTAMP_COUNT (60 * 60)

enum led_state {
  LED_OFF = 0,
  LED_ON = 1,
};

enum inverse_led_state {
  LED_ON_N = 0,
  LED_OFF_N = 1,
};

// Helper function for msleep
void
msleep(int msec) {
  struct timespec req;

  req.tv_sec = 0;
  req.tv_nsec = msec * 1000 * 1000;

  while(nanosleep(&req, &req) == -1 && errno == EINTR) {
    continue;
  }
}

// Thread for handling the Power and System Identify LED
static void *
id_led_handler() {

  int ret;
  char state[8];

  while (1) {

    memset(state, 0, sizeof(state));
    // Check if the user enabled system identify
    ret = pal_get_key_value("system_identify", state);
    if (ret)
      goto err;

    if (!strcmp(state, "on")) {

      // 0.9s ON
      pal_set_led(LED_DR_LED1, LED_ON);
      msleep(900);

      // 0.1s OFF
      pal_set_led(LED_DR_LED1, LED_OFF);
      msleep(100);
      continue;

    } else if (!strcmp(state, "off")) {

      printf("on\n");
      pal_set_led(LED_DR_LED1, LED_ON);
    }

err:
    sleep(1);
  }

}

// Thread for handling the Enclosure LED
static void *
encl_led_handler() {

  int ret;
  uint8_t peb_hlth;
  uint8_t pdpb_hlth;
  uint8_t fcb_hlth;

  while (1) {

    // Get health status for all the fru and then update the ENCL_LED status
    ret = pal_get_fru_health(FRU_PEB, &peb_hlth);
    if (ret)
      goto err;

    ret = pal_get_fru_health(FRU_PDPB, &pdpb_hlth);
    if (ret)
      goto err;

    ret = pal_get_fru_health(FRU_FCB, &fcb_hlth);
    if (ret)
      goto err;

    if (!peb_hlth | !pdpb_hlth | !fcb_hlth)
      pal_set_led(LED_ENCLOSURE, LED_ON_N);
    else
      pal_set_led(LED_ENCLOSURE, LED_OFF_N);

err:
    sleep(1);
  }

}
// Thread for monitoring debug card hotswap
static void *
debug_card_handler() {
  int i, ret;
  uint8_t btn;
  uint8_t pos;

  while (1) {
    // Check if UART channel button is pressed
    ret = pal_get_uart_chan_btn(&btn);
    if (ret) {
      goto debug_card_out;
    }
    if (!btn) {
      syslog(LOG_CRIT, "UART Channel button pressed\n");

      // Wait for the button to be released
      for (i = 0; i < BTN_MAX_SAMPLES; i++) {
        ret = pal_get_uart_chan_btn(&btn);
        if (ret || !btn) {
          msleep(100);
          continue;
        }
        syslog(LOG_WARNING, "UART Channel button released\n");
        break;
      }

      // handle error case
      if (i == BTN_MAX_SAMPLES) {
        syslog(LOG_WARNING, "UART Channel button seems to stuck for long time\n");
        goto debug_card_out;
      }

      // Get the current position for the UART
      ret = pal_get_uart_chan(&pos);
      if (ret) {
        goto debug_card_out;
      }

      // Toggle the UART position
      ret = pal_set_uart_chan(!pos);
      if (ret) {
        goto debug_card_out;
      }

      // Display Post code 0xFF for 1 secound on UART button press event
      system("/usr/local/bin/post_led.sh 255");
      msleep(1000);
      system("/usr/local/bin/post_led.sh 0");
    }
debug_card_out:
    msleep(500);
  }
}

int
main (int argc, char * const argv[]) {

  int rc;
  int pid_file;
  pthread_t tid_debug_card;
  pthread_t tid_encl_led;
  pthread_t tid_id_led;

  pid_file = open("/var/run/front-paneld.pid", O_CREAT | O_RDWR, 0666);
  rc = flock(pid_file, LOCK_EX | LOCK_NB);
  if(rc) {
    if(EWOULDBLOCK == errno) {
      printf("Another front-paneld instance is running...\n");
      exit(-1);
    }
  } else {
   daemon(0, 1);
   openlog("front-paneld", LOG_CONS, LOG_DAEMON);
  }

  if (pthread_create(&tid_debug_card, NULL, debug_card_handler, NULL) < 0) {
    syslog(LOG_WARNING, "pthread_create for debug card error\n");
    exit(1);
  }

  if (pthread_create(&tid_encl_led, NULL, encl_led_handler, NULL) < 0) {
    syslog(LOG_WARNING, "pthread_create for encl led error\n");
    exit(1);
  }

  if (pthread_create(&tid_id_led, NULL, id_led_handler, NULL) < 0) {
    syslog(LOG_WARNING, "pthread_create for system identify led error\n");
    exit(1);
  }

  pthread_join(tid_debug_card, NULL);
  pthread_join(tid_encl_led, NULL);
  pthread_join(tid_id_led, NULL);

  return 0;
}
