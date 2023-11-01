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
#include <openbmc/pal_sensors.h>
#include <openbmc/cmock.h>

DEFINE_MOCK_FUNC(int, kv_get, const char *, char *, size_t *, unsigned int);
DEFINE_MOCK_FUNC(int, sensor_cache_read, uint8_t, uint8_t, float *);

static void init_sensors(const char *json_file, size_t exp_sensors)
{
  int ret;
  size_t cnt;

  ret = aggregate_sensor_init(json_file);
  ASSERT(ret == 0, "Initialization");
  ret = aggregate_sensor_count(&cnt);
  ASSERT(ret == 0, "Getting count succeeds");
  ASSERT(cnt == exp_sensors, "Expected sensors");
}

DEFINE_TEST(test_bad_source_exp)
{
  int ret;
  ret = aggregate_sensor_init("./test_lexp_bad_sexp.json");
  ASSERT_NEQ(ret, 0, "Fail loading non-existant conf.");
}

DEFINE_TEST(test_null)
{
  init_sensors("./test_null.json", 0);
}

DEFINE_TEST(test_lexp)
{
  char got_name[64] = {0};
  int ret;
  uint32_t interval = 0;
  float val;

  init_sensors("./test_lexp.json", 1);
  ASSERT(aggregate_sensor_name(0, got_name) == 0, "Read sensor name");
  ASSERT_EQ_STR(got_name, "test1", "Matching sensor name");
  ASSERT(aggregate_sensor_units(0, got_name) == 0, "Read sensor units");
  ASSERT_EQ_STR(got_name, "TEST1", "Matching sensor units");
  ASSERT(aggregate_sensor_poll_interval(0, &interval) == 0, "Read sensor poll interval");
  ASSERT_EQ(interval, 2, "Default monitoring interval");

  int mocked_read1(uint8_t fru, uint8_t snr, float *value) {
    ASSERT((fru == 1 && snr == 1) || (fru == 2 && snr == 2), "Expected FRU/SNRID");
    if (snr == 2) {
      *value = 2.0;
      return 0;
    }
    return -1;
  }
  MOCK(sensor_cache_read, mocked_read1);
  ret = aggregate_sensor_read(0, &val);
  // One of the sensors failed.
  ASSERT_NEQ(ret, 0, "sensor read failed as expected");
  ASSERT_CALL_COUNT(sensor_cache_read, 1, 2, "sensor_cache_read was called at least once");

  int mocked_read2(uint8_t fru, uint8_t snr, float *value) {
    if (snr == 1) {
      *value = 0.0;
      return 0;
    }
    return -1;
  }
  MOCK(sensor_cache_read, mocked_read2);
  ret = aggregate_sensor_read(0, &val);
  ASSERT_NEQ(ret, 0, "Sensor read failed as expected");
  ASSERT_CALL_COUNT(sensor_cache_read, 1, 2, "sensor read called at least once");

  int mocked_read3(uint8_t fru, uint8_t snr, float *value) {
    ASSERT((fru == 1 && snr == 1) || (fru == 2 && snr == 2), "Expected FRU/SNRID");
    *value = snr == 1 ? 1.0 : 2.0;
    return 0;
  }
  MOCK(sensor_cache_read, mocked_read3);
  ret = aggregate_sensor_read(0, &val);
  ASSERT_EQ(ret, 0, "sensor read succeeded");
  /* 2.0*1.0 + 3.0*2.0 - 3.0 = 5.0 */
  ASSERT_EQ_FLT(val, 5.0, "Sensor read returned correct value");
  ASSERT_CALL_COUNT(sensor_cache_read, 2, 2, "sensor read called exactly twice");

  // Ensure any sensor returns success.
  MOCK_RETURN(sensor_cache_read, 0);
  ret = aggregate_sensor_read(1, &val);
  ASSERT(ret != 0, "sensor correctly failed to read invalid agg-sensor");
  ASSERT_CALL_COUNT(sensor_cache_read, 0, 0, "Sensor read correctly never called");
}

DEFINE_TEST(test_cond_lexp)
{
  float val;
  int ret;
  char got_name[64];
  uint32_t interval = 0;
  thresh_sensor_t thresh;

  init_sensors("./test_clexp.json", 2);
  ASSERT(aggregate_sensor_name(0, got_name) == 0, "sensor1 name read success");
  ASSERT_EQ_STR(got_name, "test_np", "Sensor1 name correct");
  ASSERT(aggregate_sensor_units(0, got_name) == 0, "Sensor1 units read success");
  ASSERT_EQ_STR(got_name, "TEST", "Sensor1 units correct");
  ASSERT(aggregate_sensor_name(1, got_name) == 0, "sensor2 name read success");
  ASSERT_EQ_STR(got_name, "test_p", "Sensor2 name correct");
  ASSERT(aggregate_sensor_units(1, got_name) == 0, "Sensor2 units read success");
  ASSERT_EQ_STR(got_name, "TEST", "Sensor2 units correct");
  ASSERT(aggregate_sensor_poll_interval(0, &interval) == 0, "Sensor 1 poll interval");
  ASSERT_EQ(interval, 1, "Sensor1 valid poll interval");
  ASSERT(aggregate_sensor_poll_interval(1, &interval) == 0, "Sensor 2 poll interval");
  ASSERT_EQ(interval, 2, "Sensor2 default poll interval");
  ASSERT(aggregate_sensor_threshold(0, &thresh) == 0, "Sensor 1 threshold");
  ASSERT_EQ(thresh.poll_interval, 1, "Thresh poll interval");
  ASSERT(aggregate_sensor_threshold(1, &thresh) == 0, "Sensor 2 threshold");
  ASSERT_EQ(thresh.poll_interval, 2, "Thresh poll interval");

  MOCK_RETURN(sensor_cache_read, 0);
  MOCK_RETURN(kv_get, -1);
  ret = aggregate_sensor_read(0, &val);
  ASSERT_NEQ(ret, 0, "agg-sensor should fail if kv_get fails with no default expression");
  ASSERT_CALL_COUNT(kv_get, 1, 1, "kv_get called exactly once");
  ASSERT_CALL_COUNT(sensor_cache_read, 0, 0, "sensor_cache_read never called");

  int mocked_kv_get1(const char *key, char *value, size_t *len, unsigned int flags)
  {
    ASSERT_EQ(flags, 0, "Flags are for non-persistent kv");
    if (!strcmp(key, "my_key")) {
      strcpy(value, "bad_val");
      if (len) *len = strlen(value);
      return 0;
    }
    return -1;
  }
  MOCK(kv_get, mocked_kv_get1);
  MOCK_RETURN(sensor_cache_read, 0);
  ret = aggregate_sensor_read(0, &val);
  ASSERT_NEQ(ret, 0, "Agg-sensor should fail if kv_get returns an unknown value");
  ASSERT_CALL_COUNT(kv_get, 1, 1, "kv_Get called exactly once");
  ASSERT_CALL_COUNT(sensor_cache_read, 0, 0, "sensor read never called");

  int mocked_kv_get2(const char *key, char *value, size_t *len, unsigned int flags)
  {
    ASSERT_EQ(flags, 0, "Flags are for non-persistent kv");
    if (!strcmp(key, "my_key")) {
      strcpy(value, "V1");
      if (len) *len = strlen(value);
      return 0;
    }
    return -1;
  }
  int mocked_snr_read1(uint8_t fru, uint8_t snr, float *value)
  {
    ASSERT(fru == 1 && snr == 1, "Expected FRU/SNRID");
    *value = 10.0;
    return 0;
  }
  MOCK(kv_get, mocked_kv_get2);
  MOCK(sensor_cache_read, mocked_snr_read1);
  ret = aggregate_sensor_read(0, &val);
  ASSERT_EQ(ret, 0, "agg-read passes");
  ASSERT_CALL_COUNT(kv_get, 1, 1, "kv_get called once");
  ASSERT_CALL_COUNT(sensor_cache_read, 1, 2, "sensor cache read called at least once");
  // v1 => exp1 = (10*10) - 4 = 100-4 = 96
  ASSERT_EQ_FLT(val, 96.0, "Correct value returned");

  MOCK_RETURN(kv_get, -1);
  MOCK(sensor_cache_read, mocked_snr_read1);
  ret = aggregate_sensor_read(1, &val);
  ASSERT_EQ(ret, 0, "agg-read success");
  ASSERT_CALL_COUNT(kv_get, 1, 1, "kv get called once");
  ASSERT_CALL_COUNT(sensor_cache_read, 1, 2, "sensor_cache_read called at least once");
  ASSERT_EQ_FLT(val, 103.0, "Correct value returned");

  // kv_get succeeds, but returns an unknown value.
  int mocked_kv_get3(const char *key, char *value, size_t *len, unsigned int flags)
  {
    ASSERT_EQ(flags, KV_FPERSIST, "Flags are for persistent kv");
    if (!strcmp(key, "my_key")) {
      strcpy(value, "bad_val");
      if (len)
        *len = strlen(value);
      return 0;
    }
    return -1;
  }
  MOCK(kv_get, mocked_kv_get3);
  MOCK(sensor_cache_read, mocked_snr_read1);
  ret = aggregate_sensor_read(1, &val);
  ASSERT_EQ(ret, 0, "agg-read success");
  ASSERT_CALL_COUNT(kv_get, 1, 1, "kv get called once");
  ASSERT_CALL_COUNT(sensor_cache_read, 1, 2, "sensor_cache_read called at least once");
  ASSERT_EQ_FLT(val, 103.0, "Correct value returned");

  int mocked_kv_get4(const char *key, char *value, size_t *len, unsigned int flags)
  {
    ASSERT_EQ(flags, KV_FPERSIST, "Flags are for persistent kv");
    if (!strcmp(key, "my_key")) {
      strcpy(value, "V1");
      if (len)
        *len = strlen(value);
      return 0;
    }
    return -1;
  }
  MOCK(kv_get, mocked_kv_get4);
  MOCK(sensor_cache_read, mocked_snr_read1);
  ret = aggregate_sensor_read(1, &val);
  ASSERT_EQ(ret, 0, "agg-read success");
  ASSERT_CALL_COUNT(kv_get, 1, 1, "kv get called once");
  ASSERT_CALL_COUNT(sensor_cache_read, 1, 2, "sensor_cache_read called at least once");
  ASSERT_EQ_FLT(val, 96.0, "Correct value returned");
}

DEFINE_TEST(test_lexp_source_exp)
{
  float val;
  int ret;

  init_sensors("./test_lexp_sexp.json", 1);

  int mocked_read1(uint8_t fru, uint8_t snr, float *value) {
    ASSERT((fru == 1 && snr == 1) || (fru == 2 && snr == 2), "Expected FRU/SNRID");
    *value = snr == 1 ? 3.0 : 5.0;
    return 0;
  }
  MOCK(sensor_cache_read, mocked_read1);
  ret = aggregate_sensor_read(0, &val);
  ASSERT_EQ(ret, 0, "agg-read success");
  ASSERT_CALL_COUNT(sensor_cache_read, 2, 2, "Expected sensor reads");
  ASSERT_EQ_FLT(val, 37.0, "Correct value read");
  /* 10.0 * ((3 + 5) / 2) - 3.0 = 10 *8/2 - 3 = 10*4-3 = 37 */

  int mocked_read2(uint8_t fru, uint8_t snr, float *value) {
    ASSERT((fru == 1 && snr == 1) || (fru == 2 && snr == 2), "Expected FRU/SNRID");
    if (snr == 2) {
      *value = 5.0;
      return 0;
    }
    return -1;
  }
  MOCK(sensor_cache_read, mocked_read2);
  ret = aggregate_sensor_read(0, &val);
  ASSERT_NEQ(ret, 0, "agg-read should fail");
  ASSERT_CALL_COUNT(sensor_cache_read, 1, 2, "cache read called at least once");
}

int main(int argc, char *argv[])
{
  if (chdir(dirname(argv[0])) != 0) {
    printf("Cannot chdir into %s\n", dirname(argv[0]));
    return -1;
  }

  CALL_TESTS();
  return 0;
}
