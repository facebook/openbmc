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
#include <openbmc/libgpio.h>
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
#define ADC_DIR "/sys/class/hwmon/hwmon1"
#define ADC_VALUE "in%d_input"

#define MAX_SENSOR_NUM FRU_SENSOR_MAX+1
#define MAX_SENSOR_THRESHOLD 8

#define READING_NA -2

/*
 * List of sensors to be monitored
 */
const uint8_t fru_sensor_list[] = {
  FRU_SENSOR_FAN0_TACH,
  FRU_SENSOR_FAN1_TACH,
  FRU_SENSOR_FAN2_TACH,
  FRU_SENSOR_FAN3_TACH,
  FRU_SENSOR_FAN4_TACH,
  FRU_SENSOR_FAN5_TACH,
  FRU_SENSOR_FAN6_TACH,
  FRU_SENSOR_FAN7_TACH,
  FRU_SENSOR_P12V_AUX,
  FRU_SENSOR_P3V3_STBY,
  FRU_SENSOR_P5V_STBY,
  FRU_SENSOR_P12V_1,
  FRU_SENSOR_P12V_2,
  FRU_SENSOR_P3V3,
  FRU_SENSOR_P3V_BAT,
};

/*
 * The name of all sensors, each should correspond to the enumeration.
 */
const char* fru_sensor_name[] = {
  "FRU_FAN0_TACH",
  "FRU_FAN1_TACH",
  "FRU_FAN2_TACH",
  "FRU_FAN3_TACH",
  "FRU_FAN4_TACH",
  "FRU_FAN5_TACH",
  "FRU_FAN6_TACH",
  "FRU_FAN7_TACH",
  "FRU_SENSOR_P12V_AUX",
  "FRU_SENSOR_P3V3_STBY",
  "FRU_SENSOR_P5V_STBY",
  "FRU_SENSOR_P12V_1",
  "FRU_SENSOR_P12V_2",
  "FRU_SENSOR_P3V3",
  "FRU_SENSOR_P3V_BAT",
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

static int read_adc_value(const int adc, const char *device, float r1, float r2, float *value)
{
  char full_name[LARGEST_DEVICE_NAME];
  char device_name[LARGEST_DEVICE_NAME];
  float val;
  int ret;

  snprintf(device_name, LARGEST_DEVICE_NAME, device, adc);
  snprintf(full_name, LARGEST_DEVICE_NAME, "%s/%s", ADC_DIR, device_name);
  ret = read_device_float(full_name, &val);
  if (ret != 0)
    return ret;

  *value = val / 1000.0 * (r1 + r2) / r2 ;
  return 0;
}

static int read_battery_status(float *value)
{
  int ret = -1;
  gpio_desc_t *gp_batt = gpio_open_by_shadow("BATTERY_DETECT");
  if (!gp_batt) {
    return -1;
  }
  if (gpio_set_value(gp_batt, GPIO_VALUE_LOW)) {
    goto bail;
  }
  msleep(10);
  ret = read_adc_value(ADC_8, ADC_VALUE, 200.0, 100.0, value);
  gpio_set_value(gp_batt, GPIO_VALUE_HIGH);
bail:
  gpio_close(gp_batt);
  return ret;
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
  // Fan Sensors
  fru_sensor_threshold[FRU_SENSOR_FAN0_TACH][UNC_THRESH] = 8500;
  fru_sensor_threshold[FRU_SENSOR_FAN0_TACH][UCR_THRESH] = 11500;
  fru_sensor_threshold[FRU_SENSOR_FAN0_TACH][LCR_THRESH] = 500;
  fru_sensor_threshold[FRU_SENSOR_FAN1_TACH][UNC_THRESH] = 8500;
  fru_sensor_threshold[FRU_SENSOR_FAN1_TACH][UCR_THRESH] = 11500;
  fru_sensor_threshold[FRU_SENSOR_FAN1_TACH][LCR_THRESH] = 500;
  fru_sensor_threshold[FRU_SENSOR_FAN2_TACH][UNC_THRESH] = 8500;
  fru_sensor_threshold[FRU_SENSOR_FAN2_TACH][UCR_THRESH] = 11500;
  fru_sensor_threshold[FRU_SENSOR_FAN2_TACH][LCR_THRESH] = 500;
  fru_sensor_threshold[FRU_SENSOR_FAN3_TACH][UNC_THRESH] = 8500;
  fru_sensor_threshold[FRU_SENSOR_FAN3_TACH][UCR_THRESH] = 11500;
  fru_sensor_threshold[FRU_SENSOR_FAN3_TACH][LCR_THRESH] = 500;
  fru_sensor_threshold[FRU_SENSOR_FAN4_TACH][UNC_THRESH] = 8500;
  fru_sensor_threshold[FRU_SENSOR_FAN4_TACH][UCR_THRESH] = 11500;
  fru_sensor_threshold[FRU_SENSOR_FAN4_TACH][LCR_THRESH] = 500;
  fru_sensor_threshold[FRU_SENSOR_FAN5_TACH][UNC_THRESH] = 8500;
  fru_sensor_threshold[FRU_SENSOR_FAN5_TACH][UCR_THRESH] = 11500;
  fru_sensor_threshold[FRU_SENSOR_FAN5_TACH][LCR_THRESH] = 500;
  fru_sensor_threshold[FRU_SENSOR_FAN6_TACH][UNC_THRESH] = 8500;
  fru_sensor_threshold[FRU_SENSOR_FAN6_TACH][UCR_THRESH] = 11500;
  fru_sensor_threshold[FRU_SENSOR_FAN6_TACH][LCR_THRESH] = 500;
  fru_sensor_threshold[FRU_SENSOR_FAN7_TACH][UNC_THRESH] = 8500;
  fru_sensor_threshold[FRU_SENSOR_FAN7_TACH][UCR_THRESH] = 11500;
  fru_sensor_threshold[FRU_SENSOR_FAN7_TACH][LCR_THRESH] = 500;

  // ADC Sensors
  fru_sensor_threshold[FRU_SENSOR_P12V_AUX][UCR_THRESH] = 13.23;
  fru_sensor_threshold[FRU_SENSOR_P12V_AUX][LCR_THRESH] = 10.773;
  fru_sensor_threshold[FRU_SENSOR_P3V3_STBY][UCR_THRESH] = 3.621;
  fru_sensor_threshold[FRU_SENSOR_P3V3_STBY][LCR_THRESH] = 2.975;
  fru_sensor_threshold[FRU_SENSOR_P5V_STBY][UCR_THRESH] = 5.486;
  fru_sensor_threshold[FRU_SENSOR_P5V_STBY][LCR_THRESH] = 4.524;
  fru_sensor_threshold[FRU_SENSOR_P12V_1][UCR_THRESH] = 13.23;
  fru_sensor_threshold[FRU_SENSOR_P12V_1][LCR_THRESH] = 10.773;
  fru_sensor_threshold[FRU_SENSOR_P12V_2][UCR_THRESH] = 13.23;
  fru_sensor_threshold[FRU_SENSOR_P12V_2][LCR_THRESH] = 10.773;
  fru_sensor_threshold[FRU_SENSOR_P3V3][UCR_THRESH] = 3.621;
  fru_sensor_threshold[FRU_SENSOR_P3V3][LCR_THRESH] = 2.975;
  fru_sensor_threshold[FRU_SENSOR_P3V_BAT][UCR_THRESH] = 3.738;
  fru_sensor_threshold[FRU_SENSOR_P3V_BAT][LCR_THRESH] = 2.73;

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

int pal_get_sensor_units(uint8_t fru, uint8_t sensor_num, char *units)
{
  if (fru != FRU_BASE)
    return -1;

  switch(sensor_num) {
    case FRU_SENSOR_FAN0_TACH:
    case FRU_SENSOR_FAN1_TACH:
    case FRU_SENSOR_FAN2_TACH:
    case FRU_SENSOR_FAN3_TACH:
    case FRU_SENSOR_FAN4_TACH:
    case FRU_SENSOR_FAN5_TACH:
    case FRU_SENSOR_FAN6_TACH:
    case FRU_SENSOR_FAN7_TACH:
      sprintf(units, "RPM");
      break;
    case FRU_SENSOR_P12V_AUX:
    case FRU_SENSOR_P3V3_STBY:
    case FRU_SENSOR_P5V_STBY:
    case FRU_SENSOR_P12V_1:
    case FRU_SENSOR_P12V_2:
    case FRU_SENSOR_P3V3:
    case FRU_SENSOR_P3V_BAT:
      sprintf(units, "Volts");
      break;
    default:
      return -1;
  }
  return 0;
}

int pal_get_sensor_poll_interval(uint8_t fru, uint8_t sensor_num, uint32_t *value)
{
  if (fru != FRU_BASE)
    return -1;

  // default poll interval
  *value = 2;

  if (sensor_num == FRU_SENSOR_P3V_BAT)
    *value = 3600;

  return PAL_EOK;
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
    case FRU_SENSOR_FAN1_TACH:
      ret = read_fan_value_f(FAN_1, FAN_TACH, (float*)value);
      break;
    case FRU_SENSOR_FAN2_TACH:
      ret = read_fan_value_f(FAN_2, FAN_TACH, (float*)value);
      break;
    case FRU_SENSOR_FAN3_TACH:
      ret = read_fan_value_f(FAN_3, FAN_TACH, (float*)value);
      break;
    case FRU_SENSOR_FAN4_TACH:
      ret = read_fan_value_f(FAN_4, FAN_TACH, (float*)value);
      break;
    case FRU_SENSOR_FAN5_TACH:
      ret = read_fan_value_f(FAN_5, FAN_TACH, (float*)value);
      break;
    case FRU_SENSOR_FAN6_TACH:
      ret = read_fan_value_f(FAN_6, FAN_TACH, (float*)value);
      break;
    case FRU_SENSOR_FAN7_TACH:
      ret = read_fan_value_f(FAN_7, FAN_TACH, (float*)value);
      break;
    // ADC Sensors (Resistance unit = 1K)
    case FRU_SENSOR_P12V_AUX:
      ret = read_adc_value(ADC_0, ADC_VALUE, 15.8, 2.0, (float*)value);
      break;
    case FRU_SENSOR_P3V3_STBY:
      ret = read_adc_value(ADC_1, ADC_VALUE, 2.87, 2.0, (float*)value);
      break;
    case FRU_SENSOR_P5V_STBY:
      ret = read_adc_value(ADC_2, ADC_VALUE, 5.36, 2.0, (float*)value);
      break;
    case FRU_SENSOR_P12V_1:
      ret = read_adc_value(ADC_3, ADC_VALUE, 15.8, 2.0, (float*)value);
      break;
    case FRU_SENSOR_P12V_2:
      ret = read_adc_value(ADC_4, ADC_VALUE, 15.8, 2.0, (float*)value);
      break;
    case FRU_SENSOR_P3V3:
      ret = read_adc_value(ADC_7, ADC_VALUE, 2.87, 2.0, (float*)value);
      break;
    case FRU_SENSOR_P3V_BAT:
      ret = read_battery_status((float*)value);
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

