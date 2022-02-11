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

/*
 * "obmc-i2c" Library - Naming Guidelines
 *
 *   - I2C Master
 *     The device which initiates a transfer, generates clock signals and
 *     terminates a transfer.
 *     Although people frequently use "i2c-bus/host-adapter" refer to an
 *     i2c master, "master" is preferred over "adapter" in the library.
 *
 *   - I2C Slave
 *     The device addressed by a master.
 *     Try to avoid using "i2c-device" to refer to an i2c slave, because
 *     technically i2c master is also a device.
 *
 *   - I2C Master-Slave
 *     It refers to i2c slave functionality of an i2c master in the library.
 *
 *   - I2C sysfs Slave UID
 *     It refers to "<bus>-<addr>" format sysfs node for an i2c slave. For
 *     example, "8-0070".
 */

#ifndef _OPENBMC_I2C_DEV_H_
#define _OPENBMC_I2C_DEV_H_

#ifdef __cplusplus
extern "C" {
#endif

/*
 * XXX "i2c_cdev.h" needs to be included before "smbus.h", because
 * i2c_smbus_status() refers to "I2C_BUS_STATUS" which is defined in
 * "i2c_cdev.h".
 * Given i2c_smbus_status() is not part of "i2c-tools" package, and it
 * only works in kernel 4.1, we should consider deleting the function
 * later.
 */
#include <openbmc/i2c_cdev.h>
#include <openbmc/i2c_core.h>
#include <openbmc/i2c_device.h>
#include <openbmc/i2c_mslave.h>
#include <openbmc/i2c_sysfs.h>
#include <openbmc/smbus.h>

#ifdef __cplusplus
} // extern "C"
#endif

#endif /* _OPENBMC_I2C_DEV_H_ */
