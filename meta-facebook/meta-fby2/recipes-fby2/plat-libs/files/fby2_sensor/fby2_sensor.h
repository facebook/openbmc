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

#ifndef __FBY2_SENSOR_H__
#define __FBY2_SENSOR_H__

#include <stdbool.h>
#include <openbmc/ipmi.h>
#include <openbmc/ipmb.h>
#include <openbmc/obmc-pal.h>
#include <openbmc/obmc_pal_sensors.h>
#include <facebook/bic.h>
#include <facebook/fby2_common.h>

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
#define EER_UNHANDLED         -4

#define PWM_UNIT_MAX 96

#define SYS_CONFIG_PATH "/mnt/data/kv_store/sys_config/"

#define MAX_NUM_DEVS 12

enum {
  DEV_TYPE_UNKNOWN,
  DEV_TYPE_SSD,
  DEV_TYPE_VSI_ACC,
  DEV_TYPE_BRCM_ACC,
  DEV_TYPE_OTHER_ACC,
  DEV_TYPE_DUAL_M2,
  DEV_TYPE_SPH_ACC,
};

enum {
  VENDOR_SAMSUNG = 0x144D,
  VENDOR_VSI = 0x1D9B,
  VENDOR_BRCM = 0x14E4,
  VENDOR_SPH = 0x8086,
};

enum {
  DEVICE_ID_RP = 0xFACE,
  DEVICE_ID_VK = 0x5E87,
  DEVICE_ID_PM983 = 0xA808,
};

typedef struct {
  const char *dimm_location_file;
  uint8_t dimm_sensor_num;
} rc_dimm_location_info;

typedef struct {
  bool is_accuracy;
  uint8_t int_value;
  uint8_t dec_value;
  uint8_t flags;
  uint8_t status;
  uint8_t ext_status;
} ipmi_general_sensor_reading_t;

// Sensors under Bridge IC
enum {
  BIC_SENSOR_MB_INLET_TEMP = 0x01,
  BIC_SENSOR_VCCIN_VR_TEMP = 0x02,
  BIC_SENSOR_VCCSA_VR_TEMP = 0x03,
  BIC_SENSOR_1V05_PCH_VR_TEMP = 0x04,
  BIC_SENSOR_SOC_TEMP = 0x05,
  BIC_SENSOR_VNN_PCH_VR_TEMP = 0x06,
  BIC_SENSOR_MB_OUTLET_TEMP = 0x07,
  BIC_SENSOR_PCH_TEMP = 0x08,
  BIC_SENSOR_SOC_THERM_MARGIN = 0x09,
  BIC_SENSOR_VDDR_DE_VR_TEMP = 0x0A,
  BIC_SENSOR_VDDR_AB_VR_TEMP = 0x0B,
  BIC_SENSOR_MB_OUTLET_TEMP_BOTTOM = 0x0D,
  BIC_SENSOR_NVME1_CTEMP = 0x0E,
  BIC_SENSOR_NVME2_CTEMP = 0x0F,
  BIC_SENSOR_SYSTEM_STATUS = 0x10, //Discrete
  BIC_SENSOR_SPS_FW_HLTH = 0x17, //Event-only
  BIC_SENSOR_VCCIO_VR_CURR = 0x20,
  BIC_SENSOR_VDDR_AB_VR_CURR = 0x21,
  BIC_SENSOR_VCCIN_VR_POUT = 0x22,
  BIC_SENSOR_VCCIN_VR_CURR = 0x23,
  BIC_SENSOR_VCCIN_VR_VOL = 0x24,
  BIC_SENSOR_VCCSA_VR_CURR = 0x26,
  BIC_SENSOR_VNN_PCH_VR_CURR = 0x27,
  BIC_SENSOR_INA230_POWER = 0x29,
  BIC_SENSOR_INA230_VOL = 0x2A,
  BIC_SENSOR_POST_ERR = 0x2B, //Event-only
  BIC_SENSOR_SOC_PACKAGE_PWR = 0x2C,
  BIC_SENSOR_SOC_TJMAX = 0x30,
  BIC_SENSOR_VDDR_AB_VR_POUT = 0x32,
  BIC_SENSOR_VDDR_DE_VR_CURR = 0x33,
  BIC_SENSOR_VDDR_DE_VR_VOL = 0x34,
  BIC_SENSOR_1V05_PCH_VR_CURR = 0x35,
  BIC_SENSOR_1V05_PCH_VR_VOL = 0x36,
  BIC_SENSOR_VCCIO_VR_TEMP = 0x37,
  BIC_SENSOR_VCCSA_VR_POUT = 0x38,
  BIC_SENSOR_VCCIO_VR_POUT = 0x39,
  BIC_SENSOR_VDDR_DE_VR_POUT = 0x3A,
  BIC_SENSOR_POWER_THRESH_EVENT = 0x3B, //Event-only
  BIC_SENSOR_VNN_PCH_VR_POUT = 0x3C,
  BIC_SENSOR_MACHINE_CHK_ERR = 0x40, //Event-only
  BIC_SENSOR_PCIE_ERR = 0x41, //Event-only
  BIC_SENSOR_1V05_PCH_VR_POUT = 0x42,
  BIC_SENSOR_OTHER_IIO_ERR = 0x43, //Event-only
  BIC_SENSOR_VCCSA_VR_VOL = 0x4B,
  BIC_SENSOR_VNN_PCH_VR_VOUT = 0x4C,
  BIC_SENSOR_PROC_HOT_EXT = 0x51, //Event-only
  BIC_SENSOR_VCCIO_VR_VOL = 0x54,
  BIC_SENSOR_VDDR_AB_VR_VOL = 0x55,
  BIC_SENSOR_POWER_ERR = 0x56, //Event-only
  BIC_SENSOR_MEM_ECC_ERR = 0x63, //Event-only
  BIC_SENSOR_PROC_FAIL = 0x65, //Discrete
  BIC_SENSOR_SYS_BOOT_STAT = 0x7E, //Discrete
  HOST_BOOT_DRIVE_TEMP = 0xA0,
  BIC_SENSOR_VR_HOT = 0xB2, //Discrete
  BIC_SENSOR_CPU_DIMM_HOT = 0xB3, //Discrete
  BIC_SENSOR_SOC_DIMMA0_TEMP = 0xB4,
  BIC_SENSOR_SOC_DIMMA1_TEMP = 0xB5,
  BIC_SENSOR_SOC_DIMMB0_TEMP = 0xB6,
  BIC_SENSOR_SOC_DIMMB1_TEMP = 0xB7,
  BIC_SENSOR_SOC_DIMMD0_TEMP = 0xB8,
  BIC_SENSOR_SOC_DIMMD1_TEMP = 0xB9,
  BIC_SENSOR_SOC_DIMME0_TEMP = 0xBA,
  BIC_SENSOR_SOC_DIMME1_TEMP = 0xBB,
  BIC_SENSOR_P3V3_MB = 0xD0,
  BIC_SENSOR_P12V_MB = 0xD2,
  BIC_SENSOR_P1V05_PCH = 0xD3,
  BIC_SENSOR_P3V3_STBY_MB = 0xD5,
  BIC_SENSOR_PVDDR_DE = 0xD6,
  BIC_SENSOR_PV_BAT = 0xD7,
  BIC_SENSOR_PVDDR_AB = 0xD8,
  BIC_SENSOR_PVNN_PCH = 0xD9,
  BIC_SENSOR_CAT_ERR = 0xEB, //Event-only
};

// Sensors under Bridge IC (RC)
enum {
  BIC_RC_SENSOR_MB_OUTLET_TEMP = 0x01,
  BIC_RC_SENSOR_MB_INLET_TEMP = 0x02,
  BIC_RC_SENSOR_SENSOR_TEMP = 0x03,
  BIC_RC_SENSOR_SOC_TEMP_DIODE = 0x04,
  BIC_RC_SENSOR_NVME1_CTEMP = 0x05,
  BIC_RC_SENSOR_P12V_MB = 0x0A,
  BIC_RC_SENSOR_P3V3_STBY_MB = 0x0B,
  BIC_RC_SENSOR_P3V2_MB = 0x0C,
  BIC_RC_SENSOR_PV_BAT = 0x0D,
  BIC_RC_SENSOR_PVDDQ_423 = 0x0E,
  BIC_RC_SENSOR_PVDDQ_510 = 0x0F,
  BIC_RC_SENSOR_SOC_TEMP_IMC = 0x14,
  BIC_RC_SENSOR_PMF2432_TEMP = 0x15,
  BIC_RC_SENSOR_PMF2344_TEMP = 0x16,
  BIC_RC_SENSOR_CVR_APC_TEMP = 0x17,
  BIC_RC_SENSOR_CVR_CBF_TEMP = 0x18,
  BIC_RC_SENSOR_SOC_DIMMB_TEMP = 0x19,
  BIC_RC_SENSOR_SOC_DIMMA_TEMP = 0x1A,
  BIC_RC_SENSOR_SOC_DIMMC_TEMP = 0x1B,
  BIC_RC_SENSOR_SOC_DIMMD_TEMP = 0x1C,
  BIC_RC_SENSOR_SOC_PWR = 0x1D,
  BIC_RC_SENSOR_PVDDQ_423_VR_TEMP = 0x28,
  BIC_RC_SENSOR_PVDDQ_510_VR_TEMP = 0x29,
  BIC_RC_SENSOR_PVDDQ_423_VR_VOL = 0x2A,
  BIC_RC_SENSOR_PVDDQ_510_VR_VOL = 0x2B,
  BIC_RC_SENSOR_CVR_APC_VOL = 0x2C,
  BIC_RC_SENSOR_CVR_CBF_VOL = 0x2D,
  BIC_RC_SENSOR_PVDDQ_423_VR_CURR = 0x2E,
  BIC_RC_SENSOR_PVDDQ_510_VR_CURR = 0x2F,
  BIC_RC_SENSOR_CVR_APC_CURR = 0x30,
  BIC_RC_SENSOR_CVR_CBF_CURR = 0x31,
  BIC_RC_SENSOR_PVDDQ_423_VR_POUT = 0x32,
  BIC_RC_SENSOR_PVDDQ_510_VR_POUT = 0x33,
  BIC_RC_SENSOR_CVR_APC_POUT = 0x34,
  BIC_RC_SENSOR_CVR_CBF_POUT = 0x35,
  BIC_RC_SENSOR_INA230_VOL = 0x36,
  BIC_RC_SENSOR_INA230_POWER = 0x37,
  BIC_RC_SENSOR_SYSTEM_STATUS = 0x10, // Discrete
  BIC_RC_SENSOR_VR_HOT = 0xB2,  // Discrete
  BIC_RC_SENSOR_SYS_BOOTING_STS = 0x7E,  // Discrete
  BIC_RC_SENSOR_PROC_FAIL = 0x65, //Discrete
  BIC_RC_SENSOR_THROTTLE_STATUS = 0x11, //Event-only
  BIC_RC_SENSOR_RAS_CRIT = 0xCB,    //Event-only
  BIC_RC_SENSOR_RAS_INFO = 0xCC,    //Event-only
  BIC_RC_SENSOR_RAS_FATAL = 0xCD,   //Event-only
  BIC_RC_SENSOR_POWER_ERR = 0x56,   //Event-only
};

// Sensors under Bridge IC (EP)
enum {
  BIC_EP_SENSOR_MB_INLET_TEMP = 0x01,
  BIC_EP_SENSOR_VDD_CORE_VR_TEMP = 0x02,
  BIC_EP_SENSOR_VDD_MEM_VR_TEMP = 0x04,
  BIC_EP_SENSOR_SOC_TEMP = 0x05,
  BIC_EP_SENSOR_MB_OUTLET_TEMP = 0x07,
  BIC_EP_SENSOR_VDD_SOC_VR_TEMP = 0x0B,
  BIC_EP_SENSOR_MB_OUTLET_TEMP_BOTTOM = 0x0D,
  BIC_EP_SENSOR_NVME1_CTEMP = 0x0E,
  BIC_EP_SENSOR_NVME2_CTEMP = 0x0F,
  BIC_EP_SENSOR_SYSTEM_STATUS = 0x10, //Discrete
  BIC_EP_SENSOR_VDD_CORE_VR_CURR = 0x20,
  BIC_EP_SENSOR_VDD_MEM_VR_CURR = 0x21,
  BIC_EP_SENSOR_VDD_CORE_VR_POUT = 0x22,
  BIC_EP_SENSOR_VDD_CORE_VR_VOL = 0x24,
  BIC_EP_SENSOR_INA230_POWER = 0x29,
  BIC_EP_SENSOR_INA230_VOL = 0x2A,
  BIC_EP_SENSOR_SOC_PACKAGE_PWR = 0x2C,
  BIC_EP_SENSOR_VDD_SRAM_VR_POUT = 0x32,
  BIC_EP_SENSOR_VDD_SOC_VR_CURR = 0x33,
  BIC_EP_SENSOR_VDD_MEM_VR_VOL = 0x34,
  BIC_EP_SENSOR_VDD_SRAM_VR_CURR = 0x35,
  BIC_EP_SENSOR_VDD_SRAM_VR_TEMP = 0x37,
  BIC_EP_SENSOR_VDD_MEM_VR_POUT = 0x3A,
  BIC_EP_SENSOR_VDD_SOC_VR_POUT = 0x42,
  BIC_EP_SENSOR_VDD_SOC_VR_VOL = 0x54,
  BIC_EP_SENSOR_VDD_SRAM_VR_VOL = 0x55,
  BIC_EP_SENSOR_POWER_ERR = 0x56, //Event-only
  BIC_EP_SENSOR_PROC_FAIL = 0x65, //Discrete
  BIC_EP_SENSOR_VR_HOT = 0xB2, //Discrete
  BIC_EP_SENSOR_CPU_DIMM_HOT = 0xB3, //Discrete
  BIC_EP_SENSOR_SOC_DIMMA_TEMP = 0xB4,
  BIC_EP_SENSOR_SOC_DIMMB_TEMP = 0xB5,
  BIC_EP_SENSOR_SOC_DIMMC_TEMP = 0xB6,
  BIC_EP_SENSOR_SOC_DIMMD_TEMP = 0xB7,
  BIC_EP_SENSOR_P3V3_MB = 0xD0,
  BIC_EP_SENSOR_P12V_STBY_MB = 0xD2,
  BIC_EP_SENSOR_P1V8_VDD = 0xD3,
  BIC_EP_SENSOR_P3V3_STBY_MB = 0xD5,
  BIC_EP_SENSOR_PVDDQ_CD = 0xD6,
  BIC_EP_SENSOR_PV_BAT = 0xD7,
  BIC_EP_SENSOR_PVDDQ_AB = 0xD8,
  BIC_EP_SENSOR_P0V8_VDD = 0xD9,
};

// logged from FBEP system firmware
enum {
  NBU_ERROR = 0x9E,
};

// Sensors Under Side Plane
enum {
  SP_SENSOR_INLET_TEMP = 0x81,
  SP_SENSOR_OUTLET_TEMP = 0x80,
  SP_SENSOR_MEZZ_TEMP = 0x82,
  SP_SENSOR_FAN0_TACH = 0x46,
  SP_SENSOR_FAN1_TACH = 0x47,
  SP_SENSOR_FAN2_TACH = 0x48,
  SP_SENSOR_FAN3_TACH = 0x49,
  SP_SENSOR_AIR_FLOW = 0x4A,
  SP_SENSOR_P5V = 0xE0,
  SP_SENSOR_P12V = 0xE1,
  SP_SENSOR_P3V3_STBY = 0xE2,
  SP_SENSOR_P12V_SLOT1 = 0xE3,
  SP_SENSOR_P12V_SLOT2 = 0xE4,
  SP_SENSOR_P12V_SLOT3 = 0xE5,
  SP_SENSOR_P12V_SLOT4 = 0xE6,
  SP_SENSOR_P3V3 = 0xE7,
  SP_SENSOR_P1V15_BMC_STBY = 0xC8,
  SP_SENSOR_P1V2_BMC_STBY = 0xC9,
  SP_SENSOR_P2V5_BMC_STBY = 0xCA,
  SP_P1V8_STBY = 0xCB,
  SP_SENSOR_HSC_IN_VOLT = 0xC0,
  SP_SENSOR_HSC_OUT_CURR = 0xC1,
  SP_SENSOR_HSC_TEMP = 0xC2,
  SP_SENSOR_HSC_IN_POWER = 0xC3,
  SP_SENSOR_HSC_IN_POWERAVG = 0xC7,
  SP_SENSOR_HSC_PEAK_IOUT = 0xC4,
  SP_SENSOR_HSC_PEAK_PIN = 0xC5,
  SP_SENSOR_BMC_HSC_PIN = 0xC6,
  SP_SENSOR_FAN0_PWM = 0xD0,
  SP_SENSOR_FAN1_PWM = 0xD1,
  SP_SENSOR_FAN2_PWM = 0xD2,
  SP_SENSOR_FAN3_PWM = 0xD3,

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

enum{
  MEZZ_SENSOR_TEMP = 0x82,
  // PLDM numeric sensors
  NIC_SOC_TEMP = PLDM_NUMERIC_SENSOR_START,
  PORT_0_TEMP,
  PORT_0_LINK_SPEED,

  // PLDM state sensors
  NIC_HEALTH_STATE = PLDM_STATE_SENSOR_START,
  PORT_0_LINK_STATE,
};

enum {
  GPV2_SENSOR_INLET_TEMP = 0x01,
  GPV2_SENSOR_OUTLET_TEMP = 0x07,
  GPV2_SENSOR_P12V_BIC_SCALED = 0xD2,
  GPV2_SENSOR_P3V3_STBY_BIC_SCALED = 0xD5,
  GPV2_SENSOR_P0V92_BIC_SCALED = 0xDA,
  GPV2_SENSOR_P1V8_BIC_SCALED = 0xD9,
  GPV2_SENSOR_INA230_POWER = 0x29,
  GPV2_SENSOR_INA230_VOLT = 0x2A,
  GPV2_SENSOR_PCIE_SW_TEMP = 0x2B,
  // VR
  GPV2_SENSOR_3V3_VR_Vol = 0x27,
  GPV2_SENSOR_0V92_VR_Vol = 0x28,
  GPV2_SENSOR_3V3_VR_Curr = 0x2C,
  GPV2_SENSOR_0V92_VR_Curr = 0x2D,
  GPV2_SENSOR_3V3_VR_Pwr = 0x2E,
  GPV2_SENSOR_0V92_VR_Pwr = 0x2F,
  GPV2_SENSOR_3V3_VR_Temp = 0x25,
  GPV2_SENSOR_0V92_VR_Temp = 0x26,
  //M.2 0
  GPV2_SENSOR_DEV0_INA231_PW = 0x30,
  GPV2_SENSOR_DEV0_INA231_VOL = 0x31,
  GPV2_SENSOR_DEV0_Temp = 0x36,
  GPV2_SENSOR_DEV0_Ambient_Temp = 0x37,
  //M.2 1
  GPV2_SENSOR_DEV1_INA231_PW = 0x40,
  GPV2_SENSOR_DEV1_INA231_VOL = 0x41,
  GPV2_SENSOR_DEV1_Temp = 0x46,
  GPV2_SENSOR_DEV1_Ambient_Temp = 0x47,
  //M.2 2
  GPV2_SENSOR_DEV2_INA231_PW = 0x50,
  GPV2_SENSOR_DEV2_INA231_VOL = 0x51,
  GPV2_SENSOR_DEV2_Temp = 0x56,
  GPV2_SENSOR_DEV2_Ambient_Temp = 0x57,
  //M.2 3
  GPV2_SENSOR_DEV3_INA231_PW = 0x60,
  GPV2_SENSOR_DEV3_INA231_VOL = 0x61,
  GPV2_SENSOR_DEV3_Temp = 0x66,
  GPV2_SENSOR_DEV3_Ambient_Temp = 0x67,
  //M.2 4
  GPV2_SENSOR_DEV4_INA231_PW = 0x70,
  GPV2_SENSOR_DEV4_INA231_VOL = 0x71,
  GPV2_SENSOR_DEV4_Temp = 0x76,
  GPV2_SENSOR_DEV4_Ambient_Temp = 0x77,
  //M.2 5
  GPV2_SENSOR_DEV5_INA231_PW = 0x80,
  GPV2_SENSOR_DEV5_INA231_VOL = 0x81,
  GPV2_SENSOR_DEV5_Temp = 0x86,
  GPV2_SENSOR_DEV5_Ambient_Temp = 0x87,
  //M.2 6
  GPV2_SENSOR_DEV6_INA231_PW = 0x90,
  GPV2_SENSOR_DEV6_INA231_VOL = 0x91,
  GPV2_SENSOR_DEV6_Temp = 0x96,
  GPV2_SENSOR_DEV6_Ambient_Temp = 0x97,
  //M.2 7
  GPV2_SENSOR_DEV7_INA231_PW = 0xA0,
  GPV2_SENSOR_DEV7_INA231_VOL = 0xA1,
  GPV2_SENSOR_DEV7_Temp = 0xA6,
  GPV2_SENSOR_DEV7_Ambient_Temp = 0xA7,
  //M.2 8
  GPV2_SENSOR_DEV8_INA231_PW = 0xB0,
  GPV2_SENSOR_DEV8_INA231_VOL = 0xB1,
  GPV2_SENSOR_DEV8_Temp = 0xB6,
  GPV2_SENSOR_DEV8_Ambient_Temp = 0xB7,
  //M.2 9
  GPV2_SENSOR_DEV9_INA231_PW = 0xC0,
  GPV2_SENSOR_DEV9_INA231_VOL = 0xC1,
  GPV2_SENSOR_DEV9_Temp = 0xC6,
  GPV2_SENSOR_DEV9_Ambient_Temp = 0xC7,
  //M.2 10
  GPV2_SENSOR_DEV10_INA231_PW = 0xE0,
  GPV2_SENSOR_DEV10_INA231_VOL = 0xE1,
  GPV2_SENSOR_DEV10_Temp = 0xE6,
  GPV2_SENSOR_DEV10_Ambient_Temp = 0xE7,
  //M.2 11
  GPV2_SENSOR_DEV11_INA231_PW = 0xF0,
  GPV2_SENSOR_DEV11_INA231_VOL = 0xF1,
  GPV2_SENSOR_DEV11_Temp = 0xF6,
  GPV2_SENSOR_DEV11_Ambient_Temp = 0xF7,
};

enum {
  GPV2_SENSOR_DEV_INA231_PW = 0,
  GPV2_SENSOR_DEV_INA231_VOL = 1,
  GPV2_SENSOR_DEV_Temp = 6,
  GPV2_SENSOR_DEV_Ambient_Temp = 7,
};

// Sensors under Bridge IC (NORTHDOME)
enum {
  BIC_ND_SENSOR_MB_INLET_TEMP = 0x01,
  BIC_ND_SENSOR_PVDDCR_CPU_VR_T = 0x02,
  BIC_ND_SENSOR_PVDDCR_SOC_VR_T = 0x03,
  BIC_ND_SENSOR_SOC_TEMP = 0x05,
  BIC_ND_SENSOR_MB_OUTLET_TEMP_T = 0x07,
  BIC_ND_SENSOR_PVDDIO_EFGH_VR_T = 0x0A,
  BIC_ND_SENSOR_PVDDIO_ABCD_VR_T = 0x0B,
  BIC_ND_SENSOR_MB_OUTLET_TEMP_B = 0x0D,
  BIC_ND_SENSOR_NVME1_CTEMP = 0x0E,
  BIC_ND_SENSOR_SYSTEM_STATUS = 0x10, //Discrete
  BIC_ND_SENSOR_PVDDCR_CPU_VR_I = 0x20,
  BIC_ND_SENSOR_PVDDIO_ABCD_VR_I = 0x21,
  BIC_ND_SENSOR_PVDDCR_CPU_VR_P = 0x22,
  BIC_ND_SENSOR_PVDDCR_SOC_VR_I = 0x23,
  BIC_ND_SENSOR_PVDDCR_CPU_VR_V = 0x24,
  BIC_ND_SENSOR_INA260_POWER = 0x28,
  BIC_ND_SENSOR_INA230_POWER = 0x29,
  BIC_ND_SENSOR_INA230_VOLTAGE = 0x2A,
  BIC_ND_SENSOR_POST_ERR = 0x2B, //Event-only
  BIC_ND_SENSOR_SOC_Package_Pwr = 0x2C,
  BIC_ND_SENSOR_INA260_VOLTAGE = 0x2E,
  BIC_ND_SENSOR_PVDDIO_ABCD_VR_P = 0x32,
  BIC_ND_SENSOR_PVDDIO_EFGH_VR_I = 0x33,
  BIC_ND_SENSOR_PVDDIO_EFGH_VR_V = 0x34,
  BIC_ND_SENSOR_PVDDCR_SOC_VR_P = 0x39,
  BIC_ND_SENSOR_PVDDIO_EFGH_VR_P = 0x3A,
  BIC_ND_SENSOR_PWR_THRESH_EVT = 0x3B, //Event-only
  BIC_ND_SENSOR_MACHINE_CHK_ERR = 0x40, //Event-only
  BIC_ND_SENSOR_PCIE_ERR = 0x41, //Event-only
  BIC_ND_SENSOR_OTHER_IIO_ERR = 0x43, //Event-only
  BIC_ND_SENSOR_PROCHOT_EXT = 0x51, //Event-only
  BIC_ND_SENSOR_PVDDCR_SOC_VR_V = 0x54,
  BIC_ND_SENSOR_PVDDIO_ABCD_VR_V = 0x55,
  BIC_ND_SENSOR_POWER_ERR = 0x56, //Event-only
  BIC_ND_SENSOR_MEMORY_ECC_ERROR = 0x63, //Event-only
  BIC_ND_SENSOR_PROCESSOR_FAIL = 0x65, //Discrete
  BIC_ND_SENSOR_VR_HOT = 0xB2, //Discrete
  BIC_ND_SENSOR_CPU_DIMM_HOT = 0xB3, //Discrete
  BIC_ND_SENSOR_SOC_DIMMA0_TEMP = 0xB4,
  BIC_ND_SENSOR_SOC_DIMMC0_TEMP = 0xB5,
  BIC_ND_SENSOR_SOC_DIMMD0_TEMP = 0xB6,
  BIC_ND_SENSOR_SOC_DIMME0_TEMP = 0xB7,
  BIC_ND_SENSOR_SOC_DIMMG0_TEMP = 0xB8,
  BIC_ND_SENSOR_SOC_DIMMH0_TEMP = 0xB9,
  BIC_ND_SENSOR_VR_ALERT = 0xBC, //Discrete
  BIC_ND_SENSOR_IHDT_PRSNT_ALERT = 0xBD,
  BIC_ND_SENSOR_P3V3_MB = 0xD0,
  BIC_ND_SENSOR_P12V_STBY_MB = 0xD2,
  BIC_ND_SENSOR_P1V8_BIC = 0xD3,
  BIC_ND_SENSOR_P3V3_STBY_MB = 0xD5,
  BIC_ND_SENSOR_PVPP_EFGH = 0xD6,
  BIC_ND_SENSOR_PV_BAT = 0xD7,
  BIC_ND_SENSOR_PVPP_ABCD = 0xD8,
  BIC_ND_SENSOR_P1V8_STBY_BIC = 0xD9,
  ND_HOST_BOOT_DRIVE_TEMP = HOST_BOOT_DRIVE_TEMP,
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

extern const uint8_t bic_rc_sensor_list[];
extern const uint8_t bic_rc_discrete_list[];

#ifdef CONFIG_FBY2_EP
  extern const uint8_t bic_ep_sensor_list[];
  extern const uint8_t bic_ep_discrete_list[];
#endif

extern const uint8_t dc_sensor_list[];

extern const uint8_t dc_cf_sensor_list[];

extern const uint8_t spb_sensor_list[];

extern const uint8_t spb_sensor_dual_r_fan_list[];

extern const uint8_t nic_sensor_list[];

#ifdef CONFIG_FBY2_GPV2
extern const uint8_t nic_yv250_sensor_list[];
extern const uint8_t gpv2_sensor_list[];
#endif

#ifdef CONFIG_FBY2_ND
extern const uint8_t bic_nd_sensor_list[];
extern const uint8_t bic_nd_discrete_list[];
#endif

//extern float spb_sensor_threshold[MAX_SENSOR_NUM][MAX_SENSOR_THRESHOLD + 1];

extern float nic_sensor_threshold[MAX_SENSOR_NUM][MAX_SENSOR_THRESHOLD + 1];

extern size_t bic_sensor_cnt;

extern size_t dc_sensor_cnt;

extern size_t bic_rc_sensor_cnt;
extern size_t bic_rc_discrete_cnt;

#ifdef CONFIG_FBY2_EP
  extern size_t bic_ep_sensor_cnt;
#endif

extern size_t bic_discrete_cnt;

extern size_t spb_sensor_cnt;

extern size_t spb_single_r_fan_sensor_cnt;

extern size_t spb_dual_r_fan_sensor_cnt;

extern size_t nic_sensor_cnt;

extern size_t dc_cf_sensor_cnt;

#ifdef CONFIG_FBY2_GPV2
extern size_t nic_yv250_sensor_cnt;
extern size_t gpv2_sensor_cnt;
#endif

#ifdef CONFIG_FBY2_ND
extern size_t bic_nd_sensor_cnt;
#endif

int read_pwm_value(uint8_t fan_num, uint8_t *value);
int fby2_sensor_read(uint8_t fru, uint8_t sensor_num, void *value);
int fby2_sensor_name(uint8_t fru, uint8_t sensor_num, char *name);
int fby2_sensor_units(uint8_t fru, uint8_t sensor_num, char *units);
int fby2_sensor_sdr_path(uint8_t fru, char *path);
int fby2_sensor_threshold(uint8_t fru, uint8_t sensor_num, uint8_t thresh, float *value);
int fby2_sensor_sdr_init(uint8_t fru, sensor_info_t *sinfo);
int fby2_get_slot_type(uint8_t fru);
int fby2_get_record_slot_type(uint8_t fru);
int fby2_set_slot_type(uint8_t fru,uint8_t type);
int fby2_get_server_type(uint8_t fru, uint8_t *type);
int fby2_get_server_type_directly(uint8_t fru, uint8_t *type);
int fby2_mux_control(char *device, uint8_t addr, uint8_t channel);
int fby2_disable_gp_m2_monior(uint8_t slot_id, uint8_t dis);
int fby2_check_hsc_sts_iout(uint8_t mask);
int fby2_check_hsc_fault(void);
int fby2_get_m2_info_from_bios(uint8_t slot_id, uint8_t dev_index, bool *present, uint16_t *vendor_id, uint16_t * device_id, uint8_t* link_speed, uint8_t* link_width);
int fby2_get_m2_type_from_bios(uint8_t slot_id, uint8_t dev_index, uint8_t *type);
int fby2_get_slot_dev_type(uint8_t slot_id, uint8_t *dev_type);
int fby2_sdr_init(uint8_t fru, bool force);
int fby2_sensor_poll_interval(uint8_t fru, uint8_t sensor_num, uint32_t *value);

#ifdef __cplusplus
} // extern "C"
#endif

#endif /* __FBY2_SENSOR_H__ */
