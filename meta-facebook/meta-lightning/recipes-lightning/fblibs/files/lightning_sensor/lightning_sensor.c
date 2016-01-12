/*
 *
 * Copyright 2015-present Facebook. All Rights Reserved.
 *
 * This file contains code to support IPMI2.0 Specificaton available @
 * http://www.intel.com/content/www/us/en/servers/ipmi/ipmi-specifications.html
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
#include <stdint.h>
#include <stdbool.h>
#include <math.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <syslog.h>
#include "lightning_sensor.h"

#define LARGEST_DEVICE_NAME 120

#define I2C_BUS_2_DIR "/sys/class/i2c-adapter/i2c-2/"
#define I2C_BUS_4_DIR "/sys/class/i2c-adapter/i2c-4/"
#define I2C_BUS_5_DIR "/sys/class/i2c-adapter/i2c-5/"
#define I2C_BUS_6_DIR "/sys/class/i2c-adapter/i2c-6/"

#define TACH_DIR "/sys/devices/platform/ast_pwm_tacho.0"
#define ADC_DIR "/sys/devices/platform/ast_adc.0"

#define PEB_TEMP_1_DEVICE I2C_BUS_2_DIR "2-004a"
#define PEB_TEMP_2_DEVICE I2C_BUS_2_DIR "2-004c"
#define PEB_HSC_DEVICE I2C_BUS_4_DIR "4-0011"

#define PDPB_TEMP_1_DEVICE I2C_BUS_6_DIR "6-0049"
#define PDPB_TEMP_2_DEVICE I2C_BUS_6_DIR "6-004a"
#define PDPB_TEMP_3_DEVICE I2C_BUS_6_DIR "6-004b"

#define FCB_HSC_DEVICE I2C_BUS_5_DIR "5-0022"

#define FAN_TACH_RPM "tacho%d_rpm"
#define ADC_VALUE "adc%d_value"
#define HSC_IN_VOLT "in1_input"
#define HSC_OUT_CURR "curr1_input"
#define HSC_TEMP "temp1_input"

#define UNIT_DIV 1000


#define MAX_SENSOR_NUM 0xFF
#define ALL_BYTES 0xFF
#define LAST_REC_ID 0xFFFF

// List of PEB sensors to be monitored
const uint8_t peb_sensor_list[] = {
  PEB_SENSOR_TEMP_1,
  PEB_SENSOR_TEMP_2,
  PEB_SENSOR_HSC_IN_VOLT,
  PEB_SENSOR_HSC_OUT_CURR,
  PEB_SENSOR_HSC_TEMP,
  PEB_SENSOR_HSC_IN_POWER,
};

// List of PDPB sensors to be monitored
const uint8_t pdpb_sensor_list[] = {
  PDPB_SENSOR_TEMP_1,
  PDPB_SENSOR_TEMP_2,
  PDPB_SENSOR_TEMP_3,
};

// List of NIC sensors to be monitored
const uint8_t fcb_sensor_list[] = {
  FCB_SENSOR_HSC_IN_VOLT,
  FCB_SENSOR_HSC_OUT_CURR,
  FCB_SENSOR_HSC_IN_POWER,
};

static sensor_info_t g_sinfo[MAX_NUM_FRUS][MAX_SENSOR_NUM] = {0};

float peb_sensor_threshold[MAX_SENSOR_NUM][MAX_SENSOR_THRESHOLD + 1] = {0};
float pdpb_sensor_threshold[MAX_SENSOR_NUM][MAX_SENSOR_THRESHOLD + 1] = {0};
float fcb_sensor_threshold[MAX_SENSOR_NUM][MAX_SENSOR_THRESHOLD + 1] = {0};

static void
sensor_thresh_array_init() {
  static bool init_done = false;

  if (init_done)
    return;

  peb_sensor_threshold[PEB_SENSOR_TEMP_1][UCR_THRESH] = 100;
  peb_sensor_threshold[PEB_SENSOR_TEMP_2][UCR_THRESH] = 100;
  peb_sensor_threshold[PEB_SENSOR_HSC_IN_VOLT][UCR_THRESH] = 100;
  peb_sensor_threshold[PEB_SENSOR_HSC_OUT_CURR][UCR_THRESH] = 100;
  peb_sensor_threshold[PEB_SENSOR_HSC_TEMP][UCR_THRESH] = 100;
  peb_sensor_threshold[PEB_SENSOR_HSC_IN_POWER][UCR_THRESH] = 100;

  pdpb_sensor_threshold[PDPB_SENSOR_TEMP_1][UCR_THRESH] = 100;
  pdpb_sensor_threshold[PDPB_SENSOR_TEMP_2][UCR_THRESH] = 100;
  pdpb_sensor_threshold[PDPB_SENSOR_TEMP_3][UCR_THRESH] = 100;

  fcb_sensor_threshold[FCB_SENSOR_HSC_IN_VOLT][UCR_THRESH] = 100;
  fcb_sensor_threshold[FCB_SENSOR_HSC_OUT_CURR][UCR_THRESH] = 100;
  fcb_sensor_threshold[FCB_SENSOR_HSC_IN_POWER][UCR_THRESH] = 100;

  init_done = true;
}

size_t peb_sensor_cnt = sizeof(peb_sensor_list)/sizeof(uint8_t);

size_t pdpb_sensor_cnt = sizeof(pdpb_sensor_list)/sizeof(uint8_t);

size_t fcb_sensor_cnt = sizeof(fcb_sensor_list)/sizeof(uint8_t);

static int
read_device(const char *device, int *value) {
  FILE *fp;
  int rc;

  fp = fopen(device, "r");
  if (!fp) {
    int err = errno;

#ifdef DEBUG
    syslog(LOG_INFO, "failed to open device %s", device);
#endif
    return err;
  }

  rc = fscanf(fp, "%d", value);
  fclose(fp);

  if (rc != 1) {
#ifdef DEBUG
    syslog(LOG_INFO, "failed to read device %s", device);
#endif
    return ENOENT;
  } else {
    return 0;
  }
}

static int
read_device_float(const char *device, float *value) {
  FILE *fp;
  int rc;
  char tmp[10];

  fp = fopen(device, "r");
  if (!fp) {
    int err = errno;
#ifdef DEBUG
    syslog(LOG_INFO, "failed to open device %s", device);
#endif
    return err;
  }

  rc = fscanf(fp, "%s", tmp);
  fclose(fp);

  if (rc != 1) {
#ifdef DEBUG
    syslog(LOG_INFO, "failed to read device %s", device);
#endif
    return ENOENT;
  }

  *value = atof(tmp);

  return 0;
}

static int
read_temp(const char *device, float *value) {
  char full_name[LARGEST_DEVICE_NAME + 1];
  int tmp;

  snprintf(
      full_name, LARGEST_DEVICE_NAME, "%s/temp1_input", device);
  if (read_device(full_name, &tmp)) {
    return -1;
  }

  *value = ((float)tmp)/UNIT_DIV;

  return 0;
}

static int
read_adc_value(const int pin, const char *device, float *value) {
  char device_name[LARGEST_DEVICE_NAME];
  char full_name[LARGEST_DEVICE_NAME];

  snprintf(device_name, LARGEST_DEVICE_NAME, device, pin);
  snprintf(full_name, LARGEST_DEVICE_NAME, "%s/%s", ADC_DIR, device_name);
  return read_device_float(full_name, value);
}

static int
read_hsc_value(const char *device, const char *param,  float *value) {
  char full_name[LARGEST_DEVICE_NAME];
  int tmp;

  snprintf(full_name, LARGEST_DEVICE_NAME, "%s/%s", device, param);
  if(read_device(full_name, &tmp)) {
    return -1;
  }

  *value = ((float) tmp)/UNIT_DIV;

  return 0;
}


int
lightning_sensor_sdr_path(uint8_t fru, char *path) {
  return 0;
}

/* Populates all sensor_info_t struct using the path to SDR dump */
int
sdr_init(char *path, sensor_info_t *sinfo) {
  return 0;
}

int
lightning_sensor_sdr_init(uint8_t fru, sensor_info_t *sinfo) {
  return -1;
}

static int
lightning_sdr_init(uint8_t fru) {

  static bool init_done[MAX_NUM_FRUS] = {false};

  if (!init_done[fru - 1]) {

    sensor_info_t *sinfo = g_sinfo[fru-1];

    if (lightning_sensor_sdr_init(fru, sinfo) < 0)
      return ERR_NOT_READY;

    init_done[fru - 1] = true;
  }

  return 0;
}

/* Get the units for the sensor */
int
lightning_sensor_units(uint8_t fru, uint8_t sensor_num, char *units) {
  uint8_t op, modifier;
  sensor_info_t *sinfo;

  switch(fru) {
    case FRU_PEB:
      switch(sensor_num) {
        case PEB_SENSOR_TEMP_1:
          sprintf(units, "C");
          break;
        case PEB_SENSOR_TEMP_2:
          sprintf(units, "C");
          break;
        case PEB_SENSOR_HSC_IN_VOLT:
          sprintf(units, "Volts");
          break;
        case PEB_SENSOR_HSC_OUT_CURR:
          sprintf(units, "Amps");
          break;
        case PEB_SENSOR_HSC_TEMP:
          sprintf(units, "C");
          break;
        case PEB_SENSOR_HSC_IN_POWER:
          sprintf(units, "Watts");
          break;
        default:
          sprintf(units, "");
          break;
      }
      break;

    case FRU_PDPB:
      switch(sensor_num) {
        case PDPB_SENSOR_TEMP_1:
          sprintf(units, "C");
          break;
        case PDPB_SENSOR_TEMP_2:
          sprintf(units, "C");
          break;
        case PDPB_SENSOR_TEMP_3:
          sprintf(units, "C");
          break;
        default:
          sprintf(units, "");
          break;
      }
      break;

    case FRU_FCB:
      switch(sensor_num) {
        case FCB_SENSOR_HSC_IN_VOLT:
          sprintf(units, "Volts");
          break;
        case FCB_SENSOR_HSC_OUT_CURR:
          sprintf(units, "Amps");
          break;
        case FCB_SENSOR_HSC_IN_POWER:
          sprintf(units, "Watts");
          break;
        default:
          sprintf(units, "");
          break;
      }
      break;
  }
  return 0;
}

int
lightning_sensor_threshold(uint8_t fru, uint8_t sensor_num, uint8_t thresh, float *value) {

  sensor_thresh_array_init();

  switch(fru) {
    case FRU_PEB:
      *value = peb_sensor_threshold[sensor_num][thresh];
      break;
    case FRU_PDPB:
      *value = pdpb_sensor_threshold[sensor_num][thresh];
      break;
    case FRU_FCB:
      *value = fcb_sensor_threshold[sensor_num][thresh];
      break;
  }
  return 0;
}

/* Get the name for the sensor */
int
lightning_sensor_name(uint8_t fru, uint8_t sensor_num, char *name) {

  switch(fru) {
    case FRU_PEB:
      switch(sensor_num) {
        case PEB_SENSOR_TEMP_1:
          sprintf(name, "PEB_TEMP_1");
          break;
        case PEB_SENSOR_TEMP_2:
          sprintf(name, "PEB_TEMP_2");
          break;
        case PEB_SENSOR_HSC_IN_VOLT:
          sprintf(name, "PEB_HSC_IN_VOLT");
          break;
        case PEB_SENSOR_HSC_OUT_CURR:
          sprintf(name, "PEB_HSC_OUT_CURR");
          break;
        case PEB_SENSOR_HSC_TEMP:
          sprintf(name, "PEB_HSC_TEMP");
          break;
        case PEB_SENSOR_HSC_IN_POWER:
          sprintf(name, "PEB_HSC_IN_POWER");
          break;
        default:
          sprintf(name, "");
          break;
      }
      break;

    case FRU_PDPB:
      switch(sensor_num) {
        case PDPB_SENSOR_TEMP_1:
          sprintf(name, "PDPB_TEMP_1");
          break;
        case PDPB_SENSOR_TEMP_2:
          sprintf(name, "PDPB_TEMP_2");
          break;
        case PDPB_SENSOR_TEMP_3:
          sprintf(name, "PDPB_TEMP_3");
          break;
      }
      break;

    case FRU_FCB:
      switch(sensor_num) {
        case FCB_SENSOR_HSC_IN_VOLT:
          sprintf(name, "FCB_HSC_IN_VOLT");
          break;
        case FCB_SENSOR_HSC_OUT_CURR:
          sprintf(name, "FCB_HSC_OUT_CURR");
          break;
        case FCB_SENSOR_HSC_IN_POWER:
          sprintf(name, "FCB_HSC_IN_POWER");
          break;
      }
      break;
  }
  return 0;
}


int
lightning_sensor_read(uint8_t fru, uint8_t sensor_num, void *value) {

  float volt;
  float curr;
  int ret;
  bool discrete;
  int i;

  switch (fru) {
    case FRU_PEB:
      switch(sensor_num) {
        case PEB_SENSOR_TEMP_1:
          return read_temp(PEB_TEMP_1_DEVICE, (float*) value);
        case PEB_SENSOR_TEMP_2:
          return read_temp(PEB_TEMP_2_DEVICE, (float*) value);

        // Hot Swap Controller
        case PEB_SENSOR_HSC_IN_VOLT:
          return read_hsc_value(PEB_HSC_DEVICE, HSC_IN_VOLT, (float*) value);
        case PEB_SENSOR_HSC_OUT_CURR:
          return read_hsc_value(PEB_HSC_DEVICE, HSC_OUT_CURR, (float*) value);
        case PEB_SENSOR_HSC_TEMP:
          return read_hsc_value(PEB_HSC_DEVICE, HSC_TEMP, (float*) value);
        case PEB_SENSOR_HSC_IN_POWER:
          if (read_hsc_value(PEB_HSC_DEVICE, HSC_IN_VOLT, &volt)) {
            return -1;
          }
          if (read_hsc_value(PEB_HSC_DEVICE, HSC_OUT_CURR, &curr)) {
            return -1;
          }
          * (float*) value = volt * curr;
          return 0;
      }
      break;

    case FRU_PDPB:
      switch(sensor_num) {

        // Temp
        case PDPB_SENSOR_TEMP_1:
          return read_temp(PDPB_TEMP_1_DEVICE, (float*) value);
        case PDPB_SENSOR_TEMP_2:
          return read_temp(PDPB_TEMP_2_DEVICE, (float*) value);
        case PDPB_SENSOR_TEMP_3:
          return read_temp(PDPB_TEMP_3_DEVICE, (float*) value);
      }
      break;

    case FRU_FCB:
      switch(sensor_num) {
        // Hot Swap Controller
        case FCB_SENSOR_HSC_IN_VOLT:
          return read_hsc_value(FCB_HSC_DEVICE, HSC_IN_VOLT, (float*) value);
        case FCB_SENSOR_HSC_OUT_CURR:
          return read_hsc_value(FCB_HSC_DEVICE, HSC_OUT_CURR, (float*) value);
        case FCB_SENSOR_HSC_IN_POWER:
          if (read_hsc_value(FCB_HSC_DEVICE, HSC_IN_VOLT, &volt)) {
            return -1;
          }
          if (read_hsc_value(FCB_HSC_DEVICE, HSC_OUT_CURR, &curr)) {
            return -1;
          }
          * (float*) value = volt * curr;
          return 0;
      }
      break;
  }
}

