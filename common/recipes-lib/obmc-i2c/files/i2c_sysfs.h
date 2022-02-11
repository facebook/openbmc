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

#ifndef _OPENBMC_I2C_SYSFS_H_
#define _OPENBMC_I2C_SYSFS_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <unistd.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

/*
 * I2C sysfs paths.
 */
#define I2C_SYSFS_DEVICES		"/sys/bus/i2c/devices"
#define I2C_SYSFS_DRIVERS		"/sys/bus/i2c/drivers"
#define I2C_SYSFS_BUS_DIR(num)		I2C_SYSFS_DEVICES"/i2c-"#num
#define I2C_SYSFS_DEV_DIR(suid)		I2C_SYSFS_DEVICES"/"#suid
#define I2C_SYSFS_DEV_ENTRY(suid, ent)	I2C_SYSFS_DEVICES"/"#suid"/"#ent

/*
 * Generate sysfs i2c-slave-uid in "<bus>-<addr>" format.
 *
 * Return:
 *   the buffer storing the uid string.
 */
char* i2c_sysfs_suid_gen(char *suid, size_t size, int bus, uint16_t addr);

/*
 * Extract i2c bus and slave address information from sysfs i2c-slave-uid
 * in "<bus>-<addr>" format.
 *
 * Return:
 *   0 for success, and -1 on failures.
 */
int i2c_sysfs_suid_parse(const char *suid, int *bus, uint16_t *addr);

/*
 * Check if the given <i2c-slave-uid> is in valid format.
 *
 * Return:
 *   true if it's valid, otherwise false.
 */
bool i2c_sysfs_is_valid_suid(const char* suid);

/*
 * Generate absolute sysfs path of the given i2c slave.
 *
 * Return:
 *   the buffer storing the absolute sysfs path.
 */
char* i2c_sysfs_slave_abspath(char *path, size_t size, int bus, uint16_t addr);

#ifdef __cplusplus
} // extern "C"
#endif

#endif /* _OPENBMC_I2C_SYSFS_H_ */
