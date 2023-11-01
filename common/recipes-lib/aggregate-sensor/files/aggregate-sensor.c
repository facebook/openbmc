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

#include <stdio.h>
#include <stdlib.h>
#include <syslog.h>
#include <string.h>
#include <stdbool.h>
#include <assert.h>
#include <dirent.h>
#include <openbmc/obmc-pal.h>
#include <openbmc/pal_sensors.h>
#include <openbmc/kv.h>
#include <jansson.h>
#include "aggregate-sensor-internal.h"

#define PRINT(slog, fmt, ...) do { \
  if (slog) { \
    syslog(LOG_INFO, fmt, ##__VA_ARGS__); \
  } else { \
    printf(fmt, ##__VA_ARGS__); \
  } \
} while(0)

#define DEFAULT_CONF_FILE_PATH "/etc/aggregate-sensors.d"

size_t g_sensors_count = 0;
aggregate_sensor_t *g_sensors = NULL;

int get_sensor_value(void *state, float *value)
{
  struct sensor_src *snr = (struct sensor_src *)state;
  assert(snr);
  assert(value);
  return sensor_cache_read(snr->fru, snr->id, value);
}

int
aggregate_sensor_count(size_t *count)
{
  *count = g_sensors_count;
  return 0;
}

int get_key(cond_key_type type, const char *cond_key, char *cond_value)
{
  int ret;
  switch(type) {
    case KEY_REGULAR:
      ret = kv_get((char *)cond_key, cond_value, NULL, 0);
      break;
    case KEY_PERSISTENT:
      ret = kv_get((char *)cond_key, cond_value, NULL, KV_FPERSIST);
      break;
    case KEY_PATH:
      {
        FILE *fp = fopen(cond_key, "r");
        if (!fp) {
          return -1;
        }
        ret = (int)fread(cond_value, 1, MAX_STRING_SIZE, fp);
        fclose(fp);
        if (ret <= 0) {
          return -1;
        }
        ret = 0;
      }
      break;
    default:
      ret = -1;
  }
  return ret;
}


int
aggregate_sensor_read(size_t index, float *value)
{
  char cond_value[MAX_VALUE_LEN] = {0};
  size_t i;
  int f_idx = -1;
  aggregate_sensor_t *snr;
  if (index >= g_sensors_count) {
    return -1;
  }
  if (pal_is_aggregate_snr_valid(index) == false) {
    return -1;
  }
  snr = &g_sensors[index];
  if (snr->conditional) {
    if (!get_key(snr->cond_type, snr->cond_key, cond_value)) {
      for (i = 0; i < snr->value_map_size; i++) {
        if (!strncmp(snr->value_map[i].condition_value, cond_value,
            sizeof(snr->value_map[i].condition_value))) {
          f_idx = snr->value_map[i].formula_index;
          break;
        }
      }
    }
    if (f_idx == -1) {
      if (snr->default_expression_idx == -1) {
        return -1;
      }
      f_idx = snr->default_expression_idx;
    }
  } else {
    f_idx = 0;
  }
  return expression_evaluate(snr->expressions[f_idx], value);
}

int
aggregate_sensor_threshold(size_t index, thresh_sensor_t *thresh)
{
  if (index >= g_sensors_count) {
    return -1;
  }
  
  *thresh = g_sensors[index].sensor;
  return 0;
}

int
aggregate_sensor_name(size_t index, char *name)
{
  if (index >= g_sensors_count) {
    return -1;
  }
  strcpy(name, g_sensors[index].sensor.name);
  return 0;
}

int
aggregate_sensor_units(size_t index, char *units)
{
  if (index >= g_sensors_count) {
    return -1;
  }
  strcpy(units, g_sensors[index].sensor.units);
  return 0;
}

int
aggregate_sensor_poll_interval(size_t index, uint32_t *poll_interval)
{
  if (index >= g_sensors_count) {
    return -1;
  }
  *poll_interval = g_sensors[index].sensor.poll_interval;
  return 0;
}

int
aggregate_sensor_init(const char *conf_file_path)
{
  if (!conf_file_path) {
    conf_file_path = DEFAULT_CONF_FILE_PATH;
  }
  // Each count will clear previous loads.
  if (g_sensors_count > 0) {
    free(g_sensors);
    g_sensors_count = 0;
    g_sensors = NULL;
  }
  DIR *cdir = opendir(conf_file_path);
  if (!cdir) {
    // Assume this is a regular file.
    return load_aggregate_conf(conf_file_path);
  }
  int ret = -1;
  for (struct dirent *dp = readdir(cdir); dp != NULL; dp = readdir(cdir)) {
    if (dp->d_type != DT_DIR) {
      // Ignore subdirs and the . and .. directories as well
      char fpath[sizeof(dp->d_name) * 2];
      snprintf(fpath, sizeof(fpath), "%s/%s", conf_file_path, dp->d_name);
      if (load_aggregate_conf(fpath)) {
        DEBUG("Loading configuration %s failed\n", fpath);
        ret = -1;
        break;
      }
      // At least one configuration is loaded.
      ret = 0;
    }
  }
  closedir(cdir);
  return ret;
}

void
aggregate_sensor_conf_print(bool slog)
{
  size_t i;
  PRINT(slog, "Found: %zu sensors\n", g_sensors_count);
  for (i = 0; i < g_sensors_count; i++) {
    aggregate_sensor_t *snr = &g_sensors[i];
    size_t j;
    PRINT(slog, "Name: %s\n", snr->sensor.name);
    PRINT(slog, "Unit: %s\n", snr->sensor.units);
    PRINT(slog, "idx: %zu\n", snr->idx);
    PRINT(slog, "cond_key: %s\n", snr->cond_key);
    PRINT(slog, "Expressions: (%zu)\n", snr->num_expressions);
    for (j = 0; j < snr->num_expressions; j++) {
      expression_print(snr->expressions[j]);
      printf("\n");
    }
    PRINT(slog, "Default expression IDX: %d\n", snr->default_expression_idx);
    PRINT(slog, "Conditions:\n");
    for (j = 0; j < snr->value_map_size; j++) {
      PRINT(slog, "%s => %zu\n", 
          snr->value_map[j].condition_value,
          snr->value_map[j].formula_index);
    }
    PRINT(slog, "Thresholds: ");
    if (snr->sensor.flag & GETMASK(UCR_THRESH))
      PRINT(slog, "UCR: %5.5f ", snr->sensor.ucr_thresh);
    else
      PRINT(slog, "UCR: NA ");
    if (snr->sensor.flag & GETMASK(UNC_THRESH))
      PRINT(slog, "UNC: %5.5f ", snr->sensor.unc_thresh);
    else
      PRINT(slog, "UNC: NA ");
    if (snr->sensor.flag & GETMASK(UNR_THRESH))
      PRINT(slog, "UNR: %5.5f ", snr->sensor.unr_thresh);
    else
      PRINT(slog, "UNR: NA ");
    if (snr->sensor.flag & GETMASK(LCR_THRESH))
      PRINT(slog, "LCR: %5.5f ", snr->sensor.lcr_thresh);
    else
      PRINT(slog, "LCR: NA ");
    if (snr->sensor.flag & GETMASK(LNC_THRESH))
      PRINT(slog, "LNC: %5.5f ", snr->sensor.lnc_thresh);
    else
      PRINT(slog, "LNC: NA ");
    if (snr->sensor.flag & GETMASK(LNR_THRESH))
      PRINT(slog, "LNR: %5.5f ", snr->sensor.lnr_thresh);
    else
      PRINT(slog, "UCR: NA ");
    PRINT(slog, "\n");

    PRINT(slog, "-------------------------\n");
  }
}

