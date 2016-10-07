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

#define PCIE_SW_FILE        "/tmp/pcie_switch_vendor"
#define SSD_SKU_INFO        "/tmp/ssd_sku_info"
#define SSD_VENDOR_INFO     "/tmp/ssd_vendor"
#define PATH_LENGTH         16

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

  FILE *fp;
  int rc;
  char sw[8];

  if (fru != FRU_PEB) {
    syslog(LOG_INFO, "No PCIe switch on this fru: %d", fru);
    return -1;
  }

  fp = fopen(PCIE_SW_FILE, "r");
  if (!fp) {
#ifdef DEBUG
    syslog(LOG_WARNING, "lightning_pcie_switch: fopen failed for fru: %d", fru);
#endif
    return -1;
  }

  rc = (int) fread(sw, 1, 8, fp);
  fclose(fp);
  if (rc <= 0) {
#ifdef DEBUG
    syslog(LOG_WARNING, "lightning_pcie_switch: fread failed for fru: %d", fru);
#endif
    return -1;
  }

  if (strstr(sw, "PMC") != NULL)
    *pcie_sw = PCIE_SW_PMC;
  else if (strstr(sw, "PLX") != NULL)
    *pcie_sw = PCIE_SW_PLX;
  else
    return -1;

  return 0;
}

int 
lightning_ssd_sku(uint8_t *ssd_sku) {

  FILE *fp;
  char sku[PATH_LENGTH] = {0};
  int rc;

  fp = fopen(SSD_SKU_INFO, "r");
  if(!fp) {
    syslog(LOG_DEBUG, "%s(): %s fopen failed", __func__, SSD_SKU_INFO);
    return -1;
  }

  rc = (int) fread(sku, 1, sizeof(sku), fp);
  fclose(fp);

  if(rc <= 0) {
    syslog(LOG_DEBUG, "%s(): %s fread failed", __func__, SSD_SKU_INFO);
    return -1;
  }

  if (strstr(sku, "U2") != NULL)
    *ssd_sku = U2_SKU;
  else if (strstr(sku, "M2") != NULL)
    *ssd_sku = M2_SKU;
  else {
    syslog(LOG_DEBUG, "%s(): Cannot find corresponding SSD SKU", __func__);
    return -1;
  }

  return 0;
}

int
lightning_ssd_vendor(uint8_t *ssd_vendor) {

  FILE *fp;
  char vendor[PATH_LENGTH] = {0};
  int rc;

  fp = fopen(SSD_VENDOR_INFO, "r");
  if(!fp) {
    syslog(LOG_DEBUG, "%s(): %s fopen failed", __func__, SSD_VENDOR_INFO);
    return -1;
  }

  rc = (int) fread(vendor, 1, sizeof(vendor), fp);
  fclose(fp);

  if(rc <= 0) {
    syslog(LOG_DEBUG, "%s(): %s fread failed", __func__, SSD_VENDOR_INFO);
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

