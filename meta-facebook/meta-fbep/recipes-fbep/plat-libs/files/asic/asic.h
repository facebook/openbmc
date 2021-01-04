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
 *
 */

#ifndef __LIB_ASIC_H__
#define __LIB_ASIC_H__

#ifdef __cplusplus
extern "C" {
#endif

#define MFR_AMD      "AMD"
#define MFR_NVIDIA   "NVIDIA"
#define MFR_UNKNOWN  "UNKNOWN"

#define asic_slot_to_bus(x) ((~(x) >> 1) & 0xB)

enum {
  GPU_NVIDIA = 0,
  GPU_AMD,
  GPU_UNKNOWN
};

#define MFR_MAX_NUM GPU_UNKNOWN

enum {
  ASIC_SUCCESS = 0,
  ASIC_ERROR = -1,
  ASIC_NOTSUP = -2
};

uint8_t asic_get_vendor_id(uint8_t);
int asic_read_gpu_temp(uint8_t, float*);
int asic_read_board_temp(uint8_t, float*);
int asic_read_mem_temp(uint8_t, float*);
int asic_read_pwcs(uint8_t, float*);
bool is_asic_prsnt(uint8_t);
int asic_set_power_limit(uint8_t, unsigned int);
int asic_get_power_limit(uint8_t, unsigned int*);
int asic_show_version(uint8_t, char*);

#ifdef __cplusplus
} // extern "C"
#endif

#endif /* __LIB_ASIC_H__ */
