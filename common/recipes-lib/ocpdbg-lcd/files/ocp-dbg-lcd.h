/*
 *
 * Copyright 2017-present Facebook. All Rights Reserved.
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

#ifndef __USB_DBG_H__
#define __USB_DBG_H__

#ifdef __cplusplus
extern "C" {
#endif
#include <stdint.h>

int usb_dbg_init(uint8_t bus, uint8_t mcu_addr, uint8_t io_exp_addr);
int usb_dbg_get_fw_ver(uint8_t comp, uint8_t *ver);
int usb_dbg_update_fw(char *path, uint8_t en_mcu_upd);
int usb_dbg_update_boot_loader(char *path);
void usb_dbg_reset(void);

#ifdef __cplusplus
} // extern "C"
#endif

#endif /* __USB_DBG_H__ */
