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

#ifndef __PAL_H__
#define __PAL_H__

#include <openbmc/obmc-pal.h>
#include <openbmc/kv.h>
#include "pal_sensors.h"
#include "pal_health.h"
#include "pal_switch.h"

#ifdef __cplusplus
extern "C" {
#endif

#define PWR_OPTION_LIST "status, off, on, cycle"
#define MB_BIN "/tmp/fruid_mb.bin"
#define MB_EEPROM "/sys/class/i2c-dev/i2c-6/device/6-0054/eeprom"
#define PDB_BIN "/tmp/fruid_pdb.bin"
#define PDB_EEPROM "/sys/class/i2c-dev/i2c-16/device/16-0054/eeprom"
#define BSM_BIN "/tmp/fruid_bsm.bin"
#define BSM_EEPROM "/sys/class/i2c-dev/i2c-%d/device/%d-0056/eeprom"

#define LARGEST_DEVICE_NAME 120
#define ERR_NOT_READY -2

#define PFR_MAILBOX_BUS  (4)
#define PFR_MAILBOX_ADDR (0xB0)

extern size_t pal_pwm_cnt;
extern size_t pal_tach_cnt;
extern const char pal_pwm_list[];
extern const char pal_tach_list[];
extern const char pal_fru_list[];
extern const char pal_server_list[];

enum {
  FRU_ALL = 0,
  FRU_MB,
  FRU_PDB,
  FRU_BSM,
  FRU_ASIC0,
  FRU_ASIC1,
  FRU_ASIC2,
  FRU_ASIC3,
  FRU_ASIC4,
  FRU_ASIC5,
  FRU_ASIC6,
  FRU_ASIC7,
};

enum {
  SERVER_1 = 0x0,
  SERVER_2,
  SERVER_3,
  SERVER_4,
};

enum {
  BMC = 0x0,
  PCH,
};

enum {
  SKU_2S = 0x2,
  SKU_4SEX = 0x4,
  SKU_4S = 0x8
};

enum {
  I2C_BUS_0 = 0,
  I2C_BUS_1,
  I2C_BUS_2,
  I2C_BUS_3,
  I2C_BUS_4,
  I2C_BUS_5,
  I2C_BUS_6,
  I2C_BUS_7,
  I2C_BUS_8,
  I2C_BUS_9,
  I2C_BUS_10,
  I2C_BUS_11,
  I2C_BUS_12,
  I2C_BUS_13,
  I2C_BUS_14,
  I2C_BUS_15,
  I2C_BUS_16,
  I2C_BUS_17,
  I2C_BUS_18,
  I2C_BUS_19,
  I2C_BUS_20,
  I2C_BUS_21,
  I2C_BUS_22,
  I2C_BUS_23,
  I2C_BUS_24,
  I2C_BUS_25,
  I2C_BUS_26,
  I2C_BUS_27,
  I2C_BUS_28,
  I2C_BUS_29,
  I2C_BUS_30,
  I2C_BUS_31,
  I2C_BUS_32,
};

#define MAX_NUM_FRUS 11
#define MAX_NODES    1

int read_device(const char *device, int *value);
int write_device(const char *device, int value);
bool pal_is_server_off(void);
bool is_device_ready(void);
int pal_get_platform_id(uint8_t *id);
int pal_set_id_led(uint8_t status);
int pal_force_sled_cycle(void);
int pal_check_switch_config(void);

#ifdef __cplusplus
} // extern "C"
#endif

#endif /* __PAL_H__ */
