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

#ifndef __BIOS_H__
#define __BIOS_H__
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

int bios_get_ver(uint8_t slot_id, char *ver);
int bios_program(uint8_t slot_id, const char *file, bool check);

#ifdef __cplusplus
} // extern "C"
#endif

#endif /* __BIOS_H__ */
