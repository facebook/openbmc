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
#define PLDM_CM_IID_MASK       0x1f
#define PLDM_CM_HDR_VER_MASK   0x00a00000
#define PLDM_CM_PLDM_TYPE_MASK 0x3f
#define PLDM_CM_CMD_MASK       0x0000ff00
#define PLDM_CM_COMP_MASK      0x000000ff

#define PLDM_COMMON_REQ_LEN   3  // 3 bytes common field for PLDM requests
#define PLDM_COMMON_RES_LEN   4  // 4 bytes for PLDM Responses

#define PLDM_MAX_IID         32  // 5 bits for IID field, max 32 IIDs

// some offsets in PLDM Common fields
#define PLDM_IID_OFFSET       0  // offset for command IID
      #define PLDM_RESP_MASK       0x7f // turn off Req bit in IID byte
#define PLDM_TYPE_OFFSET      1  // pldm type
#define PLDM_CMD_OFFSET       2  // offset of PLDM cmd in request/response
#define PLDM_CC_OFFSET        3  // offset of completion code in PLDM response
                                 //   Indicates the start of response payload

// PLDM supported command type bitfields
typedef enum pldm_types {
  PLDM_MSG_CTRL    =  0,
  PLDM_SMBIOS      =  1,
  PLDM_MONITORING  =  2,
  PLDM_BIOS_CTRL   =  3,
  PLDM_FRUDATA     =  4,
  PLDM_FW_UPDATE   =  5,
  PLDM_REDFISH_DEVICE_ENABLEMENT = 6,
  PLDM_RSV,
} pldmTypes;



typedef  struct timestamp104 { uint8_t x[13]; } timestamp104;

typedef enum pldm_base_cmds {
  CMD_SET_TID          = 0x1,
  CMD_GET_TID          = 0x2,
  CMD_GET_PLDM_VERSION = 0x3,
  CMD_GET_PLDM_TYPES   = 0x4,
  CMD_GET_PLDM_CMDS    = 0x05,
} PldmBaseCmds;

typedef struct pldm_versoin {
  uint8_t major;
  uint8_t minor;
  uint8_t update;
  uint8_t alpha;
}  pldm_ver_t;

#define PLDM_VERSION_SIZE  4


// cdb for pldm base cmd 0x03 - get PLDM version
typedef struct {
  uint32_t dataTransferHandle;
  uint8_t transferOperationFlag;
  uint8_t pldmType;
} __attribute__((packed)) PLDM_GetPldmVersion_t;

// response for pldm base cmd 0x03 - get PLDM version
typedef struct {
  uint8_t completionCode;
  uint32_t nextTransferHandle;
  uint8_t transferFlag;
  char version[PLDM_VERSION_SIZE];
} __attribute__((packed)) PLDM_GetPldmVersion_Response_t;


// response for pldm base cmd 0x04 - get PLDM types
typedef struct {
  uint8_t completionCode;
  uint64_t supported_types;
} __attribute__((packed)) PLDM_GetPldmTypes_Response_t;


#endif
