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
#include <stdbool.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>
#include <fcntl.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/utsname.h>
#include <linux/limits.h>
#include <linux/version.h>

#include "misc-utils.h"

/*
 * Read the version of running kernel.
 *
 * Return:
 *   kernel version.
 */
k_version_t get_kernel_version(void)
{
	int i;
	char *pos;
	struct utsname buf;
	unsigned long versions[3] = {0}; /* major.minor.patch */

	if (uname(&buf) != 0) {
		return 0;	/* 0 is an invalid kernel version. */
	}

	i = 0;
	pos = buf.release;
	while (*pos != '\0' && i < ARRAY_SIZE(versions)) {
		if (isdigit(*pos)) {
			versions[i++] = strtol(pos, &pos, 10);
		} else {
			pos++;
		}
	}

	return KERNEL_VERSION(versions[0], versions[1], versions[2]);
}

/*
 * Lock pid file to ensure there is no duplicated instance running.
 *
 * Returns file descriptor if the pid file can be locked; otherwise -1 is
 * returned.
 */
int single_instance_lock(const char *name)
{
	int fd, ret;
	char path[PATH_MAX];

	if (name == NULL) {
		errno = EINVAL;
		return -1;
	}

	if (name[0] != '/') {
		snprintf(path, sizeof(path), "/var/run/%s.lock", name);
	} else {
		snprintf(path, sizeof(path), "%s", name);
	}

	fd = open(path, O_CREAT | O_RDWR, 0666);
	if (fd < 0)
		return -1;

	ret = flock(fd, LOCK_EX | LOCK_NB);
	if (ret < 0) {
		int saved_errno = errno;
		close(fd);
		errno = saved_errno;
		return -1;
	}

	return fd;
}

/*
 * Release the resources acquired by single_instance_lock().
 *
 * Callers don't have to call this function explicitly because all the
 * file descriptors will be closed when the process is terminated.
 *
 * Returns 0 for success, and -1 on failures.
 */
int single_instance_unlock(int lock_fd)
{
	int ret;

	ret = flock(lock_fd, LOCK_UN);
	if (ret < 0)
		return -1;

	close(lock_fd);
	return 0;
}

soc_model_t get_soc_model(void)
{
	return SOC_MODEL;
}

cpu_model_t get_cpu_model(void)
{
	return CPU_MODEL;
}
