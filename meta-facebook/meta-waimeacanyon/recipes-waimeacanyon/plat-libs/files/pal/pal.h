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
#include <facebook/fbwc_common.h>
#include <facebook/fbwc_fruid.h>

extern const char pal_fru_list[];

#define MAX_NODES       1
#define FRUID_SIZE      512

enum {
  PCIE_CONFIG_IOM_E1S          = 0x01,  // dual compute system
  PCIE_CONFIG_IOM_SAS          = 0x02,  // single compute system
};

void pal_get_chassis_status(uint8_t fru, uint8_t *req_data, uint8_t *res_data, uint8_t *res_len);


#ifdef __cplusplus
} // extern "C"
#endif

#endif /* __PAL_H__ */
