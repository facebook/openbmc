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

#include <openbmc/obmc-pal.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_NUM_FAN     2

#define MAX_NUM_FRUS 6
#define MAX_NODES 4

//extern char * key_list[];
extern size_t pal_pwm_cnt;
extern size_t pal_tach_cnt;
extern char pal_pwm_list[];
extern char pal_tach_list[];
extern const char pal_fru_list[];
extern const char pal_server_list[];
extern const char pal_dev_list[];
extern const char pal_dev_pwr_option_list[];

enum {
  FAN_0 = 0,
  FAN_1,
};


enum {
  FRU_ALL   = 0,
  FRU_SLOT1 = 1,
  FRU_SLOT2 = 2,
  FRU_SLOT3 = 3,
  FRU_SLOT4 = 4,
  FRU_SPB   = 5,
  FRU_NIC   = 6,
  FRU_BMC   = 7,
};




#ifdef __cplusplus
} // extern "C"
#endif

#endif /* __PAL_H__ */
