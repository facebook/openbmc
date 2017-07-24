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
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <syslog.h>
#include <string.h>
#include <sys/file.h>
#include "unqlite.h"
#include "edb.h"

#define MAX_BUF 80
#define MAX_RETRY 5

int
edb_cache_set(char *key, char *value) {

  FILE *fp;
  int rc;
  char kpath[MAX_KEY_PATH_LEN] = {0};

  sprintf(kpath, CACHE_STORE, key);

  if (access(CACHE_STORE_PATH, F_OK) == -1) {
        mkdir(CACHE_STORE_PATH, 0777);
  }

  fp = fopen(kpath, "r+");
  if (!fp && (errno == ENOENT))
      fp = fopen(kpath, "w");
  if (!fp) {
      int err = errno;
#ifdef DEBUG
      syslog(LOG_WARNING, "cache_set: failed to open %s", kpath);
#endif
      return err;
  }

  rc = flock(fileno(fp), LOCK_EX);
  if (rc < 0) {
     int err = errno;
#ifdef DEBUG
    syslog(LOG_WARNING, "cache_set: failed to flock on %s, err %d", kpath, err);
#endif
    fclose(fp);
    return -1;
  }

  if (ftruncate(fileno(fp), 0) < 0) {  //truncate cache file after getting flock
     fclose(fp);
     return -1;
  }

  rc = fwrite(value, 1, strlen(value), fp);
  if (rc < 0) {
#ifdef DEBUG
     syslog(LOG_WARNING, "cache_set: failed to write to %s", kpath);
#endif
     fclose(fp);
     return ENOENT;
  }
  fflush(fp);

  rc = flock(fileno(fp), LOCK_UN);
  if (rc < 0) {
     int err = errno;
#ifdef DEBUG
     syslog(LOG_WARNING, "cache_set: failed to unlock flock on %s, err %d", kpath, err);
#endif
     fclose(fp);
     return -1;
  }

  fclose(fp);

  return 0;
}

int
edb_cache_get(char *key, char *value) {

  FILE *fp;
  int rc, retry = 0;
  char kpath[MAX_KEY_PATH_LEN] = {0};

  sprintf(kpath, CACHE_STORE, key);

  if (access(CACHE_STORE_PATH, F_OK) == -1) {
     mkdir(CACHE_STORE_PATH, 0777);
  }

  while (retry < MAX_RETRY) {
    fp = fopen(kpath, "r");
    if (fp != NULL)
      break;
    retry++;
  }
  if (!fp) {
     int err = errno;
#ifdef DEBUG
    syslog(LOG_WARNING, "cache_get: failed to open %s, err %d", kpath, err);
#endif
    return -1;
  }

  rc = flock(fileno(fp), LOCK_EX);
  if (rc < 0) {
     int err = errno;
#ifdef DEBUG
     syslog(LOG_WARNING, "cache_get: failed to flock %s, err %d", kpath, err);
#endif
    fclose(fp);
    return -1;
  }

  rc = (int) fread(value, 1, MAX_VALUE_LEN, fp);
  if (rc <= 0) {
#ifdef DEBUG
     syslog(LOG_INFO, "cache_get: failed to read %s", kpath);
#endif
    fclose(fp);
    return ENOENT;
  }
  value[(rc < MAX_VALUE_LEN)?(rc):(rc-1)] = 0;

  rc = flock(fileno(fp), LOCK_UN);
  if (rc < 0) {
     int err = errno;
#ifdef DEBUG
     syslog(LOG_WARNING, "cache_get: failed to unlock flock on %s, err %d", kpath, err);
#endif
    fclose(fp);
    return -1;
  }

  fclose(fp);

  return 0;
}
