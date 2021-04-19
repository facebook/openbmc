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

#ifndef __PAL_POWER_H__
#define __PAL_POWER_H__

#define DELAY_GRACEFUL_SHUTDOWN 5
#define SCM_COM_PWR_ENBLE       I2C_SYSFS_DEV_ENTRY(2-003e, com_exp_pwr_enable)

int pal_set_com_pwr_btn_n(char *status);
bool is_server_on(void);
int pal_set_gb_power(int option);

#endif /* __PAL_POWER_H__ */
