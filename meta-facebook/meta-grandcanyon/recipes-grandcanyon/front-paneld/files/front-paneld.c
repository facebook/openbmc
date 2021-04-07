/*
 *
 * Copyright 2020-present Facebook. All Rights Reserved.
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

#define LED_INTERVAL_DEFAULT              500 //millisecond
#define MONITOR_FRU_HEALTH_INTERVAL       1 //second
#define SYNC_SYSTEM_STATUS_LED_INTERVAL   1 //second

#define MAX_NUM_CHECK_FRU_HEALTH          5

// Thread to handle LED state of the SLED
static void *
led_sync_handler() {
  int ret = 0, ret2 = 0;
  char identify[MAX_VALUE_LEN] = {0};
  char interval[MAX_VALUE_LEN] = {0};

  while (1) {
    // Handle Slot IDENTIFY condition
    memset(identify, 0x0, sizeof(identify));
    memset(interval, 0x0, sizeof(interval));
    
    ret = pal_get_key_value("system_identify_server", identify);
    ret2 = pal_get_key_value("system_identify_led_interval", interval);
    
    if ((ret == 0) && (strcmp(identify, "on") == 0)) {
      // Start blinking the ID LED
      pal_set_id_led(FRU_UIC, LED_ON);
      
      if ((ret2 == 0) && (strcmp(interval, "default") == 0)) {
        msleep(LED_INTERVAL_DEFAULT);
      } else if (ret2 == 0) {
        sleep(atoi(interval));
      }
      
      pal_set_id_led(FRU_UIC, LED_OFF);

      if ((ret2 == 0) && (strcmp(interval, "default") == 0)) {
        msleep(LED_INTERVAL_DEFAULT);
      } else if (ret2 == 0) {
        sleep(atoi(interval));
      }
      continue;
    } else if ((ret == 0) && (strcmp(identify, "off") == 0)) {
      pal_set_id_led(FRU_UIC, LED_ON);
    }
    sleep(1);
  }

  return NULL;
}

// Thread to handle different case of system status LED
// Define on OpenBMC spec. "3.2.4 Status/Fault LED"
static void *
system_status_led_handler() {
  int ret = 0, blink_count = 1;
  uint8_t server_power_status = SERVER_12V_OFF;
  uint8_t error[MAX_NUM_ERR_CODES] = {0}, error_count = 0;
  
  memset(error, 0, sizeof(error));

  while(1) {
    //1. Solid Yellow: BMC booting, BMC fault, Expander fault, sensor fault, NIC fault, or server fault
    ret = pal_get_error_code(error, &error_count);
    if ((error_count > 0) || (ret < 0)) {
      ret = pal_set_status_led(FRU_UIC, STATUS_LED_YELLOW);
      if (ret < 0) {
        syslog(LOG_WARNING, "%s(): failed to set server status LED to solid yellow", __func__);
      }
      
    } else {
      ret = pal_get_server_power(FRU_SERVER, &server_power_status);
      if (ret < 0) {
        //if can't get server power status, keep system status LED solid yellow
        syslog(LOG_WARNING, "%s(): failed to get server power status", __func__);
        ret = pal_set_status_led(FRU_UIC, STATUS_LED_YELLOW);
      
      } else {
        //2. Solid Blue: BMC and server are both power on and ok
        if (server_power_status == SERVER_POWER_ON) {
          ret = pal_set_status_led(FRU_UIC, STATUS_LED_BLUE);
          if (ret < 0) {
            syslog(LOG_WARNING, "%s(): failed to set server status LED to solid blue", __func__);
          }
        
        //3. Blinking Yellow: BMC on, but server power off
        } else {
          if (blink_count > 0) {
            ret = pal_set_status_led(FRU_UIC, STATUS_LED_YELLOW);
          } else {
            ret = pal_set_status_led(FRU_UIC, STATUS_LED_OFF);
          }
          blink_count = blink_count * -1;
        }
      }
    }
    
    sleep(SYNC_SYSTEM_STATUS_LED_INTERVAL);
  } // end while loop 
  
  return NULL;
}

// Thread to handle fru health
static void *
fru_health_handler() {
  uint8_t health = FRU_STATUS_GOOD;
  uint8_t fru_list[MAX_NUM_CHECK_FRU_HEALTH] = {FRU_SERVER, FRU_UIC, FRU_DPB, FRU_SCC, FRU_NIC};
  uint8_t health_error_code_list[MAX_NUM_CHECK_FRU_HEALTH] = {
       ERR_CODE_SERVER_HEALTH, ERR_CODE_UIC_HEALTH, ERR_CODE_DPB_HEALTH, 
       ERR_CODE_SCC_HEALTH, ERR_CODE_NIC_HEALTH};
  int i = 0, ret = 0;

  while(1) {
    // fru health
    for (i = 0; i < sizeof(fru_list); i++) {
      ret = pal_get_fru_health(fru_list[i], &health);
      if (ret < 0) {
        health = FRU_STATUS_BAD;
      }
      
      if (health == FRU_STATUS_GOOD) {
        pal_set_error_code(health_error_code_list[i], ERR_CODE_DISABLE);
      } else if (health == FRU_STATUS_BAD) {
        pal_set_error_code(health_error_code_list[i], ERR_CODE_ENABLE);
      }
    } // end for loop
    sleep(MONITOR_FRU_HEALTH_INTERVAL);
  } // end while loop 
  
  return NULL;
}

int
main (int argc, char * const argv[]) {
  pthread_t tid_sync_led = 0;
  pthread_t tid_monitor_fru_health = 0;
  pthread_t tid_sync_system_status_led = 0;
  int ret = 0, pid_file = 0;
  int ret_sync_led = 0, ret_monitor_fru = 0, ret_sync_system_status_led = 0;

  pid_file = open("/var/run/front-paneld.pid", O_CREAT | O_RDWR, 0666);
  if (pid_file < 0) {
    syslog(LOG_WARNING, "Fail to open front-paneld.pid file\n");
    return -1;
  }

  ret = flock(pid_file, LOCK_EX | LOCK_NB);
  if (ret != 0) {
    if(EWOULDBLOCK == errno) {
      syslog(LOG_WARNING, "Another front-paneld instance is running...\n");
      ret = -1;
      goto err;
    }
  } else {
    openlog("front-paneld", LOG_CONS, LOG_DAEMON);
  }

  if (pthread_create(&tid_sync_led, NULL, led_sync_handler, NULL) < 0) {
    syslog(LOG_WARNING, "pthread_create for led sync error\n");
    ret_sync_led = -1;
  }

  if (pthread_create(&tid_monitor_fru_health, NULL, fru_health_handler, NULL) < 0) {
    syslog(LOG_WARNING, "fail to creat thread for monitor fru health\n");
    ret_monitor_fru = -1;
  }

  if (pthread_create(&tid_sync_system_status_led, NULL, system_status_led_handler, NULL) < 0) {
    syslog(LOG_WARNING, "fail to creat thread for sync system status LED\n");
    ret_sync_system_status_led = -1;
  }
  
  if (ret_sync_led == 0) {
    pthread_join(tid_sync_led, NULL);
  }
  if (ret_monitor_fru == 0) {
    pthread_join(tid_monitor_fru_health, NULL);
  }
  if (ret_sync_system_status_led == 0) {
    pthread_join(tid_sync_system_status_led, NULL);
  }

err:
  flock(pid_file, LOCK_UN);
  close(pid_file);
  return ret;
}
