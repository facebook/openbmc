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
#define MAX_SENSOR_NUM      0xFF

#define MAX_SENSOR_RATE_UNIT  7
#define MAX_SENSOR_BASE_UNIT  92

#define SETBIT(x, y)        (x | (1 << y))
#define GETBIT(x, y)        ((x & (1 << y)) > y)
#define CLEARBIT(x, y)      (x & (~(1 << y)))
#define GETMASK(y)          (1 << y)

/* To hold the sensor info and calculated threshold values from the SDR */
typedef struct {
  uint16_t flag;
  float ucr_thresh;
  float unc_thresh;
  float unr_thresh;
  float lcr_thresh;
  float lnc_thresh;
  float lnr_thresh;
  float pos_hyst;
  float neg_hyst;
  int curr_state;
  char name[32];
  char units[64];

} thresh_sensor_t;

int sdr_get_sensor_name(uint8_t fru, uint8_t snr_num, char *name);
int sdr_get_sensor_units(uint8_t fru, uint8_t snr_num, char *units);
int sdr_get_snr_thresh(uint8_t fru, uint8_t snr_num, thresh_sensor_t *snr);

#ifdef __cplusplus
} // extern "C"
#endif

#endif /* __SDR_H__ */
