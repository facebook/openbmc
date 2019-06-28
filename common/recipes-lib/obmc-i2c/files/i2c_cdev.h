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

#ifndef _OPENBMC_I2C_CDEV_H_
#define _OPENBMC_I2C_CDEV_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <unistd.h>
#include <stdlib.h>
#include <stdint.h>
#include <linux/types.h>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>

/*
 * Addendum flags to be used for the 'flags' field of "struct i2c_msg".
 * NOTE: these flags are only valid for kernel 4.1: the same value will
 * be interpreted differently in upstream kernel.
 */
#define I2C_CLIENT_PEC		0x0004	/* Use Packet error checking */
#define I2C_S_EN		0x2000	/* Slave mode enable */
#define I2C_S_ALT		0x2001	/* Slave issue slt */

/*
 * Additional ioctl commands for "/dev/i2c-#".
 * NOTE: these additional ioctl commands are only valid for kernel 4.1:
 * the command codes are not defined in upstream kernel.
 */
#define I2C_SLAVE_RD		0x0710 /* Slave Read */
#define I2C_SLAVE_WR		0x0711 /* Slave /Write */
#define I2C_SLAVE_RDWR		0x0712 /* Slave Read/Write */
#define I2C_BUS_STATUS		0x0799 /* Get Bus Status */

/*
 * Flags for slave open function.
 */
#define I2C_SLAVE_FORCE_CLAIM	0x1

/*
 * Get absolute pathname of i2c master character device.
 *
 * Return:
 *   the buffer storing the path name.
 */
char* i2c_cdev_master_abspath(char *buf, size_t size, int bus);

/*
 * open the i2c slave for read/write.
 *
 * Return:
 *   file descriptor for smbus or plain i2c transactions, or -1 on failures.
 */
int i2c_cdev_slave_open(int bus, uint16_t addr, int flags);

/*
 * close the file descriptor.
 *
 * Return:
 *   0 for success, and -1 on failures.
 */
int i2c_cdev_slave_close(int fd);

/*
 * Issue read/write transactions to the given i2c device.
 *
 * Return:
 *   0 for success, and -1 on failures.
 */
int i2c_rdwr_msg_transfer(int file, __u8 addr, __u8 *tbuf,
			  __u8 tcount, __u8 *rbuf, __u8 rcount);

#ifdef __cplusplus
} // extern "C"
#endif

#endif /* _OPENBMC_I2C_CDEV_H_ */
