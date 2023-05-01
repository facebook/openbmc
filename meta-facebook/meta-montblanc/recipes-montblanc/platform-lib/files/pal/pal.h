/*
 *
 * Copyright (c) Meta Platforms, Inc. and affiliates. (http://www.meta.com)
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
#ifndef __PAL_H__
#define __PAL_H__

#include <openbmc/obmc-pal.h>
#include <openbmc/kv.h>
#ifdef __cplusplus
extern "C" {
#endif
#include <openbmc/ipmi.h>
#include <stdbool.h>
#include <math.h>

#define PLATFORM_NAME "montblanc"
#define READ_UNIT_SENSOR_TIMEOUT 5

extern const char pal_fru_list[];
enum {
  FRU_ALL = 0,
  FRU_SCM = 1,
  MAX_NUM_FRUS = 2,
};



#ifdef __cplusplus
} // extern "C"
#endif

#endif /* __PAL_H__ */