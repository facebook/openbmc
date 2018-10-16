/*
 *
 * Copyright 2015-present Facebook. All Rights Reserved.
 *
 * This file contains code to support IPMI2.0 Specificaton available @
 * http://www.intel.com/content/www/us/en/servers/ipmi/ipmi-specifications.html
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
#include <stdint.h>
#include <stdbool.h>
#include <math.h>
#include <fcntl.h>
#include <errno.h>
#include <syslog.h>
#include <pthread.h>
#include <string.h>
#include <unistd.h>
#include <openbmc/kv.h>
#include "yosemite_common.h"

#define CRASHDUMP_BIN       "/usr/local/bin/dump.sh"
#define CRASHDUMP_FILE      "/mnt/data/crashdump_"

struct threadinfo {
  uint8_t is_running;
  uint8_t fru;
  pthread_t pt;
};

static struct threadinfo t_dump[MAX_NUM_FRUS] = {0, };

int
yosemite_common_fru_name(uint8_t fru, char *str) {

  switch(fru) {
    case FRU_SLOT1:
      sprintf(str, "slot1");
      break;

    case FRU_SLOT2:
      sprintf(str, "slot2");
      break;

    case FRU_SLOT3:
      sprintf(str, "slot3");
      break;

    case FRU_SLOT4:
      sprintf(str, "slot4");
      break;

    case FRU_SPB:
      sprintf(str, "spb");
      break;

    case FRU_NIC:
      sprintf(str, "nic");
      break;

    default:
#ifdef DEBUG
      syslog(LOG_WARNING, "yosemite_common_fru_id: Wrong fru id");
#endif
    return -1;
  }

  return 0;
}

int
yosemite_common_fru_id(char *str, uint8_t *fru) {

  if (!strcmp(str, "all")) {
    *fru = FRU_ALL;
  } else if (!strcmp(str, "slot1")) {
    *fru = FRU_SLOT1;
  } else if (!strcmp(str, "slot2")) {
    *fru = FRU_SLOT2;
  } else if (!strcmp(str, "slot3")) {
    *fru = FRU_SLOT3;
  } else if (!strcmp(str, "slot4")) {
    *fru = FRU_SLOT4;
  } else if (!strcmp(str, "spb")) {
    *fru = FRU_SPB;
  } else if (!strcmp(str, "nic")) {
    *fru = FRU_NIC;
  } else if (!strcmp(str, "bmc")) {
    *fru = FRU_BMC;
  } else {
#ifdef DEBUG
    syslog(LOG_WARNING, "yosemite_common_fru_id: Wrong fru id");
#endif
    return -1;
  }

  return 0;
}

void *
generate_dump(void *arg) {

  uint8_t fru = *(uint8_t *) arg;
  char cmd[128];
  char fruname[16];

  // Usually the pthread cancel state are enable by default but
  // here we explicitly would like to enable them
  (void) pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
  (void) pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);

  yosemite_common_fru_name(fru, fruname);

  // HEADER LINE for the dump
  memset(cmd, 0, 128);
  sprintf(cmd, "%s time > %s%s", CRASHDUMP_BIN, CRASHDUMP_FILE, fruname);
  system(cmd);

  // COREID dump
  memset(cmd, 0, 128);
  sprintf(cmd, "%s %s coreid >> %s%s", CRASHDUMP_BIN, fruname, CRASHDUMP_FILE, fruname);
  system(cmd);

  // MSR dump
  memset(cmd, 0, 128);
  sprintf(cmd, "%s %s msr >> %s%s", CRASHDUMP_BIN, fruname, CRASHDUMP_FILE, fruname);
  system(cmd);

  syslog(LOG_CRIT, "Crashdump for FRU: %d is generated.", fru);

  t_dump[fru-1].is_running = 0;

  sprintf(cmd, CRASHDUMP_KEY, fru);
  kv_set(cmd, "0", 0, 0);

  return NULL;
}


int
yosemite_common_crashdump(uint8_t fru) {

  int ret;
  char cmd[100];

  // Check if the crashdump script exist
  if (access(CRASHDUMP_BIN, F_OK) == -1) {
    syslog(LOG_CRIT, "Crashdump for FRU: %d failed : "
        "crashdump binary is not preset", fru);
    return 0;
  }

  // Check if a crashdump for that fru is already running.
  // If yes, kill that thread and start a new one.
  if (t_dump[fru-1].is_running) {
    ret = pthread_cancel(t_dump[fru-1].pt);
    if (ret == ESRCH) {
      syslog(LOG_INFO, "yosemite_common_crashdump: No Crashdump pthread exists");
    } else {
      pthread_join(t_dump[fru-1].pt, NULL);
      sprintf(cmd, "ps | grep '{dump.sh}' | grep 'slot%d' | awk '{print $1}'| xargs kill", fru);
      system(cmd);
      sprintf(cmd, "ps | grep 'me-util' | grep 'slot%d' | awk '{print $1}'| xargs kill", fru);
      system(cmd);
#ifdef DEBUG
      syslog(LOG_INFO, "yosemite_common_crashdump: Previous crashdump thread is cancelled");
#endif
    }
  }

  // Start a thread to generate the crashdump
  t_dump[fru-1].fru = fru;
  if (pthread_create(&(t_dump[fru-1].pt), NULL, generate_dump, (void*) &t_dump[fru-1].fru) < 0) {
    syslog(LOG_WARNING, "pal_store_crashdump: pthread_create for"
        " FRU %d failed\n", fru);
    return -1;
  }

  t_dump[fru-1].is_running = 1;

  syslog(LOG_INFO, "Crashdump for FRU: %d is being generated.", fru);

  return 0;
}
