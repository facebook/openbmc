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

#include <libmctp-smbus.h>
#include <openbmc/ncsi.h>
#include <openbmc/pldm.h>

enum {
  MCTP_TYPE_CONTROL  = 0x0,
  MCTP_TYPE_PLDM     = 0x1,
  MCTP_TYPE_NCSI     = 0x2,
  MCTP_TYPE_ETHERNET = 0x3,
  MCTP_TYPE_NVME     = 0x4,
  MCTP_TYPE_SPDM     = 0x5,
};

/* mctp control msg response header */
struct mctp_ctrl_msg_hdr {
  uint8_t mctp_ctrl;
  uint8_t iid;
  uint8_t cmd;
  uint8_t comp_code;
} __attribute__((packed));

/* NC-SI structure */
struct ncsi_req_pkt {
  CTRL_MSG_HDR_t hdr;
  uint32_t chksum;
  uint8_t padding[26];
} __attribute__((packed));

struct mctp_ncsi_req {
  uint8_t Msg_Type;
  struct ncsi_req_pkt pkt;
} __attribute__((packed));

struct ncsi_rsp_pkt {
  CTRL_MSG_HDR_t hdr;
  NCSI_Response_Packet data;
} __attribute__((packed));

struct mctp_ncsi_rsp {
  uint8_t Msg_Type;
  struct ncsi_rsp_pkt pkt;
} __attribute__((packed));

#define NIC_SLAVE_ADDR 0x64

/* PLDM Definition */
#define PLDM_HDR_VER (0x0 << 6) // 2-bits

// Request Datagram Field
#define PLDM_REQUEST    (1 << 7)
#define PLDM_RESPONSE   (0 << 7)
#define PLDM_DATA_ASYNC (1 << 6)
#define PLDM_DATA_SYNC  (0 << 6)
#define PLDM_RSP_MSG   (PLDM_RESPONSE | PLDM_DATA_SYNC)
#define PLDM_REQ_MSG   (PLDM_REQUEST | PLDM_DATA_SYNC)
#define PLDM_REQ_ASYNC (PLDM_RESPONSE | PLDM_DATA_ASYNC)

// PLDM Struct
struct pldm_hdr {
  uint8_t RQD_IID;
  uint8_t Command_Type;
  uint8_t Command_Code;
} __attribute__((packed));

struct mctp_pldm_req_pkt {
  struct pldm_hdr hdr;
  uint8_t Payload_Data[MAX_PLDM_MSG_SIZE-PLDM_COMMON_REQ_LEN-1];
} __attribute__((packed));

struct mctp_pldm_req {
  uint8_t Msg_Type;
  struct mctp_pldm_req_pkt pkt;
} __attribute__((packed));

struct mctp_pldm_rsp_pkt {
  struct pldm_hdr hdr;
  uint8_t Complete_Code;
  uint8_t Payload_Data[MAX_PLDM_MSG_SIZE-PLDM_COMMON_RES_LEN-1];
} __attribute__((packed));

struct mctp_pldm_rsp {
  uint8_t Msg_Type;
  struct mctp_pldm_rsp_pkt pkt;
} __attribute__((packed));

// SPDM Struct
#define SPDM_MAX_PAYLOAD 4099


/* OBMC Binding */
struct obmc_mctp_binding {
  struct mctp *mctp;
  void *prot;
};

struct obmc_mctp_hdr {
  uint8_t flag_tag;
  uint8_t Msg_Size;
  uint8_t Msg_Type;
} __attribute__((packed));

struct obmc_mctp_spdm_hdr {
  uint8_t flag_tag;
  uint16_t Msg_Size;
  uint8_t Msg_Type;
} __attribute__((packed));

struct obmc_mctp_ncsi_rsp {
  struct obmc_mctp_hdr hdr;
  struct ncsi_rsp_pkt pkt;
} __attribute__((packed));

struct obmc_mctp_pldm_req {
  struct obmc_mctp_hdr hdr;
  struct mctp_pldm_req_pkt pkt;
} __attribute__((packed));

struct obmc_mctp_pldm_rsp {
  struct obmc_mctp_hdr hdr;
  struct mctp_pldm_rsp_pkt pkt;
} __attribute__((packed));

// Initialize libmctp core
struct obmc_mctp_binding* obmc_mctp_smbus_init(uint8_t bus, uint8_t src_addr, uint8_t dst_addr, uint8_t src_eid,
                                               int pkt_size);
void obmc_mctp_smbus_free(struct obmc_mctp_binding* binding);

// Debugging
int mctp_smbus_send_data(struct mctp* mctp, uint8_t dst, uint8_t flag_tag,
                         struct mctp_binding_smbus *smbus,
                         void *req, size_t size);

int mctp_smbus_recv_data_timeout_raw(struct mctp *mctp, uint8_t dst,
                                 struct mctp_binding_smbus *smbus,
                                 void *data, int TOsec);

int mctp_smbus_recv_data(struct mctp *mctp, uint8_t dst,
                         struct mctp_binding_smbus *smbus,
                         void *data);
// Command APIs
int obmc_mctp_clear_init_state(struct obmc_mctp_binding *binding, uint8_t dst_eid,
                               uint8_t tag, uint8_t iid);

int obmc_mctp_get_version_id(struct obmc_mctp_binding *binding, uint8_t dst_eid,
                             uint8_t tag, uint8_t iid,
                             Get_Version_ID_Response *payload);

int obmc_mctp_set_tid(struct obmc_mctp_binding *binding, uint8_t dst_eid,
                      uint8_t tag, uint8_t iid,
                      uint8_t tid);

int send_mctp_cmd(uint8_t bus, uint16_t src_addr, uint8_t dst_addr, uint8_t src_eid, uint8_t dst_eid,
                  uint8_t *tbuf, int tlen, uint8_t *rbuf, int *rlen);

int send_spdm_cmd(uint8_t bus, uint8_t dst_eid,
                  uint8_t *tbuf, int tlen,
                  uint8_t *rbuf, int *rlen);

int obmc_mctp_get_tid(struct obmc_mctp_binding *binding, uint8_t dst_eid,
                      uint8_t tag, uint8_t iid,
                      uint8_t *tid);

int obmc_mctp_fw_update(struct obmc_mctp_binding *binding, uint8_t dst_eid,
                        uint8_t tag, char *path);

int mctp_send_recv_w_mctpd(uint8_t bus, uint8_t eid, uint8_t* req_msg, size_t req_msg_len,
                            uint8_t **resp_msg, size_t *resp_msg_len);

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* _OBMC_MCTP_H_ */
