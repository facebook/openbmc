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

#include "test-defs.h"

#define FILE_BUF_MAX	32
#define TEST_FILE_RD	"/var/log/dmesg"
#define TEST_FILE_WR	"/dev/null"

/*
 * test file_read_bytes() function.
 */
void test_file_read(struct test_stats *stats)
{
	int fd, nread;
	char buf[FILE_BUF_MAX];

	LOG_DEBUG("test file_read_bytes()\n");
	stats->num_total++;

	fd = open(TEST_FILE_RD, O_RDONLY);
	if (fd < 0) {
		LOG_ERR("failed to open %s: %s\n",
			TEST_FILE_RD, strerror(errno));
		stats->num_errors++;
		return;
	}

	nread = sizeof(buf);
	if (file_read_bytes(fd, buf, nread) != nread) {
		LOG_ERR("failed to read %d bytes from %s\n",
			nread, TEST_FILE_RD);
		stats->num_errors++;
	}

	if (close(fd) != 0)
		LOG_ERR("failed to close %s: %s\n",
			TEST_FILE_RD, strerror(errno));
}

/*
 * test file_write_bytes() function.
 */
void test_file_write(struct test_stats *stats)
{
	int fd, nwrite;
	char buf[FILE_BUF_MAX] = "write to file";

	LOG_DEBUG("test file_write_bytes()\n");
	stats->num_total++;

	fd = open(TEST_FILE_WR, O_RDWR);
	if (fd < 0) {
		LOG_ERR("failed to open %s: %s\n",
			TEST_FILE_WR, strerror(errno));
		stats->num_errors++;
		return;
	}

	nwrite = strlen(buf);
	if (file_write_bytes(fd, buf, nwrite) != nwrite) {
		LOG_ERR("failed to write %d bytes from %s\n",
			nwrite, TEST_FILE_WR);
		stats->num_errors++;
	}

	if (close(fd) != 0)
		LOG_ERR("failed to close %s: %s\n",
			TEST_FILE_WR, strerror(errno));
}
