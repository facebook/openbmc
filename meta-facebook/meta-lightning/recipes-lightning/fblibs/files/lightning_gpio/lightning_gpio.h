/*
 *
 * Copyright 2018-present Facebook. All Rights Reserved.
 *
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

#ifndef __LIGHTNING_GPIO_H__
#define __LIGHTNING_GPIO_H__

#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>
#include <openbmc/gpio.h>
#include <facebook/lightning_common.h>
#ifdef __cplusplus
extern "C" {
#endif

#define fcb_sensor_fan1_front_speed 0x20
extern size_t pal_tach_count;

int gpio_peer_tray_detection(uint8_t *value);
int gpio_reset_ssd_switch();

#ifdef __cplusplus
} // extern "C"
#endif

#endif /* __LIGHTNING_GPIO_H__ */
