/*
 *
 * Copyright 2014-present Facebook. All Rights Reserved.
 *
 * This file is a helper file to fill timestamps from platform
 * used by SEL Logs, SDR records etc.
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
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <syslog.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>

// Local helper function to fill time stamp
void
time_stamp_fill(unsigned char *ts) {
  unsigned int time;
  struct timeval tv;

  gettimeofday(&tv, NULL);

  time = tv.tv_sec;
  ts[0] = time & 0xFF;
  ts[1] = (time >> 8) & 0xFF;
  ts[2] = (time >> 16) & 0xFF;
  ts[3] = (time >> 24) & 0xFF;

  return;
}
