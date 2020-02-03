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

#define FRU_NIC_BIN    "/tmp/fruid_nic.bin"
#define FRU_BMC_BIN    "/tmp/fruid_bmc.bin"
#define FRU_BB_BIN     "/tmp/fruid_bb.bin"
#define FRU_NICEXP_BIN "/tmp/fruid_nicexp.bin"
#define FRU_SLOT_BIN   "/tmp/fruid_slot%d.bin"

#define I2CDEV "/dev/i2c-%d"

#define SB_CPLD_ADDR 0x0f

#define NIC_FRU_BUS     8
#define CLASS1_FRU_BUS 11
#define CLASS2_FRU_BUS 10
#define BMC_FRU_ADDR 0x54
#define BB_FRU_ADDR  0x51
#define NICEXP_FRU_ADDR 0x51
#define NIC_FRU_ADDR 0x50
#define I2C_PATH "/sys/class/i2c-dev/i2c-%d/device/new_device"
#define EEPROM_PATH "/sys/bus/i2c/devices/%d-00%X/eeprom"

extern const char *slot_usage;

#define MAX_NUM_FRUS 8
enum {
  FRU_ALL   = 0,
  FRU_SLOT1 = 1,
  FRU_SLOT2 = 2,
  FRU_SLOT3 = 3,
  FRU_SLOT4 = 4,
  FRU_BB    = 5,
  FRU_NIC   = 6,
  FRU_BMC   = 7,
  FRU_NICEXP = 8, //the fru is used when bmc is located on class 2
};

enum {
  IPMB_SLOT1_I2C_BUS = 0,
  IPMB_SLOT2_I2C_BUS = 1,
  IPMB_SLOT3_I2C_BUS = 2,
  IPMB_SLOT4_I2C_BUS = 3,
};

enum {
  NIC_BMC = 0x09,
  BB_BMC  = 0x0E,
};

// Server type
enum {
  SERVER_TYPE_DL = 0x0,
  SERVER_TYPE_NONE = 0xFF,
};

//TODO: These pins should be modified because the slot number is rollbacked
//setup-gpio.sh
const static char *gpio_server_prsnt[] =
{
  "",
  "PRSNT_MB_BMC_SLOT0_BB_N",
  "PRSNT_MB_BMC_SLOT1_BB_N",
  "PRSNT_MB_BMC_SLOT2_BB_N",
  "PRSNT_MB_BMC_SLOT3_BB_N"
};

const static char *gpio_server_stby_pwr_sts[] =
{
  "",
  "PWROK_STBY_BMC_SLOT0",
  "PWROK_STBY_BMC_SLOT1",
  "PWROK_STBY_BMC_SLOT2",
  "PWROK_STBY_BMC_SLOT3"
};

const static char *gpio_server_i2c_isolated[] =
{
  "",
  "FM_BMC_SLOT0_ISOLATED_EN_R",
  "FM_BMC_SLOT1_ISOLATED_EN_R",
  "FM_BMC_SLOT2_ISOLATED_EN_R",
  "FM_BMC_SLOT3_ISOLATED_EN_R"
};

int fby3_common_set_fru_i2c_isolated(uint8_t fru, uint8_t val);
int fby3_common_is_bic_ready(uint8_t fru, uint8_t *val);
int fby3_common_server_stby_pwr_sts(uint8_t fru, uint8_t *val);
int fby3_common_get_bmc_location(uint8_t *id);
int fby3_common_get_fru_id(char *str, uint8_t *fru);
int fby3_common_check_slot_id(uint8_t fru);
int fby3_common_get_slot_id(char *str, uint8_t *fru);
int fby3_common_get_bus_id(uint8_t slot_id);
int fby3_common_is_fru_prsnt(uint8_t fru, uint8_t *val);
int fby3_common_get_slot_type(uint8_t fru);
int fby3_common_crashdump(uint8_t fru, bool ierr, bool platform_reset);
int fby3_common_dev_id(char *str, uint8_t *dev);
#ifdef __cplusplus
} // extern "C"
#endif

#endif /* __FBY3_COMMON_H__ */
