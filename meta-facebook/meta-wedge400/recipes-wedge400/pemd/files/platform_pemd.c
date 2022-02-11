/*
 * Copyright 2021-present Facebook. All Rights Reserved.
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
 * This file is intended to contain platform specific definitions, and
 * it's set to empty intentionally. Each platform needs to override the
 * file if platform specific settings are required.
 */

#include "platform_pemd.h"
#include <openbmc/pal.h>
int pal_set_thx_power_off(void)
{
    if (pal_set_th3_power(TH3_POWER_OFF)) {
        return 1;
    }
    return 0;
}
