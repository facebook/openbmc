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
#include <openbmc/ipmi.h>
#include <openbmc/ipmb.h>
#include <openbmc/pal.h>

#define BTN_MAX_SAMPLES 200
#define MAX_NUM_SLOTS 4

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

// Thread for monitoring debug card hotswap
static void *
debug_card_handler() {
  int curr = -1;
  int prev = -1;
  uint8_t prsnt;
  uint8_t pos;
  uint8_t lpc;
  int i, ret;

  while (1) {
    // Check if debug card present or not
    ret = pal_is_debug_card_prsnt(&prsnt);
    if (ret) {
      goto debug_card_out;
    }

    curr = prsnt;
    if (curr == prev) {
      // No state change, continue
      goto debug_card_out;
    }

    if (curr) {
      syslog(LOG_ALERT, "Debug Card Insertion\n");
      // Get current position of hand switch
      ret = pal_get_hand_sw(&pos);
      if (ret) {
        goto debug_card_out;
      }

      // Switch USB mux based on hand switch
      ret = pal_switch_usb_mux(pos);
      if (ret) {
        goto debug_card_out;
      }
      // Switch UART mux based on hand switch
      ret = pal_switch_uart_mux(pos);
      if (ret) {
        goto debug_card_out;
      }

      // Enable POST code based on hand switch
      if (pos == HAND_SW_BMC) {
        // For BMC, there is no need to have POST specific code
        goto debug_card_done;
      }

      // Make sure the server at selected position is present
      ret = pal_is_server_prsnt(pos, &prsnt);
      if (ret || !prsnt) {
        goto debug_card_done;
      }

      // Enable POST codes for all slots
      ret = pal_post_enable(pos);
      if (ret) {
        goto debug_card_out;
      }

      // Get last post code and display it
      ret = pal_post_get_last(pos, &lpc);
      if (ret) {
        goto debug_card_out;
      }

      ret = pal_post_handle(pos, lpc);
      if (ret) {
        goto debug_card_out;
      }
    } else {
      syslog(LOG_ALERT, "Debug Card Extraction\n");
      // Switch UART mux to BMC
      ret = pal_switch_uart_mux(HAND_SW_BMC);
      if (ret) {
        goto debug_card_out;
      }
    }
debug_card_done:
    prev = curr;
debug_card_out:
    sleep(1);
  }
}

// Thread to monitor the hand switch
static void *
hand_sw_handler() {
  int curr = -1;
  int prev = -1;
  int ret;
  uint8_t pos;
  uint8_t prsnt;
  uint8_t lpc;

  while (1) {
    // Get the current hand switch position
    ret = pal_get_hand_sw(&pos);
    if (ret) {
      goto hand_sw_out;
    }
    curr = pos;
    if (curr == prev) {
      // No state change, continue;
      goto hand_sw_out;
    }

    // Switch USB Mux to selected server
    ret = pal_switch_usb_mux(pos);
    if (ret) {
      goto hand_sw_out;
    }

    // If Debug Card is present, update UART MUX
    ret = pal_is_debug_card_prsnt(&prsnt);
    if (ret) {
      goto hand_sw_out;
    }

    if (prsnt) {
      // Switch UART mux based on position
      ret = pal_switch_uart_mux(pos);
      if (ret) {
        goto hand_sw_out;
      }

      if (pos == HAND_SW_BMC) {
        // For BMC, there is no need for POST enable/disable code
        goto hand_sw_done;
      }

      ret = pal_is_server_prsnt(pos, &prsnt);
      if (ret || !prsnt) {
        // Server at chosen position is not present
        goto hand_sw_done;
      }

      // Enable post for the chosen server
      ret = pal_post_enable(pos);
      if (ret) {
        goto hand_sw_out;
      }

      // Get last post code and display it
      ret = pal_post_get_last(pos, &lpc);
      if (ret) {
        goto hand_sw_out;
      }

      ret = pal_post_handle(pos, lpc);
      if (ret) {
        goto hand_sw_out;
      }
    }
hand_sw_done:
    prev = curr;
hand_sw_out:
    sleep(1);
    continue;
  }
}

// Thread to monitor Reset Button and propagate to selected server
static void *
rst_btn_handler() {
  int ret;
  uint8_t pos;
  int i;
  uint8_t btn;

  while (1) {
    // Check the position of hand switch
    ret = pal_get_hand_sw(&pos);
    if (ret || pos == HAND_SW_BMC) {
      // For BMC, no need to handle Reset Button
      sleep (1);
      continue;
    }

    // Check if reset button is pressed
    ret = pal_get_rst_btn(&btn);
    if (ret || !btn) {
      goto rst_btn_out;
    }

    // Pass the reset button to the selected slot
    syslog(LOG_ALERT, "reset button pressed\n");
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
      syslog(LOG_ALERT, "Reset button released\n");
      ret = pal_set_rst_btn(pos, 1);
      goto rst_btn_out;
    }

    // handle error case
    if (i == BTN_MAX_SAMPLES) {
      syslog(LOG_ALERT, "Reset button seems to stuck for long time\n");
      goto rst_btn_out;
    }
rst_btn_out:
    msleep(100);
  }
}

// Thread to handle Power Button and power on/off the selected server
static void *
pwr_btn_handler() {
  int ret;
  uint8_t pos, btn;
  int i;
  uint8_t power;

  while (1) {
    // Check the position of hand switch
    ret = pal_get_hand_sw(&pos);
    if (ret || pos == HAND_SW_BMC) {
      sleep(1);
      continue;
    }

    // Check if power button is pressed
    ret = pal_get_pwr_btn(&btn);
    if (ret || !btn) {
      goto pwr_btn_out;
    }

    syslog(LOG_ALERT, "power button pressed\n");

    // Wait for the button to be released
    for (i = 0; i < BTN_MAX_SAMPLES; i++) {
      ret = pal_get_pwr_btn(&btn);
      if (ret || btn ) {
        msleep(100);
        continue;
      }
      syslog(LOG_ALERT, "power button released\n");
      break;
    }

    // handle error case
    if (i == BTN_MAX_SAMPLES) {
      syslog(LOG_ALERT, "Power button seems to stuck for long time\n");
      goto pwr_btn_out;
    }

    // Get the current power state (power on vs. power off)
    ret = pal_get_server_power(pos, &power);
    if (ret) {
      goto pwr_btn_out;
    }

    // Reverse the power state of the given server
    ret = pal_set_server_power(pos, !power);
pwr_btn_out:
    msleep(100);
  }
}

// Thread to handle LED state of the server at given slot
static void *
led_handler(void *num) {
  int ret;
  uint8_t prsnt;
  uint8_t power;
  uint8_t pos;
  uint8_t ident;
  uint8_t led_blink;
  int led_on_time, led_off_time;

  uint8_t slot = (*(int*) num) + 1;

  syslog(LOG_INFO, "led_handler for slot %d\n", slot);

  ret = pal_is_server_prsnt(slot, &prsnt);
  if (ret || !prsnt) {
    // Turn off led and exit
    ret = pal_set_led(slot, 0);
    goto led_handler_exit;
  }

  while (1) {
    // Get power status for this slot
    ret = pal_get_server_power(slot, &power);
    if (ret) {
      sleep(1);
      continue;
    }

    // Get hand switch position to see if this is selected server
    ret = pal_get_hand_sw(&pos);
    if (ret) {
      sleep(1);
      continue;
    }

    if (pos == slot) {
      // This server is selcted one, set ident flag
      ident = 1;
    } else {
      ident = 0;
    }

    // Update LED based on current state
    if (ident) {
      // If this is selected server the blink flag is one
      led_blink = 1;
      // update the blink rate based on power state
      if (power) {
        led_on_time = 900;
        led_off_time = 100;
      } else {
        led_on_time = 100;
        led_off_time = 900;
      }
    } else {
      // This server is not selected one
      led_blink = 0;
    }

    if (!led_blink) {
      // Set the led state based on power state
      ret = pal_set_led(slot, power);
      goto led_handler_out;
    }

    // Since this is selected slot, start blinking the LED
    ret = pal_set_led(slot, 1);
    if (ret) {
      goto led_handler_out;
    }

    msleep(led_on_time);

    ret = pal_set_led(slot, 0);
    if (ret) {
      goto led_handler_out;
    }

    msleep(led_off_time);
led_handler_out:
    msleep(100);
  }

led_handler_exit:
  free(num);
}

int
main (int argc, char * const argv[]) {
  pthread_t tid_hand_sw;
  pthread_t tid_debug_card;
  pthread_t tid_rst_btn;
  pthread_t tid_pwr_btn;
  pthread_t tid_leds[MAX_NUM_SLOTS];
  int i;
  int *ip;

  daemon(1, 0);
  openlog("front-paneld", LOG_CONS, LOG_DAEMON);

 if (pthread_create(&tid_debug_card, NULL, debug_card_handler, NULL) < 0) {
    syslog(LOG_ALERT, "pthread_create for debug card error\n");
    exit(1);
  }

  if (pthread_create(&tid_hand_sw, NULL, hand_sw_handler, NULL) < 0) {
    syslog(LOG_ALERT, "pthread_create for hand switch error\n");
    exit(1);
  }

  if (pthread_create(&tid_rst_btn, NULL, rst_btn_handler, NULL) < 0) {
    syslog(LOG_ALERT, "pthread_create for reset button error\n");
    exit(1);
  }

  if (pthread_create(&tid_pwr_btn, NULL, pwr_btn_handler, NULL) < 0) {
    syslog(LOG_ALERT, "pthread_create for power button error\n");
    exit(1);
  }

  for (i = 0; i < MAX_NUM_SLOTS; i++) {
    ip = malloc(sizeof(int));
    *ip = i;
    if (pthread_create(&tid_leds[i], NULL, led_handler, (void*)ip) < 0) {
      syslog(LOG_ALERT, "pthread_create for hand switch error\n");
      exit(1);
    }
  }

  pthread_join(tid_debug_card, NULL);
  pthread_join(tid_hand_sw, NULL);
  pthread_join(tid_rst_btn, NULL);
  pthread_join(tid_pwr_btn, NULL);
  for (i = 0;  i < MAX_NUM_SLOTS; i++) {
    pthread_join(tid_leds[i], NULL);
  }

  return 0;
}
