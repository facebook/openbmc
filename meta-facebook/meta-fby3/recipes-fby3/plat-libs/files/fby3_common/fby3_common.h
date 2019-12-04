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

#ifndef __FBY3_COMMON_H__
#define __FBY3_COMMON_H__

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(a)   (sizeof(a) / sizeof((a)[0]))
#endif

#define BIC_CACHED_PID "/var/run/bic-cached_%d.lock"

#define FRU_BMC_BIN   "/tmp/fruid_bmc.bin"
#define FRU_BB_BIN    "/tmp/fruid_bb.bin"
#define FRU_SLOT1_BIN "/tmp/fruid_slot1.bin"

#define CLASS1_FRU_BUS 11
#define CLASS2_FRU_BUS 10
#define BMC_FRU_ADDR 0x54
#define BB_FRU_ADDR  0x51
#define I2C_PATH "/sys/class/i2c-dev/i2c-%d/device/new_device"
#define EEPROM_PATH "/sys/bus/i2c/devices/%d-00%X/eeprom"

#define MAX_NUM_FRUS 7
enum {
  FRU_ALL   = 0,
  FRU_SLOT0 = 1,
  FRU_SLOT1 = 2,
  FRU_SLOT2 = 3,
  FRU_SLOT3 = 4,
  FRU_BB    = 5,
  FRU_NIC   = 6,
  FRU_BMC   = 7,
};

enum {
  IPMB_SLOT0_I2C_BUS = 0,
  IPMB_SLOT1_I2C_BUS = 1,
  IPMB_SLOT2_I2C_BUS = 2,
  IPMB_SLOT3_I2C_BUS = 3,
};

enum {
  NIC_BMC = 0x09,
  BB_BMC  = 0x0E,
};

const static uint8_t gpio_server_prsnt[] =
{
/*  GPIOB4_PRSNT_MB_BMC_SLOT1_BB_N,
  GPIOB5_PRSNT_MB_BMC_SLOT2_BB_N,
  GPIOB6_PRSNT_MB_BMC_SLOT3_BB_N,
  GPIOB7_PRSNT_MB_BMC_SLOT4_BB_N
*/
};

const static uint8_t gpio_bic_ready[] =
{
/*
  GPIOB0_SMB_BMC_SLOT1_ALT_N,
  GPIOB1_SMB_BMC_SLOT2_ALT_N,
  GPIOB2_SMB_BMC_SLOT3_ALT_N,
  GPIOB3_SMB_BMC_SLOT4_ALT_N
*/
};

const static uint8_t gpio_server_hsc_pgood_sts[] =
{
/*
  GPIOS0_PWROK_STBY_BMC_SLOT1,
  GPIOS1_PWROK_STBY_BMC_SLOT2,
  GPIOS2_PWROK_STBY_BMC_SLOT3,
  GPIOS3_PWROK_STBY_BMC_SLOT4
*/
};

int get_bmc_location(uint8_t *id);
int is_valid_slot_str(char *str, uint8_t *fru);
int is_valid_slot_id(uint8_t fru);
int fby3_common_get_bus_id(uint8_t slot_id);

#ifdef __cplusplus
} // extern "C"
#endif

#endif /* __FBY3_COMMON_H__ */
