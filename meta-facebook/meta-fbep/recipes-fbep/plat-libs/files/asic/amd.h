/*
 *
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
 *
 */

#ifndef __AMD_ASIC_H__
#define __AMD_ASIC_H__

#ifdef __cplusplus
extern "C" {
#endif

int amd_read_die_temp(uint8_t, float*);
int amd_read_edge_temp(uint8_t, float*);
int amd_read_hbm_temp(uint8_t, float*);
int amd_read_pwcs(uint8_t, float*);

#ifdef __cplusplus
} // extern "C"
#endif

#endif /* __AMD_ASIC_H__ */
