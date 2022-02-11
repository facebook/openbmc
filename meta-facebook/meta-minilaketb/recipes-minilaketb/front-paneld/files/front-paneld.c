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
#include <openbmc/kv.h>
#include <openbmc/ipmi.h>
#include <openbmc/ipmb.h>
#include <openbmc/pal.h>

#define BTN_MAX_SAMPLES   200
#define BTN_POWER_OFF     40
#define BTN_HSVC          100
#define MAX_NUM_SLOTS 1
#define HB_SLEEP_TIME (5 * 60)
#define HB_TIMESTAMP_COUNT (60 * 60 / HB_SLEEP_TIME)

#define LED_ON 1
#define LED_OFF 0

#define ID_LED_ON 0
#define ID_LED_OFF 1

#define LED_ON_TIME_IDENTIFY 200
#define LED_OFF_TIME_IDENTIFY 200

#define LED_ON_TIME_HEALTH 900
#define LED_OFF_TIME_HEALTH 100

#define LED_ON_TIME_BMC_SELECT 500
#define LED_OFF_TIME_BMC_SELECT 500

#define GPIO_VAL "/sys/class/gpio/gpio%d/value"

static uint8_t m_pos = 0xff;

static int
get_handsw_pos(uint8_t *pos) {
  if ((m_pos > HAND_SW_BMC) || (m_pos < HAND_SW_SERVER1))
    return -1;

  *pos = m_pos;
  return 0;
}

// Thread for monitoring debug card hotswap
static void *
debug_card_handler() {
  int curr = -1;
  int prev = -1;
  int ret;
  uint8_t prsnt = 0;
  uint8_t pos;
  uint8_t prev_pos = 0xff, prev_phy_pos = 0xff;
  uint8_t lpc;
  char str[8];

  while (1) {
    ret = pal_get_hand_sw_physically(&pos);
    if (ret) {
      goto debug_card_out;
    }

    if (pos == prev_phy_pos) {
      goto get_hand_sw_cache;
    }

    msleep(10);
    ret = pal_get_hand_sw_physically(&pos);
    if (ret) {
      goto debug_card_out;
    }

    prev_phy_pos = pos;
    sprintf(str, "%u", pos);
    ret = kv_set("spb_hand_sw", str, 0, 0);
    if (ret) {
      goto debug_card_out;
    }

get_hand_sw_cache:
    ret = pal_get_hand_sw(&pos);
    if (ret) {
      goto debug_card_out;
    }

    if (pos == prev_pos) {
      goto debug_card_prs;
    }
    m_pos = pos;

    ret = pal_switch_vga_mux(pos);
    if (ret) {
      goto debug_card_out;
    }

    // ret = pal_switch_usb_mux(pos);
    // if (ret) {
    //   goto debug_card_out;
    // }

    ret = pal_switch_uart_mux(pos);
    if (ret) {
      goto debug_card_out;
    }

debug_card_prs:
    // Check if debug card present or not
    ret = pal_is_debug_card_prsnt(&prsnt);
    if (ret) {
      goto debug_card_out;
    }
    curr = prsnt;

    // Check if Debug Card was either inserted or removed
    if (curr != prev) {
      if (!curr) {
        // Debug Card was removed
        syslog(LOG_WARNING, "Debug Card Extraction\n");
      } else {
        // Debug Card was inserted
        syslog(LOG_WARNING, "Debug Card Insertion\n");
      }
    }

    if ((pos == prev_pos) && (curr == prev)) {
      goto debug_card_out;
    }

    ret = pal_switch_uart_mux(pos);
    if (ret) {
      goto debug_card_out;
    }

    // If Debug Card is present
    if (curr) {
      // Enable POST code based on hand switch
      if (pos == HAND_SW_BMC) {
        // For BMC, there is no need to have POST specific code
        goto debug_card_done;
      }

      // Make sure the server at selected position is ready
      ret = pal_is_fru_ready(pos, &prsnt);
      if (ret || !prsnt) {
        goto debug_card_done;
      }

      // Enable POST codes for all slots
      ret = pal_post_enable(pos);
      if (ret) {
        goto debug_card_done;
      }

      // Get last post code and display it
      ret = pal_post_get_last(pos, &lpc);
      if (ret) {
        goto debug_card_done;
      }

      ret = pal_post_handle(pos, lpc);
      if (ret) {
        goto debug_card_out;
      }
    }

debug_card_done:
    prev = curr;
    prev_pos = pos;
debug_card_out:
    if (curr == 1)
      msleep(500);
    else
      sleep(1);
  }

  return 0;
}

// Thread to monitor Reset Button and propagate to selected server
static void *
rst_btn_handler() {
  int ret;
  uint8_t pos;
  int i;
  uint8_t btn;
  uint8_t last_btn = 0;

  ret = pal_get_rst_btn(&btn);
  if (0 == ret) {
    last_btn = btn;
  }

  while (1) {
    // Check the position of hand switch
    ret = get_handsw_pos(&pos);
    if (ret || pos == HAND_SW_BMC) {
      // For BMC, no need to handle Reset Button
      sleep(1);
      continue;
    }

    // Check if reset button is pressed
    ret = pal_get_rst_btn(&btn);
    if (ret || !btn) {
      if (last_btn != btn) {
        pal_set_rst_btn(pos, 1);
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

// Thread to handle Power Button and power on/off the selected server
static void *
pwr_btn_handler() {
  int ret, i;
  uint8_t pos, btn;
  uint8_t cmd = 0;
  uint8_t power, st_12v = 0;
  char tstr[64];
  bool release_flag = true;

  while (1) {

    // Check the position of hand switch
    ret = get_handsw_pos(&pos);
    if (ret) {
      sleep(1);
      continue;
    }

    // Check if power button is pressed
    ret = pal_get_pwr_btn(&btn);
    if (ret || !btn) {
      if (false == release_flag)
        release_flag = true;

      goto pwr_btn_out;
    }

    if (false == release_flag)
      goto pwr_btn_out;

    release_flag = false;
    syslog(LOG_WARNING, "Power button pressed\n");

    // Wait for the button to be released
    for (i = 0; i < BTN_HSVC; i++) {
      ret = pal_get_pwr_btn(&btn);
      if (ret || btn ) {
        msleep(100);
        continue;
      }

      release_flag = true;
      syslog(LOG_WARNING, "Power button released\n");
      break;
    }

    // Get the current power state (power on vs. power off)
    if (pos != HAND_SW_BMC) {
      ret = pal_is_server_12v_on(pos, &st_12v);
      if (ret) {
        goto pwr_btn_out;
      }

      if (i >= BTN_HSVC) {
        pal_update_ts_sled();
        syslog(LOG_CRIT, "Power Button Long Press for FRU: %d\n", pos);

      }

      if (st_12v) {
        ret = pal_get_server_power(pos, &power);
        if (ret) {
          goto pwr_btn_out;
        }
        // Set power command should reverse of current power state
        cmd = !power;
      }
    }

    // To determine long button press
    if (i >= BTN_POWER_OFF) {
      // if long press (>4s) and hand-switch position == bmc, then initiate
      // sled-cycle
      if (pos == HAND_SW_BMC) {
          // minilaketb do nothing here.
      } else {
        if (i < BTN_HSVC) {
          pal_update_ts_sled();
          syslog(LOG_CRIT, "Power Button Long Press for FRU: %d", pos);
        }

        if (!st_12v) {
          sprintf(tstr, "/usr/local/bin/power-util slot%u 12V-on", pos);
          run_command(tstr);
          goto pwr_btn_out;
        }
      }
    } else {
      // If current power state is ON and it is not a long press,
      // the power off should be Graceful Shutdown
      if (power == SERVER_POWER_ON)
        cmd = SERVER_GRACEFUL_SHUTDOWN;

      pal_update_ts_sled();
      syslog(LOG_CRIT, "Power Button Press for FRU: %d\n", pos);
    }

    if ((pos != HAND_SW_BMC) && st_12v) {
      if (cmd == SERVER_POWER_ON)
        pal_set_restart_cause(pos, RESTART_CAUSE_PWR_ON_PUSH_BUTTON);

      // Reverse the power state of the given server
      ret = pal_set_server_power(pos, cmd);
    }

pwr_btn_out:
    msleep(100);
  }
  return 0;
}

// Thread to monitor SLED Cycles by using time stamp
static void *
ts_handler() {
  int count = 0;
  struct timespec ts;
  struct timespec mts;
  char tstr[64] = {0};
  char buf[128] = {0};
  uint8_t time_init = 0;
  long time_sled_on;
  long time_sled_off;

  // Read the last timestamp from KV storage
  pal_get_key_value("timestamp_sled", tstr);
  time_sled_off = (long) strtoul(tstr, NULL, 10);

  while (1) {

    // Make sure the time is initialized properly
    // Since there is no battery backup, the time could be reset to build time
    if (time_init < 100) {  // wait 100s at most, to prevent infinite waiting
      // Read current time
      clock_gettime(CLOCK_REALTIME, &ts);

      if ((ts.tv_sec < time_sled_off) && (++time_init < 100)) {
        sleep(1);
        continue;
      }

      // If current time is more than the stored time, the date is correct
      time_init = 100;
      // Need to log SLED ON event, if this is Power-On-Reset
      if (pal_is_bmc_por()) {
        ctime_r(&time_sled_off, buf);
        syslog(LOG_CRIT, "SLED Powered OFF at %s", buf);

        sprintf(buf, "AC lost");
        pal_add_cri_sel(buf);

        // Get uptime
        clock_gettime(CLOCK_MONOTONIC, &mts);
        // To find out when SLED was on, subtract the uptime from current time
        time_sled_on = ts.tv_sec - mts.tv_sec;

        ctime_r(&time_sled_on, buf);
        // Log an event if this is Power-On-Reset
        syslog(LOG_CRIT, "SLED Powered ON at %s", buf);
      }
    }

    // Store timestamp every one hour to keep track of SLED power
    if (count++ == HB_TIMESTAMP_COUNT) {
      pal_update_ts_sled();
      count = 0;
    }

    sleep(HB_SLEEP_TIME);
  }

  return 0;
}

// Thread to handle LED state of the server at given slot
static void *
led_handler() {
  char identify[16] = {0};
  int ret;
  uint8_t pos;
  uint8_t ready;
  uint8_t power, hlth;
  int led_on_time, led_off_time;

  while (1) {
    // Get hand switch position to see if this is selected server
    ret = get_handsw_pos(&pos);
    if (ret != 0) {
      sleep(1);
      continue;
    }

    // Handle Slot IDENTIFY condition
    memset(identify, 0x0, sizeof(identify));
    ret = pal_get_key_value("identify_slot1", identify);
    if (ret == 0 && !strcmp(identify, "on")) {
      // Turn OFF Blue LED
      pal_set_led(SERVER1, LED_OFF);

      // Start blinking the ID LED
      pal_set_id_led(SERVER1, ID_LED_ON);

      msleep(LED_ON_TIME_IDENTIFY);

      pal_set_id_led(SERVER1, ID_LED_OFF);

      msleep(LED_OFF_TIME_IDENTIFY);
      continue;
    }

    if (pos == HAND_SW_BMC) {
      // Turn OFF Yellow LED
      pal_set_id_led(BMC, ID_LED_OFF);

      // Start blinking the LED
      pal_set_led(BMC, LED_ON);

      msleep(LED_ON_TIME_BMC_SELECT);

      pal_set_led(BMC, LED_OFF);

      msleep(LED_OFF_TIME_BMC_SELECT);
      continue;
    }

    ret = pal_is_fru_ready(pos, &ready);
    if (!ret && ready) {
      // Get power status for this slot
      ret = pal_get_server_power(pos, &power);
      if (ret) {
        continue;
      }

      // Get health status for this slot
      ret = pal_get_fru_health(pos, &hlth);
      if (ret) {
	continue;
      }
    } else {
      power = SERVER_POWER_OFF;
      hlth = FRU_STATUS_GOOD;
    }

    // Set blink rate
    if (power == SERVER_POWER_ON) {
      led_on_time = 900;
      led_off_time = 100;
    } else {
      led_on_time = 100;
      led_off_time = 900;
    }

    if (hlth == FRU_STATUS_GOOD) {
      pal_set_led(pos, LED_ON);
      pal_set_id_led(pos, ID_LED_OFF);
    } else {
      pal_set_led(pos, LED_OFF);
      pal_set_id_led(pos, ID_LED_ON);
    }

    msleep(led_on_time);

    if (hlth == FRU_STATUS_GOOD) {
      pal_set_led(pos, LED_OFF);
    } else {
      pal_set_id_led(pos, ID_LED_OFF);
    }

    msleep(led_off_time);

  }

  return 0;
}

int
main (int argc, char * const argv[]) {
  pthread_t tid_debug_card;
  pthread_t tid_rst_btn;
  pthread_t tid_pwr_btn;
  pthread_t tid_ts;
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
   openlog("front-paneld", LOG_CONS, LOG_DAEMON);
  }


  if (pthread_create(&tid_debug_card, NULL, debug_card_handler, NULL) < 0) {
    syslog(LOG_WARNING, "pthread_create for debug card error\n");
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

  if (pthread_create(&tid_ts, NULL, ts_handler, NULL) < 0) {
    syslog(LOG_WARNING, "pthread_create for time stamp error\n");
    exit(1);
  }

  if (pthread_create(&tid_led, NULL, led_handler, NULL) < 0) {
    syslog(LOG_WARNING, "pthread_create for led error\n");
    exit(1);
  }

  pthread_join(tid_debug_card, NULL);
  pthread_join(tid_rst_btn, NULL);
  pthread_join(tid_pwr_btn, NULL);
  pthread_join(tid_ts, NULL);
  pthread_join(tid_led, NULL);

  return 0;
}
