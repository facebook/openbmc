/*
 *
 * Copyright 2020-present Facebook. All Rights Reserved.
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
#include <stdbool.h>
#include <unistd.h>
#include <math.h>
#include <fcntl.h>
#include <errno.h>
#include <syslog.h>
#include <pthread.h>
#include <string.h>
#include <time.h>
#include <openbmc/libgpio.h>
#include <openbmc/obmc-i2c.h>
#include "fbwc_common.h"
#include <openbmc/kv.h>

char*
get_component_name(uint8_t comp) {
  switch (comp) {
    case FW_MB_CPLD:
      return "MB CPLD";
    case FW_MB_BIC:
      return "MB BIC";
    case FW_VR_VCCIN:
      return "VCCIN/VCCFA_EHV_FIVRA";
    case FW_VR_VCCD:
      return "VCCD";
    case FW_VR_VCCINFAON:
      return "VCCINFAON/VCCFA_EHV";
    case FW_BIOS:
      return "BIOS";

    default:
      return "Unknown";
  }

  return "NULL";
}
