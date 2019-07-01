/*
 * Copyright 2015-present Facebook. All Rights Reserved.
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

#ifndef __KV_H__
#define __KV_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#define MAX_KEY_PATH_LEN  96
#define MAX_KEY_LEN       64
#define MAX_VALUE_LEN     64

/*=====================================================================
 *                              Flags
 *====================================================================*/

/* Use the persistant database to lookup key */
#define KV_FPERSIST       (1 << 0)

/* Will set the key:value only if the key does not already exist */
#define KV_FCREATE        (1 << 1)

int kv_get(const char *key, char *value, size_t *len, unsigned int flags);
int kv_set(const char *key, const char *value, size_t len, unsigned int flags);

#ifdef __cplusplus
}
#endif

#endif /* __KV_H__ */
