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

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>

#include "i2c_cdev.h"
#include "i2c_sysfs.h"
#include "i2c_device.h"
#include "smbus.h"

int i2c_detect_device(int bus, uint16_t addr) {
	int fd = -1, rc = -1;

	fd = i2c_cdev_slave_open(bus, addr, I2C_SLAVE_FORCE_CLAIM);
	if (fd == -1)
		return -1;

	rc = i2c_smbus_read_byte(fd);
	i2c_cdev_slave_close(fd);

	if (rc < 0)
		return -1;

	return 0;
}

int i2c_add_device(int bus, uint16_t addr, const char *device_name) {
    char cmd[64];

    snprintf(cmd, sizeof(cmd),
             "echo %s %d > "I2C_SYSFS_DEVICES"/i2c-%d/new_device",
             device_name, addr, bus);
    return system(cmd);
}

int i2c_delete_device(int bus, uint16_t addr) {
    char cmd[64];

    snprintf(cmd, sizeof(cmd),
             "echo %d > "I2C_SYSFS_DEVICES"/i2c-%d/delete_device",
             addr, bus);
    return system(cmd);
}
