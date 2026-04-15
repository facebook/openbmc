/*
 *
 * Copyright 2024-present Facebook. All Rights Reserved.
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

#ifndef __DIMM_UTIL_PLAT_H__
#define __DIMM_UTIL_PLAT_H__

#define NUM_CPU_NETLAKE2      1
#define MAX_DIMM_NUM_NETLAKE2  2

#define NUM_FRU_NETLAKE2  1
#define FRU_ID_MIN_NETLAKE2 1
#define FRU_ID_MAX_NETLAKE2 1
#define FRU_ID_ALL_NETLAKE2 1

// 8-bits address
#define DIMMA_SPD_ADDR 0xa0
#define DIMMB_SPD_ADDR 0xa2

#define DIMMA_PMIC_ADDR 0x90
#define DIMMB_PMIC_ADDR 0x92
#endif
