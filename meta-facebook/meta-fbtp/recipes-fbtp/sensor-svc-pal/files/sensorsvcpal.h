/*
 * sensorsvcpal.c
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

#ifndef __SENSOR_SVC_PAL_H__
#define __SENSOR_SVC_PAL_H__

#include <openbmc/obmc-pal.h>

#ifdef __cplusplus
extern "C" {
#endif

#include <openbmc/ipmi.h>
#include <openbmc/ipmb.h>
#include <stdbool.h>

enum {
  FRU_ALL   = 0,
  FRU_MB = 1,
  FRU_NIC = 2,
  FRU_RISER_SLOT2 = 3,
  FRU_RISER_SLOT3 = 4,
  FRU_RISER_SLOT4 = 5,
};

// Sensors Under Side Plane
enum {
  MB_SENSOR_HSC_IN_POWER = 0x29,
  MB_SENSOR_HSC_IN_VOLT = 0x2A,
  MB_SENSOR_CPU0_PKG_POWER = 0x2C,
  MB_SENSOR_CPU1_PKG_POWER = 0x2D,

  MB_SENSOR_CPU0_TJMAX = 0x30,
  MB_SENSOR_CPU1_TJMAX = 0x31,

  MB_SENSOR_PCH_TEMP = 0x08,

  MB_SENSOR_C4_AVA_FTEMP = 0x7E,
  MB_SENSOR_C4_AVA_RTEMP = 0x7F,

  MB_SENSOR_C2_AVA_FTEMP = 0x80,
  MB_SENSOR_C2_AVA_RTEMP = 0x81,
  MB_SENSOR_C2_1_NVME_CTEMP = 0x82,
  MB_SENSOR_C2_2_NVME_CTEMP = 0x83,
  MB_SENSOR_C2_3_NVME_CTEMP = 0x84,
  MB_SENSOR_C2_4_NVME_CTEMP = 0x85,
  MB_SENSOR_C3_AVA_FTEMP = 0x86,
  MB_SENSOR_C3_AVA_RTEMP = 0x87,
  MB_SENSOR_C3_1_NVME_CTEMP = 0x88,
  MB_SENSOR_C3_2_NVME_CTEMP = 0x89,
  MB_SENSOR_C3_3_NVME_CTEMP = 0x8A,
  MB_SENSOR_C3_4_NVME_CTEMP = 0x8B,
  MB_SENSOR_C4_1_NVME_CTEMP = 0x8C,
  MB_SENSOR_C4_2_NVME_CTEMP = 0x8D,
  MB_SENSOR_C4_3_NVME_CTEMP = 0x8E,
  MB_SENSOR_C4_4_NVME_CTEMP = 0x8F,

  MB_SENSOR_C2_P12V_INA230_VOL = 0x90,
  MB_SENSOR_C2_P12V_INA230_CURR = 0x91,
  MB_SENSOR_C2_P12V_INA230_PWR = 0x92,
  MB_SENSOR_C3_P12V_INA230_VOL = 0x93,
  MB_SENSOR_C3_P12V_INA230_CURR = 0x94,
  MB_SENSOR_C3_P12V_INA230_PWR = 0x95,
  MB_SENSOR_C4_P12V_INA230_VOL = 0x96,
  MB_SENSOR_C4_P12V_INA230_CURR = 0x97,
  MB_SENSOR_C4_P12V_INA230_PWR = 0x98,
  MB_SENSOR_CONN_P12V_INA230_VOL = 0x99,
  MB_SENSOR_CONN_P12V_INA230_CURR = 0x9A,
  MB_SENSOR_CONN_P12V_INA230_PWR = 0x9B,

  MB_SENSOR_INLET_TEMP = 0xA0,
  MB_SENSOR_OUTLET_TEMP = 0xA1,
  MB_SENSOR_INLET_REMOTE_TEMP = 0xA3,
  MB_SENSOR_OUTLET_REMOTE_TEMP = 0xA4,
  MB_SENSOR_CPU0_TEMP = 0xAA,
  MB_SENSOR_CPU1_TEMP = 0xAB,
  MB_SENSOR_CPU0_DIMM_GRPA_TEMP = 0xAC,
  MB_SENSOR_CPU0_DIMM_GRPB_TEMP = 0xAD,
  MB_SENSOR_CPU1_DIMM_GRPC_TEMP = 0xAE,
  MB_SENSOR_CPU1_DIMM_GRPD_TEMP = 0xAF,

  MB_SENSOR_VR_CPU0_VCCIN_VOLT = 0xB0,
  MB_SENSOR_VR_CPU0_VCCIN_TEMP = 0xB1,
  MB_SENSOR_VR_CPU0_VCCIN_CURR = 0xB2,
  MB_SENSOR_VR_CPU0_VCCIN_POWER = 0xB3,
  MB_SENSOR_VR_CPU0_VSA_VOLT = 0xB4,
  MB_SENSOR_VR_CPU0_VSA_TEMP = 0xB5,
  MB_SENSOR_VR_CPU0_VSA_CURR = 0xB6,
  MB_SENSOR_VR_CPU0_VSA_POWER = 0xB7,
  MB_SENSOR_VR_CPU0_VCCIO_VOLT = 0xB8,
  MB_SENSOR_VR_CPU0_VCCIO_TEMP = 0xB9,
  MB_SENSOR_VR_CPU0_VCCIO_CURR = 0xBA,
  MB_SENSOR_VR_CPU0_VCCIO_POWER = 0xBB,
  MB_SENSOR_VR_CPU0_VDDQ_GRPA_VOLT = 0xBC,
  MB_SENSOR_VR_CPU0_VDDQ_GRPA_TEMP = 0xBD,
  MB_SENSOR_VR_CPU0_VDDQ_GRPA_CURR = 0xBE,
  MB_SENSOR_VR_CPU0_VDDQ_GRPA_POWER = 0xBF,

  MB_SENSOR_FAN0_TACH = 0xC0,
  MB_SENSOR_HSC_OUT_CURR = 0xC1,
  MB_SENSOR_HSC_TEMP = 0xC2,
  MB_SENSOR_FAN1_TACH = 0xC3,
  MB_SENSOR_VR_PCH_P1V05_VOLT = 0xC4,
  MB_SENSOR_VR_PCH_P1V05_TEMP = 0xC5,
  MB_SENSOR_VR_PCH_P1V05_CURR = 0xC6,
  MB_SENSOR_VR_PCH_P1V05_POWER = 0xC7,
  MB_SENSOR_VR_PCH_PVNN_VOLT = 0xC8,
  MB_SENSOR_VR_PCH_PVNN_TEMP = 0xC9,
  MB_SENSOR_VR_PCH_PVNN_CURR = 0xCA,
  MB_SENSOR_VR_PCH_PVNN_POWER = 0xCB,
  MB_SENSOR_VR_CPU0_VDDQ_GRPB_VOLT = 0xCC,
  MB_SENSOR_VR_CPU0_VDDQ_GRPB_TEMP = 0xCD,
  MB_SENSOR_VR_CPU0_VDDQ_GRPB_CURR = 0xCE,
  MB_SENSOR_VR_CPU0_VDDQ_GRPB_POWER = 0xCF,

  MB_SENSOR_P3V_BAT = 0xD0,
  MB_SENSOR_P3V3 = 0xD1,
  MB_SENSOR_P5V = 0xD2,
  MB_SENSOR_P12V = 0xD3,
  MB_SENSOR_P1V05 = 0xD4,
  MB_SENSOR_PVNN_PCH_STBY = 0xD5,
  MB_SENSOR_P3V3_STBY = 0xD6,
  MB_SENSOR_P5V_STBY = 0xD7,
  MB_SENSOR_VR_CPU1_VDDQ_GRPC_POWER = 0xD8,
  MB_SENSOR_VR_CPU1_VDDQ_GRPD_VOLT = 0xD9,
  MB_SENSOR_VR_CPU1_VDDQ_GRPD_TEMP = 0xDA,
  MB_SENSOR_VR_CPU1_VDDQ_GRPD_CURR = 0XDB,
  MB_SENSOR_VR_CPU1_VDDQ_GRPD_POWER = 0XDC,

  MB_SENSOR_VR_CPU1_VCCIN_VOLT = 0xF0,
  MB_SENSOR_VR_CPU1_VCCIN_TEMP = 0xF1,
  MB_SENSOR_VR_CPU1_VCCIN_CURR = 0xF2,
  MB_SENSOR_VR_CPU1_VCCIN_POWER = 0xF3,
  MB_SENSOR_VR_CPU1_VSA_VOLT = 0xF4,
  MB_SENSOR_VR_CPU1_VSA_TEMP = 0xF5,
  MB_SENSOR_VR_CPU1_VSA_CURR = 0xF6,
  MB_SENSOR_VR_CPU1_VSA_POWER = 0xF7,
  MB_SENSOR_VR_CPU1_VCCIO_VOLT = 0xF8,
  MB_SENSOR_VR_CPU1_VCCIO_TEMP = 0xF9,
  MB_SENSOR_VR_CPU1_VCCIO_CURR = 0xFA,
  MB_SENSOR_VR_CPU1_VCCIO_POWER = 0xFB,
  MB_SENSOR_VR_CPU1_VDDQ_GRPC_VOLT = 0xFC,
  MB_SENSOR_VR_CPU1_VDDQ_GRPC_TEMP = 0xFD,
  MB_SENSOR_VR_CPU1_VDDQ_GRPC_CURR = 0xFE,

//Discrete sensor
  MB_SENSOR_POWER_FAIL = 0x9C,
  MB_SENSOR_MEMORY_LOOP_FAIL = 0x9D,
  MB_SENSOR_PROCESSOR_FAIL = 0x65,
};

enum{
  MEZZ_SENSOR_TEMP = 0xA2,
};

enum {
  SLOT_CFG_SS_2x16  = 0x00,
  SLOT_CFG_DS_2x8   = 0x01,
  SLOT_CFG_SS_3x8   = 0x02,
  SLOT_CFG_EMPTY    = 0x03,
};

//CPLD power status register deifne
enum {
  MAIN_PWR_STS_REG = 0x00,
  CPU0_PWR_STS_REG = 0x01,
  CPU1_PWR_STS_REG = 0x02,
  PWRDATA1_REG = 0x03,
  PWRDATA2_REG = 0x04,
  PWRDATA3_REG = 0x05,
  PWRDATA4_REG = 0x06,
  PWRDATA5_REG = 0x07,
  SYSDATA1_REG = 0x08,
  SYSDATA21_REG = 0x09,
};
//CPLD power status register normal value deifne
enum {
  MAIN_PWR_STS_VAL = 0x04,
  CPU0_PWR_STS_VAL = 0x13,
  CPU1_PWR_STS_VAL = 0x13,
  PWRDATA1_VAL = 0xff,
  PWRDATA2_VAL = 0xff,
  PWRDATA3_VAL = 0xc0,
  PWRDATA4_VAL = 0x00,
  PWRDATA5_VAL = 0xff,
  SYSDATA1_VAL = 0xf3,
  SYSDATA21_VAL = 0x1f,
};

int pal_read_ava_temp(uint8_t sensor_num, float *value);
int pal_read_nvme_temp(uint8_t sensor_num, float *value);
int pal_read_INA230 (uint8_t sensor_num, float *value, int pot);
void pal_apply_inlet_correction(float *value);
int pal_read_sensor_reading_from_ME(uint8_t snr_num, float *value);
int pal_read_hsc_current_value(float *value);
int pal_read_CPLD_power_fail_sts (uint8_t fru, uint8_t sensor_num, float *value, int pot);
int pal_read_cpu_temp(uint8_t snr_num, float *value);
int pal_read_dimm_temp(uint8_t snr_num, float *value);
int pal_read_cpu_package_power(uint8_t snr_num, float *value);
int pal_check_postcodes(uint8_t fru_id, uint8_t sensor_num, float *value);
int pal_check_frb3(uint8_t fru_id, uint8_t sensor_num, float *value);
bool pal_is_cpu1_socket_occupy(void);

#ifdef __cplusplus
} // extern "C"
#endif

#endif /* __SENSOR_SVC_PAL_H__ */
