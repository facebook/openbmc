/*
 * Copyright 2019-present Facebook. All Rights Reserved.
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
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <errno.h>
#include <string.h>
#include <openbmc/obmc-i2c.h>

#include "obmc-pmbus.h"

/*
 * Maximum number of pages defined by pmbus spec is 0x1F, so it should
 * be safe to take below value as invalid page.
 */
#define PMBUS_PAGE_INVALID	(0x1F + 1)

/*
 * The opaque structure to represent a pmbus device.
 */
struct pmbus_device {
	int bus;
	uint16_t addr;

	uint8_t cur_page;
	int device_fd;
};
#define IS_VALID_PMBUS_DEV(dev)	((dev) != NULL && (dev)->device_fd >= 0)

pmbus_dev_t* pmbus_device_open(int bus, uint16_t addr, int flags)
{
	int fd, i2c_flags = 0;
	pmbus_dev_t *pmdev;

	pmdev = malloc(sizeof(*pmdev));
	if (pmdev == NULL)
		return NULL;

	if (flags & PMBUS_FLAG_FORCE_CLAIM) {
		i2c_flags |= I2C_SLAVE_FORCE_CLAIM;
	}
	fd = i2c_cdev_slave_open(bus, addr, i2c_flags);
	if (fd < 0) {
		free(pmdev);
		return NULL;
	}

	pmdev->bus = bus;
	pmdev->addr = addr;
	pmdev->device_fd = fd;
	pmdev->cur_page = PMBUS_PAGE_INVALID;
	return pmdev;
}

void pmbus_device_close(pmbus_dev_t *pmdev)
{
	if (pmdev != NULL) {
		if (pmdev->device_fd >= 0) {
			close(pmdev->device_fd);
		}
		free(pmdev);
	}
}

int pmbus_set_page(pmbus_dev_t *pmdev, uint8_t page)
{
	if (!IS_VALID_PMBUS_DEV(pmdev)) {
		errno = EINVAL;
		return -1;
	}

	if (i2c_smbus_write_byte_data(pmdev->device_fd,
				      PMBUS_PAGE, page) != 0) {
		return -1;
	}

	pmdev->cur_page = page;
	return 0;
}

int pmbus_write_byte_data(pmbus_dev_t *pmdev,
			  uint8_t page,
			  uint8_t reg,
			  uint8_t value)
{
	if (!IS_VALID_PMBUS_DEV(pmdev)) {
		errno = EINVAL;
		return -1;
	}

	if ((pmdev->cur_page != page) &&
	    (pmbus_set_page(pmdev, page) != 0)) {
		return -1;
	}

	return i2c_smbus_write_byte_data(pmdev->device_fd, reg, value);
}
