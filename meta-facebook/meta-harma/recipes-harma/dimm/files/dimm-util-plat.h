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

#define NUM_CPU_F0M      1
#define MAX_DIMM_NUM_F0M  8

#define NUM_FRU_F0M  1
#define FRU_ID_MIN_F0M 1
#define FRU_ID_MAX_F0M 1
#define FRU_ID_ALL_F0M 1

#define DIMM0_SPD_ADDR 0xa0
#define DIMM1_SPD_ADDR 0xa4
#define DIMM2_SPD_ADDR 0xa2
#define DIMM3_SPD_ADDR 0xa6

#define DIMM0_PMIC_ADDR 0x90
#define DIMM1_PMIC_ADDR 0x94
#define DIMM2_PMIC_ADDR 0x92
#define DIMM3_PMIC_ADDR 0x96
#endif
