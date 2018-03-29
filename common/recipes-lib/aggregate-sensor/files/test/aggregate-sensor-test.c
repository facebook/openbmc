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
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <jansson.h>
#include "aggregate-sensor.h"
#include <openbmc/edb.h>
#include <openbmc/obmc-sensor.h>

#define MAX_TEST_SENSORS 32
#define MAX_TEST_VALUE_MAP_SIZE 32

typedef struct {
  char    name[32];
  uint8_t fru;
  uint8_t snr;
  float curr_val;
  int   curr_ret;
} sensor_test_t;

sensor_test_t test_sensors[MAX_TEST_SENSORS];
size_t        num_test_sensors = 0;
char          test_values[MAX_TEST_VALUE_MAP_SIZE][MAX_VALUE_LEN];
size_t        num_test_values = 0;

char          *curr_val = NULL;

int edb_cache_get(char *key, char *value)
{
  if (curr_val) {
    strcpy(value, curr_val);
    return 0;
  }
  return -1;
}

int sensor_cache_read(uint8_t fru, uint8_t snr_num, float *value)
{
  size_t i;
  for (i = 0; i < num_test_sensors; i++) {
    sensor_test_t *s = &test_sensors[i];
    if (s->fru == fru && s->snr == snr_num) {
      *value = s->curr_val;
      return s->curr_ret;
    }
  }
  return -1;
}

static json_t *get_sensor(const char *file, size_t num)
{
  static json_t *conf = NULL;
  json_t *tmp;

  if (!conf) {
    json_error_t error;
    conf = json_load_file(file, 0, &error);
    assert(conf);
  }

  tmp = json_object_get(conf, "sensors");
  assert(tmp);
  tmp = json_array_get(tmp, num);
  assert(tmp);
  return tmp;
}

static void load_test_params(const char *file, size_t num)
{
  json_t *tmp = get_sensor(file, num);
  json_t *tmp2, *iter;
  size_t i;

  /* Reset previous test loads */
  memset(test_sensors, 0, sizeof(test_sensors));
  num_test_sensors = 0;
  memset(test_values, 0, sizeof(test_values));
  num_test_values = 0;

  tmp = json_object_get(tmp, "composition");
  assert(tmp);
  tmp2 = json_object_get(tmp, "sources");
  num_test_sensors = json_object_size(tmp2);
  for (i = 0, iter = json_object_iter(tmp2);
      i < num_test_sensors && iter;
      i++, iter = json_object_iter_next(tmp2, iter)) {
    const char *name = json_object_iter_key(iter);
    json_t *val = json_object_iter_value(iter);
    json_t *o;
    assert(name);
    assert(val);
    strcpy(test_sensors[i].name, name);

    o = json_object_get(val, "fru");
    assert(o);
    assert(json_is_number(o));
    test_sensors[i].fru = json_integer_value(o);
    o = json_object_get(val, "sensor_id");
    assert(o);
    assert(json_is_number(o));
    test_sensors[i].snr = json_integer_value(o);
  }
  tmp2 = json_object_get(tmp, "type");
  assert(tmp2);
  if (!strcmp(json_string_value(tmp2), "conditional_linear_expression")) {
    tmp2 = json_object_get(tmp, "condition");
    assert(tmp2);
    tmp2 = json_object_get(tmp2, "value_map");
    assert(tmp2);
    for (i = 0, iter = json_object_iter(tmp2);
        i < MAX_TEST_VALUE_MAP_SIZE && iter;
        iter = json_object_iter_next(tmp2, iter), i++) {
      const char *key = json_object_iter_key(iter);
      json_t *val = json_object_iter_value(iter);
      assert(key);
      assert(val);
      assert(json_is_string(val));
      strncpy(test_values[num_test_values], key, MAX_VALUE_LEN);
      num_test_values++;
    }
  }
}

void set_all_sensors_valid(void)
{
  int i;
  for (i = 0; i < num_test_sensors; i++) {
    test_sensors[i].curr_ret = 0;
    test_sensors[i].curr_val = 10.0;
  }
}

bool test_sensor_ret(size_t num, bool should_pass)
{
  float value;
  int ret = aggregate_sensor_read(num, &value);
  if (should_pass) {
    if (ret != 0) {
      assert(0);
    }
  } else {
    if (ret == 0) {
      assert(0);
    }
  }
  return true;
}

void test_sensor_values(size_t num, bool accepted_value)
{
  int i;
  for (i = 0; i < num_test_sensors; i++) {
    set_all_sensors_valid();
    test_sensors[i].curr_val = 0;
    test_sensors[i].curr_ret = -1;
    printf("SNR: -1 fail expected: PASSED: %d\n", test_sensor_ret(num, false));
    test_sensors[i].curr_ret = -2;
    printf("SNR: -2 fail expected: PASSED: %d\n", test_sensor_ret(num, false));
  }
  set_all_sensors_valid();
  printf("SNR: -2 fail expected: PASSED: %d\n", test_sensor_ret(num, accepted_value));
}

int test_sensor(const char *file, size_t num, const char *name)
{
  int i;
  printf("Testing sensor %zu (%s)\n", num, name);
  load_test_params(file, num);
  printf("Sensors (%zu): ", num_test_sensors);
  for (i = 0; i < num_test_sensors; i++) {
    sensor_test_t *t = &test_sensors[i];
    printf("{%s %u %u} ", t->name, t->fru, t->snr);
  }
  printf("\nValues: ");
  for (i = 0; i < num_test_values; i++) {
    printf("%s ", test_values[i]);
  }
  printf("\n");

  for (i = 0; i < num_test_values; i++) {
    curr_val = test_values[i];
    test_sensor_values(num, true);
  }
  curr_val = "INVALID_VALUE";
  test_sensor_values(num, true); // TODO check if default is set
  curr_val = NULL;
  test_sensor_values(num, true); // TODO check if default is set

  return 0;
}

int aggregate_sensor_test(const char *file)
{
  size_t num = 0, i;
  if (aggregate_sensor_init(file)) {
    printf("Sensor initialization failed!\n");
    return -1;
  }

  aggregate_sensor_conf_print(false);

  if (aggregate_sensor_count(&num)) {
    printf("Getting sensor count failed!\n");
    return -1;
  }
  if (num == 0) {
    printf("No sensors! Nothing to do\n");
    return 0;
  }
  for(i = 0; i < num; i++) {
    char sensor_name[128];
    char sensor_unit[128];
    int ret;

    if (aggregate_sensor_name(i, sensor_name) ||
        aggregate_sensor_units(i, sensor_unit)) {
      printf("[%zu] Getting name/unit failed\n", i);
      continue;
    }
    ret = test_sensor(file, i, sensor_name);
    if (ret) {
      printf("TEst for sensor %zu failed with %d\n", i, ret);
    }
  }
  return 0;
}

int main(int argc, char *argv[])
{
  if (argc < 2) {
    printf("Usage: %s <JSON Config>\n", argv[0]);
    return -1;
  }
  return aggregate_sensor_test(argv[1]);
}
