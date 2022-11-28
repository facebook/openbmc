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
#include <openbmc/obmc-i2c.h>
#include <switchtec/switchtec.h>
#include <facebook/asic.h>
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

static int sensors_read_fan_speed(uint8_t, float*);
static int sensors_read_fan_health(uint8_t, float*);
static int sensors_read_common_adc(uint8_t, float*);
static int sensors_read_12v_adc(uint8_t, float*);
static int read_battery_value(uint8_t, float*);
static int sensors_read_vicor(uint8_t, float*);
static int sensors_read_common_therm(uint8_t, float*);
static int sensors_read_pax_therm(uint8_t, float*);
static int sensors_read_vr(uint8_t, float*);
static int sensors_read_12v_hsc(uint8_t, float*);
static int sensors_read_12v_hsc_vout(uint8_t, float*);
static int sensors_read_48v_hsc(uint8_t, float*);
static int sensors_read_48v_hsc_vout(uint8_t, float*);
static int read_gpu_temp(uint8_t, float*);
static int read_asic_board_temp(uint8_t, float*);
static int read_asic_mem_temp(uint8_t, float*);
static int read_gpu_pwcs(uint8_t, float*);
static int sensors_read_infineon(uint8_t, float*);
static int sensors_read_brcm_vr(uint8_t, float*);

/*
 * List of sensors to be monitored
 */
const uint8_t mb_micro_switch_sensor_list[] = {
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
  MB_SENSOR_INLET,
  MB_SENSOR_INLET_REMOTE,
  MB_SENSOR_OUTLET,
  MB_SENSOR_OUTLET_REMOTE,
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

const uint8_t mb_brcm_switch_sensor_list[] = {
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
  MB_SENSOR_INLET,
  MB_SENSOR_INLET_REMOTE,
  MB_SENSOR_OUTLET,
  MB_SENSOR_OUTLET_REMOTE,
  MB_SENSOR_PEX0_THERM_REMOTE,
  MB_SENSOR_PEX1_THERM_REMOTE,
  MB_SENSOR_PEX2_THERM_REMOTE,
  MB_SENSOR_PEX3_THERM_REMOTE,
  MB_VR_P0V9_VDD0_VOUT,
  MB_VR_P0V9_VDD0_CURR,
  MB_VR_P0V9_VDD0_POUT,
  MB_VR_P0V9_VDD0_TEMP,
  MB_VR_P0V9_VDD1_VOUT,
  MB_VR_P0V9_VDD1_CURR,
  MB_VR_P0V9_VDD1_POUT,
  MB_VR_P0V9_VDD1_TEMP,
  MB_VR_P0V9_VDD2_VOUT,
  MB_VR_P0V9_VDD2_CURR,
  MB_VR_P0V9_VDD2_POUT,
  MB_VR_P0V9_VDD2_TEMP,
  MB_VR_P0V9_VDD3_VOUT,
  MB_VR_P0V9_VDD3_CURR,
  MB_VR_P0V9_VDD3_POUT,
  MB_VR_P0V9_VDD3_TEMP,
  MB_VR_P1V8_AVD0_VOUT,
  MB_VR_P1V8_AVD0_CURR,
  MB_VR_P1V8_AVD0_TEMP,
  MB_VR_P1V8_AVD1_VOUT,
  MB_VR_P1V8_AVD1_CURR,
  MB_VR_P1V8_AVD1_TEMP,
  MB_VR_P1V8_AVD2_VOUT,
  MB_VR_P1V8_AVD2_CURR,
  MB_VR_P1V8_AVD2_TEMP,
  MB_VR_P1V8_AVD3_VOUT,
  MB_VR_P1V8_AVD3_CURR,
  MB_VR_P1V8_AVD3_TEMP
};

const uint8_t pdb_vicor_sensor_list[] = {
  PDB_HSC_P12V_1_VIN,
  PDB_HSC_P12V_1_VOUT,
  PDB_HSC_P12V_1_CURR,
  PDB_HSC_P12V_1_PWR,
  PDB_HSC_P12V_1_PWR_PEAK,
  PDB_HSC_P12V_2_VIN,
  PDB_HSC_P12V_2_VOUT,
  PDB_HSC_P12V_2_CURR,
  PDB_HSC_P12V_2_PWR,
  PDB_HSC_P12V_2_PWR_PEAK,
  PDB_HSC_P12V_AUX_VIN,
  PDB_HSC_P12V_AUX_VOUT,
  PDB_HSC_P12V_AUX_CURR,
  PDB_HSC_P12V_AUX_PWR,
  PDB_HSC_P12V_AUX_PWR_PEAK,
  PDB_HSC_P48V_1_VIN,
  PDB_HSC_P48V_1_VOUT,
  PDB_HSC_P48V_1_CURR,
  PDB_HSC_P48V_1_PWR,
  PDB_HSC_P48V_1_PWR_PEAK,
  PDB_HSC_P48V_2_VIN,
  PDB_HSC_P48V_2_VOUT,
  PDB_HSC_P48V_2_CURR,
  PDB_HSC_P48V_2_PWR,
  PDB_HSC_P48V_2_PWR_PEAK,
  PDB_ADC_1_VICOR0_TEMP,
  PDB_ADC_1_VICOR1_TEMP,
  PDB_ADC_1_VICOR2_TEMP,
  PDB_ADC_1_VICOR3_TEMP,
  PDB_ADC_2_VICOR0_TEMP,
  PDB_ADC_2_VICOR1_TEMP,
  PDB_ADC_2_VICOR2_TEMP,
  PDB_ADC_2_VICOR3_TEMP,
  PDB_SENSOR_OUTLET,
  PDB_SENSOR_OUTLET_REMOTE
};

const uint8_t pdb_infineon_sensor_list[] = {
  PDB_HSC_P12V_1_VIN,
  PDB_HSC_P12V_1_VOUT,
  PDB_HSC_P12V_1_CURR,
  PDB_HSC_P12V_1_PWR,
  PDB_HSC_P12V_1_PWR_PEAK,
  PDB_HSC_P12V_2_VIN,
  PDB_HSC_P12V_2_VOUT,
  PDB_HSC_P12V_2_CURR,
  PDB_HSC_P12V_2_PWR,
  PDB_HSC_P12V_2_PWR_PEAK,
  PDB_HSC_P12V_AUX_VIN,
  PDB_HSC_P12V_AUX_VOUT,
  PDB_HSC_P12V_AUX_CURR,
  PDB_HSC_P12V_AUX_PWR,
  PDB_HSC_P12V_AUX_PWR_PEAK,
  PDB_HSC_P48V_1_VIN,
  PDB_HSC_P48V_1_VOUT,
  PDB_HSC_P48V_1_CURR,
  PDB_HSC_P48V_1_PWR,
  PDB_HSC_P48V_1_PWR_PEAK,
  PDB_HSC_P48V_2_VIN,
  PDB_HSC_P48V_2_VOUT,
  PDB_HSC_P48V_2_CURR,
  PDB_HSC_P48V_2_PWR,
  PDB_HSC_P48V_2_PWR_PEAK,
  PDB_NTC_1_INFINEON0_TEMP,
  PDB_NTC_1_INFINEON1_TEMP,
  PDB_NTC_2_INFINEON0_TEMP,
  PDB_NTC_2_INFINEON1_TEMP,
  PDB_SENSOR_OUTLET,
  PDB_SENSOR_OUTLET_REMOTE
};

const uint8_t asic0_sensor_list[] = {
  MB_GPU0_TEMP,
  MB_GPU0_EDGE_TEMP,
  MB_GPU0_HBM_TEMP,
  MB_GPU0_PWCS,
};

const uint8_t asic1_sensor_list[] = {
  MB_GPU1_TEMP,
  MB_GPU1_EDGE_TEMP,
  MB_GPU1_HBM_TEMP,
  MB_GPU1_PWCS,
};

const uint8_t asic2_sensor_list[] = {
  MB_GPU2_TEMP,
  MB_GPU2_EDGE_TEMP,
  MB_GPU2_HBM_TEMP,
  MB_GPU2_PWCS,
};

const uint8_t asic3_sensor_list[] = {
  MB_GPU3_TEMP,
  MB_GPU3_EDGE_TEMP,
  MB_GPU3_HBM_TEMP,
  MB_GPU3_PWCS,
};

const uint8_t asic4_sensor_list[] = {
  MB_GPU4_TEMP,
  MB_GPU4_EDGE_TEMP,
  MB_GPU4_HBM_TEMP,
  MB_GPU4_PWCS,
};

const uint8_t asic5_sensor_list[] = {
  MB_GPU5_TEMP,
  MB_GPU5_EDGE_TEMP,
  MB_GPU5_HBM_TEMP,
  MB_GPU5_PWCS,
};

const uint8_t asic6_sensor_list[] = {
  MB_GPU6_TEMP,
  MB_GPU6_EDGE_TEMP,
  MB_GPU6_HBM_TEMP,
  MB_GPU6_PWCS,
};

const uint8_t asic7_sensor_list[] = {
  MB_GPU7_TEMP,
  MB_GPU7_EDGE_TEMP,
  MB_GPU7_HBM_TEMP,
  MB_GPU7_PWCS,
};

float sensors_threshold[MAX_SENSOR_NUM + 1][MAX_SENSOR_THRESHOLD + 1] = {
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
  {0,	15.0,	10.0,	0,	0,	0,	0,	0,	0},
  [MB_FAN1_CURR] =
  {0,	15.0,	10.0,	0,	0,	0,	0,	0,	0},
  [MB_FAN2_CURR] =
  {0,	15.0,	10.0,	0,	0,	0,	0,	0,	0},
  [MB_FAN3_CURR] =
  {0,	15.0,	10.0,	0,	0,	0,	0,	0,	0},
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
  [MB_SENSOR_INLET] =
  {0,	55.0,	0,	0,	10.0,	0,	0,	0,	0},
  [MB_SENSOR_INLET_REMOTE] =
  {0,	50.0,	0,	0,	10.0,	0,	0,	0,	0},
  [MB_SENSOR_OUTLET] =
  {0,	75.0,	0,	0,	10.0,	0,	0,	0,	0},
  [MB_SENSOR_OUTLET_REMOTE] =
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
  [MB_GPU0_TEMP] =
  {0,	0,	0,	0,	0,	0,	0,	0,	0},
  [MB_GPU1_TEMP] =
  {0,	0,	0,	0,	0,	0,	0,	0,	0},
  [MB_GPU2_TEMP] =
  {0,	0,	0,	0,	0,	0,	0,	0,	0},
  [MB_GPU3_TEMP] =
  {0,	0,	0,	0,	0,	0,	0,	0,	0},
  [MB_GPU4_TEMP] =
  {0,	0,	0,	0,	0,	0,	0,	0,	0},
  [MB_GPU5_TEMP] =
  {0,	0,	0,	0,	0,	0,	0,	0,	0},
  [MB_GPU6_TEMP] =
  {0,	0,	0,	0,	0,	0,	0,	0,	0},
  [MB_GPU7_TEMP] =
  {0,	0,	0,	0,	0,	0,	0,	0,	0},
  [MB_GPU0_EDGE_TEMP] =
  {0,	0,	0,	0,	0,	0,	0,	0,	0},
  [MB_GPU1_EDGE_TEMP] =
  {0,	0,	0,	0,	0,	0,	0,	0,	0},
  [MB_GPU2_EDGE_TEMP] =
  {0,	0,	0,	0,	0,	0,	0,	0,	0},
  [MB_GPU3_EDGE_TEMP] =
  {0,	0,	0,	0,	0,	0,	0,	0,	0},
  [MB_GPU4_EDGE_TEMP] =
  {0,	0,	0,	0,	0,	0,	0,	0,	0},
  [MB_GPU5_EDGE_TEMP] =
  {0,	0,	0,	0,	0,	0,	0,	0,	0},
  [MB_GPU6_EDGE_TEMP] =
  {0,	0,	0,	0,	0,	0,	0,	0,	0},
  [MB_GPU7_EDGE_TEMP] =
  {0,	0,	0,	0,	0,	0,	0,	0,	0},
  [MB_GPU0_HBM_TEMP] =
  {0,	0,	0,	0,	0,	0,	0,	0,	0},
  [MB_GPU1_HBM_TEMP] =
  {0,	0,	0,	0,	0,	0,	0,	0,	0},
  [MB_GPU2_HBM_TEMP] =
  {0,	0,	0,	0,	0,	0,	0,	0,	0},
  [MB_GPU3_HBM_TEMP] =
  {0,	0,	0,	0,	0,	0,	0,	0,	0},
  [MB_GPU4_HBM_TEMP] =
  {0,	0,	0,	0,	0,	0,	0,	0,	0},
  [MB_GPU5_HBM_TEMP] =
  {0,	0,	0,	0,	0,	0,	0,	0,	0},
  [MB_GPU6_HBM_TEMP] =
  {0,	0,	0,	0,	0,	0,	0,	0,	0},
  [MB_GPU7_HBM_TEMP] =
  {0,	0,	0,	0,	0,	0,	0,	0,	0},
  [MB_GPU0_PWCS] =
  {0,	0,	0,	0,	0,	0,	0,	0,	0},
  [MB_GPU1_PWCS] =
  {0,	0,	0,	0,	0,	0,	0,	0,	0},
  [MB_GPU2_PWCS] =
  {0,	0,	0,	0,	0,	0,	0,	0,	0},
  [MB_GPU3_PWCS] =
  {0,	0,	0,	0,	0,	0,	0,	0,	0},
  [MB_GPU4_PWCS] =
  {0,	0,	0,	0,	0,	0,	0,	0,	0},
  [MB_GPU5_PWCS] =
  {0,	0,	0,	0,	0,	0,	0,	0,	0},
  [MB_GPU6_PWCS] =
  {0,	0,	0,	0,	0,	0,	0,	0,	0},
  [MB_GPU7_PWCS] =
  {0,	0,	0,	0,	0,	0,	0,	0,	0},
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
  {0,	0.861,	0,	0,	0.819,	0,	0,	0,	0},
  [MB_VR_P1V0_AVD1_VOUT] =
  {0,	0.861,	0,	0,	0.819,	0,	0,	0,	0},
  [MB_VR_P1V0_AVD2_VOUT] =
  {0,	0.861,	0,	0,	0.819,	0,	0,	0,	0},
  [MB_VR_P1V0_AVD3_VOUT] =
  {0,	0.861,	0,	0,	0.819,	0,	0,	0,	0},
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
  [MB_SENSOR_PEX0_THERM_REMOTE] =
  {0,	115.0,	0,	0,	10.0,	0,	0,	0,	0},
  [MB_SENSOR_PEX1_THERM_REMOTE] =
  {0,	115.0,	0,	0,	10.0,	0,	0,	0,	0},
  [MB_SENSOR_PEX2_THERM_REMOTE] =
  {0,	115.0,	0,	0,	10.0,	0,	0,	0,	0},
  [MB_SENSOR_PEX3_THERM_REMOTE] =
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
  {0,	281.75,	150.0,	0,	0,	0,	0,	0,	0},
  [PDB_HSC_P12V_2_CURR] =
  {0,	281.75,	150.0,	0,	0,	0,	0,	0,	0},
  [PDB_HSC_P12V_AUX_PWR] =
  {0,	600.0,	0,	0,	0,	0,	0,	0,	0},
  [PDB_HSC_P12V_1_PWR] =
  {0,	3375.0,	1800,	0,	0,	0,	0,	0,	0},
  [PDB_HSC_P12V_2_PWR] =
  {0,	3375.0,	1800,	0,	0,	0,	0,	0,	0},
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
  [PDB_SENSOR_OUTLET] =
  {0,	75.0,	0,	0,	10.0,	0,	0,	0,	0},
  [PDB_SENSOR_OUTLET_REMOTE] =
  {0,	70.0,	0,	0,	10.0,	0,	0,	0,	0},
  [PDB_NTC_1_INFINEON0_TEMP] =
  {0,	105.0,	0,	0,	10.0,	0,	0,	0,	0},
  [PDB_NTC_1_INFINEON1_TEMP] =
  {0,	105.0,	0,	0,	10.0,	0,	0,	0,	0},
  [PDB_NTC_2_INFINEON0_TEMP] =
  {0,	105.0,	0,	0,	10.0,	0,	0,	0,	0},
  [PDB_NTC_2_INFINEON1_TEMP] =
  {0,	105.0,	0,	0,	10.0,	0,	0,	0,	0},
  [MB_VR_P0V9_VDD0_VOUT] =
  {0,	0.94,	0,	0,	0.85,	0,	0,	0,	0},
  [MB_VR_P0V9_VDD0_CURR] =
  {0,	0,	0,	0,	0,	0,	0,	0,	0},
  [MB_VR_P0V9_VDD0_POUT] =
  {0,	0,	0,	0,	0,	0,	0,	0,	0},
  [MB_VR_P0V9_VDD0_TEMP] =
  {0,	0,	0,	0,	0,	0,	0,	0,	0},
  [MB_VR_P0V9_VDD1_VOUT] =
  {0,	0.94,	0,	0,	0.85,	0,	0,	0,	0},
  [MB_VR_P0V9_VDD1_CURR] =
  {0,	0,	0,	0,	0,	0,	0,	0,	0},
  [MB_VR_P0V9_VDD1_POUT] =
  {0,	0,	0,	0,	0,	0,	0,	0,	0},
  [MB_VR_P0V9_VDD1_TEMP] =
  {0,	0,	0,	0,	0,	0,	0,	0,	0},
  [MB_VR_P0V9_VDD2_VOUT] =
  {0,	0.94,	0,	0,	0.85,	0,	0,	0,	0},
  [MB_VR_P0V9_VDD2_CURR] =
  {0,	0,	0,	0,	0,	0,	0,	0,	0},
  [MB_VR_P0V9_VDD2_POUT] =
  {0,	0,	0,	0,	0,	0,	0,	0,	0},
  [MB_VR_P0V9_VDD2_TEMP] =
  {0,	0,	0,	0,	0,	0,	0,	0,	0},
  [MB_VR_P0V9_VDD3_VOUT] =
  {0,	0.94,	0,	0,	0.85,	0,	0,	0,	0},
  [MB_VR_P0V9_VDD3_CURR] =
  {0,	0,	0,	0,	0,	0,	0,	0,	0},
  [MB_VR_P0V9_VDD3_POUT] =
  {0,	0,	0,	0,	0,	0,	0,	0,	0},
  [MB_VR_P0V9_VDD3_TEMP] =
  {0,	0,	0,	0,	0,	0,	0,	0,	0},
  [MB_VR_P1V8_AVD0_VOUT] =
  {0,	1.89,	0,	0,	1.71,	0,	0,	0,	0},
  [MB_VR_P1V8_AVD0_CURR] =
  {0,	0,	0,	0,	0,	0,	0,	0,	0},
  [MB_VR_P1V8_AVD0_TEMP] =
  {0,	0,	0,	0,	0,	0,	0,	0,	0},
  [MB_VR_P1V8_AVD1_VOUT] =
  {0,	1.89,	0,	0,	1.71,	0,	0,	0,	0},
  [MB_VR_P1V8_AVD1_CURR] =
  {0,	0,	0,	0,	0,	0,	0,	0,	0},
  [MB_VR_P1V8_AVD1_TEMP] =
  {0,	0,	0,	0,	0,	0,	0,	0,	0},
  [MB_VR_P1V8_AVD2_VOUT] =
  {0,	1.89,	0,	0,	1.71,	0,	0,	0,	0},
  [MB_VR_P1V8_AVD2_CURR] =
  {0,	0,	0,	0,	0,	0,	0,	0,	0},
  [MB_VR_P1V8_AVD2_TEMP] =
  {0,	0,	0,	0,	0,	0,	0,	0,	0},
  [MB_VR_P1V8_AVD3_VOUT] =
  {0,	1.89,	0,	0,	1.71,	0,	0,	0,	0},
  [MB_VR_P1V8_AVD3_CURR] =
  {0,	0,	0,	0,	0,	0,	0,	0,	0},
  [MB_VR_P1V8_AVD3_TEMP] =
  {0,	0,	0,	0,	0,	0,	0,	0,	0},
};

struct sensor_map {
  int (*sensor_read)(uint8_t, float*);
  const char* name;
  int unit;
} fbep_sensors_map[] = {
  [MB_FAN0_TACH_I] =
  {sensors_read_fan_speed, "EP_MB_FAN0_TACH_I", SNR_TACH},
  [MB_FAN0_TACH_O] =
  {sensors_read_fan_speed, "EP_MB_FAN0_TACH_O", SNR_TACH},
  [MB_FAN1_TACH_I] =
  {sensors_read_fan_speed, "EP_MB_FAN1_TACH_I", SNR_TACH},
  [MB_FAN1_TACH_O] =
  {sensors_read_fan_speed, "EP_MB_FAN1_TACH_O", SNR_TACH},
  [MB_FAN2_TACH_I] =
  {sensors_read_fan_speed, "EP_MB_FAN2_TACH_I", SNR_TACH},
  [MB_FAN2_TACH_O] =
  {sensors_read_fan_speed, "EP_MB_FAN2_TACH_O", SNR_TACH},
  [MB_FAN3_TACH_I] =
  {sensors_read_fan_speed, "EP_MB_FAN3_TACH_I", SNR_TACH},
  [MB_FAN3_TACH_O] =
  {sensors_read_fan_speed, "EP_MB_FAN3_TACH_O", SNR_TACH},
  [MB_FAN0_VOLT] =
  {sensors_read_fan_health, "EP_MB_FAN0_VOLT", SNR_VOLT},
  [MB_FAN1_VOLT] =
  {sensors_read_fan_health, "EP_MB_FAN1_VOLT", SNR_VOLT},
  [MB_FAN2_VOLT] =
  {sensors_read_fan_health, "EP_MB_FAN2_VOLT", SNR_VOLT},
  [MB_FAN3_VOLT] =
  {sensors_read_fan_health, "EP_MB_FAN3_VOLT", SNR_VOLT},
  [MB_FAN0_CURR] =
  {sensors_read_fan_health, "EP_MB_FAN0_CURR", SNR_CURR},
  [MB_FAN1_CURR] =
  {sensors_read_fan_health, "EP_MB_FAN1_CURR", SNR_CURR},
  [MB_FAN2_CURR] =
  {sensors_read_fan_health, "EP_MB_FAN2_CURR", SNR_CURR},
  [MB_FAN3_CURR] =
  {sensors_read_fan_health, "EP_MB_FAN3_CURR", SNR_CURR},
  [MB_ADC_P12V_AUX] =
  {sensors_read_common_adc, "EP_MB_ADC_P12V_AUX", SNR_VOLT},
  [MB_ADC_P12V_1] =
  {sensors_read_12v_adc, "EP_MB_ADC_P12V_1", SNR_VOLT},
  [MB_ADC_P12V_2] =
  {sensors_read_12v_adc, "EP_MB_ADC_P12V_2", SNR_VOLT},
  [MB_ADC_P3V3_STBY] =
  {sensors_read_common_adc, "EP_MB_ADC_P3V3_STBY", SNR_VOLT},
  [MB_ADC_P3V3] =
  {sensors_read_12v_adc, "EP_MB_ADC_P3V3", SNR_VOLT},
  [MB_ADC_P3V_BAT] =
  {read_battery_value, "EP_MB_ADC_P3V_BAT", SNR_VOLT},
  [MB_ADC_P5V_STBY] =
  {sensors_read_common_adc, "EP_MB_ADC_P5V_STBY", SNR_VOLT},
  [MB_SENSOR_INLET] =
  {sensors_read_common_therm, "EP_MB_SENSOR_INLET", SNR_TEMP},
  [MB_SENSOR_INLET_REMOTE] =
  {sensors_read_common_therm, "EP_MB_SENSOR_INLET_REMOTE", SNR_TEMP},
  [MB_SENSOR_OUTLET] =
  {sensors_read_common_therm, "EP_MB_SENSOR_OUTLET", SNR_TEMP},
  [MB_SENSOR_OUTLET_REMOTE] =
  {sensors_read_common_therm, "EP_MB_SENSOR_OUTLET_REMOTE", SNR_TEMP},
  [MB_SENSOR_PAX01_THERM] =
  {sensors_read_pax_therm, "EP_MB_SENSOR_PAX01_THERM", SNR_TEMP},
  [MB_SENSOR_PAX23_THERM] =
  {sensors_read_pax_therm, "EP_MB_SENSOR_PAX23_THERM", SNR_TEMP},
  [MB_SENSOR_PAX0_THERM_REMOTE] =
  {sensors_read_pax_therm, "EP_MB_SENSOR_PAX0_THERM", SNR_TEMP},
  [MB_SENSOR_PAX1_THERM_REMOTE] =
  {sensors_read_pax_therm, "EP_MB_SENSOR_PAX1_THERM", SNR_TEMP},
  [MB_SENSOR_PAX2_THERM_REMOTE] =
  {sensors_read_pax_therm, "EP_MB_SENSOR_PAX2_THERM", SNR_TEMP},
  [MB_SENSOR_PAX3_THERM_REMOTE] =
  {sensors_read_pax_therm, "EP_MB_SENSOR_PAX3_THERM", SNR_TEMP},
  [MB_SWITCH_PAX0_DIE_TEMP] =
  {pal_read_pax_dietemp, "EP_MB_SWITCH_PAX0_DIE_TEMP", SNR_TEMP},
  [MB_SWITCH_PAX1_DIE_TEMP] =
  {pal_read_pax_dietemp, "EP_MB_SWITCH_PAX1_DIE_TEMP", SNR_TEMP},
  [MB_SWITCH_PAX2_DIE_TEMP] =
  {pal_read_pax_dietemp, "EP_MB_SWITCH_PAX2_DIE_TEMP", SNR_TEMP},
  [MB_SWITCH_PAX3_DIE_TEMP] =
  {pal_read_pax_dietemp, "EP_MB_SWITCH_PAX3_DIE_TEMP", SNR_TEMP},
  [MB_GPU0_TEMP] =
  {read_gpu_temp, "EP_MB_GPU0_TEMP", SNR_TEMP},
  [MB_GPU1_TEMP] =
  {read_gpu_temp, "EP_MB_GPU1_TEMP", SNR_TEMP},
  [MB_GPU2_TEMP] =
  {read_gpu_temp, "EP_MB_GPU2_TEMP", SNR_TEMP},
  [MB_GPU3_TEMP] =
  {read_gpu_temp, "EP_MB_GPU3_TEMP", SNR_TEMP},
  [MB_GPU4_TEMP] =
  {read_gpu_temp, "EP_MB_GPU4_TEMP", SNR_TEMP},
  [MB_GPU5_TEMP] =
  {read_gpu_temp, "EP_MB_GPU5_TEMP", SNR_TEMP},
  [MB_GPU6_TEMP] =
  {read_gpu_temp, "EP_MB_GPU6_TEMP", SNR_TEMP},
  [MB_GPU7_TEMP] =
  {read_gpu_temp, "EP_MB_GPU7_TEMP", SNR_TEMP},
  [MB_GPU0_EDGE_TEMP] =
  {read_asic_board_temp, "EP_MB_GPU0_EDGE_TEMP", SNR_TEMP},
  [MB_GPU1_EDGE_TEMP] =
  {read_asic_board_temp, "EP_MB_GPU1_EDGE_TEMP", SNR_TEMP},
  [MB_GPU2_EDGE_TEMP] =
  {read_asic_board_temp, "EP_MB_GPU2_EDGE_TEMP", SNR_TEMP},
  [MB_GPU3_EDGE_TEMP] =
  {read_asic_board_temp, "EP_MB_GPU3_EDGE_TEMP", SNR_TEMP},
  [MB_GPU4_EDGE_TEMP] =
  {read_asic_board_temp, "EP_MB_GPU4_EDGE_TEMP", SNR_TEMP},
  [MB_GPU5_EDGE_TEMP] =
  {read_asic_board_temp, "EP_MB_GPU5_EDGE_TEMP", SNR_TEMP},
  [MB_GPU6_EDGE_TEMP] =
  {read_asic_board_temp, "EP_MB_GPU6_EDGE_TEMP", SNR_TEMP},
  [MB_GPU7_EDGE_TEMP] =
  {read_asic_board_temp, "EP_MB_GPU7_EDGE_TEMP", SNR_TEMP},
  [MB_GPU0_HBM_TEMP] =
  {read_asic_mem_temp, "EP_MB_GPU0_HBM_TEMP", SNR_TEMP},
  [MB_GPU1_HBM_TEMP] =
  {read_asic_mem_temp, "EP_MB_GPU1_HBM_TEMP", SNR_TEMP},
  [MB_GPU2_HBM_TEMP] =
  {read_asic_mem_temp, "EP_MB_GPU2_HBM_TEMP", SNR_TEMP},
  [MB_GPU3_HBM_TEMP] =
  {read_asic_mem_temp, "EP_MB_GPU3_HBM_TEMP", SNR_TEMP},
  [MB_GPU4_HBM_TEMP] =
  {read_asic_mem_temp, "EP_MB_GPU4_HBM_TEMP", SNR_TEMP},
  [MB_GPU5_HBM_TEMP] =
  {read_asic_mem_temp, "EP_MB_GPU5_HBM_TEMP", SNR_TEMP},
  [MB_GPU6_HBM_TEMP] =
  {read_asic_mem_temp, "EP_MB_GPU6_HBM_TEMP", SNR_TEMP},
  [MB_GPU7_HBM_TEMP] =
  {read_asic_mem_temp, "EP_MB_GPU7_HBM_TEMP", SNR_TEMP},
  [MB_GPU0_PWCS] =
  {read_gpu_pwcs, "EP_MB_GPU0_PWCS", SNR_PWR},
  [MB_GPU1_PWCS] =
  {read_gpu_pwcs, "EP_MB_GPU1_PWCS", SNR_PWR},
  [MB_GPU2_PWCS] =
  {read_gpu_pwcs, "EP_MB_GPU2_PWCS", SNR_PWR},
  [MB_GPU3_PWCS] =
  {read_gpu_pwcs, "EP_MB_GPU3_PWCS", SNR_PWR},
  [MB_GPU4_PWCS] =
  {read_gpu_pwcs, "EP_MB_GPU4_PWCS", SNR_PWR},
  [MB_GPU5_PWCS] =
  {read_gpu_pwcs, "EP_MB_GPU5_PWCS", SNR_PWR},
  [MB_GPU6_PWCS] =
  {read_gpu_pwcs, "EP_MB_GPU6_PWCS", SNR_PWR},
  [MB_GPU7_PWCS] =
  {read_gpu_pwcs, "EP_MB_GPU7_PWCS", SNR_PWR},
  [MB_VR_P0V8_VDD0_VIN] =
  {sensors_read_vr, "EP_MB_VR_P0V8_VDD0_VIN", SNR_VOLT},
  [MB_VR_P0V8_VDD1_VIN] =
  {sensors_read_vr, "EP_MB_VR_P0V8_VDD1_VIN", SNR_VOLT},
  [MB_VR_P0V8_VDD2_VIN] =
  {sensors_read_vr, "EP_MB_VR_P0V8_VDD2_VIN", SNR_VOLT},
  [MB_VR_P0V8_VDD3_VIN] =
  {sensors_read_vr, "EP_MB_VR_P0V8_VDD3_VIN", SNR_VOLT},
  [MB_VR_P0V8_VDD0_VOUT] =
  {sensors_read_vr, "EP_MB_VR_P0V8_VDD0_VOUT", SNR_VOLT},
  [MB_VR_P0V8_VDD1_VOUT] =
  {sensors_read_vr, "EP_MB_VR_P0V8_VDD1_VOUT", SNR_VOLT},
  [MB_VR_P0V8_VDD2_VOUT] =
  {sensors_read_vr, "EP_MB_VR_P0V8_VDD2_VOUT", SNR_VOLT},
  [MB_VR_P0V8_VDD3_VOUT] =
  {sensors_read_vr, "EP_MB_VR_P0V8_VDD3_VOUT", SNR_VOLT},
  [MB_VR_P0V8_VDD0_CURR] =
  {sensors_read_vr, "EP_MB_VR_P0V8_VDD0_CURR", SNR_CURR},
  [MB_VR_P0V8_VDD1_CURR] =
  {sensors_read_vr, "EP_MB_VR_P0V8_VDD1_CURR", SNR_CURR},
  [MB_VR_P0V8_VDD2_CURR] =
  {sensors_read_vr, "EP_MB_VR_P0V8_VDD2_CURR", SNR_CURR},
  [MB_VR_P0V8_VDD3_CURR] =
  {sensors_read_vr, "EP_MB_VR_P0V8_VDD3_CURR", SNR_CURR},
  [MB_VR_P0V8_VDD0_TEMP] =
  {sensors_read_vr, "EP_MB_VR_P0V8_VDD0_TEMP", SNR_TEMP},
  [MB_VR_P0V8_VDD1_TEMP] =
  {sensors_read_vr, "EP_MB_VR_P0V8_VDD1_TEMP", SNR_TEMP},
  [MB_VR_P0V8_VDD2_TEMP] =
  {sensors_read_vr, "EP_MB_VR_P0V8_VDD2_TEMP", SNR_TEMP},
  [MB_VR_P0V8_VDD3_TEMP] =
  {sensors_read_vr, "EP_MB_VR_P0V8_VDD3_TEMP", SNR_TEMP},
  [MB_VR_P1V0_AVD0_VIN] =
  {sensors_read_vr, "EP_MB_VR_P1V0_AVD0_VIN", SNR_VOLT},
  [MB_VR_P1V0_AVD1_VIN] =
  {sensors_read_vr, "EP_MB_VR_P1V0_AVD1_VIN", SNR_VOLT},
  [MB_VR_P1V0_AVD2_VIN] =
  {sensors_read_vr, "EP_MB_VR_P1V0_AVD2_VIN", SNR_VOLT},
  [MB_VR_P1V0_AVD3_VIN] =
  {sensors_read_vr, "EP_MB_VR_P1V0_AVD3_VIN", SNR_VOLT},
  [MB_VR_P1V0_AVD0_VOUT] =
  {sensors_read_vr, "EP_MB_VR_P1V0_AVD0_VOUT", SNR_VOLT},
  [MB_VR_P1V0_AVD1_VOUT] =
  {sensors_read_vr, "EP_MB_VR_P1V0_AVD1_VOUT", SNR_VOLT},
  [MB_VR_P1V0_AVD2_VOUT] =
  {sensors_read_vr, "EP_MB_VR_P1V0_AVD2_VOUT", SNR_VOLT},
  [MB_VR_P1V0_AVD3_VOUT] =
  {sensors_read_vr, "EP_MB_VR_P1V0_AVD3_VOUT", SNR_VOLT},
  [MB_VR_P1V0_AVD0_CURR] =
  {sensors_read_vr, "EP_MB_VR_P1V0_AVD0_CURR", SNR_CURR},
  [MB_VR_P1V0_AVD1_CURR] =
  {sensors_read_vr, "EP_MB_VR_P1V0_AVD1_CURR", SNR_CURR},
  [MB_VR_P1V0_AVD2_CURR] =
  {sensors_read_vr, "EP_MB_VR_P1V0_AVD2_CURR", SNR_CURR},
  [MB_VR_P1V0_AVD3_CURR] =
  {sensors_read_vr, "EP_MB_VR_P1V0_AVD3_CURR", SNR_CURR},
  [MB_VR_P1V0_AVD0_TEMP] =
  {sensors_read_vr, "EP_MB_VR_P1V0_AVD0_TEMP", SNR_TEMP},
  [MB_VR_P1V0_AVD1_TEMP] =
  {sensors_read_vr, "EP_MB_VR_P1V0_AVD1_TEMP", SNR_TEMP},
  [MB_VR_P1V0_AVD2_TEMP] =
  {sensors_read_vr, "EP_MB_VR_P1V0_AVD2_TEMP", SNR_TEMP},
  [MB_VR_P1V0_AVD3_TEMP] =
  {sensors_read_vr, "EP_MB_VR_P1V0_AVD3_TEMP", SNR_TEMP},
  [MB_SENSOR_PEX0_THERM_REMOTE] =
  {sensors_read_pax_therm, "EP_MB_SENSOR_PEX0_THERM", SNR_TEMP},
  [MB_SENSOR_PEX1_THERM_REMOTE] =
  {sensors_read_pax_therm, "EP_MB_SENSOR_PEX1_THERM", SNR_TEMP},
  [MB_SENSOR_PEX2_THERM_REMOTE] =
  {sensors_read_pax_therm, "EP_MB_SENSOR_PEX2_THERM", SNR_TEMP},
  [MB_SENSOR_PEX3_THERM_REMOTE] =
  {sensors_read_pax_therm, "EP_MB_SENSOR_PEX3_THERM", SNR_TEMP},
  [PDB_HSC_P12V_AUX_VIN] =
  {sensors_read_12v_hsc, "EP_PDB_HSC_P12V_AUX_VIN", SNR_VOLT},
  [PDB_HSC_P12V_1_VIN] =
  {sensors_read_12v_hsc, "EP_PDB_HSC_P12V_1_VIN", SNR_VOLT},
  [PDB_HSC_P12V_2_VIN] =
  {sensors_read_12v_hsc, "EP_PDB_HSC_P12V_2_VIN", SNR_VOLT},
  [PDB_HSC_P12V_AUX_VOUT] =
  {sensors_read_12v_hsc, "EP_PDB_HSC_P12V_AUX_VOUT", SNR_VOLT},
  [PDB_HSC_P12V_1_VOUT] =
  {sensors_read_12v_hsc_vout, "EP_PDB_HSC_P12V_1_VOUT", SNR_VOLT},
  [PDB_HSC_P12V_2_VOUT] =
  {sensors_read_12v_hsc_vout, "EP_PDB_HSC_P12V_2_VOUT", SNR_VOLT},
  [PDB_HSC_P12V_AUX_CURR] =
  {sensors_read_12v_hsc, "EP_PDB_HSC_P12V_AUX_CURR", SNR_CURR},
  [PDB_HSC_P12V_1_CURR] =
  {sensors_read_12v_hsc, "EP_PDB_HSC_P12V_1_CURR", SNR_CURR},
  [PDB_HSC_P12V_2_CURR] =
  {sensors_read_12v_hsc, "EP_PDB_HSC_P12V_2_CURR", SNR_CURR},
  [PDB_HSC_P12V_AUX_PWR] =
  {sensors_read_12v_hsc, "EP_PDB_HSC_P12V_AUX_PWR", SNR_PWR},
  [PDB_HSC_P12V_AUX_PWR_PEAK] =
  {sensors_read_12v_hsc, "EP_PDB_HSC_P12V_AUX_PWR_PEAK", SNR_PWR},
  [PDB_HSC_P12V_1_PWR] =
  {sensors_read_12v_hsc, "EP_PDB_HSC_P12V_1_PWR", SNR_PWR},
  [PDB_HSC_P12V_1_PWR_PEAK] =
  {sensors_read_12v_hsc, "EP_PDB_HSC_P12V_1_PWR_PEAK", SNR_PWR},
  [PDB_HSC_P12V_2_PWR] =
  {sensors_read_12v_hsc, "EP_PDB_HSC_P12V_2_PWR", SNR_PWR},
  [PDB_HSC_P12V_2_PWR_PEAK] =
  {sensors_read_12v_hsc, "EP_PDB_HSC_P12V_2_PWR_PEAK", SNR_PWR},
  [PDB_HSC_P48V_1_VIN] =
  {sensors_read_48v_hsc, "EP_PDB_HSC_P48V_1_VIN", SNR_VOLT},
  [PDB_HSC_P48V_1_VOUT] =
  {sensors_read_48v_hsc_vout, "EP_PDB_HSC_P48V_1_VOUT", SNR_VOLT},
  [PDB_HSC_P48V_1_CURR] =
  {sensors_read_48v_hsc, "EP_PDB_HSC_P48V_1_CURR", SNR_CURR},
  [PDB_HSC_P48V_1_PWR] =
  {sensors_read_48v_hsc, "EP_PDB_HSC_P48V_1_PWR", SNR_PWR},
  [PDB_HSC_P48V_1_PWR_PEAK] =
  {sensors_read_48v_hsc, "EP_PDB_HSC_P48V_1_PWR_PEAK", SNR_PWR},
  [PDB_HSC_P48V_2_VIN] =
  {sensors_read_48v_hsc, "EP_PDB_HSC_P48V_2_VIN", SNR_VOLT},
  [PDB_HSC_P48V_2_VOUT] =
  {sensors_read_48v_hsc_vout, "EP_PDB_HSC_P48V_2_VOUT", SNR_VOLT},
  [PDB_HSC_P48V_2_CURR] =
  {sensors_read_48v_hsc, "EP_PDB_HSC_P48V_2_CURR", SNR_CURR},
  [PDB_HSC_P48V_2_PWR] =
  {sensors_read_48v_hsc, "EP_PDB_HSC_P48V_2_PWR", SNR_PWR},
  [PDB_HSC_P48V_2_PWR_PEAK] =
  {sensors_read_48v_hsc, "EP_PDB_HSC_P48V_2_PWR_PEAK", SNR_PWR},
  [PDB_ADC_1_VICOR0_TEMP] =
  {sensors_read_vicor, "EP_PDB_ADC_1_VICOR0_TEMP", SNR_TEMP},
  [PDB_ADC_1_VICOR1_TEMP] =
  {sensors_read_vicor, "EP_PDB_ADC_1_VICOR1_TEMP", SNR_TEMP},
  [PDB_ADC_1_VICOR2_TEMP] =
  {sensors_read_vicor, "EP_PDB_ADC_1_VICOR2_TEMP", SNR_TEMP},
  [PDB_ADC_1_VICOR3_TEMP] =
  {sensors_read_vicor, "EP_PDB_ADC_1_VICOR3_TEMP", SNR_TEMP},
  [PDB_ADC_2_VICOR0_TEMP] =
  {sensors_read_vicor, "EP_PDB_ADC_2_VICOR0_TEMP", SNR_TEMP},
  [PDB_ADC_2_VICOR1_TEMP] =
  {sensors_read_vicor, "EP_PDB_ADC_2_VICOR1_TEMP", SNR_TEMP},
  [PDB_ADC_2_VICOR2_TEMP] =
  {sensors_read_vicor, "EP_PDB_ADC_2_VICOR2_TEMP", SNR_TEMP},
  [PDB_ADC_2_VICOR3_TEMP] =
  {sensors_read_vicor, "EP_PDB_ADC_2_VICOR3_TEMP", SNR_TEMP},
  [PDB_SENSOR_OUTLET] =
  {sensors_read_common_therm, "EP_PDB_SENSOR_OUTLET", SNR_TEMP},
  [PDB_SENSOR_OUTLET_REMOTE] =
  {sensors_read_common_therm, "EP_PDB_SENSOR_OUTLET_REMOTE", SNR_TEMP},
  [PDB_NTC_1_INFINEON0_TEMP] =
  {sensors_read_infineon, "EP_PDB_NTC_1_INFINEON0_TEMP", SNR_TEMP},
  [PDB_NTC_1_INFINEON1_TEMP] =
  {sensors_read_infineon, "EP_PDB_NTC_1_INFINEON1_TEMP", SNR_TEMP},
  [PDB_NTC_2_INFINEON0_TEMP] =
  {sensors_read_infineon, "EP_PDB_NTC_2_INFINEON0_TEMP", SNR_TEMP},
  [PDB_NTC_2_INFINEON1_TEMP] =
  {sensors_read_infineon, "EP_PDB_NTC_2_INFINEON1_TEMP", SNR_TEMP},
  [MB_VR_P0V9_VDD0_VOUT] =
  {sensors_read_brcm_vr, "EP_MB_VR_P0V9_PEX0_VOUT", SNR_VOLT},
  [MB_VR_P0V9_VDD1_VOUT] =
  {sensors_read_brcm_vr, "EP_MB_VR_P0V9_PEX1_VOUT", SNR_VOLT},
  [MB_VR_P0V9_VDD2_VOUT] =
  {sensors_read_brcm_vr, "EP_MB_VR_P0V9_PEX2_VOUT", SNR_VOLT},
  [MB_VR_P0V9_VDD3_VOUT] =
  {sensors_read_brcm_vr, "EP_MB_VR_P0V9_PEX3_VOUT", SNR_VOLT},
  [MB_VR_P1V8_AVD0_VOUT] =
  {sensors_read_brcm_vr, "EP_MB_VR_P1V8_PEX0_VOUT", SNR_VOLT},
  [MB_VR_P1V8_AVD1_VOUT] =
  {sensors_read_brcm_vr, "EP_MB_VR_P1V8_PEX1_VOUT", SNR_VOLT},
  [MB_VR_P1V8_AVD2_VOUT] =
  {sensors_read_brcm_vr, "EP_MB_VR_P1V8_PEX2_VOUT", SNR_VOLT},
  [MB_VR_P1V8_AVD3_VOUT] =
  {sensors_read_brcm_vr, "EP_MB_VR_P1V8_PEX3_VOUT", SNR_VOLT},
  [MB_VR_P0V9_VDD0_CURR] =
  {sensors_read_brcm_vr, "EP_MB_VR_P0V9_PEX0_CURR", SNR_CURR},
  [MB_VR_P0V9_VDD1_CURR] =
  {sensors_read_brcm_vr, "EP_MB_VR_P0V9_PEX1_CURR", SNR_CURR},
  [MB_VR_P0V9_VDD2_CURR] =
  {sensors_read_brcm_vr, "EP_MB_VR_P0V9_PEX2_CURR", SNR_CURR},
  [MB_VR_P0V9_VDD3_CURR] =
  {sensors_read_brcm_vr, "EP_MB_VR_P0V9_PEX3_CURR", SNR_CURR},
  [MB_VR_P1V8_AVD0_CURR] =
  {sensors_read_brcm_vr, "EP_MB_VR_P1V8_PEX0_CURR", SNR_CURR},
  [MB_VR_P1V8_AVD1_CURR] =
  {sensors_read_brcm_vr, "EP_MB_VR_P1V8_PEX1_CURR", SNR_CURR},
  [MB_VR_P1V8_AVD2_CURR] =
  {sensors_read_brcm_vr, "EP_MB_VR_P1V8_PEX2_CURR", SNR_CURR},
  [MB_VR_P1V8_AVD3_CURR] =
  {sensors_read_brcm_vr, "EP_MB_VR_P1V8_PEX3_CURR", SNR_CURR},
  [MB_VR_P0V9_VDD0_POUT] =
  {sensors_read_brcm_vr, "EP_MB_VR_P0V9_PEX0_POUT", SNR_PWR},
  [MB_VR_P0V9_VDD1_POUT] =
  {sensors_read_brcm_vr, "EP_MB_VR_P0V9_PEX1_POUT", SNR_PWR},
  [MB_VR_P0V9_VDD2_POUT] =
  {sensors_read_brcm_vr, "EP_MB_VR_P0V9_PEX2_POUT", SNR_PWR},
  [MB_VR_P0V9_VDD3_POUT] =
  {sensors_read_brcm_vr, "EP_MB_VR_P0V9_PEX3_POUT", SNR_PWR},
  [MB_VR_P0V9_VDD0_TEMP] =
  {sensors_read_brcm_vr, "EP_MB_VR_P0V9_PEX0_TEMP", SNR_TEMP},
  [MB_VR_P0V9_VDD1_TEMP] =
  {sensors_read_brcm_vr, "EP_MB_VR_P0V9_PEX1_TEMP", SNR_TEMP},
  [MB_VR_P0V9_VDD2_TEMP] =
  {sensors_read_brcm_vr, "EP_MB_VR_P0V9_PEX2_TEMP", SNR_TEMP},
  [MB_VR_P0V9_VDD3_TEMP] =
  {sensors_read_brcm_vr, "EP_MB_VR_P0V9_PEX3_TEMP", SNR_TEMP},
  [MB_VR_P1V8_AVD0_TEMP] =
  {sensors_read_brcm_vr, "EP_MB_VR_P1V8_PEX0_TEMP", SNR_TEMP},
  [MB_VR_P1V8_AVD1_TEMP] =
  {sensors_read_brcm_vr, "EP_MB_VR_P1V8_PEX1_TEMP", SNR_TEMP},
  [MB_VR_P1V8_AVD2_TEMP] =
  {sensors_read_brcm_vr, "EP_MB_VR_P1V8_PEX2_TEMP", SNR_TEMP},
  [MB_VR_P1V8_AVD3_TEMP] =
  {sensors_read_brcm_vr, "EP_MB_VR_P1V8_PEX3_TEMP", SNR_TEMP},
};

static const char* asic_sensor_name_by_mfr[MFR_MAX_NUM][32] = {
  [GPU_AMD] = {
    "EP_MB_AMD_GPU0_TEMP",
    "EP_MB_AMD_GPU1_TEMP",
    "EP_MB_AMD_GPU2_TEMP",
    "EP_MB_AMD_GPU3_TEMP",
    "EP_MB_AMD_GPU4_TEMP",
    "EP_MB_AMD_GPU5_TEMP",
    "EP_MB_AMD_GPU6_TEMP",
    "EP_MB_AMD_GPU7_TEMP",
    "EP_MB_GPU0_EDGE_TEMP",
    "EP_MB_GPU1_EDGE_TEMP",
    "EP_MB_GPU2_EDGE_TEMP",
    "EP_MB_GPU3_EDGE_TEMP",
    "EP_MB_GPU4_EDGE_TEMP",
    "EP_MB_GPU5_EDGE_TEMP",
    "EP_MB_GPU6_EDGE_TEMP",
    "EP_MB_GPU7_EDGE_TEMP",
    "EP_MB_AMD_HBM0_TEMP",
    "EP_MB_AMD_HBM1_TEMP",
    "EP_MB_AMD_HBM2_TEMP",
    "EP_MB_AMD_HBM3_TEMP",
    "EP_MB_AMD_HBM4_TEMP",
    "EP_MB_AMD_HBM5_TEMP",
    "EP_MB_AMD_HBM6_TEMP",
    "EP_MB_AMD_HBM7_TEMP",
    "EP_MB_GPU0_PWCS",
    "EP_MB_GPU1_PWCS",
    "EP_MB_GPU2_PWCS",
    "EP_MB_GPU3_PWCS",
    "EP_MB_GPU4_PWCS",
    "EP_MB_GPU5_PWCS",
    "EP_MB_GPU6_PWCS",
    "EP_MB_GPU7_PWCS"
  },
  [GPU_NVIDIA] = {
    "EP_MB_NVIDIA_GPU0_TEMP",
    "EP_MB_NVIDIA_GPU1_TEMP",
    "EP_MB_NVIDIA_GPU2_TEMP",
    "EP_MB_NVIDIA_GPU3_TEMP",
    "EP_MB_NVIDIA_GPU4_TEMP",
    "EP_MB_NVIDIA_GPU5_TEMP",
    "EP_MB_NVIDIA_GPU6_TEMP",
    "EP_MB_NVIDIA_GPU7_TEMP",
    "EP_MB_GPU0_EDGE_TEMP",
    "EP_MB_GPU1_EDGE_TEMP",
    "EP_MB_GPU2_EDGE_TEMP",
    "EP_MB_GPU3_EDGE_TEMP",
    "EP_MB_GPU4_EDGE_TEMP",
    "EP_MB_GPU5_EDGE_TEMP",
    "EP_MB_GPU6_EDGE_TEMP",
    "EP_MB_GPU7_EDGE_TEMP",
    "EP_MB_NVIDIA_HBM0_TEMP",
    "EP_MB_NVIDIA_HBM1_TEMP",
    "EP_MB_NVIDIA_HBM2_TEMP",
    "EP_MB_NVIDIA_HBM3_TEMP",
    "EP_MB_NVIDIA_HBM4_TEMP",
    "EP_MB_NVIDIA_HBM5_TEMP",
    "EP_MB_NVIDIA_HBM6_TEMP",
    "EP_MB_NVIDIA_HBM7_TEMP",
    "EP_MB_GPU0_PWCS",
    "EP_MB_GPU1_PWCS",
    "EP_MB_GPU2_PWCS",
    "EP_MB_GPU3_PWCS",
    "EP_MB_GPU4_PWCS",
    "EP_MB_GPU5_PWCS",
    "EP_MB_GPU6_PWCS",
    "EP_MB_GPU7_PWCS"
  }
};

size_t mb_micro_switch_sensor_cnt = sizeof(mb_micro_switch_sensor_list)/sizeof(uint8_t);
size_t mb_brcm_switch_sensor_cnt = sizeof(mb_brcm_switch_sensor_list)/sizeof(uint8_t);
size_t pdb_vicor_sensor_cnt = sizeof(pdb_vicor_sensor_list)/sizeof(uint8_t);
size_t pdb_infineon_sensor_cnt = sizeof(pdb_infineon_sensor_list)/sizeof(uint8_t);
size_t asic0_sensor_cnt = sizeof(asic0_sensor_list)/sizeof(uint8_t);
size_t asic1_sensor_cnt = sizeof(asic1_sensor_list)/sizeof(uint8_t);
size_t asic2_sensor_cnt = sizeof(asic2_sensor_list)/sizeof(uint8_t);
size_t asic3_sensor_cnt = sizeof(asic3_sensor_list)/sizeof(uint8_t);
size_t asic4_sensor_cnt = sizeof(asic4_sensor_list)/sizeof(uint8_t);
size_t asic5_sensor_cnt = sizeof(asic5_sensor_list)/sizeof(uint8_t);
size_t asic6_sensor_cnt = sizeof(asic6_sensor_list)/sizeof(uint8_t);
size_t asic7_sensor_cnt = sizeof(asic7_sensor_list)/sizeof(uint8_t);

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

static int sensors_read_vicor(uint8_t sensor_num, float *value)
{
  int val;
  int ain, index;
  char ain_name[LARGEST_DEVICE_NAME] = {0};
  char device[LARGEST_DEVICE_NAME] = {0};

  if (pal_is_server_off())
    return ERR_SENSOR_NA;

  index = (sensor_num - PDB_ADC_1_VICOR0_TEMP) / 4 + 1;
  snprintf(device, LARGEST_DEVICE_NAME, (index == 1)? ADC_1_DIR: ADC_2_DIR, TLA2024_FSR);
  // Set FSR to 4.096
  if (write_device(device, 2) < 0)
    return -1;

  ain = (sensor_num - PDB_ADC_1_VICOR0_TEMP) % 4;
  snprintf(ain_name, LARGEST_DEVICE_NAME, TLA2024_AIN, ain);
  snprintf(device, LARGEST_DEVICE_NAME, (index == 1)? ADC_1_DIR: ADC_2_DIR, ain_name);
  if (read_device(device, &val) < 0)
    return ERR_SENSOR_NA;

  /*
   * Convert ADC to temperature
   * Volt = ADC * 4.096 / 2^11
   * Temp = (Volt - 2.73) * 100.0
   */
  *value = (float)(val*0.2-273.0);
  if (*value < 0)
    return ERR_SENSOR_NA;

  return 0;
}

static int sensors_read_infineon(uint8_t sensor_num, float *value)
{
  char dev[64] = {0};
  uint8_t tbuf[8] = {0};
  uint8_t rbuf[8] = {0};
  uint8_t addr, bus;

  switch (sensor_num) {
    case PDB_NTC_1_INFINEON0_TEMP:
      bus = 17;
      addr = 0x8a;
      tbuf[0] = 0x8d;
    break;
    case PDB_NTC_1_INFINEON1_TEMP:
      bus = 17;
      addr = 0x8a;
      tbuf[0] = 0x8e;
    break;
    case PDB_NTC_2_INFINEON0_TEMP:
      bus = 18;
      addr = 0x8c;
      tbuf[0] = 0x8d;
    break;
    case PDB_NTC_2_INFINEON1_TEMP:
      bus = 18;
      addr = 0x8c;
      tbuf[0] = 0x8e;
    break;

    default:
      return -1;
  }

  snprintf(dev, sizeof(dev), "/dev/i2c-%d", bus);
  int fd = open(dev, O_RDWR);
  if (fd < 0)
    goto exit;

  i2c_rdwr_msg_transfer(fd, addr, tbuf, 1, rbuf, 1);

  *value = rbuf[0];

exit:
  close(fd);
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
  *rpm = ret < 0? 0: (int)value;
  return 0;
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

int pal_check_switch_config(void)
{
  char switch_vendor[16] = {0};
  int switch_config;
  int ret = 0;

  ret = kv_get("switch_chip", switch_vendor, NULL, 0);
  if (ret < 0 || !strcmp(switch_vendor, "MICRO")) {
    switch_config = 0;
  } else {
    switch_config = 1;
  }

  return switch_config;
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

static bool is_fan_prsnt(uint8_t sensor_num)
{
  gpio_value_t value;
  gpio_desc_t *desc;
  char shadow[16] = {0};

  switch (sensor_num) {
    case MB_FAN0_TACH_I:
    case MB_FAN0_TACH_O:
    case MB_FAN0_VOLT:
    case MB_FAN0_CURR:
      snprintf(shadow, sizeof(shadow), "FAN0_PRESENT");
      break;
    case MB_FAN1_TACH_I:
    case MB_FAN1_TACH_O:
    case MB_FAN1_VOLT:
    case MB_FAN1_CURR:
      snprintf(shadow, sizeof(shadow), "FAN1_PRESENT");
      break;
    case MB_FAN2_TACH_I:
    case MB_FAN2_TACH_O:
    case MB_FAN2_VOLT:
    case MB_FAN2_CURR:
      snprintf(shadow, sizeof(shadow), "FAN2_PRESENT");
      break;
    case MB_FAN3_TACH_I:
    case MB_FAN3_TACH_O:
    case MB_FAN3_VOLT:
    case MB_FAN3_CURR:
      snprintf(shadow, sizeof(shadow), "FAN3_PRESENT");
      break;
    default:
      return false;
  }

  desc = gpio_open_by_shadow(shadow);
  if (!desc) {
    syslog(LOG_CRIT, "Open failed for GPIO: %s\n", shadow);
    return false;
  }
  if (gpio_get_value(desc, &value)) {
    syslog(LOG_CRIT, "Get failed for GPIO: %s\n", shadow);
    value = GPIO_VALUE_INVALID;
  }
  gpio_close(desc);

  return value == GPIO_VALUE_LOW? true: false;
}

bool pal_is_fan_prsnt(uint8_t fan)
{
  // fan id to sensor num
  uint8_t sensor_num = fan + (fan & ~(0x1));
  return is_fan_prsnt(sensor_num);
}

static int sensors_read_fan_speed(uint8_t sensor_num, float *value)
{
  int ret, index;
  static bool checked[8] = {false, false, false, false, false, false, false, false};
  *value = 0.0;

  // Although PWM controller driver had been loaded when BMC booted
  // Some attributes are not accessible right after 12V is on
  if (!is_device_ready() || !is_fan_prsnt(sensor_num))
    return ERR_SENSOR_NA;

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
    default:
      return ERR_SENSOR_NA;
  }

  if (ret == 0) {
    index = sensor_num - MB_FAN0_TACH_I;
    index = index/2 + index%2;
    if (*value <= sensors_threshold[sensor_num][LCR_THRESH]) {
      if (!checked[index]) {
        // In case the fans just got installed, fans are still accelerating.
        // Ignore this reading
        checked[index] = true;
        return ERR_SENSOR_NA;
      }
    } else {
      checked[index] = false;
    }
  }

  return 0;
}

int pal_check_pdb_vr_config(void)
{
  char vr_vendor[16] = {0};
  int vr_config;
  int ret = 0;

  ret = kv_get("VR_IC", vr_vendor, NULL, 0);
  if (ret < 0 || !strcmp(vr_vendor, "VI")) {
    vr_config = 1;
  } else {
    vr_config = 0;
  }

  return vr_config;
}

static int sensors_read_fan_health(uint8_t sensor_num, float *value)
{
  int ret, index;
  static bool checked[8] = {false, false, false, false, false, false, false, false};
  *value = 0.0;

  if (pal_is_server_off() || !is_fan_prsnt(sensor_num))
    return ERR_SENSOR_NA;

  switch (sensor_num) {
    case MB_FAN0_VOLT:
      ret = sensors_read("adc128d818-i2c-18-1d", "FAN0_VOLT", (float *)value);
      break;
    case MB_FAN0_CURR:
      if(pal_check_pdb_vr_config()) {
        ret = sensors_read("adc128d818-i2c-18-1d", "FAN0_CURR", (float *)value);
      } else {
        ret = sensors_read("adc128d818-i2c-18-1d", "FAN0_CURR", (float *)value);
        hsc_value_adjust(mps_efuse_current_table, value);
      }
      break;
    case MB_FAN1_VOLT:
      ret = sensors_read("adc128d818-i2c-18-1d", "FAN1_VOLT", (float *)value);
      break;
    case MB_FAN1_CURR:
      if(pal_check_pdb_vr_config()) {
        ret = sensors_read("adc128d818-i2c-18-1d", "FAN1_CURR", (float *)value);
      } else {
        ret = sensors_read("adc128d818-i2c-18-1d", "FAN1_CURR", (float *)value);
        hsc_value_adjust(mps_efuse_current_table, value);
      }
      break;
    case MB_FAN2_VOLT:
      ret = sensors_read("adc128d818-i2c-18-1d", "FAN2_VOLT", (float *)value);
      break;
    case MB_FAN2_CURR:
      if(pal_check_pdb_vr_config()) {
        ret = sensors_read("adc128d818-i2c-18-1d", "FAN2_CURR", (float *)value);
      } else {
        ret = sensors_read("adc128d818-i2c-18-1d", "FAN2_CURR", (float *)value);
        hsc_value_adjust(mps_efuse_current_table, value);
      }
      break;
    case MB_FAN3_VOLT:
      ret = sensors_read("adc128d818-i2c-18-1d", "FAN3_VOLT", (float *)value);
      break;
    case MB_FAN3_CURR:
      if(pal_check_pdb_vr_config()) {
        ret = sensors_read("adc128d818-i2c-18-1d", "FAN3_CURR", (float *)value);
      } else {
        ret = sensors_read("adc128d818-i2c-18-1d", "FAN3_CURR", (float *)value);
        hsc_value_adjust(mps_efuse_current_table, value);
      }
      break;
    default:
      return ERR_SENSOR_NA;
  }

  if (ret == 0) {
    index = sensor_num - MB_FAN0_VOLT;
    index = index/2 + index%2;
    if (*value <= sensors_threshold[sensor_num][LCR_THRESH]) {
      if (!checked[index]) {
        // In case the fans just got installed, ADC might not finish sampling
        // Ignore this reading
        checked[index] = true;
        return ERR_SENSOR_NA;
      }
    } else {
      checked[index] = false;
    }
  }

  return ret < 0? ERR_SENSOR_NA: 0;
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
    default:
      return ERR_SENSOR_NA;
  }

  return ret < 0? ERR_SENSOR_NA: 0;
}

static int sensors_read_12v_adc(uint8_t sensor_num, float *value)
{
  int ret;

  if (pal_is_server_off())
    return ERR_SENSOR_NA;

  switch (sensor_num) {
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
      return ERR_SENSOR_NA;
  }

  return ret < 0? ERR_SENSOR_NA: 0;
}

static int sensors_read_common_therm(uint8_t sensor_num, float *value)
{
  int ret, index;
  static bool checked[6] = {false, false, false, false, false, false};
  *value = 0.0;

  switch (sensor_num) {
    case MB_SENSOR_INLET:
      ret = sensors_read("tmp421-i2c-6-4c", "INLET", (float *)value);
      break;
    case MB_SENSOR_INLET_REMOTE:
      ret = sensors_read("tmp421-i2c-6-4c", "INLET_REMOTE", (float *)value);
      break;
    case MB_SENSOR_OUTLET:
      ret = sensors_read("tmp421-i2c-6-4f", "OUTLET", (float *)value);
      break;
    case MB_SENSOR_OUTLET_REMOTE:
      ret = sensors_read("tmp421-i2c-6-4f", "OUTLET_REMOTE", (float *)value);
      break;
    case PDB_SENSOR_OUTLET:
      ret = sensors_read("tmp421-i2c-17-4c", "OUTLET", (float *)value);
      break;
    case PDB_SENSOR_OUTLET_REMOTE:
      ret = sensors_read("tmp421-i2c-17-4c", "OUTLET_REMOTE", (float *)value);
      break;
    default:
      return ERR_SENSOR_NA;
  }

  if (ret == 0) {
    if (sensor_num == PDB_SENSOR_OUTLET || sensor_num == PDB_SENSOR_OUTLET_REMOTE)
      index = sensor_num - PDB_SENSOR_OUTLET;
    else
      index = sensor_num - MB_SENSOR_INLET;

    if (*value <= sensors_threshold[sensor_num][LCR_THRESH]) {
      if (!checked[index]) {
        checked[index] = true;
        return ERR_SENSOR_NA;
      }
    } else {
      checked[index] = false;
    }
  }

  return ret < 0? ERR_SENSOR_NA: 0;
}

static int sensors_read_pax_therm(uint8_t sensor_num, float *value)
{
  int ret;

  if (!is_device_ready())
    return ERR_SENSOR_NA;

  if (pal_check_switch_config()) {
    ret = pal_get_pex_therm(sensor_num, value);
  } else {
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
        return ERR_SENSOR_NA;
    }
  }

  return ret < 0? ERR_SENSOR_NA: 0;
}

static int sensors_read_brcm_vr(uint8_t sensor_num, float *value)
{
  int ret;

  if (!is_device_ready())
    return ERR_SENSOR_NA;

  switch (sensor_num) {
    case MB_VR_P0V9_VDD0_VOUT:
      ret = sensors_read("isl69260-i2c-5-60", "VR_P0V9_VDD0_VOUT", value);
      break;
    case MB_VR_P0V9_VDD0_CURR:
      ret = sensors_read("isl69260-i2c-5-60", "VR_P0V9_VDD0_IOUT", value);
      break;
    case MB_VR_P0V9_VDD0_POUT:
      ret = sensors_read("isl69260-i2c-5-60", "VR_P0V9_VDD0_POUT", value);
      break;
    case MB_VR_P0V9_VDD0_TEMP:
      ret = sensors_read("isl69260-i2c-5-60", "VR_P0V9_VDD0_TEMP", value);
      break;
    case MB_VR_P0V9_VDD1_VOUT:
      ret = sensors_read("isl69260-i2c-5-61", "VR_P0V9_VDD1_VOUT", value);
      break;
    case MB_VR_P0V9_VDD1_CURR:
      ret = sensors_read("isl69260-i2c-5-61", "VR_P0V9_VDD1_IOUT", value);
      break;
    case MB_VR_P0V9_VDD1_POUT:
      ret = sensors_read("isl69260-i2c-5-61", "VR_P0V9_VDD1_POUT", value);
      break;
    case MB_VR_P0V9_VDD1_TEMP:
      ret = sensors_read("isl69260-i2c-5-61", "VR_P0V9_VDD1_TEMP", value);
      break;
    case MB_VR_P0V9_VDD2_VOUT:
      ret = sensors_read("isl69260-i2c-5-72", "VR_P0V9_VDD2_VOUT", value);
      break;
    case MB_VR_P0V9_VDD2_CURR:
      ret = sensors_read("isl69260-i2c-5-72", "VR_P0V9_VDD2_IOUT", value);
      break;
    case MB_VR_P0V9_VDD2_POUT:
      ret = sensors_read("isl69260-i2c-5-72", "VR_P0V9_VDD2_POUT", value);
      break;
    case MB_VR_P0V9_VDD2_TEMP:
      ret = sensors_read("isl69260-i2c-5-72", "VR_P0V9_VDD2_TEMP", value);
      break;
    case MB_VR_P0V9_VDD3_VOUT:
      ret = sensors_read("isl69260-i2c-5-75", "VR_P0V9_VDD3_VOUT", value);
      break;
    case MB_VR_P0V9_VDD3_CURR:
      ret = sensors_read("isl69260-i2c-5-75", "VR_P0V9_VDD3_IOUT", value);
      break;
    case MB_VR_P0V9_VDD3_POUT:
      ret = sensors_read("isl69260-i2c-5-75", "VR_P0V9_VDD3_POUT", value);
      break;
    case MB_VR_P0V9_VDD3_TEMP:
      ret = sensors_read("isl69260-i2c-5-75", "VR_P0V9_VDD3_TEMP", value);
      break;
    case MB_VR_P1V8_AVD0_VOUT:
      ret = sensors_read("mpq8645p-i2c-5-34", "VR_P1V0_AVD0_VOUT", value);
      break;
    case MB_VR_P1V8_AVD0_CURR:
      ret = sensors_read("mpq8645p-i2c-5-34", "VR_P1V0_AVD0_CURR", value);
      break;
    case MB_VR_P1V8_AVD0_TEMP:
      ret = sensors_read("mpq8645p-i2c-5-34", "VR_P1V0_AVD0_TEMP", value);
      break;
    case MB_VR_P1V8_AVD1_VOUT:
      ret = sensors_read("mpq8645p-i2c-5-35", "VR_P1V0_AVD1_VOUT", value);
      break;
    case MB_VR_P1V8_AVD1_CURR:
      ret = sensors_read("mpq8645p-i2c-5-35", "VR_P1V0_AVD1_CURR", value);
      break;
    case MB_VR_P1V8_AVD1_TEMP:
      ret = sensors_read("mpq8645p-i2c-5-35", "VR_P1V0_AVD1_TEMP", value);
      break;
    case MB_VR_P1V8_AVD2_VOUT:
      ret = sensors_read("mpq8645p-i2c-5-36", "VR_P1V0_AVD2_VOUT", value);
      break;
    case MB_VR_P1V8_AVD2_CURR:
      ret = sensors_read("mpq8645p-i2c-5-36", "VR_P1V0_AVD2_CURR", value);
      break;
    case MB_VR_P1V8_AVD2_TEMP:
      ret = sensors_read("mpq8645p-i2c-5-36", "VR_P1V0_AVD2_TEMP", value);
      break;
    case MB_VR_P1V8_AVD3_VOUT:
      ret = sensors_read("mpq8645p-i2c-5-3b", "VR_P1V0_AVD3_VOUT", value);
      break;
    case MB_VR_P1V8_AVD3_CURR:
      ret = sensors_read("mpq8645p-i2c-5-3b", "VR_P1V0_AVD3_CURR", value);
      break;
    case MB_VR_P1V8_AVD3_TEMP:
      ret = sensors_read("mpq8645p-i2c-5-3b", "VR_P1V0_AVD3_TEMP", value);
      break;
    default:
      ret = ERR_SENSOR_NA;
  }

  return ret < 0? ERR_SENSOR_NA: 0;
}

static int sensors_read_vr(uint8_t sensor_num, float *value)
{
  int ret;

  if (!is_device_ready())
    return ERR_SENSOR_NA;

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
      ret = ERR_SENSOR_NA;
  }

  return ret < 0? ERR_SENSOR_NA: 0;
}

static int sensors_read_12v_hsc(uint8_t sensor_num, float *value)
{
  int ret, index;
  static bool checked[8] = {false, false, false, false, false, false, false, false};
  *value = 0.0;

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
      ret = sensors_read("adm1278-i2c-18-42", "P12V_AUX_CURR", value);
      if (ret < 0)
        ret = sensors_read("ltc4282-i2c-18-43", "P12V_AUX_CURR", value);
      if (ret == 0)
        hsc_value_adjust(aux_current_table, value);
      break;
    case PDB_HSC_P12V_1_PWR:
      ret = sensors_read("ltc4282-i2c-16-53", "P12V_1_PWR", value);
      if (ret == 0)
        hsc_value_adjust(power_table, value);
      break;
    case PDB_HSC_P12V_1_PWR_PEAK:
      ret = sensors_read("ltc4282-i2c-16-53", "power1_input_highest", value);
      if (ret == 0)
        hsc_value_adjust(power_table, value);
      break;
    case PDB_HSC_P12V_2_PWR:
      ret = sensors_read("ltc4282-i2c-17-40", "P12V_2_PWR", value);
      if (ret == 0)
        hsc_value_adjust(power_table, value);
      break;
    case PDB_HSC_P12V_2_PWR_PEAK:
      ret = sensors_read("ltc4282-i2c-17-40", "power1_input_highest", value);
      if (ret == 0)
        hsc_value_adjust(power_table, value);
      break;
    case PDB_HSC_P12V_AUX_PWR:
      ret = sensors_read("adm1278-i2c-18-42", "P12V_AUX_PWR", value);
      if (ret < 0)
        ret = sensors_read("ltc4282-i2c-18-43", "P12V_AUX_PWR", value);
      if (ret == 0)
        hsc_value_adjust(aux_power_table, value);
      break;
    case PDB_HSC_P12V_AUX_PWR_PEAK:
      ret = sensors_read("adm1278-i2c-18-42", "power1_input_highest", value);
      if (ret < 0)
        ret = sensors_read("ltc4282-i2c-18-43", "power1_input_highest", value);
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
      ret = sensors_read("adm1278-i2c-18-42", "P12V_AUX_VIN", value);
      if (ret < 0)
        ret = sensors_read("ltc4282-i2c-18-43", "P12V_AUX_VIN", value);
      break;
    case PDB_HSC_P12V_AUX_VOUT:
      ret = sensors_read("adm1278-i2c-18-42", "P12V_AUX_VOUT", value);
      if (ret < 0)
        ret = sensors_read("ltc4282-i2c-18-43", "P12V_AUX_VOUT", value);
      break;
    default:
      ret = ERR_SENSOR_NA;
  }

  if (ret == 0) {
    if (sensor_num <= PDB_HSC_P12V_1_PWR)
      index = sensor_num - PDB_HSC_P12V_1_VIN;
    else if (sensor_num <= PDB_HSC_P12V_2_PWR)
      index = sensor_num - PDB_HSC_P12V_2_VIN + 4;
    else
      return 0;

    if (*value <= sensors_threshold[sensor_num][LCR_THRESH]) {
      if (!checked[index]) {
        checked[index] = true;
        return ERR_SENSOR_NA;
      }
    } else {
      checked[index] = false;
    }
  }

  return ret < 0? ERR_SENSOR_NA: 0;
}

static int sensors_read_12v_hsc_vout(uint8_t sensor_num, float *value)
{
  int ret;

  if (pal_is_server_off())
    return ERR_SENSOR_NA;

  switch (sensor_num) {
    case PDB_HSC_P12V_1_VOUT:
      ret = sensors_read("ltc4282-i2c-16-53", "P12V_1_VOUT", value);
      break;
    case PDB_HSC_P12V_2_VOUT:
      ret = sensors_read("ltc4282-i2c-17-40", "P12V_2_VOUT", value);
      break;
    default:
      ret = ERR_SENSOR_NA;
  }
  return ret < 0? ERR_SENSOR_NA: 0;
}

static int sensors_read_48v_hsc(uint8_t sensor_num, float *value)
{
  int ret;

  if (!is_device_ready())
    return ERR_SENSOR_NA;

  switch (sensor_num) {
    case PDB_HSC_P48V_1_VIN:
      ret = sensors_read("adm1272-i2c-16-13", "P48V_1_VIN", value);
      break;
    case PDB_HSC_P48V_2_VIN:
      ret = sensors_read("adm1272-i2c-17-10", "P48V_2_VIN", value);
      break;
    case PDB_HSC_P48V_1_CURR:
      ret = sensors_read("adm1272-i2c-16-13", "P48V_1_CURR", value);
      if (ret == 0)
        hsc_value_adjust(p48v_current_table, value);
      break;
    case PDB_HSC_P48V_2_CURR:
      ret = sensors_read("adm1272-i2c-17-10", "P48V_2_CURR", value);
      if (ret == 0)
        hsc_value_adjust(p48v_current_table, value);
      break;
    case PDB_HSC_P48V_1_PWR:
      ret = sensors_read("adm1272-i2c-16-13", "P48V_1_PWR", value);
      if (ret == 0)
        hsc_value_adjust(p48v_power_table, value);
      break;
    case PDB_HSC_P48V_1_PWR_PEAK:
      ret = sensors_read("adm1272-i2c-16-13", "power1_input_highest", value);
      if (ret == 0)
        hsc_value_adjust(p48v_power_table, value);
      break;
    case PDB_HSC_P48V_2_PWR:
      ret = sensors_read("adm1272-i2c-17-10", "P48V_2_PWR", value);
      if (ret == 0)
        hsc_value_adjust(p48v_power_table, value);
      break;
    case PDB_HSC_P48V_2_PWR_PEAK:
      ret = sensors_read("adm1272-i2c-17-10", "power1_input_highest", value);
      if (ret == 0)
        hsc_value_adjust(p48v_power_table, value);
      break;
    default:
      ret = ERR_SENSOR_NA;
  }

  if (*value < 0)
    *value = 0.0;

  return ret < 0? ERR_SENSOR_NA: 0;
}

static void enable_adm1272_vout(int bus, int addr)
{
  int fd;
  char dev[64] = {0};
  uint8_t tbuf[8] = {0};
  uint8_t rbuf[8] = {0};

  snprintf(dev, sizeof(dev), "/dev/i2c-%d", bus);
  fd = open(dev, O_RDWR);
  if (fd < 0)
    goto exit;

  // Enable Vout
  tbuf[0] = 0xd4; // PMON_CONFIG
  tbuf[1] = 0x37;
  tbuf[2] = 0x3f;
  i2c_rdwr_msg_transfer(fd, addr << 1, tbuf, 3, rbuf, 0);
  // ADM127x don't recommend to read data just after modifying power monitor register
  // The sampling probably was interrupted then reported wrong data.
  close(fd);
  return;
exit:
  syslog(LOG_CRIT, "Failed to enable P48V VOUT monitoring");
}

static int sensors_read_48v_hsc_vout(uint8_t sensor_num, float *value)
{
  int ret;
  static int pwr_flag[2] = {0, 0};

  if (!is_device_ready()) {
    pwr_flag[0] = pwr_flag[1] = 0;
    return ERR_SENSOR_NA;
  }

  switch (sensor_num) {
    case PDB_HSC_P48V_1_VOUT:
      if (pwr_flag[0] == 0) {
        enable_adm1272_vout(16, 0x13);
        pwr_flag[0] = 1;
        return ERR_SENSOR_NA;
      }

      ret = sensors_read("adm1272-i2c-16-13", "P48V_1_VOUT", value);
      break;
    case PDB_HSC_P48V_2_VOUT:
      if (pwr_flag[1] == 0) {
        enable_adm1272_vout(17, 0x10);
        pwr_flag[1] = 1;
        return ERR_SENSOR_NA;
      }

      ret = sensors_read("adm1272-i2c-17-10", "P48V_2_VOUT", value);
      break;
    default:
      ret = ERR_SENSOR_NA;
  }

  if (*value < 0)
    *value = 0.0;

  return ret < 0? ERR_SENSOR_NA: 0;
}

static int read_gpu_temp(uint8_t sensor_num, float *value)
{
  int ret;
  uint8_t slot = sensor_num - MB_GPU0_TEMP;

  if (!is_asic_prsnt(slot) || pal_is_server_off())
    return ERR_SENSOR_NA;

  ret = asic_read_gpu_temp(slot, value);
  return ret < 0? ERR_SENSOR_NA: 0;
}

static int read_asic_board_temp(uint8_t sensor_num, float *value)
{
  int ret;
  uint8_t slot = sensor_num - MB_GPU0_EDGE_TEMP;

  if (!is_asic_prsnt(slot) || pal_is_server_off())
    return ERR_SENSOR_NA;

  ret = asic_read_board_temp(slot, value);
  return ret < 0? ERR_SENSOR_NA: 0;
}

static int read_asic_mem_temp(uint8_t sensor_num, float *value)
{
  int ret;
  uint8_t slot = sensor_num - MB_GPU0_HBM_TEMP;

  if (!is_asic_prsnt(slot) || pal_is_server_off())
    return ERR_SENSOR_NA;

  ret = asic_read_mem_temp(slot, value);
  return ret < 0? ERR_SENSOR_NA: 0;
}

static int read_gpu_pwcs(uint8_t sensor_num, float *value)
{
  int ret;
  uint8_t slot = sensor_num - MB_GPU0_PWCS;

  if (!is_asic_prsnt(slot) || pal_is_server_off())
    return ERR_SENSOR_NA;

  ret = asic_read_pwcs(slot, value);
  return ret < 0? ERR_SENSOR_NA: 0;
}

int pal_get_fru_sensor_list(uint8_t fru, uint8_t **sensor_list, int *cnt)
{
  switch (fru) {
    case FRU_MB:
      if (pal_check_switch_config()) {
        *sensor_list = (uint8_t *) mb_brcm_switch_sensor_list;
        *cnt = mb_brcm_switch_sensor_cnt;
      } else {
        *sensor_list = (uint8_t *) mb_micro_switch_sensor_list;
        *cnt = mb_micro_switch_sensor_cnt;
      }
      break;
    case FRU_PDB:
      if (pal_check_pdb_vr_config()) {
        *sensor_list = (uint8_t *) pdb_vicor_sensor_list;
        *cnt = pdb_vicor_sensor_cnt;
      } else {
        *sensor_list = (uint8_t *) pdb_infineon_sensor_list;
        *cnt = pdb_infineon_sensor_cnt;
      }
      break;
    case FRU_BSM:
      *sensor_list = NULL;
      *cnt = 0;
      break;
    case FRU_ASIC0:
      *sensor_list = (uint8_t *) asic0_sensor_list;
      *cnt = asic0_sensor_cnt;
      break;
    case FRU_ASIC1:
      *sensor_list = (uint8_t *) asic1_sensor_list;
      *cnt = asic1_sensor_cnt;
      break;
    case FRU_ASIC2:
      *sensor_list = (uint8_t *) asic2_sensor_list;
      *cnt = asic2_sensor_cnt;
      break;
    case FRU_ASIC3:
      *sensor_list = (uint8_t *) asic3_sensor_list;
      *cnt = asic3_sensor_cnt;
      break;
    case FRU_ASIC4:
      *sensor_list = (uint8_t *) asic4_sensor_list;
      *cnt = asic4_sensor_cnt;
      break;
    case FRU_ASIC5:
      *sensor_list = (uint8_t *) asic5_sensor_list;
      *cnt = asic5_sensor_cnt;
      break;
    case FRU_ASIC6:
      *sensor_list = (uint8_t *) asic6_sensor_list;
      *cnt = asic6_sensor_cnt;
      break;
    case FRU_ASIC7:
      *sensor_list = (uint8_t *) asic7_sensor_list;
      *cnt = asic7_sensor_cnt;
      break;
    default:
      *sensor_list = NULL;
      *cnt = 0;
      return -1;
  };
  return 0;
}

static void init_sensor_threshold_by_mfr(uint8_t vendor)
{
  int sensor;

  if (vendor == GPU_AMD) {
    for (sensor = MB_GPU0_TEMP; sensor <= MB_GPU7_TEMP; sensor++) {
      sensors_threshold[sensor][LCR_THRESH] = 10.0;
      sensors_threshold[sensor][UCR_THRESH] = 100.0;
    }
    for (sensor = MB_GPU0_HBM_TEMP; sensor <= MB_GPU7_HBM_TEMP; sensor++) {
      sensors_threshold[sensor][LCR_THRESH] = 10.0;
      sensors_threshold[sensor][UCR_THRESH] = 94.0;
    }
  } else if (vendor == GPU_NVIDIA) {
    for (sensor = MB_GPU0_TEMP; sensor <= MB_GPU7_TEMP; sensor++) {
      sensors_threshold[sensor][LCR_THRESH] = 10.0;
      sensors_threshold[sensor][UCR_THRESH] = 85.0;
    }
    for (sensor = MB_GPU0_HBM_TEMP; sensor <= MB_GPU7_HBM_TEMP; sensor++) {
      sensors_threshold[sensor][LCR_THRESH] = 10.0;
      sensors_threshold[sensor][UCR_THRESH] = 95.0;
    }
  }
  return;
}

int pal_get_sensor_threshold(uint8_t fru, uint8_t sensor_num, uint8_t thresh, void *value)
{
  float *val = (float*) value;
  char vendor[16] = {0};
  static uint8_t vendor_id = GPU_UNKNOWN;

  if (fru == FRU_BSM || fru > FRU_ASIC7 || sensor_num >= FBEP_SENSOR_MAX)
    return -1;

  if (vendor_id == GPU_UNKNOWN) {
      pal_get_key_value("asic_mfr", vendor);
      if (!strcmp(vendor, MFR_AMD)) {
	vendor_id = GPU_AMD;
      } else if (!strcmp(vendor, MFR_NVIDIA)) {
	vendor_id = GPU_NVIDIA;
      }
      init_sensor_threshold_by_mfr(vendor_id);
  }

  *val = sensors_threshold[sensor_num][thresh];

  return 0;
}

int pal_get_sensor_name(uint8_t fru, uint8_t sensor_num, char *name)
{
  char vendor[16] = {0};
  static uint8_t vendor_id = GPU_UNKNOWN;

  if (fru == FRU_BSM || fru > FRU_ASIC7 || sensor_num >= FBEP_SENSOR_MAX)
    return -1;

  if (vendor_id == GPU_UNKNOWN) {
      pal_get_key_value("asic_mfr", vendor);
      if (!strcmp(vendor, MFR_AMD))
	vendor_id = GPU_AMD;
      else if (!strcmp(vendor, MFR_NVIDIA))
	vendor_id = GPU_NVIDIA;
  }

  if (sensor_num >= FBEP_SENSOR_MAX) {
    sprintf(name, "INVAILD SENSOR");
  } else if (vendor_id != GPU_UNKNOWN &&
             sensor_num >= MB_GPU0_TEMP && sensor_num <= MB_GPU7_PWCS) {
    sprintf(name, "%s", asic_sensor_name_by_mfr[vendor_id][sensor_num-MB_GPU0_TEMP]);
  } else {
    sprintf(name, "%s", fbep_sensors_map[sensor_num].name);
  }

  return 0;
}

int pal_get_sensor_units(uint8_t fru, uint8_t sensor_num, char *units)
{
  if (fru == FRU_BSM || fru > FRU_ASIC7 || sensor_num >= FBEP_SENSOR_MAX)
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
  if (fru == FRU_BSM || fru > FRU_ASIC7 || sensor_num >= FBEP_SENSOR_MAX)
    return -1;

  // default poll interval
  *value = 2;

  if (sensor_num == MB_ADC_P3V_BAT)
    *value = 3600;

  return PAL_EOK;
}

int pal_sensor_read_raw(uint8_t fru, uint8_t sensor_num, void *value)
{
  int ret, lock;
  char asic_lock[32] = {0};

  if (fru == FRU_BSM || fru > FRU_ASIC7 || sensor_num >= FBEP_SENSOR_MAX)
    return ERR_SENSOR_NA;

  if (fru >= FRU_ASIC0 && fru <= FRU_ASIC7) {
    snprintf(asic_lock, sizeof(asic_lock), "/tmp/asic_lock%d", (int)fru-FRU_ASIC0);
    lock = open(asic_lock, O_CREAT | O_RDWR, 0666);
    if (lock < 0) {
      return ERR_FAILURE; // Skip
    }
    if (flock(lock, LOCK_EX | LOCK_NB) && errno == EWOULDBLOCK) {
      close(lock);
      return ERR_FAILURE; // Skip
    }
  }

  ret = fbep_sensors_map[sensor_num].sensor_read(sensor_num, (float *)value);

  if (fru >= FRU_ASIC0 && fru <= FRU_ASIC7) {
    flock(lock, LOCK_UN);
    close(lock);
  }

  return ret;
}

