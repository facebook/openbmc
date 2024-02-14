/*
 * ncsi-meta.c
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

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <syslog.h>
#include <arpa/inet.h>
#include "ncsi.h"
#include "ncsi-meta.h"

const char *ncsi_meta_cmd_type_to_name(uint8_t cmd_type)
{
  switch (cmd_type) {
    case NCSI_META_GET_DEBUG_DATA:
      return "GET_DEBUG_DATA";
    default:
      return "UNKNOWN";
  }
}

int ncsi_meta_create_req_pkt(NCSI_NL_MSG_T *nl_msg, uint8_t ch, uint8_t cmd,
                             uint16_t req_len, void *req)
{
  NCSI_Meta_Request_Hdr *req_hdr = req;

  req_hdr->manufacturer_id = htonl(META_IANA);
  req_hdr->meta_payload_ver = NCSI_META_PAYLOAD_VERSION;
  req_hdr->meta_cmd_type = cmd;
  req_hdr->meta_payload_len = htons(req_len - sizeof(*req_hdr));
  req_hdr->reserved = 0;

  return create_ncsi_ctrl_pkt(nl_msg, ch, NCSI_OEM_CMD, req_len, req);
}

int ncsi_meta_check_resp_type(NCSI_NL_RSP_T *nl_resp, uint8_t meta_cmd_type)
{
  NCSI_Meta_Response_Hdr *resp_hdr = ncsi_get_meta_resp(nl_resp);
  uint32_t oem_iana;

  if (nl_resp->hdr.cmd != NCSI_OEM_CMD) {
    ncsi_log(LOG_ERR, "Error: not ncsi oem response 0x%x", nl_resp->hdr.cmd);
    return -1;
  }

  oem_iana = ntohl(resp_hdr->manufacturer_id);
  if (oem_iana != META_IANA) {
    ncsi_log(LOG_ERR, "Error: bad ncsi oem manufacturer id 0x%x", oem_iana);
    return -1;
  }

  if (resp_hdr->meta_cmd_type != meta_cmd_type) {
    ncsi_log(LOG_ERR, "Error: bad ncsi meta cmd 0x%x, expect 0x%x",
          resp_hdr->meta_cmd_type, meta_cmd_type);
    return -1;
  }

  return 0;
}

int ncsi_meta_check_resp_len(NCSI_NL_RSP_T *nl_resp, uint16_t min_resp_size)
{
  NCSI_Meta_Response_Hdr *resp_hdr = ncsi_get_meta_resp(nl_resp);
  uint16_t meta_payload_len;

  if (nl_resp->hdr.payload_length < min_resp_size) {
    ncsi_log(LOG_ERR, "Error: ncsi response payload length %u too short for "
             "meta min size %u", nl_resp->hdr.payload_length, min_resp_size);
    return -1;
  }

  meta_payload_len = ntohs(resp_hdr->meta_payload_len);
  if (nl_resp->hdr.payload_length != sizeof(*resp_hdr) + meta_payload_len) {
    ncsi_log(LOG_ERR, "Error: ncsi payload len %u not match "
             "meta len (hdr %zu + payload %u)",
             (unsigned int)nl_resp->hdr.payload_length,
             sizeof(*resp_hdr),
             (unsigned int)meta_payload_len);
    return -1;
  }

  return 0;
}

int ncsi_meta_check_resp(NCSI_NL_RSP_T *nl_resp, uint8_t meta_cmd_type,
                         uint16_t min_resp_size)
{
  if (ncsi_meta_check_resp_type(nl_resp, meta_cmd_type)) {
    return -1;
  }

  if (ncsi_meta_check_resp_len(nl_resp, min_resp_size)) {
    return -1;
  }

  return 0;
}

void ncsi_meta_print_resp_meta_hdr(NCSI_NL_RSP_T *nl_resp)
{
  NCSI_Meta_Response_Hdr *resp_hdr = ncsi_get_meta_resp(nl_resp);
  uint32_t oem_iana = ntohl(resp_hdr->manufacturer_id);

  printf("IANA: %s(0x%x)\n",
         (oem_iana == META_IANA) ? "Meta" : "Other", oem_iana);
  printf("Meta cmd: %s(0x%x)\n",
         ncsi_meta_cmd_type_to_name(resp_hdr->meta_cmd_type),
         resp_hdr->meta_cmd_type);
  printf("Meta payload length: %u\n", ntohs(resp_hdr->meta_payload_len));
  printf("Meta payload version: %u\n", resp_hdr->meta_payload_ver);
}

void ncsi_meta_print_resp(NCSI_NL_RSP_T *nl_resp)
{
  NCSI_Meta_Response_Hdr *resp_hdr = ncsi_get_meta_resp(nl_resp);

  print_ncsi_completion_codes(nl_resp);
  if (get_cmd_status(nl_resp) != RESP_COMMAND_COMPLETED)
    return;

  ncsi_meta_print_resp_meta_hdr(nl_resp);

  switch (resp_hdr->meta_cmd_type) {
  case NCSI_META_GET_DEBUG_DATA:
    ncsi_meta_print_get_debug_data(nl_resp);
    break;
  default:
    print_ncsi_data(resp_hdr + 1, ntohs(resp_hdr->meta_payload_len), 64,
                    sizeof(CTRL_MSG_HDR_t) + sizeof(*resp_hdr));
    break;
  }
}

/*
 * GetDebugData functions
 */
const char *ncsi_meta_get_debug_type_to_name(int type)
{
  switch (type) {
    case NCSI_META_GET_DEBUG_DATA_TYPE_CORE_DUMP:
      return "coredump";
    case NCSI_META_GET_DEBUG_DATA_TYPE_CRASH_DUMP:
      return "crashdump";
    default:
      return "unknown";
  }
}

int ncsi_meta_check_get_debug_data(NCSI_NL_RSP_T *nl_resp, int debug_data_type,
                                   bool is_first)
{
  NCSI_Meta_Get_Debug_Data_Response *gdd_resp = ncsi_get_meta_resp(nl_resp);
  size_t data_len = nl_resp->hdr.payload_length - sizeof(*gdd_resp);

  /* Check GetDebugData response data */
  gdd_resp = ncsi_get_meta_resp(nl_resp);
  if (ntohs(gdd_resp->type) != debug_data_type) {
    ncsi_log(LOG_ERR, "Error: bad GetDebugData type 0x%x, expect 0x%x",
             ntohs(gdd_resp->type), debug_data_type);
    return -1;
  }


  if (is_first) {
    /* First response must have op of star not middle */
    if (!(gdd_resp->op & NCSI_META_GET_DEBUG_DATA_RESP_OP_START) ||
        (gdd_resp->op & NCSI_META_GET_DEBUG_DATA_RESP_OP_MIDDLE)) {
      ncsi_log(LOG_ERR, "Error: bad GetDebugData Start response op 0x%x",
               gdd_resp->op);
      return -1;
    }

    /* First response must contain debug data length word */
    if (data_len < sizeof(uint32_t)) {
      ncsi_log(LOG_ERR, "Error: GetDebugData Start response data too short (%zu)"
               ,  data_len);
      return -1;
    }

    /* Exclude debug data length */
    data_len -= sizeof(uint32_t);

  } else {
      /* Next response must has op of middle or end not start */
    if ((gdd_resp->op & NCSI_META_GET_DEBUG_DATA_RESP_OP_START) ||
        !(gdd_resp->op & (NCSI_META_GET_DEBUG_DATA_RESP_OP_MIDDLE |
                          NCSI_META_GET_DEBUG_DATA_RESP_OP_END))) {
      ncsi_log(LOG_ERR, "Error: bad GetDebugData GetNext response op 0x%x",
               gdd_resp->op);
      return -1;
    }
  }

  if (gdd_resp->op & NCSI_META_GET_DEBUG_DATA_RESP_OP_END) {
    /* Last response must have CRC32 checksum word */
    if (data_len < sizeof(uint32_t)) {
      ncsi_log(LOG_ERR, "Error: GetDebugData End response data too short (%zu)",
               data_len);
      return -1;
    }

    /* Exclude checksum */
    data_len -= sizeof(uint32_t);
  }

  return 0;
}

void ncsi_meta_print_get_debug_data(NCSI_NL_RSP_T *nl_resp)
{
  NCSI_Meta_Get_Debug_Data_Response *gdd_resp = ncsi_get_meta_resp(nl_resp);
  int debug_data_len = ntohs(gdd_resp->header.meta_payload_len)
                        - NCSI_META_RESP_STRUCT_PAYLOAD_SIZE(gdd_resp);

  printf("Data length: %d\n", debug_data_len);
  printf("Type: %s\n", ncsi_meta_get_debug_type_to_name(ntohs(gdd_resp->type)));
  printf("Op:");
  if (gdd_resp->op & NCSI_META_GET_DEBUG_DATA_RESP_OP_START) {
    printf(" start");
  }
  if (gdd_resp->op & NCSI_META_GET_DEBUG_DATA_RESP_OP_MIDDLE) {
    printf(" middle");
  }
  if (gdd_resp->op & NCSI_META_GET_DEBUG_DATA_RESP_OP_END) {
    printf(" end");
  }
  printf("\n");
  printf("Next handle: 0x%x\n", ntohl(gdd_resp->next_data_handle));
  printf("Debug data:\n");
  print_ncsi_data(gdd_resp->debug_data, debug_data_len, 32 /* max print num */,
                  sizeof(CTRL_MSG_HDR_t) + sizeof(*gdd_resp));
}
