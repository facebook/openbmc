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
#include <facebook/netlakemtp_common.h>

#define IDENTIFY_LED_DELAY_PERIOD   2    /*second*/
#define FAULT_LED_DELAY_PERIOD      2    /*second*/
#define MIN_POLL_INTERVAL           2    /*second*/

#define MAX_NUM_CHECK_FRU_HEALTH    2    /*second*/

static void *
identify_led_handler() {
  char identify[MAX_VALUE_LEN] = {0};
  uint8_t status = 0, led_mode = 0;
  int ret = 0;

  while (1) {
    memset(identify, 0x0, sizeof(identify));
    ret = pal_get_key_value("system_identify_led", identify);

    if ((ret == 0) && (strcmp(identify, "on") == 0)) {
      pal_set_id_led(FRU_SERVER, led_mode = !led_mode);
    } else {
      if (pal_get_server_power(FRU_SERVER, &status) < 0) {
        syslog(LOG_WARNING,"%s() failed to get the power status\n", __func__);
        continue;
      }

      led_mode = (status == SERVER_POWER_OFF) ? LED_OFF : LED_ON;
      pal_set_id_led(FRU_SERVER, led_mode);
    }

    sleep(IDENTIFY_LED_DELAY_PERIOD);
  }

  return 0;
}

static void *
fault_led_handler() {
  uint8_t error[MAX_NUM_ERR_CODES] = {0}, cur_error_count = 0;

  while (1) {
    pal_get_error_code(error, &cur_error_count);
    if (cur_error_count == 0) {
      pal_set_fault_led(FRU_SERVER, LED_OFF);
    } else {
      pal_set_fault_led(FRU_SERVER, LED_ON);
    }
    sleep(FAULT_LED_DELAY_PERIOD);
  }
  return 0;
}

static void *
fru_health_handler() {
  int fru_id = -1, ret = 0;
  uint8_t fru_list[MAX_NUM_CHECK_FRU_HEALTH] = {FRU_SERVER, FRU_PDB};
  uint8_t health_error_code_list[MAX_NUM_CHECK_FRU_HEALTH] = {ERR_CODE_FRU_SERVER_HEALTH, ERR_CODE_FRU_PDB_HEALTH};
  uint8_t health = FRU_STATUS_BAD;

  while (1) {
    for (fru_id = 0; fru_id < MAX_NUM_CHECK_FRU_HEALTH; fru_id++) {
      ret = pal_get_fru_health(fru_list[fru_id], &health);
      if (ret < 0) {
        health = FRU_STATUS_BAD;
      }

      if (health == FRU_STATUS_GOOD) {
        pal_set_error_code(health_error_code_list[fru_id], ERR_CODE_DISABLE);
      } else if (health == FRU_STATUS_BAD) {
        pal_set_error_code(health_error_code_list[fru_id], ERR_CODE_ENABLE);
      } else {
        syslog(LOG_WARNING, "Unknown status of FRU health\n");
      }
    }
    sleep(MIN_POLL_INTERVAL);
  }
  return 0;
}

int
main (int argc, char * const argv[]) {
  pthread_t tid_led;
  pthread_t tid_fault_led;
  pthread_t tid_monitor_fru_health;
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

  if (pthread_create(&tid_led, NULL, identify_led_handler, NULL) < 0) {
    syslog(LOG_WARNING, "pthread_create for led error\n");
  }

  if (pthread_create(&tid_fault_led, NULL, fault_led_handler, NULL) < 0) {
    syslog(LOG_WARNING, "pthread_create for led error\n");
  }

  if (pthread_create(&tid_monitor_fru_health, NULL, fru_health_handler, NULL) < 0) {
    syslog(LOG_WARNING, "fail to create thread to monitor fru health\n");
  }

  pthread_join(tid_led, NULL);
  pthread_join(tid_fault_led, NULL);
  pthread_join(tid_monitor_fru_health, NULL);

  return 0;
}
