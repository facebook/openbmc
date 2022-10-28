/*
 * ncsi-nvidia.h
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
 */
#ifndef _NCSI_NVIDIA_H_
#define _NCSI_NVIDIA_H_

#include <stdint.h>
#include <stdbool.h>
#include "ncsi.h"

/*
 * NVIDIA OEM request and response header
 */
typedef struct {
  uint32_t manufacturer_id;
  uint8_t  nvidia_payload_ver;
  uint8_t  nvidia_cmd_type;
  uint8_t  nvidia_para;
  uint8_t  nvidia_info_type;
} __attribute__((packed)) NCSI_Nvidia_Request_Hdr;

typedef struct {
  uint16_t response_code;
  uint16_t reason_code;
  uint32_t manufacturer_id;
  uint8_t  nvidia_payload_ver;
  uint8_t  nvidia_cmd_type;
  uint8_t  nvidia_para;
  uint8_t  reserved1;
  uint16_t nvidia_payload_len;
  uint8_t  reserved2;
  uint8_t  nvidia_info_type;
} __attribute__((packed)) NCSI_Nvidia_Response_Hdr;

/* NVIDIA OEM payload version */
#define NCSI_NVIDIA_PAYLOAD_VERSION       0

/*
 * NVIDIA OEM command types
 */
#define NCSI_NVIDIA_GET_DEBUG_DATA_CMD_ID      0x0
#define NCSI_NVIDIA_GET_DEBUG_DATA_PARA        0x32
#define NCSI_NVIDIA_GET_DEBUG_DATA_CHANNEL_ID  0x1f

/*
 * GetDebugData
 */
typedef struct {
  NCSI_Nvidia_Request_Hdr header;
  uint32_t                data_handle;
} __attribute__((packed)) NCSI_Nvidia_Get_Debug_Data_Request;

typedef struct {
  NCSI_Nvidia_Response_Hdr  header;
  uint32_t                  next_data_handle;
  uint8_t                   debug_data[];
} __attribute__((packed)) NCSI_Nvidia_Get_Debug_Data_Response;

#define NCSI_NVIDIA_GET_DEBUG_DATA_TYPE_DEV_DUMP     0
#define NCSI_NVIDIA_GET_DEBUG_DATA_TYPE_FW_DUMP     1

const char *ncsi_nvidia_cmd_type_to_name(uint8_t cmd);

int ncsi_nvidia_create_req_pkt(NCSI_NL_MSG_T *nl_msg, uint8_t ch, uint8_t cmd,
                             uint8_t para, uint16_t req_len, void *req);

// /* Get NVIDIA OEM response from NC-SI response message */
static inline void *ncsi_get_nvidia_resp(NCSI_NL_RSP_T *nl_resp)
{
  return nl_resp->msg_payload;
}

int ncsi_nvidia_check_resp_type(NCSI_NL_RSP_T *nl_resp, uint8_t nvidia_cmd_type);
int ncsi_nvidia_check_resp_len(NCSI_NL_RSP_T *nl_resp, uint16_t min_resp_size);
int ncsi_nvidia_check_resp(NCSI_NL_RSP_T *nl_resp, uint8_t nvidia_cmd_type,
                         uint16_t min_resp_size);
void ncsi_nvidia_print_resp_nvidia_hdr(NCSI_NL_RSP_T *nl_resp);
void ncsi_nvidia_print_resp(NCSI_NL_RSP_T *nl_resp);

const char *ncsi_nvidia_get_debug_type_to_name(int type);
int ncsi_nvidia_check_get_debug_data(NCSI_NL_RSP_T *nl_resp, int debug_data_type,
                                   bool is_first);
void ncsi_nvidia_print_get_debug_data(NCSI_NL_RSP_T *nl_resp);

#endif /* _NCSI_NVIDIA_H_ */
