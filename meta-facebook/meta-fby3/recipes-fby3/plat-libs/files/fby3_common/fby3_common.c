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
#include <unistd.h>
#include <math.h>
#include <fcntl.h>
#include <errno.h>
#include <syslog.h>
#include <pthread.h>
#include <string.h>
#include <openbmc/libgpio.h>
#include "fby3_common.h"

static int
get_gpio_shadow_array(const char **shadows, int num, uint8_t *mask)
{
  int i;
  *mask = 0;
  for (i = 0; i < num; i++) {
    int ret;
    gpio_value_t value;
    gpio_desc_t *gpio = gpio_open_by_shadow(shadows[i]);
    if (!gpio) {
      return -1;
    }
    ret = gpio_get_value(gpio, &value);
    gpio_close(gpio);
    if (ret != 0) {
      return -1;
    }
    *mask |= (value == GPIO_VALUE_HIGH ? 1 : 0) << i;
  }
  return 0;
}

int
is_valid_slot_str(char *str, uint8_t *fru) {
  if (!strcmp(str, "slot0")) {
    *fru = FRU_SLOT0;
  } else if (!strcmp(str, "slot1")) {
    *fru = FRU_SLOT1;
  } else if (!strcmp(str, "slot2")) {
    *fru = FRU_SLOT2;
  } else if (!strcmp(str, "slot3")) {
    *fru = FRU_SLOT3;
  } else {
    return -1;
  }

  return 0;
}

int
is_valid_slot_id(uint8_t fru) {
  int ret = 0;

  switch(fru) {
    case FRU_SLOT0:
    case FRU_SLOT1:
    case FRU_SLOT2:
    case FRU_SLOT3:
      break;
    default:
      syslog(LOG_WARNING, "%s() wrong fru id 0x%2X", __func__, fru);
      ret = -1;
      break;
  }

  return ret;
}

int
fby3_common_get_bus_id(uint8_t slot_id) {
  int bus_id = 0;
  switch(slot_id) {
    case FRU_SLOT0:
      bus_id = IPMB_SLOT0_I2C_BUS;
    break;
    case FRU_SLOT1:
      bus_id = IPMB_SLOT1_I2C_BUS;
    break;
    case FRU_SLOT2:
      bus_id = IPMB_SLOT2_I2C_BUS;
    break;
    case FRU_SLOT3:
      bus_id = IPMB_SLOT3_I2C_BUS;
    break;
    default:
      bus_id = -1;
    break;
  }
  return bus_id; 
}

int
get_bmc_location(uint8_t *id) {
  static bool is_cached = false;
  static uint8_t cached_id = 0;

  if ( is_cached == false ) {
    const char *shadows[] = {
      "BOARD_BMC_ID0_R",
      "BOARD_BMC_ID1_R",
      "BOARD_BMC_ID2_R",
      "BOARD_BMC_ID3_R",
    };

    if ( get_gpio_shadow_array(shadows, ARRAY_SIZE(shadows), &cached_id) ) {
      return -1;
    }

    is_cached = true;
  }

  *id = cached_id;

  return 0;
}
