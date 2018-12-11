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

int main(int argc, char **argv)
{
	struct test_stats test_info = {0, 0};

	test_str_strip(&test_info);
	test_str_pattern(&test_info);
	test_file_read(&test_info);
	test_file_write(&test_info);
	test_path_exists(&test_info);
	test_path_split(&test_info);
	test_path_join(&test_info);

	printf("total %d tests, failed %d\n",
	       test_info.num_total, test_info.num_errors);
	return 0;
}
