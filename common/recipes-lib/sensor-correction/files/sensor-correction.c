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

#include "sensor-correction.h"
#include <assert.h>
#include <jansson.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#ifndef __TEST__
#include <syslog.h>
#endif
#include <openbmc/kv.h>


#ifdef __DEBUG__
  #ifdef __TEST__
    #define DEBUG(fmt, ...) printf(fmt, ## __VA_ARGS__)
  #else
    #define DEBUG(fmt, ...) syslog(LOG_DEBUG, fmt, ## __VA_ARGS__)
  #endif
#else
#define DEBUG(fmt, ...)
#endif

#ifdef __TEST__
#define INFO(fmt, ...) printf(fmt, ## __VA_ARGS__)
#else
#define INFO(fmt, ...) syslog(LOG_INFO, fmt, ## __VA_ARGS__)
#endif

#define get_float(obj) (json_is_real(obj) ? json_real_value(obj) : (float)json_integer_value(obj))

#define MAX_NUM_CONDITIONS 32
#define MAX_NUM_TABLES     32

typedef struct {
  char cond_value[MAX_VALUE_LEN];
  size_t table_idx;
} value_map_element_t;

typedef struct {
  float cond_value;
  float correction;
} correction_element_t;

typedef struct {
  char name[32];
  size_t num;
  correction_element_t *corr_table;
} correction_table_t;

typedef enum {
  KEY_REGULAR,
  KEY_PERSISTENT
} key_type;

typedef struct {
  char    name[32];
  uint8_t fru;
  uint8_t id;
  size_t  num_tables;
  correction_table_t tables[MAX_NUM_TABLES];
  size_t  default_table;
  key_type cond_key_type;
  char    cond_key[MAX_KEY_LEN];
  size_t  value_map_size;
  value_map_element_t value_map[MAX_NUM_CONDITIONS];
} sensor_correction_t;

static sensor_correction_t *g_sensors = NULL;
static size_t g_sensors_count = 0;

static int get_table(value_map_element_t *value_map, size_t num, char *value, size_t *idx)
{
  size_t i;
  for (i = 0; i < num; i++) {
    if (!strcmp(value, value_map[i].cond_value)) {
      *idx = value_map[i].table_idx;
      return 0;
    }
  }
  DEBUG("Could not get table for value: %s\n", value);
  return -1;
}

static sensor_correction_t *get_correction(uint8_t fru, uint8_t sensor_id)
{
  size_t i;
  for(i = 0; i < g_sensors_count; i++) {
    if (g_sensors[i].fru == fru && g_sensors[i].id == sensor_id) {
      return &g_sensors[i];
    }
  }
  return NULL;
}

static int load_table(json_t *obj, correction_table_t *tbl)
{
  size_t i;

  tbl->num = json_array_size(obj);
  if (!tbl->num) {
    DEBUG("Unsupported number of entries in correction table: %zu\n", tbl->num);
    return -1;
  }
  tbl->corr_table = calloc(tbl->num, sizeof(correction_element_t));
  if (!tbl->corr_table) {
    return -1;
  }
  for (i = 0; i < tbl->num; i++) {
    json_t *e = json_array_get(obj, i);
    json_t *cond_value_o, *correction_o;
    if (!e || !json_is_array(e) || json_array_size(e) != 2) {
      DEBUG("Could not get correction: %zu\n", i);
      free(tbl->corr_table);
      return -1;
    }
    cond_value_o = json_array_get(e, 0);
    correction_o = json_array_get(e, 1);
    if (!cond_value_o || !correction_o ||
        !json_is_number(cond_value_o) ||
        !json_is_number(correction_o)) {
      DEBUG("Invalid value in index: %zu\n", i);
      free(tbl->corr_table);
      return -1;
    }
    tbl->corr_table[i].cond_value = get_float(cond_value_o);
    tbl->corr_table[i].correction = get_float(correction_o);
  }
  return 0;
}

static int search_table(const char *table_name, sensor_correction_t *snr, size_t *idx)
{
  size_t i;

  for (i = 0; i < snr->num_tables; i++) {
    if (!strcmp(table_name, snr->tables[i].name)) {
      *idx = i;
      return 0;
    }
  }
  DEBUG("Could not get index for %s\n", value);
  return -1;
}

static int load_conditional_sensor_correction(json_t *obj, sensor_correction_t *snr)
{
  json_t *tmp;
  void *iter;
  size_t i;

  tmp = json_object_get(obj, "tables");
  if (!tmp) {
    DEBUG("Could not get tables\n");
    return -1;
  }
  snr->num_tables = json_object_size(tmp);
  if (!snr->num_tables || snr->num_tables > MAX_NUM_TABLES) {
    DEBUG("Unsupported number of tables: %zu\n", snr->num_tables);
    return -1;
  }
  for (i = 0, iter = json_object_iter(tmp);
      i < snr->num_tables && iter;
      i++, iter = json_object_iter_next(tmp, iter)) {
    const char *table_name = json_object_iter_key(iter);
    json_t *tbl_o = json_object_iter_value(iter);
    strncpy(snr->tables[i].name, table_name, sizeof(snr->tables[i].name) - 1);
    snr->tables[i].name[sizeof(snr->tables[i].name) - 1] = '\0';
    if (!tbl_o || !json_is_array(tbl_o)) {
      DEBUG("Could not get correction table for %s\n", table_name);
      return -1;
    }
    if (load_table(tbl_o, &snr->tables[i])) {
      DEBUG("Could not load correction table for %s\n", table_name);
      return -1;
    }
  }

  obj = json_object_get(obj, "condition");
  if (!obj) {
    DEBUG("Could not get condition\n");
    return -1;
  }

  tmp = json_object_get(obj, "default_table");
  if (!tmp || !json_is_string(tmp) ||
      search_table(json_string_value(tmp), snr, &snr->default_table)) {
    DEBUG("Could not get the default table!\n");
    return -1;
  }

  tmp = json_object_get(obj, "key");
  if (!tmp || !json_is_string(tmp)) {
    DEBUG("Could not get the key\n");
    return -1;
  }
  strncpy(snr->cond_key, json_string_value(tmp), sizeof(snr->cond_key) - 1);
  snr->cond_key[sizeof(snr->cond_key) - 1] = '\0';

  tmp = json_object_get(obj, "key_type");
  snr->cond_key_type = KEY_REGULAR;
  if (tmp && json_is_string(tmp)) {
    if (!strcmp("persistent", json_string_value(tmp))) {
      snr->cond_key_type = KEY_PERSISTENT;
    }
  }

  obj = json_object_get(obj, "value_map");
  if (!obj) {
    DEBUG("Could not get value_map\n");
    return -1;
  }
  snr->value_map_size = json_object_size(obj);
  if (!snr->value_map_size || snr->value_map_size > MAX_NUM_CONDITIONS) {
    DEBUG("Unsupported value map size: %zu\n", snr->value_map_size);
    return -1;
  }
  for (i = 0, iter = json_object_iter(obj);
      i < snr->value_map_size && iter;
      i++, iter = json_object_iter_next(obj, iter)) {
    const char *value_str = json_object_iter_key(iter);
    json_t *table_name = json_object_iter_value(iter);
    size_t idx;
    if (!value_str || !table_name || !json_is_string(table_name)) {
      DEBUG("Getting valuemap[%zu] failed\n", i);
      return -1;
    }
    if (search_table(json_string_value(table_name), snr, &idx)) {
      DEBUG("Getting index for valuemap[%zu] failed\n", i);
      return -1;
    }
    strncpy(snr->value_map[i].cond_value, value_str, MAX_VALUE_LEN);
    snr->value_map[i].table_idx = idx;
  }
  return 0;
}

static int load_sensor_correction(json_t *obj, sensor_correction_t *snr)
{
  json_t *tmp, *tmp2;
  int ret = -1;

  tmp = json_object_get(obj, "name");
  if (!tmp || !json_is_string(tmp)) {
    DEBUG("Getting sensor name failed!\n");
    return -1;
  }
  strncpy(snr->name, json_string_value(tmp), sizeof(snr->name) - 1);
  snr->name[sizeof(snr->name) - 1] = '\0';

  tmp = json_object_get(obj, "fru");
  if (!tmp || !json_is_number(tmp)) {
    DEBUG("Getting FRU failed!\n");
    return -1;
  }
  snr->fru = json_integer_value(tmp);

  tmp = json_object_get(obj, "id");
  if (!tmp || !json_is_number(tmp)) {
    DEBUG("Getting sensor ID failed!\n");
    return -1;
  }
  snr->id = json_integer_value(tmp);

  tmp = json_object_get(obj, "correction");
  if (!tmp) {
    DEBUG("Getting correction failed!\n");
    return -1;
  }

  tmp2 = json_object_get(tmp, "type");
  if (!tmp2 || !json_is_string(tmp2)) {
    DEBUG("Getting correction type failed!\n");
    return -1;
  }
  if (!strcmp(json_string_value(tmp2), "conditional_table")) {
    ret = load_conditional_sensor_correction(tmp, snr);
  } else {
    DEBUG("Unknown correction type: %s\n", json_string_value(tmp2));
  }
  return ret;
}

int sensor_correction_init(const char *file)
{
  json_t *conf, *tmp;
  json_error_t error;
  size_t i;

  conf = json_load_file(file, 0, &error);
  if (!conf) {
    return -1;
  }
  tmp = json_object_get(conf, "version");
  if (tmp && json_is_string(tmp)) {
    INFO("Loaded sensor correction configuration version: %s\n", 
        json_string_value(tmp));
  }
  tmp = json_object_get(conf, "sensors");
  if (!tmp || !json_is_array(tmp)) {
    DEBUG("Failed to get sensors");
    return -1;
  }
  g_sensors_count = json_array_size(tmp);
  if (!g_sensors_count) {
    DEBUG("No sensors found in configuration");
    return 0;
  }
  g_sensors = calloc(g_sensors_count, sizeof(sensor_correction_t));
  if (!g_sensors) {
    g_sensors_count = 0;
    DEBUG("Allocation failure!\n");
    return -1;
  }

  for (i = 0; i < g_sensors_count; i++) {
    json_t *s_o = json_array_get(tmp, i);
    if (!s_o) {
      DEBUG("Getting sensor[%zu] failed", i);
      goto bail;
    }
    if (load_sensor_correction(s_o, &g_sensors[i])) {
      DEBUG("Loading sensor correction for sensor %zu failed!\n", i);
      goto bail;
    }
  }
  json_decref(conf);
  return 0;
bail:
  free(g_sensors);
  json_decref(conf);
  g_sensors = NULL;
  g_sensors_count = 0;
  return -1;
}

int sensor_correction_apply(uint8_t fru, uint8_t sensor_id, float cond_value, float *sensor_reading)
{
  char value[MAX_VALUE_LEN] = {0};
  size_t table_idx = 0;
  correction_table_t *table;
  size_t i;
  float correction;
  unsigned int flags;

  sensor_correction_t *snr = get_correction(fru, sensor_id);
  if (!snr) {
    /* No correction defined for this sensor. Return success without
     * manipulating it */
    return 0;
  }
  flags = snr->cond_key_type == KEY_PERSISTENT ? KV_FPERSIST : 0;
  if (kv_get(snr->cond_key, value, NULL, flags) ||
      get_table(snr->value_map, snr->value_map_size, value, &table_idx)) {
    table_idx = snr->default_table;
  }
  table = &snr->tables[table_idx];
  correction = table->corr_table[0].correction;
  for (i = 0; i < table->num; i++) {
    if (cond_value < table->corr_table[i].cond_value) {
      break;
    }
    correction = table->corr_table[i].correction;
  }
  *sensor_reading = *sensor_reading - correction;
  return 0;
}

#ifdef __TEST__
int main(int argc, char *argv[])
{
  float value;
  float orig_value;
  float cond_value;
  uint8_t fru = 1;
  uint8_t id  = 163;

  if (argc < 4) {
    printf("USAGE: %s JSON_FILE COND_VALUE SENSOR_VLAUE [SENSOR_ID FRU_ID]\n", argv[0]);
    return -1;
  }
  if (sensor_correction_init(argv[1])) {
    printf("Failed to load: %s\n", argv[1]);
    return -1;
  }

  cond_value = atof(argv[2]);
  orig_value = value = atof(argv[3]);
  if (argc >= 5) {
    id = atoi(argv[4]);
  }
  if (argc >= 6) {
    fru = atoi(argv[5]);
  }

  if (sensor_correction_apply(fru, id, cond_value, &value)) {
    printf("sensor correction failed!\n");
    return -1;
  }
  printf("sensor value (%4.3f) post correction: %4.3f\n", orig_value, value);
  return 0;
}
#endif
