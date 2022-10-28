/*
 * ncsi-nvidia.c
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
#include "ncsi-nvidia.h"

const char *ncsi_nvidia_cmd_type_to_name(uint8_t cmd_type)
{
  switch (cmd_type) {
    case NCSI_NVIDIA_GET_DEBUG_DATA_CMD_ID:
      return "GET_DEBUG_DATA";
    default:
      return "UNKNOWN";
  }
}

int ncsi_nvidia_create_req_pkt(NCSI_NL_MSG_T *nl_msg, uint8_t ch, uint8_t cmd,
                               uint8_t para, uint16_t req_len, void *req)
{
  NCSI_Nvidia_Request_Hdr *req_hdr = req;

  req_hdr->manufacturer_id = htonl(MLX_IANA);
  req_hdr->nvidia_payload_ver = NCSI_NVIDIA_PAYLOAD_VERSION;
  req_hdr->nvidia_cmd_type = cmd;
  req_hdr->nvidia_para = para;

  return create_ncsi_ctrl_pkt(nl_msg, ch, NCSI_OEM_CMD, req_len, req);
}

int ncsi_nvidia_check_resp_type(NCSI_NL_RSP_T *nl_resp, uint8_t nvidia_cmd_type)
{
  NCSI_Nvidia_Response_Hdr *resp_hdr = ncsi_get_nvidia_resp(nl_resp);
  uint32_t oem_iana;

  if (nl_resp->hdr.cmd != NCSI_OEM_CMD) {
    ncsi_log(LOG_ERR, "Error: not ncsi oem response 0x%x", nl_resp->hdr.cmd);
    return -1;
  }

  oem_iana = ntohl(resp_hdr->manufacturer_id);
  if (oem_iana != MLX_IANA) {
    ncsi_log(LOG_ERR, "Error: bad ncsi oem manufacturer id 0x%x", oem_iana);
    return -1;
  }

  if (resp_hdr->nvidia_cmd_type != nvidia_cmd_type) {
    ncsi_log(LOG_ERR, "Error: bad ncsi nvidia cmd 0x%x, expect 0x%x",
          resp_hdr->nvidia_cmd_type, nvidia_cmd_type);
    return -1;
  }

  return 0;
}

int ncsi_nvidia_check_resp_len(NCSI_NL_RSP_T *nl_resp, uint16_t min_resp_size)
{
  NCSI_Nvidia_Response_Hdr *resp_hdr = ncsi_get_nvidia_resp(nl_resp);
  uint16_t nvidia_payload_len;

  if (nl_resp->hdr.payload_length < min_resp_size) {
    ncsi_log(LOG_ERR, "Error: ncsi response payload length %u too short for "
             "nvidia min size %u", nl_resp->hdr.payload_length, min_resp_size);
    return -1;
  }

  nvidia_payload_len = ntohs(resp_hdr->nvidia_payload_len);
  if (nl_resp->hdr.payload_length != sizeof(*resp_hdr) + 4 + nvidia_payload_len) {
    ncsi_log(LOG_ERR, "Error: ncsi payload len %u not match "
             "nvidia len (hdr %zu + payload %u)",
             (unsigned int)nl_resp->hdr.payload_length,
             sizeof(*resp_hdr) + 4,
             (unsigned int)nvidia_payload_len);
    return -1;
  }

  return 0;
}

int ncsi_nvidia_check_resp(NCSI_NL_RSP_T *nl_resp, uint8_t nvidia_cmd_type,
                         uint16_t min_resp_size)
{
  if (ncsi_nvidia_check_resp_type(nl_resp, nvidia_cmd_type)) {
    return -1;
  }

  if (ncsi_nvidia_check_resp_len(nl_resp, min_resp_size)) {
    return -1;
  }

  return 0;
}

void ncsi_nvidia_print_resp_nvidia_hdr(NCSI_NL_RSP_T *nl_resp)
{
  NCSI_Nvidia_Response_Hdr *resp_hdr = ncsi_get_nvidia_resp(nl_resp);
  uint32_t oem_iana = ntohl(resp_hdr->manufacturer_id);

  printf("IANA: %s(0x%x)\n",
         (oem_iana == MLX_IANA) ? "NVIDIA" : "Other", oem_iana);
  printf("Nvidia cmd: %s(0x%x)\n",
         ncsi_nvidia_cmd_type_to_name(resp_hdr->nvidia_cmd_type),
         resp_hdr->nvidia_cmd_type);
  printf("Nvidia payload length: %u\n", ntohs(resp_hdr->nvidia_payload_len));
  printf("Nvidia payload version: %u\n", resp_hdr->nvidia_payload_ver);
}

void ncsi_nvidia_print_resp(NCSI_NL_RSP_T *nl_resp)
{
  NCSI_Nvidia_Response_Hdr *resp_hdr = ncsi_get_nvidia_resp(nl_resp);

  print_ncsi_completion_codes(nl_resp);
  if (get_cmd_status(nl_resp) != RESP_COMMAND_COMPLETED)
    return;

  ncsi_nvidia_print_resp_nvidia_hdr(nl_resp);

  switch (resp_hdr->nvidia_para) {
  case NCSI_NVIDIA_GET_DEBUG_DATA_PARA:
    ncsi_nvidia_print_get_debug_data(nl_resp);
    break;
  default:
    print_ncsi_data(resp_hdr + 1, ntohs(resp_hdr->nvidia_payload_len), 64,
                    sizeof(CTRL_MSG_HDR_t) + sizeof(*resp_hdr));
    break;
  }
}

/*
 * GetDebugData functions
 */
const char *ncsi_nvidia_get_debug_type_to_name(int type)
{
  switch (type) {
    case NCSI_NVIDIA_GET_DEBUG_DATA_TYPE_DEV_DUMP:
      return "devdump";
    case NCSI_NVIDIA_GET_DEBUG_DATA_TYPE_FW_DUMP:
      return "fwdump";
    default:
      return "unknown";
  }
}

int ncsi_nvidia_check_get_debug_data(NCSI_NL_RSP_T *nl_resp, int debug_data_type,
                                   bool is_first __attribute__((unused)))
{
  NCSI_Nvidia_Get_Debug_Data_Response *gdd_resp = ncsi_get_nvidia_resp(nl_resp);
  int data_len = nl_resp->hdr.payload_length - sizeof(*gdd_resp);

  /* Check GetDebugData response data */
  gdd_resp = ncsi_get_nvidia_resp(nl_resp);
  if (ntohs(gdd_resp->header.nvidia_info_type) != debug_data_type) {
    ncsi_log(LOG_ERR, "Error: bad GetDebugData type 0x%x, expect 0x%x",
             ntohs(gdd_resp->header.nvidia_info_type), debug_data_type);
    return -1;
  }

  ncsi_log(LOG_ERR, "GetDebugData Start response data length = %u",  data_len);
  return 0;
}

void ncsi_nvidia_print_get_debug_data(NCSI_NL_RSP_T *nl_resp)
{
  NCSI_Nvidia_Get_Debug_Data_Response *gdd_resp = ncsi_get_nvidia_resp(nl_resp);
  int debug_data_len = ntohs(gdd_resp->header.nvidia_payload_len);

  printf("Data length: %u\n", debug_data_len);
  printf("Type: %s\n", ncsi_nvidia_get_debug_type_to_name(ntohs(gdd_resp->header.nvidia_info_type)));
  printf("Next handle: 0x%x\n", ntohl(gdd_resp->next_data_handle));
  printf("Debug data:\n");;
  print_ncsi_data(gdd_resp->debug_data, debug_data_len, 0 /* print all */,
                  sizeof(CTRL_MSG_HDR_t) + sizeof(*gdd_resp));
}
