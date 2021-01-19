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

#include "pal_power.h"
#include "pal_sensors.h"
#include <openbmc/obmc-pal.h>
#include <openbmc/kv.h>
#include "pal_sensors.h"
#include "pal_switch.h"

#ifdef __cplusplus
extern "C" {
#endif

extern size_t pal_pwm_cnt;
extern size_t pal_tach_cnt;
extern const char pal_pwm_list[];
extern const char pal_tach_list[];
extern const char pal_fru_list[];
extern const char pal_server_list[];

enum {
  FRU_ALL = 0,
  FRU_MB,
  FRU_BSM,
  FRU_PDB,
  FRU_CARRIER1,
  FRU_CARRIER2,
  FRU_FIO,
  FRU_CNT,
};

enum {
  SERVER_1 = 0x0,
  SERVER_2,
};

enum {
  BMC = 0x0,
  PCH,
};

enum {
  M2 = 0,
  E1S,
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
  I2C_BUS_21 = 21,
  I2C_BUS_22,
};

#define MAX_NUM_FRUS (FRU_CNT-1)
#define MAX_NODES    1
#define LARGEST_DEVICE_NAME 120
#define READING_SKIP    (1)
#define READING_NA      (-2)
#define ERR_NOT_READY   (-2)
#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))

#define KEY_MB_SNR_HEALTH  "mb_sensor_health"
#define KEY_MB_SEL_ERROR   "mb_sel_error"
#define KEY_PDB_SNR_HEALTH "pdb_sensor_health"
#define KEY_AVA1_SNR_HEALTH "ava1_sensor_health"
#define KEY_AVA2_SNR_HEALTH "ava2_sensor_health"
#define KEY_E1S1_SNR_HEALTH "e1s1_sensor_health"
#define KEY_E1S2_SNR_HEALTH "e1s2_sensor_health"


#ifdef __cplusplus
} // extern "C"
#endif

int pal_get_rst_btn(uint8_t *status);
int pal_control_mux_to_target_ch(uint8_t channel, uint8_t bus, uint8_t mux_addr);
bool pal_is_server_off(void);
bool is_device_ready(void);
int pal_get_platform_id(uint8_t *id);
int pal_check_carrier_type(int index);
int pal_set_id_led(uint8_t status);

#endif /* __PAL_H__ */
