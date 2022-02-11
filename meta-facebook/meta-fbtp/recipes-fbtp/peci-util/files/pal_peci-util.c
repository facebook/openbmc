/*
 * peci-util
 *
 * Copyright 2015-present Facebook. All Rights Reserved.
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
#include <openbmc/libgpio.h>

int pal_before_peci(void) {
  gpio_desc_t *gpio = gpio_open_by_shadow("PECI_MUX_SELECT");
  int ret = -1;

  if (gpio) {
    if (gpio_set_value(gpio, GPIO_VALUE_HIGH) == 0) {
      ret = 0;
    }
    gpio_close(gpio);
  }
  return ret;
}

int pal_after_peci(void) {
  gpio_desc_t *gpio = gpio_open_by_shadow("PECI_MUX_SELECT");
  int ret = -1;

  if (gpio) {
    if (gpio_set_value(gpio, GPIO_VALUE_LOW) == 0) {
      ret = 0;
    }
    gpio_close(gpio);
  }
  return ret;
}
