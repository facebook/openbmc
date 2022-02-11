/*
 *
 * Copyright 2020-present Facebook. All Rights Reserved.
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

#ifndef __BIC_VR_FWUPDATE_H__
#define __BIC_VR_FWUPDATE_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "bic_xfer.h"
#include "bic.h"

// VR device ID length
#define IFX_DEVID_LEN 0x2
#define ISL_DEVID_LEN 0x4
#define TI_DEVID_LEN  0x6

// VR ADDR
#define VCCIN_ADDR    0xC0
#define VCCIO_ADDR    0xC4
#define VDDQ_AB_ADDR  0xC8
#define VDDQ_DE_ADDR  0xCC

int update_bic_vr(char *image, uint8_t force);

#ifdef __cplusplus
} // extern "C"
#endif

#endif /* __BIC_FWUPDATE_H__ */
