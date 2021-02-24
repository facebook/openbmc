/*
 * decode.c
 *
 * Copyright 2015-present Facebook. All Rights Reserved.
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

#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <openbmc/obmc-mctp.h>
#include <openbmc/ncsi.h>
#include "decode.h"
#include "mctp-util.h"

// NCSI response code string
const char *ncsi_resp_string[RESP_MAX] = {
  "COMMAND_COMPLETED",
  "COMMAND_FAILED",
  "COMMAND_UNAVAILABLE",
  "COMMAND_UNSUPPORTED",
};

// NCSI reason code string
const char *ncsi_reason_string[NUM_NCSI_REASON_CODE] = {
  "NO_ERROR",
  "INTF_INIT_REQD",
  "PARAM_INVALID",
  "CHANNEL_NOT_RDY",
  "PKG_NOT_RDY",
  "INVALID_PAYLOAD_LEN",
  "INFO_NOT_AVAIL",
  "UNKNOWN_CMD_TYPE",
};

void print_raw_resp(uint8_t *rbuf, int rlen)
{
  int i = 0;
  for (i = 0; i<rlen; ++i)
    printf("%02x ", rbuf[i]);
  printf("\n");
}

static const char *
ncsi_cc_resp_name(int cc_resp)
{
  if ((cc_resp < 0) ||
      (cc_resp >= RESP_MAX) ||
      (ncsi_resp_string[cc_resp] == NULL)) {
    return "unknown_response";
  } else {
    return ncsi_resp_string[cc_resp];
  }
}

static const char *
ncsi_cc_reson_name(int cc_reason)
{
  switch (cc_reason) {
    case REASON_UNKNOWN_CMD_TYPE:
      return "UNKNOWN_CMD_TYPE";
    default:
      if ((cc_reason < 0) ||
          (cc_reason >= NUM_NCSI_REASON_CODE) ||
          (ncsi_reason_string[cc_reason] == NULL)) {
        return "unknown_reason";
      } else {
        return ncsi_reason_string[cc_reason];
      }
  }
}

static int parse_ncsi(uint8_t *rbuf, int rlen)
{
// min NC-SI response includes header + 2 bytes response code + 2 bytes reason code
#define MIN_NCSI_RESP_SIZE  (sizeof(CTRL_MSG_HDR_t) + 4)
  int i = 0;
  CTRL_MSG_HDR_t *phdr = NULL;
  NCSI_Response_Packet *pResp = NULL;

  if (rlen < (int) MIN_NCSI_RESP_SIZE || rbuf == NULL) {
    printf("Invalid NC-SI response length (%d) (min(%d))\n", rlen, MIN_NCSI_RESP_SIZE);
    return -1;
  }

  phdr = (CTRL_MSG_HDR_t *)rbuf;
  printf("NCSI control message\n");
  printf("  NCSI header:\n");
  printf("    MC ID(%02x), Header Revision(%02x), Rsv(%02x), IID(%02x), Packet Type(%02x), Channel(%02x), Payload Length(%02x)\n",
          phdr->MC_ID, phdr->Header_Revision, phdr->Reserved_1, phdr->IID,
          phdr->Command, phdr->Channel_ID, ntohs(phdr->Payload_Length));

  pResp = (NCSI_Response_Packet *)(rbuf + sizeof(CTRL_MSG_HDR_t));
  int cc_resp = ntohs(pResp->Response_Code);
  int cc_reason = ntohs(pResp->Reason_Code);
  printf("  Response: %s(0x%04x)\n", ncsi_cc_resp_name(cc_resp), cc_resp);
  printf("  Reason:   %s(0x%04x)\n", ncsi_cc_reson_name(cc_reason), cc_reason);
  printf("  Payload       : ");
  for (i = MIN_NCSI_RESP_SIZE; i < rlen; ++i)
    printf("%02x ", rbuf[i]);
  printf("\n");

  return 0;
}

static int parse_pldm(uint8_t *rbuf, int rlen)
{
  return 0;
}

static int parse_mctp_ctrl(uint8_t *rbuf, int rlen)
{
  int i, idx = 0;

  if (rlen < 3 || rbuf == NULL) {
    printf("Invalid control msg length (%d)\n", rlen);
    return -1;
  }

  printf("MCTP control message\n");
  printf("  Rq[7],D[6],IID[4:0]: %02x\n", rbuf[idx++]);
  printf("  Command Code:        %02x\n", rbuf[idx++]);
  printf("  Completion Code:     %02x\n", rbuf[idx++]);
  printf("  response data:       ");
  for (i = idx; i < rlen; ++i)
    printf("%02x ", rbuf[i]);
  printf("\n");
  return 0;
}

int print_parsed_resp(uint8_t *rbuf, int rlen)
{
  uint8_t msg_type = 0;
  int ret = -1;

  if (rlen < 3 || rbuf == NULL)
    return -1;

  msg_type = rbuf[2];
  printf("\nMCTP transport header: src_eid(%02x) tag(%02x)\n", rbuf[0], rbuf[1]);
  printf("MCTP header:           msg_type(%02x)\n\n", msg_type);

  rbuf += 3;
  rlen -= 3;
  switch (msg_type) {
  case MCTP_TYPE_CONTROL:
          ret = parse_mctp_ctrl(rbuf, rlen);
          break;
  case MCTP_TYPE_PLDM:
          ret = parse_pldm(rbuf, rlen);
          break;
  case MCTP_TYPE_NCSI:
          ret = parse_ncsi(rbuf, rlen);
          break;
  case MCTP_TYPE_ETHERNET:
  case MCTP_TYPE_NVME:
  case MCTP_TYPE_SPDM:
  default:
          printf("unknown MCTP type %x\n", msg_type);
          break;
  }

  return ret;
}
