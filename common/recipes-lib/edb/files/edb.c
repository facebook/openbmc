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

static int
edb_set(char *path, char *key, char *value) {
  unqlite *pDb;               /* Database handle */
  int rc;

  /* Open our database */
  rc = unqlite_open(&pDb, path, UNQLITE_OPEN_CREATE);
  if( rc != UNQLITE_OK ){
    syslog(LOG_WARNING, "db_get: unqlite_open failed with rc: %d\n", rc);
    return -1;
  }

  rc = unqlite_kv_store(pDb, key, -1, value, MAX_BUF);
  if( rc != UNQLITE_OK ){
    syslog(LOG_WARNING, "db_get: unqlite_kv_store failed with rc: %d\n", rc);
    return -1;
  }

  /* Auto-commit the transaction and close our database */
  unqlite_close(pDb);

  return 0;
}

static int
edb_get(char *path, char *key, char *value) {
  int rc;
  unqlite *pDb;
  size_t nBytes;      //Data length
  char zBuf[MAX_BUF] = {0};

  while (1) {
    // Open our database;
    rc = unqlite_open(&pDb, path, UNQLITE_OPEN_READONLY|UNQLITE_OPEN_MMAP);
    if( rc != UNQLITE_OK ) {
      syslog(LOG_WARNING, "db_get: unqlite_open fails with rc: %d\n", rc);
      return -1;
    }

    //Extract record content
    nBytes = MAX_BUF;
    memset(zBuf, 0, MAX_BUF);
    rc = unqlite_kv_fetch(pDb, key, -1, value, &nBytes);
    if( rc != UNQLITE_OK ){
      if (rc == UNQLITE_NOTFOUND) {
        syslog(LOG_WARNING, "db_get: can not find the key\n");
        return -1;
      } else {
        syslog(LOG_WARNING, "db_get: unqlite_key_fetch returns %d\n", rc);
        /* Auto-commit the transaction and close our database */
        unqlite_close(pDb);
        continue;
      }
    } else {
      /* Auto-commit the transaction and close our database */
      unqlite_close(pDb);
      return 0;
    }
  }

  return 0;
}

int
edb_cache_set(char *key, char *value) {
  return edb_set(EDB_CACHE_PATH, key, value);
}

int
edb_cache_get(char *key, char *value) {
  return edb_get(EDB_CACHE_PATH, key, value);
}

int
edb_flash_set(char *key, char *value) {
  return edb_set(EDB_FLASH_PATH, key, value);
}
int
edb_flash_get(char *key, char *value) {
  return edb_get(EDB_FLASH_PATH, key, value);
}
