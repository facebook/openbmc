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
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <errno.h>
#include <unistd.h>
#include <syslog.h>
#include <openbmc/kv.h>
#include "pal.h"
#include "pal_sensors.h"

size_t pal_pwm_cnt = 4;
size_t pal_tach_cnt = 8;
const char pal_pwm_list[] = "0..3";
const char pal_tach_list[] = "0..7";

#define LARGEST_DEVICE_NAME 120

#define FAN_TACH "fan%d_input"
#define FAN_PWM "pwm%d"
#define FAN_DIR "/sys/class/hwmon/hwmon0"
#define PWM_UNIT_MAX 255

#define MAX_SENSOR_NUM FRU_SENSOR_MAX+1
#define MAX_SENSOR_THRESHOLD 8

#define READING_NA -2

/*
 * List of sensors to be monitored
 */
const uint8_t fru_sensor_list[] = {
  FRU_SENSOR_FAN0_TACH,
  FRU_SENSOR_FAN2_TACH,
};

/*
 * The name of all sensors, each should correspond to the enumeration.
 */
const char* fru_sensor_name[] = {
  "FRU_FAN0_TACH",
  "FRU_FAN2_TACH",
};

float fru_sensor_threshold[MAX_SENSOR_NUM][MAX_SENSOR_THRESHOLD + 1] = {0};

size_t fru_sensor_cnt = sizeof(fru_sensor_list)/sizeof(uint8_t);

static int read_device(const char *device, int *value)
{
  FILE *fp;
  int rc;

  fp = fopen(device, "r");
  if (!fp) {
    int err = errno;
#ifdef DEBUG
    syslog(LOG_WARNING, "failed to open device %s", device);
#endif
    return err;
  }

  rc = fscanf(fp, "%d", value);
  fclose(fp);
  if (rc != 1) {
#ifdef DEBUG
    syslog(LOG_WARNING, "failed to read device %s", device);
#endif
    return ENOENT;
  } else {
    return 0;
  }
}

static int read_device_float(const char *device, float *value)
{
  FILE *fp;
  int rc;
  char tmp[10];

  fp = fopen(device, "r");
  if (!fp) {
    int err = errno;
#ifdef DEBUG
    syslog(LOG_WARNING, "failed to open device %s", device);
#endif
    return err;
  }

  rc = fscanf(fp, "%s", tmp);
  fclose(fp);

  if (rc != 1) {
#ifdef DEBUG
    syslog(LOG_WARNING, "failed to read device %s", device);
#endif
    return ENOENT;
  }

  *value = atof(tmp);

  return 0;
}

static int write_device(const char *device, const char *value)
{
  FILE *fp;
  int rc;

  fp = fopen(device, "w");
  if (!fp) {
    int err = errno;
#ifdef DEBUG
    syslog(LOG_WARNING, "failed to open device for write %s", device);
#endif
    return err;
  }

  rc = fputs(value, fp);
  fclose(fp);

  if (rc < 0) {
#ifdef DEBUG
    syslog(LOG_WARNING, "failed to write device %s", device);
#endif
    return ENOENT;
  } else {
    return 0;
  }
}

static int read_fan_value(const int fan, const char *device, int *value)
{
  char device_name[LARGEST_DEVICE_NAME];
  char full_name[LARGEST_DEVICE_NAME];

  snprintf(device_name, LARGEST_DEVICE_NAME, device, fan);
  snprintf(full_name, LARGEST_DEVICE_NAME, "%s/%s", FAN_DIR, device_name);
  return read_device(full_name, value);
}

static int read_fan_value_f(const int fan, const char *device, float *value)
{
  char device_name[LARGEST_DEVICE_NAME];
  char full_name[LARGEST_DEVICE_NAME];
  int ret;

  snprintf(device_name, LARGEST_DEVICE_NAME, device, fan);
  snprintf(full_name, LARGEST_DEVICE_NAME, "%s/%s", FAN_DIR, device_name);
  ret = read_device_float(full_name, value);
  if (*value < 500 || *value > fru_sensor_threshold[FRU_SENSOR_FAN0_TACH][UCR_THRESH]) {
    sleep(2);
    ret = read_device_float(full_name, value);
  }

  return ret;
}

static int write_fan_value(const int fan, const char *device, const int value)
{
  char full_name[LARGEST_DEVICE_NAME];
  char device_name[LARGEST_DEVICE_NAME];
  char output_value[LARGEST_DEVICE_NAME];

  snprintf(device_name, LARGEST_DEVICE_NAME, device, fan);
  snprintf(full_name, LARGEST_DEVICE_NAME, "%s/%s", FAN_DIR, device_name);
  snprintf(output_value, LARGEST_DEVICE_NAME, "%d", value);
  return write_device(full_name, output_value);
}

int pal_set_fan_speed(uint8_t fan, uint8_t pwm)
{
  int unit;
  int ret;

  if (fan >= pal_pwm_cnt) {
    syslog(LOG_WARNING, "%s: fan number is invalid - %d", __func__, fan);
    return -1;
  }

  unit = pwm * PWM_UNIT_MAX / 100;

  ret = write_fan_value(fan/2+1, FAN_PWM, unit);
  if (ret < 0) {
    syslog(LOG_WARNING, "%s: write_fan_value failed", __func__);
    return -1;
  }

  return 0;
}

int pal_get_fan_speed(uint8_t fan, int *rpm)
{
  if (fan >= pal_tach_cnt) {
    syslog(LOG_WARNING, "%s: invalid fan#:%d", __func__, fan);
    return -1;
  }

  return read_fan_value(fan+1, FAN_TACH, rpm);
}

int pal_get_fan_name(uint8_t num, char *name)
{
  if (num >= pal_tach_cnt) {
    syslog(LOG_WARNING, "%s: invalid fan#:%d", __func__, num);
    return -1;
  }

  sprintf(name, "Fan %d", num);

  return 0;
}

int pal_get_pwm_value(uint8_t fan_num, uint8_t *value)
{
  int val = 0;

  if (fan_num >= pal_pwm_cnt) {
    syslog(LOG_WARNING, "%s: fan number is invalid - %d", __func__, fan_num);
    return -1;
  }

  if (read_fan_value(fan_num/2+1, FAN_PWM, &val)) {
    syslog(LOG_WARNING, "%s: read pwm%d failed", __func__, fan_num);
    return -1;
  }

  *value = 100 * val / PWM_UNIT_MAX;

  return 0;
}

int pal_get_fru_sensor_list(uint8_t fru, uint8_t **sensor_list, int *cnt)
{
  if (fru != FRU_BASE) {
    *sensor_list = NULL;
    *cnt = 0;
    return -1;
  }

  *sensor_list = (uint8_t *) fru_sensor_list;
  *cnt = fru_sensor_cnt;

  return 0;
}

static void sensor_thresh_array_init()
{
  fru_sensor_threshold[FRU_SENSOR_FAN0_TACH][UNC_THRESH] = 8500;
  fru_sensor_threshold[FRU_SENSOR_FAN2_TACH][UNC_THRESH] = 8500;
  fru_sensor_threshold[FRU_SENSOR_FAN0_TACH][UCR_THRESH] = 11500;
  fru_sensor_threshold[FRU_SENSOR_FAN2_TACH][UCR_THRESH] = 11500;
  fru_sensor_threshold[FRU_SENSOR_FAN0_TACH][LCR_THRESH] = 500;
  fru_sensor_threshold[FRU_SENSOR_FAN2_TACH][LCR_THRESH] = 500;

  return;
}

int pal_get_sensor_threshold(uint8_t fru, uint8_t sensor_num, uint8_t thresh, void *value)
{
  static bool is_thresh_init = false;
  float *val = (float*) value;

  if (!is_thresh_init) {
    sensor_thresh_array_init();
    is_thresh_init = true;
  }

  if (fru != FRU_BASE || sensor_num >= FRU_SENSOR_MAX)
    return -1;

  *val = fru_sensor_threshold[sensor_num][thresh];

  return 0;
}

int pal_get_sensor_name(uint8_t fru, uint8_t sensor_num, char *name)
{
  if (fru != FRU_BASE)
    return -1;

  if (sensor_num >= FRU_SENSOR_MAX)
    sprintf(name, "INVAILD SENSOR");
  else
    sprintf(name, fru_sensor_name[sensor_num]);

  return 0;
}

int pal_sensor_read_raw(uint8_t fru, uint8_t sensor_num, void *value)
{
  char key[MAX_KEY_LEN] = {0};
  char str[MAX_VALUE_LEN] = {0};
  int ret;

  if (fru != FRU_BASE)
    return -1;

  switch(sensor_num) {
    // Fan Sensors
    case FRU_SENSOR_FAN0_TACH:
      ret = read_fan_value_f(FAN_0, FAN_TACH, (float*)value);
      break;
    case FRU_SENSOR_FAN2_TACH:
      ret = read_fan_value_f(FAN_2, FAN_TACH, (float*)value);
      break;
    default:
      ret = READING_NA;
  }

  if (ret) {
    if (ret == READING_NA) {
      strcpy(str, "NA");
    } else {
      return ret;
    }
  } else {
    sprintf(str, "%.2f",*((float*)value));
  }

  sprintf(key, "fru_sensor%d", sensor_num);
  if(kv_set(key, str, 0, 0) < 0) {
#ifdef DEBUG
     syslog(LOG_WARNING, "%s: cache_set key = %s, str = %s failed.", __func__, key, str);
#endif
    return -1;
  }

  return 0;
}

