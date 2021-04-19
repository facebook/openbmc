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

#ifndef __BIC_IPMI_H__
#define __BIC_IPMI_H__

#include "bic_xfer.h"

#ifdef __cplusplus
extern "C" {
#endif

// command for INF VR
#define CMD_INF_VR_SET_PAGE       0x00
#define CMD_INF_VR_READ_DATA_LOW  0x42
#define CMD_INF_VR_READ_DATA_HIGH 0x43
// command for ISL VR
#define CMD_ISL_VR_DEVICE_ID      0xAD
#define CMD_ISL_VR_DMAFIX         0xC5
#define CMD_ISL_VR_DMAADDR        0xC7
#define ISL_MFR_CODE              "49"
// command for TI VR
#define CMD_TI_VR_NVM_CHECKSUM    0xF0

#define MAX_IPMB_BUFFER           256

int bic_get_fw_ver(uint8_t slot_id, uint8_t comp, uint8_t *ver);
int bic_me_recovery(uint8_t command);
int bic_get_vr_device_id(uint8_t *rbuf, uint8_t *rlen, uint8_t bus, uint8_t addr);
int bic_get_ifx_vr_remaining_writes(uint8_t bus, uint8_t addr, uint8_t *writes);
int bic_get_isl_vr_remaining_writes(uint8_t bus, uint8_t addr, uint8_t *writes);
int bic_get_vr_ver(uint8_t bus, uint8_t addr, char *key, char *ver_str);
int bic_get_vr_ver_cache(uint8_t bus, uint8_t addr, char *ver_str);
int bic_switch_mux_for_bios_spi(uint8_t mux);
int bic_get_fruid_info(uint8_t fru_id, ipmi_fruid_info_t *info);
int bic_read_fruid(uint8_t fru_id, char *path, int *fru_size);
int bic_write_fruid(uint8_t fru_id, const char *path);
int bic_get_sdr(ipmi_sel_sdr_req_t *req, ipmi_sel_sdr_res_t *res, uint8_t *rlen);
int bic_get_sensor_reading(uint8_t sensor_num, ipmi_sensor_reading_t *sensor);
int bic_get_self_test_result(uint8_t *self_test_result);
int bic_get_sys_guid(uint8_t slot_id, uint8_t *guid, uint8_t guid_size);
int bic_set_sys_guid(uint8_t slot_id, uint8_t *guid, uint8_t guid_size);
int bic_get_80port_record(uint16_t max_len, uint8_t *rbuf, uint8_t *rlen);
int bic_master_write_read(uint8_t slot_id, uint8_t bus, uint8_t addr, uint8_t *wbuf, uint8_t wcnt, uint8_t *rbuf, uint8_t rcnt);

#ifdef __cplusplus
} // extern "C"
#endif

#endif /* __BIC_IPMI_H__ */
