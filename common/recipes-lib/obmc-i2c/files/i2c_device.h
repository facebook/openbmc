/*
 * Copyright 2020-present Facebook. All Rights Reserved.
 *
 * This file contains code to provide addendum functionality over the I2C
 * device interfaces to utilize additional driver functionality.
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

#ifndef _OPENBMC_I2C_DEVICE_H_
#define _OPENBMC_I2C_DEVICE_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

int i2c_detect_device(int bus, uint16_t addr);
int i2c_add_device(int bus, uint16_t addr, const char *device_name);
int i2c_delete_device(int bus, uint16_t addr);

#ifdef __cplusplus
} // extern "C"
#endif

#endif /* _OPENBMC_I2C_DEVICE_H_ */
