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
#include <syslog.h>
#include <stdio.h>
#include <string.h>
#include <openbmc/libgpio.h>
#include "pal.h"

#define MB_BIN "/tmp/fruid_mb.bin"
#define MB_EEPROM "/sys/class/i2c-dev/i2c-6/device/6-0054/eeprom"
#define PDB_BIN "/tmp/fruid_pdb.bin"
#define PDB_EEPROM "/sys/class/i2c-dev/i2c-5/device/5-0054/eeprom"
#define BSM_BIN "/tmp/fruid_bsm.bin"
#define BSM_EEPROM "/sys/class/i2c-dev/i2c-23/device/23-0056/eeprom"
#define AVA1_BIN "/tmp/fruid_ava1.bin"
#define AVA1_EEPROM "/sys/class/i2c-dev/i2c-21/device/21-0050/eeprom"
#define AVA2_BIN "/tmp/fruid_ava2.bin"
#define AVA2_EEPROM "/sys/class/i2c-dev/i2c-22/device/22-0050/eeprom"

#define BMC_IPMB_SLAVE_ADDR 0x17

const char pal_fru_list[] = "all, mb, bsm, pdb, ava1, ava2";
const char pal_server_list[] = "";

int pal_get_fru_id(char *str, uint8_t *fru)
{
  if (!strcmp(str, "all")) {
    *fru = FRU_ALL;
  } else if (!strcmp(str, "mb") || !strcmp(str, "vr") || !strcmp(str, "bmc")) {
    *fru = FRU_MB;
  } else if (!strcmp(str, "bsm")) {
    *fru = FRU_BSM;
  } else if (!strcmp(str, "ava1")) {
    *fru = FRU_AVA1;
  } else if (!strcmp(str, "ava2")) {
    *fru = FRU_AVA2;
  } else {
    syslog(LOG_WARNING, "%s: Wrong fru name %s", __func__, str);
    return -1;
  }

  return 0;
}

int pal_get_fruid_name(uint8_t fru, char *name)
{
  if (fru == FRU_MB)
    sprintf(name, "Base Board");
  else if (fru == FRU_BSM)
    sprintf(name, "BSM");
  else if (fru == FRU_AVA1)
    sprintf(name, "M2 Carrier1");
  else if (fru == FRU_AVA2)
    sprintf(name, "M2 Carrier2");
  else if (fru == FRU_PDB)
    sprintf(name, "PDB");
  else
    return -1;

  return 0;
}

int pal_is_fru_prsnt(uint8_t fru, uint8_t *status)
{
  if (fru == FRU_MB || fru == FRU_PDB || fru == FRU_BSM || FRU_AVA1 || FRU_AVA2)
    *status = 1;
  else
    return -1;

  return 0;
}

int pal_is_fru_ready(uint8_t fru, uint8_t *status)
{
  if (fru == FRU_MB || fru == FRU_PDB || fru == FRU_BSM || FRU_AVA1 || FRU_AVA2)
    *status = 1;
  else
    return -1;

  return 0;
}

int pal_get_fru_name(uint8_t fru, char *name)
{
  if (fru == FRU_MB) {
    strcpy(name, "mb");
  } else if (fru == FRU_BSM) {
    strcpy(name, "bsm");
  } else if (fru == FRU_PDB) {
    strcpy(name, "pdb");
  } else if (fru == FRU_AVA1) {
    strcpy(name, "ava1");
  } else if (fru == FRU_AVA2) {
    strcpy(name, "ava2");
  } else {
    syslog(LOG_WARNING, "%s: Wrong fruid %d", __func__, fru);
    return -1;
  }

  return 0;
}

int pal_get_fruid_path(uint8_t fru, char *path)
{
  if (fru == FRU_MB)
    sprintf(path, MB_BIN);
  else if (fru == FRU_PDB)
    sprintf(path, PDB_BIN);
  else if (fru == FRU_BSM)
    sprintf(path, BSM_BIN);
  else if (fru == FRU_AVA1)
    sprintf(path, AVA1_BIN);
  else if (fru == FRU_AVA2)
    sprintf(path, AVA2_BIN);
  else
    return -1;

  return 0;
}

void pal_get_eth_intf_name(char* intf_name)
{
  snprintf(intf_name, 8, "usb0");
}

int pal_get_fruid_eeprom_path(uint8_t fru, char *path)
{
  if (fru == FRU_MB)
    sprintf(path, MB_EEPROM);
  else if (fru == FRU_PDB)
    sprintf(path, PDB_EEPROM);
  else if (fru == FRU_BSM)
    sprintf(path, BSM_EEPROM);
  else if (fru == FRU_AVA1)
    sprintf(path, AVA1_EEPROM);
  else if (fru == FRU_AVA2)
    sprintf(path, AVA2_EEPROM);
  else
    return -1;

  return 0;
}

int pal_get_fru_list(char *list)
{
  strcpy(list, pal_fru_list);
  return 0;
}

int pal_get_bmc_ipmb_slave_addr(uint16_t *slave_addr, uint8_t bus_id)
{
  if (bus_id == 8) {
    // DBG Card used default slave addr
    *slave_addr = 0x10;
  } else {
    *slave_addr = BMC_IPMB_SLAVE_ADDR;
  }

  return 0;
}

int pal_set_usb_path(uint8_t slot, uint8_t endpoint)
{
  int ret = CC_SUCCESS;
  char gpio_name[16] = {0};
  gpio_desc_t *desc;
  gpio_value_t value;

  if (slot > SERVER_2 || endpoint > PCH) {
    ret = CC_PARAM_OUT_OF_RANGE;
    goto exit;
  }

  strcpy(gpio_name ,"USB2_SEL0");
  desc = gpio_open_by_shadow(gpio_name);
  if (!desc) {
    ret = CC_UNSPECIFIED_ERROR;
    goto exit;
  }
  value = endpoint == PCH? GPIO_VALUE_HIGH: GPIO_VALUE_LOW;
  if (gpio_set_value(desc, value)) {
    ret = CC_UNSPECIFIED_ERROR;
    goto bail;
  }
  gpio_close(desc);

  strcpy(gpio_name ,"USB2_SEL1");
  desc = gpio_open_by_shadow(gpio_name);
  if (!desc) {
    ret = CC_UNSPECIFIED_ERROR;
    goto exit;
  }
  value = slot%2 == 0? GPIO_VALUE_HIGH: GPIO_VALUE_LOW;
  if (gpio_set_value(desc, value)) {
    ret = CC_UNSPECIFIED_ERROR;
    goto bail;
  }

bail:
  gpio_close(desc);
exit:
  return ret;
}