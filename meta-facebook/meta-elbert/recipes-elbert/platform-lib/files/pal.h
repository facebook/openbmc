#ifndef FILES_PAL_H
#define FILES_PAL_H

/*
 * Copyright 2020-present Facebook. All Rights Reserved.
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

#include <openbmc/obmc-pal.h>

#ifdef __cplusplus
extern "C" {
#endif

#include <openbmc/kv.h>
#include <openbmc/ipmi.h>
#include <stdbool.h>

#define MAX_NODES     2
#define MAX_NUM_FRUS  2

#define FRU_EEPROM "/sys/class/i2c-adapter/i2c-12/12-0050/eeprom"
#define PAGE_SIZE  0x1000

#define ELBERT_PLATFORM_NAME "elbert"
#define ELBERT_MAX_NUM_SLOTS 8

enum {
  FRU_ALL = 0,
  FRU_MB = 1,
};

#ifdef __cplusplus
} // extern "C"
#endif


#endif // FILES_PAL_H
