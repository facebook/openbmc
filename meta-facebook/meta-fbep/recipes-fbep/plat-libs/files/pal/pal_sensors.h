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


#ifndef __PAL_SENSORS_H__
#define __PAL_SENSORS_H__

#include <openbmc/obmc_pal_sensors.h>

#ifdef __cplusplus
extern "C" {
#endif

enum {
  FAN_0 = 1,
  FAN_1,
  FAN_2,
  FAN_3,
  FAN_4,
  FAN_5,
  FAN_6,
  FAN_7,
};

enum {
  ADC_0 = 1,
  ADC_1,
  ADC_2,
  ADC_3,
  ADC_4,
  ADC_7,
  ADC_8,
};

// Total sensors list
enum {
  FRU_FAN0_TACH_I = 0x0,
  FRU_FAN0_TACH_O,
  FRU_FAN1_TACH_I,
  FRU_FAN1_TACH_O,
  FRU_FAN2_TACH_I,
  FRU_FAN2_TACH_O,
  FRU_FAN3_TACH_I,
  FRU_FAN3_TACH_O,
  FRU_ADC_P12V_AUX,
  FRU_ADC_P3V3_STBY,
  FRU_ADC_P5V_STBY,
  FRU_ADC_P12V_1,
  FRU_ADC_P12V_2,
  FRU_ADC_P3V3,
  FRU_ADC_P3V_BAT,
  FRU_SENSOR_GPU_INLET,
  FRU_SENSOR_GPU_OUTLET,
  FRU_SENSOR_PAX01_THERM,
  FRU_SENSOR_PAX23_THERM,
  FRU_SWITCH_PAX0_DIE_TEMP,
  FRU_SWITCH_PAX1_DIE_TEMP,
  FRU_SWITCH_PAX2_DIE_TEMP,
  FRU_SWITCH_PAX3_DIE_TEMP,
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
  FRU_SENSOR_MAX,  //keep this at the tail
};

int pal_set_fan_speed(uint8_t fan, uint8_t pwm);
int pal_get_fan_speed(uint8_t fan, int *rpm);
int pal_get_fan_name(uint8_t num, char *name);
int pal_get_pwm_value(uint8_t fan_num, uint8_t *value);

#ifdef __cplusplus
} // extern "C"
#endif

#endif /* __PAL_SENSORS_H__ */
