/* Copyright 2015-present Facebook. All Rights Reserved.
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
#include <syslog.h>
#include <facebook/fby3_common.h>
#include "fby3_fruid.h"

/* Populate char path[] with the path to the fru's fruid binary dump */
int
fby3_get_fruid_path(uint8_t fru, uint8_t dev_id, char *path) {
  char fname[16] = {0};

  switch(fru) {
    case FRU_SLOT1:
      sprintf(fname, "slot1");
      break;
    case FRU_SLOT2:
      sprintf(fname, "slot2");
      break;
    case FRU_SLOT3:
      sprintf(fname, "slot3");
      break;
    case FRU_SLOT4:
      sprintf(fname, "slot4");
      break;
    case FRU_BMC:
      sprintf(fname, "bmc");
      break;
    case FRU_NIC:
      sprintf(fname, "nic");
      break;
    case FRU_BB:
      sprintf(fname, "bb");
      break;
    default:
      syslog(LOG_WARNING, "%s() unknown fruid %d", __func__, fru);
      return -1;
    }

  sprintf(path, "/tmp/fruid_%s.bin", fname);

  if ( dev_id == DEV_NONE ) {
    sprintf(path, FBY3_FRU_PATH, fname);
    return 0;
  }

  if ( fru < FRU_SLOT1 && fru > FRU_SLOT4 )
    return -1;

  if ( dev_id < 1 || dev_id > 12)
    return -1;

  sprintf(path, FBY3_FRU_DEV_PATH, fname, dev_id);
  return 0;
}

/* Populate char name[] with the path to the fru's name */
int
fby3_get_fruid_name(uint8_t fru, char *name) {

  switch(fru) {
    case FRU_SLOT1:
      sprintf(name, "Server board 1");
      break;
    case FRU_SLOT2:
      sprintf(name, "Server board 2");
      break;
    case FRU_SLOT3:
      sprintf(name, "Server board 3");
      break;
    case FRU_SLOT4:
      sprintf(name, "Server board 4");
      break;
    case FRU_BMC:
      sprintf(name, "BMC");
      break;
    case FRU_BB:
      sprintf(name, "Baseboard");
      break;
    case FRU_NICEXP:
      sprintf(name, "NIC Expansion");
      break;
    case FRU_NIC:
      sprintf(name, "NIC");
      break;
    default:
      syslog(LOG_WARNING, "%s() wrong fru %d", __func__, fru);
      return -1;
  }
  return 0;
}
