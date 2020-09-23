/*
 *
 * Copyright 2020-present Facebook. All Rights Reserved.
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

#ifndef __PAL_LED_H__
#define __PAL_LED_H__

enum {
    LED_COLOR_BLUE,
    SCM_LED_BLUE = LED_COLOR_BLUE,
    LED_COLOR_GREEN,
    LED_COLOR_RED,
    LED_COLOR_AMBER,
    SCM_LED_AMBER = LED_COLOR_AMBER,
    LED_COLOR_OFF,
};

enum {
    LED_SYS,
    LED_FAN,
    LED_PSU,
    LED_SCM,
};

int init_led(int color);
int set_sled(int led, int color);
int set_sys_led(int color);
int set_fan_led(int color);
int set_psu_led(int color);
int set_scm_led(int color);
int pal_light_scm_led(int color);

#endif /* __PAL_LED_H__ */


