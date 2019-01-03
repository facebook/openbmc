/*
 *
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
#ifndef _OPENBMC_I2C_DEV_H_
#define _OPENBMC_I2C_DEV_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>

/***********************************************************
 * I2C Addendum
 **********************************************************/

/* Addendum flags to be used for the 'flags' field of 
 * struct i2c_msg */
#define I2C_CLIENT_PEC		0x0004	/* Use Packet error checking */
#define I2C_S_EN		0x2000	/* Slave mode enable */
#define I2C_S_ALT		0x2001	/* Slave issue slt */

#define I2C_ADDR_10BIT_MASK	0xa000

/* Addendum I2C functionality */
#define I2C_FUNC_SMBUS_HWPEC_CALC I2C_FUNC_SMBUS_PEC /* Hardware PEC support */

/***********************************************************
 * I2C-DEV Addendum
 **********************************************************/

/* Additional ioctl commands for /dev/i2c-X */
#define I2C_SLAVE_RD		0x0710 /* Slave Read */
#define I2C_SLAVE_WR		0x0711 /* Slave /Write */
#define I2C_SLAVE_RDWR		0x0712 /* Slave Read/Write */
#define I2C_BUS_STATUS		0x0799 /* Get Bus Status */
/***********************************************************
 * I2C-DEV Helper functions
 **********************************************************/

#define _I2C_MIN(a, b)		(((a) <= (b)) ? (a) : (b))

static inline __s32 i2c_smbus_access(int file, char read_write, __u8 command,
				     int size, union i2c_smbus_data *data)
{
	struct i2c_smbus_ioctl_data args;

	args.read_write = read_write;
	args.command = command;
	args.size = size;
	args.data = data;
	return ioctl(file,I2C_SMBUS,&args);
}

static inline __s32 i2c_smbus_status(int file)
{
	struct i2c_smbus_ioctl_data args;
	return ioctl(file, I2C_BUS_STATUS, &args);
}

static inline __s32 i2c_smbus_write_quick(int file, __u8 value)
{
	return i2c_smbus_access(file,value,0,I2C_SMBUS_QUICK,NULL);
}

static inline __s32 i2c_smbus_read_byte(int file)
{
	union i2c_smbus_data data;
	if (i2c_smbus_access(file,I2C_SMBUS_READ,0,I2C_SMBUS_BYTE,&data))
		return -1;
	else
		return 0x0FF & data.byte;
}

static inline __s32 i2c_smbus_write_byte(int file, __u8 value)
{
	return i2c_smbus_access(file,I2C_SMBUS_WRITE,value,
				I2C_SMBUS_BYTE,NULL);
}

static inline __s32 i2c_smbus_read_byte_data(int file, __u8 command)
{
	union i2c_smbus_data data;
	if (i2c_smbus_access(file,I2C_SMBUS_READ,command,
			     I2C_SMBUS_BYTE_DATA,&data))
		return -1;
	else
		return 0x0FF & data.byte;
}

static inline __s32 i2c_smbus_write_byte_data(int file, __u8 command,
					      __u8 value)
{
	union i2c_smbus_data data;
	data.byte = value;
	return i2c_smbus_access(file,I2C_SMBUS_WRITE,command,
				I2C_SMBUS_BYTE_DATA, &data);
}

static inline __s32 i2c_smbus_read_word_data(int file, __u8 command)
{
	union i2c_smbus_data data;
	if (i2c_smbus_access(file,I2C_SMBUS_READ,command,
			     I2C_SMBUS_WORD_DATA,&data))
		return -1;
	else
		return 0x0FFFF & data.word;
}

static inline __s32 i2c_smbus_write_word_data(int file, __u8 command,
					      __u16 value)
{
	union i2c_smbus_data data;
	data.word = value;
	return i2c_smbus_access(file,I2C_SMBUS_WRITE,command,
				I2C_SMBUS_WORD_DATA, &data);
}

static inline __s32 i2c_smbus_process_call(int file, __u8 command, __u16 value)
{
	union i2c_smbus_data data;
	data.word = value;
	if (i2c_smbus_access(file,I2C_SMBUS_WRITE,command,
			     I2C_SMBUS_PROC_CALL,&data))
		return -1;
	else
		return 0x0FFFF & data.word;
}


/* Returns the number of read bytes */
static inline __s32 i2c_smbus_read_block_data(int file, __u8 command,
					      __u8 *values)
{
	union i2c_smbus_data data;
	if (i2c_smbus_access(file,I2C_SMBUS_READ,command,
			     I2C_SMBUS_BLOCK_DATA,&data))
		return -1;
	else {
		memcpy(values, &data.block[1], _I2C_MIN(data.block[0], I2C_SMBUS_BLOCK_MAX));
		return data.block[0];
	}
}

static inline __s32 i2c_smbus_write_block_data(int file, __u8 command,
					       __u8 length, const __u8 *values)
{
	union i2c_smbus_data data;
	if (length > 32)
		length = 32;
	memcpy(&data.block[1], values, length);
	data.block[0] = length;
	return i2c_smbus_access(file,I2C_SMBUS_WRITE,command,
				I2C_SMBUS_BLOCK_DATA, &data);
}

/* Returns the number of read bytes */
/* Until kernel 2.6.22, the length is hardcoded to 32 bytes. If you
   ask for less than 32 bytes, your code will only work with kernels
   2.6.23 and later. */
static inline __s32 i2c_smbus_read_i2c_block_data(int file, __u8 command,
						  __u8 length, __u8 *values)
{
	union i2c_smbus_data data;

	if (length > 32)
		length = 32;
	data.block[0] = length;
	if (i2c_smbus_access(file,I2C_SMBUS_READ,command,
			     length == 32 ? I2C_SMBUS_I2C_BLOCK_BROKEN :
			      I2C_SMBUS_I2C_BLOCK_DATA,&data))
		return -1;
	else {
		memcpy(values, &data.block[1], _I2C_MIN(data.block[0], I2C_SMBUS_BLOCK_MAX));
		return data.block[0];
	}
}

static inline __s32 i2c_smbus_write_i2c_block_data(int file, __u8 command,
						   __u8 length,
						   const __u8 *values)
{
	union i2c_smbus_data data;
	if (length > 32)
		length = 32;
	memcpy(&data.block[1], values, length);
	data.block[0] = length;
	return i2c_smbus_access(file,I2C_SMBUS_WRITE,command,
				I2C_SMBUS_I2C_BLOCK_BROKEN, &data);
}

/* Returns the number of read bytes */
static inline __s32 i2c_smbus_block_process_call(int file, __u8 command,
						 __u8 length, __u8 *values)
{
	union i2c_smbus_data data;
	if (length > 32)
		length = 32;
	memcpy(&data.block[1], values, length);
	data.block[0] = length;
	if (i2c_smbus_access(file,I2C_SMBUS_WRITE,command,
			     I2C_SMBUS_BLOCK_PROC_CALL,&data))
		return -1;
	else {
		memcpy(values, &data.block[1], _I2C_MIN(data.block[0], I2C_SMBUS_BLOCK_MAX));
		return data.block[0];
	}
}

static inline int i2c_rdwr_msg_transfer(int file, __u8 addr, __u8 *tbuf, 
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

/*
 * Extract i2c bus and device address from sysfs path "<bus>-<addr>".
 *
 * Return:
 *   0 for success, and -1 on failures.
 */
static inline int i2c_sysfs_path_parse(const char *i2c_device,
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

#undef _I2C_MIN

#ifdef __cplusplus
} // extern "C"
#endif

#endif
