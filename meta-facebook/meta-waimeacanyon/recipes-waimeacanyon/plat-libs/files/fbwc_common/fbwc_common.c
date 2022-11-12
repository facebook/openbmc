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

int
get_sys_sku(uint8_t *sys_sku) {
  gpio_value_t gpio_sku_id0, gpio_sku_id1;
  *sys_sku = SKU_UNKNOW;
  // 0 0 -> dual compute system
  // 1 1 -> single compute system

  gpio_sku_id0 = gpio_get_value_by_shadow("SYS_SKU_ID0_R");
  if (gpio_sku_id0 == GPIO_VALUE_INVALID) {
    syslog(LOG_WARNING, "%s: Fail to get gpio SYS_SKU_ID0_R", __FUNCTION__);
    return -1;
  }
  gpio_sku_id1 = gpio_get_value_by_shadow("SYS_SKU_ID1_R");
  if (gpio_sku_id1 == GPIO_VALUE_INVALID) {
    syslog(LOG_WARNING, "%s: Fail to get gpio SYS_SKU_ID1_R", __FUNCTION__);
    return -1;
  }

  if (gpio_sku_id0 == GPIO_VALUE_HIGH && gpio_sku_id1 == GPIO_VALUE_HIGH) {
    *sys_sku = SKU_SINGLE_COMPUTE;
  } else if (gpio_sku_id0 == GPIO_VALUE_LOW && gpio_sku_id1 == GPIO_VALUE_LOW) {
    *sys_sku = SKU_DUAL_COMPUTE;
  } else {
    syslog(LOG_WARNING, "%s: invalid system sku, SYS_SKU_ID0_R:%d SYS_SKU_ID1_R:%d "
    , __FUNCTION__, gpio_sku_id0, gpio_sku_id1);
    return -1;
  }

  return 0;
}

int
set_fw_update_ongoing(uint8_t fruid, uint16_t tmout) {
  char key[64] = {0};
  char value[64] = {0};
  struct timespec ts;

  sprintf(key, "fru%d_fwupd", fruid);

  clock_gettime(CLOCK_MONOTONIC, &ts);
  ts.tv_sec += tmout;
  sprintf(value, "%ld", ts.tv_sec);

  if (kv_set(key, value, 0, 0) < 0) {
     return -1;
  }

  return 0;
}

int
get_exp_bus_addr(uint8_t fru_id, uint8_t *bus, uint8_t *addr) {
  switch (fru_id) {
    case FRU_SCB:
      *bus = I2C_BUS_EXP;
      *addr = EXP_SLAVE_ADDR;
      break;
      
    case FRU_SCB_PEER:  // used in single system
      *bus = I2C_BUS_EXP;
      *addr = EXP_PEER_SLAVE_ADDR;
      break;
      
    case FRU_JBOD1_L_SCB:
      *bus = I2C_BUS_EXP_JBOD1_L;
      *addr = EXP_SLAVE_ADDR;
      break;
      
    case FRU_JBOD1_R_SCB:
      *bus = I2C_BUS_EXP_JBOD1_R;
      *addr = EXP_PEER_SLAVE_ADDR;
      break;
      
    case FRU_JBOD2_L_SCB:
      *bus = I2C_BUS_EXP_JBOD2_L;
      *addr = EXP_SLAVE_ADDR;
      break;
      
    case FRU_JBOD2_R_SCB:
      *bus = I2C_BUS_EXP_JBOD2_R;
      *addr = EXP_PEER_SLAVE_ADDR;
      break;
      
    default:
      syslog(LOG_WARNING, "%s: not support FRU:%d", __FUNCTION__, fru_id);
      return -1;
  }

  return 0;
}
