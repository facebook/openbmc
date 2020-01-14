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

#include "bic_xfer.h"

#ifdef __cplusplus
extern "C" {
#endif

int bic_get_dev_id(uint8_t slot_id, ipmi_dev_id_t *dev_id, uint8_t intf);
int bic_get_self_test_result(uint8_t slot_id, uint8_t *self_test_result, uint8_t intf);
int bic_get_fruid_info(uint8_t slot_id, uint8_t fru_id, ipmi_fruid_info_t *info, uint8_t intf);
int bic_read_fruid(uint8_t slot_id, uint8_t fru_id, const char *path, int *fru_size, uint8_t intf);
int bic_write_fruid(uint8_t slot_id, uint8_t fru_id, const char *path, uint8_t intf);
int bic_get_sdr(uint8_t slot_id, ipmi_sel_sdr_req_t *req, ipmi_sel_sdr_res_t *res, uint8_t *rlen, uint8_t intf);
int bic_get_fw_ver(uint8_t slot_id, uint8_t comp, uint8_t *ver, uint8_t intf);
int bic_get_80port_record(uint8_t slot_id, uint8_t *rbuf, uint8_t *rlen, uint8_t intf);
int bic_get_cpld_ver(uint8_t slot_id, uint8_t comp, uint8_t *ver, uint8_t bus, uint8_t addr, uint8_t intf);
int bic_get_vr_device_id(uint8_t slot_id, uint8_t comp, uint8_t *rbuf, uint8_t *rlen, uint8_t bus, uint8_t addr, uint8_t intf);
int bic_get_vr_ver(uint8_t slot_id, uint8_t comp, uint8_t *rbuf, uint8_t *rlen, uint8_t addr, uint8_t intf);
int bic_get_exp_cpld_ver(uint8_t slot_id, uint8_t comp, uint8_t *ver, uint8_t bus, uint8_t addr, uint8_t intf);
int bic_get_sensor_reading(uint8_t slot_id, uint8_t sensor_num, ipmi_sensor_reading_t *sensor, uint8_t intf);
int bic_is_m2_exp_prsnt(uint8_t slot_id);
int me_recovery(uint8_t slot_id, uint8_t command);
int bic_switch_mux_for_bios_spi(uint8_t slot_id, uint8_t mux);
int bic_set_gpio(uint8_t slot_id, uint8_t gpio_num,uint8_t value);
int bic_get_sys_guid(uint8_t slot_id, uint8_t *guid);
int bic_set_sys_guid(uint8_t slot_id, uint8_t *guid);
int bic_do_sled_cycle(uint8_t slot_id);
#ifdef __cplusplus
} // extern "C"
#endif

#endif /* __BIC_IPMI_H__ */
