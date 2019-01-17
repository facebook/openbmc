/*
 *
 * Copyright 2018-present Facebook. All Rights Reserved.
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
#ifndef _PLDM_BASE_H_
#define _PLDM_BASE_H_

// defines data structure specified in DSP0240 v1.0.0 PLDM Base Specification

// PLDM base codes (generic completion codes)
#define CC_SUCCESS    0x00
#define CC_ERROR      0x01
#define CC_ERR_INVALID_DATA 0x02
#define CC_ERR_INVALID_LENGTH 0x03
#define CC_ERR_NOT_READY 0x04
#define CC_ERR_UNSUPPORTED_PLDM_CMD 0x05
#define CC_INVALID_PLDM_TYPE 0x20


// PLDM Common fields
#define PLDM_CM_RQ             0x80000000
#define PLDM_CM_D              0x40000000
#define PLDM_CM_IID_MASK       0x1f000000
#define PLDM_CM_HDR_VER_MASK   0x00a00000
#define PLDM_CM_PLDM_TYPE_MASK 0x003f0000
#define PLDM_CM_CMD_MASK       0x0000ff00
#define PLDM_CM_COMP_MASK      0x000000ff

#define PLDM_COMMON_REQ_LEN   3  // 3 bytes common field for PLDM requests
#define PLDM_COMMON_RES_LEN   4  // 4 bytes for PLDM Responses

typedef  struct timestamp104 { uint8_t x[13]; } timestamp104;
#endif
