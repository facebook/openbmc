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

#define SYS_CONFIG_PATH "/mnt/data/kv_store/sys_config/"

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

// Sensors Under Side Plane
enum {
  SP_SENSOR_INLET_TEMP = 0x81,
  SP_SENSOR_OUTLET_TEMP = 0x80,
  SP_SENSOR_MEZZ_TEMP = 0x82,
  SP_SENSOR_FAN0_TACH = 0x46,
  SP_SENSOR_FAN1_TACH = 0x47,
  SP_SENSOR_FAN2_TACH = 0x48,
  SP_SENSOR_FAN3_TACH = 0x49,
  SP_SENSOR_FAN4_TACH = 0x4A,
  SP_SENSOR_FAN5_TACH = 0x4B,
  SP_SENSOR_FAN6_TACH = 0x4C,
  SP_SENSOR_FAN7_TACH = 0x4D,
  SP_SENSOR_AIR_FLOW = 0x4E,
  SP_SENSOR_P5V = 0xE0,
  SP_SENSOR_P12V = 0xE1,
  SP_SENSOR_P3V3_STBY = 0xE2,
  SP_SENSOR_P1V15_BMC_STBY = 0xC8,
  SP_SENSOR_P1V2_BMC_STBY = 0xC9,
  SP_SENSOR_P2V5_BMC_STBY = 0xCA,
  SP_SENSOR_P12V_MEDUSA = 0xCB,
  SP_SENSOR_IMON_VTEMP = 0xCC,
  SP_SENSOR_HSC_IN_VOLT = 0xC0,
  SP_SENSOR_HSC_OUT_CURR = 0xC1,
  SP_SENSOR_HSC_TEMP = 0xC2,
  SP_SENSOR_HSC_IN_POWER = 0xC3,
  SP_SENSOR_HSC_PEAK_IOUT = 0xC4,
  SP_SENSOR_HSC_PEAK_PIN = 0xC5,
};

enum{
  MEZZ_SENSOR_TEMP = 0x82,
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
extern const uint8_t spb_sensor_list[];
extern const uint8_t nic_sensor_list[];

extern size_t bic_sensor_cnt;
extern size_t bic_discrete_cnt;
extern size_t spb_sensor_cnt;
extern size_t nic_sensor_cnt;

extern float nic_sensor_threshold[MAX_SENSOR_NUM][MAX_SENSOR_THRESHOLD + 1];

int fby2_sensor_read(uint8_t fru, uint8_t sensor_num, void *value);
int fby2_sensor_name(uint8_t fru, uint8_t sensor_num, char *name);
int fby2_sensor_units(uint8_t fru, uint8_t sensor_num, char *units);
int fby2_sensor_sdr_path(uint8_t fru, char *path);
int fby2_sensor_threshold(uint8_t fru, uint8_t sensor_num, uint8_t thresh, float *value);
int fby2_sensor_sdr_init(uint8_t fru, sensor_info_t *sinfo);
int fby2_get_slot_type(uint8_t fru);
int fby2_get_server_type(uint8_t fru, uint8_t *type);
int fby2_get_server_type_directly(uint8_t fru, uint8_t *type);
int fby2_mux_control(char *device, uint8_t addr, uint8_t channel);
int fby2_disable_gp_m2_monior(uint8_t slot_id, uint8_t dis);
int fby2_check_hsc_sts_iout(uint8_t mask);
int fby2_check_hsc_fault(void);

#ifdef __cplusplus
} // extern "C"
#endif

#endif /* __FBY2_SENSOR_H__ */
