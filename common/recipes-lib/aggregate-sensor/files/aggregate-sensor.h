/*
 *
 * Copyright 2017-present Facebook. All Rights Reserved.
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

#ifndef _AGGREGATE_SENSOR_H_
#define _AGGREGATE_SENSOR_H_

#ifdef __cplusplus
extern "C"{
#endif

#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <openbmc/sdr.h>

int aggregate_sensor_count(size_t *count);
int aggregate_sensor_read(size_t index, float *value);
int aggregate_sensor_threshold(size_t index, thresh_sensor_t *thresh);
int aggregate_sensor_name(size_t index, char *name);
int aggregate_sensor_units(size_t index, char *units);
int aggregate_sensor_poll_interval(size_t index, uint32_t *poll_interval);

/* Called once per process. Safe to call multiple times */
int aggregate_sensor_init(const char *conf_file);

void aggregate_sensor_conf_print(bool syslog);

#ifdef __cplusplus
}
#endif

#endif
