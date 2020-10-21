/*
 *
 * Copyright 2020-present Facebook. All Rights Reserved.
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

#ifndef __PAL_IPMI_H__
#define __PAL_IPMI_H__

int pal_get_board_id(uint8_t slot, uint8_t *req_data, uint8_t req_len,
                     uint8_t *res_data, uint8_t *res_len);
void pal_get_chassis_status(uint8_t slot, uint8_t *req_data,
                            uint8_t *res_data, uint8_t *res_len);
uint8_t pal_set_power_restore_policy(uint8_t slot, uint8_t *pwr_policy,
                                     uint8_t *res_data);
int pal_get_plat_sku_id(void);
int pal_get_poss_pcie_config(uint8_t slot, uint8_t *req_data, uint8_t req_len,
                             uint8_t *res_data, uint8_t *res_len);
int pal_set_sysfw_ver(uint8_t slot, uint8_t *ver);
int pal_get_sysfw_ver(uint8_t slot, uint8_t *ver);
int pal_post_enable(uint8_t slot);
int pal_post_get_last(uint8_t slot, uint8_t *status);
int pal_post_get_last_and_len(uint8_t slot, uint8_t *data, uint8_t *len);
int pal_get_80port_record(uint8_t slot, uint8_t *res_data,
                          size_t max_len, size_t *res_len);

#endif /* __PAL_IPMI_H__ */
