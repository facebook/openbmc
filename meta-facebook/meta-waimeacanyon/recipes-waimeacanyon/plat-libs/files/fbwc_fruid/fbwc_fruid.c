/* Copyright 2020-present Facebook. All Rights Reserved.
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
#include <syslog.h>

#include <openbmc/fruid.h>
#include <facebook/fbwc_common.h>
#include <facebook/bic.h>
#include <facebook/expander.h>
#include "fbwc_fruid.h"

int
fbwc_get_fruid_name(uint8_t fru, char *name) {
  switch(fru) {
    case FRU_IOM:
      snprintf(name, MAX_FRU_NAME_STR, "IOM");
      break;
    case FRU_MB:
      snprintf(name, MAX_FRU_NAME_STR, "MB");
      break;
    case FRU_SCB:
      snprintf(name, MAX_FRU_NAME_STR, "SCB");
      break;
    case FRU_BMC:
      snprintf(name, MAX_FRU_NAME_STR, "BMC");
      break;
    case FRU_SCM:
      snprintf(name, MAX_FRU_NAME_STR, "SCM");
      break;
    case FRU_BSM:
      snprintf(name, MAX_FRU_NAME_STR, "BSM");
      break;
    case FRU_HDBP:
      snprintf(name, MAX_FRU_NAME_STR, "HDBP");
      break;
    case FRU_PDB:
      snprintf(name, MAX_FRU_NAME_STR, "PDB");
      break;
    case FRU_NIC:
      snprintf(name, MAX_FRU_NAME_STR, "NIC");
      break;
    case FRU_FAN0:
      snprintf(name, MAX_FRU_NAME_STR, "FAN0");
      break;
    case FRU_FAN1:
      snprintf(name, MAX_FRU_NAME_STR, "FAN1");
      break;
    case FRU_FAN2:
      snprintf(name, MAX_FRU_NAME_STR, "FAN2");
      break;
    case FRU_FAN3:
      snprintf(name, MAX_FRU_NAME_STR, "FAN3");
      break;
    default:
      syslog(LOG_WARNING, "%s: wrong fruid", __func__);
      return -1;
  }

  return 0;
}

int
fbwc_get_fruid_path(uint8_t fru, char *path) {
  char fname[8] = {0};

  switch(fru) {
    case FRU_IOM:
      snprintf(fname, sizeof(fname), "iom");
      break;
    case FRU_MB:
      snprintf(fname, sizeof(fname), "mb");
      break;
    case FRU_SCB:
      snprintf(fname, sizeof(fname), "scb");
      break;
    case FRU_BMC:
      snprintf(fname, sizeof(fname), "bmc");
      break;
    case FRU_SCM:
      snprintf(fname, sizeof(fname), "scm");
      break;
    case FRU_BSM:
      snprintf(fname, sizeof(fname), "bsm");
      break;
    case FRU_HDBP:
      snprintf(fname, sizeof(fname), "hdbp");
      break;
    case FRU_PDB:
      snprintf(fname, sizeof(fname), "pdb");
      break;
    case FRU_NIC:
      snprintf(fname, sizeof(fname), "nic");
      break;
    case FRU_FAN0:
      snprintf(fname, sizeof(fname), "fan0");
      break;
    case FRU_FAN1:
      snprintf(fname, sizeof(fname), "fan1");
      break;
    case FRU_FAN2:
      snprintf(fname, sizeof(fname), "fan2");
      break;
    case FRU_FAN3:
      snprintf(fname, sizeof(fname), "fan3");
      break;
    default:
      syslog(LOG_WARNING, "%s: wrong fruid", __func__);
      return -1;
  }

  snprintf(path, MAX_PATH_LEN, COMMON_FRU_PATH, fname);
  return 0;
}

int
fbwc_get_fruid_tmp_path(uint8_t fru, char *path) {
  char fname[8] = {0};

  switch(fru) {
    case FRU_IOM:
      snprintf(fname, sizeof(fname), "iom");
      break;
    case FRU_MB:
      snprintf(fname, sizeof(fname), "mb");
      break;
    case FRU_SCB:
      snprintf(fname, sizeof(fname), "scb");
      break;
    case FRU_BMC:
      snprintf(fname, sizeof(fname), "bmc");
      break;
    case FRU_SCM:
      snprintf(fname, sizeof(fname), "scm");
      break;
    case FRU_BSM:
      snprintf(fname, sizeof(fname), "bsm");
      break;
    case FRU_HDBP:
      snprintf(fname, sizeof(fname), "hdbp");
      break;
    case FRU_PDB:
      snprintf(fname, sizeof(fname), "pdb");
      break;
    case FRU_NIC:
      snprintf(fname, sizeof(fname), "nic");
      break;
    case FRU_FAN0:
      snprintf(fname, sizeof(fname), "fan0");
      break;
    case FRU_FAN1:
      snprintf(fname, sizeof(fname), "fan1");
      break;
    case FRU_FAN2:
      snprintf(fname, sizeof(fname), "fan2");
      break;
    case FRU_FAN3:
      snprintf(fname, sizeof(fname), "fan3");
      break;
    default:
      syslog(LOG_WARNING, "%s: wrong fruid", __func__);
      return -1;
  }

  snprintf(path, MAX_PATH_LEN, COMMON_TMP_FRU_PATH, fname);
  return 0;
}

int
fbwc_get_fruid_eeprom_path(uint8_t fru, char *path) {
  switch(fru) {
    case FRU_IOM:
    case FRU_MB:
    case FRU_SCB:
    case FRU_HDBP:
    case FRU_PDB:
    case FRU_FAN0:
    case FRU_FAN1:
    case FRU_FAN2:
    case FRU_FAN3:
    case FRU_BMC:
      return -1;
    case FRU_SCM:
      snprintf(path, MAX_PATH_LEN, EEPROM_PATH, I2C_BUS_SCM, SCM_FRU_ADDR);
      break;
    case FRU_BSM:
      snprintf(path, MAX_PATH_LEN, EEPROM_PATH, I2C_BUS_SCM, BSM_FRU_ADDR);
      break;
    case FRU_NIC:
      snprintf(path, MAX_PATH_LEN, EEPROM_PATH, I2C_BUS_NIC, NIC_FRU_ADDR);
      break;
    default:
      syslog(LOG_WARNING, "%s: wrong fruid", __func__);
      return -1;
  }

  return 0;
}

int
fbwc_fruid_write(uint8_t fru, char *path) {
  switch (fru) {
    case FRU_IOM:
      return bic_write_fruid(BIC_FRU_ID_IOM, path);
    case FRU_MB:
      return bic_write_fruid(BIC_FRU_ID_MB, path);
    case FRU_SCB:
      return exp_write_fruid(EXP_FRU_ID_SCB, path);
    case FRU_HDBP:
      return exp_write_fruid(EXP_FRU_ID_HDBP, path);
    case FRU_PDB:
      return exp_write_fruid(EXP_FRU_ID_PDB, path);
    case FRU_FAN0:
      return exp_write_fruid(EXP_FRU_ID_FAN0, path);
    case FRU_FAN1:
      return exp_write_fruid(EXP_FRU_ID_FAN1, path);
    case FRU_FAN2:
      return exp_write_fruid(EXP_FRU_ID_FAN2, path);
    case FRU_FAN3:
      return exp_write_fruid(EXP_FRU_ID_FAN3, path);
    default:
      return -1;
  }

  return 0;
}
