#pragma once

/* SPDX-License-Identifier: GPL-2.0-or-later
 * Copyright 2015-present Facebook. All Rights Reserved.
 */

#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_KEY_PATH_LEN  96
#define MAX_KEY_LEN       64
#define MAX_VALUE_LEN     256

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
