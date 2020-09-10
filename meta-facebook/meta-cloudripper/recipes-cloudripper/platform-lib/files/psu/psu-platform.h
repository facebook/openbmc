/*
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
 * This file is intended to contain platform specific declarations, and
 * it's set to empty intentionally. Each platform needs to override the
 * file if platform specific settings are required.
 */

#ifndef _PSU_PLATFORM_H_
#define _PSU_PLATFORM_H_

#include "psu.h"

#ifdef __cplusplus
extern "C" {
#endif

#define PSU1_EEPROM  "/sys/bus/i2c/devices/40-0050/eeprom"
#define PSU2_EEPROM  "/sys/bus/i2c/devices/41-0050/eeprom"

i2c_info_t psu[] = {
  {-1, 40, 0x50, 0x58, PSU1_EEPROM},
  {-1, 41, 0x50, 0x58, PSU2_EEPROM},
};

extern void sensord_operation(uint8_t num, uint8_t action);

#endif /* _DELTA_PSU_PLATFORM_H_ */
