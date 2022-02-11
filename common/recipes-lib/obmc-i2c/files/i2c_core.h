/*
 * Copyright 2019-present Facebook. All Rights Reserved.
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

#ifndef _OPENBMC_I2C_CORE_H_
#define _OPENBMC_I2C_CORE_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <unistd.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

/*
 * I2C address definitions (from kernel drivers/i2c/i2c-core-base.c).
 */
#define I2C_ADDR_OFFSET_TEN_BIT	0xa000
#define I2C_ADDR_OFFSET_SLAVE	0x1000

/*
 * Structure to identify an i2c device.
 */
typedef struct i2c_device_id {
	int bus;
	uint16_t addr;
} i2c_id_t;

/*
 * Helper function to validate i2c slave address.
 *
 * The logic is similar with i2c_check_addr_validity() function in
 * kernel drivers/i2c/i2c-core-base.c.
 *
 * Return:
 *   true if address is valid; false otherwise.
 */
static inline bool
i2c_addr_is_valid(uint16_t addr)
{
	if (addr & I2C_ADDR_OFFSET_TEN_BIT) { /* 10-bit address */
		addr &= ~I2C_ADDR_OFFSET_TEN_BIT;
		if (addr > 0x3ff)
			return false;
	} else { /* 7-bit address */
		addr &= ~I2C_ADDR_OFFSET_SLAVE;
		if (addr == 0 || addr > 0x7f)
			return false;
	}

	return true;
}

#ifdef __cplusplus
} // extern "C"
#endif

#endif /* _OPENBMC_I2C_CORE_H_ */
