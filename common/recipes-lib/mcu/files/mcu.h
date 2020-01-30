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

#ifndef __MCU_H__
#define __MCU_H__

#ifdef __cplusplus
extern "C" {
#endif
#include <stdint.h>

enum {
  MCU_FW_RUNTIME    = 0,
  MCU_FW_BOOTLOADER
};

int mcu_get_fw_ver(uint8_t bus, uint8_t addr, uint8_t comp, uint8_t *ver);
int mcu_update_firmware(uint8_t bus, uint8_t addr, const char *path, const char *key, uint8_t is_signed);
int mcu_update_bootloader(uint8_t bus, uint8_t addr, uint8_t target, const char *path);

int usb_dbg_reset_ioexp(uint8_t bus, uint8_t addr);

#ifdef __cplusplus
} // extern "C"
#endif

#endif /* __MCU_H__ */
