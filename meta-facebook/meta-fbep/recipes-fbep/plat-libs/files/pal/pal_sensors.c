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

static int sensors_read_common_fan(uint8_t, float*);
static int sensors_read_common_adc(uint8_t, float*);
static int read_battery_value(uint8_t, float*);
static int read_pax_dietemp(uint8_t, float*);
static int sensors_read_vicor(uint8_t, float*);
static int sensors_read_common_therm(uint8_t, float*);
static int sensors_read_pax_therm(uint8_t, float*);
static int sensors_read_vr(uint8_t, float*);
static int sensors_read_12v_hsc(uint8_t, float*);
static int sensors_read_48v_hsc(uint8_t, float*);

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
  PDB_HSC_P48V_1_VIN,
  PDB_HSC_P48V_1_VOUT,
  PDB_HSC_P48V_1_CURR,
  PDB_HSC_P48V_1_PWR,
  PDB_HSC_P48V_2_VIN,
  PDB_HSC_P48V_2_VOUT,
  PDB_HSC_P48V_2_CURR,
  PDB_HSC_P48V_2_PWR,
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

float sensors_threshold[MAX_SENSOR_NUM][MAX_SENSOR_THRESHOLD + 1] = {
//{	UCR,	UNC,	UNR,	LCR,	LNC,	LNR,	POS,	NEG}
  [MB_FAN0_TACH_I] =
  {0,	13000,	0,	0,	1000,	0,	0,	0,	0},
  [MB_FAN0_TACH_O] =
  {0,	12000,	0,	0,	1000,	0,	0,	0,	0},
  [MB_FAN1_TACH_I] =
  {0,	13000,	0,	0,	1000,	0,	0,	0,	0},
  [MB_FAN1_TACH_O] =
  {0,	12000,	0,	0,	1000,	0,	0,	0,	0},
  [MB_FAN2_TACH_I] =
  {0,	13000,	0,	0,	1000,	0,	0,	0,	0},
  [MB_FAN2_TACH_O] =
  {0,	12000,	0,	0,	1000,	0,	0,	0,	0},
  [MB_FAN3_TACH_I] =
  {0,	13000,	0,	0,	1000,	0,	0,	0,	0},
  [MB_FAN3_TACH_O] =
  {0,	12000,	0,	0,	1000,	0,	0,	0,	0},
  [MB_FAN0_VOLT] =
  {0,	13.2,	13.0,	0,	10.8,	0,	0,	0,	0},
  [MB_FAN1_VOLT] =
  {0,	13.2,	13.0,	0,	10.8,	0,	0,	0,	0},
  [MB_FAN2_VOLT] =
  {0,	13.2,	13.0,	0,	10.8,	0,	0,	0,	0},
  [MB_FAN3_VOLT] =
  {0,	13.2,	13.0,	0,	10.8,	0,	0,	0,	0},
  [MB_FAN0_CURR] =
  {0,	8.0,	7.26,	0,	0,	0,	0,	0,	0},
  [MB_FAN1_CURR] =
  {0,	8.0,	7.26,	0,	0,	0,	0,	0,	0},
  [MB_FAN2_CURR] =
  {0,	8.0,	7.26,	0,	0,	0,	0,	0,	0},
  [MB_FAN3_CURR] =
  {0,	8.0,	7.26,	0,	0,	0,	0,	0,	0},
  [MB_ADC_P12V_AUX] =
  {0,	13.2,	13.0,	0,	10.8,	0,	0,	0,	0},
  [MB_ADC_P12V_1] =
  {0,	13.2,	13.0,	0,	10.8,	0,	0,	0,	0},
  [MB_ADC_P12V_2] =
  {0,	13.2,	13.0,	0,	10.8,	0,	0,	0,	0},
  [MB_ADC_P3V3_STBY] =
  {0,	3.465,	0,	0,	3.135,	0,	0,	0,	0},
  [MB_ADC_P3V3] =
  {0,	3.465,	0,	0,	3.135,	0,	0,	0,	0},
  [MB_ADC_P3V_BAT] =
  {0,	3.465,	0,	0,	2.0,	0,	0,	0,	0},
  [MB_ADC_P5V_STBY] =
  {0,	5.25,	0,	0,	4.75,	0,	0,	0,	0},
  [MB_SENSOR_GPU_INLET] =
  {0,	55.0,	0,	0,	10.0,	0,	0,	0,	0},
  [MB_SENSOR_GPU_INLET_REMOTE] =
  {0,	50.0,	0,	0,	10.0,	0,	0,	0,	0},
  [MB_SENSOR_GPU_OUTLET] =
  {0,	75.0,	0,	0,	10.0,	0,	0,	0,	0},
  [MB_SENSOR_GPU_OUTLET_REMOTE] =
  {0,	70.0,	0,	0,	10.0,	0,	0,	0,	0},
  [MB_SENSOR_PAX01_THERM] =
  {0,	0,	0,	0,	0,	0,	0,	0,	0},
  [MB_SENSOR_PAX23_THERM] =
  {0,	0,	0,	0,	0,	0,	0,	0,	0},
  [MB_SENSOR_PAX0_THERM_REMOTE] =
  {0,	115.0,	0,	0,	10.0,	0,	0,	0,	0},
  [MB_SENSOR_PAX1_THERM_REMOTE] =
  {0,	115.0,	0,	0,	10.0,	0,	0,	0,	0},
  [MB_SENSOR_PAX2_THERM_REMOTE] =
  {0,	115.0,	0,	0,	10.0,	0,	0,	0,	0},
  [MB_SENSOR_PAX3_THERM_REMOTE] =
  {0,	115.0,	0,	0,	10.0,	0,	0,	0,	0},
  [MB_SWITCH_PAX0_DIE_TEMP] =
  {0,	115.0,	0,	0,	10.0,	0,	0,	0,	0},
  [MB_SWITCH_PAX1_DIE_TEMP] =
  {0,	115.0,	0,	0,	10.0,	0,	0,	0,	0},
  [MB_SWITCH_PAX2_DIE_TEMP] =
  {0,	115.0,	0,	0,	10.0,	0,	0,	0,	0},
  [MB_SWITCH_PAX3_DIE_TEMP] =
  {0,	115.0,	0,	0,	10.0,	0,	0,	0,	0},
  [MB_VR_P0V8_VDD0_VIN] =
  {0,	13.2,	13.0,	0,	10.8,	0,	0,	0,	0},
  [MB_VR_P0V8_VDD1_VIN] =
  {0,	13.2,	13.0,	0,	10.8,	0,	0,	0,	0},
  [MB_VR_P0V8_VDD2_VIN] =
  {0,	13.2,	13.0,	0,	10.8,	0,	0,	0,	0},
  [MB_VR_P0V8_VDD3_VIN] =
  {0,	13.2,	13.0,	0,	10.8,	0,	0,	0,	0},
  [MB_VR_P0V8_VDD0_VOUT] =
  {0,	0.861,	0,	0,	0.819,	0,	0,	0,	0},
  [MB_VR_P0V8_VDD1_VOUT] =
  {0,	0.861,	0,	0,	0.819,	0,	0,	0,	0},
  [MB_VR_P0V8_VDD2_VOUT] =
  {0,	0.861,	0,	0,	0.819,	0,	0,	0,	0},
  [MB_VR_P0V8_VDD3_VOUT] =
  {0,	0.861,	0,	0,	0.819,	0,	0,	0,	0},
  [MB_VR_P0V8_VDD0_CURR] =
  {0,	55.8,	0,	0,	0,	0,	0,	0,	0},
  [MB_VR_P0V8_VDD1_CURR] =
  {0,	55.8,	0,	0,	0,	0,	0,	0,	0},
  [MB_VR_P0V8_VDD2_CURR] =
  {0,	55.8,	0,	0,	0,	0,	0,	0,	0},
  [MB_VR_P0V8_VDD3_CURR] =
  {0,	55.8,	0,	0,	0,	0,	0,	0,	0},
  [MB_VR_P0V8_VDD0_TEMP] =
  {0,	115.0,	0,	0,	10.0,	0,	0,	0,	0},
  [MB_VR_P0V8_VDD1_TEMP] =
  {0,	115.0,	0,	0,	10.0,	0,	0,	0,	0},
  [MB_VR_P0V8_VDD2_TEMP] =
  {0,	115.0,	0,	0,	10.0,	0,	0,	0,	0},
  [MB_VR_P0V8_VDD3_TEMP] =
  {0,	115.0,	0,	0,	10.0,	0,	0,	0,	0},
  [MB_VR_P1V0_AVD0_VIN] =
  {0,	13.2,	13,	0,	10.8,	0,	0,	0,	0},
  [MB_VR_P1V0_AVD1_VIN] =
  {0,	13.2,	13,	0,	10.8,	0,	0,	0,	0},
  [MB_VR_P1V0_AVD2_VIN] =
  {0,	13.2,	13,	0,	10.8,	0,	0,	0,	0},
  [MB_VR_P1V0_AVD3_VIN] =
  {0,	13.2,	13,	0,	10.8,	0,	0,	0,	0},
  [MB_VR_P1V0_AVD0_VOUT] =
  {0,	1.025,	0,	0,	0.975,	0,	0,	0,	0},
  [MB_VR_P1V0_AVD1_VOUT] =
  {0,	1.025,	0,	0,	0.975,	0,	0,	0,	0},
  [MB_VR_P1V0_AVD2_VOUT] =
  {0,	1.025,	0,	0,	0.975,	0,	0,	0,	0},
  [MB_VR_P1V0_AVD3_VOUT] =
  {0,	1.025,	0,	0,	0.975,	0,	0,	0,	0},
  [MB_VR_P1V0_AVD0_CURR] =
  {0,	24.0,	0,	0,	0,	0,	0,	0,	0},
  [MB_VR_P1V0_AVD1_CURR] =
  {0,	24.0,	0,	0,	0,	0,	0,	0,	0},
  [MB_VR_P1V0_AVD2_CURR] =
  {0,	24.0,	0,	0,	0,	0,	0,	0,	0},
  [MB_VR_P1V0_AVD3_CURR] =
  {0,	24.0,	0,	0,	0,	0,	0,	0,	0},
  [MB_VR_P1V0_AVD0_TEMP] =
  {0,	115.0,	0,	0,	10.0,	0,	0,	0,	0},
  [MB_VR_P1V0_AVD1_TEMP] =
  {0,	115.0,	0,	0,	10.0,	0,	0,	0,	0},
  [MB_VR_P1V0_AVD2_TEMP] =
  {0,	115.0,	0,	0,	10.0,	0,	0,	0,	0},
  [MB_VR_P1V0_AVD3_TEMP] =
  {0,	115.0,	0,	0,	10.0,	0,	0,	0,	0},
  [PDB_HSC_P12V_AUX_VIN] =
  {0,	13.2,	13,	0,	10.8,	0,	0,	0,	0},
  [PDB_HSC_P12V_1_VIN] =
  {0,	13.2,	13,	0,	10.8,	0,	0,	0,	0},
  [PDB_HSC_P12V_2_VIN] =
  {0,	13.2,	13,	0,	10.8,	0,	0,	0,	0},
  [PDB_HSC_P12V_AUX_VOUT] =
  {0,	13.2,	13,	0,	10.8,	0,	0,	0,	0},
  [PDB_HSC_P12V_1_VOUT] =
  {0,	13.2,	13,	0,	10.8,	0,	0,	0,	0},
  [PDB_HSC_P12V_2_VOUT] =
  {0,	13.2,	13,	0,	10.8,	0,	0,	0,	0},
  [PDB_HSC_P12V_AUX_CURR] =
  {0,	50.0,	42.0,	0,	0,	0,	0,	0,	0},
  [PDB_HSC_P12V_1_CURR] =
  {0,	343.75,	290.84,	0,	0,	0,	0,	0,	0},
  [PDB_HSC_P12V_2_CURR] =
  {0,	343.75,	290.84,	0,	0,	0,	0,	0,	0},
  [PDB_HSC_P12V_AUX_PWR] =
  {0,	600.0,	0,	0,	0,	0,	0,	0,	0},
  [PDB_HSC_P12V_1_PWR] =
  {0,	4125.0,	0,	0,	0,	0,	0,	0,	0},
  [PDB_HSC_P12V_2_PWR] =
  {0,	4125.0,	0,	0,	0,	0,	0,	0,	0},
  [PDB_HSC_P48V_1_VIN] =
  {0,	60,	0,	0,	44,	0,	0,	0,	0},
  [PDB_HSC_P48V_1_VOUT] =
  {0,	60,	0,	0,	44,	0,	0,	0,	0},
  [PDB_HSC_P48V_1_CURR] =
  {0,	63,	50,	0,	0,	0,	0,	0,	0},
  [PDB_HSC_P48V_1_PWR] =
  {0,	3000,	2400,	0,	0,	0,	0,	0,	0},
  [PDB_HSC_P48V_2_VIN] =
  {0,	60,	0,	0,	44,	0,	0,	0,	0},
  [PDB_HSC_P48V_2_VOUT] =
  {0,	60,	0,	0,	44,	0,	0,	0,	0},
  [PDB_HSC_P48V_2_CURR] =
  {0,	63,	50,	0,	0,	0,	0,	0,	0},
  [PDB_HSC_P48V_2_PWR] =
  {0,	3000,	2400,	0,	0,	0,	0,	0,	0},
  [PDB_ADC_1_VICOR0_TEMP] =
  {0,	115.0,	0,	0,	10.0,	0,	0,	0,	0},
  [PDB_ADC_1_VICOR1_TEMP] =
  {0,	115.0,	0,	0,	10.0,	0,	0,	0,	0},
  [PDB_ADC_1_VICOR2_TEMP] =
  {0,	115.0,	0,	0,	10.0,	0,	0,	0,	0},
  [PDB_ADC_1_VICOR3_TEMP] =
  {0,	115.0,	0,	0,	10.0,	0,	0,	0,	0},
  [PDB_ADC_2_VICOR0_TEMP] =
  {0,	115.0,	0,	0,	10.0,	0,	0,	0,	0},
  [PDB_ADC_2_VICOR1_TEMP] =
  {0,	115.0,	0,	0,	10.0,	0,	0,	0,	0},
  [PDB_ADC_2_VICOR2_TEMP] =
  {0,	115.0,	0,	0,	10.0,	0,	0,	0,	0},
  [PDB_ADC_2_VICOR3_TEMP] =
  {0,	115.0,	0,	0,	10.0,	0,	0,	0,	0},
  [PDB_SENSOR_OUTLET_TEMP] =
  {0,	75.0,	0,	0,	10.0,	0,	0,	0,	0},
  [PDB_SENSOR_OUTLET_TEMP_REMOTE] =
  {0,	70.0,	0,	0,	10.0,	0,	0,	0,	0}
};

struct sensor_map {
  int (*sensor_read)(uint8_t, float*);
  const char* name;
  int unit;
} fbep_sensors_map[] = {
  [MB_FAN0_TACH_I] =
  {sensors_read_common_fan, "MB_FAN0_TACH_I", SNR_TACH},
  [MB_FAN0_TACH_O] =
  {sensors_read_common_fan, "MB_FAN0_TACH_O", SNR_TACH},
  [MB_FAN1_TACH_I] =
  {sensors_read_common_fan, "MB_FAN1_TACH_I", SNR_TACH},
  [MB_FAN1_TACH_O] =
  {sensors_read_common_fan, "MB_FAN1_TACH_O", SNR_TACH},
  [MB_FAN2_TACH_I] =
  {sensors_read_common_fan, "MB_FAN2_TACH_I", SNR_TACH},
  [MB_FAN2_TACH_O] =
  {sensors_read_common_fan, "MB_FAN2_TACH_O", SNR_TACH},
  [MB_FAN3_TACH_I] =
  {sensors_read_common_fan, "MB_FAN3_TACH_I", SNR_TACH},
  [MB_FAN3_TACH_O] =
  {sensors_read_common_fan, "MB_FAN3_TACH_O", SNR_TACH},
  [MB_FAN0_VOLT] =
  {sensors_read_common_fan, "MB_FAN0_VOLT", SNR_VOLT},
  [MB_FAN1_VOLT] =
  {sensors_read_common_fan, "MB_FAN1_VOLT", SNR_VOLT},
  [MB_FAN2_VOLT] =
  {sensors_read_common_fan, "MB_FAN2_VOLT", SNR_VOLT},
  [MB_FAN3_VOLT] =
  {sensors_read_common_fan, "MB_FAN3_VOLT", SNR_VOLT},
  [MB_FAN0_CURR] =
  {sensors_read_common_fan, "MB_FAN0_CURR", SNR_CURR},
  [MB_FAN1_CURR] =
  {sensors_read_common_fan, "MB_FAN1_CURR", SNR_CURR},
  [MB_FAN2_CURR] =
  {sensors_read_common_fan, "MB_FAN2_CURR", SNR_CURR},
  [MB_FAN3_CURR] =
  {sensors_read_common_fan, "MB_FAN3_CURR", SNR_CURR},
  [MB_ADC_P12V_AUX] =
  {sensors_read_common_adc, "MB_ADC_P12V_AUX", SNR_VOLT},
  [MB_ADC_P12V_1] =
  {sensors_read_common_adc, "MB_ADC_P12V_1", SNR_VOLT},
  [MB_ADC_P12V_2] =
  {sensors_read_common_adc, "MB_ADC_P12V_2", SNR_VOLT},
  [MB_ADC_P3V3_STBY] =
  {sensors_read_common_adc, "MB_ADC_P3V3_STBY", SNR_VOLT},
  [MB_ADC_P3V3] =
  {sensors_read_common_adc, "MB_ADC_P3V3", SNR_VOLT},
  [MB_ADC_P3V_BAT] =
  {read_battery_value, "MB_ADC_P3V_BAT", SNR_VOLT},
  [MB_ADC_P5V_STBY] =
  {sensors_read_common_adc, "MB_ADC_P5V_STBY", SNR_VOLT},
  [MB_SENSOR_GPU_INLET] =
  {sensors_read_common_therm, "MB_SENSOR_GPU_INLET", SNR_TEMP},
  [MB_SENSOR_GPU_INLET_REMOTE] =
  {sensors_read_common_therm, "MB_SENSOR_GPU_INLET_REMOTE", SNR_TEMP},
  [MB_SENSOR_GPU_OUTLET] =
  {sensors_read_common_therm, "MB_SENSOR_GPU_OUTLET", SNR_TEMP},
  [MB_SENSOR_GPU_OUTLET_REMOTE] =
  {sensors_read_common_therm, "MB_SENSOR_GPU_OUTLET_REMOTE", SNR_TEMP},
  [MB_SENSOR_PAX01_THERM] =
  {sensors_read_pax_therm, "MB_SENSOR_PAX01_THERM", SNR_TEMP},
  [MB_SENSOR_PAX23_THERM] =
  {sensors_read_pax_therm, "MB_SENSOR_PAX23_THERM", SNR_TEMP},
  [MB_SENSOR_PAX0_THERM_REMOTE] =
  {sensors_read_pax_therm, "MB_SENSOR_PAX0_THERM", SNR_TEMP},
  [MB_SENSOR_PAX1_THERM_REMOTE] =
  {sensors_read_pax_therm, "MB_SENSOR_PAX1_THERM", SNR_TEMP},
  [MB_SENSOR_PAX2_THERM_REMOTE] =
  {sensors_read_pax_therm, "MB_SENSOR_PAX2_THERM", SNR_TEMP},
  [MB_SENSOR_PAX3_THERM_REMOTE] =
  {sensors_read_pax_therm, "MB_SENSOR_PAX3_THERM", SNR_TEMP},
  [MB_SWITCH_PAX0_DIE_TEMP] =
  {read_pax_dietemp, "MB_SWITCH_PAX0_DIE_TEMP", SNR_TEMP},
  [MB_SWITCH_PAX1_DIE_TEMP] =
  {read_pax_dietemp, "MB_SWITCH_PAX1_DIE_TEMP", SNR_TEMP},
  [MB_SWITCH_PAX2_DIE_TEMP] =
  {read_pax_dietemp, "MB_SWITCH_PAX2_DIE_TEMP", SNR_TEMP},
  [MB_SWITCH_PAX3_DIE_TEMP] =
  {read_pax_dietemp, "MB_SWITCH_PAX3_DIE_TEMP", SNR_TEMP},
  [MB_VR_P0V8_VDD0_VIN] =
  {sensors_read_vr, "MB_VR_P0V8_VDD0_VIN", SNR_VOLT},
  [MB_VR_P0V8_VDD1_VIN] =
  {sensors_read_vr, "MB_VR_P0V8_VDD1_VIN", SNR_VOLT},
  [MB_VR_P0V8_VDD2_VIN] =
  {sensors_read_vr, "MB_VR_P0V8_VDD2_VIN", SNR_VOLT},
  [MB_VR_P0V8_VDD3_VIN] =
  {sensors_read_vr, "MB_VR_P0V8_VDD3_VIN", SNR_VOLT},
  [MB_VR_P0V8_VDD0_VOUT] =
  {sensors_read_vr, "MB_VR_P0V8_VDD0_VOUT", SNR_VOLT},
  [MB_VR_P0V8_VDD1_VOUT] =
  {sensors_read_vr, "MB_VR_P0V8_VDD1_VOUT", SNR_VOLT},
  [MB_VR_P0V8_VDD2_VOUT] =
  {sensors_read_vr, "MB_VR_P0V8_VDD2_VOUT", SNR_VOLT},
  [MB_VR_P0V8_VDD3_VOUT] =
  {sensors_read_vr, "MB_VR_P0V8_VDD3_VOUT", SNR_VOLT},
  [MB_VR_P0V8_VDD0_CURR] =
  {sensors_read_vr, "MB_VR_P0V8_VDD0_CURR", SNR_CURR},
  [MB_VR_P0V8_VDD1_CURR] =
  {sensors_read_vr, "MB_VR_P0V8_VDD1_CURR", SNR_CURR},
  [MB_VR_P0V8_VDD2_CURR] =
  {sensors_read_vr, "MB_VR_P0V8_VDD2_CURR", SNR_CURR},
  [MB_VR_P0V8_VDD3_CURR] =
  {sensors_read_vr, "MB_VR_P0V8_VDD3_CURR", SNR_CURR},
  [MB_VR_P0V8_VDD0_TEMP] =
  {sensors_read_vr, "MB_VR_P0V8_VDD0_TEMP", SNR_TEMP},
  [MB_VR_P0V8_VDD1_TEMP] =
  {sensors_read_vr, "MB_VR_P0V8_VDD1_TEMP", SNR_TEMP},
  [MB_VR_P0V8_VDD2_TEMP] =
  {sensors_read_vr, "MB_VR_P0V8_VDD2_TEMP", SNR_TEMP},
  [MB_VR_P0V8_VDD3_TEMP] =
  {sensors_read_vr, "MB_VR_P0V8_VDD3_TEMP", SNR_TEMP},
  [MB_VR_P1V0_AVD0_VIN] =
  {sensors_read_vr, "MB_VR_P1V0_AVD0_VIN", SNR_VOLT},
  [MB_VR_P1V0_AVD1_VIN] =
  {sensors_read_vr, "MB_VR_P1V0_AVD1_VIN", SNR_VOLT},
  [MB_VR_P1V0_AVD2_VIN] =
  {sensors_read_vr, "MB_VR_P1V0_AVD2_VIN", SNR_VOLT},
  [MB_VR_P1V0_AVD3_VIN] =
  {sensors_read_vr, "MB_VR_P1V0_AVD3_VIN", SNR_VOLT},
  [MB_VR_P1V0_AVD0_VOUT] =
  {sensors_read_vr, "MB_VR_P1V0_AVD0_VOUT", SNR_VOLT},
  [MB_VR_P1V0_AVD1_VOUT] =
  {sensors_read_vr, "MB_VR_P1V0_AVD1_VOUT", SNR_VOLT},
  [MB_VR_P1V0_AVD2_VOUT] =
  {sensors_read_vr, "MB_VR_P1V0_AVD2_VOUT", SNR_VOLT},
  [MB_VR_P1V0_AVD3_VOUT] =
  {sensors_read_vr, "MB_VR_P1V0_AVD3_VOUT", SNR_VOLT},
  [MB_VR_P1V0_AVD0_CURR] =
  {sensors_read_vr, "MB_VR_P1V0_AVD0_CURR", SNR_CURR},
  [MB_VR_P1V0_AVD1_CURR] =
  {sensors_read_vr, "MB_VR_P1V0_AVD1_CURR", SNR_CURR},
  [MB_VR_P1V0_AVD2_CURR] =
  {sensors_read_vr, "MB_VR_P1V0_AVD2_CURR", SNR_CURR},
  [MB_VR_P1V0_AVD3_CURR] =
  {sensors_read_vr, "MB_VR_P1V0_AVD3_CURR", SNR_CURR},
  [MB_VR_P1V0_AVD0_TEMP] =
  {sensors_read_vr, "MB_VR_P1V0_AVD0_TEMP", SNR_TEMP},
  [MB_VR_P1V0_AVD1_TEMP] =
  {sensors_read_vr, "MB_VR_P1V0_AVD1_TEMP", SNR_TEMP},
  [MB_VR_P1V0_AVD2_TEMP] =
  {sensors_read_vr, "MB_VR_P1V0_AVD2_TEMP", SNR_TEMP},
  [MB_VR_P1V0_AVD3_TEMP] =
  {sensors_read_vr, "MB_VR_P1V0_AVD3_TEMP", SNR_TEMP},
  [PDB_HSC_P12V_AUX_VIN] =
  {sensors_read_12v_hsc, "PDB_HSC_P12V_AUX_VIN", SNR_VOLT},
  [PDB_HSC_P12V_1_VIN] =
  {sensors_read_12v_hsc, "PDB_HSC_P12V_1_VIN", SNR_VOLT},
  [PDB_HSC_P12V_2_VIN] =
  {sensors_read_12v_hsc, "PDB_HSC_P12V_2_VIN", SNR_VOLT},
  [PDB_HSC_P12V_AUX_VOUT] =
  {sensors_read_12v_hsc, "PDB_HSC_P12V_AUX_VOUT", SNR_VOLT},
  [PDB_HSC_P12V_1_VOUT] =
  {sensors_read_12v_hsc, "PDB_HSC_P12V_1_VOUT", SNR_VOLT},
  [PDB_HSC_P12V_2_VOUT] =
  {sensors_read_12v_hsc, "PDB_HSC_P12V_2_VOUT", SNR_VOLT},
  [PDB_HSC_P12V_AUX_CURR] =
  {sensors_read_12v_hsc, "PDB_HSC_P12V_AUX_CURR", SNR_CURR},
  [PDB_HSC_P12V_1_CURR] =
  {sensors_read_12v_hsc, "PDB_HSC_P12V_1_CURR", SNR_CURR},
  [PDB_HSC_P12V_2_CURR] =
  {sensors_read_12v_hsc, "PDB_HSC_P12V_2_CURR", SNR_CURR},
  [PDB_HSC_P12V_AUX_PWR] =
  {sensors_read_12v_hsc, "PDB_HSC_P12V_AUX_PWR", SNR_PWR},
  [PDB_HSC_P12V_1_PWR] =
  {sensors_read_12v_hsc, "PDB_HSC_P12V_1_PWR", SNR_PWR},
  [PDB_HSC_P12V_2_PWR] =
  {sensors_read_12v_hsc, "PDB_HSC_P12V_2_PWR", SNR_PWR},
  [PDB_HSC_P48V_1_VIN] =
  {sensors_read_48v_hsc, "PDB_HSC_P48V_1_VIN", SNR_VOLT},
  [PDB_HSC_P48V_1_VOUT] =
  {sensors_read_48v_hsc, "PDB_HSC_P48V_1_VOUT", SNR_VOLT},
  [PDB_HSC_P48V_1_CURR] =
  {sensors_read_48v_hsc, "PDB_HSC_P48V_1_CURR", SNR_CURR},
  [PDB_HSC_P48V_1_PWR] =
  {sensors_read_48v_hsc, "PDB_HSC_P48V_1_PWR", SNR_PWR},
  [PDB_HSC_P48V_2_VIN] =
  {sensors_read_48v_hsc, "PDB_HSC_P48V_2_VIN", SNR_VOLT},
  [PDB_HSC_P48V_2_VOUT] =
  {sensors_read_48v_hsc, "PDB_HSC_P48V_2_VOUT", SNR_VOLT},
  [PDB_HSC_P48V_2_CURR] =
  {sensors_read_48v_hsc, "PDB_HSC_P48V_2_CURR", SNR_CURR},
  [PDB_HSC_P48V_2_PWR] =
  {sensors_read_48v_hsc, "PDB_HSC_P48V_2_PWR", SNR_PWR},
  [PDB_ADC_1_VICOR0_TEMP] =
  {sensors_read_vicor, "PDB_ADC_1_VICOR0_TEMP", SNR_TEMP},
  [PDB_ADC_1_VICOR1_TEMP] =
  {sensors_read_vicor, "PDB_ADC_1_VICOR1_TEMP", SNR_TEMP},
  [PDB_ADC_1_VICOR2_TEMP] =
  {sensors_read_vicor, "PDB_ADC_1_VICOR2_TEMP", SNR_TEMP},
  [PDB_ADC_1_VICOR3_TEMP] =
  {sensors_read_vicor, "PDB_ADC_1_VICOR3_TEMP", SNR_TEMP},
  [PDB_ADC_2_VICOR0_TEMP] =
  {sensors_read_vicor, "PDB_ADC_2_VICOR0_TEMP", SNR_TEMP},
  [PDB_ADC_2_VICOR1_TEMP] =
  {sensors_read_vicor, "PDB_ADC_2_VICOR1_TEMP", SNR_TEMP},
  [PDB_ADC_2_VICOR2_TEMP] =
  {sensors_read_vicor, "PDB_ADC_2_VICOR2_TEMP", SNR_TEMP},
  [PDB_ADC_2_VICOR3_TEMP] =
  {sensors_read_vicor, "PDB_ADC_2_VICOR3_TEMP", SNR_TEMP},
  [PDB_SENSOR_OUTLET_TEMP] =
  {sensors_read_common_therm, "PDB_SENSOR_OUTLET_TEMP", SNR_TEMP},
  [PDB_SENSOR_OUTLET_TEMP_REMOTE] =
  {sensors_read_common_therm, "PDB_SENSOR_OUTLET_TEMP_REMOTE", SNR_TEMP},
};

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

static int read_battery_value(uint8_t sensor_num, float *value)
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

static int read_pax_dietemp(uint8_t sensor_num, float *value)
{
  int ret = 0;
  uint8_t addr, status;
  uint8_t paxid = sensor_num - MB_SWITCH_PAX0_DIE_TEMP;
  uint32_t temp, sub_cmd_id;
  char device_name[LARGEST_DEVICE_NAME] = {0};
  struct switchtec_dev *dev;

  if (pal_get_server_power(FRU_MB, &status) < 0 || status == SERVER_POWER_OFF
      || pal_is_pax_proc_ongoing(paxid)) {
    return READING_NA;
  }

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
  val = (float)(val*0.2-273.0);
  if (val < 0)
    return READING_NA;

  *value = val;

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

static int sensors_read_common_fan(uint8_t sensor_num, float *value)
{
  int ret;
  uint8_t status;

  if (pal_get_server_power(FRU_MB, &status) < 0 || status == SERVER_POWER_OFF)
    return READING_NA;

  switch (sensor_num) {
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
    default:
      return READING_NA;
  }

  return ret;
}

static int sensors_read_common_adc(uint8_t sensor_num, float *value)
{
  int ret;

  switch (sensor_num) {
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
    default:
      return READING_NA;
  }

  return ret;
}

static int sensors_read_common_therm(uint8_t sensor_num, float *value)
{
  int ret;

  switch (sensor_num) {
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
    case PDB_SENSOR_OUTLET_TEMP:
      ret = sensors_read("tmp421-i2c-17-4c", "OUTLET_TEMP", (float *)value);
      break;
    case PDB_SENSOR_OUTLET_TEMP_REMOTE:
      ret = sensors_read("tmp421-i2c-17-4c", "OUTLET_TEMP_REMOTE", (float *)value);
      break;
    default:
      return READING_NA;
  }

  return ret;
}

static int sensors_read_pax_therm(uint8_t sensor_num, float *value)
{
  int ret;
  uint8_t status;

  if (pal_get_server_power(FRU_MB, &status) < 0 || status == SERVER_POWER_OFF)
    return READING_NA;

  switch (sensor_num) {
    case MB_SENSOR_PAX01_THERM:
      ret = sensors_read("tmp422-i2c-6-4d", "PAX01_THERM", value);
      break;
    case MB_SENSOR_PAX0_THERM_REMOTE:
      ret = sensors_read("tmp422-i2c-6-4d", "PAX0_THERM_REMOTE", value);
      break;
    case MB_SENSOR_PAX1_THERM_REMOTE:
      ret = sensors_read("tmp422-i2c-6-4d", "PAX1_THERM_REMOTE", value);
      break;
    case MB_SENSOR_PAX23_THERM:
      ret = sensors_read("tmp422-i2c-6-4e", "PAX23_THERM", value);
      break;
    case MB_SENSOR_PAX2_THERM_REMOTE:
      ret = sensors_read("tmp422-i2c-6-4e", "PAX2_THERM_REMOTE", value);
      break;
    case MB_SENSOR_PAX3_THERM_REMOTE:
      ret = sensors_read("tmp422-i2c-6-4e", "PAX3_THERM_REMOTE", value);
      break;
    default:
      return READING_NA;
  }

  return ret;
}

static int sensors_read_vr(uint8_t sensor_num, float *value)
{
  int ret;
  uint8_t status;

  if (pal_get_server_power(FRU_MB, &status) < 0 || status == SERVER_POWER_OFF)
    return READING_NA;

  switch (sensor_num) {
    case MB_VR_P0V8_VDD0_VIN:
      ret = sensors_read("mpq8645p-i2c-5-30", "VR_P0V8_VDD0_VIN", value);
      break;
    case MB_VR_P0V8_VDD0_VOUT:
      ret = sensors_read("mpq8645p-i2c-5-30", "VR_P0V8_VDD0_VOUT", value);
      break;
    case MB_VR_P0V8_VDD0_CURR:
      ret = sensors_read("mpq8645p-i2c-5-30", "VR_P0V8_VDD0_CURR", value);
      break;
    case MB_VR_P0V8_VDD0_TEMP:
      ret = sensors_read("mpq8645p-i2c-5-30", "VR_P0V8_VDD0_TEMP", value);
      break;
    case MB_VR_P0V8_VDD1_VIN:
      ret = sensors_read("mpq8645p-i2c-5-31", "VR_P0V8_VDD1_VIN", value);
      break;
    case MB_VR_P0V8_VDD1_VOUT:
      ret = sensors_read("mpq8645p-i2c-5-31", "VR_P0V8_VDD1_VOUT", value);
      break;
    case MB_VR_P0V8_VDD1_CURR:
      ret = sensors_read("mpq8645p-i2c-5-31", "VR_P0V8_VDD1_CURR", value);
      break;
    case MB_VR_P0V8_VDD1_TEMP:
      ret = sensors_read("mpq8645p-i2c-5-31", "VR_P0V8_VDD1_TEMP", value);
      break;
    case MB_VR_P0V8_VDD2_VIN:
      ret = sensors_read("mpq8645p-i2c-5-32", "VR_P0V8_VDD2_VIN", value);
      break;
    case MB_VR_P0V8_VDD2_VOUT:
      ret = sensors_read("mpq8645p-i2c-5-32", "VR_P0V8_VDD2_VOUT", value);
      break;
    case MB_VR_P0V8_VDD2_CURR:
      ret = sensors_read("mpq8645p-i2c-5-32", "VR_P0V8_VDD2_CURR", value);
      break;
    case MB_VR_P0V8_VDD2_TEMP:
      ret = sensors_read("mpq8645p-i2c-5-32", "VR_P0V8_VDD2_TEMP", value);
      break;
    case MB_VR_P0V8_VDD3_VIN:
      ret = sensors_read("mpq8645p-i2c-5-33", "VR_P0V8_VDD3_VIN", value);
      break;
    case MB_VR_P0V8_VDD3_VOUT:
      ret = sensors_read("mpq8645p-i2c-5-33", "VR_P0V8_VDD3_VOUT", value);
      break;
    case MB_VR_P0V8_VDD3_CURR:
      ret = sensors_read("mpq8645p-i2c-5-33", "VR_P0V8_VDD3_CURR", value);
      break;
    case MB_VR_P0V8_VDD3_TEMP:
      ret = sensors_read("mpq8645p-i2c-5-33", "VR_P0V8_VDD3_TEMP", value);
      break;
    case MB_VR_P1V0_AVD0_VIN:
      ret = sensors_read("mpq8645p-i2c-5-34", "VR_P1V0_AVD0_VIN", value);
      break;
    case MB_VR_P1V0_AVD0_VOUT:
      ret = sensors_read("mpq8645p-i2c-5-34", "VR_P1V0_AVD0_VOUT", value);
      break;
    case MB_VR_P1V0_AVD0_CURR:
      ret = sensors_read("mpq8645p-i2c-5-34", "VR_P1V0_AVD0_CURR", value);
      break;
    case MB_VR_P1V0_AVD0_TEMP:
      ret = sensors_read("mpq8645p-i2c-5-34", "VR_P1V0_AVD0_TEMP", value);
      break;
    case MB_VR_P1V0_AVD1_VIN:
      ret = sensors_read("mpq8645p-i2c-5-35", "VR_P1V0_AVD1_VIN", value);
      break;
    case MB_VR_P1V0_AVD1_VOUT:
      ret = sensors_read("mpq8645p-i2c-5-35", "VR_P1V0_AVD1_VOUT", value);
      break;
    case MB_VR_P1V0_AVD1_CURR:
      ret = sensors_read("mpq8645p-i2c-5-35", "VR_P1V0_AVD1_CURR", value);
      break;
    case MB_VR_P1V0_AVD1_TEMP:
      ret = sensors_read("mpq8645p-i2c-5-35", "VR_P1V0_AVD1_TEMP", value);
      break;
    case MB_VR_P1V0_AVD2_VIN:
      ret = sensors_read("mpq8645p-i2c-5-36", "VR_P1V0_AVD2_VIN", value);
      break;
    case MB_VR_P1V0_AVD2_VOUT:
      ret = sensors_read("mpq8645p-i2c-5-36", "VR_P1V0_AVD2_VOUT", value);
      break;
    case MB_VR_P1V0_AVD2_CURR:
      ret = sensors_read("mpq8645p-i2c-5-36", "VR_P1V0_AVD2_CURR", value);
      break;
    case MB_VR_P1V0_AVD2_TEMP:
      ret = sensors_read("mpq8645p-i2c-5-36", "VR_P1V0_AVD2_TEMP", value);
      break;
    case MB_VR_P1V0_AVD3_VIN:
      ret = sensors_read("mpq8645p-i2c-5-3b", "VR_P1V0_AVD3_VIN", value);
      break;
    case MB_VR_P1V0_AVD3_VOUT:
      ret = sensors_read("mpq8645p-i2c-5-3b", "VR_P1V0_AVD3_VOUT", value);
      break;
    case MB_VR_P1V0_AVD3_CURR:
      ret = sensors_read("mpq8645p-i2c-5-3b", "VR_P1V0_AVD3_CURR", value);
      break;
    case MB_VR_P1V0_AVD3_TEMP:
      ret = sensors_read("mpq8645p-i2c-5-3b", "VR_P1V0_AVD3_TEMP", value);
      break;
    default:
      ret = READING_NA;
  }

  return ret;
}

static int sensors_read_12v_hsc(uint8_t sensor_num, float *value)
{
  int ret;

  switch (sensor_num) {
    case PDB_HSC_P12V_1_CURR:
      ret = sensors_read("ltc4282-i2c-16-53", "P12V_1_CURR", value);
      if (ret == 0)
        hsc_value_adjust(current_table, value);
      break;
    case PDB_HSC_P12V_2_CURR:
      ret = sensors_read("ltc4282-i2c-17-40", "P12V_2_CURR", value);
      if (ret == 0)
        hsc_value_adjust(current_table, value);
      break;
    case PDB_HSC_P12V_AUX_CURR:
      ret = sensors_read("ltc4282-i2c-18-43", "P12V_AUX_CURR", value);
      if (ret == 0)
        hsc_value_adjust(aux_current_table, value);
      break;
    case PDB_HSC_P12V_1_PWR:
      ret = sensors_read("ltc4282-i2c-16-53", "P12V_1_PWR", value);
      if (ret == 0)
        hsc_value_adjust(power_table, value);
      break;
    case PDB_HSC_P12V_2_PWR:
      ret = sensors_read("ltc4282-i2c-17-40", "P12V_2_PWR", value);
      if (ret == 0)
        hsc_value_adjust(power_table, value);
      break;
    case PDB_HSC_P12V_AUX_PWR:
      ret = sensors_read("ltc4282-i2c-18-43", "P12V_AUX_PWR", value);
      if (ret == 0)
        hsc_value_adjust(aux_power_table, value);
      break;
    case PDB_HSC_P12V_1_VIN:
      ret = sensors_read("ltc4282-i2c-16-53", "P12V_1_VIN", value);
      break;
    case PDB_HSC_P12V_2_VIN:
      ret = sensors_read("ltc4282-i2c-17-40", "P12V_2_VIN", value);
      break;
    case PDB_HSC_P12V_AUX_VIN:
      ret = sensors_read("ltc4282-i2c-18-43", "P12V_AUX_VIN", value);
      break;
    case PDB_HSC_P12V_1_VOUT:
      ret = sensors_read("ltc4282-i2c-16-53", "P12V_1_VOUT", value);
      break;
    case PDB_HSC_P12V_2_VOUT:
      ret = sensors_read("ltc4282-i2c-17-40", "P12V_2_VOUT", value);
      break;
    case PDB_HSC_P12V_AUX_VOUT:
      ret = sensors_read("ltc4282-i2c-18-43", "P12V_AUX_VOUT", value);
      break;
    default:
      ret = READING_NA;
  }

  return ret;
}

static int sensors_read_48v_hsc(uint8_t sensor_num, float *value)
{
  int ret;
  uint8_t status;

  if (pal_get_server_power(FRU_MB, &status) < 0 || status == SERVER_POWER_OFF)
    return READING_NA;

  switch (sensor_num) {
    case PDB_HSC_P48V_1_VIN:
      ret = sensors_read("adm1272-i2c-16-13", "P48V_1_VIN", value);
      break;
    case PDB_HSC_P48V_2_VIN:
      ret = sensors_read("adm1272-i2c-17-10", "P48V_2_VIN", value);
      break;
    case PDB_HSC_P48V_1_VOUT:
      ret = sensors_read("adm1272-i2c-16-13", "P48V_1_VOUT", value);
      break;
    case PDB_HSC_P48V_2_VOUT:
      ret = sensors_read("adm1272-i2c-17-10", "P48V_2_VOUT", value);
      break;
    case PDB_HSC_P48V_1_CURR:
      ret = sensors_read("adm1272-i2c-16-13", "P48V_1_CURR", value);
      break;
    case PDB_HSC_P48V_2_CURR:
      ret = sensors_read("adm1272-i2c-17-10", "P48V_2_CURR", value);
      break;
    case PDB_HSC_P48V_1_PWR:
      ret = sensors_read("adm1272-i2c-16-13", "P48V_1_PWR", value);
      break;
    case PDB_HSC_P48V_2_PWR:
      ret = sensors_read("adm1272-i2c-17-10", "P48V_2_PWR", value);
      break;
    default:
      ret = READING_NA;
  }

  if (*value < 0)
    *value = 0.0;

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

int pal_get_sensor_threshold(uint8_t fru, uint8_t sensor_num, uint8_t thresh, void *value)
{
  float *val = (float*) value;

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
    sprintf(name, "%s", fbep_sensors_map[sensor_num].name);

  return 0;
}

int pal_get_sensor_units(uint8_t fru, uint8_t sensor_num, char *units)
{
  if (fru > FRU_PDB || sensor_num >= FBEP_SENSOR_MAX)
    return -1;

  switch(fbep_sensors_map[sensor_num].unit) {
    case SNR_TACH:
      sprintf(units, "RPM");
      break;
    case SNR_VOLT:
      sprintf(units, "Volts");
      break;
    case SNR_TEMP:
      sprintf(units, "C");
      break;
    case SNR_CURR:
      sprintf(units, "Amps");
      break;
    case SNR_PWR:
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

  if (fru > FRU_PDB || sensor_num >= FBEP_SENSOR_MAX)
    return -1;

  ret = fbep_sensors_map[sensor_num].sensor_read(sensor_num, (float *)value);

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

