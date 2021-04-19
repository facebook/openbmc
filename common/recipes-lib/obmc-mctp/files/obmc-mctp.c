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

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <syslog.h>
#include <libmctp-alloc.h>
#include <libmctp-log.h>
#include "obmc-mctp.h"

//#define DEBUG
#define SYSFS_SLAVE_QUEUE "/sys/bus/i2c/devices/%d-10%02x/slave-mqueue"
#define NIC_SLAVE_ADDR 0x64

struct mctp_smbus_pkt_private *smbus_extra_params = NULL;

// TODO:
//      Migrate this library to C++ if BMC need to support MCTP over PCIe

/*
 * This rx_handler is the callback function which is registered by mctp_set_rx_all()
 * if we received a request/response by calling mctp_smbus_read().
 * It should be useful to reply a basic request if we are not the bus owner.
 */
static void rx_handler(uint8_t eid, void *data, void *msg, size_t len,
                       bool tag_owner, uint8_t tag, void *prv)
{
  // TODO:
  //    We should send device the response according to the EID
  struct obmc_mctp_hdr *p = (struct obmc_mctp_hdr *)data;

  if (tag_owner)
    p->flag_tag |= MCTP_HDR_FLAG_TO;
  MCTP_HDR_SET_TAG(p->flag_tag, tag);
  p->Msg_Size = len;
  memcpy(&p->Msg_Type, msg, len);
}

struct obmc_mctp_binding* obmc_mctp_smbus_init(uint8_t bus, uint8_t addr, uint8_t src_eid,
                                               int pkt_size)
{
  int fd;
  char dev[64] = {0};
  char slave_queue[64] = {0};
  struct mctp *mctp;
  struct mctp_binding_smbus *smbus;
  struct obmc_mctp_binding *mctp_binding;

  mctp_binding = (struct obmc_mctp_binding *)malloc(sizeof(struct obmc_mctp_binding));
  if (mctp_binding == NULL) {
    syslog(LOG_ERR, "%s: out of memory", __func__);
    return NULL;
  }

  if (pkt_size < MCTP_PAYLOAD_SIZE + MCTP_HEADER_SIZE)
    pkt_size = MCTP_PAYLOAD_SIZE + MCTP_HEADER_SIZE;
  mctp_smbus_set_pkt_size(pkt_size);

  mctp = mctp_init();
  smbus = mctp_smbus_init();
  if (mctp == NULL || smbus == NULL || mctp_smbus_register_bus(smbus, mctp, src_eid) < 0) {
    syslog(LOG_ERR, "%s: MCTP init failed", __func__);
    goto bail;
  }

  smbus_extra_params = (struct mctp_smbus_pkt_private *)
                       malloc(sizeof(struct mctp_smbus_pkt_private));
  if (smbus_extra_params == NULL) {
    syslog(LOG_ERR, "%s: out of memory", __func__);
    goto bail;
  }

  snprintf(dev, sizeof(dev), "/dev/i2c-%d", bus);
  fd = open(dev, O_RDWR);
  if (fd < 0) {
    syslog(LOG_ERR, "%s: open %s failed", __func__, dev);
    goto bail;
  }
  smbus_extra_params->mux_hold_timeout = 0;
  smbus_extra_params->mux_flags = 0;
  smbus_extra_params->fd = fd;
  smbus_extra_params->slave_addr = NIC_SLAVE_ADDR;

  snprintf(slave_queue, sizeof(slave_queue), SYSFS_SLAVE_QUEUE, bus, addr);
  fd = open(slave_queue, O_RDONLY);
  if (fd < 0) {
    syslog(LOG_ERR, "%s: open %s failed", __func__, slave_queue);
    goto bail;
  }
  mctp_smbus_set_in_fd(smbus, fd);

#ifdef DEBUG
  mctp_set_log_stdio(MCTP_LOG_DEBUG);
  mctp_set_tracing_enabled(true);
#endif

  mctp_binding->mctp = mctp;
  mctp_binding->prot = (void *)smbus;
  return mctp_binding;
bail:
  obmc_mctp_smbus_free(mctp_binding);
  return NULL;
}

void obmc_mctp_smbus_free(struct obmc_mctp_binding* binding)
{
  mctp_smbus_free(binding->prot);
  mctp_destroy(binding->mctp);
  free(binding);
  free(smbus_extra_params);
}

int mctp_smbus_send_data(struct mctp* mctp, uint8_t dst, uint8_t flag_tag,
                         struct mctp_binding_smbus *smbus,
                         void *req, size_t size)
{
  bool tag_owner = flag_tag & MCTP_HDR_FLAG_TO? true: false;
  uint8_t tag = MCTP_HDR_GET_TAG(flag_tag);
  // TODO:
  //    Function overloading

  if (mctp_message_tx(mctp, dst, req, size,
                      tag_owner, tag, smbus_extra_params) < 0) {
    syslog(LOG_ERR, "%s: MCTP TX error", __func__);
    return -1;
  }
  return 0;
}

// reads MCTP response off smbus
// decodes response, returns NC-SI over MCTP or PLDM over MCTP status,
// returns error for other MCTP msg types
int mctp_smbus_recv_data_timeout(struct mctp *mctp, uint8_t dst,
                                 struct mctp_binding_smbus *smbus,
                                 void *data, int TOsec)
{
  // TODO:
  //    Function overloading
  int retry = 6*10; // Default 6 secs
  struct obmc_mctp_hdr *p = (struct obmc_mctp_hdr *)data;

  mctp_set_rx_all(mctp, rx_handler, data);

  if (TOsec > 0)
    retry = TOsec*10;

  while (retry--) {
    usleep(100*1000);

    if (mctp_smbus_read(smbus) < 0) {
      syslog(LOG_ERR, "%s: MCTP RX error", __func__);
      return -1;
    }

    if (p->Msg_Size > 0) {
      if (p->Msg_Type == MCTP_TYPE_NCSI) {
        struct obmc_mctp_ncsi_rsp *tmp = (struct obmc_mctp_ncsi_rsp *)data;
        return tmp->pkt.data.Response_Code;

      } else if (p->Msg_Type == MCTP_TYPE_PLDM) {
        struct obmc_mctp_pldm_rsp *tmp = (struct obmc_mctp_pldm_rsp *)data;
        if (tmp->hdr.flag_tag & MCTP_HDR_FLAG_TO) {
          // Request comes from device, just return CC_SUCCESS
          return CC_SUCCESS;
        } else {
          return tmp->pkt.Complete_Code;
        }
      } else {
        syslog(LOG_ERR, "%s: Unknown message (0x%02X)", __func__, p->Msg_Type);
        return -1;
      }
    }

  }
  syslog(LOG_ERR, "%s: MCTP timeout", __func__);
  return -1;
}

// reads MCTP response off smbus
// return byte count, do not interpret data (e.g. MCTP msg type)
int mctp_smbus_recv_data_timeout_raw(struct mctp *mctp, uint8_t dst,
                                 struct mctp_binding_smbus *smbus,
                                 void *data, int TOsec)
{
  int retry = 6*10; // Default 6 secs
  struct obmc_mctp_hdr *p = (struct obmc_mctp_hdr *)data;

  mctp_set_rx_all(mctp, rx_handler, data);

  if (TOsec > 0)
    retry = TOsec*10;

  while (retry--) {
    usleep(100*1000);

    if (mctp_smbus_read(smbus) < 0) {
      syslog(LOG_ERR, "%s: MCTP RX error", __func__);
      return -1;
    }

    if (p->Msg_Size > 0) {
      return p->Msg_Size;
    }
  }
  syslog(LOG_ERR, "%s: MCTP timeout", __func__);
  return -1;
}

int mctp_smbus_recv_data(struct mctp *mctp, uint8_t dst,
                         struct mctp_binding_smbus *smbus,
                         void *data)
{
  return mctp_smbus_recv_data_timeout(mctp, dst, smbus, data, -1);
}

int obmc_mctp_clear_init_state(struct obmc_mctp_binding *binding, uint8_t dst_eid,
                               uint8_t tag, uint8_t iid)
{
  int ret;
  struct mctp *mctp = binding->mctp;
  struct mctp_binding_smbus *smbus = (struct mctp_binding_smbus *)binding->prot;
  struct mctp_ncsi_req req = {0};
  struct obmc_mctp_ncsi_rsp rsp = {0};

  /* NC-SI: Clear init state */
  tag |= MCTP_HDR_FLAG_TO;
  req.Msg_Type                = MCTP_TYPE_NCSI;
  req.pkt.hdr.MC_ID           = 0x00;
  req.pkt.hdr.Header_Revision = 0x01;
  req.pkt.hdr.IID             = iid;
  req.pkt.hdr.Command         = NCSI_CLEAR_INITIAL_STATE;
  req.pkt.hdr.Channel_ID      = 0x00;
  req.pkt.hdr.Payload_Length  = 0x00;

  ret = mctp_smbus_send_data(mctp, dst_eid, tag, smbus, &req, sizeof(req));
  if (ret < 0)
    return -1;

  ret = mctp_smbus_recv_data(mctp, dst_eid, smbus, &rsp);
  if (ret != RESP_COMMAND_COMPLETED) {
    syslog(LOG_ERR, "%s: Response code = 0x%02X, Reason code = 0x%02X",
                    __func__, rsp.pkt.data.Response_Code, rsp.pkt.data.Reason_Code);
    return -1;
  }
  return 0;
}

int obmc_mctp_get_version_id(struct obmc_mctp_binding *binding, uint8_t dst_eid,
                             uint8_t tag, uint8_t iid,
                             Get_Version_ID_Response *payload)
{
  int ret = -1;
  struct mctp *mctp = binding->mctp;
  struct mctp_binding_smbus *smbus = (struct mctp_binding_smbus *)binding->prot;
  struct mctp_ncsi_req req = {0};
  struct obmc_mctp_ncsi_rsp rsp = {0};

  /* NC-SI: Get version */
  tag |= MCTP_HDR_FLAG_TO;
  req.Msg_Type                = MCTP_TYPE_NCSI;
  req.pkt.hdr.MC_ID           = 0x00;
  req.pkt.hdr.Header_Revision = 0x01;
  req.pkt.hdr.IID             = iid;
  req.pkt.hdr.Command         = NCSI_GET_VERSION_ID;
  req.pkt.hdr.Channel_ID      = 0x00;
  req.pkt.hdr.Payload_Length  = 0x00;

  ret = mctp_smbus_send_data(mctp, dst_eid, tag, smbus, &req, sizeof(req));
  if (ret < 0)
    return -1;

  ret = mctp_smbus_recv_data(mctp, dst_eid, smbus, &rsp);
  if (ret != RESP_COMMAND_COMPLETED) {
    syslog(LOG_ERR, "%s: Response code = 0x%02X, Reason code = 0x%02X",
                    __func__, rsp.pkt.data.Response_Code, rsp.pkt.data.Reason_Code);
    return -1;
  }
  memcpy(payload, rsp.pkt.data.Payload_Data, sizeof(Get_Version_ID_Response));
  return 0;
}

int obmc_mctp_set_tid(struct obmc_mctp_binding *binding, uint8_t dst_eid,
                      uint8_t tag, uint8_t iid,
                      uint8_t tid)
{
  int ret = -1;
  struct mctp *mctp = binding->mctp;
  struct mctp_binding_smbus *smbus = (struct mctp_binding_smbus *)binding->prot;
  struct mctp_pldm_req req = {0};
  struct obmc_mctp_pldm_rsp rsp = {0};
  size_t req_size = 1 + PLDM_COMMON_REQ_LEN + sizeof(tid);

  /* PLDM: Set TID */
  tag |= MCTP_HDR_FLAG_TO;
  req.Msg_Type             = MCTP_TYPE_PLDM;
  req.pkt.hdr.RQD_IID      = iid | PLDM_REQ_MSG;
  req.pkt.hdr.Command_Type = PLDM_HDR_VER | PLDM_TYPE_MSG_CTRL_AND_DISCOVERY;
  req.pkt.hdr.Command_Code = CMD_SET_TID;
  req.pkt.Payload_Data[0]  = tid;

  ret = mctp_smbus_send_data(mctp, dst_eid, tag, smbus, &req, req_size);
  if (ret < 0)
    return -1;

  ret = mctp_smbus_recv_data(mctp, dst_eid, smbus, &rsp);
  if (ret != CC_SUCCESS) {
    syslog(LOG_ERR, "%s: Complete code = 0x%02X", __func__, rsp.pkt.Complete_Code);
    return -1;
  }
  return 0;
}

int obmc_mctp_get_tid(struct obmc_mctp_binding *binding, uint8_t dst_eid,
                      uint8_t tag, uint8_t iid,
                      uint8_t *tid)
{
  int ret = -1;
  struct mctp *mctp = binding->mctp;
  struct mctp_binding_smbus *smbus = (struct mctp_binding_smbus *)binding->prot;
  struct mctp_pldm_req req = {0};
  struct obmc_mctp_pldm_rsp rsp = {0};
  size_t req_size = 1 + PLDM_COMMON_REQ_LEN;

  /* PLDM: Get TID */
  tag |= MCTP_HDR_FLAG_TO;
  req.Msg_Type             = MCTP_TYPE_PLDM;
  req.pkt.hdr.RQD_IID      = iid | PLDM_REQ_MSG;
  req.pkt.hdr.Command_Type = PLDM_HDR_VER | PLDM_TYPE_MSG_CTRL_AND_DISCOVERY;
  req.pkt.hdr.Command_Code = CMD_GET_TID;

  ret = mctp_smbus_send_data(mctp, dst_eid, tag, smbus, &req, req_size);
  if (ret < 0)
    return -1;

  ret = mctp_smbus_recv_data(mctp, dst_eid, smbus, &rsp);
  if (ret != CC_SUCCESS) {
    syslog(LOG_ERR, "%s: Complete code = 0x%02X", __func__, rsp.pkt.Complete_Code);
    return -1;
  }
  *tid = rsp.pkt.Payload_Data[0];
  return 0;
}

int send_mctp_cmd(uint8_t bus, uint16_t addr, uint8_t src_eid, uint8_t dst_eid,
                  uint8_t *tbuf, int tlen, uint8_t *rbuf, int *rlen)
{
  int ret = -1;
  uint8_t tag = 0;
  struct obmc_mctp_binding *mctp_binding;
  struct mctp_binding_smbus *smbus;

  mctp_binding = obmc_mctp_smbus_init(bus, addr, src_eid, NCSI_MAX_PAYLOAD);
  if (mctp_binding == NULL) {
    syslog(LOG_ERR, "%s: Error: mctp binding failed", __func__);
    return -1;
  }
  smbus = (struct mctp_binding_smbus *)mctp_binding->prot;

  tag |= MCTP_HDR_FLAG_TO;
  ret = mctp_smbus_send_data(mctp_binding->mctp, dst_eid, tag, smbus, tbuf, tlen);
  if (ret < 0) {
    syslog(LOG_ERR, "error: %s send failed\n", __func__);
    goto bail;
  }

  //ret = mctp_smbus_recv_data(mctp_binding->mctp, dst_eid, smbus, rbuf);
  ret = mctp_smbus_recv_data_timeout_raw(mctp_binding->mctp, dst_eid, smbus, rbuf, -1);
  if (ret < 0) {
    syslog(LOG_ERR, "%s: error getting response\n", __func__);
    goto bail;
  } else {
    *rlen = ret;
    ret = 0;
  }

bail:
  obmc_mctp_smbus_free(mctp_binding);
  return ret;
}

static void pldmReq_to_mctpReq(struct mctp_pldm_req *req, pldm_cmd_req *pldmReq)
{
  req->Msg_Type = MCTP_TYPE_PLDM;
  memcpy(&req->pkt, pldmReq->common, pldmReq->payload_size);
}

static void mctpReq_to_pldmReq(pldm_cmd_req *pldmReq, struct obmc_mctp_pldm_req *req)
{
  pldmReq->payload_size = req->hdr.Msg_Size - 1;
  memcpy(pldmReq->common, &req->pkt, req->hdr.Msg_Size - 1);
}

static void pldmRes_to_mctpRes(struct mctp_pldm_rsp *rsp, pldm_response *pldmRes)
{
  rsp->Msg_Type = MCTP_TYPE_PLDM;
  memcpy(&rsp->pkt, pldmRes->common, pldmRes->resp_size);
}

int obmc_mctp_fw_update(struct obmc_mctp_binding *binding, uint8_t dst_eid,
                        uint8_t tag, char *path)
{
  int ret = -1;
  struct mctp *mctp = binding->mctp;
  struct mctp_binding_smbus *smbus = (struct mctp_binding_smbus *)binding->prot;
  struct mctp_pldm_req req = {0};
  struct mctp_pldm_rsp rsp = {0};
  struct obmc_mctp_pldm_req obmc_req = {0};
  struct obmc_mctp_pldm_rsp obmc_rsp = {0};
  pldm_fw_pkg_hdr_t *pkgHdr;
  pldm_cmd_req pldmReq = {0};
  pldm_response pldmRes = {0};
  int pldmCmdStatus = -1;
  int i = 0;
  int waitTOsec = 0;
  uint8_t pldmCmd = 0;

  pkgHdr = pldm_parse_fw_pkg(path);
  if (!pkgHdr) {
    ret = -1;
    goto free_exit;
  }

  pldmCreateReqUpdateCmd(pkgHdr, &pldmReq, 1024);
  printf("\n01 PldmRequestUpdateOp: payload_size=%d\n", pldmReq.payload_size);
  pldmReq_to_mctpReq(&req, &pldmReq);

  ret = mctp_smbus_send_data(mctp, dst_eid, (tag | MCTP_HDR_FLAG_TO),
                             smbus, &req, pldmReq.payload_size+1);
  if (ret < 0) {
    goto free_exit;
  }
  memset(&obmc_rsp, 0, sizeof(obmc_rsp));
  ret = mctp_smbus_recv_data(mctp, dst_eid, smbus, &obmc_rsp);
  if (ret != CC_SUCCESS) {
    goto free_exit;
  }

  for (i = 0; i < pkgHdr->componentImageCnt; ++i) {
    memset(&pldmReq, 0, sizeof(pldm_cmd_req));
    pldmCreatePassComponentTblCmd(pkgHdr, i, &pldmReq);
    printf("\n02 PldmPassComponentTableOp[%d]: payload_size=%d\n", i,
            pldmReq.payload_size);
    pldmReq_to_mctpReq(&req, &pldmReq);
    ret = mctp_smbus_send_data(mctp, dst_eid, (tag | MCTP_HDR_FLAG_TO),
                               smbus, &req, pldmReq.payload_size+1);
    if (ret < 0) {
      goto exit;
    }
    memset(&obmc_rsp, 0, sizeof(obmc_rsp));
    ret = mctp_smbus_recv_data(mctp, dst_eid, smbus, &obmc_rsp);
    if (ret != CC_SUCCESS) {
      goto exit;
    }
  }

  for (i = 0; i < pkgHdr->componentImageCnt; ++i) {
    memset(&pldmReq, 0, sizeof(pldm_cmd_req));
    pldmCreateUpdateComponentCmd(pkgHdr, i, &pldmReq);
    printf("\n03 PldmUpdateComponentOp[%d]: payload_size=%d\n", i,
            pldmReq.payload_size);
    pldmReq_to_mctpReq(&req, &pldmReq);
    ret = mctp_smbus_send_data(mctp, dst_eid, (tag | MCTP_HDR_FLAG_TO),
                               smbus, &req, pldmReq.payload_size+1);
    if (ret < 0) {
      goto exit;
    }
    memset(&obmc_rsp, 0, sizeof(obmc_rsp));
    ret = mctp_smbus_recv_data(mctp, dst_eid, smbus, &obmc_rsp);
    if (ret != CC_SUCCESS) {
      goto exit;
    }
  }

  // FW data transfer
  setPldmTimeout(CMD_UPDATE_COMPONENT, &waitTOsec);
  while (1) {
    memset(&obmc_req, 0, sizeof(obmc_req));
    ret = mctp_smbus_recv_data_timeout(mctp, dst_eid, smbus, &obmc_req, waitTOsec);
    if (ret != CC_SUCCESS) {
      break;
    }
    pldmCmd = obmc_req.pkt.hdr.Command_Code;
    if ( (pldmCmd == CMD_REQUEST_FIRMWARE_DATA) ||
         (pldmCmd == CMD_TRANSFER_COMPLETE) ||
         (pldmCmd == CMD_VERIFY_COMPLETE) ||
         (pldmCmd == CMD_APPLY_COMPLETE)) {
      setPldmTimeout(pldmCmd, &waitTOsec);
      mctpReq_to_pldmReq(&pldmReq, &obmc_req);
      pldmCmdStatus = pldmFwUpdateCmdHandler(pkgHdr, &pldmReq, &pldmRes);
      pldmRes_to_mctpRes(&rsp, &pldmRes);

      ret = mctp_smbus_send_data(mctp, dst_eid, tag,
                                 smbus, &rsp, pldmRes.resp_size+1);
      if (ret < 0) {
        break;
      }
      if ((pldmCmd == CMD_APPLY_COMPLETE) || (pldmCmdStatus == -1))
        break;
    } else {
      printf("unknown PLDM cmd 0x%02X\n", pldmCmd);
      break;
    }
  }

exit:
  // only activate FW if update loop exists with good status
  if (!pldmCmdStatus && (pldmCmd == CMD_APPLY_COMPLETE)) {
    // update successful,  activate FW
    memset(&pldmReq, 0, sizeof(pldm_cmd_req));
    pldmCreateActivateFirmwareCmd(&pldmReq);
    printf("\n05 PldmActivateFirmwareOp\n");
    pldmReq_to_mctpReq(&req, &pldmReq);
    ret = mctp_smbus_send_data(mctp, dst_eid, tag | MCTP_HDR_FLAG_TO,
                               smbus, &req, pldmReq.payload_size+1);
    if (ret < 0) {
      goto free_exit;
    }
    memset(&obmc_rsp, 0, sizeof(obmc_rsp));
    ret = mctp_smbus_recv_data(mctp, dst_eid, smbus, &obmc_rsp);
    if (ret != CC_SUCCESS) {
      goto free_exit;
    }
  } else {
    printf("PLDM cmd (0x%02X) failed (status %d), abort update\n",
      pldmCmd, pldmCmdStatus);

    // send abort update req
    memset(&pldmReq, 0, sizeof(pldm_cmd_req));
    pldmCreateCancelUpdateCmd(&pldmReq);
    pldmReq_to_mctpReq(&req, &pldmReq);
    ret = mctp_smbus_send_data(mctp, dst_eid, tag | MCTP_HDR_FLAG_TO,
                               smbus, &req, pldmReq.payload_size+1);
    if (ret < 0) {
      goto free_exit;
    }
    memset(&obmc_rsp, 0, sizeof(obmc_rsp));
    ret = mctp_smbus_recv_data(mctp, dst_eid, smbus, &obmc_rsp);
    if (ret != CC_SUCCESS) {
      goto free_exit;
    }
    ret = -1;
  }

free_exit:
  if (pkgHdr)
    free_pldm_pkg_data(&pkgHdr);
  return ret;
}
