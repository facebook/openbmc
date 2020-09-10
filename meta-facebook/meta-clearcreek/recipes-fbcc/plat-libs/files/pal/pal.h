/*
 *
 * Copyright 2015-present Facebook. All Rights Reserved.
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

#ifndef __PAL_H__
#define __PAL_H__

#include "pal_power.h"
#include "pal_sensors.h"
#include <openbmc/obmc-pal.h>
#include <openbmc/kv.h>
#include "pal_sensors.h"
#include "pal_switch.h"

#ifdef __cplusplus
extern "C" {
#endif

extern size_t pal_pwm_cnt;
extern size_t pal_tach_cnt;
extern const char pal_pwm_list[];
extern const char pal_tach_list[];
extern const char pal_fru_list[];
extern const char pal_server_list[];

enum {
  FRU_ALL = 0,
  FRU_MB,
  FRU_BSM,
  FRU_PDB,
  FRU_AVA1,
  FRU_AVA2,
  FRU_CNT,
};

#define MAX_NUM_FRUS (FRU_CNT-1)
#define MAX_NODES    1
#define LARGEST_DEVICE_NAME 120

#ifdef __cplusplus
} // extern "C"
#endif

int pal_get_rst_btn(uint8_t *status);
bool pal_is_server_off(void);

#endif /* __PAL_H__ */
