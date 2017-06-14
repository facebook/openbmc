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
#include "fbttn_common.h"

#define CRASHDUMP_BIN       "/usr/local/bin/me-util"
#define CRASHDUMP_FILE "/mnt/data/crashdump_slot1"

struct threadinfo {
  uint8_t is_running;
  pthread_t pt;
};

static struct threadinfo t_dump[MAX_NUM_FRUS] = {0, };

int
fbttn_common_fru_name(uint8_t fru, char *str) {

  switch(fru) {
    case FRU_SLOT1:
      sprintf(str, "server");
      break;

    case FRU_IOM:
      sprintf(str, "iom");
      break;

    case FRU_DPB:
      sprintf(str, "dpb");
      break;

    case FRU_SCC:
      sprintf(str, "scc");
      break;

    case FRU_NIC:
      sprintf(str, "nic");
      break;

    default:
#ifdef DEBUG
      syslog(LOG_WARNING, "fbttn_common_fru_id: Wrong fru id");
#endif
    return -1;
  }

  return 0;
}

int
fbttn_common_fru_id(char *str, uint8_t *fru) {
  if (!strcmp(str, "all")) {
    *fru = FRU_ALL;
  } else if (!strcmp(str, "server")) {
    *fru = FRU_SLOT1;
  } else if (!strcmp(str, "iom")) {
    *fru = FRU_IOM;
  } else if (!strcmp(str, "dpb")) {
    *fru = FRU_DPB;
  } else if (!strcmp(str, "scc")) {
    *fru = FRU_SCC;
  } else if (!strcmp(str, "nic")) {
    *fru = FRU_NIC;
  } else {
#ifdef DEBUG
    syslog(LOG_WARNING, "fbttn_common_fru_id: Wrong fru id");
#endif
    return -1;
  }

  return 0;
}

void *
generate_dump(void *arg) {

  uint8_t fru = (uint8_t*) arg;
  char cmd[128];
  char fruname[16];
  int tmpf;
  int rc;

  // Usually the pthread cancel state are enable by default but
  // here we explicitly would like to enable them
  rc = pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
  rc = pthread_setcanceltype(PTHREAD_CANCEL_ENABLE, NULL);

  fbttn_common_fru_name(fru, fruname);

  // HEADER LINE for the dump
  memset(cmd, 0, 128);
  sprintf(cmd, "date > %s", CRASHDUMP_FILE);
  system(cmd);

  // COREID dump
  memset(cmd, 0, 128);
  sprintf(cmd, "%s %s 48 coreid", CRASHDUMP_BIN, fruname);
  system(cmd);

  // MSR dump
  memset(cmd, 0, 128);
  sprintf(cmd, "%s %s 48 msr", CRASHDUMP_BIN, fruname);
  system(cmd);

  syslog(LOG_CRIT, "Crashdump for FRU: %d is generated.", fru);

  t_dump[fru-1].is_running = 0;
}


int
fbttn_common_crashdump(uint8_t fru) {

  int ret;

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
      syslog(LOG_INFO, "fbttn_common_crashdump: No Crashdump pthread exists");
#ifdef DEBUG
    } else {
      syslog(LOG_INFO, "fbttn_common_crashdump: Previous crashdump thread is cancelled");
#endif
    }
  }

  // Start a thread to generate the crashdump
  if (pthread_create(&(t_dump[fru-1].pt), NULL, generate_dump, (void*) fru) < 0) {
    syslog(LOG_WARNING, "pal_store_crashdump: pthread_create for"
        " FRU %d failed\n", fru);
    return -1;
  }

  t_dump[fru-1].is_running = 1;

  syslog(LOG_INFO, "Crashdump for FRU: %d is being generated.", fru);

  return 0;
}
