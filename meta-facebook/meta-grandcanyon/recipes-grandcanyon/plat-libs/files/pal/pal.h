/*
 *
 * Copyright 2015-present Facebook. All Rights Reserved.
 *
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

#ifdef __cplusplus
extern "C" {
#endif

#include <openbmc/obmc-pal.h>
#include <openbmc/kv.h>

enum {
  FRU_ALL = 0,
  FRU_SERVER,
  FRU_UIC,
  FRU_DPB,
  FRU_SCC,
  FRU_NIC,
  FRU_IOCM,
  FRU_CNT,
};

#define MAX_NUM_FRUS (FRU_CNT-1)
#define MAX_NODES    1


#ifdef __cplusplus
} // extern "C"
#endif

#endif /* __PAL_H__ */
