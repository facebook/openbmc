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

#include <unistd.h>
#include <stdio.h>
#include <syslog.h>
#include <pthread.h>
#include <openbmc/kv.h>
#include <openbmc/pal.h>
#include <facebook/fby35_common.h>

// Thread for update the uart_select
static void *
debug_card_handler() {
#define  DELAY_READ 500
  int ret = 0;
  uint8_t card_prsnt = 0x00;
  uint8_t uart_select = 0x00;
  uint8_t prev_uart_select = 0xff;
  char str[8] = "\n";

  while (1) {
    // we start updating uart position sts when card is present
    if ( pal_is_debug_card_prsnt(&card_prsnt) < 0 || card_prsnt == 0 ) {
      msleep(DELAY_READ); //sleep
      continue;
    }

    // get the uart position
    ret = pal_get_uart_select_from_cpld(&uart_select);
    if (ret < 0) {
      syslog(LOG_WARNING, "%s() Failed to get debug_card_uart_select\n", __func__);
      msleep(DELAY_READ); //sleep
      continue;
    }

    // check if we need to update kv
    if ( uart_select != prev_uart_select ) {
      //detect the value has been changed
      sprintf(str, "%u", uart_select);
      ret = kv_set("debug_card_uart_select", str, 0, 0);
      if (ret < 0)  {
        syslog(LOG_WARNING, "%s() Failed to set debug_card_uart_select\n", __func__);
      } else {
        //update prev_uart_select
        prev_uart_select = uart_select;
      }
    }
    msleep(DELAY_READ);
  }

  return 0;
}

// Thread to handle LED state of the server at given slot
static void *
led_handler() {
#define DELAY_PERIOD 1
  int i = 0;
  uint8_t slot_hlth = 0;
  uint8_t bmc_location = 0;
  uint8_t num_of_slots = 0;
  uint8_t status = 0;
  uint8_t slot_err[FRU_SLOT4+1] = {0xff, 0xff, 0xff, 0xff, 0xff};
  uint8_t led_mode = LED_CRIT_PWR_OFF_MODE;
  bool set_led = false;
  int ret;

  fby35_common_get_bmc_location(&bmc_location);
  num_of_slots = (bmc_location == NIC_BMC)?FRU_SLOT1:FRU_SLOT4;

  // set flag to notice BMC front-paneld system_status_led_handler is ready
  kv_set("flag_front_sys_status_led", STR_VALUE_1, 0, 0);

  while(1) {
    for ( i = FRU_SLOT1; i <= num_of_slots; i++ ) {
      ret = pal_get_server_power(i, &status);
      if ( ret < 0 || status == SERVER_12V_OFF ) {
        slot_err[i] = 0xff;
        continue;
      }

      // get health status
      if ( pal_get_fru_health(i, &slot_hlth) < 0 ) {
        syslog(LOG_WARNING,"%s() failed to get the health status of slot%d\n", __func__, i);
        sleep(DELAY_PERIOD);
        continue;
      }

      set_led = false;
      if ( slot_hlth == FRU_STATUS_BAD && slot_err[i] != true ) {
        // if it's FRU_STATUS_BAD, get the current power status
        if ( pal_get_server_power(i, &status) < 0 ) {
          syslog(LOG_WARNING,"%s() failed to get the power status of slot%d\n", __func__, i);
        } else {
          // set status
          slot_err[i] = true;
          led_mode = ( status == SERVER_POWER_OFF )?LED_CRIT_PWR_OFF_MODE:LED_CRIT_PWR_ON_MODE;
          set_led = true;
        }
      } else if ( slot_hlth == FRU_STATUS_GOOD && slot_err[i] != false ) {
        // set status
        // led_mode - dont care
        slot_err[i] = false;
        set_led = true;
      }

      // skip it if it's not needed
      if ( set_led == true ) {
        if ( pal_sb_set_amber_led(i, slot_err[i], led_mode) < 0 ) {
          syslog(LOG_WARNING,"%s() failed to set the fault led of slot%d\n", __func__, i);
          slot_err[i] = 0xff;  // to set LED in next loop
        }
      }
    }
    sleep(DELAY_PERIOD);
  }
  return 0;
}


int
main() {
  pthread_t tid_debug_card;
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

  if (pthread_create(&tid_led, NULL, led_handler, NULL) < 0) {
    syslog(LOG_WARNING, "pthread_create for led error\n");
    exit(1);
  }

  pthread_join(tid_debug_card, NULL);
  pthread_join(tid_led, NULL);
  return 0;
}
