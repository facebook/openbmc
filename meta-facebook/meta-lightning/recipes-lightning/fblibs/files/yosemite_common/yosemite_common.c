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
#include "yosemite_common.h"

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
  } else {
    syslog(LOG_ALERT, "yosemite_common_fru_id: Wrong fru id");
    return -1;
  }

  return 0;
}
