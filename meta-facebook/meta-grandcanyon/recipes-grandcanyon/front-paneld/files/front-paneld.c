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

#define LED_INTERVAL_DEFAULT 500 //millisecond

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

int
main (int argc, char * const argv[]) {
  pthread_t tid_sync_led = 0;
  int ret = 0, pid_file = 0;

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
    ret = -1;
    goto err;
  }

  pthread_join(tid_sync_led, NULL);

err:
  flock(pid_file, LOCK_UN);
  close(pid_file);
  return ret;
}
