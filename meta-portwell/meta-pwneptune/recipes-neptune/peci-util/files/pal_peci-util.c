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
#include <stdlib.h>
#include <unistd.h>
#include <syslog.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <fcntl.h>
#include <openbmc/gpio.h>
#include <openbmc/pal.h>

#define GPIO_PECI_MUX_SEL 212
#define GPIO_PECI_MUX_SEL_PREDVT 218

int pal_before_peci(void) {
  gpio_st gpio_peci_mux;
  uint8_t board_rev;
  int gpio_peci_mux_num, ret;

  ret = pal_get_board_rev_id(&board_rev);
  if (ret)
    board_rev = BOARD_REV_MP;

  if (board_rev <= BOARD_REV_PREDVT)
    gpio_peci_mux_num = GPIO_PECI_MUX_SEL_PREDVT;
  else
    gpio_peci_mux_num = GPIO_PECI_MUX_SEL;

  gpio_open(&gpio_peci_mux,  gpio_peci_mux_num);
  gpio_write(&gpio_peci_mux, GPIO_VALUE_HIGH);
  gpio_close(&gpio_peci_mux);
  return 0;
}

int pal_after_peci(void) {
  gpio_st gpio_peci_mux;
  uint8_t board_rev;
  int gpio_peci_mux_num, ret;

  ret = pal_get_board_rev_id(&board_rev);
  if (ret)
    board_rev = BOARD_REV_MP;

  if (board_rev <= BOARD_REV_PREDVT)
    gpio_peci_mux_num = GPIO_PECI_MUX_SEL_PREDVT;
  else
    gpio_peci_mux_num = GPIO_PECI_MUX_SEL;

  gpio_open(&gpio_peci_mux,  gpio_peci_mux_num);
  gpio_write(&gpio_peci_mux, GPIO_VALUE_LOW);
  gpio_close(&gpio_peci_mux);;
  return 0;
}
