/*
 * Copyright 2014-present Facebook. All Rights Reserved.
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
#ifndef GPIO_H
#define GPIO_H

typedef struct {
  int gs_gpio;
  int gs_fd;
} gpio_st;

typedef enum {
  GPIO_DIRECTION_IN,
  GPIO_DIRECTION_OUT,
} gpio_direction_en;

typedef enum {
  GPIO_VALUE_LOW = 0,
  GPIO_VALUE_HIGH = 1,
} gpio_value_en;

int gpio_open(gpio_st* g, int gpio);
void gpio_close(gpio_st *g);
gpio_value_en gpio_read(gpio_st *g);
void gpio_write(gpio_st *g, gpio_value_en v);
int gpio_change_direction(gpio_st *g, gpio_direction_en dir);

#endif
