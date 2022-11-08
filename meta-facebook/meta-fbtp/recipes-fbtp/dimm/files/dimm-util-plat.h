/*
 *
 * Copyright 2014-present Facebook. All Rights Reserved.
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

#define NUM_CPU_FBTP      2   // TP has 2 CPUs
#define MAX_DIMM_NUM_FBTP 6  // per CPU,
#define NUM_FRU_FBTP      1

#define ME_BUS_ADDR         0x04  // ME bus address in FBTP
#define ME_SLAVE_ADDR       0x2C  // ME slave address in FBTP
#define IPMB_BMC_SLAVE_ADDR 0x20

#define FRU_ID_MIN_FBTP 1
#define FRU_ID_MAX_FBTP 1
#define FRU_ID_ALL_FBTP 1

#define DIMM0_SPD_ADDR 0xa0
#define DIMM1_SPD_ADDR 0xa2
#define DIMM2_SPD_ADDR 0xa4
#define DIMM3_SPD_ADDR 0xa6
#define DIMM4_SPD_ADDR 0xa8
#define DIMM5_SPD_ADDR 0xaa
#define DIMM6_SPD_ADDR 0xac
#define DIMM7_SPD_ADDR 0xae

#endif
