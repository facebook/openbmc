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

#define TEST_PATH_EXISTS	"/var/log/dmesg"
#define TEST_PATH_NON_EXISTS	"/var/log/does-not-exist.txt"
#define	TEST_PATH_REG_FILE	"/var/log/messages"
#define	TEST_PATH_DIR_FILE	"/bin"
#define	TEST_PATH_LINK_FILE	"/dev/fd"

void test_path_exists(struct test_stats *stats)
{
	/* path_exists() positive test. */
	LOG_DEBUG("test path_exists(%s)\n", TEST_PATH_EXISTS);
	if (!path_exists(TEST_PATH_EXISTS)) {
		LOG_ERR("path_exists(%s) failed\n", TEST_PATH_EXISTS);
		stats->num_errors++;
	}
	stats->num_total++;

	/* path_exists() negative test. */
	LOG_DEBUG("test path_exists(%s)\n", TEST_PATH_NON_EXISTS);
	if (path_exists(TEST_PATH_NON_EXISTS)) {
		LOG_ERR("path_exists(%s) failed\n", TEST_PATH_NON_EXISTS);
		stats->num_errors++;
	}
	stats->num_total++;

	/* path_isfile() positive test. */
	LOG_DEBUG("test path_isfile(%s)\n", TEST_PATH_REG_FILE);
	if (!path_isfile(TEST_PATH_REG_FILE)) {
		LOG_ERR("path_isfile(%s) failed\n", TEST_PATH_REG_FILE);
		stats->num_errors++;
	}
	stats->num_total++;

	/* path_isfile() negative test. */
	LOG_DEBUG("test path_isfile(%s)\n", TEST_PATH_DIR_FILE);
	if (path_isfile(TEST_PATH_DIR_FILE)) {
		LOG_ERR("path_isfile(%s) failed\n", TEST_PATH_DIR_FILE);
		stats->num_errors++;
	}
	stats->num_total++;

	/* path_isdir() positive test. */
	LOG_DEBUG("test path_isdir(%s)\n", TEST_PATH_DIR_FILE);
	if (!path_isdir(TEST_PATH_DIR_FILE)) {
		LOG_ERR("path_isdir(%s) failed\n", TEST_PATH_DIR_FILE);
		stats->num_errors++;
	}
	stats->num_total++;

	/* path_isdir() negative test. */
	LOG_DEBUG("test path_isdir(%s)\n", TEST_PATH_REG_FILE);
	if (path_isdir(TEST_PATH_REG_FILE)) {
		LOG_ERR("path_isdir(%s) failed\n", TEST_PATH_REG_FILE);
		stats->num_errors++;
	}
	stats->num_total++;

	/* path_islink() positive test. */
	LOG_DEBUG("test path_islink(%s)\n", TEST_PATH_LINK_FILE);
	if (!path_islink(TEST_PATH_LINK_FILE)) {
		LOG_ERR("path_islink(%s) failed\n", TEST_PATH_LINK_FILE);
		stats->num_errors++;
	}
	stats->num_total++;

	/* path_islink() negative test. */
	LOG_DEBUG("test path_islink(%s)\n", TEST_PATH_REG_FILE);
	if (path_islink(TEST_PATH_REG_FILE)) {
		LOG_ERR("path_islink(%s) failed\n", TEST_PATH_REG_FILE);
		stats->num_errors++;
	}
	stats->num_total++;
}

void test_path_split(struct test_stats *stats)
{
	int i, size;
	char buf[PATH_MAX];
	char *entries[] = {
		"/",
		"var",
		"log",
		"one-more-level-dir",
		"log.txt",
	};
	char *split_entries[16];

	stats->num_total++;
	snprintf(buf, sizeof(buf),
		"%s/%s//////%s/%s/%s   ",
		entries[0], entries[1], entries[2],
		entries[3], entries[4]);

	LOG_DEBUG("test path_split(%s)\n", buf);
	size = ARRAY_SIZE(split_entries);
	if (path_split(buf, split_entries, &size) != 0) {
		LOG_ERR("path_split(%s) failed\n", buf);
		stats->num_errors++;
		return;
	}

	if (size != ARRAY_SIZE(entries)) {
		LOG_ERR("path_split(%s) returned unexpected entries, "
			"expect %ld, actual %d\n",
			buf, (long)ARRAY_SIZE(entries), size);
		stats->num_errors++;
		return;
	}

	for (i = 0; i < size; i++) {
		if (strcmp(entries[i], split_entries[i]) != 0) {
			LOG_ERR("path_split(%s), entry %d mismatched: "
				"expect <%s>, actual <%s>\n",
				buf, i, entries[i], split_entries[i]);
			stats->num_errors++;
			return;
		}
	}
}

void test_path_join(struct test_stats *stats)
{
	char buf[PATH_MAX];

	LOG_DEBUG("test path_join(/var/log/messages)\n");
	path_join(buf, sizeof(buf), "/", "var", "log", "messages", NULL);
	if (strcmp(buf, "/var/log/messages") != 0) {
		LOG_ERR("path_join failed, expect <%s>, actual <%s>\n",
			"/var/log/messages", buf);
		stats->num_errors++;
	}
	stats->num_total++;
}
