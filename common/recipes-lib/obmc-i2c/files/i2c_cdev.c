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
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>
#include <linux/limits.h>

#include "i2c_cdev.h"

char* i2c_cdev_master_abspath(char *buf, size_t size, int bus)
{
	snprintf(buf, size, "/dev/i2c-%d", bus);
	return buf;
}

int i2c_cdev_slave_open(int bus, uint16_t addr, int flags)
{
	int fd;
	unsigned long request;
	char cdev_path[PATH_MAX];

	i2c_cdev_master_abspath(cdev_path, sizeof(cdev_path), bus);
	fd = open(cdev_path, O_RDWR);
	if (fd < 0)
		return -1;

	if (flags & I2C_SLAVE_FORCE_CLAIM) {
		request = I2C_SLAVE_FORCE;
	} else {
		request = I2C_SLAVE;
	}

	if (ioctl(fd, request, addr) < 0) {
		int save_errno = errno;
		close(fd); /* ignore errors */
		errno = save_errno;
		return -1;
	}

	if ((flags & I2C_CLIENT_PEC) && (ioctl(fd, I2C_PEC, addr) < 0)) {
		int save_errno = errno;
		close(fd); /* ignore errors */
		errno = save_errno;
		return -1;
	}

	return fd;
}

int i2c_cdev_slave_close(int fd)
{
	return close(fd);
}

int i2c_rdwr_msg_transfer(int file, __u8 addr, __u8 *tbuf, 
			  __u8 tcount, __u8 *rbuf, __u8 rcount)
{
	struct i2c_rdwr_ioctl_data data;
	struct i2c_msg msg[2];
	int n_msg = 0;
	int rc;

	memset(&msg, 0, sizeof(msg));

	if (tcount) {
		msg[n_msg].addr = addr >> 1;
		msg[n_msg].flags = 0;
		msg[n_msg].len = tcount;
		msg[n_msg].buf = tbuf;
		n_msg++;
	}

	if (rcount) {
		msg[n_msg].addr = addr >> 1;
		msg[n_msg].flags = I2C_M_RD;
		msg[n_msg].len = rcount;
		msg[n_msg].buf = rbuf;
		n_msg++;
	}

	data.msgs = msg;
	data.nmsgs = n_msg;

	rc = ioctl(file, I2C_RDWR, &data);
	if (rc < 0) {
		// syslog(LOG_ERR, "Failed to do raw io");
		return -1;
	}
	return 0;
}
