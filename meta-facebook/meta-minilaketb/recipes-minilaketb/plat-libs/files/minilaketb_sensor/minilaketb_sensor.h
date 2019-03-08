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

#ifndef __MINILAKETB_SENSOR_H__
#define __MINILAKETB_SENSOR_H__

#include <stdbool.h>
#include <openbmc/ipmi.h>
#include <openbmc/ipmb.h>
#include <openbmc/obmc-pal.h>
#include <facebook/bic.h>
#include <facebook/minilaketb_common.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_SDR_LEN           64
#define MAX_SENSOR_NUM        0xFF
#define MAX_SENSOR_THRESHOLD  8
#define MAX_RETRIES_SDR_INIT  30
#define THERMAL_CONSTANT      256
#define ERR_NOT_READY         -2
#define EER_READ_NA           -3

enum{ //sync Minilake BIC spec 0.03
    BIC_SENSOR_MB_OUTLET_TEMP = 0x01,
    BIC_SENSOR_MB_INLET_TEMP = 0x07,
    BIC_SENSOR_PCH_TEMP = 0x08,
    BIC_SENSOR_VCCIN_VR_TEMP = 0x80,
    BIC_SENSOR_1V05MIX_VR_TEMP = 0x81,
    BIC_SENSOR_SOC_TEMP = 0x05,
    BIC_SENSOR_SOC_THERM_MARGIN = 0x09,
    BIC_SENSOR_VDDR_VR_TEMP = 0x82,
    BIC_SENSOR_SOC_DIMMA0_TEMP = 0xB4,
    BIC_SENSOR_SOC_DIMMA1_TEMP = 0xB5,
    BIC_SENSOR_SOC_DIMMB0_TEMP = 0xB6,
    BIC_SENSOR_SOC_DIMMB1_TEMP = 0xB7,
    BIC_SENSOR_SOC_PACKAGE_PWR = 0x2C,
    BIC_SENSOR_VCCIN_VR_POUT = 0x8B,
    BIC_SENSOR_VDDR_VR_POUT = 0x8D,
    BIC_SENSOR_SOC_TJMAX = 0x30,
    BIC_SENSOR_P3V3_MB = 0xD0,
    BIC_SENSOR_P12V_MB = 0xD2,
    BIC_SENSOR_P1V05_PCH = 0xD3,
    BIC_SENSOR_P3V3_STBY_MB = 0xD5,
    BIC_SENSOR_P5V_STBY_MB = 0xD6,
    BIC_SENSOR_PV_BAT = 0xD7,
    BIC_SENSOR_PVDDR = 0xD8,
    BIC_SENSOR_P1V05_MIX = 0x8E,
    BIC_SENSOR_1V05MIX_VR_CURR = 0x84,
    BIC_SENSOR_VDDR_VR_CURR = 0x85,
    BIC_SENSOR_VCCIN_VR_CURR = 0x83,
    BIC_SENSOR_VCCIN_VR_VOL = 0x88,
    BIC_SENSOR_VDDR_VR_VOL = 0x8A,
    BIC_SENSOR_P1V05MIX_VR_VOL = 0x89,
    BIC_SENSOR_P1V05MIX_VR_Pout = 0x8C,
    BIC_SENSOR_INA230_POWER = 0x29,
    BIC_SENSOR_INA230_VOL = 0x2A,

    BIC_SENSOR_SYSTEM_STATUS = 0x10, //Discrete
    BIC_SENSOR_PROC_FAIL = 0x65, //Discrete
    BIC_SENSOR_SYS_BOOT_STAT = 0x7E, //Discrete
    BIC_SENSOR_CPU_DIMM_HOT = 0xB3, //Discrete
    BIC_SENSOR_VR_HOT = 0xB2, //Discrete

    BIC_SENSOR_POWER_THRESH_EVENT = 0x3B, //Event-only
    BIC_SENSOR_POST_ERR = 0x2B, //Event-only
    BIC_SENSOR_POWER_ERR = 0x56, //Event-only
    BIC_SENSOR_PROC_HOT_EXT = 0x51, //Event-only
    BIC_SENSOR_MACHINE_CHK_ERR = 0x40, //Event-only
    BIC_SENSOR_PCIE_ERR = 0x41, //Event-only
    BIC_SENSOR_OTHER_IIO_ERR = 0x43, //Event-only
    BIC_SENSOR_MEM_ECC_ERR = 0x63, //Event-only
    BIC_SENSOR_SPS_FW_HLTH = 0x17, //Event-only
    BIC_SENSOR_CAT_ERR = 0xEB, //Event-only
};

// Sensors Under Side Plane
enum {
  SP_SENSOR_INLET_TEMP = 0x91,
  SP_SENSOR_OUTLET_TEMP = 0x90,
  SP_SENSOR_MEZZ_TEMP = 0x82,
  SP_SENSOR_FAN0_TACH = 0x46,
  SP_SENSOR_FAN1_TACH = 0x47,
  SP_SENSOR_AIR_FLOW = 0x4A,
  SP_SENSOR_P5V = 0xE0,
  SP_SENSOR_P12V = 0xE1,
  SP_SENSOR_P3V3_STBY = 0xE2,
  SP_SENSOR_P12V_SLOT1 = 0xE3,
  SP_SENSOR_P3V3 = 0xE7,
  SP_SENSOR_P1V15_BMC_STBY = 0xC8,
  SP_SENSOR_P1V2_BMC_STBY = 0xC9,
  SP_SENSOR_P2V5_BMC_STBY = 0xCA,
  SP_P1V8_STBY = 0xCB,
  SP_SENSOR_HSC_IN_VOLT = 0xC0,
  SP_SENSOR_HSC_OUT_CURR = 0xC1,
  SP_SENSOR_HSC_TEMP = 0xC2,
  SP_SENSOR_HSC_IN_POWER = 0xC3,
};

//Glacier Point
enum {
  DC_SENSOR_OUTLET_TEMP = 0x84,
  DC_SENSOR_INLET_TEMP = 0x85,
  DC_SENSOR_INA230_VOLT = 0x86,
  DC_SENSOR_INA230_POWER = 0x87,
  DC_SENSOR_NVMe1_CTEMP = 0x8A,
  DC_SENSOR_NVMe2_CTEMP = 0x8B,
  DC_SENSOR_NVMe3_CTEMP = 0x8C,
  DC_SENSOR_NVMe4_CTEMP = 0x8D,
  DC_SENSOR_NVMe5_CTEMP = 0x8E,
  DC_SENSOR_NVMe6_CTEMP = 0x8F,
};

//Crane Flat
enum {
  DC_CF_SENSOR_OUTLET_TEMP = 0x91,
  DC_CF_SENSOR_INLET_TEMP = 0x92,
  DC_CF_SENSOR_INA230_VOLT = 0x93,
  DC_CF_SENSOR_INA230_POWER = 0x94,
};


enum {
  MUX_CH_0 = 0,
  MUX_CH_1 = 1,
  MUX_CH_2 = 2,
  MUX_CH_3 = 3,
  MUX_CH_4 = 4,
  MUX_CH_5 = 5,
};


extern const uint8_t bic_sensor_list[];
extern const uint8_t bic_discrete_list[];

extern const uint8_t dc_sensor_list[];

extern const uint8_t dc_cf_sensor_list[];

extern const uint8_t spb_sensor_list[];

extern const uint8_t nic_sensor_list[];

//extern float spb_sensor_threshold[MAX_SENSOR_NUM][MAX_SENSOR_THRESHOLD + 1];

extern float nic_sensor_threshold[MAX_SENSOR_NUM][MAX_SENSOR_THRESHOLD + 1];

extern size_t bic_sensor_cnt;

extern size_t dc_sensor_cnt;

extern size_t bic_discrete_cnt;

extern size_t spb_sensor_cnt;

extern size_t nic_sensor_cnt;

extern size_t dc_cf_sensor_cnt;

int minilaketb_sensor_read(uint8_t fru, uint8_t sensor_num, void *value);
int minilaketb_sensor_name(uint8_t fru, uint8_t sensor_num, char *name);
int minilaketb_sensor_units(uint8_t fru, uint8_t sensor_num, char *units);
int minilaketb_sensor_sdr_path(uint8_t fru, char *path);
int minilaketb_sensor_threshold(uint8_t fru, uint8_t sensor_num, uint8_t thresh, float *value);
int minilaketb_sensor_sdr_init(uint8_t fru, sensor_info_t *sinfo);
int minilaketb_get_slot_type(uint8_t fru);
int minilaketb_get_server_type(uint8_t fru, uint8_t *type);


#ifdef __cplusplus
} // extern "C"
#endif

#endif /* __MINILAKETB_SENSOR_H__ */
