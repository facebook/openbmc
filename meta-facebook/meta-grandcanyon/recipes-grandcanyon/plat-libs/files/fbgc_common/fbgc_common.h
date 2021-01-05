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
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define EEPROM_PATH     "/sys/bus/i2c/devices/%d-00%X/eeprom"
#define COMMON_FRU_PATH "/tmp/fruid_%s.bin"
#define FRU_BMC_BIN     "/tmp/fruid_bmc.bin"
#define FRU_UIC_BIN     "/tmp/fruid_uic.bin"
#define FRU_NIC_BIN     "/tmp/fruid_nic.bin"
#define FRU_IOCM_BIN    "/tmp/fruid_iocm.bin"

#define BMC_FRU_ADDR  0x54
#define UIC_FRU_ADDR  0x50
#define NIC_FRU_ADDR  0x50
#define IOCM_FRU_ADDR 0x50

//UIC FPGA slave address (8-bit)
#define UIC_FPGA_SLAVE_ADDR 0x1e

// Expander slave address (7-bit)
#define EXPANDER_SLAVE_ADDR    0x71

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(a)   (sizeof(a) / sizeof((a)[0]))
#endif

#define MIN(a , b)      ((a) < (b) ? (a) : (b))

#define SERVER_SENSOR_LOCK "/var/run/sensor_read_server.lock"

#define MAX_PATH_LEN 128  // include the string terminal
#define E1S0_IOCM_PRESENT_BIT   (1 << 0)
#define E1S1_IOCM_PRESENT_BIT   (1 << 1)

#define I2C_BASE           0x1e78a000
#define I2C_BASE_INTERVAL  0x80
#define I2C_BASE_MAP(bus)  (I2C_BASE + (((bus) + 1) * I2C_BASE_INTERVAL))

#define CHASSIS_TYPE_BIT_0(value)   (value << 0)
#define CHASSIS_TYPE_BIT_1(value)   (value << 1)
#define CHASSIS_TYPE_BIT_2(value)   (value << 2)
#define CHASSIS_TYPE_BIT_3(value)   (value << 3)

#define WAIT_POWER_STATUS_CHANGE_TIME 30 // second

//                 UIC_LOC_TYPE_IN   UIC_RMT_TYPE_IN   SCC_LOC_TYPE_0   SCC_RMT_TYPE_0
// Type 5                        0                 0                0                0
// Type 7 Headnode               0                 1                0                1

#define CHASSIS_TYPE_5_VALUE (CHASSIS_TYPE_BIT_3(0) | CHASSIS_TYPE_BIT_2(0) | CHASSIS_TYPE_BIT_1(0) | CHASSIS_TYPE_BIT_0(0)) // 0000b
#define CHASSIS_TYPE_7_VALUE (CHASSIS_TYPE_BIT_3(0) | CHASSIS_TYPE_BIT_2(1) | CHASSIS_TYPE_BIT_1(0) | CHASSIS_TYPE_BIT_0(1)) // 0101b

enum {
  FRU_ALL = 0,
  FRU_SERVER,
  FRU_BMC,
  FRU_UIC,
  FRU_DPB,
  FRU_SCC,
  FRU_NIC,
  FRU_E1S_IOCM,
  FRU_FAN0,
  FRU_FAN1,
  FRU_FAN2,
  FRU_FAN3,
  FRU_CNT,
};

// AC Power status
enum {
  STAT_AC_OFF = 0,
  STAT_AC_ON = 1,
};

// DC power status
enum {
  STAT_DC_OFF = 0,
  STAT_DC_ON = 1,
};

enum {
  CHASSIS_TYPE5 = 0,
  CHASSIS_TYPE7,
};

enum {
  I2C_SYS_HSC_BUS = 1,
  I2C_BIC_BUS = 2,
  I2C_BS_FPGA_BUS = 3,
  I2C_UIC_BUS = 4,
  I2C_UIC_FPGA_BUS = 5,
  I2C_BSM_BUS = 6,
  I2C_DBG_CARD_BUS = 7,
  I2C_NIC_BUS = 8,
  I2C_IOEXP_BUS = 9,
  I2C_EXP_BUS = 10,
  I2C_T5IOC_BUS = 11,
  I2C_T5E1S0_T7IOC_BUS = 12,  // T5: E1.S 1; T7: IOC
  I2C_T5E1S1_T7IOC_BUS = 13,  // T5: E1.S 2; T7: IOCM FRU, Voltage sensor, Temp sensor
  I2C_SCC_BUS = 14,
  I2C_TPM_BUS = 15,  // Reserved

};

// IPMB payload ID
enum {
  PAYLOAD_BIC = 1,
  PAYLOAD_DBG_CARD = 2,
  PAYLOAD_EXP = 3,
};

// server 12v power status
enum {
  STAT_12V_OFF = 0,
  STAT_12V_ON = 1,
};

int fbgc_common_get_chassis_type(uint8_t *type);
void msleep(int msec);
int fbgc_common_server_stby_pwr_sts(uint8_t *val);
uint8_t cal_crc8(uint8_t crc, uint8_t const *data, uint8_t len);
uint8_t hex_c2i(const char c);
int string_2_byte(const char* c);
bool start_with(const char *s, const char *p);
int split(char **dst, char *src, char *delim, int max_size);

#ifdef __cplusplus
} // extern "C"
#endif

#endif /* __FBGC_COMMON_H__ */
