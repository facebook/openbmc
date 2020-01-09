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
#include <openbmc/obmc-sensors.h>
#include <switchtec/switchtec.h>
#include "pal.h"
#include "pal_calibration.h"

size_t pal_pwm_cnt = 4;
size_t pal_tach_cnt = 8;
const char pal_pwm_list[] = "0..3";
const char pal_tach_list[] = "0..7";

#define TLA2024_DIR(bus, addr, index) \
  "/sys/bus/i2c/drivers/tla2024/"#bus"-00"#addr"/iio:device"#index"/%s"
#define TLA2024_AIN     "in_voltage%d_raw"
#define TLA2024_FSR     "scale"
#define ADC_1_DIR       TLA2024_DIR(16, 48, 1)
#define ADC_2_DIR       TLA2024_DIR(16, 49, 2)

#define MAX_SENSOR_NUM FBEP_SENSOR_MAX
#define MAX_SENSOR_THRESHOLD 8

#define READING_NA -2

/*
 * List of sensors to be monitored
 */
const uint8_t mb_sensor_list[] = {
  MB_FAN0_TACH_I,
  MB_FAN0_TACH_O,
  MB_FAN0_VOLT,
  MB_FAN0_CURR,
  MB_FAN1_TACH_I,
  MB_FAN1_TACH_O,
  MB_FAN1_VOLT,
  MB_FAN1_CURR,
  MB_FAN2_TACH_I,
  MB_FAN2_TACH_O,
  MB_FAN2_VOLT,
  MB_FAN2_CURR,
  MB_FAN3_TACH_I,
  MB_FAN3_TACH_O,
  MB_FAN3_VOLT,
  MB_FAN3_CURR,
  MB_ADC_P12V_AUX,
  MB_ADC_P3V3_STBY,
  MB_ADC_P5V_STBY,
  MB_ADC_P12V_1,
  MB_ADC_P12V_2,
  MB_ADC_P3V3,
  MB_ADC_P3V_BAT,
  MB_SENSOR_GPU_INLET,
  MB_SENSOR_GPU_INLET_REMOTE,
  MB_SENSOR_GPU_OUTLET,
  MB_SENSOR_GPU_OUTLET_REMOTE,
  MB_SENSOR_PAX01_THERM,
  MB_SENSOR_PAX0_THERM_REMOTE,
  MB_SENSOR_PAX1_THERM_REMOTE,
  MB_SENSOR_PAX23_THERM,
  MB_SENSOR_PAX2_THERM_REMOTE,
  MB_SENSOR_PAX3_THERM_REMOTE,
  MB_SWITCH_PAX0_DIE_TEMP,
  MB_SWITCH_PAX1_DIE_TEMP,
  MB_SWITCH_PAX2_DIE_TEMP,
  MB_SWITCH_PAX3_DIE_TEMP,
  MB_VR_P0V8_VDD0_VIN,
  MB_VR_P0V8_VDD0_VOUT,
  MB_VR_P0V8_VDD0_CURR,
  MB_VR_P0V8_VDD0_TEMP,
  MB_VR_P0V8_VDD1_VIN,
  MB_VR_P0V8_VDD1_VOUT,
  MB_VR_P0V8_VDD1_CURR,
  MB_VR_P0V8_VDD1_TEMP,
  MB_VR_P0V8_VDD2_VIN,
  MB_VR_P0V8_VDD2_VOUT,
  MB_VR_P0V8_VDD2_CURR,
  MB_VR_P0V8_VDD2_TEMP,
  MB_VR_P0V8_VDD3_VIN,
  MB_VR_P0V8_VDD3_VOUT,
  MB_VR_P0V8_VDD3_CURR,
  MB_VR_P0V8_VDD3_TEMP,
  MB_VR_P1V0_AVD0_VIN,
  MB_VR_P1V0_AVD0_VOUT,
  MB_VR_P1V0_AVD0_CURR,
  MB_VR_P1V0_AVD0_TEMP,
  MB_VR_P1V0_AVD1_VIN,
  MB_VR_P1V0_AVD1_VOUT,
  MB_VR_P1V0_AVD1_CURR,
  MB_VR_P1V0_AVD1_TEMP,
  MB_VR_P1V0_AVD2_VIN,
  MB_VR_P1V0_AVD2_VOUT,
  MB_VR_P1V0_AVD2_CURR,
  MB_VR_P1V0_AVD2_TEMP,
  MB_VR_P1V0_AVD3_VIN,
  MB_VR_P1V0_AVD3_VOUT,
  MB_VR_P1V0_AVD3_CURR,
  MB_VR_P1V0_AVD3_TEMP
};

const uint8_t pdb_sensor_list[] = {
  PDB_HSC_P12V_1_VIN,
  PDB_HSC_P12V_1_VOUT,
  PDB_HSC_P12V_1_CURR,
  PDB_HSC_P12V_1_PWR,
  PDB_HSC_P12V_2_VIN,
  PDB_HSC_P12V_2_VOUT,
  PDB_HSC_P12V_2_CURR,
  PDB_HSC_P12V_2_PWR,
  PDB_HSC_P12V_AUX_VIN,
  PDB_HSC_P12V_AUX_VOUT,
  PDB_HSC_P12V_AUX_CURR,
  PDB_HSC_P12V_AUX_PWR,
  PDB_ADC_1_VICOR0_TEMP,
  PDB_ADC_1_VICOR1_TEMP,
  PDB_ADC_1_VICOR2_TEMP,
  PDB_ADC_1_VICOR3_TEMP,
  PDB_ADC_2_VICOR0_TEMP,
  PDB_ADC_2_VICOR1_TEMP,
  PDB_ADC_2_VICOR2_TEMP,
  PDB_ADC_2_VICOR3_TEMP,
  PDB_SENSOR_OUTLET_TEMP,
  PDB_SENSOR_OUTLET_TEMP_REMOTE
};

/*
 * The name of all sensors, each should correspond to the enumeration.
 */
const char* sensors_name[] = {
  "MB_FAN0_TACH_I",
  "MB_FAN0_TACH_O",
  "MB_FAN0_VOLT",
  "MB_FAN0_CURR",
  "MB_FAN1_TACH_I",
  "MB_FAN1_TACH_O",
  "MB_FAN1_VOLT",
  "MB_FAN1_CURR",
  "MB_FAN2_TACH_I",
  "MB_FAN2_TACH_O",
  "MB_FAN2_VOLT",
  "MB_FAN2_CURR",
  "MB_FAN3_TACH_I",
  "MB_FAN3_TACH_O",
  "MB_FAN3_VOLT",
  "MB_FAN3_CURR",
  "MB_ADC_P12V_AUX",
  "MB_ADC_P3V3_STBY",
  "MB_ADC_P5V_STBY",
  "MB_ADC_P12V_1",
  "MB_ADC_P12V_2",
  "MB_ADC_P3V3",
  "MB_ADC_P3V_BAT",
  "MB_SENSOR_GPU_INLET",
  "MB_SENSOR_GPU_INLET_REMOTE",
  "MB_SENSOR_GPU_OUTLET",
  "MB_SENSOR_GPU_OUTLET_REMOTE",
  "MB_SENSOR_PAX01_THERM",
  "MB_SENSOR_PAX0_THERM_REMOTE",
  "MB_SENSOR_PAX1_THERM_REMOTE",
  "MB_SENSOR_PAX23_THERM",
  "MB_SENSOR_PAX2_THERM_REMOTE",
  "MB_SENSOR_PAX3_THERM_REMOTE",
  "MB_SWITCH_PAX0_DIE_TEMP",
  "MB_SWITCH_PAX1_DIE_TEMP",
  "MB_SWITCH_PAX2_DIE_TEMP",
  "MB_SWITCH_PAX3_DIE_TEMP",
  "MB_VR_P0V8_VDD0_VIN",
  "MB_VR_P0V8_VDD0_VOUT",
  "MB_VR_P0V8_VDD0_CURR",
  "MB_VR_P0V8_VDD0_TEMP",
  "MB_VR_P0V8_VDD1_VIN",
  "MB_VR_P0V8_VDD1_VOUT",
  "MB_VR_P0V8_VDD1_CURR",
  "MB_VR_P0V8_VDD1_TEMP",
  "MB_VR_P0V8_VDD2_VIN",
  "MB_VR_P0V8_VDD2_VOUT",
  "MB_VR_P0V8_VDD2_CURR",
  "MB_VR_P0V8_VDD2_TEMP",
  "MB_VR_P0V8_VDD3_VIN",
  "MB_VR_P0V8_VDD3_VOUT",
  "MB_VR_P0V8_VDD3_CURR",
  "MB_VR_P0V8_VDD3_TEMP",
  "MB_VR_P1V0_AVD0_VIN",
  "MB_VR_P1V0_AVD0_VOUT",
  "MB_VR_P1V0_AVD0_CURR",
  "MB_VR_P1V0_AVD0_TEMP",
  "MB_VR_P1V0_AVD1_VIN",
  "MB_VR_P1V0_AVD1_VOUT",
  "MB_VR_P1V0_AVD1_CURR",
  "MB_VR_P1V0_AVD1_TEMP",
  "MB_VR_P1V0_AVD2_VIN",
  "MB_VR_P1V0_AVD2_VOUT",
  "MB_VR_P1V0_AVD2_CURR",
  "MB_VR_P1V0_AVD2_TEMP",
  "MB_VR_P1V0_AVD3_VIN",
  "MB_VR_P1V0_AVD3_VOUT",
  "MB_VR_P1V0_AVD3_CURR",
  "MB_VR_P1V0_AVD3_TEMP",
  "PDB_HSC_P12V_1_VIN",
  "PDB_HSC_P12V_1_VOUT",
  "PDB_HSC_P12V_1_CURR",
  "PDB_HSC_P12V_1_PWR",
  "PDB_HSC_P12V_2_VIN",
  "PDB_HSC_P12V_2_VOUT",
  "PDB_HSC_P12V_2_CURR",
  "PDB_HSC_P12V_2_PWR",
  "PDB_HSC_P12V_AUX_VIN",
  "PDB_HSC_P12V_AUX_VOUT",
  "PDB_HSC_P12V_AUX_CURR",
  "PDB_HSC_P12V_AUX_PWR",
  "PDB_ADC_1_VICOR0_TEMP",
  "PDB_ADC_1_VICOR1_TEMP",
  "PDB_ADC_1_VICOR2_TEMP",
  "PDB_ADC_1_VICOR3_TEMP",
  "PDB_ADC_2_VICOR0_TEMP",
  "PDB_ADC_2_VICOR1_TEMP",
  "PDB_ADC_2_VICOR2_TEMP",
  "PDB_ADC_2_VICOR3_TEMP",
  "PDB_SENSOR_OUTLET_TEMP",
  "PDB_SENSOR_OUTLET_TEMP_REMOTE"
};

float sensors_threshold[MAX_SENSOR_NUM][MAX_SENSOR_THRESHOLD + 1] = {0};
size_t mb_sensor_cnt = sizeof(mb_sensor_list)/sizeof(uint8_t);
size_t pdb_sensor_cnt = sizeof(pdb_sensor_list)/sizeof(uint8_t);

static void hsc_value_adjust(struct calibration_table *table, float *value)
{
    float x0, x1, y0, y1, x;
    int i;
    x = *value;
    x0 = table[0].ein;
    y0 = table[0].coeff;
    if (x0 > *value) {
      *value = x * y0;
      return;
    }
    for (i = 0; table[i].ein > 0.0; i++) {
       if (*value < table[i].ein)
         break;
      x0 = table[i].ein;
      y0 = table[i].coeff;
    }
    if (table[i].ein <= 0.0) {
      *value = x * y0;
      return;
    }
   //if value is bwtween x0 and x1, use linear interpolation method.
   x1 = table[i].ein;
   y1 = table[i].coeff;
   *value = (y0 + (((y1 - y0)/(x1 - x0)) * (x - x0))) * x;
   return;
}

static int read_battery_value(float *value)
{
  int ret = -1;

  gpio_desc_t *gp_batt = gpio_open_by_shadow("BATTERY_DETECT");
  if (!gp_batt) {
    return -1;
  }
  if (gpio_set_value(gp_batt, GPIO_VALUE_HIGH)) {
    goto bail;
  }
  msleep(10);
  ret = sensors_read_adc("MB_ADC_P3V_BAT", value);
  gpio_set_value(gp_batt, GPIO_VALUE_LOW);
bail:
  gpio_close(gp_batt);
  return ret;
}

static int read_pax_dietemp(uint8_t paxid, float *value)
{
  int ret = 0;
  uint8_t addr;
  uint32_t temp, sub_cmd_id;
  char device_name[LARGEST_DEVICE_NAME] = {0};
  struct switchtec_dev *dev;

  addr = SWITCH_BASE_ADDR + paxid;
  snprintf(device_name, LARGEST_DEVICE_NAME, SWITCHTEC_DEV, addr);

  dev = switchtec_open(device_name);
  if (dev == NULL) {
    syslog(LOG_WARNING, "%s: switchtec open %s failed", __func__, device_name);
    return -1;
  }

  sub_cmd_id = MRPC_DIETEMP_GET_GEN4;
  ret = switchtec_cmd(dev, MRPC_DIETEMP, &sub_cmd_id,
                      sizeof(sub_cmd_id), &temp, sizeof(temp));

  switchtec_close(dev);

  if (ret == 0)
    *value = (float) temp / 100.0;

  return ret < 0? -1: 0;
}

static int sensors_read_vicor(uint8_t sensor_num, float *value)
{
  int val;
  int ain, index;
  char ain_name[LARGEST_DEVICE_NAME] = {0};
  char device[LARGEST_DEVICE_NAME] = {0};

  index = (sensor_num - PDB_ADC_1_VICOR0_TEMP) / 4 + 1;
  snprintf(device, LARGEST_DEVICE_NAME, (index == 1)? ADC_1_DIR: ADC_2_DIR, TLA2024_FSR);
  // Set FSR to 4.096
  if (write_device(device, 2) < 0)
    return -1;

  ain = (sensor_num - PDB_ADC_1_VICOR0_TEMP) % 4;
  snprintf(ain_name, LARGEST_DEVICE_NAME, TLA2024_AIN, ain);
  snprintf(device, LARGEST_DEVICE_NAME, (index == 1)? ADC_1_DIR: ADC_2_DIR, ain_name);
  if (read_device(device, &val) < 0)
    return READING_NA;

  /*
   * Convert ADC to temperature
   * Volt = ADC * 4.096 / 2^11
   * Temp = (Volt - 2.73) * 100.0
   */
  *value = (float)(val*0.2-273.0);

  return 0;
}

int pal_set_fan_speed(uint8_t fan, uint8_t pwm)
{
  char label[32] = {0};

  if (fan >= pal_pwm_cnt ||
      snprintf(label, sizeof(label), "pwm%d", fan + 1) > sizeof(label)) {
    return -1;
  }
  return sensors_write_fan(label, (float)pwm);
}

int pal_get_fan_speed(uint8_t fan, int *rpm)
{
  char label[32] = {0};
  float value;
  int ret;

  if (fan >= pal_tach_cnt ||
      snprintf(label, sizeof(label), "fan%d", fan + 1) > sizeof(label)) {
    syslog(LOG_WARNING, "%s: invalid fan#:%d", __func__, fan);
    return -1;
  }
  ret = sensors_read_fan(label, &value);
  *rpm = (int)value;
  return ret;
}

int pal_get_fan_name(uint8_t num, char *name)
{
  if (num >= pal_tach_cnt) {
    syslog(LOG_WARNING, "%s: invalid fan#:%d", __func__, num);
    return -1;
  }

  sprintf(name, "Fan %d %s", num/2, num%2==0? "In":"Out");

  return 0;
}

int pal_get_pwm_value(uint8_t fan, uint8_t *pwm)
{
  char label[32] = {0};
  float value;
  int ret;

  if (fan >= pal_tach_cnt ||
      snprintf(label, sizeof(label), "pwm%d", fan/2 + 1) > sizeof(label)) {
    syslog(LOG_WARNING, "%s: invalid fan#:%d", __func__, fan);
    return -1;
  }
  ret = sensors_read_fan(label, &value);
  if (ret == 0)
    *pwm = (int)value;
  return ret;
}

int pal_get_fru_sensor_list(uint8_t fru, uint8_t **sensor_list, int *cnt)
{
  if (fru == FRU_MB) {
    *sensor_list = (uint8_t *) mb_sensor_list;
    *cnt = mb_sensor_cnt;
  } else if (fru == FRU_PDB) {
    *sensor_list = (uint8_t *) pdb_sensor_list;
    *cnt = pdb_sensor_cnt;
  } else if (fru == FRU_BSM) {
    *sensor_list = NULL;
    *cnt = 0;
  } else {
    *sensor_list = NULL;
    *cnt = 0;
    return -1;
  }

  return 0;
}

static void sensor_thresh_array_init()
{
  // Fan Sensors
  sensors_threshold[MB_FAN0_TACH_I][UNC_THRESH] = 8500;
  sensors_threshold[MB_FAN0_TACH_I][UCR_THRESH] = 11500;
  sensors_threshold[MB_FAN0_TACH_I][LCR_THRESH] = 500;
  sensors_threshold[MB_FAN0_TACH_O][UNC_THRESH] = 8500;
  sensors_threshold[MB_FAN0_TACH_O][UCR_THRESH] = 11500;
  sensors_threshold[MB_FAN0_TACH_O][LCR_THRESH] = 500;
  sensors_threshold[MB_FAN1_TACH_I][UNC_THRESH] = 8500;
  sensors_threshold[MB_FAN1_TACH_I][UCR_THRESH] = 11500;
  sensors_threshold[MB_FAN1_TACH_I][LCR_THRESH] = 500;
  sensors_threshold[MB_FAN1_TACH_O][UNC_THRESH] = 8500;
  sensors_threshold[MB_FAN1_TACH_O][UCR_THRESH] = 11500;
  sensors_threshold[MB_FAN1_TACH_O][LCR_THRESH] = 500;
  sensors_threshold[MB_FAN2_TACH_I][UNC_THRESH] = 8500;
  sensors_threshold[MB_FAN2_TACH_I][UCR_THRESH] = 11500;
  sensors_threshold[MB_FAN2_TACH_I][LCR_THRESH] = 500;
  sensors_threshold[MB_FAN2_TACH_O][UNC_THRESH] = 8500;
  sensors_threshold[MB_FAN2_TACH_O][UCR_THRESH] = 11500;
  sensors_threshold[MB_FAN2_TACH_O][LCR_THRESH] = 500;
  sensors_threshold[MB_FAN3_TACH_I][UNC_THRESH] = 8500;
  sensors_threshold[MB_FAN3_TACH_I][UCR_THRESH] = 11500;
  sensors_threshold[MB_FAN3_TACH_I][LCR_THRESH] = 500;
  sensors_threshold[MB_FAN3_TACH_O][UNC_THRESH] = 8500;
  sensors_threshold[MB_FAN3_TACH_O][UCR_THRESH] = 11500;
  sensors_threshold[MB_FAN3_TACH_O][LCR_THRESH] = 500;

  // ADC Sensors
  sensors_threshold[MB_ADC_P12V_AUX][UCR_THRESH] = 13.23;
  sensors_threshold[MB_ADC_P12V_AUX][LCR_THRESH] = 10.773;
  sensors_threshold[MB_ADC_P3V3_STBY][UCR_THRESH] = 3.621;
  sensors_threshold[MB_ADC_P3V3_STBY][LCR_THRESH] = 2.975;
  sensors_threshold[MB_ADC_P5V_STBY][UCR_THRESH] = 5.486;
  sensors_threshold[MB_ADC_P5V_STBY][LCR_THRESH] = 4.524;
  sensors_threshold[MB_ADC_P12V_1][UCR_THRESH] = 13.23;
  sensors_threshold[MB_ADC_P12V_1][LCR_THRESH] = 10.773;
  sensors_threshold[MB_ADC_P12V_2][UCR_THRESH] = 13.23;
  sensors_threshold[MB_ADC_P12V_2][LCR_THRESH] = 10.773;
  sensors_threshold[MB_ADC_P3V3][UCR_THRESH] = 3.621;
  sensors_threshold[MB_ADC_P3V3][LCR_THRESH] = 2.975;
  sensors_threshold[MB_ADC_P3V_BAT][UCR_THRESH] = 3.738;
  sensors_threshold[MB_ADC_P3V_BAT][LCR_THRESH] = 2.73;

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

  if (fru > FRU_PDB || sensor_num >= FBEP_SENSOR_MAX)
    return -1;

  *val = sensors_threshold[sensor_num][thresh];

  return 0;
}

int pal_get_sensor_name(uint8_t fru, uint8_t sensor_num, char *name)
{
  if (fru > FRU_PDB)
    return -1;

  if (sensor_num >= FBEP_SENSOR_MAX)
    sprintf(name, "INVAILD SENSOR");
  else
    sprintf(name, "%s", sensors_name[sensor_num]);

  return 0;
}

int pal_get_sensor_units(uint8_t fru, uint8_t sensor_num, char *units)
{
  if (fru > FRU_PDB)
    return -1;

  switch(sensor_num) {
    case MB_FAN0_TACH_I:
    case MB_FAN0_TACH_O:
    case MB_FAN1_TACH_I:
    case MB_FAN1_TACH_O:
    case MB_FAN2_TACH_I:
    case MB_FAN2_TACH_O:
    case MB_FAN3_TACH_I:
    case MB_FAN3_TACH_O:
      sprintf(units, "RPM");
      break;
    case MB_FAN0_VOLT:
    case MB_FAN1_VOLT:
    case MB_FAN2_VOLT:
    case MB_FAN3_VOLT:
    case MB_ADC_P12V_AUX:
    case MB_ADC_P3V3_STBY:
    case MB_ADC_P5V_STBY:
    case MB_ADC_P12V_1:
    case MB_ADC_P12V_2:
    case MB_ADC_P3V3:
    case MB_ADC_P3V_BAT:
    case MB_VR_P0V8_VDD0_VIN:
    case MB_VR_P0V8_VDD0_VOUT:
    case MB_VR_P0V8_VDD1_VIN:
    case MB_VR_P0V8_VDD1_VOUT:
    case MB_VR_P0V8_VDD2_VIN:
    case MB_VR_P0V8_VDD2_VOUT:
    case MB_VR_P0V8_VDD3_VIN:
    case MB_VR_P0V8_VDD3_VOUT:
    case MB_VR_P1V0_AVD0_VIN:
    case MB_VR_P1V0_AVD0_VOUT:
    case MB_VR_P1V0_AVD1_VIN:
    case MB_VR_P1V0_AVD1_VOUT:
    case MB_VR_P1V0_AVD2_VIN:
    case MB_VR_P1V0_AVD2_VOUT:
    case MB_VR_P1V0_AVD3_VIN:
    case MB_VR_P1V0_AVD3_VOUT:
    case PDB_HSC_P12V_1_VIN:
    case PDB_HSC_P12V_2_VIN:
    case PDB_HSC_P12V_AUX_VIN:
    case PDB_HSC_P12V_1_VOUT:
    case PDB_HSC_P12V_2_VOUT:
    case PDB_HSC_P12V_AUX_VOUT:
      sprintf(units, "Volts");
      break;
    case MB_SENSOR_GPU_INLET:
    case MB_SENSOR_GPU_INLET_REMOTE:
    case MB_SENSOR_GPU_OUTLET:
    case MB_SENSOR_GPU_OUTLET_REMOTE:
    case MB_SENSOR_PAX01_THERM:
    case MB_SENSOR_PAX0_THERM_REMOTE:
    case MB_SENSOR_PAX1_THERM_REMOTE:
    case MB_SENSOR_PAX23_THERM:
    case MB_SENSOR_PAX2_THERM_REMOTE:
    case MB_SENSOR_PAX3_THERM_REMOTE:
    case MB_SWITCH_PAX0_DIE_TEMP:
    case MB_SWITCH_PAX1_DIE_TEMP:
    case MB_SWITCH_PAX2_DIE_TEMP:
    case MB_SWITCH_PAX3_DIE_TEMP:
    case MB_VR_P0V8_VDD0_TEMP:
    case MB_VR_P0V8_VDD1_TEMP:
    case MB_VR_P0V8_VDD2_TEMP:
    case MB_VR_P0V8_VDD3_TEMP:
    case MB_VR_P1V0_AVD0_TEMP:
    case MB_VR_P1V0_AVD1_TEMP:
    case MB_VR_P1V0_AVD2_TEMP:
    case MB_VR_P1V0_AVD3_TEMP:
    case PDB_ADC_1_VICOR0_TEMP:
    case PDB_ADC_1_VICOR1_TEMP:
    case PDB_ADC_1_VICOR2_TEMP:
    case PDB_ADC_1_VICOR3_TEMP:
    case PDB_ADC_2_VICOR0_TEMP:
    case PDB_ADC_2_VICOR1_TEMP:
    case PDB_ADC_2_VICOR2_TEMP:
    case PDB_ADC_2_VICOR3_TEMP:
    case PDB_SENSOR_OUTLET_TEMP:
    case PDB_SENSOR_OUTLET_TEMP_REMOTE:
      sprintf(units, "Degree C");
      break;
    case MB_FAN0_CURR:
    case MB_FAN1_CURR:
    case MB_FAN2_CURR:
    case MB_FAN3_CURR:
    case MB_VR_P0V8_VDD0_CURR:
    case MB_VR_P0V8_VDD1_CURR:
    case MB_VR_P0V8_VDD2_CURR:
    case MB_VR_P0V8_VDD3_CURR:
    case MB_VR_P1V0_AVD0_CURR:
    case MB_VR_P1V0_AVD1_CURR:
    case MB_VR_P1V0_AVD2_CURR:
    case MB_VR_P1V0_AVD3_CURR:
    case PDB_HSC_P12V_1_CURR:
    case PDB_HSC_P12V_2_CURR:
    case PDB_HSC_P12V_AUX_CURR:
      sprintf(units, "Amps");
      break;
    case PDB_HSC_P12V_1_PWR:
    case PDB_HSC_P12V_2_PWR:
    case PDB_HSC_P12V_AUX_PWR:
      sprintf(units, "Watts");
      break;
    default:
      return -1;
  }
  return 0;
}

int pal_get_sensor_poll_interval(uint8_t fru, uint8_t sensor_num, uint32_t *value)
{
  if (fru > FRU_PDB)
    return -1;

  // default poll interval
  *value = 2;

  if (sensor_num == MB_ADC_P3V_BAT)
    *value = 3600;

  return PAL_EOK;
}

int pal_sensor_read_raw(uint8_t fru, uint8_t sensor_num, void *value)
{
  char key[MAX_KEY_LEN] = {0};
  char str[MAX_VALUE_LEN] = {0};
  int ret;
  uint8_t status;

  if (fru > FRU_PDB)
    return -1;

  switch(sensor_num) {
    // Fan Sensors
    case MB_FAN0_TACH_I:
      ret = sensors_read_fan("fan1", (float *)value);
      break;
    case MB_FAN0_TACH_O:
      ret = sensors_read_fan("fan2", (float *)value);
      break;
    case MB_FAN1_TACH_I:
      ret = sensors_read_fan("fan3", (float *)value);
      break;
    case MB_FAN1_TACH_O:
      ret = sensors_read_fan("fan4", (float *)value);
      break;
    case MB_FAN2_TACH_I:
      ret = sensors_read_fan("fan5", (float *)value);
      break;
    case MB_FAN2_TACH_O:
      ret = sensors_read_fan("fan6", (float *)value);
      break;
    case MB_FAN3_TACH_I:
      ret = sensors_read_fan("fan7", (float *)value);
      break;
    case MB_FAN3_TACH_O:
      ret = sensors_read_fan("fan8", (float *)value);
      break;
    case MB_FAN0_VOLT:
      ret = sensors_read("adc128d818-i2c-18-1d", "FAN0_VOLT", (float *)value);
      break;
    case MB_FAN0_CURR:
      ret = sensors_read("adc128d818-i2c-18-1d", "FAN0_CURR", (float *)value);
      break;
    case MB_FAN1_VOLT:
      ret = sensors_read("adc128d818-i2c-18-1d", "FAN1_VOLT", (float *)value);
      break;
    case MB_FAN1_CURR:
      ret = sensors_read("adc128d818-i2c-18-1d", "FAN1_CURR", (float *)value);
      break;
    case MB_FAN2_VOLT:
      ret = sensors_read("adc128d818-i2c-18-1d", "FAN2_VOLT", (float *)value);
      break;
    case MB_FAN2_CURR:
      ret = sensors_read("adc128d818-i2c-18-1d", "FAN2_CURR", (float *)value);
      break;
    case MB_FAN3_VOLT:
      ret = sensors_read("adc128d818-i2c-18-1d", "FAN3_VOLT", (float *)value);
      break;
    case MB_FAN3_CURR:
      ret = sensors_read("adc128d818-i2c-18-1d", "FAN3_CURR", (float *)value);
      break;
    // ADC Sensors (Resistance unit = 1K)
    case MB_ADC_P12V_AUX:
      ret = sensors_read_adc("MB_ADC_P12V_AUX", (float *)value);
      break;
    case MB_ADC_P3V3_STBY:
      ret = sensors_read_adc("MB_ADC_P3V3_STBY", (float *)value);
      break;
    case MB_ADC_P5V_STBY:
      ret = sensors_read_adc("MB_ADC_P5V_STBY", (float *)value);
      break;
    case MB_ADC_P12V_1:
      ret = sensors_read_adc("MB_ADC_P12V_1", (float *)value);
      break;
    case MB_ADC_P12V_2:
      ret = sensors_read_adc("MB_ADC_P12V_2", (float *)value);
      break;
    case MB_ADC_P3V3:
      ret = sensors_read_adc("MB_ADC_P3V3", (float *)value);
      break;
    case MB_ADC_P3V_BAT:
      ret = read_battery_value((float*)value);
      break;
    // Thermal sensors
    case MB_SENSOR_GPU_INLET:
      ret = sensors_read("tmp421-i2c-6-4c", "GPU_INLET", (float *)value);
      break;
    case MB_SENSOR_GPU_INLET_REMOTE:
      ret = sensors_read("tmp421-i2c-6-4c", "GPU_INLET_REMOTE", (float *)value);
      break;
    case MB_SENSOR_GPU_OUTLET:
      ret = sensors_read("tmp421-i2c-6-4f", "GPU_OUTLET", (float *)value);
      break;
    case MB_SENSOR_GPU_OUTLET_REMOTE:
      ret = sensors_read("tmp421-i2c-6-4f", "GPU_OUTLET_REMOTE", (float *)value);
      break;
    case MB_SENSOR_PAX01_THERM:
      if (pal_get_server_power(FRU_MB, &status) == 0 && status == SERVER_POWER_ON)
        ret = sensors_read("tmp422-i2c-6-4d", "PAX01_THERM", (float *)value);
      else
        ret = READING_NA;
      break;
    case MB_SENSOR_PAX0_THERM_REMOTE:
      if (pal_get_server_power(FRU_MB, &status) == 0 && status == SERVER_POWER_ON)
        ret = sensors_read("tmp422-i2c-6-4d", "PAX0_THERM_REMOTE", (float *)value);
      else
        ret = READING_NA;
      break;
    case MB_SENSOR_PAX1_THERM_REMOTE:
      if (pal_get_server_power(FRU_MB, &status) == 0 && status == SERVER_POWER_ON)
        ret = sensors_read("tmp422-i2c-6-4d", "PAX1_THERM_REMOTE", (float *)value);
      else
        ret = READING_NA;
      break;
    case MB_SENSOR_PAX23_THERM:
      if (pal_get_server_power(FRU_MB, &status) == 0 && status == SERVER_POWER_ON)
        ret = sensors_read("tmp422-i2c-6-4e", "PAX23_THERM", (float *)value);
      else
        ret = READING_NA;
      break;
    case MB_SENSOR_PAX2_THERM_REMOTE:
      if (pal_get_server_power(FRU_MB, &status) == 0 && status == SERVER_POWER_ON)
        ret = sensors_read("tmp422-i2c-6-4e", "PAX2_THERM_REMOTE", (float *)value);
      else
        ret = READING_NA;
      break;
    case MB_SENSOR_PAX3_THERM_REMOTE:
      if (pal_get_server_power(FRU_MB, &status) == 0 && status == SERVER_POWER_ON)
        ret = sensors_read("tmp422-i2c-6-4e", "PAX3_THERM_REMOTE", (float *)value);
      else
        ret = READING_NA;
      break;
    case PDB_SENSOR_OUTLET_TEMP:
      ret = sensors_read("tmp421-i2c-17-4c", "OUTLET_TEMP", (float *)value);
      break;
    case PDB_SENSOR_OUTLET_TEMP_REMOTE:
      ret = sensors_read("tmp421-i2c-17-4c", "OUTLET_TEMP_REMOTE", (float *)value);
      break;
    // Temperature within PCIe switch
    case MB_SWITCH_PAX0_DIE_TEMP:
    case MB_SWITCH_PAX1_DIE_TEMP:
    case MB_SWITCH_PAX2_DIE_TEMP:
    case MB_SWITCH_PAX3_DIE_TEMP:
      if (pal_get_server_power(FRU_MB, &status) == 0 && status == SERVER_POWER_ON
	  && !pal_is_pax_proc_ongoing(sensor_num - MB_SWITCH_PAX0_DIE_TEMP)) {
        ret = read_pax_dietemp(sensor_num - MB_SWITCH_PAX0_DIE_TEMP, (float*)value);
      } else {
        ret = READING_NA;
      }
      break;
    // HSC reading
    case PDB_HSC_P12V_1_CURR:
      ret = sensors_read("ltc4282-i2c-16-53", "P12V_1_CURR", (float *)value);
      if (ret == 0)
        hsc_value_adjust(current_table, (float *)value);
      break;
    case PDB_HSC_P12V_2_CURR:
      ret = sensors_read("ltc4282-i2c-17-40", "P12V_2_CURR", (float *)value);
      if (ret == 0)
        hsc_value_adjust(current_table, (float *)value);
      break;
    case PDB_HSC_P12V_AUX_CURR:
      ret = sensors_read("ltc4282-i2c-18-43", "P12V_AUX_CURR", (float *)value);
      if (ret == 0)
        hsc_value_adjust(aux_current_table, (float *)value);
      break;
    case PDB_HSC_P12V_1_PWR:
      ret = sensors_read("ltc4282-i2c-16-53", "P12V_1_PWR", (float *)value);
      if (ret == 0)
        hsc_value_adjust(power_table, (float *)value);
      break;
    case PDB_HSC_P12V_2_PWR:
      ret = sensors_read("ltc4282-i2c-17-40", "P12V_2_PWR", (float *)value);
      if (ret == 0)
        hsc_value_adjust(power_table, (float *)value);
      break;
    case PDB_HSC_P12V_AUX_PWR:
      ret = sensors_read("ltc4282-i2c-18-43", "P12V_AUX_PWR", (float *)value);
      if (ret == 0)
        hsc_value_adjust(aux_power_table, (float *)value);
      break;
    case PDB_HSC_P12V_1_VIN:
      ret = sensors_read("ltc4282-i2c-16-53", "P12V_1_VIN", (float *)value);
      break;
    case PDB_HSC_P12V_2_VIN:
      ret = sensors_read("ltc4282-i2c-17-40", "P12V_2_VIN", (float *)value);
      break;
    case PDB_HSC_P12V_AUX_VIN:
      ret = sensors_read("ltc4282-i2c-18-43", "P12V_AUX_VIN", (float *)value);
      break;
    case PDB_HSC_P12V_1_VOUT:
      ret = sensors_read("ltc4282-i2c-16-53", "P12V_1_VOUT", (float *)value);
      break;
    case PDB_HSC_P12V_2_VOUT:
      ret = sensors_read("ltc4282-i2c-17-40", "P12V_2_VOUT", (float *)value);
      break;
    case PDB_HSC_P12V_AUX_VOUT:
      ret = sensors_read("ltc4282-i2c-18-43", "P12V_AUX_VOUT", (float *)value);
      break;
    // Vicor temperature
    case PDB_ADC_1_VICOR0_TEMP:
    case PDB_ADC_1_VICOR1_TEMP:
    case PDB_ADC_1_VICOR2_TEMP:
    case PDB_ADC_1_VICOR3_TEMP:
    case PDB_ADC_2_VICOR0_TEMP:
    case PDB_ADC_2_VICOR1_TEMP:
    case PDB_ADC_2_VICOR2_TEMP:
    case PDB_ADC_2_VICOR3_TEMP:
      ret = sensors_read_vicor(sensor_num, (float *)value);
      break;
    // Voltage regulator
    case MB_VR_P0V8_VDD0_VIN:
      ret = sensors_read("mpq8645p-i2c-5-30", "VR_P0V8_VDD0_VIN", (float *)value);
      break;
    case MB_VR_P0V8_VDD0_VOUT:
      ret = sensors_read("mpq8645p-i2c-5-30", "VR_P0V8_VDD0_VOUT", (float *)value);
      break;
    case MB_VR_P0V8_VDD0_CURR:
      ret = sensors_read("mpq8645p-i2c-5-30", "VR_P0V8_VDD0_CURR", (float *)value);
      break;
    case MB_VR_P0V8_VDD0_TEMP:
      ret = sensors_read("mpq8645p-i2c-5-30", "VR_P0V8_VDD0_TEMP", (float *)value);
      break;
    case MB_VR_P0V8_VDD1_VIN:
      ret = sensors_read("mpq8645p-i2c-5-31", "VR_P0V8_VDD1_VIN", (float *)value);
      break;
    case MB_VR_P0V8_VDD1_VOUT:
      ret = sensors_read("mpq8645p-i2c-5-31", "VR_P0V8_VDD1_VOUT", (float *)value);
      break;
    case MB_VR_P0V8_VDD1_CURR:
      ret = sensors_read("mpq8645p-i2c-5-31", "VR_P0V8_VDD1_CURR", (float *)value);
      break;
    case MB_VR_P0V8_VDD1_TEMP:
      ret = sensors_read("mpq8645p-i2c-5-31", "VR_P0V8_VDD1_TEMP", (float *)value);
      break;
    case MB_VR_P0V8_VDD2_VIN:
      ret = sensors_read("mpq8645p-i2c-5-32", "VR_P0V8_VDD2_VIN", (float *)value);
      break;
    case MB_VR_P0V8_VDD2_VOUT:
      ret = sensors_read("mpq8645p-i2c-5-32", "VR_P0V8_VDD2_VOUT", (float *)value);
      break;
    case MB_VR_P0V8_VDD2_CURR:
      ret = sensors_read("mpq8645p-i2c-5-32", "VR_P0V8_VDD2_CURR", (float *)value);
      break;
    case MB_VR_P0V8_VDD2_TEMP:
      ret = sensors_read("mpq8645p-i2c-5-32", "VR_P0V8_VDD2_TEMP", (float *)value);
      break;
    case MB_VR_P0V8_VDD3_VIN:
      ret = sensors_read("mpq8645p-i2c-5-33", "VR_P0V8_VDD3_VIN", (float *)value);
      break;
    case MB_VR_P0V8_VDD3_VOUT:
      ret = sensors_read("mpq8645p-i2c-5-33", "VR_P0V8_VDD3_VOUT", (float *)value);
      break;
    case MB_VR_P0V8_VDD3_CURR:
      ret = sensors_read("mpq8645p-i2c-5-33", "VR_P0V8_VDD3_CURR", (float *)value);
      break;
    case MB_VR_P0V8_VDD3_TEMP:
      ret = sensors_read("mpq8645p-i2c-5-33", "VR_P0V8_VDD3_TEMP", (float *)value);
      break;
    case MB_VR_P1V0_AVD0_VIN:
      ret = sensors_read("mpq8645p-i2c-5-34", "VR_P1V0_AVD0_VIN", (float *)value);
      break;
    case MB_VR_P1V0_AVD0_VOUT:
      ret = sensors_read("mpq8645p-i2c-5-34", "VR_P1V0_AVD0_VOUT", (float *)value);
      break;
    case MB_VR_P1V0_AVD0_CURR:
      ret = sensors_read("mpq8645p-i2c-5-34", "VR_P1V0_AVD0_CURR", (float *)value);
      break;
    case MB_VR_P1V0_AVD0_TEMP:
      ret = sensors_read("mpq8645p-i2c-5-34", "VR_P1V0_AVD0_TEMP", (float *)value);
      break;
    case MB_VR_P1V0_AVD1_VIN:
      ret = sensors_read("mpq8645p-i2c-5-35", "VR_P1V0_AVD1_VIN", (float *)value);
      break;
    case MB_VR_P1V0_AVD1_VOUT:
      ret = sensors_read("mpq8645p-i2c-5-35", "VR_P1V0_AVD1_VOUT", (float *)value);
      break;
    case MB_VR_P1V0_AVD1_CURR:
      ret = sensors_read("mpq8645p-i2c-5-35", "VR_P1V0_AVD1_CURR", (float *)value);
      break;
    case MB_VR_P1V0_AVD1_TEMP:
      ret = sensors_read("mpq8645p-i2c-5-35", "VR_P1V0_AVD1_TEMP", (float *)value);
      break;
    case MB_VR_P1V0_AVD2_VIN:
      ret = sensors_read("mpq8645p-i2c-5-36", "VR_P1V0_AVD2_VIN", (float *)value);
      break;
    case MB_VR_P1V0_AVD2_VOUT:
      ret = sensors_read("mpq8645p-i2c-5-36", "VR_P1V0_AVD2_VOUT", (float *)value);
      break;
    case MB_VR_P1V0_AVD2_CURR:
      ret = sensors_read("mpq8645p-i2c-5-36", "VR_P1V0_AVD2_CURR", (float *)value);
      break;
    case MB_VR_P1V0_AVD2_TEMP:
      ret = sensors_read("mpq8645p-i2c-5-36", "VR_P1V0_AVD2_TEMP", (float *)value);
      break;
    case MB_VR_P1V0_AVD3_VIN:
      ret = sensors_read("mpq8645p-i2c-5-3b", "VR_P1V0_AVD3_VIN", (float *)value);
      break;
    case MB_VR_P1V0_AVD3_VOUT:
      ret = sensors_read("mpq8645p-i2c-5-3b", "VR_P1V0_AVD3_VOUT", (float *)value);
      break;
    case MB_VR_P1V0_AVD3_CURR:
      ret = sensors_read("mpq8645p-i2c-5-3b", "VR_P1V0_AVD3_CURR", (float *)value);
      break;
    case MB_VR_P1V0_AVD3_TEMP:
      ret = sensors_read("mpq8645p-i2c-5-3b", "VR_P1V0_AVD3_TEMP", (float *)value);
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

