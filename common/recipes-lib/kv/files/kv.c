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

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>
#include <libgen.h>
#include <fcntl.h>
#include <errno.h>
#include <syslog.h>
#include <string.h>
#include <sys/file.h>
#include <sys/stat.h>
#include "kv.h"

/* Used for the non-persist database */
const char *cache_store = "/tmp/cache_store/%s";
const char *kv_store    = "/mnt/data/kv_store/%s";

#ifdef DEBUG
#ifdef __TEST__
#define KV_DEBUG(fmt, ...) printf(fmt "\n", ## __VA_ARGS__)
#else
#define KV_DEBUG(fmt, ...) syslog(LOG_WARNING, fmt, ## __VA_ARGS__)
#endif
#else
#define KV_DEBUG(fmt, ...)
#endif

static void mkdir_recurse(char *dir, mode_t mode)
{
  if (access(dir, F_OK) == -1) {
    char curr[MAX_KEY_PATH_LEN];
    char parent[MAX_KEY_PATH_LEN];
    strcpy(curr, dir);
    strcpy(parent, dirname(curr));
    mkdir_recurse(parent, mode);
    mkdir(dir, mode);
  }
}

static void key_path_setup(char *kpath, const char *key, unsigned int flags)
{
  char path[MAX_KEY_PATH_LEN];
  /* Create the path if they dont already exist */
  if ((flags & KV_FPERSIST) != 0) {
    sprintf(kpath, kv_store, key);
  } else {
    sprintf(kpath, cache_store, key);
  }
  strcpy(path, kpath);
  mkdir_recurse(dirname(path), 0777);
}

/*
*  set key::value
*  len is the size of value. If 0, it is assumed value is
*      a string and strlen() is used to determine the length.
*  flags is bitmask of options.
*
*  return 0 on success, negative error code on failure.
*/
int
kv_set(const char *key, const char *value, size_t len, unsigned int flags) {
  FILE *fp;
  int rc, ret = -1;
  char kpath[MAX_KEY_PATH_LEN] = {0};
  bool present = true;
  char curr_value[MAX_VALUE_LEN] = {0};

  if (key == NULL || value == NULL) {
    errno = EINVAL;
    return -1;
  }

  key_path_setup(kpath, key, flags);

  /* If the key already exists, and user wants to create it,
   * deny the access */
  if ((flags & KV_FCREATE) && access(kpath, F_OK) != -1) {
    KV_DEBUG("kv_set: FCREATE is provided and %s already exist!\n", kpath);
    return -1;
  }

  /* Length of zero implies we should treat it like a string. */
  if (len == 0) {
    /* The typical buffer allocated is exactly MAX_VALUE_LEN bytes, so we
     * cannot go past this when calculating strlen. */
    len = strnlen(value, MAX_VALUE_LEN);

    /* Unfortunately, this means we do not know if the buffer was null
     * terminated at value[MAX_VALUE_LEN] or missing a null terminator
     * (and we cannot look at it without possibly exceeding the buffer).
     * Assume it is missing and give E2BIG error. */
    if (len >= MAX_VALUE_LEN) {
      errno = E2BIG;
      return -1;
    }
  } else if (len > MAX_VALUE_LEN) {
      errno = E2BIG;
      return -1;
  }

  fp = fopen(kpath, "r+");
  if (!fp && (errno == ENOENT)) {
    fp = fopen(kpath, "w");
    present = false;
  }
  if (!fp) {
    KV_DEBUG("kv_set: failed to open %s %d", kpath, errno);
    return -1;
  }

  rc = flock(fileno(fp), LOCK_EX);
  if (rc < 0) {
    KV_DEBUG("kv_set: failed to flock on %s, err %d", kpath, errno);
    goto close_bail;
  }

  // Check if we are writing the same value. If so, exit early
  // to save on number of times flash is updated.
  if (present && (flags & KV_FPERSIST)) {
    rc = (int)fread(curr_value, 1, MAX_VALUE_LEN, fp);
    if (len == rc && !memcmp(value, curr_value, len)) {
      ret = 0;
      goto unlock_bail;
    }
    fseek(fp, 0, SEEK_SET);
  }

  if (ftruncate(fileno(fp), 0) < 0) {  //truncate cache file after getting flock
    KV_DEBUG("kv_set: truncate failed!\n");
    goto unlock_bail;
  }

  rc = fwrite(value, 1, len, fp);
  if (rc < 0) {
    KV_DEBUG("kv_set: failed to write to %s, err %d", kpath, errno);
    goto unlock_bail;
  }
  fflush(fp);

  if (rc != len) {
    KV_DEBUG("kv_set: Wrote only %d of %d bytes as value", (int)ret, (int)len);
    goto unlock_bail;
  }
  ret = 0;
unlock_bail:
  rc = flock(fileno(fp), LOCK_UN);
  if (rc < 0) {
    KV_DEBUG("kv_set: failed to unlock flock on %s, err %d", kpath, errno);
    ret = -1;
  }
close_bail:
  fclose(fp);
  return ret;
}

/*
*  get key::value
*  len is the return size of value. If NULL then this information
*      is not returned to the user (And assumed to be a string).
*  flags is bitmask of options.
*
*  return 0 on success, negative error code on failure.
*/
int
kv_get(const char *key, char *value, size_t *len, unsigned int flags) {
  FILE *fp;
  int rc, ret=-1;
  char kpath[MAX_KEY_PATH_LEN] = {0};

  if (key == NULL || value == NULL) {
    errno = EINVAL;
    return -1;
  }

  key_path_setup(kpath, key, flags);

  fp = fopen(kpath, "r");
  if (!fp) {
    KV_DEBUG("kv_get: failed to open %s, err %d", kpath, errno);
    return -1;
  }

  rc = flock(fileno(fp), LOCK_EX);
  if (rc < 0) {
    KV_DEBUG("kv_get: failed to flock %s, err %d", kpath, errno);
    goto close_bail;
  }

  rc = (int) fread(value, 1, MAX_VALUE_LEN, fp);
  if (rc < 0 || ferror(fp)) {
    KV_DEBUG("kv_get: failed to read %s", kpath);
    goto unlock_bail;
  }

  // Update length variable.
  if (len)
    *len = rc;
  // If no length was given, treat it as a string.  Ensure we have a null
  // terminator.
  else if ((rc + 1) < MAX_VALUE_LEN)
    value[rc] = '\0';
  else if (value[MAX_VALUE_LEN-1] != '\0') {
    syslog(LOG_WARNING,
	   "kv_get: truncating string-type value for key %s with length %d",
	   key, rc);
    value[MAX_VALUE_LEN-1] = '\0';
  }

  ret = 0;
unlock_bail:
  rc = flock(fileno(fp), LOCK_UN);
  if (rc < 0) {
    KV_DEBUG("kv_get: failed to unlock flock on %s, err %d", kpath, errno);
    ret = -1;
  }
close_bail:
  fclose(fp);
  return ret;
}

#ifdef __TEST__
#include <assert.h>
int main(int argc, char *argv[])
{
  char value[MAX_VALUE_LEN*2];
  size_t len;

  cache_store = "./test/tmp/%s";
  kv_store    = "./test/persist/%s";

  assert(kv_set("test1", "val", 0, KV_FPERSIST) == 0);
  printf("SUCCESS: Creating persist key func call\n");
  assert(access("./test/persist/test1", F_OK) == 0);
  printf("SUCCESS: key file created as expected!\n");
  assert(kv_get("test1", value, NULL, KV_FPERSIST) == 0);
  printf("SUCCESS: Read of key succeeded!\n");
  assert(strcmp(value, "val") == 0);
  printf("SUCCESS: Read key is of value what was expected!\n");

  assert(kv_set("test1", "val", 0, 0) == 0);
  printf("SUCCESS: Creating non-persist key func call\n");
  assert(access("./test/tmp/test1", F_OK) == 0);
  printf("SUCCESS: key file created as expected!\n");
  assert(kv_get("test1", value, NULL, 0) == 0);
  printf("SUCCESS: Read of key succeeded!\n");
  assert(strcmp(value, "val") == 0);
  printf("SUCCESS: Read key is of value what was expected!\n");

  assert(kv_set("test1", "val", 2, KV_FPERSIST) == 0);
  printf("SUCCESS: Updating key with len\n");
  len = 0;
  memset(value, 0, sizeof(value));
  assert(kv_get("test1", value, &len, KV_FPERSIST) == 0);
  printf("SUCCESS: Read of key succeeded!\n");
  assert(strcmp(value, "va") == 0);
  assert(len == 2);
  printf("SUCCESS: Read key is of value & len what was expected!\n");

  assert(kv_set("test1", "v2", 0, KV_FCREATE) != 0);
  memset(value, 0, sizeof(value));
  assert(kv_get("test1", value, NULL, 0) == 0);
  assert(strcmp(value, "val") == 0);
  printf("SUCCESS: KV_FCREATE failed on existing key and did not modify existing value\n");

  assert(kv_set("test2", "val2", 0, KV_FCREATE) == 0);
  memset(value, 0, sizeof(value));
  assert(kv_get("test2", value, NULL, 0) == 0);
  assert(strcmp(value, "val2") == 0);
  printf("SUCCESS: KV_FCREATE succeeded on non-existing key\n");

  memset(value, 'a', sizeof(value));
  errno = 0;
  assert(kv_set("test2", value, 0, 0) < 0);
  assert(errno == E2BIG);
  printf("SUCCESS: string longer than MAX_VALUE_LEN caught\n");

  memset(value, 'a', sizeof(value));
  errno = 0;
  assert(kv_set("test2", value, MAX_VALUE_LEN+1, 0) < 0);
  assert(errno == E2BIG);
  printf("SUCCESS: non-string longer than MAX_VALUE_LEN caught\n");

  memset(value, 'a', sizeof(value));
  assert(kv_set("test2", "asdfasdf", 4, 0) == 0);
  assert(kv_get("test2", value, NULL, 0) == 0);
  assert(strcmp(value, "asdf") == 0);
  printf("SUCCESS: Read key adds nul terminator for strings\n");

  system("rm -rf ./test");

  return 0;
}
#endif
