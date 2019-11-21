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
#include "pal_power.h"
#include "pal_sensors.h"
#include "pal_health.h"

#ifdef __cplusplus
extern "C" {
#endif

#define PWR_OPTION_LIST "status, graceful-shutdown, off, on, reset, cycle"
#define FRU_EEPROM_MB    "/sys/class/i2c-dev/i2c-4/device/4-0054/eeprom"
#define LARGEST_DEVICE_NAME (120)
#define UNIT_DIV            (1000)
#define ERR_NOT_READY       (-2)
#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))

#define ALTERA_CPLD_I2C_PFR_ADDR    (0x5A)
#define ALTERA_CPLD_I2C_MOD_ADDR    (0x55)
#define ALTERA_CPLD_I2C_BUS         (4)
#define I2C_FILE_NAME               "/dev/i2c-%d"

// According to QSYS setting in FPGA project

// on-chip Flash IP
#define ON_CHIP_FLASH_IP_CSR_BASE        (0x00100020)
#define ON_CHIP_FLASH_IP_CSR_STATUS_REG  (ON_CHIP_FLASH_IP_CSR_BASE + 0x0)
#define ON_CHIP_FLASH_IP_CSR_CTRL_REG    (ON_CHIP_FLASH_IP_CSR_BASE + 0x4)

#define ON_CHIP_FLASH_IP_DATA_REG        (0x00000000)
// Dual-boot IP
#define DUAL_BOOT_IP_BASE                (0x00100000)
#define CFM1_START_ADDR                  (0x00008000)
#define CFM1_END_ADDR                    (0x00049FFF)

extern size_t pal_pwm_cnt;
extern size_t pal_tach_cnt;
extern const char pal_pwm_list[];
extern const char pal_tach_list[];
extern const char pal_fru_list[];
extern const char pal_server_list[];

enum {
  FAN_0 = 0,
  FAN_1,
};

enum {
  FRU_ALL  = 0,
  FRU_MB = 1,
  FRU_NIC0 = 2,
  FRU_NIC1 = 3,
};

#define MAX_NUM_FRUS 3
#define MAX_NODES    1
#define READING_SKIP    (1)
#define READING_NA      (-2)

// Sensors Under Side Plane
enum {
  MB_SENSOR_TBD,
};

enum{
  MEZZ_SENSOR_TBD,
};

enum {
  BOOT_DEVICE_IPV4     = 0x1,
  BOOT_DEVICE_HDD      = 0x2,
  BOOT_DEVICE_CDROM    = 0x3,
  BOOT_DEVICE_OTHERS   = 0x4,
  BOOT_DEVICE_IPV6     = 0x9,
  BOOT_DEVICE_RESERVED = 0xff,
};

enum {
  IPMI_CHANNEL_0 = 0,
  IPMI_CHANNEL_1,
  IPMI_CHANNEL_2,
  IPMI_CHANNEL_3,
  IPMI_CHANNEL_4,
  IPMI_CHANNEL_5,
  IPMI_CHANNEL_6,
  IPMI_CHANNEL_7,
  IPMI_CHANNEL_8,
  IPMI_CHANNEL_9,
  IPMI_CHANNEL_A,
  IPMI_CHANNEL_B,
  IPMI_CHANNEL_C,
  IPMI_CHANNEL_D,
  IPMI_CHANNEL_E,
  IPMI_CHANNEL_F,
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
};

enum {
  PAL_LCMXO2_2000HC = 0,
  PAL_LCMXO2_4000HC,
  PAL_LCMXO2_7000HC,
  PAL_MAX10_10M16_PFR,
  PAL_MAX10_10M16_MOD,
  PAL_UNKNOWN_DEV
};

int pal_is_fru_prsnt(uint8_t fru, uint8_t *status);
int pal_is_slot_server(uint8_t fru);
int pal_set_id_led(uint8_t slot, uint8_t status);
int pal_get_fru_id(char *fru_str, uint8_t *fru);
int pal_get_fru_name(uint8_t fru, char *name);
int pal_set_key_value(char *key, char *value);
int pal_get_key_value(char *key, char *value);
void pal_update_ts_sled();
int read_device(const char *device, int *value);
int write_device(const char *device, int value);
int pal_get_me_fw_ver(uint8_t bus, uint8_t addr, uint8_t *ver);

#ifdef __cplusplus
} // extern "C"
#endif

#endif /* __PAL_H__ */
