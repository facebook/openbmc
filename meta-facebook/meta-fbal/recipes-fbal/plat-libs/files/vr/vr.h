/*
 *
 * Copyright 2019-present Facebook. All Rights Reserved.
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

#ifndef __VR_H__
#define __VR_H__
#include <openbmc/kv.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_VER_STR_LEN (MAX_KEY_LEN + 16)

enum {
  VR_PCH_PVNN      = 0,
  VR_CPU0_VCCIN    = 1,
  VR_CPU0_VCCIO    = 2,
  VR_CPU0_VDDQ_ABC = 3,
  VR_CPU0_VDDQ_DEF = 4,
  VR_CPU1_VCCIN    = 5,
  VR_CPU1_VCCIO    = 6,
  VR_CPU1_VDDQ_ABC = 7,
  VR_CPU1_VDDQ_DEF = 8,
};

enum {
  ADDR_PCH_PVNN      = 0x94,
  ADDR_CPU0_VCCIN    = 0xC0,
  ADDR_CPU0_VCCIO    = 0xC4,
  ADDR_CPU0_VDDQ_ABC = 0xCC,
  ADDR_CPU0_VDDQ_DEF = 0xD0,
  ADDR_CPU1_VCCIN    = 0xE0,
  ADDR_CPU1_VCCIO    = 0xE4,
  ADDR_CPU1_VDDQ_ABC = 0xEC,
  ADDR_CPU1_VDDQ_DEF = 0xD8,
};

enum {
  VR_STATUS_SUCCESS       = 0,
  VR_STATUS_FAILURE       = -1,
  VR_STATUS_NOT_AVAILABLE = -2,
};

int vr_fw_version(uint8_t vr, char *ver_str);
int vr_fw_update(char *path);

#ifdef __cplusplus
} // extern "C"
#endif

#endif
