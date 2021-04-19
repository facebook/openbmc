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

#ifndef __FBGC_FRUID_H__
#define __FBGC_FRUID_H__

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_FRU_NAME_STR  64
#define MAX_BIN_FILE_STR  32
#define MAX_FILE_PATH     64

int fbgc_get_fruid_name(uint8_t fru, char *name);
int fbgc_get_fruid_path(uint8_t fru, char *path);
int fbgc_get_fruid_eeprom_path(uint8_t fru, char *path);
int fbgc_fruid_write(uint8_t fru, char *path);

#ifdef __cplusplus
} // extern "C"
#endif

#endif /* __FBGC_FRUID_H__ */
