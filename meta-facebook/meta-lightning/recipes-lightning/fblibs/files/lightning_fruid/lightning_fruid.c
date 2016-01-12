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
#include "lightning_fruid.h"

/* Populate char path[] with the path to the fru's fruid binary dump */
int
lightning_get_fruid_path(uint8_t fru, char *path) {
  char fname[16] = {0};

  switch(fru) {
    case FRU_PEB:
      sprintf(fname, "peb");
      break;
    case FRU_PDPB:
      sprintf(fname, "pdpb");
      break;
    case FRU_FCB:
      sprintf(fname, "fcb");
      break;
    default:
#ifdef DEBUG
      syslog(LOG_WARNING, "lightning_get_fruid_path: wrong fruid");
#endif
      return -1;
  }

  sprintf(path, LIGHTNING_FRU_PATH, fname);
  return 0;
}

int
lightning_get_fruid_eeprom_path(uint8_t fru, char *path) {
  char fname[16] = {0};

  switch(fru) {
    case FRU_PEB:
      sprintf(path, "/sys/class/i2c-adapter/i2c-4/4-0050/eeprom");
      break;
    case FRU_PDPB:
      sprintf(path, "/sys/class/i2c-adapter/i2c-6/6-0051/eeprom");
      break;
    case FRU_FCB:
      sprintf(path, "/sys/class/i2c-adapter/i2c-5/5-0051/eeprom");
      break;
    default:
#ifdef DEBUG
      syslog(LOG_WARNING, "lightning_get_fruid_eeprom_path: wrong fruid");
#endif
      return -1;
  }

  return 0;
}

/* Populate char name[] with the path to the fru's name */
int
lightning_get_fruid_name(uint8_t fru, char *name) {

  switch(fru) {
    case FRU_PEB:
      sprintf(name, "PCIe Expander Board");
      break;
    case FRU_PDPB:
      sprintf(name, "PCIe Drive Plane Board");
      break;
    case FRU_FCB:
      sprintf(name, "Fan Control Board");
      break;
    default:
#ifdef DEBUG
      syslog(LOG_WARNING, "lightning_get_fruid_name: wrong fruid");
#endif
      return -1;
  }
  return 0;
}
