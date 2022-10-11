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
#include <string.h>
#include <openbmc/kv.h>
#include <openbmc/pal.h>
#include <facebook/fby3_common.h>
#include <facebook/bic.h>
#include <openbmc/obmc-i2c.h>

// Function to reverse elements of an array
void reverse(uint8_t arr[], int n)
{
    uint8_t aux[n];

    for ( int i = 0; i < n; i++ ) {
        aux[n-1-i] = arr[i];
    }

    for ( int i = 0; i < n; i++ ) {
        arr[i] = aux[i];
    }
}

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
    if ( pal_is_debug_card_prsnt(&card_prsnt, READ_FROM_BIC) < 0 || card_prsnt == 0 ) {
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
#define CPLD_STAGE 0x0A
#define CPLD_VER 0x05
  int i = 0;
  uint8_t slot_hlth = 0;
  uint8_t bmc_location = 0;
  uint8_t num_of_slots = 0;
  uint8_t status = 0;
  bool slot_err[4] = {false, false, false, false};
  bool slot_support[4] = {false, false, false, false};
  bool slot_ready[4] = {false, false, false, false};
  uint8_t led_mode = LED_CRIT_PWR_OFF_MODE;
  bool set_led = false;
  uint32_t ver_reg = ON_CHIP_FLASH_USER_VER;
  uint8_t tbuf[4] = {0x00};
  uint8_t rbuf[4] = {0x00};
  uint8_t tlen = 4;
  uint8_t rlen = 4;
  int ret, i2cfd = 0;
  uint8_t bus = 0;

  // cwc fru health
  uint8_t cwc_fru_hlth = FRU_STATUS_GOOD;
  uint8_t top_fru_hlth = FRU_STATUS_GOOD;
  uint8_t bot_fru_hlth = FRU_STATUS_GOOD;

  memcpy(tbuf, (uint8_t *)&ver_reg, tlen);
  reverse(tbuf, tlen);

  fby3_common_get_bmc_location(&bmc_location);
  num_of_slots = (bmc_location == NIC_BMC)?FRU_SLOT1:FRU_SLOT4;

  while(1) {
    for ( i = FRU_SLOT1; i <= num_of_slots; i++ ) {
      if ( pal_is_fw_update_ongoing(i) == true ) {
        // the time it takes to update the firmware depends on the component
        // sleep periodically because we don't need to check it frequently
        sleep(10);
        continue;
      }

      ret = pal_get_server_power(i, &status);
      if ( ret < 0 || status == SERVER_12V_OFF ) {
        slot_ready[i-1] = false;
        continue;
      }

      // Re-init when Server reset
      if ( slot_ready[i-1] == false ) {
        slot_ready[i-1] = true;
        //Check SB CPLD Version
        ret = fby3_common_get_bus_id(i) + 4;
        if ( ret < 0 ) {
          syslog(LOG_WARNING, "%s() Cannot get the bus with fru%d", __func__, i);
          continue;
        }

        bus = (uint8_t)ret;
        i2cfd = i2c_cdev_slave_open(bus, CPLD_UPDATE_ADDR, I2C_SLAVE_FORCE_CLAIM);
        if ( i2cfd < 0 ) {
          syslog(LOG_WARNING, "%s() Failed to open bus %d. Err: %s", __func__, bus, strerror(errno));
          continue;
        }

        ret = i2c_rdwr_msg_transfer(i2cfd, (CPLD_UPDATE_ADDR << 1), tbuf, tlen, rbuf, rlen);
        if ( i2cfd > 0 ) close(i2cfd);
        if ( ret < 0 ) {
          syslog(LOG_WARNING, "%s() Failed to do i2c_rdwr_msg_transfer, tlen=%d, fru %d", __func__, tlen, i);
          continue;
        }

        // CPLD firmware version should later than A05.
        if (( rbuf[1] == CPLD_STAGE ) && ( rbuf[0] >= CPLD_VER )) {
          slot_support[i-1] = true;
        } else {
          slot_support[i-1] = false;
          syslog(LOG_WARNING, "%s() Cannot handle LED state with fru%d, please check the SB CPLD f/w version is later than %02X%02X", __func__, i, CPLD_STAGE, CPLD_VER);
        }
      }

      if ( slot_support[i-1] == false ) {
        continue;
      }

      // get health status
      if (pal_is_cwc() == PAL_EOK) {

        if ( pal_get_fru_health(FRU_CWC, &cwc_fru_hlth) < 0 ) {
          syslog(LOG_WARNING,"%s() failed to get the health status of cwc fru%d\n", __func__, FRU_CWC);
          sleep(DELAY_PERIOD);
          continue;
        }

        if ( pal_get_fru_health(FRU_2U_TOP, &top_fru_hlth) < 0 ) {
          syslog(LOG_WARNING,"%s() failed to get the health status of top gpv3 fru%d\n", __func__, FRU_2U_TOP);
          sleep(DELAY_PERIOD);
          continue;
        }

        if ( pal_get_fru_health(FRU_2U_BOT, &bot_fru_hlth) < 0 ) {
          syslog(LOG_WARNING,"%s() failed to get the health status of bot gpv3 fru%d\n", __func__, FRU_2U_BOT);
          sleep(DELAY_PERIOD);
          continue;
        }
      }

      if ( pal_get_fru_health(i, &slot_hlth) < 0 ) {
        syslog(LOG_WARNING,"%s() failed to get the health status of slot%d\n", __func__, i);
        sleep(DELAY_PERIOD);
        continue;
      }

      if ( pal_is_cwc() == PAL_EOK ) {
        slot_hlth = slot_hlth & cwc_fru_hlth & top_fru_hlth & bot_fru_hlth;
      }

      // if it's FRU_STATUS_BAD, get the current power status
      if ( slot_hlth == FRU_STATUS_BAD && slot_err[i-1] == false ) {
        if ( pal_get_server_power(i, &status) < 0 ) {
          syslog(LOG_WARNING,"%s() failed to get the power status of slot%d\n", __func__, i);
        } else {
          // set status
          led_mode = ( status == SERVER_POWER_OFF )?LED_CRIT_PWR_OFF_MODE:LED_CRIT_PWR_ON_MODE;
          set_led = true;
          slot_err[i-1] = true;
        }
      } else if ( slot_hlth == FRU_STATUS_GOOD && slot_err[i-1] == true ) {
        // set status
        // led_mode - dont care
        set_led = true;
        slot_err[i-1] = false;
      }

      // skip it if it's not needed
      if ( set_led == true ) {
        if ( pal_sb_set_amber_led(i, slot_err[i-1], led_mode) < 0 ) {
          syslog(LOG_WARNING,"%s() failed to set the fault led of slot%d\n", __func__, i);
        } else set_led = false;
      }
    }
    sleep(DELAY_PERIOD);
  }
  return 0;
}

int
main (int argc, char * const argv[]) {
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
