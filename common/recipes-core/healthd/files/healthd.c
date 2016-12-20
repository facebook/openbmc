/*
 * healthd
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

#include <stdlib.h>
#include <stdio.h>
#include <syslog.h>
#include <errno.h>
#include <unistd.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/file.h>
#include <pthread.h>
#include <openbmc/pal.h>
#include "watchdog.h"

static void
initilize_all_kv() {
  pal_set_def_key_value();
}

static void *
hb_handler() {
  int hb_interval;

#ifdef HB_INTERVAL
  hb_interval = HB_INTERVAL;
#else
  hb_interval = 500;
#endif

  while(1) {
    /* Turn ON the HB Led*/
    pal_set_hb_led(1);
    msleep(hb_interval);

    /* Turn OFF the HB led */
    pal_set_hb_led(0);
    msleep(hb_interval);
  }
}

static void *
watchdog_handler() {

  /* Start watchdog in manual mode */
  start_watchdog(0);

  /* Set watchdog to persistent mode so timer expiry will happen independent
   * of this process's liveliness.
   */
  set_persistent_watchdog(WATCHDOG_SET_PERSISTENT);

  while(1) {

    sleep(5);

    /*
     * Restart the watchdog countdown. If this process is terminated,
     * the persistent watchdog setting will cause the system to reboot after
     * the watchdog timeout.
     */
    kick_watchdog();
  }
}

int
main(int argc, void **argv) {
  int dev, rc, pid_file;
  pthread_t tid_watchdog;
  pthread_t tid_hb_led;

  if (argc > 1) {
    exit(1);
  }

  pid_file = open("/var/run/healthd.pid", O_CREAT | O_RDWR, 0666);
  rc = flock(pid_file, LOCK_EX | LOCK_NB);
  if(rc) {
    if(EWOULDBLOCK == errno) {
      printf("Another healthd instance is running...\n");
      exit(-1);
    }
  } else {

    daemon(1,1);

    openlog("healthd", LOG_CONS, LOG_DAEMON);
    syslog(LOG_INFO, "healthd: daemon started");
  }

  initilize_all_kv();

// For current platforms, we are using WDT from either fand or fscd
// TODO: keeping this code until we make healthd as central daemon that
//  monitors all the important daemons for the platforms.
  if (pthread_create(&tid_watchdog, NULL, watchdog_handler, NULL) < 0) {
    syslog(LOG_WARNING, "pthread_create for watchdog error\n");
    exit(1);
  }

  if (pthread_create(&tid_hb_led, NULL, hb_handler, NULL) < 0) {
    syslog(LOG_WARNING, "pthread_create for heartbeat error\n");
    exit(1);
  }

  pthread_join(tid_watchdog, NULL);

  pthread_join(tid_hb_led, NULL);

  return 0;
}

