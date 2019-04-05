/*
 * Copyright 2017-present Facebook. All Rights Reserved.
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

#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#define I2C_ADDR_10BIT_MASK	0xa000

/*
 * Extract i2c bus and device address from sysfs path "<bus>-<addr>".
 *
 * Return:
 *   0 for success, and -1 on failures.
 */
int i2c_sysfs_path_parse(const char *i2c_device,
			 int *i2c_bus,
			 int *i2c_addr)
{
	int bus, addr;
	char *endptr;

	if (i2c_device == NULL)
		goto error;

	/* parse bus field */
	bus = (int)strtol(i2c_device, &endptr, 10);
	if (bus < 0 || endptr == i2c_device || *endptr != '-')
		goto error;

	/* parse address field */
	endptr++;
	if (strlen(endptr) != 4)
		goto error;
	addr = (int)strtol(endptr, NULL, 16);
	if (addr < 0) {
		goto error;
	} else if (addr & I2C_ADDR_10BIT_MASK) {
		addr &= (~I2C_ADDR_10BIT_MASK);
		if (addr > 0x3ff)
			goto error;	/* invalid 10-bit address */
	} else if (addr == 0 || addr > 0x7f) {
		goto error;		/* invalid 7-bit address */
	}

	if (i2c_bus != NULL)
		*i2c_bus = bus;
	if (i2c_addr != NULL)
		*i2c_addr = addr;
	return 0;

error:
	errno = EINVAL;
	return -1;
}
