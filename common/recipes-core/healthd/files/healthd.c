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
#include <openbmc/pal.h>

static void
initilize_all_kv() {
  pal_set_def_key_value();
}

static void
check_all_daemon_health() {
  while(1) {

    /*
     * TODO: Check if all the daemons are running fine
     * TODO: Move the HB led control to this daemon
     * TODO: Move the watchdog timer to this daemon
     */
    sleep(10);
  }
}

int
main(int argc, void **argv) {
  int dev, rc, pid_file;

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

    daemon(0,1);
    openlog("healthd", LOG_CONS, LOG_DAEMON);
    syslog(LOG_INFO, "healthd: daemon started");
  }

  initilize_all_kv();
  check_all_daemon_health();
  return 0;
}

