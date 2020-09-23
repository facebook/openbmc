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

 /*
  * This file contains functions and logics that depends on Cloudripper specific
  * hardware and kernel drivers. Here, some of the empty "default" functions
  * are overridden with simple functions that returns non-zero error code.
  * This is for preventing any potential escape of failures through the
  * default functions that will return 0 no matter what.
  */

#include "pal-led.h"
#include "pal.h"

int init_led(int color)
{
    // TODO
}

int set_sled(int led, int color)
{
    // TODO
    return 0;
}

int set_sys_led(int color)
{
    // TODO
    return 0;
}

int set_fan_led(int color)
{
    // TODO
    return 0;
}

int set_psu_led(int color)
{
    // TODO
    return 0;
}

int set_scm_led(int color)
{
    // TODO
    return 0;
}

int pal_light_scm_led(int color)
{
    // TODO
    return 0;
}
