/*
 * Copyright 2018-present Facebook. All Rights Reserved.
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

#include "misc-utils.h"

#define FIO_MAX_RETRY	4

static bool io_error_worth_retry(int retry)
{
	return ((retry < FIO_MAX_RETRY) &&
		(errno == EINTR ||
		 errno == EAGAIN ||
		 errno == EWOULDBLOCK));
}

/*
 * Read <count> bytes from the given file descriptor.
 *
 * Return:
 *   On success, the number of bytes read is returned. The return value
 *   might be smaller than <count>, and it's caller's responsibility to
 *   determine whether it's an error.
 *   On error, -1 is returned, and errno is set appropriately.
 */
ssize_t file_read_bytes(int fd, void *buf, size_t count)
{
	int retry = 0;
	ssize_t nread;
	ssize_t offset = 0;

	if (fd < 0 || buf == NULL) {
		errno = EINVAL;
		return -1;
	}

	while (count > 0) {
		nread = read(fd, buf + offset, count);
		if (nread == 0) {
			break;		/* end of file */
		} else if (nread < 0) {
			if (io_error_worth_retry(retry)) {
				retry++;
				continue;
			}

			if (offset == 0)
				offset = -1;
			break;
		}

		count -= nread;
		offset += nread;
	}

	return offset;
}

/*
 * Write <count> bytes to the given file.
 *
 * Return:
 *   On success, the number of bytes written is returned. The return
 *   value might be smaller than <count>, and it's caller's
 *   responsibility to determine whether it's an error.
 *   On error, -1 is returned, and errno is set appropriately.
 */
ssize_t file_write_bytes(int fd, const void *buf, size_t count)
{
	int retry = 0;
	ssize_t nwrite;
	ssize_t offset = 0;

	if (fd < 0 || buf == NULL) {
		errno = EINVAL;
		return -1;
	}

	while (count > 0) {
		nwrite = write(fd, buf + offset, count);
		if (nwrite == 0) {
			break;
		} else if (nwrite < 0) {
			if (io_error_worth_retry(retry)) {
				retry++;
				continue;
			}

			if (offset == 0)
				offset = -1;
			break;
		}

		offset += nwrite;
		count -= nwrite;
	}

	return offset;
}
