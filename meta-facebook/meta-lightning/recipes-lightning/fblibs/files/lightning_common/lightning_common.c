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
#include "lightning_common.h"

#define CRASHDUMP_BIN       "/usr/local/bin/dump.sh"
#define CRASHDUMP_FILE      "/mnt/data/crashdump_"

int
lightning_common_fru_name(uint8_t fru, char *str) {

  switch(fru) {
    case FRU_PEB:
      sprintf(str, "peb");
      break;

    case FRU_PDPB:
      sprintf(str, "pdpb");
      break;

    case FRU_FCB:
      sprintf(str, "fcb");
      break;

    default:
#ifdef DEBUG
      syslog(LOG_WARNING, "lightning_common_fru_id: Wrong fru id");
#endif
    return -1;
  }

  return 0;
}

int
lightning_common_fru_id(char *str, uint8_t *fru) {

  if (!strcmp(str, "all")) {
    *fru = FRU_ALL;
  } else if (!strcmp(str, "peb")) {
    *fru = FRU_PEB;
  } else if (!strcmp(str, "pdpb")) {
    *fru = FRU_PDPB;
  } else if (!strcmp(str, "fcb")) {
    *fru = FRU_FCB;
  } else {
#ifdef DEBUG
    syslog(LOG_WARNING, "lightning_common_fru_id: Wrong fru id");
#endif
    return -1;
  }

  return 0;
}
