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
#include "kv.h"

/*
*  set binary value
*  retrun number of successfully write
*/
int
kv_set_bin(char *key, char *value, unsigned char len) {
  FILE *fp;
  int rc, ret = 0;
  char kpath[MAX_KEY_PATH_LEN] = {0};
  char *p;

  sprintf(kpath, KV_STORE, key);

  p = &kpath[strlen(KV_STORE_PATH)-1];
  while ((*p != '\0') && ((p = strchr(p+1, '/')) != NULL)) {
    *p = '\0';
    if (access(kpath, F_OK) == -1) {
      mkdir(kpath, 0777);
    }
    *p = '/';
  }

  fp = fopen(kpath, "r+");
  if (!fp && (errno == ENOENT))
    fp = fopen(kpath, "w");
  if (!fp) {
    int err = errno;
#ifdef DEBUG
    syslog(LOG_WARNING, "kv_set: failed to open %s", kpath);
#endif
    return err;
  }

  rc = flock(fileno(fp), LOCK_EX);
  if (rc < 0) {
    int err = errno;
#ifdef DEBUG
    syslog(LOG_WARNING, "kv_set: failed to flock on %s, err %d", kpath, err);
#endif
    fclose(fp);
    return -1;
  }

  if (ftruncate(fileno(fp), 0) < 0) {  //truncate cache file after getting flock
    fclose(fp);
    return -1;
  }

  ret = fwrite(value, 1, len, fp);
  if (ret < 0) {
#ifdef DEBUG
    syslog(LOG_WARNING, "kv_set: failed to write to %s", kpath);
#endif
    fclose(fp);
    return ENOENT;
  }
  fflush(fp);

  rc = flock(fileno(fp), LOCK_UN);
  if (rc < 0) {
    int err = errno;
#ifdef DEBUG
    syslog(LOG_WARNING, "kv_set: failed to unlock flock on %s, err %d", kpath, err);
#endif
    fclose(fp);
    return -1;
  }

  fclose(fp);

  return ret;
}

/*
*  get binary value
*  retrun number of successfully read
*/
int
kv_get_bin(char *key, char *value) {
  FILE *fp;
  int rc, ret=0;
  char kpath[MAX_KEY_PATH_LEN] = {0};
  char *p;

  sprintf(kpath, KV_STORE, key);

  p = &kpath[strlen(KV_STORE_PATH)-1];
  while ((*p != '\0') && ((p = strchr(p+1, '/')) != NULL)) {
    *p = '\0';
    if (access(kpath, F_OK) == -1) {
      mkdir(kpath, 0777);
    }
    *p = '/';
  }

  fp = fopen(kpath, "r");
  if (!fp) {
    int err = errno;
#ifdef DEBUG
    syslog(LOG_WARNING, "kv_get: failed to open %s, err %d", kpath, err);
#endif
    return -1;
  }

  rc = flock(fileno(fp), LOCK_EX);
  if (rc < 0) {
    int err = errno;
#ifdef DEBUG
    syslog(LOG_WARNING, "kv_get: failed to flock %s, err %d", kpath, err);
#endif
    fclose(fp);
    return -1;
  }

  ret = (int) fread(value, 1, MAX_VALUE_LEN, fp);
  if (ret < 0 || ferror(fp)) {
#ifdef DEBUG
    syslog(LOG_INFO, "kv_get: failed to read %s", kpath);
#endif
    fclose(fp);
    return -1;
  }

  rc = flock(fileno(fp), LOCK_UN);
  if (rc < 0) {
    int err = errno;
#ifdef DEBUG
    syslog(LOG_WARNING, "kv_get: failed to unlock flock on %s, err %d", kpath, err);
#endif
    fclose(fp);
    return -1;
  }

  fclose(fp);

  return ret;
}

/*
*  set string value which is terminated by a null
*  retrun 0 on success, else on failure
*/
int
kv_set(char *key, char *value) {
  int ret = kv_set_bin(key, value, strlen(value));

  if (ret == strlen(value))
    return 0;
  else
    return -1;
}

/*
*  get string value which is terminated by a null
*  retrun 0 on success, else on failure
*/
int
kv_get(char *key, char *value) {
  int ret = kv_get_bin(key, value);

  if (ret >= 0)
    value[(ret < MAX_VALUE_LEN)?(ret):(ret-1)] = '\0';

  if (ret < 0)
    return -1;

  return 0;
}
