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
#ifndef _PLDM_H_
#define _PLDM_H_


// PLDM types, defined in DMTF DSP0245 v1.2.0
typedef enum pldm_type {
  PLDM_TYPE_MSG_CTRL_AND_DISCOVERY       = 0,
  PLDM_TYPE_SMBIOS                       = 1,
  PLDM_TYPE_PLATFORM_MONITORING_AND_CTRL = 2,
  PLDM_TYPE_BIOS_CTRL_AND_CFG            = 3,
  PLDM_TYPE_FRU_DATA                     = 4,
  PLDM_TYPE_FIRMWARE_UPDATE              = 5,
} PldmType;


#define MAX_PLDM_PAYLOAD_SIZE 256
#define MAX_PLDM_RESPONSE_SIZE 256

typedef struct {
  uint16_t payload_size;
  uint32_t common;
  uint8_t  payload[MAX_PLDM_PAYLOAD_SIZE];
} pldm_cmd_req;

typedef struct {
  uint16_t payload_size;
  uint32_t common;
  uint8_t  response[MAX_PLDM_RESPONSE_SIZE];
} pldm_response;



int pldm_parse_fw_pkg(char *path);

#endif
