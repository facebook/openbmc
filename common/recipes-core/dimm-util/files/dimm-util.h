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

#ifndef __DIMM_UTIL_H__
#define __DIMM_UTIL_H__

#define MAX_CPU_NUM       2  // max number of CPUs
#define MAX_DIMM_PER_CPU 12 //  max number of dimms per CPU

#define OFFSET_SERIAL 0x45
#define LEN_SERIAL    4
#define LEN_SERIAL_STRING ((LEN_SERIAL * 2) + 1) // 2 hex digit per byte + null
#define OFFSET_PART_NUMBER 0x49
#define LEN_PART_NUMBER    20
#define LEN_PN_STRING      (LEN_PART_NUMBER + 1)

#define DEFAULT_DUMP_OFFSET 0
#define DEFAULT_DUMP_LEN    0x100
#define MAX_RETRY 3
#define MAX_FAIL_CNT LEN_SERIAL

#define ERR_INVALID_SYNTAX -2

#define INTEL_ID_LEN  3
#define MANU_INTEL_0  0x57
#define MANU_INTEL_1  0x01
#define MANU_INTEL_2  0x00

#define FACEBOOK_ID_LEN  3
#define MANU_FACEBOOK_0  0x15
#define MANU_FACEBOOK_1  0xa0
#define MANU_FACEBOOK_2  0x00

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(_a) (sizeof(_a) / sizeof((_a)[0]))
#endif /* ARRAY_SIZE */



extern char *vendor_name[];
extern const char * manu_string(uint8_t id);

extern int num_frus;
extern int num_cpus;
extern int num_dimms_per_cpu;
extern int total_dimms;
extern char const **fru_name;
extern int fru_id_all;
extern int fru_id_min;
extern int fru_id_max;

int get_die_capacity(uint8_t data);
int get_bus_width_bits(uint8_t data);
int get_device_width_bits(uint8_t data);
int get_package_rank(uint8_t data);
#endif
