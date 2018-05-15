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

/* TODO: Move to source once dependence is removed */
#define KV_STORE "/mnt/data/kv_store/%s"
#define KV_STORE_PATH "/mnt/data/kv_store"

/*=====================================================================
 *                              Flags
 *====================================================================*/

/* Use the persistant database to lookup key */
#define KV_FPERSIST       (1 << 0)

/* Will set the key:value only if the key does not already exist */
#define KV_FCREATE        (1 << 1)

int kv_get(char *key, char *value, size_t *len, unsigned int flags);
int kv_set(char *key, char *value, size_t len, unsigned int flags);

/*=====================================================================
 *                  Backwards compatibiliy APIs
 *====================================================================*/

/* Poor man's function overloading in C.
 * If user calls kv_get(a,b), it will call kv_get_str().
 * If the user calls kv_get(a,b,c,d), it will call kv_get() as above.
 * Same applies to kv_set().
 * Remove these when all have migrated to the new improved prototypes! */
#define MKFN_N(fn,n0,n1,n2,n3,n,...) fn##n
#define MKFN(fn,...) MKFN_N(fn,##__VA_ARGS__,,bad3,_str,bad1,bad0)(__VA_ARGS__)
#define kv_get(...) MKFN(kv_get,##__VA_ARGS__)
#define kv_set(...) MKFN(kv_set,##__VA_ARGS__)

static inline int kv_set_bin(char *key, char *value, unsigned char len) {
  int rc = kv_set(key, value, (size_t)len, KV_FPERSIST);
  if (rc == 0)
    rc = (int)len;
  return rc;
}

static inline int kv_get_bin(char *key, char *value) {
  size_t len;
  int rc = kv_get(key, value, &len, KV_FPERSIST);
  if (rc == 0)
    rc = (int)len;
  return rc;
}

static inline int kv_get_str(char *key, char *value) {
  return kv_get(key, value, NULL, KV_FPERSIST);
}

static inline int kv_set_str(char *key, char *value) {
  return kv_set(key, value, 0, KV_FPERSIST);
}

#ifdef __cplusplus
}
#endif

#endif /* __KV_H__ */
