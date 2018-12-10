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

#ifndef _OBMC_MISC_UTILS_H_
#define _OBMC_MISC_UTILS_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <unistd.h>
#include <stdbool.h>

/*
 * String utility functions.
 */
char* str_lstrip(char *input);
char* str_rstrip(char *input);
char* str_strip(char *input);
bool str_startswith(const char *str, const char *pattern);
bool str_endswith(const char *str, const char *pattern);

/*
 * File IO utility functions.
 */
ssize_t file_read_bytes(int fd, void *buf, size_t count);
ssize_t file_write_bytes(int fd, const void *buf, size_t count);

/*
 * pathname utility functions.
 */
int path_split(char *path, char **entries, int *size);
char* path_join(char *buf, size_t size, ...);
bool path_exists(const char *path);
bool path_isfile(const char *path);
bool path_isdir(const char *path);
bool path_islink(const char *path);

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* _OBMC_MISC_UTILS_H_ */
