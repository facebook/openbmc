/*
 *
 * Copyright 2019-present Facebook. All Rights Reserved.
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


#ifndef __PAL_SWITCHTEC_H__
#define __PAL_SWITCHTEC_H__

#ifdef __cplusplus
extern "C" {
#endif

#define SWITCHTEC_DEV "/dev/i2c-12@0x%x"
#define SWITCH_BASE_ADDR 0x18

enum {
  PAX0 = 0,
  PAX1,
  PAX2,
  PAX3,
};

int pal_set_pax_proc_ongoing(uint8_t paxid, uint16_t tmout);
bool pal_is_pax_proc_ongoing(uint8_t paxid);
int pal_get_pax_version(uint8_t paxid, char *version);

#ifdef __cplusplus
} // extern "C"
#endif

#endif /* __PAL_SWITCHTEC_H__ */
