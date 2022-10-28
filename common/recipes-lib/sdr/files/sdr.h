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

#ifndef __SDR_H__
#define __SDR_H__

#include <stdbool.h>
#include <openbmc/ipmi.h>
#include <openbmc/pal.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_SDR_LEN           64
#define NORMAL_STATE        0x00
#ifndef MAX_SENSOR_NUM
#define MAX_SENSOR_NUM      (0xFF)
#endif

#define MAX_SENSOR_RATE_UNIT  7
#define MAX_SENSOR_BASE_UNIT  92

int sdr_get_sensor_name(uint8_t fru, uint8_t snr_num, char *name);
int sdr_get_sensor_units(uint8_t fru, uint8_t snr_num, char *units);
int sdr_get_snr_thresh(uint8_t fru, uint8_t snr_num, thresh_sensor_t *snr);

#define FORMAT_CONV(X) ((int)(X*100 + 0.5)*0.01)  //take the second decimal place

#ifdef __cplusplus
} // extern "C"
#endif

#endif /* __SDR_H__ */
