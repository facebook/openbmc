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

#ifndef __EXPANDER_H__
#define __EXPANDER_H__

#include <openbmc/ipmi.h>
#include <openbmc/ipmb.h>
#include <facebook/fbwc_common.h>


#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
  uint8_t iana_id[3];
  uint32_t value;
  uint8_t flags;
  uint8_t status;
  uint8_t ext_status;
  uint8_t read_type;
} exp_extend_sensor_reading_t;

typedef struct
{
  uint8_t board_fru_id;
  uint8_t exp_fru_id;
} target_id_info;

#define EXP_MAX_FRU_DATA_WRITE_COUNT  0x20
#define EXP_VERSION_LEN               4
#define EXP_IPMB_WRITE_COUNT_MAX      224
#define PKT_SIZE                      (64*1024)

enum {
  CMD_GET_EXP_VERSION            = 0x12,
  CMD_OEM_EXP_GET_SENSOR_READING = 0x2D,
  CMD_OEM_EXP_SLED_CYCLE         = 0x30,
  CMD_OEM_EXP_GET_FAN_TACH       = 0xD0,
  CMD_OEM_EXP_SET_FAN_PWM        = 0xD1,
};

enum EXP_FW_VERSION_STATUS {
  EXP_FW_VERSION_DEACTIVATE  = 0,
  EXP_FW_VERSION_ACTIVATE    = 1,
};

enum {
  // exp definition
  UPDATE_EXP = 0,
};

enum {
  // this is defined by the EXP FW
  // Storage - Write FRU Data, Netfn: 0x0A, Cmd: 0x12
  EXP_FRU_ID_SCB = 0,
  EXP_FRU_ID_HDBP = 1,
  EXP_FRU_ID_PDB = 2,
  EXP_FRU_ID_FAN0 = 3,
  EXP_FRU_ID_FAN1 = 4,
  EXP_FRU_ID_FAN2 = 5,
  EXP_FRU_ID_FAN3 = 6,
};

typedef struct Version {
  uint8_t status;
  uint8_t major_ver;
  uint8_t minor_ver;
  uint8_t unit_ver;
  uint8_t dev_ver;
} ver_set;

typedef struct ExpanderVersion {
  ver_set bootloader;
  ver_set firmware_region1;
  ver_set firmware_region2;
  ver_set configuration;
} exp_ver;

int exp_ipmb_wrapper(uint8_t fru_id, uint8_t netfn, uint8_t cmd,
    uint8_t *txbuf, uint16_t txlen, uint8_t *rxbuf, uint8_t *rxlen);
int exp_get_fruid_info(uint8_t fru_id, uint8_t exp_fru_id, ipmi_fruid_info_t *info);
int exp_read_fru_data(uint8_t fru_id, uint8_t exp_fru_id, uint32_t offset, uint8_t count, uint8_t *rbuf, uint8_t *rlen);
int exp_write_fru_data(uint8_t fru_id, uint8_t exp_fru_id, uint32_t offset, uint8_t count, uint8_t *buf);
int exp_read_fruid(uint8_t target_id, char *path);
int exp_write_fruid(uint8_t target_id, char *path);
int exp_get_self_test_result(uint8_t fru_id, uint8_t *self_test_result);
int exp_get_sensor_reading(uint8_t sensor_num, float *value);
int exp_get_accur_sensor(uint8_t fru_id, uint8_t sensor_num, exp_extend_sensor_reading_t *sensor);
int exp_sled_cycle(uint8_t fru_id);
int exp_get_exp_ver(uint8_t fru_id, uint8_t *ver, uint8_t ver_len);
int exp_update_fw(uint8_t fru_id, uint8_t comp, char *path, int fd, uint8_t force);
int exp_get_fan_pwm(uint8_t fru_id, uint8_t pwm_index, uint8_t *pwm);
int exp_set_fan_speed(uint8_t fru_id, uint8_t fan, uint8_t pwm);
int exp_set_fan_mode(uint8_t fru_id, uint8_t mode);
int exp_get_fan_mode(uint8_t fru_id, uint8_t *mode);



#ifdef __cplusplus
} // extern "C"
#endif

#endif /* __EXPANDER_H__ */
