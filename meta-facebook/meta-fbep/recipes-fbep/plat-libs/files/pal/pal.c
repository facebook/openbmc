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
#include <stdint.h>
#include <unistd.h>
#include <syslog.h>
#include <string.h>
#include "pal.h"

const char pal_fru_list[] = "all, fru";
const char pal_server_list[] = "";

int pal_channel_to_bus(int channel)
{
  switch (channel) {
    case 0:
      return 14; // USB (LCD Debug Board)
    case 1:
      return 1; // MB#1
    case 2:
      return 2; // MB#2
    case 3:
      return 3; // MB#3
    case 4:
      return 4; // MB#4
  }

  // Debug purpose, map to real bus number
  if (channel & 0x80) {
    return (channel & 0x7f);
  }

  return channel;
}

int pal_get_fruid_path(uint8_t fru, char *path)
{

  sprintf(path, FRU_BIN);
  return 0;
}

int pal_get_fruid_eeprom_path(uint8_t fru, char *path)
{

  sprintf(path, FRU_EEPROM);
  return 0;
}

int pal_get_fru_id(char *str, uint8_t *fru)
{
  if (!strcmp(str, "all")) {
    *fru = FRU_ALL;
  } else if (!strcmp(str, "fru")) {
    *fru = FRU_BASE;
  } else {
    syslog(LOG_WARNING, "%s: Wrong fru name %s", __func__, str);
    return -1;
  }

  return 0;
}

int pal_get_fruid_name(uint8_t fru, char *name)
{

  sprintf(name, "Base Board");
  return 0;
}

int pal_is_fru_ready(uint8_t fru, uint8_t *status)
{

  *status = 1;
  return 0;
}

int pal_get_fru_name(uint8_t fru, char *name)
{
  if (fru != FRU_BASE) {
    syslog(LOG_WARNING, "%s: Wrong fruid %d", __func__, fru);
    return -1;
  }

  strcpy(name, "fru");

  return 0;
}
