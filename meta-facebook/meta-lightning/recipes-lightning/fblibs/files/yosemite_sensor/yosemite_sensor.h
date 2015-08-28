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

#ifndef __YOSEMITE_SENSOR_H__
#define __YOSEMITE_SENSOR_H__

#include <stdbool.h>
#include <openbmc/ipmi.h>
#include <openbmc/ipmb.h>
#include <openbmc/sdr.h>
#include <facebook/bic.h>
#include <facebook/yosemite_common.h>

#ifdef __cplusplus
extern "C" {
#endif

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
  //BIC_SENSOR_1V05_PCH_VR_POUT = 0x40,
  BIC_SENSOR_MACHINE_CHK_ERR = 0x40, //Event-only
  BIC_SENSOR_PCIE_ERR = 0x41, //Event-only
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

// Sensors Under Side Plane
enum {
  SP_SENSOR_INLET_TEMP = 0x81,
  SP_SENSOR_OUTLET_TEMP = 0x80,
  SP_SENSOR_MEZZ_TEMP = 0x82,
  SP_SENSOR_FAN0_TACH = 0x46,
  SP_SENSOR_FAN1_TACH = 0x47,
  SP_SENSOR_AIR_FLOW = 0x4A,
  SP_SENSOR_P5V = 0xE0,
  SP_SENSOR_P12V = 0xE1,
  SP_SENSOR_P3V3_STBY = 0xE2,
  SP_SENSOR_P12V_SLOT0 = 0xE3,
  SP_SENSOR_P12V_SLOT1 = 0xE4,
  SP_SENSOR_P12V_SLOT2 = 0xE5,
  SP_SENSOR_P12V_SLOT3 = 0xE6,
  SP_SENSOR_P3V3 = 0xE7,
  SP_SENSOR_HSC_IN_VOLT = 0xC0,
  SP_SENSOR_HSC_OUT_CURR = 0xC1,
  SP_SENSOR_HSC_TEMP = 0xC2,
  SP_SENSOR_HSC_IN_POWER = 0xC3,
};

extern const uint8_t bic_sensor_list[];

extern const uint8_t spb_sensor_list[];

extern size_t bic_sensor_cnt;

extern size_t spb_sensor_cnt;

int yosemite_sensor_read(uint8_t slot_id, uint8_t sensor_num, void *value);
int yosemite_sensor_name(uint8_t fru, uint8_t sensor_num, char *name);
int yosemite_sensor_units(uint8_t fru, uint8_t sensor_num, char *units);
int get_fru_sdr_path(uint8_t fru, char *path);

#ifdef __cplusplus
} // extern "C"
#endif

#endif /* __YOSEMITE_SENSOR_H__ */
