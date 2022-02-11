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

#ifndef _OPENBMC_I2C_MSLAVE_H_
#define _OPENBMC_I2C_MSLAVE_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <unistd.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <linux/types.h>

typedef struct i2c_mslave_handle i2c_mslave_t;

/*
 * open/enable slave functionality of the given i2c master.
 *
 * Return:
 *   the opaque handler for futher I/O operations.
 */
i2c_mslave_t* i2c_mslave_open(int bus, uint16_t addr);

/*
 * release all the resources allocated by i2c_mslave_close().
 *
 * Return:
 *   0 for success, and -1 on failures.
 */
int i2c_mslave_close(i2c_mslave_t *ms);

/*
 * read data from the i2c master (acting as slave).
 *
 * Return:
 *   number of bytes returned, or -1 on failures.
 */
int i2c_mslave_read(i2c_mslave_t *ms, void *buf, size_t size);

/*
 * function to test if the i2c master (acting as slave) has data
 * available.
 *
 * Return:
 *   - positive number when data is available.
 *   - 0 when data is not available before timeout.
 *   - -1 on failures.
 */
int i2c_mslave_poll(i2c_mslave_t *ms, int timeout);

#ifdef __cplusplus
} // extern "C"
#endif

#endif /* _OPENBMC_I2C_MSLAVE_H_ */
