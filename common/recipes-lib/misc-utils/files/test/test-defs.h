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

#ifndef _OBMC_TEST_DEFS_H_
#define _OBMC_TEST_DEFS_H_

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <linux/limits.h>

#include <openbmc/misc-utils.h>

/*
 * Constants.
 */
#define VERBOSE_LOGGING 1

/*
 * Logging utilities.
 */
#ifdef VERBOSE_LOGGING
#define LOG_DEBUG(fmt, args...)	printf(fmt, ##args)
#else
#define LOG_DEBUG(fmt, args...)
#endif /* VERBOSE_LOGGING */
#define LOG_ERR(fmt, args...)	fprintf(stderr, fmt, ##args)

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(a)	(sizeof(a) / sizeof((a)[0]))
#endif /* ARRAY_SIZE */

struct test_stats {
	int num_total;
	int num_errors;
};

/*
 * Test function declarations.
 */
void test_str_strip(struct test_stats *stats);
void test_str_pattern(struct test_stats *stats);
void test_file_read(struct test_stats *stats);
void test_file_write(struct test_stats *stats);
void test_path_exists(struct test_stats *stats);
void test_path_split(struct test_stats *stats);
void test_path_join(struct test_stats *stats);

#endif /* _OBMC_TEST_DEFS_H_ */
