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

int pal_set_last_pwr_state(uint8_t fru, char *state);
int pal_get_last_pwr_state(uint8_t fru, char *state);
int pal_set_com_pwr_btn_n(char *status);
int pal_get_server_power(uint8_t slot_id, uint8_t *status);
int pal_set_server_power(uint8_t slot_id, uint8_t cmd);
bool is_server_on(void);
int pal_set_gb_power(int option);

#endif /* __PAL_POWER_H__ */