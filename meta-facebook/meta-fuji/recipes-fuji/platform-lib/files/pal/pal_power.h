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

#ifndef __PAL_POWER_H__
#define __PAL_POWER_H__

#include <openbmc/obmc-pal.h>
#include <openbmc/kv.h>

#ifdef __cplusplus
extern "C" {
#endif

#define DELAY_POWER_CYCLE 10
#define DELAY_GRACEFUL_SHUTDOWN 5
#define SCM_COM_PWR_BTN      I2C_SYSFS_DEV_ENTRY(2-0035, cb_pwr_btn_n)
#define SCM_COM_PWR_ENBLE    I2C_SYSFS_DEV_ENTRY(2-0035, com_exp_pwr_enable)
#define SCM_COM_PWR_FORCEOFF I2C_SYSFS_DEV_ENTRY(2-0035, com_exp_pwr_force_off)
#define SCM_COM_RST_BTN      I2C_SYSFS_DEV_ENTRY(2-0035, sys_reset_n)

bool is_server_on(void);
int pal_get_server_power(uint8_t slot_id, uint8_t *status);
int pal_set_server_power(uint8_t slot_id, uint8_t cmd);
void pal_get_chassis_status(uint8_t slot, uint8_t *req_data, uint8_t *res_data, uint8_t *res_len);
uint8_t pal_set_power_restore_policy(uint8_t slot, uint8_t *pwr_policy, uint8_t *res_data);
#ifdef __cplusplus
} // extern "C"
#endif

#endif /* __PAL_POWER_H__ */
