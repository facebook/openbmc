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
#include <sys/file.h>
#include <sys/types.h>

#define EMMC_LIFE_TIME "/sys/class/mmc_host/mmc0/mmc0:0001/life_time"
#define LIFE_TIME_INTERVAL 3600

// Thread to monitor eMMC life time
static void* emmc_health_monitor()
{
  FILE *fp;
  unsigned int life_type_A, life_type_B;

  fp = fopen(EMMC_LIFE_TIME, "r");
  if (fp == NULL) {
    syslog(LOG_CRIT, "Open %s failed", EMMC_LIFE_TIME);
    return NULL;
  }

  while (1) {
    if (fscanf(fp, "%x %x", &life_type_A, &life_type_B) != 2) {
      syslog(LOG_WARNING, "Read %s failed", EMMC_LIFE_TIME);
    } else {
      
      if (life_type_A >= 0x0A || life_type_B >= 0x0A)
        syslog(LOG_CRIT, "eMMC life 90%% used");
      else if (life_type_A >= 0x07 || life_type_B >= 0x07)
        syslog(LOG_WARNING, "eMMC life 70%% used");

#ifdef DEBUG
      syslog(LOG_INFO, "eMMC life_A %d%% & life_B %d%% used", life_type_A*10, life_type_B*10);
#endif
    }
    sleep(LIFE_TIME_INTERVAL);
  }

  fclose(fp);
  return NULL;
}

int main (int argc, char * const argv[])
{
  pthread_t tid_emmc;
  int rc;
  int pid_file;

  pid_file = open("/var/run/emmcd.pid", O_CREAT | O_RDWR, 0666);
  rc = flock(pid_file, LOCK_EX | LOCK_NB);
  if(rc) {
    if(EWOULDBLOCK == errno) {
      syslog(LOG_ERR, "Another emmcd instance is running...\n");
      exit(-1);
    }
  } else {
    openlog("emmcd", LOG_CONS, LOG_DAEMON);
    syslog(LOG_INFO, "emmcd: daemon started");
  }

  if (pthread_create(&tid_emmc, NULL, emmc_health_monitor, NULL) < 0) {
    syslog(LOG_WARNING, "pthread_create for emmc health error\n");
    exit(1);
  }

  pthread_join(tid_emmc, NULL);
  return 0;
}
