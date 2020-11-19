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

#ifndef __PAL_PIM_H__
#define __PAL_PIM_H__

#define PIM_I2C_BASE            80
#define PIM_I2C_OFFSET          8

enum {
    PIM_16Q_EVT1 = 0,
    PIM_16Q_EVT2 = 1,
    PIM_16Q_EVT3 = 2,
    PIM_16Q_DVT,
};

enum {
    PIM_16Q_PHY_16NM,
    PIM_16Q_PHY_7NM,
    PIM_16Q_PHY_UNKNOWN,
};

int pal_pim_is_7nm(int fru);

#endif
