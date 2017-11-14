/*
 * Copyright 2014-present Facebook. All Rights Reserved.
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
#ifndef LOG_H
#define LOG_H

#include <stdio.h>
#include <string.h>

//#define DEBUG
//#define VERBOSE

#define _LOG(dst, fmt, ...) do {                  \
  fprintf(dst, "%s:%d " fmt "\n",                 \
          __FUNCTION__, __LINE__, ##__VA_ARGS__); \
  fflush(dst);                                    \
} while(0)

#define LOG_ERR(err, fmt, ...) do {                       \
  char buf[128];                                          \
  strerror_r(err, buf, sizeof(buf));                      \
  _LOG(stderr, "ERROR " fmt ": %s", ##__VA_ARGS__, buf);  \
} while(0)

#define LOG_INFO(fmt, ...) do {                 \
  _LOG(stdout, fmt, ##__VA_ARGS__);             \
} while(0)

#ifdef DEBUG
#define LOG_DBG(fmt, ...) do {                  \
  _LOG(stdout, fmt, ##__VA_ARGS__);             \
} while(0)
#else
#define LOG_DBG(fmt, ...)
#endif

#ifdef VERBOSE
#define LOG_VER(fmt, ...) do {                  \
  _LOG(stdout, fmt, ##__VA_ARGS__);             \
} while(0)
#else
#define LOG_VER(fmt, ...)
#endif

#endif
