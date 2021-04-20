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
#include "pal.h"

#ifndef MAX_NUM_FRUS
#define MAX_NUM_FRUS 0
#endif

/* This source file contains any definitions which relies on
 * things defined in the platform override of pal.h.
 *
 * We cannot have these defined in obmc-pal.c since it would
 * conflict with the definition/declaration of pal_pwm_cnt/
 * pal_tach_cnt/etc.
 */

int __attribute__((weak))
pal_get_fru_count(void)
{
  return MAX_NUM_FRUS;
}
