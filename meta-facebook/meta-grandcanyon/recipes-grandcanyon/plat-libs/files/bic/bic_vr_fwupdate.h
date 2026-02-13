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
#ifdef CONFIG_GRANDCANYON2
#define PVCCIN_FIVRA_ADDR       0xC0
#define PVCCD_HV_ADDR           0xC4
#define PVCCINFAON_ADDR         0xEC
#else
#define VCCIN_ADDR    0xC0
#define VCCIO_ADDR    0xC4
#define VDDQ_AB_ADDR  0xC8
#define VDDQ_DE_ADDR  0xCC
#endif

#ifdef CONFIG_GRANDCANYON2
#define VR_XDPE152XX 3
#define XDPE15254_PRODUCT_ID    0x90
#define XDPE15284_PRODUCT_ID    0x8A
#define XDPE152C4_PRODUCT_ID    0x8C
#define XDPE152XX_DEVID_LEN     2
#define VR_UNKNOWN              0xFF
#endif

#ifdef CONFIG_GRANDCANYON2
#define MAX_SECT_DATA_NUM 200  // up to 200 DWORDs per section
#define MAX_SECT_NUM      16   // up to 16 sections

struct config_sect {
  uint8_t type;          // section type (0x04=Config, 0x07=PMBus LoopA, etc.)
  uint16_t data_cnt;     // number of DWORDs
  uint32_t data[MAX_SECT_DATA_NUM];  // header + data + CRC
};

struct xdpe152xx_config {
  uint8_t addr;          // PMBus address
  uint8_t sect_cnt;      // number of sections
  uint16_t total_cnt;    // total number of DWORDs
  uint32_t sum_exp;      // overall checksum (read from "Checksum :")
  struct config_sect section[MAX_SECT_NUM];
  uint32_t row_1D0_dw[4];
  uint32_t row_210_dw[4];
  bool     have_revD_rows;
};

#endif // CONFIG_GRANDCANYON2

int update_bic_vr(char *image, uint8_t force);

#ifdef __cplusplus
} // extern "C"
#endif

#endif /* __BIC_FWUPDATE_H__ */
