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
#include <openbmc/kv.h>
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

int
lightning_pcie_switch(uint8_t fru, uint8_t *pcie_sw) {

  char value[MAX_VALUE_LEN] = {0};

  if (fru != FRU_PEB) {
    syslog(LOG_INFO, "No PCIe switch on this fru: %d", fru);
    return -1;
  }

  if (kv_get("pcie_switch_vendor", value, NULL, 0)) {
    return -1;
  }

  if (strstr(value, "PMC") != NULL)
    *pcie_sw = PCIE_SW_PMC;
  else if (strstr(value, "PLX") != NULL)
    *pcie_sw = PCIE_SW_PLX;
  else
    return -1;

  return 0;
}

int 
lightning_ssd_sku(uint8_t *ssd_sku) {

  char sku[MAX_VALUE_LEN] = {0};
  int rc;

  rc = kv_get("ssd_sku_info", sku, NULL, 0);
  if (rc < 0) {
    syslog(LOG_DEBUG, "%s(): kv_get for ssd_sku_info failed", __func__);
    return -1;
  }

  if (!strcmp(sku, "U2"))
    *ssd_sku = U2_SKU;
  else if (!strcmp(sku, "M2"))
    *ssd_sku = M2_SKU;
  else {
    syslog(LOG_WARNING, "%s(): Cannot find corresponding SSD SKU for %s", __func__, sku);
    return -1;
  }

  return 0;
}

int
lightning_ssd_vendor(uint8_t *ssd_vendor) {
  char vendor[MAX_VALUE_LEN] = {0};

  if (kv_get("ssd_vendor", vendor, NULL, 0)) {
    syslog(LOG_DEBUG, "%s(): ssd_vendor key_get failed failed", __func__);
    return -1;
  }

  if (strstr(vendor, "intel") != NULL)
    *ssd_vendor = INTEL;
  else if (strstr(vendor, "seagate") != NULL)
    *ssd_vendor = SEAGATE;
  else if (strstr(vendor, "samsung") != NULL)
    *ssd_vendor = SAMSUNG;
  else {
    syslog(LOG_DEBUG, "%s(): Cannot find corresponding SSD vendor", __func__);
    return -1;
  }

  return 0;
}

