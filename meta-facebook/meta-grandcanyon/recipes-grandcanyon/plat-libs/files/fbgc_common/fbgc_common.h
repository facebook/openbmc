/*
 *
 * Copyright 2020-present Facebook. All Rights Reserved.
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

#ifndef __FBGC_COMMON_H__
#define __FBGC_COMMON_H__

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define EEPROM_PATH     "/sys/bus/i2c/devices/%d-00%X/eeprom"
#define COMMON_FRU_PATH "/tmp/fruid_%s.bin"
#define FRU_BMC_BIN     "/tmp/fruid_bmc.bin"
#define FRU_UIC_BIN     "/tmp/fruid_uic.bin"
#define FRU_NIC_BIN     "/tmp/fruid_nic.bin"


#define BMC_FRU_BUS  6
#define BMC_FRU_ADDR 0x54
#define UIC_FRU_BUS  4
#define UIC_FRU_ADDR 0x50
#define NIC_FRU_BUS  8
#define NIC_FRU_ADDR 0x50

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(a)   (sizeof(a) / sizeof((a)[0]))
#endif

enum {
  FRU_ALL = 0,
  FRU_SERVER,
  FRU_BMC,
  FRU_UIC,
  FRU_DPB,
  FRU_SCC,
  FRU_NIC,
  FRU_E1S_IOCM,
  FRU_CNT,
};

enum {
  CHASSIS_TYPE5 = 0,
  CHASSIS_TYPE7,
};

enum {
  I2C_BIC_BUS = 2,
  I2C_BS_FPGA_BUS = 3,
  I2C_UIC_FPGA_BUS = 5,
  I2C_DBG_CARD_BUS = 7,
  I2C_NIC_BUS = 8,
  I2C_EXP_BUS = 10,
  I2C_T5IOC_BUS = 11,
  I2C_T5E1S1_T7IOC_BUS = 12,  // T5: E1.S 1; T7: IOC
  I2C_T5E1S2_T7IOC_BUS = 13,  // T5: E1.S 2; T7: IOCM FRU, Voltage sensor, Temp sensor
};

int fbgc_common_get_chassis_type(uint8_t *type);
void msleep(int msec);

#ifdef __cplusplus
} // extern "C"
#endif

#endif /* __FBGC_COMMON_H__ */
