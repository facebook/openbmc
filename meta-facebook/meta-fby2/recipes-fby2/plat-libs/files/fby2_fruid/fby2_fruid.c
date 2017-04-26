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
#include "fby2_fruid.h"

enum {
  IPMB_BUS_SLOT1 = 1,
  IPMB_BUS_SLOT2 = 3,
  IPMB_BUS_SLOT3 = 5,
  IPMB_BUS_SLOT4 = 7,
};

// Common IPMB Wrapper function

static int
plat_get_ipmb_bus_id(uint8_t slot_id) {
  int bus_id;

  switch(slot_id) {
  case FRU_SLOT1:
    bus_id = IPMB_BUS_SLOT1;
    break;
  case FRU_SLOT2:
    bus_id = IPMB_BUS_SLOT2;
    break;
  case FRU_SLOT3:
    bus_id = IPMB_BUS_SLOT3;
    break;
  case FRU_SLOT4:
    bus_id = IPMB_BUS_SLOT4;
    break;
  default:
    bus_id = -1;
    break;
  }

  return bus_id;
}

/* Populate char path[] with the path to the fru's fruid binary dump */
int
fby2_get_fruid_path(uint8_t fru, char *path) {
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
    case FRU_SPB:
      sprintf(fname, "spb");
      break;
    case FRU_NIC:
      sprintf(fname, "nic");
      break;
    default:
#ifdef DEBUG
      syslog(LOG_WARNING, "fby2_get_fruid_path: wrong fruid");
#endif
      return -1;
  }

  sprintf(path, FBY2_FRU_PATH, fname);
  return 0;
}

int
fby2_get_fruid_eeprom_path(uint8_t fru, char *path) {
  char fname[16] = {0};

  switch(fru) {
    case FRU_SLOT1:
    case FRU_SLOT2:
    case FRU_SLOT3:
    case FRU_SLOT4:
      switch(fby2_get_slot_type(fru))
      {
        case SLOT_TYPE_SERVER:
          return -1;
          break;
        case SLOT_TYPE_CF:
        case SLOT_TYPE_GP:
          sprintf(path, "/sys/class/i2c-adapter/i2c-%d/%d-0051/eeprom",plat_get_ipmb_bus_id(fru),plat_get_ipmb_bus_id(fru));
          break;
      }
      break;
    case FRU_SPB:
      sprintf(path, "/sys/class/i2c-adapter/i2c-8/8-0051/eeprom");
      break;
    case FRU_NIC:
      sprintf(path, "/sys/class/i2c-adapter/i2c-12/12-0051/eeprom");
      break;
    default:
#ifdef DEBUG
      syslog(LOG_WARNING, "fby2_get_fruid_eeprom_path: wrong fruid");
#endif
      return -1;
  }

  return 0;
}

/* Populate char name[] with the path to the fru's name */
int
fby2_get_fruid_name(uint8_t fru, char *name) {

  switch(fru) {
    case FRU_SLOT1:
    case FRU_SLOT2:
    case FRU_SLOT3:
    case FRU_SLOT4:
      switch(fby2_get_slot_type(fru))
      {
        case SLOT_TYPE_SERVER:
          sprintf(name, "Server Board %d",fru);
          break;
        case SLOT_TYPE_CF:
          sprintf(name, "Crane Flat %d",fru);
          break;
        case SLOT_TYPE_GP:
          sprintf(name, "Glacier Point %d",fru);
          break;
      }
      break;
    case FRU_SPB:
      sprintf(name, "Side Plane Board");
      break;
    case FRU_NIC:
      sprintf(name, "CX4 NIC");
      break;
    default:
#ifdef DEBUG
      syslog(LOG_WARNING, "fby2_get_fruid_name: wrong fruid");
#endif
      return -1;
  }
  return 0;
}
