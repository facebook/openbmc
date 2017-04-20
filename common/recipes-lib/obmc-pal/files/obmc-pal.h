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
#ifndef __OBMC_PAL_H__
#define __OBMC_PAL_H__

#include <stdint.h>
#include <errno.h>

#ifdef __cplusplus
extern "C" {
#endif

enum {
  PAL_EOK = 0,
  PAL_ENOTSUP = -ENOTSUP,
  /* non system errors start from -256 downwards */
};

int pal_is_fw_update_ongoing(uint8_t fru);
void set_fw_update_ongoing(uint8_t fru, uint16_t tmout);

int pal_init_sensor_check(uint8_t fru, uint8_t snr_num, void *snr);

int pal_set_last_boot_time(uint8_t slot, uint8_t *last_boot_time);
int pal_get_last_boot_time(uint8_t slot, uint8_t *last_boot_time);

#ifdef __cplusplus
}
#endif

#endif
