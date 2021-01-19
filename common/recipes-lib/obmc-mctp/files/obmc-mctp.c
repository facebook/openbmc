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
#include <libmctp-smbus.h>
#include <libmctp-alloc.h>
#include <libmctp-log.h>
#include "obmc-mctp.h"

//#define DEBUG
#define SYSFS_SLAVE_QUEUE "/sys/bus/i2c/devices/%d-10%02x/slave-mqueue"
#define NIC_SLAVE_ADDR 0x64

// TODO:
// 	Migrate this library to C++ if BMC need to support MCTP over PCIe

static void rx_handler(uint8_t eid, void *data, void *msg, size_t len,
                       bool tag_owner, uint8_t tag, void *prv)
{
  memcpy(data, msg, len);
}

struct obmc_mctp_binding* obmc_mctp_smbus_init(uint8_t bus, uint8_t addr,
                                               uint8_t src_eid)
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

  mctp = mctp_init();
  smbus = mctp_smbus_init();
  if (mctp == NULL || smbus == NULL || mctp_smbus_register_bus(smbus, mctp, src_eid) < 0) {
    syslog(LOG_ERR, "%s: MCTP init failed", __func__);
    goto bail;
  }

  snprintf(dev, sizeof(dev), "/dev/i2c-%d", bus);
  fd = open(dev, O_RDWR);
  if (fd < 0) {
    syslog(LOG_ERR, "%s: open %s failed", __func__, dev);
    goto bail;
  }
  mctp_smbus_set_out_fd(smbus, fd);

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
}

static int mctp_smbus_send_ncsi_data(struct mctp* mctp, uint8_t dst, uint8_t tag,
                                     struct mctp_binding_smbus *smbus,
                                     struct mctp_ncsi_cmd *cmd,
                                     struct mctp_smbus_extra_params *smbus_extra_params)
{
  smbus_extra_params->muxHoldTimeOut = 0;
  smbus_extra_params->muxFlags = 0;
  smbus_extra_params->fd = smbus->out_fd;
  smbus_extra_params->slave_addr = NIC_SLAVE_ADDR;

  if (mctp_message_tx(mctp, dst, cmd, sizeof(struct mctp_ncsi_cmd),
                      true, tag, smbus_extra_params) < 0) {
    syslog(LOG_ERR, "%s: MCTP TX error", __func__);
    return -1;
  }
  return 0;
}

static int mctp_smbus_recv_ncsi_data(struct mctp *mctp, uint8_t dst, uint8_t tag,
                                     struct mctp_binding_smbus *smbus,
                                     struct mctp_ncsi_rsp *rsp)
{
  int retry = 3;

  rsp->pkt.data.Response_Code = 0xFFFF;
  rsp->pkt.data.Reason_Code = 0xFFFF;
  mctp_set_rx_all(mctp, rx_handler, rsp);

  while (retry--) {
    usleep(100*1000);
    if (mctp_smbus_read(smbus) < 0) {
      syslog(LOG_ERR, "%s: MCTP RX error", __func__);
      return -1;
    }
    if (rsp->pkt.data.Response_Code == RESP_COMMAND_COMPLETED &&
        rsp->pkt.data.Reason_Code == REASON_NO_ERROR) {
      return 0;
    }
  }
  syslog(LOG_ERR, "%s: MCTP timeout", __func__);
  return -1;;
}

int obmc_mctp_clear_init_state(struct obmc_mctp_binding *binding, uint8_t dst_eid,
                               uint8_t tag, uint8_t iid)
{
  int ret = -1;
  struct mctp *mctp = binding->mctp;
  struct mctp_binding_smbus *smbus = (struct mctp_binding_smbus *)binding->prot;
  struct mctp_ncsi_cmd cmd;
  struct mctp_ncsi_rsp rsp;
  struct mctp_smbus_extra_params *smbus_extra_params;

  /* NC-SI: Clear init state */
  memset(&cmd, 0x0, sizeof(struct mctp_ncsi_cmd));
  cmd.msg_type                = 0x02;
  cmd.pkt.hdr.MC_ID           = 0x00;
  cmd.pkt.hdr.Header_Revision = 0x01;
  cmd.pkt.hdr.IID             = iid;
  cmd.pkt.hdr.Command         = NCSI_CLEAR_INITIAL_STATE;
  cmd.pkt.hdr.Channel_ID      = 0x00;
  cmd.pkt.hdr.Payload_Length  = 0x00;

  smbus_extra_params = (struct mctp_smbus_extra_params *)
                       malloc(sizeof(struct mctp_smbus_extra_params));
  if (smbus_extra_params == NULL) {
    syslog(LOG_ERR, "%s: out of memory", __func__);
    goto bail;
  }

  ret = mctp_smbus_send_ncsi_data(mctp, dst_eid, tag, smbus, &cmd, smbus_extra_params);
  if (ret < 0) {
    goto bail;
  }

  memset(&rsp, 0x0, sizeof(struct mctp_ncsi_rsp));
  ret = mctp_smbus_recv_ncsi_data(mctp, dst_eid, tag, smbus,  &rsp);
  if (ret < 0) {
    syslog(LOG_ERR, "%s: Response code = 0x%02X, Reason code = 0x%02X",
	            __func__, rsp.pkt.data.Response_Code, rsp.pkt.data.Reason_Code);
    goto bail;
  }
  ret = 0;
bail:
  free(smbus_extra_params);
  return ret;
}

int obmc_mctp_get_version_id(struct obmc_mctp_binding *binding, uint8_t dst_eid,
                             uint8_t tag, uint8_t iid,
                             Get_Version_ID_Response *payload)
{
  int ret = -1;
  struct mctp *mctp = binding->mctp;
  struct mctp_binding_smbus *smbus = (struct mctp_binding_smbus *)binding->prot;
  struct mctp_ncsi_cmd cmd;
  struct mctp_ncsi_rsp rsp;
  struct mctp_smbus_extra_params *smbus_extra_params;

  /* NC-SI: Get version */
  memset(&cmd, 0x0, sizeof(struct mctp_ncsi_cmd));
  cmd.msg_type                = 0x02;
  cmd.pkt.hdr.MC_ID           = 0x00;
  cmd.pkt.hdr.Header_Revision = 0x01;
  cmd.pkt.hdr.IID             = iid;
  cmd.pkt.hdr.Command         = NCSI_GET_VERSION_ID;
  cmd.pkt.hdr.Channel_ID      = 0x00;
  cmd.pkt.hdr.Payload_Length  = 0x00;

  smbus_extra_params = (struct mctp_smbus_extra_params *)
                       malloc(sizeof(struct mctp_smbus_extra_params));
  if (smbus_extra_params == NULL) {
    syslog(LOG_ERR, "%s: out of memory", __func__);
    goto bail;
  }

  ret = mctp_smbus_send_ncsi_data(mctp, dst_eid, tag, smbus, &cmd, smbus_extra_params);
  if (ret < 0) {
    goto bail;
  }

  memset(&rsp, 0x0, sizeof(rsp));
  ret = mctp_smbus_recv_ncsi_data(mctp, dst_eid, tag, smbus, &rsp);
  if (ret < 0) {
    syslog(LOG_ERR, "%s: Response code = 0x%02X, Reason code = 0x%02X",
	            __func__, rsp.pkt.data.Response_Code, rsp.pkt.data.Reason_Code);
    goto bail;
  }
  memcpy(payload, rsp.pkt.data.Payload_Data, sizeof(Get_Version_ID_Response));
  ret = 0;
bail:
  free(smbus_extra_params);
  return ret;
}
