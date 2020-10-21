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

 /*
  * This file contains functions and logics that depends on Cloudripper specific
  * hardware and kernel drivers. Here, some of the empty "default" functions
  * are overridden with simple functions that returns non-zero error code.
  * This is for preventing any potential escape of failures through the
  * default functions that will return 0 no matter what.
  */

#ifndef __PAL_DEBUGCARD_H__
#define __PAL_DEBUGCARD_H__

int pal_set_post_gpio_out(void);
int pal_post_display(uint8_t status);
int pal_post_handle(uint8_t slot, uint8_t status);
int pal_get_hand_sw_physically(uint8_t *pos);
int pal_get_hand_sw(uint8_t *pos);
int pal_get_dbg_pwr_btn(uint8_t *status);
int pal_get_dbg_rst_btn(uint8_t *status);
int pal_set_rst_btn(uint8_t slot, uint8_t status);
int pal_get_dbg_uart_btn(uint8_t *status);
int pal_clr_dbg_uart_btn();
int pal_switch_uart_mux();

#endif /* __PAL_DEBUGCARD_H__ */