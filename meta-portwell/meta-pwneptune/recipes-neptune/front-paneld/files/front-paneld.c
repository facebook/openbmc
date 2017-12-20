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

#define BTN_MAX_SAMPLES   200
#define BTN_POWER_OFF     40
#define MAX_NUM_SLOTS 4
#define HB_SLEEP_TIME (5 * 60)
#define HB_TIMESTAMP_COUNT (60 * 60 / HB_SLEEP_TIME)

#define LED_ON 1
#define LED_OFF 0

#define ID_LED_ON 0
#define ID_LED_OFF 1

#define LED_ON_TIME_IDENTIFY 100
#define LED_OFF_TIME_IDENTIFY 900

#define LED_ON_TIME_HEALTH 900
#define LED_OFF_TIME_HEALTH 100

#define LED_ON_TIME_BMC_SELECT 500
#define LED_OFF_TIME_BMC_SELECT 500

//SLED Time Sync Timeout
#define SLED_TS_TIMEOUT 100

uint8_t g_sync_led[MAX_NUM_SLOTS+1] = {0x0};

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
}

#if 0
// Thread to monitor the hand switch
static void *
usb_handler() {
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
      goto rst_btn_out;
    }

    // handle error case
    if (i == BTN_MAX_SAMPLES) {
      pal_update_ts_sled();
      syslog(LOG_WARNING, "Reset button seems to stuck for long time\n");
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
  uint8_t pos, btn, cmd;
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

    syslog(LOG_WARNING, "power button pressed\n");

    // Wait for the button to be released
    for (i = 0; i < BTN_POWER_OFF; i++) {
      ret = pal_get_pwr_btn(&btn);
      if (ret || btn ) {
        msleep(100);
        continue;
      }
      syslog(LOG_WARNING, "power button released\n");
      break;
    }


    // Get the current power state (power on vs. power off)
    ret = pal_get_server_power(pos, &power);
    if (ret) {
      goto pwr_btn_out;
    }

    // Set power command should reverse of current power state
    cmd = !power;

    // To determine long button press
    if (i >= BTN_POWER_OFF) {
      pal_update_ts_sled();
      syslog(LOG_CRIT, "Power Button Long Press for FRU: %d\n", pos);
    } else {

      // If current power state is ON and it is not a long press,
      // the power off should be Graceful Shutdown
      if (power == SERVER_POWER_ON)
        cmd = SERVER_GRACEFUL_SHUTDOWN;

      pal_update_ts_sled();
      syslog(LOG_CRIT, "Power Button Press for FRU: %d\n", pos);
    }

    // Reverse the power state of the given server
    ret = pal_set_server_power(pos, cmd);
pwr_btn_out:
    msleep(100);
  }
}
#endif

// Thread to monitor SLED Cycles by using time stamp
static void *
ts_handler() {
  int count = 0;
  struct timespec ts;
  struct timespec mts;
  char tstr[64] = {0};
  char buf[128] = {0};
  char temp_log[28] = {0};
  uint8_t por = 0;
  uint8_t time_init = 0;
  long time_sled_on;
  long time_sled_off;

  // Read the last timestamp from KV storage
  pal_get_key_value("timestamp_sled", tstr);
  time_sled_off = (long) strtoul(tstr, NULL, 10);

  // If this reset is due to Power-On-Reset, we detected SLED power OFF event
  if (pal_is_bmc_por()) {
    ctime_r(&time_sled_off, buf);
    syslog(LOG_CRIT, "SLED Powered OFF at %s", buf);
    sprintf(temp_log, "AC lost");
    pal_add_cri_sel(temp_log);
  } else {
    syslog(LOG_CRIT, "BMC Reboot detected");
  }

  while (1) {

    // Make sure the time is initialized properly
    // Since there is no battery backup, the time could be reset to build time
    // wait 100s at most, to prevent infinite waiting
    if ( time_init < SLED_TS_TIMEOUT ) {
      // Read current time
      clock_gettime(CLOCK_REALTIME, &ts);

      if ( (ts.tv_sec < time_sled_off) && (++time_init < SLED_TS_TIMEOUT) ) {
        sleep(1);
        continue;
      }
      
      // If get the correct time or time sync timeout
      time_init = SLED_TS_TIMEOUT;
      
      // Need to log SLED ON event, if this is Power-On-Reset
      if (pal_is_bmc_por()) {
        // Get uptime
        clock_gettime(CLOCK_MONOTONIC, &mts);
        // To find out when SLED was on, subtract the uptime from current time
        time_sled_on = ts.tv_sec - mts.tv_sec;

        ctime_r(&time_sled_on, buf);
        // Log an event if this is Power-On-Reset
        syslog(LOG_CRIT, "SLED Powered ON at %s", buf);
      }
      pal_update_ts_sled();
    }

    // Store timestamp every one hour to keep track of SLED power
    if (count++ == HB_TIMESTAMP_COUNT) {
      pal_update_ts_sled();
      count = 0;
    }

    sleep(HB_SLEEP_TIME);
  }
}

#if 0
// Thread to handle LED state of the server at given slot
static void *
led_handler(void *num) {
  int ret;
  uint8_t prsnt;
  uint8_t power;
  uint8_t pos;
  uint8_t led_blink;
  uint8_t ident = 0;
  int led_on_time, led_off_time;
  char identify[16] = {0};
  char tstr[64] = {0};
  int power_led_on_time = 500;
  int power_led_off_time = 500;
  uint8_t hlth = 0;

  uint8_t slot = (*(int*) num) + 1;

#ifdef DEBUG
  syslog(LOG_INFO, "led_handler for slot %d\n", slot);
#endif

  ret = pal_is_fru_prsnt(slot, &prsnt);
  if (ret || !prsnt) {
    // Turn off led and exit
    ret = pal_set_led(slot, 0);
    goto led_handler_exit;
  }

  while (1) {
    // Check if this LED is managed by sync_led thread
    if (g_sync_led[slot]) {
      sleep(1);
      continue;
    }

    // Get power status for this slot
    ret = pal_get_server_power(slot, &power);
    if (ret) {
      sleep(1);
      continue;
    }

    // Get health status for this slot
    ret = pal_get_fru_health(slot, &hlth);
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
      // This server is selcted one, set led_blink flag
      led_blink = 1;
    } else {
      led_blink = 0;
    }

    //If no identify: Set LEDs based on power and hlth status
    if (!led_blink) {
      if (!power) {
        pal_set_led(slot, LED_OFF);
        pal_set_id_led(slot, ID_LED_OFF);
        goto led_handler_out;
      }

      if (hlth == FRU_STATUS_GOOD) {
        pal_set_led(slot, LED_ON);
        pal_set_id_led(slot, ID_LED_OFF);
      } else {
        pal_set_led(slot, LED_OFF);
        pal_set_id_led(slot, ID_LED_ON);
      }
      goto led_handler_out;
    }

    // Set blink rate
    if (power) {
      led_on_time = 900;
      led_off_time = 100;
    } else {
      led_on_time = 100;
      led_off_time = 900;
    }

    // Start blinking the LED
    if (hlth == FRU_STATUS_GOOD) {
      pal_set_led(slot, LED_ON);
    } else {
      pal_set_id_led(slot, ID_LED_ON);
    }

    msleep(led_on_time);

    if (hlth == FRU_STATUS_GOOD) {
      pal_set_led(slot, LED_OFF);
    } else {
      pal_set_id_led(slot, ID_LED_OFF);
    }

    msleep(led_off_time);
led_handler_out:
    msleep(100);
  }

led_handler_exit:
  free(num);
}
#endif

// Thread to handle LED state of the SLED
static void *
led_sync_handler() {
  int ret;
  uint8_t pos;
  uint8_t ident = 0;
  char identify[16] = {0};
  char tstr[64] = {0};
  char id_arr[5] = {0};
  uint8_t slot;
  uint8_t spb_hlth = 0;
  uint8_t nic_hlth = 0;

  while (1) {
    // Handle Slot IDENTIFY condition
    memset(identify, 0x0, 16);
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

#if 0
    // Handle Sled level health condition
    ret = pal_get_fru_health(FRU_MB, &mb_hlth);
    if (ret) {
      sleep(1);
      continue;
    }

    ret = pal_get_fru_health(FRU_NIC, &nic_hlth);
    if (ret) {
      sleep(1);
      continue;
    }

    if (spb_hlth == FRU_STATUS_BAD || nic_hlth == FRU_STATUS_BAD) {
      // Turn OFF Blue LED
      for (slot = 1; slot <= MAX_NUM_SLOTS; slot++) {
        g_sync_led[slot] = 1;
        pal_set_led(slot, LED_OFF);
      }

      // Start blinking the Yellow/ID LED
      for (slot = 1; slot <= MAX_NUM_SLOTS; slot++) {
        pal_set_id_led(slot, ID_LED_ON);
      }

      msleep(LED_ON_TIME_HEALTH);

      for (slot = 1; slot <= MAX_NUM_SLOTS; slot++) {
        pal_set_id_led(slot, ID_LED_OFF);
      }
      msleep(LED_OFF_TIME_HEALTH);
      continue;
    }

    // Check if slot needs to be identified
    ident = 0;
    for (slot = 1; slot <= MAX_NUM_SLOTS; slot++)  {
      id_arr[slot] = 0x0;
      sprintf(tstr, "identify_slot%d", slot);
      memset(identify, 0x0, 16);
      ret = pal_get_key_value(tstr, identify);
      if (ret == 0 && !strcmp(identify, "on")) {
        id_arr[slot] = 0x1;
        ident = 1;
      }
    }

    // Get hand switch position to see if this is selected server
    ret = pal_get_hand_sw(&pos);
    if (ret) {
      sleep(1);
      continue;
    }

    // Handle BMC select condition when no slot is being identified
    if ((pos == HAND_SW_BMC) && (ident == 0)) {
       // Check hand sw position for bounce logic
      msleep(100);
      ret = pal_get_hand_sw(&pos);
      if ((ret) || (pos != HAND_SW_BMC)) {
        sleep(1);
        continue;
      }

      // Turn OFF Yellow LED
      for (slot = 1; slot <= MAX_NUM_SLOTS; slot++) {
        g_sync_led[slot] = 1;
        pal_set_id_led(slot, ID_LED_OFF);
      }

      // Start blinking Blue LED
      for (slot = 1; slot <= 4; slot++) {
        pal_set_led(slot, LED_ON);
      }

      msleep(LED_ON_TIME_BMC_SELECT);

      for (slot = 1; slot <= 4; slot++) {
        pal_set_led(slot, LED_OFF);
      }

      msleep(LED_OFF_TIME_BMC_SELECT);
      continue;
    }

    // Handle individual identify slot condition
    if (ident) {
      for (slot = 1; slot <=4; slot++) {
        if (id_arr[slot]) {
          g_sync_led[slot] = 1;
          pal_set_led(slot, LED_OFF);
          pal_set_id_led(slot, ID_LED_ON);
        } else {
          g_sync_led[slot] = 0;
        }
      }

      msleep(LED_ON_TIME_IDENTIFY);

      for (slot = 1; slot <=4; slot++) {
        if (id_arr[slot]) {
          pal_set_id_led(slot, ID_LED_OFF);
        }
      }

      msleep(LED_OFF_TIME_IDENTIFY);
      continue;
    }
    for (slot = 1; slot <= 4; slot++) {
      g_sync_led[slot] = 0;
    }
    msleep(200);
#endif
  }
}

int
main (int argc, char * const argv[]) {
  pthread_t tid_hand_sw;
  pthread_t tid_debug_card;
  pthread_t tid_rst_btn;
  pthread_t tid_pwr_btn;
  pthread_t tid_ts;
  pthread_t tid_sync_led;
  pthread_t tid_leds[MAX_NUM_SLOTS];
  int i;
  int *ip;
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
   daemon(0, 1);
   openlog("front-paneld", LOG_CONS, LOG_DAEMON);
  }


  if (pthread_create(&tid_debug_card, NULL, debug_card_handler, NULL) < 0) {
    syslog(LOG_WARNING, "pthread_create for debug card error\n");
    exit(1);
  }
#if 0
  if (pthread_create(&tid_hand_sw, NULL, usb_handler, NULL) < 0) {
    syslog(LOG_WARNING, "pthread_create for hand switch error\n");
    exit(1);
  }

  if (pthread_create(&tid_rst_btn, NULL, rst_btn_handler, NULL) < 0) {
    syslog(LOG_WARNING, "pthread_create for reset button error\n");
    exit(1);
  }

  if (pthread_create(&tid_pwr_btn, NULL, pwr_btn_handler, NULL) < 0) {
    syslog(LOG_WARNING, "pthread_create for power button error\n");
    exit(1);
  }

#endif
  if (pthread_create(&tid_ts, NULL, ts_handler, NULL) < 0) {
    syslog(LOG_WARNING, "pthread_create for time stamp error\n");
    exit(1);
  }

  if (pthread_create(&tid_sync_led, NULL, led_sync_handler, NULL) < 0) {
    syslog(LOG_WARNING, "pthread_create for led sync error\n");
    exit(1);
  }

#if 0
  for (i = 0; i < MAX_NUM_SLOTS; i++) {
    ip = malloc(sizeof(int));
    *ip = i;
    if (pthread_create(&tid_leds[i], NULL, led_handler, (void*)ip) < 0) {
      syslog(LOG_WARNING, "pthread_create for led error\n");
      exit(1);
    }
  }
#endif
  pthread_join(tid_debug_card, NULL);
  pthread_join(tid_sync_led, NULL);
  pthread_join(tid_ts, NULL);
#if 0
  pthread_join(tid_hand_sw, NULL);
  pthread_join(tid_rst_btn, NULL);
  pthread_join(tid_pwr_btn, NULL);
  for (i = 0;  i < MAX_NUM_SLOTS; i++) {
    pthread_join(tid_leds[i], NULL);
  }
#endif
  return 0;
}
