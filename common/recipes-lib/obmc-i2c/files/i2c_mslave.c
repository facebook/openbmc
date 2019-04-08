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

#include <unistd.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>
#include <poll.h>
#include <fcntl.h>
#include <assert.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <linux/limits.h>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>

#include <openbmc/log.h>
#include <openbmc/misc-utils.h>

#include "i2c_core.h"
#include "i2c_cdev.h"
#include "i2c_sysfs.h"
#include "i2c_mslave.h"

/*
 * The number 4096 comes from aspeed-i2c driver in kernel 4.1, and it's
 * much bigger than the default max message size (128) defined in mqueue
 * slave backend.
 */
#define I2C_MSLAVE_MAX_MSG_SIZE		4096

#define I2C_MSLAVE_MAGIC		0x68616861
#define I2C_MSLAVE_MQUEUE_FILE		"slave-mqueue"
#define IS_VALID_MSLAVE_HANDLE(ms)	((ms) != NULL && \
					 (ms)->magic == I2C_MSLAVE_MAGIC)

#define SAVE_ERRNO_RUN(statement)	\
	do {				\
		int save_errno = errno;	\
		statement;		\
		errno = save_errno;	\
	} while (0)

#ifdef OBMC_I2C_DEBUG
#define I2C_VERBOSE(fmt, args...) OBMC_INFO(fmt, ##args)
#else
#define I2C_VERBOSE(fmt, args...)
#endif /* OBMC_I2C_DEBUG */

struct i2c_mslave_handle {
	uint32_t magic;
	int bus;	/* i2c bus number */
	uint16_t addr;	/* slave address */

	int fd;
	char *pathname;

	int (*ms_poll)(i2c_mslave_t *ms, int timeout);
	int (*ms_read)(i2c_mslave_t *ms, void *buf, size_t size);
};

static char* mslave_mqueue_abspath(char *path, size_t size,
				   int bus, uint16_t addr)
{
	char suid[NAME_MAX];

	addr |= I2C_ADDR_OFFSET_SLAVE;
	i2c_sysfs_suid_gen(suid, sizeof(suid), bus, addr);
	return path_join(path, size, I2C_SYSFS_DEVICES, suid,
			 I2C_MSLAVE_MQUEUE_FILE, NULL);
}

static int mslave_mqueue_read(i2c_mslave_t *ms, void *buf, size_t size)
{
	assert(IS_VALID_MSLAVE_HANDLE(ms));

	if (lseek(ms->fd, 0, SEEK_SET) == (off_t) -1)
		return -1;

	return read(ms->fd, buf, size);
}

static int mslave_mqueue_poll(i2c_mslave_t *ms, int timeout)
{
	struct pollfd pfd;

	assert(IS_VALID_MSLAVE_HANDLE(ms));

	pfd.fd = ms->fd;
	pfd.events = POLLPRI;
	return poll(&pfd, 1, timeout);
}

static int mslave_mqueue_init(i2c_mslave_t *ms)
{
	assert(ms != NULL && ms->pathname != NULL);

	ms->fd = open(ms->pathname, O_RDONLY);
	if (ms->fd < 0)
		return -1;

	ms->ms_read = mslave_mqueue_read;
	ms->ms_poll = mslave_mqueue_poll;
	return 0;
}

static int mslave_legacy_read(i2c_mslave_t *ms, void *buf, size_t size)
{
	struct i2c_msg msg;
	struct i2c_rdwr_ioctl_data data = {
		.nmsgs = 1,
		.msgs = &msg,
	};
	uint8_t msg_buf[I2C_MSLAVE_MAX_MSG_SIZE];

	assert(IS_VALID_MSLAVE_HANDLE(ms));

	msg.addr = ms->addr;
	msg.flags = 0; /* 0 for read */
	msg.len = I2C_MSLAVE_MAX_MSG_SIZE;
	msg.buf = msg_buf;
	if (ioctl(ms->fd, I2C_SLAVE_RDWR, &data) < 0)
		return -1;

	if (msg.len > size) {
		OBMC_WARN("%s-%x: i2c message truncated!",
			  ms->pathname, ms->addr);
	} else {
		size = msg.len;
	}
	if (size > 0)
		memcpy(buf, msg.buf, size);

	return size;
}

static int mslave_legacy_poll(i2c_mslave_t *ms, int timeout)
{
	struct pollfd pfd;

	assert(IS_VALID_MSLAVE_HANDLE(ms));

	pfd.fd = ms->fd;
	pfd.events = POLLIN;
	return poll(&pfd, 1, timeout);
}

static int mslave_legacy_init(i2c_mslave_t *ms)
{
	uint8_t msg_buf[4];
	struct i2c_msg msg;
	struct i2c_rdwr_ioctl_data data = {
		.nmsgs = 1,
		.msgs = &msg,
	};

	assert(ms != NULL && ms->pathname != NULL);

	/*
	 * open i2c master chardev file.
	 */
	ms->fd = open(ms->pathname, O_RDWR);
	if (ms->fd < 0)
		return -1;

	/*
	 * enable i2c master's slave functionality.
	 * NOTE: this approach is only valid for kernel 4.1.
	 */
	memset(&msg, 0, sizeof(msg));
	msg.addr = ms->addr;
	msg.flags = I2C_S_EN;
	msg.len = 1;
	msg.buf = msg_buf;
	msg.buf[0] = 1;
	if (ioctl(ms->fd, I2C_SLAVE_RDWR, &data) < 0) {
		SAVE_ERRNO_RUN(close(ms->fd));
		return -1;
	}

	ms->ms_read = mslave_legacy_read;
	ms->ms_poll = mslave_legacy_poll;
	return 0;
}

static i2c_mslave_t* mslave_handle_alloc(int bus, uint16_t addr)
{
	i2c_mslave_t *ms;

	ms = calloc(1, sizeof(*ms));
	if (ms == NULL) {
		errno = ENOMEM;
		return NULL;
	}

	ms->fd = -1;
	ms->bus = bus;
	ms->addr = addr;
	ms->magic = I2C_MSLAVE_MAGIC;
	return ms;
}

static int mslave_handle_free(i2c_mslave_t *ms)
{
	assert(ms != NULL);

	ms->magic = (uint32_t)-1;
	free(ms->pathname); /* allocated by strdup(). */
	if (ms->fd >= 0)
		close(ms->fd);
	free(ms);

	return 0;
}

i2c_mslave_t* i2c_mslave_open(int bus, uint16_t addr)
{
	int ret;
	i2c_mslave_t *ms;
	char mqs_path[PATH_MAX], cdev_path[PATH_MAX];

	if (bus < 0 || !i2c_addr_is_valid(addr)) {
		errno = EINVAL;
		return NULL;
	}

	ms = mslave_handle_alloc(bus, addr);
	if (ms == NULL)
		return NULL;

	/*
	 * I2C slave support is implemented differently in kernel 4.1 and
	 * upstream linux kernel. In general, mqueue slave backend is
	 * preferred over the "legacy" interface in kernel 4.1. Details
	 * as below:
	 * - kernel-4.1:
	 *   i2c slave functionality is enabled by sending ioctl command
	 *   to i2c master character device (/dev/i2c-#).
	 * - kernel-4.18 (or higher versions):
	 *   i2c slave functionality is enabled by kernel config options,
	 *   but in order to read from i2c master-slave, mqueue backend
	 *   is needed.
	 */
	mslave_mqueue_abspath(mqs_path, sizeof(mqs_path), bus, addr);
	if (path_exists(mqs_path)) {
		ms->pathname = strdup(mqs_path);
		ret = mslave_mqueue_init(ms);
	} else {
		i2c_cdev_master_abspath(cdev_path, sizeof(cdev_path), bus);
		ms->pathname = strdup(cdev_path);
		ret = mslave_legacy_init(ms);
	}
	if (ret != 0) {
		SAVE_ERRNO_RUN(mslave_handle_free(ms));
		return NULL;
	}
	I2C_VERBOSE("slave feature enabled on bus %d, addr %#x, "
		    "file=%s, fd=%d",
		    ms->bus, ms->addr, ms->pathname, ms->fd);

	return ms;
}

int i2c_mslave_close(i2c_mslave_t *ms)
{
	if (!IS_VALID_MSLAVE_HANDLE(ms)) {
		errno = EINVAL;
		return -1;
	}
	return mslave_handle_free(ms);
}

int i2c_mslave_read(i2c_mslave_t *ms, void *buf, size_t size)
{
	if (!IS_VALID_MSLAVE_HANDLE(ms) || buf == NULL) {
		errno = EINVAL;
		return -1;
	}

	assert(ms->ms_read != NULL);
	return ms->ms_read(ms, buf, size);
}

int i2c_mslave_poll(i2c_mslave_t *ms, int timeout)
{
	if (!IS_VALID_MSLAVE_HANDLE(ms)) {
		errno = EINVAL;
		return -1;
	}

	assert(ms->ms_poll != NULL);
	return ms->ms_poll(ms, timeout);
}
