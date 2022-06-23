/*
 * Copyright 2022-present Facebook. All Rights Reserved.
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

#pragma once

#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <ctype.h>

#ifdef __cplusplus
extern "C" {
#endif

#define SET_SENSOR_TO_CACHE      (0)
#define SKIP_SENSOR_FAILURE      (1)

int register_sensor_failure_tolerance_policy(uint16_t fru, uint16_t snr_num, int tol_time);
int update_sensor_poll_timestamp(uint16_t fru, uint16_t snr_num, bool available, float value);

#ifdef __cplusplus
}
#endif
