/* Copyright 2015-present Facebook. All Rights Reserved.
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

#ifndef __FBY2_FRUID_H__
#define __FBY2_FRUID_H__

#include <facebook/fby2_common.h>
#include <facebook/fby2_sensor.h>

#define FBY2_FRU_PATH "/tmp/fruid_%s.bin"
#define FBY2_FRU_DEV_PATH "/tmp/fruid_%s_dev%d.bin"

#define MFG_MELLANOX 0x19810000
#define MFG_BROADCOM 0x3D110000
#define MFG_UNKNOWN 0xFFFFFFFF

#ifdef __cplusplus
extern "C" {
#endif

int plat_get_ipmb_bus_id(uint8_t slot_id);
uint32_t fby2_get_nic_mfgid(void);
int fby2_get_fruid_path(uint8_t fru, uint8_t dev_id, char *path);
int fby2_get_fruid_eeprom_path(uint8_t fru, char *path);
int fby2_get_fruid_name(uint8_t fru, char *name);

#ifdef __cplusplus
} // extern "C"
#endif

#endif /* __FBY2_FRUID_H__ */
