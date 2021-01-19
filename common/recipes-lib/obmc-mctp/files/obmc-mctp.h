/*
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

#ifndef _OBMC_MCTP_H_
#define _OBMC_MCTP_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <openbmc/ncsi.h>

struct ncsi_cmd_pkt {
  CTRL_MSG_HDR_t hdr;
  uint32_t chksum;
  uint8_t padding[26];
} __attribute__((packed));

struct mctp_ncsi_cmd {
  uint8_t msg_type;
  struct ncsi_cmd_pkt pkt;
} __attribute__((packed));

struct ncsi_rsp_pkt {
  CTRL_MSG_HDR_t hdr;
  NCSI_Response_Packet data;
} __attribute__((packed));

struct mctp_ncsi_rsp {
  uint8_t msg_type;
  struct ncsi_rsp_pkt pkt;
} __attribute__((packed));

struct obmc_mctp_binding {
  struct mctp *mctp;
  void *prot;
};

// Initialize libmctp core
struct obmc_mctp_binding* obmc_mctp_smbus_init(uint8_t, uint8_t, uint8_t);
void obmc_mctp_smbus_free(struct obmc_mctp_binding*);

// Command APIs
int obmc_mctp_clear_init_state(struct obmc_mctp_binding*, uint8_t,
                               uint8_t, uint8_t);

int obmc_mctp_get_version_id(struct obmc_mctp_binding*, uint8_t,
                             uint8_t, uint8_t,
                             Get_Version_ID_Response*);
#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* _OBMC_MCTP_H_ */
