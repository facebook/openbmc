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

#ifndef __ME_H__
#define __ME_H__

#include <openbmc/ipmi.h>
#include <openbmc/ipmb.h>

#ifdef __cplusplus
extern "C" {
#endif

int me_get_dev_id(ipmi_dev_id_t *id);
int me_get_fw_ver(uint8_t *ver);

int me_get_fruid_info(uint8_t fru_id, ipmi_fruid_info_t *info);
int me_read_fruid(uint8_t fru_id, const char *path);
int me_write_fruid(uint8_t fru_id, const char *path);

int me_get_sel_info(ipmi_sel_sdr_info_t *info);
int me_get_sel_rsv(uint16_t *rsv);
int me_get_sel(ipmi_sel_sdr_req_t *req, ipmi_sel_sdr_res_t *res, uint8_t *rlen);

int me_get_sdr_info(ipmi_sel_sdr_info_t *info);
int me_get_sdr_rsv(uint16_t *rsv);
int me_get_sdr(ipmi_sel_sdr_req_t *req, ipmi_sel_sdr_res_t *res, uint8_t *rlen);

int me_read_sensor(uint8_t sensor_num, ipmi_sensor_reading_t *sensor);

int me_get_sys_guid(uint8_t *guid);

int me_xmit(uint8_t *txbuf, uint8_t txlen, uint8_t *rxbuf, uint8_t *rxlen);

#ifdef __cplusplus
} // extern "C"
#endif

#endif /* __BIC_H__ */
