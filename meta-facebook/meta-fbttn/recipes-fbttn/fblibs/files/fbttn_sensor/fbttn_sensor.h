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

#ifndef __FBTTN_SENSOR_H__
#define __FBTTN_SENSOR_H__

#include <stdbool.h>
#include <openbmc/ipmi.h>
#include <openbmc/ipmb.h>
#include <openbmc/obmc-pal.h>
#include <facebook/bic.h>
#include <facebook/mctp.h>
#include <facebook/fbttn_common.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_SDR_LEN           64
#define MAX_SENSOR_NUM        0xFF
#define MAX_SENSOR_THRESHOLD  8
#define MAX_RETRIES_SDR_INIT  30
#define THERMAL_CONSTANT      255
#define ERR_NOT_READY         -2

typedef struct _sensor_info_t {
  bool valid;
  sdr_full_t sdr;
} sensor_info_t;

// Sensors under Bridge IC
enum {
  BIC_SENSOR_MB_OUTLET_TEMP = 0x01,
  BIC_SENSOR_VCCIN_VR_TEMP = 0x02,
  BIC_SENSOR_VCC_GBE_VR_TEMP = 0x03,
  BIC_SENSOR_1V05PCH_VR_TEMP = 0x04,
  BIC_SENSOR_SOC_TEMP = 0x05,
  BIC_SENSOR_MB_INLET_TEMP = 0x07,
  BIC_SENSOR_PCH_TEMP = 0x08,
  BIC_SENSOR_SOC_THERM_MARGIN = 0x09,
  BIC_SENSOR_VDDR_VR_TEMP = 0x0B,
  BIC_SENSOR_SYSTEM_STATUS = 0x10, //Discrete
  BIC_SENSOR_SPS_FW_HLTH = 0x17, //Event-only
  BIC_SENSOR_VCC_GBE_VR_CURR = 0x20,
  BIC_SENSOR_1V05_PCH_VR_CURR = 0x21,
  BIC_SENSOR_VCCIN_VR_POUT = 0x22,
  BIC_SENSOR_VCCIN_VR_CURR = 0x23,
  BIC_SENSOR_VCCIN_VR_VOL = 0x24,
  BIC_SENSOR_INA230_POWER = 0x29,
  BIC_SENSOR_INA230_VOL = 0x2A,
  BIC_SENSOR_POST_ERR = 0x2B, //Event-only
  BIC_SENSOR_SOC_PACKAGE_PWR = 0x2C,
  BIC_SENSOR_SOC_TJMAX = 0x30,
  BIC_SENSOR_VDDR_VR_POUT = 0x32,
  BIC_SENSOR_VDDR_VR_CURR = 0x33,
  BIC_SENSOR_VDDR_VR_VOL = 0x34,
  BIC_SENSOR_VCC_SCSUS_VR_CURR = 0x35,
  BIC_SENSOR_VCC_SCSUS_VR_VOL = 0x36,
  BIC_SENSOR_VCC_SCSUS_VR_TEMP = 0x37,
  BIC_SENSOR_VCC_SCSUS_VR_POUT = 0x38,
  BIC_SENSOR_VCC_GBE_VR_POUT = 0x39,
  BIC_SENSOR_POWER_THRESH_EVENT = 0x3B, //Event-only
  BIC_SENSOR_MACHINE_CHK_ERR = 0x40, //Event-only
  BIC_SENSOR_PCIE_ERR = 0x41, //Event-only
  BIC_SENSOR_1V05_PCH_VR_POUT = 0x42,
  BIC_SENSOR_OTHER_IIO_ERR = 0x43, //Event-only
  BIC_SENSOR_PROC_HOT_EXT = 0x51, //Event-only
  BIC_SENSOR_VCC_GBE_VR_VOL = 0x54,
  BIC_SENSOR_1V05_PCH_VR_VOL = 0x55,
  BIC_SENSOR_POWER_ERR = 0x56, //Event-only
  BIC_SENSOR_MEM_ECC_ERR = 0x63, //Event-only
  BIC_SENSOR_PROC_FAIL = 0x65, //Discrete
  BIC_SENSOR_SYS_BOOT_STAT = 0x7E, //Discrete
  BIC_SENSOR_VR_HOT = 0xB2, //Discrete
  BIC_SENSOR_CPU_DIMM_HOT = 0xB3, //Discrete
  BIC_SENSOR_SOC_DIMMA0_TEMP = 0xB4,
  BIC_SENSOR_SOC_DIMMA1_TEMP = 0xB5,
  BIC_SENSOR_SOC_DIMMB0_TEMP = 0xB6,
  BIC_SENSOR_SOC_DIMMB1_TEMP = 0xB7,
  BIC_SENSOR_P3V3_MB = 0xD0,
  BIC_SENSOR_P12V_MB = 0xD2,
  BIC_SENSOR_P1V05_PCH = 0xD3,
  BIC_SENSOR_P3V3_STBY_MB = 0xD5,
  BIC_SENSOR_P5V_STBY_MB = 0xD6,
  BIC_SENSOR_PV_BAT = 0xD7,
  BIC_SENSOR_PVDDR = 0xD8,
  BIC_SENSOR_PVCC_GBE = 0xD9,
  BIC_SENSOR_CAT_ERR = 0xEB, //Event-only
};

// Sensors under IOM
enum{
  ML_SENSOR_HSC_VOLT = 0xC,
  ML_SENSOR_HSC_CURR = 0xD,
  ML_SENSOR_HSC_PWR  = 0xE,
  IOM_SENSOR_MEZZ_TEMP = 0x44,
  IOM_SENSOR_HSC_POWER = 0x58,
  IOM_SENSOR_HSC_VOLT = 0x46,
  IOM_SENSOR_HSC_CURR = 0x47,
  IOM_SENSOR_ADC_12V = 0x4A,
  IOM_SENSOR_ADC_P5V_STBY = 0x4B,
  IOM_SENSOR_ADC_P3V3_STBY = 0x4C,
  IOM_SENSOR_ADC_P1V8_STBY = 0x4D,
  IOM_SENSOR_ADC_P2V5_STBY = 0x4E,
  IOM_SENSOR_ADC_P1V2_STBY = 0x4F,
  IOM_SENSOR_ADC_P1V15_STBY = 0x50,
  IOM_SENSOR_ADC_P3V3 = 0x45,
  IOM_SENSOR_ADC_P1V8 = 0x52,
  IOM_SENSOR_ADC_P1V5 = 0x53,     // type VII only
  IOM_SENSOR_ADC_P0V975 = 0x57,   // type VII only
  IOM_SENSOR_ADC_P3V3_M2 = 0x76,  // type V only
  IOM_SENSOR_M2_AMBIENT_TEMP1 = 0x85,
  IOM_SENSOR_M2_AMBIENT_TEMP2 = 0x86,
  IOM_SENSOR_M2_SMART_TEMP1 = 0x48,
  IOM_SENSOR_M2_SMART_TEMP2 = 0x49,
  IOM_IOC_TEMP = 0x77, // type VII only
};

// Sensors under DPB, queried by Expander
enum{
  DPB_SENSOR_FAN1_FRONT = 0x69,
  DPB_SENSOR_FAN1_REAR = 0x6A,
  DPB_SENSOR_FAN2_FRONT = 0x6B,
  DPB_SENSOR_FAN2_REAR = 0x6C,
  DPB_SENSOR_FAN3_FRONT = 0x6D,
  DPB_SENSOR_FAN3_REAR = 0x6E,
  DPB_SENSOR_FAN4_FRONT = 0x6F,
  DPB_SENSOR_FAN4_REAR = 0x70,
  DPB_SENSOR_HSC_POWER = 0x71,
  DPB_SENSOR_HSC_VOLT = 0x72,
  DPB_SENSOR_HSC_CURR = 0x73,
  P3V3_SENSE = 0x18,
  P5V_1_SENSE = 0x19,
  P5V_2_SENSE = 0x1a,
  P5V_3_SENSE = 0x1b,
  P5V_4_SENSE = 0x1c,
  DPB_SENSOR_12V_POWER_CLIP = 0x66,
  DPB_SENSOR_P12V_CLIP = 0x67,
  DPB_SENSOR_12V_CURR_CLIP = 0x68,
  DPB_SENSOR_A_TEMP = 0x74,
  DPB_SENSOR_B_TEMP = 0x75,
  DPB_SENSOR_HDD_0 = 0x8E,
  DPB_SENSOR_HDD_1 = 0x8F,
  DPB_SENSOR_HDD_2 = 0x90,
  DPB_SENSOR_HDD_3 = 0x91,
  DPB_SENSOR_HDD_4 = 0x92,
  DPB_SENSOR_HDD_5 = 0x93,
  DPB_SENSOR_HDD_6 = 0x94,
  DPB_SENSOR_HDD_7 = 0x95,
  DPB_SENSOR_HDD_8 = 0x96,
  DPB_SENSOR_HDD_9 = 0x97,
  DPB_SENSOR_HDD_10 = 0x98,
  DPB_SENSOR_HDD_11 = 0x99,
  DPB_SENSOR_HDD_12 = 0x9A,
  DPB_SENSOR_HDD_13 = 0x9B,
  DPB_SENSOR_HDD_14 = 0x9C,
  DPB_SENSOR_HDD_15 = 0x9D,
  DPB_SENSOR_HDD_16 = 0x9E,
  DPB_SENSOR_HDD_17 = 0x9F,
  DPB_SENSOR_HDD_18 = 0xA0,
  DPB_SENSOR_HDD_19 = 0xA1,
  DPB_SENSOR_HDD_20 = 0xA2,
  DPB_SENSOR_HDD_21 = 0xA3,
  DPB_SENSOR_HDD_22 = 0xA4,
  DPB_SENSOR_HDD_23 = 0xA5,
  DPB_SENSOR_HDD_24 = 0xA6,
  DPB_SENSOR_HDD_25 = 0xA7,
  DPB_SENSOR_HDD_26 = 0xA8,
  DPB_SENSOR_HDD_27 = 0xA9,
  DPB_SENSOR_HDD_28 = 0xAA,
  DPB_SENSOR_HDD_29 = 0xAB,
  DPB_SENSOR_HDD_30 = 0xAC,
  DPB_SENSOR_HDD_31 = 0xAD,
  DPB_SENSOR_HDD_32 = 0xAE,
  DPB_SENSOR_HDD_33 = 0xAF,
  DPB_SENSOR_HDD_34 = 0xB0,
  DPB_SENSOR_HDD_35 = 0xB1,
  AIRFLOW = 0x76,
};

// Sensors under SCC, queried by Expander
enum{
  SCC_Drawer = 0x0, //discrete for Drawer Pull Out and Push In
  SCC_SENSOR_EXPANDER_TEMP = 0x60,
  SCC_SENSOR_IOC_TEMP = 0x61,
  SCC_SENSOR_HSC_POWER = 0x62,
  SCC_SENSOR_HSC_CURR = 0x83,
  SCC_SENSOR_HSC_VOLT = 0x84,
  SCC_SENSOR_P3V3_SENSE = 0x59,
  SCC_SENSOR_P1V8_E_SENSE = 0x5A,
  SCC_SENSOR_P1V5_E_SENSE = 0x5B,
  SCC_SENSOR_P0V9_SENSE = 0x5C,
  SCC_SENSOR_P1V8_C_SENSE = 0x5D,
  SCC_SENSOR_P1V5_C_SENSE = 0x5E,
  SCC_SENSOR_P0V975_SENSE = 0x5F,
};

// Sensors under NIC
enum{
  MEZZ_SENSOR_TEMP = 0x82,
};

// PCA9555 GPIO ports
enum {
  SLOT_INS = 472,
  FAN_0_INS = 473,
  FAN_1_INS = 474,
  FAN_2_INS = 475,
  FAN_3_INS = 476,
  RMT_IOM_INS = 477,
  SCC_A_INS = 478,
  SCC_B_INS = 479,
  SCC_TYPE_0 = 480,
  SCC_TYPE_1 = 481,
  SCC_TYPE_2 = 482,
  SCC_TYPE_3 = 483,
  SCC_STBY_PGOOD = 484,
  SCC_FULL_PGOOD = 485,
  SLOT_PGOOD = 486,
  DRAWER_CLOSED = 487,
};

// Server status
enum {
  SERVER_STATUS_POWER_OFF,
  SERVER_STATUS_POWER_ON,
  SERVER_STATUS_POWER_CYCLE,
  SERVER_STATUS_POWER_RESET,
  SERVER_STATUS_GRACEFUL_SHUTDOWN,
  SERVER_STATUS_12V_OFF,
  SERVER_STATUS_12V_ON,
  SERVER_STATUS_12V_CYCLE,
};

extern const uint8_t bic_sensor_list[];

extern const uint8_t bic_discrete_list[];

extern const uint8_t iom_sensor_list_type5[];

extern const uint8_t iom_sensor_list_type5_dvt[];

extern const uint8_t iom_sensor_list_type7[];

extern const uint8_t dpb_sensor_list[];

extern const uint8_t scc_sensor_list[];

extern const uint8_t dpb_discrete_list[];

extern const uint8_t nic_sensor_list[];

extern const uint8_t iom_t5_non_stby_sensor_list[];

extern const uint8_t iom_t7_non_stby_sensor_list[];

extern float nic_sensor_threshold[MAX_SENSOR_NUM][MAX_SENSOR_THRESHOLD + 1];

extern size_t bic_sensor_cnt;

extern size_t bic_discrete_cnt;

extern size_t iom_sensor_cnt_type5;

extern size_t iom_sensor_cnt_type5_dvt;

extern size_t iom_sensor_cnt_type7;

extern size_t dpb_sensor_cnt;

extern size_t dpb_discrete_cnt;

extern size_t scc_sensor_cnt;

extern size_t nic_sensor_cnt;

extern size_t iom_t5_non_stby_sensor_cnt;

extern size_t iom_t7_non_stby_sensor_cnt;

int fbttn_sensor_read(uint8_t fru, uint8_t sensor_num, void *value, uint8_t status, char *key);
int fbttn_sensor_name(uint8_t fru, uint8_t sensor_num, char *name);
int fbttn_sensor_units(uint8_t fru, uint8_t sensor_num, char *units);
int fbttn_sensor_sdr_path(uint8_t fru, char *path);
int fbttn_sensor_threshold(uint8_t fru, uint8_t sensor_num, uint8_t thresh, float *value);
int fbttn_sensor_sdr_init(uint8_t fru, sensor_info_t *sinfo);
bool is_server_prsnt(uint8_t fru);
bool is_scc_prsnt(uint8_t scc_port);
bool is_fan_prsnt(uint8_t fan_port);


#ifdef __cplusplus
} // extern "C"
#endif

#endif /* __FBTTN_SENSOR_H__ */
