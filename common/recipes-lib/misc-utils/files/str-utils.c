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
#include <ctype.h>
#include <string.h>
#include <stdbool.h>

/*
 * Remove leading spaces from the <input> string. The <input> string may
 * be modified (leading spaces being replaced with '\0').
 *
 * Return:
 *   The pointer to the first non-whitespace character (or null terminator)
 *   of the <input> string.
 */
char* str_lstrip(char *input)
{
	if (input != NULL) {
		while (isspace(*input))
			*input++ = '\0';
	}

	return input;
}

/*
 * Remove trailing spaces from the <input> string. The <input> string may
 * be modified (trailing spaces being replaced with '\0').
 *
 * Return:
 *   The pointer to the original string (with trailing spaces removed).
 */
char* str_rstrip(char *input)
{
	if (input != NULL) {
		int i = strlen(input) - 1;
		while (i >= 0 && isspace(input[i]))
			input[i--] = '\0';
	}

	return input;
}

/*
 * Remove both leading and trailing spaces from the <input> string. The
 * <input> string may be modified (leading/trailing spaces being replaced
 * with '\0').
 *
 * Return:
 *   The pointer to the first non-whitespace character (or null terminator)
 *   of the <input> string.
 */
char* str_strip(char *input)
{
	str_rstrip(input);
	return str_lstrip(input);
}

/*
 * Check if a string starts with the given pattern.
 *
 * Return:
 *   true if the string starts with the given pattern; otherwise, false
 *   is returned.
 */
bool str_startswith(const char *str, const char *pattern)
{
	if (str == NULL || pattern == NULL)
		return false;

	return strncmp(str, pattern, strlen(pattern)) == 0;
}

/*
 * Check if a string ends with the given pattern.
 *
 * Return:
 *   true if the string ends with the given pattern; otherwise, false is
 *   returned.
 */
bool str_endswith(const char *str, const char *pattern)
{
	int plen, slen;

	if (str == NULL || pattern == NULL)
		return false;

	slen = strlen(str);
	plen = strlen(pattern);
	if (slen < plen)
		return false;

	return strcmp(&str[slen - plen], pattern) == 0;
}
