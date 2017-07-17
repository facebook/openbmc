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

#ifndef _AGGREGATE_SENSOR_INTERNAL_H_
#define _AGGREGATE_SENSOR_INTERNAL_H_
#include "aggregate-sensor.h"
#include "math_expression.h"
#include <openbmc/edb.h>
#include <openbmc/pal.h>
#include <openbmc/sdr.h>

#ifdef __DEBUG__
#define DEBUG(fmt, ...) printf(fmt, ##__VA_ARGS__)
#else
#define DEBUG(fmt, ...)
#endif

#define MAX_COMPOSITION 8
#define MAX_CONDITIONALS 16
#define MAX_STRING_SIZE 128

struct sensor_src {
  uint8_t fru;
  uint8_t id;
};

typedef struct {
  char condition_value[MAX_VALUE_LEN];
  size_t formula_index;
} value_map_element_type;

typedef struct {
  thresh_sensor_t sensor;
  size_t idx;
  size_t num_expressions;
  expression_type **expressions;
  char cond_key[MAX_KEY_LEN];
  size_t value_map_size;
  value_map_element_type value_map[MAX_CONDITIONALS];
  int default_expression_idx; /* -1 == invalid */
} aggregate_sensor_t;

extern size_t g_sensors_count;
extern aggregate_sensor_t *g_sensors;

int load_aggregate_conf(const char *conf_path);
int get_sensor_value(void *state, float *value);

#endif
