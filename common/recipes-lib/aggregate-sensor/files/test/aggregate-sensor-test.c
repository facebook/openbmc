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
#include <unistd.h>
#include <libgen.h>
#include <assert.h>
#include <jansson.h>
#include "aggregate-sensor.h"
#include <openbmc/kv.h>
#include <openbmc/obmc-sensor.h>

const char *test_name = "unknown";
const char *test_case = "unknown";

#define FLOAT_EQ(v1, v2) do { \
  ASSERT(v1 < (v2 + 0.001)); \
  ASSERT(v1 > (v2 - 0.001)); \
} while(0)

#define TEST_START(file, cnt) do { \
  int _rc; \
  size_t _cnt; \
  test_name = __func__; \
  test_case = "init"; \
  mock_start(); \
  _rc = aggregate_sensor_init(file); \
  ASSERT(_rc == 0); \
  _rc = aggregate_sensor_count(&_cnt); \
  ASSERT(_rc == 0); \
  ASSERT(_cnt == cnt); \
} while(0)

#define TEST_CASE(name) do { \
  mock_start(); \
  test_case = name; \
} while(0)

#define PASS() \
  printf("%s PASS\n", test_name)

#define ASSERT(cond) do { \
  if (cond) { \
    printf("%s[%s] PASS\n", test_name, test_case); \
  } else { \
    printf("%s[%s] FAIL\n", test_name, test_case); \
  } \
  assert(cond); \
} while(0)

#define MAX_SENSORS 256
#define MAX_FRUS    16
#define MAX_KEYS    16

typedef struct {
  float curr_val;
  int   curr_ret;
  int   call_cnt;
  bool  valid;
} sensor_test_t;

typedef struct {
  char key[MAX_KEY_LEN];
  char value[MAX_VALUE_LEN];
  size_t value_len;
  int  ret;
  int  call_cnt;
  bool persistent;
  bool valid;
} kv_test_t;

sensor_test_t test_sensors[MAX_FRUS][MAX_SENSORS];
kv_test_t     test_kv[MAX_KEYS];

kv_test_t *get_test_kv(bool valid, char *key)
{
  int i;
  for (i = 0; i < MAX_KEYS; i++) {
    if (!valid && test_kv[i].valid == false) {
      return &test_kv[i];
    }
    if (valid && test_kv[i].valid == true && !strcmp(test_kv[i].key, key))
      return &test_kv[i];
  }
  return NULL;
}

/* Mocked kv_get */
int kv_get(char *key, char *value, size_t *len, unsigned int flags)
{
  kv_test_t *kv = NULL;

  ASSERT(key);
  ASSERT(value);

  kv = get_test_kv(true, key);
  ASSERT(kv);
  memcpy(value, kv->value, kv->value_len);
  if (len)
    *len = kv->value_len;

  if (kv->persistent) {
    ASSERT((flags & KV_FPERSIST) != 0);
  } else {
    ASSERT((flags & KV_FPERSIST) == 0);
  }
  kv->call_cnt++;
  return kv->ret;
}

/* Mocked sensor_cache_read */
int sensor_cache_read(uint8_t fru, uint8_t snr_num, float *value)
{
  sensor_test_t *snr;
  ASSERT(fru < MAX_FRUS);
  ASSERT(snr_num < MAX_SENSORS);
  snr = &test_sensors[fru][snr_num];
  ASSERT(snr->valid);
  *value = snr->curr_val;
  snr->call_cnt++;
  return snr->curr_ret;
}

void mock_sensor_cache_read_start(void)
{
  memset(test_sensors, 0, sizeof(test_sensors));
}

void mock_kv_get_start(void)
{
  memset(test_kv, 0, sizeof(test_kv));
}

void mock_start(void)
{
  mock_sensor_cache_read_start();
  mock_kv_get_start();
}

void mock_sensor_cache_read(uint8_t fru, uint8_t snr_num, int ret, float value)
{
  sensor_test_t *snr;
  ASSERT(fru < MAX_FRUS);
  ASSERT(snr_num < MAX_SENSORS);
  snr = &test_sensors[fru][snr_num];
  snr->curr_ret = ret;
  snr->curr_val = value;
  snr->call_cnt = 0;
  snr->valid = true;
}

void mock_kv_get(char *key, char *value, bool persistent, int ret)
{
  kv_test_t *kv = get_test_kv(true, key);
  ASSERT(kv == NULL);
  kv = get_test_kv(false, key);
  ASSERT(kv);
  strcpy(kv->key, key);
  if (ret == 0) {
    strcpy(kv->value, value);
    kv->value_len = strlen(value);
  } else {
    kv->value_len = 0;
    strcpy(kv->value, "");
  }
  kv->persistent = persistent;
  kv->valid = true;
  kv->call_cnt = 0;
  kv->ret = ret;
}

void mock_sensor_cache_read_call_assert(uint8_t fru, uint8_t snr_num, int min_calls, int max_calls)
{
  sensor_test_t *snr = &test_sensors[fru][snr_num];
  ASSERT(snr->call_cnt >= min_calls);
  ASSERT(snr->call_cnt <= max_calls);
}

void mock_kv_get_call_assert(char *key, int min_calls, int max_calls)
{
  kv_test_t *kv = get_test_kv(true, key);
  ASSERT(kv);
  ASSERT(kv->call_cnt >= min_calls);
  ASSERT(kv->call_cnt <= max_calls);
}

void test_null(void)
{
  TEST_START("./test_null.json", 0);
  PASS();
}

static void name_assert(int idx, char *name, int (*func)(size_t,char *))
{
  char got_name[64] = {0};
  int ret;

  ret = func(idx, got_name);
  ASSERT(ret == 0);
  ret = strcmp(name, got_name);
  ASSERT(ret == 0);
}

void test_lexp(void)
{
  int ret;
  float value;

  TEST_START("./test_lexp.json", 1);

  TEST_CASE("get_name");
  name_assert(0, "test1", aggregate_sensor_name);

  TEST_CASE("get_units");
  name_assert(0, "TEST1", aggregate_sensor_units);

  TEST_CASE("snr1_fail");
  mock_sensor_cache_read(1, 1, -1, 0.0);
  mock_sensor_cache_read(2, 2, 0, 2.0);
  ret = aggregate_sensor_read(0, &value);
  ASSERT(ret != 0);
  mock_sensor_cache_read_call_assert(1, 1, 1, 1);
  mock_sensor_cache_read_call_assert(2, 2, 0, 1);

  TEST_CASE("snr2_fail");
  mock_sensor_cache_read(1, 1, 0, 0.0);
  mock_sensor_cache_read(2, 2, -1, 2.0);
  ret = aggregate_sensor_read(0, &value);
  ASSERT(ret != 0);
  mock_sensor_cache_read_call_assert(1, 1, 0, 1);
  mock_sensor_cache_read_call_assert(2, 2, 1, 1);

  TEST_CASE("snr_read_good");
  mock_sensor_cache_read(1, 1, 0, 1.0);
  mock_sensor_cache_read(2, 2, 0, 2.0);
  ret = aggregate_sensor_read(0, &value);
  ASSERT(ret == 0);
  mock_sensor_cache_read_call_assert(1, 1, 1, 1);
  mock_sensor_cache_read_call_assert(2, 2, 1, 1);
  /* 2.0*1.0 + 3.0*2.0 - 3.0 = 5.0 */
  FLOAT_EQ(value, 5.0);

  TEST_CASE("bad_sensor");
  ret = aggregate_sensor_read(1, &value);
  ASSERT(ret != 0);

  PASS();
}

void test_cond_lexp(void)
{
  float value;
  int ret;

  TEST_START("./test_clexp.json", 2);

  TEST_CASE("test_loaded_sensors");
  name_assert(0, "test_np", aggregate_sensor_name);
  name_assert(0, "TEST", aggregate_sensor_units);
  name_assert(1, "test_p", aggregate_sensor_name);
  name_assert(1, "TEST", aggregate_sensor_units);

  TEST_CASE("no_default_key_read_fail");
  mock_kv_get("my_key", "", false, -1);
  // we should not even call read!
  ret = aggregate_sensor_read(0, &value);
  ASSERT(ret != 0);
  mock_kv_get_call_assert("my_key", 1, 1);

  TEST_CASE("no_default_unknown_value");
  mock_kv_get("my_key", "bad_val", false, 0);
  ret = aggregate_sensor_read(0, &value);
  ASSERT(ret != 0);
  mock_kv_get_call_assert("my_key", 1, 1);

  TEST_CASE("no_default_good_reg_val");
  mock_kv_get("my_key", "V1", false, 0);
  mock_sensor_cache_read(1, 1, 0, 10.0);
  ret = aggregate_sensor_read(0, &value);
  ASSERT(ret == 0);
  mock_kv_get_call_assert("my_key", 1, 1);
  // It is dumb. so it should call cache_read for each
  // time the sensor is seen. Future, we may optimize it
  // so use min = 1, max = 2.
  mock_sensor_cache_read_call_assert(1, 1, 1, 2);
  // v1 => exp1 = (10*10) - 4 = 100-4 = 96
  FLOAT_EQ(value, 96.0);

  TEST_CASE("default_set_key_read_fail");
  mock_kv_get("my_key", "", true, -1);
  mock_sensor_cache_read(1, 1, 0, 10.0);
  ret = aggregate_sensor_read(1, &value);
  ASSERT(ret == 0);
  mock_kv_get_call_assert("my_key", 1, 1);
  mock_sensor_cache_read_call_assert(1, 1, 1, 2);
  //  def: exp2 = (snr * snr ) + 3. = 100+3 = 103.
  FLOAT_EQ(value, 103.0);

  TEST_CASE("default_set_unknown_value");
  mock_kv_get("my_key", "bad", true, 0);
  mock_sensor_cache_read(1,1,0,5.0);
  ret = aggregate_sensor_read(1, &value);
  ASSERT(ret == 0);
  FLOAT_EQ(value, 28.0);

  TEST_CASE("default_set_good_key");
  mock_kv_get("my_key", "V1", true, 0);
  mock_sensor_cache_read(1,1,0,5.0);
  ret = aggregate_sensor_read(1, &value);
  ASSERT(ret == 0);
  FLOAT_EQ(value, 21.0);

  PASS();
}

void test_lexp_source_exp(void)
{
  float value;
  int ret;

  TEST_START("./test_lexp_sexp.json", 1);

  TEST_CASE("correct_reading");
  mock_sensor_cache_read(1, 1, 0, 3.0);
  mock_sensor_cache_read(2, 2, 0, 5.0);
  ret = aggregate_sensor_read(0, &value);
  ASSERT(ret == 0);
  mock_sensor_cache_read_call_assert(1, 1, 1, 1);
  mock_sensor_cache_read_call_assert(2, 2, 1, 1);
  /* 10.0 * ((3 + 5) / 2) - 3.0 = 10 *8/2 - 3 = 10*4-3 = 37 */
  FLOAT_EQ(value, 37.0);

  TEST_CASE("snr1_na");
  mock_sensor_cache_read(1, 1, -1, 0.0);
  mock_sensor_cache_read(2, 2, 0, 5.0);
  ret = aggregate_sensor_read(0, &value);
  ASSERT(ret != 0);

  PASS();
}

void test_bad_source_exp(void)
{
  int ret;

  test_name = __func__;
  test_case = "init";
  mock_start();
  ret = aggregate_sensor_init("./test_lexp_bad_sexp.json");
  ASSERT(ret != 0);
  PASS();
}

int main(int argc, char *argv[])
{
  /* Make sure we are in the directory where the
   * executable lies. Thats where all the json
   * is copied! */
  chdir(dirname(argv[0]));

  test_null();
  test_lexp();
  test_cond_lexp();
  test_lexp_source_exp();
  test_bad_source_exp();
  printf("All tests PASS!\n");
  return 0;
}
