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
#include <stdarg.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "misc-utils.h"

/*
 * Split pathname <path> into list of entries and store the entires in
 * <entries> argument. Callers put the size of <entries> array in <size>
 * argument, and when the function returns, <size> contains the number
 * of entries extracted from <path>.
 *
 * Example:
 *   pathname "/var/log/messages" will be splitted to 4 entries: "/",
 *   "var", "log" and "messages".
 *
 * Return:
 *   on for success, and -1 on failures.
 *
 * Note:
 *   <path> string may be modified by the function.
 */
int path_split(char *path, char **entries, int *size)
{
	int i = 0;
	bool new_entry = false;

	if (path == NULL || entries == NULL ||
	    size == NULL || *size <= 0) {
		errno = EINVAL;
		return -1;
	}

	path = str_strip(path);

	/* Store root directory '/' from abs path. */
	if (path[0] == '/') {
		entries[i++] = "/";
		path++;
	}

	for (; *path != '\0' && i < *size; path++) {
		if (*path == '/') {
			*path = '\0';
			new_entry = false;
		} else if (!new_entry) {
			entries[i++] = path;
			new_entry = true;
		}
	}

	*size = i;
	return 0;
}

/*
 * Concatenate path entries into pathname and store the pathname in
 * <buf> argument.
 *
 * Example:
 *   path_join(buf, sizeof(buf), "/var", "log", "dmesg", NULL) will
 *   store "/var/log/dmesg" into <buf>.
 *
 * Return:
 *   The pointer to <buf>.
 *
 * Note:
 *   The path entry list needs to be terminated with NULL.
 */
char* path_join(char *buf, size_t size, ...)
{
	char *cur;
	va_list entry_list;
	size_t offset, len;

	if (buf == NULL || size == 0)
		return NULL;

	offset = 0;
	va_start(entry_list, size);
	while ((cur = va_arg(entry_list, char*)) != NULL &&
	       (offset + 1) < size) {
		if (offset > 0 && buf[offset - 1] != '/')
			buf[offset++] = '/';

		len = strlen(cur);
		if (offset + len >= size)
			break;
		strcpy(&buf[offset], cur);
		offset += len;
	}
	va_end(entry_list);

	buf[offset] = '\0';
	return buf;
}

/*
 * Check if the file <path> exists.
 *
 * Return:
 *   true if the file exists, or false otherwise.
 */
bool path_exists(const char *path)
{
	if (access(path, F_OK) == 0)
		return true;

	return false;
}

/*
 * Check if the given file <path> is a regular file.
 *
 * Return:
 *   true if it's a regular file, or false otherwise.
 */
bool path_isfile(const char *path)
{
	struct stat sbuf;

	if (lstat(path, &sbuf) != 0)
		return false;

	return S_ISREG(sbuf.st_mode);
}

/*
 * Check if the given file <path> is a directory.
 *
 * Return:
 *   true if it's a directory, or false otherwise.
 */
bool path_isdir(const char *path)
{
	struct stat sbuf;

	if (lstat(path, &sbuf) != 0)
		return false;

	return S_ISDIR(sbuf.st_mode);
}

/*
 * Check if the given file <path> is a symbolic link.
 *
 * Return:
 *   true if it's a symbolic link, or false otherwise.
 */
bool path_islink(const char *path)
{
	struct stat sbuf;

	if (lstat(path, &sbuf) != 0)
		return false;

	return S_ISLNK(sbuf.st_mode);
}
