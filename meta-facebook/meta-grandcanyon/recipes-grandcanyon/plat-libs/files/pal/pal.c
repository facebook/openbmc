/*
 *
 * Copyright 2015-present Facebook. All Rights Reserved.
 *
 * This file contains code to support IPMI2.0 Specification available @
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
#include <unistd.h>
#include <stdint.h>
#include <stdbool.h>
#include <fcntl.h>
#include <errno.h>
#include <syslog.h>
#include <sys/mman.h>
#include <string.h>
#include <pthread.h>
#include "pal.h"


const char pal_fru_list[] = "all, server, bmc, uic, dpb, scc, nic, iocm";
const char pal_fru_list_print[] = "all, server, bmc, uic, dpb, scc, nic, iocm"; 
const char pal_fru_list_rw[] = "server, bmc, uic, nic, iocm";
const char pal_server_list[] = "server";
const char *fru_str_list[] = {"all", "server", "bmc", "uic", "dpb", "scc", "nic", "iocm"};


int
pal_get_fru_id(char *str, uint8_t *fru) {
  int fru_id = -1;
  bool found_id = false;

  for (fru_id = FRU_ALL; fru_id <= MAX_NUM_FRUS; fru_id++) {
    if ( strncmp(str, fru_str_list[fru_id], MAX_FRU_CMD_STR) == 0 ) {
      *fru = fru_id;
      found_id = true;
      break;
    }
  }

  return found_id ? 0 : -1;
}

int
pal_is_fru_ready(uint8_t fru, uint8_t *status) {
  switch (fru) {
    case FRU_SERVER:
    // TODO: Get server power status and BIC ready pin
    case FRU_BMC:
    case FRU_UIC:
    case FRU_DPB:
    case FRU_SCC:
    case FRU_NIC:
    case FRU_E1S_IOCM:
      *status = 1;
      break;
    default:
      *status = 0;
      syslog(LOG_WARNING, "%s() wrong fru id 0x%02x", __func__, fru);
      return PAL_ENOTSUP;
  }

  return 0;
}

int
pal_is_fru_prsnt(uint8_t fru, uint8_t *status) {
  // TODO: implement SERVER, SCC, NIC, E1S/IOCM present determination
  switch (fru) {
    case FRU_SERVER:
    case FRU_BMC:
    case FRU_UIC:
    case FRU_DPB:
    case FRU_SCC:
    case FRU_NIC:
    case FRU_E1S_IOCM:
      *status = 1;
      break;
    default:
      *status = 0;
      syslog(LOG_WARNING, "%s() wrong fru id 0x%02x", __func__, fru);
      return PAL_ENOTSUP;
  }

  return 0;
}

int
pal_get_fruid_name(uint8_t fru, char *name) {
  return fbgc_get_fruid_name(fru, name);
}

int
pal_get_fruid_path(uint8_t fru, char *path) {
  return fbgc_get_fruid_path(fru, path);
}

int
pal_get_fruid_eeprom_path(uint8_t fru, char *path) {
  return fbgc_get_fruid_eeprom_path(fru, path);
}
