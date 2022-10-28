/*
 * ncsi-brcm.h
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
#ifndef _NCSI_BRCM_H_
#define _NCSI_BRCM_H_

#include <stdint.h>
#include <stdbool.h>
#include "ncsi.h"

/*
 * Broadcom OEM request and response header
 */
typedef struct {
  uint32_t manufacturer_id;
  uint8_t  brcm_payload_ver;
  uint8_t  brcm_cmd_type;
  uint16_t brcm_payload_len;
  uint32_t reserved;
} __attribute__((packed)) NCSI_Brcm_Request_Hdr;

typedef struct {
  uint16_t response_code;
  uint16_t reason_code;
  uint32_t manufacturer_id;
  uint8_t  brcm_payload_ver;
  uint8_t  brcm_cmd_type;
  uint16_t brcm_payload_len;
} __attribute__((packed)) NCSI_Brcm_Response_Hdr;

/* Broadcom OEM payload version */
#define NCSI_BRCM_PAYLOAD_VERSION       0

/*
 * Broadcom OEM command types
 */
#define NCSI_BRCM_GET_DEBUG_DATA        0x30

/*
 * GetDebugData
 */
typedef struct {
  NCSI_Brcm_Request_Hdr   header;
  uint32_t                data_handle;
  uint8_t                 reserved1;
  uint8_t                 op;
  uint16_t                type;
} __attribute__((packed)) NCSI_Brcm_Get_Debug_Data_Request;

typedef struct {
  NCSI_Brcm_Response_Hdr  header;
  uint32_t                next_data_handle;
  uint8_t                 reserved;
  uint8_t                 op;
  uint16_t                type;
  uint8_t                 debug_data[]; /* data of variable length */
} __attribute__((packed)) NCSI_Brcm_Get_Debug_Data_Response;

#define NCSI_BRCM_GET_DEBUG_DATA_TYPE_CORE_DUMP     0
#define NCSI_BRCM_GET_DEBUG_DATA_TYPE_CRASH_DUMP    1

#define NCSI_BRCM_GET_DEBUG_DATA_REQ_OP_MASK        0xf
#define NCSI_BRCM_GET_DEBUG_DATA_REQ_OP_GET_NEXT    0
#define NCSI_BRCM_GET_DEBUG_DATA_REQ_OP_START_XFER  1

#define NCSI_BRCM_GET_DEBUG_DATA_RESP_OP_MASK       0xf
#define NCSI_BRCM_GET_DEBUG_DATA_RESP_OP_START      0x1
#define NCSI_BRCM_GET_DEBUG_DATA_RESP_OP_MIDDLE     0x2
#define NCSI_BRCM_GET_DEBUG_DATA_RESP_OP_END        0x4


/* Broadcom OEM response structure payload size */
#define NCSI_BRCM_RESP_STRUCT_PAYLOAD_SIZE(brcm_resp) \
  (sizeof(*(brcm_resp)) - sizeof(NCSI_Brcm_Response_Hdr))

const char *ncsi_brcm_cmd_type_to_name(uint8_t cmd);

int ncsi_brcm_create_req_pkt(NCSI_NL_MSG_T *nl_msg, uint8_t ch, uint8_t cmd,
                             uint16_t req_len, void *req);

/* Get Broadcom OEM response from NC-SI response message */
static inline void *ncsi_get_brcm_resp(NCSI_NL_RSP_T *nl_resp)
{
  return nl_resp->msg_payload;
}

int ncsi_brcm_check_resp_type(NCSI_NL_RSP_T *nl_resp, uint8_t brcm_cmd_type);
int ncsi_brcm_check_resp_len(NCSI_NL_RSP_T *nl_resp, uint16_t min_resp_size);
int ncsi_brcm_check_resp(NCSI_NL_RSP_T *nl_resp, uint8_t brcm_cmd_type,
                         uint16_t min_resp_size);
void ncsi_brcm_print_resp_brcm_hdr(NCSI_NL_RSP_T *nl_resp);
void ncsi_brcm_print_resp(NCSI_NL_RSP_T *nl_resp);

const char *ncsi_brcm_get_debug_type_to_name(int type);
int ncsi_brcm_check_get_debug_data(NCSI_NL_RSP_T *nl_resp, int debug_data_type,
                                   bool is_first);
void ncsi_brcm_print_get_debug_data(NCSI_NL_RSP_T *nl_resp);

#endif /* _NCSI_BRCM_H_ */
