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
  FRU_MB   = 1,
  FRU_PDB  = 2,
  FRU_NIC0 = 3,
  FRU_NIC1 = 4,
  FRU_DBG  = 5,
  FRU_BMC  = 6,
};

#define MAX_NUM_FRUS    (4)
#define MAX_NODES       (1)
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

int pal_is_fru_prsnt(uint8_t fru, uint8_t *status);
int pal_is_slot_server(uint8_t fru);
int pal_get_server_power(uint8_t fru, uint8_t *status);
int pal_set_server_power(uint8_t fru, uint8_t cmd);
int pal_sled_cycle(void);
int pal_set_rst_btn(uint8_t slot, uint8_t status);
int pal_set_led(uint8_t slot, uint8_t status);
int pal_set_id_led(uint8_t slot, uint8_t status);
int pal_get_fru_id(char *fru_str, uint8_t *fru);
int pal_get_fru_name(uint8_t fru, char *name);
int pal_get_key_value(char *key, char *value);
int pal_set_key_value(char *key, char *value);
int pal_get_last_pwr_state(uint8_t fru, char *state);
int pal_set_last_pwr_state(uint8_t fru, char *state);
bool is_server_off(void);
void pal_update_ts_sled();
void pal_get_chassis_status(uint8_t slot, uint8_t *req_data, uint8_t *res_data, uint8_t *res_len);
int read_device(const char *device, int *value);
int pal_get_rst_btn(uint8_t *status);


#ifdef __cplusplus
} // extern "C"
#endif

#endif /* __PAL_H__ */
