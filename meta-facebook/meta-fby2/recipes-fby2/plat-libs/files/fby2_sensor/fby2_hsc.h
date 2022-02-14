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

#ifndef __FBY2_HSC_H__
#define __FBY2_HSC_H__

#include <stdint.h>

struct fby2_spb_hsc_info {
    char i2c_dev_path[16];
    uint8_t i2c_addr;
};

struct fby2_spb_hsc_op {
    int (*read_reg)(const struct fby2_spb_hsc_info hsc_info, float *val);
    int (*read_vin)(const struct fby2_spb_hsc_info hsc_info, float *val);
    int (*read_iout)(const struct fby2_spb_hsc_info hsc_info, float *val);
    int (*read_temp)(const struct fby2_spb_hsc_info hsc_info, float *val);
    int (*read_pin)(const struct fby2_spb_hsc_info hsc_info, float *val);
    int (*read_pin_avg)(const struct fby2_spb_hsc_info hsc_info, float *val);
    int (*read_iout_peak)(const struct fby2_spb_hsc_info hsc_info, float *val);
    int (*read_pin_peak)(const struct fby2_spb_hsc_info hsc_info, float *val);
};

#endif