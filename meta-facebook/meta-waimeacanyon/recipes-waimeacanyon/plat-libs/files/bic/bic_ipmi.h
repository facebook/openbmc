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

#ifndef __BIC_IPMI_H__
#define __BIC_IPMI_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <openbmc/ipmi.h>

typedef struct
{
  uint8_t iana_id[3];
  uint32_t value;
  uint8_t flags;
  uint8_t status;
  uint8_t ext_status;
  uint8_t read_type;
} ipmi_extend_sensor_reading_t;

struct bic_get_fw_cksum_sha256_req {
  uint8_t iana_id[3];
  uint8_t target;
  uint32_t offset;
  uint32_t length;
} __attribute__((packed));

struct bic_get_fw_cksum_sha256_res {
  uint8_t iana_id[3];
  uint8_t cksum[32];
} __attribute__((packed));

enum {
  SNR_READ_CACHE = 0,
  SNR_READ_FORCE = 1,
};

enum {
  DISABLE = 0,
  ENABLE = 1,
};


enum {
  BIC_CMD_OEM_GET_SET_GPIO          = 0x41,
  BIC_CMD_OEM_FW_CKSUM_SHA256       = 0x43,
  BIC_CMD_OEM_SET_REGISTER          = 0x69,
};

int bic_get_fw_ver(uint8_t slot_id, uint8_t comp, uint8_t *ver);
int bic_update_firmware(uint8_t target, uint32_t offset, uint16_t len, uint8_t *buf);
int bic_reset(uint8_t slot_id);
int bic_get_80port_record(uint8_t slot_id, uint8_t *rbuf, uint8_t *rlen);
int bic_get_fruid_info(uint8_t slot_id, uint8_t bic_fru_id, ipmi_fruid_info_t *info);
int bic_read_fru_data(uint8_t slot_id, uint8_t bic_fru_id, uint32_t offset, uint8_t count, uint8_t *rbuf, uint8_t *rlen);
int bic_write_fru_data(uint8_t slot_id, uint8_t bic_fru_id, uint32_t offset, uint8_t count, uint8_t *buf);
int bic_get_self_test_result(uint8_t slot_id, uint8_t *self_test_result);
int bic_get_accur_sensor(uint8_t slot_id, uint8_t sensor_num, ipmi_extend_sensor_reading_t *sensor);
int bic_get_sys_guid(uint8_t slot_id, uint8_t *guid);
int bic_set_sys_guid(uint8_t slot_id, uint8_t *guid);
int bic_bypass_get_vr_device_id(uint8_t slot_id, uint8_t *devid, uint8_t *id_len, uint8_t bus, uint8_t addr);
int bic_set_single_gpio(uint8_t slot_id, uint8_t gpio_num, uint8_t value);
int bic_set_gpio_passthrough(uint8_t slot_id, uint8_t enable);
int bic_get_fw_cksum_sha256(uint8_t slot_id, uint8_t target, uint32_t offset, uint32_t len, uint8_t *cksum);
int me_recovery(uint8_t slot_id, uint8_t command);


#ifdef __cplusplus
} // extern "C"
#endif

#endif /* __BIC_IPMI_H__ */
