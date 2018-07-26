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
#include <openbmc/obmc-sensor.h>

#define BTN_MAX_SAMPLES   200
#define BTN_POWER_OFF     40
#define HB_TIMESTAMP_COUNT (60 * 60)
#define POST_LED_DELAY_TIME 2
#define MIN_TRAY_VOL 0.3

struct _tray_last_status {
  uint8_t self;
  uint8_t peer;
} tray_last_status;

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
  pthread_exit(NULL);

}

// Thread for handling the Enclosure LED
static void *
encl_led_handler() {

  int ret;
  uint8_t peb_hlth;
  uint8_t pdpb_hlth;
  uint8_t fcb_hlth;
  uint8_t bmc_hlth;

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

    ret = pal_get_fru_health(BMC_HEALTH, &bmc_hlth);
    if (ret)
      goto err;

    if (!peb_hlth | !pdpb_hlth | !fcb_hlth | !bmc_hlth)
      pal_set_led(LED_ENCLOSURE, LED_N_ON);
    else
      pal_set_led(LED_ENCLOSURE, LED_N_OFF);

err:
    sleep(1);
  }
  pthread_exit(NULL);

}
// Thread for monitoring debug card hotswap
static void *
debug_card_handler() {
  int i = 0;
  int bit_num = 0;
  int ret = 0;
  uint8_t current_tray_state = 0;
  uint8_t btn = 0;
  uint8_t pos = 0;
  uint8_t err_num = 0;
  char self_tray_name[16] = "";
  char peer_tray_name[16] = "";
  uint8_t peer_tray_pwr = 0;
  float peer_tray_vol = 0.0;
  uint8_t error_code_assert[ERROR_CODE_NUM * 8];
  uint8_t error_code_array[ERROR_CODE_NUM] = {0};
  int errCount = 0;
  int displayCount = 0;

  pal_write_error_code_file(0, ERR_DEASSERT);
  memset(&tray_last_status, 0, sizeof(tray_last_status));

  /* Getting tray location information */
  i = 0; //setting the retry limit to 3
  ret = pal_get_tray_location(self_tray_name, sizeof(self_tray_name),
                              peer_tray_name, sizeof(peer_tray_name),
                              &peer_tray_pwr);
  while ((ret == -1) && (i < 3)){
    sleep(1);
    ret = pal_get_tray_location(self_tray_name, sizeof(self_tray_name),
                                peer_tray_name, sizeof(peer_tray_name),
                                &peer_tray_pwr);
    syslog(LOG_WARNING, "%s(): pal_get_tray_location() failed", __func__);
    i++;
  }

  while (1) {

    /* Get self tray and peer tray location then set error code and record log.*/
    current_tray_state = 0;
    if (!pal_self_tray_insertion(&current_tray_state)) {
      if (tray_last_status.self != current_tray_state) {
        tray_last_status.self = current_tray_state;
        if (current_tray_state) {
          pal_err_code_enable(ERR_CODE_SELF_TRAY_PULL_OUT);
          syslog(LOG_CRIT, "ASSERT: Self Tray (%s) Pulled-out", self_tray_name);
        } else {
          pal_err_code_disable(ERR_CODE_SELF_TRAY_PULL_OUT);
          syslog(LOG_CRIT, "DEASSERT: Self Tray (%s) Pulled-out", self_tray_name);
        }
      }
    }
    current_tray_state = 0;
    if (!pal_peer_tray_insertion(&current_tray_state)) {
      sensor_cache_read(FRU_FCB, peer_tray_pwr, &peer_tray_vol);
      if ((tray_last_status.peer != current_tray_state) && (peer_tray_vol > MIN_TRAY_VOL)) {
        tray_last_status.peer = current_tray_state;
        if (current_tray_state) {
          pal_err_code_enable(ERR_CODE_PEER_TRAY_PULL_OUT);
          syslog(LOG_CRIT, "ASSERT: Peer Tray (%s) Pulled-out", peer_tray_name);
        } else {
          pal_err_code_disable(ERR_CODE_PEER_TRAY_PULL_OUT);
          syslog(LOG_CRIT, "DEASSERT: Peer Tray (%s) Pulled-out", peer_tray_name);
        }
      }
    }

    pal_read_error_code_file(error_code_array);

    /* Read error code file for displaying error number on debug card*/
    memset(error_code_assert, 0, ERROR_CODE_NUM * 8);
    errCount = 0;
    for(i = 0; i < ERROR_CODE_NUM; i++ ) {
      for(bit_num = 0; bit_num < 8; bit_num++ ) {
        if((( error_code_array[i] >> bit_num )&0x01 ) == 1) {
          *(error_code_assert + errCount) = i * 8 + bit_num;
          errCount++;
        }
      }
    }
    err_num = error_code_assert[displayCount];
    displayCount++;
    if(displayCount >= errCount)
      displayCount = 0;

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

      // Display Post code on UART button press event
      if (pal_set_debug_card_led(pos))
        syslog(LOG_WARNING, "%s(): pal_set_debug_card_led() failed", __func__);
      sleep(POST_LED_DELAY_TIME);
    }
    if (pal_set_debug_card_led(err_num))
      syslog(LOG_WARNING, "%s(): pal_set_debug_card_led() failed", __func__);
debug_card_out:
    sleep(POST_LED_DELAY_TIME);
  }
  pthread_exit(NULL);

}

int
main (int argc, char * const argv[]) {
  pthread_t tid_debug_card;
  pthread_t tid_encl_led;
  pthread_t tid_id_led;

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
