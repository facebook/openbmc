/*
 * Copyright 2018-present Facebook. All Rights Reserved.
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
#ifndef YAMP_EEPROM_H
#define YAMP_EEPROM_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <facebook/wedge_eeprom.h>
int yamp_eeprom_parse(const char *fn, struct wedge_eeprom_st *eeprom);
void yamp_calculate_mac(uint8_t *base, int offset, uint8_t *result);
void yamp_parse_mac(uint8_t *dest, char* src, int location);
int yamp_get_lc_bus_name(const char *lc_name, char *bus_name);
#ifdef __cplusplus
}
#endif

#endif
