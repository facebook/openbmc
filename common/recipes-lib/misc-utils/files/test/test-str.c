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

#define TEST_STR_MAX_LEN	128

struct str_strip_test_data {
	char *input;
	char *after_lstrip;
	char *after_rstrip;
	char *after_strip;
};

/*
 * test str_strip() related functions.
 */
void test_str_strip(struct test_stats *stats)
{
	int i;
	char *output;
	char buf[TEST_STR_MAX_LEN];
	struct str_strip_test_data str_strip_tests[] = {
		{
			"string without leading/trailing spaces",
			"string without leading/trailing spaces",
			"string without leading/trailing spaces",
			"string without leading/trailing spaces",
		},
		{
			"    string with leading spaces",
			"string with leading spaces",
			"    string with leading spaces",
			"string with leading spaces",
		},
		{
			"string with trailing spaces  \n",
			"string with trailing spaces  \n",
			"string with trailing spaces",
			"string with trailing spaces",
		},
		{
			"     ",
			"",
			"",
			"",
		},
		{
			"",
			"",
			"",
			"",
		},
		/* This is always the last element. */
		{
			NULL,
			NULL,
			NULL,
			NULL,
		},
	};

	for (i = 0; str_strip_tests[i].input != NULL; i++) {
		struct str_strip_test_data *td = &str_strip_tests[i];

		/* Test str_lstrip(). */
		LOG_DEBUG("test str_lstrip():\n");
		LOG_DEBUG("    - input: <%s>\n", td->input);
		strncpy(buf, td->input, sizeof(buf));
		output = str_lstrip(buf);
		if (strcmp(output, td->after_lstrip) != 0) {
			LOG_ERR("str_lstrip() failed with input:\n<%s>\n",
				td->input);
			stats->num_errors++;
		}
		stats->num_total++;

		/* Test str_rstrip(). */
		LOG_DEBUG("test str_rstrip():\n");
		LOG_DEBUG("    - input: <%s>\n", td->input);
		strncpy(buf, td->input, sizeof(buf));
		output = str_rstrip(buf);
		if (strcmp(output, td->after_rstrip) != 0) {
			LOG_ERR("str_rstrip() failed with input:\n<%s>\n",
				td->input);
			stats->num_errors++;
		}
		stats->num_total++;

		/* Test str_rstrip(). */
		LOG_DEBUG("test str_strip():\n");
		LOG_DEBUG("    - input: <%s>\n", td->input);
		strncpy(buf, td->input, sizeof(buf));
		output = str_strip(buf);
		if (strcmp(output, td->after_strip) != 0) {
			LOG_ERR("str_strip() failed with input:\n<%s>\n",
				td->input);
			stats->num_errors++;
		}
		stats->num_total++;
	}
}

/*
 * test str_starts|endswith() functions.
 */
void test_str_pattern(struct test_stats *stats)
{
	int i;
	char *test_str = "this is test string";
	char *valid_prefixes[] = {
		"t",
		"this is",
		NULL,
	};
	char *invalid_prefixes[] = {
		"this test",
		"this is test string oops more bytes",
		NULL,
	};
	char *valid_suffixes[] = {
		"ing",
		"est string",
		NULL,
	};
	char *invalid_suffixes[] = {
		"teststring",
		"...this is test string",
		NULL,
	};

	/* test str_startswith() with valid prefix. */
	for (i = 0; valid_prefixes[i] != NULL; i++) {
		LOG_DEBUG("test str_startswith():\n");
		LOG_DEBUG("    - str: <%s>\n", test_str);
		LOG_DEBUG("    - pattern: <%s>\n", valid_prefixes[i]);
		if (!str_startswith(test_str, valid_prefixes[i])) {
			LOG_ERR("str_startswith(<%s>, <%s>) test failed\n",
				test_str, valid_prefixes[i]);
			stats->num_errors++;
		}
		stats->num_total++;
	}

	/* test str_startswith() with invalid prefix. */
	for (i = 0; invalid_prefixes[i] != NULL; i++) {
		LOG_DEBUG("test str_startswith():\n");
		LOG_DEBUG("    - str: <%s>\n", test_str);
		LOG_DEBUG("    - pattern: <%s>\n", invalid_prefixes[i]);
		if (str_startswith(test_str, invalid_prefixes[i])) {
			LOG_ERR("str_startswith(<%s>, <%s>) test failed\n",
				test_str, invalid_prefixes[i]);
			stats->num_errors++;
		}
		stats->num_total++;
	}

	/* test str_endswith() with valid suffix. */
	for (i = 0; valid_suffixes[i] != NULL; i++) {
		LOG_DEBUG("test str_endswith():\n");
		LOG_DEBUG("    - str: <%s>\n", test_str);
		LOG_DEBUG("    - pattern: <%s>\n", valid_suffixes[i]);
		if (!str_endswith(test_str, valid_suffixes[i])) {
			LOG_ERR("str_endswith(<%s>, <%s>) test failed\n",
				test_str, valid_suffixes[i]);
			stats->num_errors++;
		}
		stats->num_total++;
	}

	/* test str_endswith() with invalid suffix. */
	for (i = 0; invalid_suffixes[i] != NULL; i++) {
		LOG_DEBUG("test str_endswith():\n");
		LOG_DEBUG("    - str: <%s>\n", test_str);
		LOG_DEBUG("    - pattern: <%s>\n", invalid_suffixes[i]);
		if (str_endswith(test_str, invalid_suffixes[i])) {
			LOG_ERR("str_endswith(<%s>, <%s>) test failed\n",
				test_str, invalid_suffixes[i]);
			stats->num_errors++;
		}
		stats->num_total++;
	}
}
