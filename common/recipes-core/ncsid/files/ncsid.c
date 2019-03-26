/*
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

/*
 * This module handles all the NCSI communication protocol
 *
 *
 */
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <syslog.h>
#include <sys/un.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <linux/netlink.h>
#include <sys/un.h>
#include <unistd.h>
#include <stdint.h>
#include <mqueue.h>
#include <semaphore.h>
#include <poll.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <stddef.h>
#include <openbmc/obmc-pal.h>
#include <time.h>
#include <openbmc/ncsi.h>
#include <openbmc/aen.h>
#include <openbmc/pldm.h>
#include <openbmc/pldm_base.h>
#include <openbmc/pldm_fw_update.h>
#include <inttypes.h>
//#define DEBUG 0

// special  channel/cmd  used for register AEN handler with kernel
#define REG_AEN_CH  0xfa
#define REG_AEN_CMD 0xce

#ifndef MAX
#define MAX(a, b) ((a) > (b)) ? (a) : (b)
#endif

/*
   Default config:
      - poll NIC status once every 60 seconds
*/
/* POLL nic status every N seconds */
#define NIC_STATUS_SAMPLING_DELAY  60


// Structure for netlink file descriptor and socket
typedef struct _nl_sfd_t {
  int fd;
  int sock;
} nl_sfd_t;

static struct timespec last_config_ts;
static NCSI_Get_Capabilities_Response gNicCapability = {0};
static uint32_t vendor_IANA = 0;
static uint32_t aen_enable_mask = AEN_ENABLE_DEFAULT;


static int
prepare_ncsi_req_msg(struct msghdr *msg, uint8_t ch, uint8_t cmd,
                     uint16_t payload_len, unsigned char *payload,
                     uint16_t response_len) {

  struct sockaddr_nl *dest_addr = NULL;
  struct nlmsghdr *nlh = NULL;
  NCSI_NL_MSG_T *nl_msg = NULL;
  struct iovec *iov;
  int req_msg_size = offsetof(NCSI_NL_MSG_T, msg_payload) + payload_len;

  // msg_size will be used to hold both tx and response data (if there's a respsonse)
  //   hence we need to take size of the response into account
  int msg_size = MAX(response_len, req_msg_size);

  dest_addr = (struct sockaddr_nl *)malloc(sizeof(struct sockaddr_nl));
  if (!dest_addr) {
    syslog(LOG_ERR, "prepare_ncsi_req_msg: failed to allocate msg_name");
    return -1;
  }
  memset(dest_addr, 0, sizeof(*dest_addr));
  dest_addr->nl_family = AF_NETLINK;
  dest_addr->nl_pid = 0;    /* For Linux Kernel */
  dest_addr->nl_groups = 0;

  nlh = (struct nlmsghdr *)malloc(NLMSG_SPACE(msg_size));
  if (!nlh) {
    syslog(LOG_ERR, "prepare_ncsi_req_msg: failed to allocate message buffer");
    goto free_and_exit;
  }
  memset(nlh, 0, NLMSG_SPACE(msg_size));
  nlh->nlmsg_len = NLMSG_SPACE(msg_size);
  nlh->nlmsg_pid = getpid();
  nlh->nlmsg_flags = 0;

  nl_msg = (NCSI_NL_MSG_T *)NLMSG_DATA(nlh);

  create_ncsi_ctrl_pkt(nl_msg, ch, cmd, payload_len, payload);

  iov = (struct iovec *)malloc(sizeof(struct iovec));
  if (!iov) {
    syslog(LOG_ERR, "prepare_ncsi_req_msg: failed to allocate iovec");
    goto free_and_exit;
  }
  iov->iov_base = (void *)nlh;
  iov->iov_len = nlh->nlmsg_len;

  msg->msg_name = (void *)dest_addr;
  msg->msg_namelen = sizeof(*dest_addr);
  msg->msg_iov = iov;
  msg->msg_iovlen = 1;

  return 0;

free_and_exit:
  if (dest_addr)
    free(dest_addr);
  if (nlh)
    free(nlh);
  if (iov)
    free(iov);

  return -1;
}

static void
free_ncsi_req_msg(struct msghdr *msg) {
  if (msg->msg_name)
    free(msg->msg_name);

  if (msg->msg_iov) {
    if (msg->msg_iov->iov_base)
      free(msg->msg_iov->iov_base);

    free(msg->msg_iov);
  }
}


static int
send_registration_msg(nl_sfd_t *sfd)
{
  struct sockaddr_nl src_addr;
  struct msghdr msg;
  int ret = 0;
  int sock_fd = ((nl_sfd_t *)sfd)->fd;

  memset(&msg, 0, sizeof(msg));
  memset(&src_addr, 0, sizeof(src_addr));
  src_addr.nl_family = AF_NETLINK;
  src_addr.nl_pid = getpid(); /* self pid */

  if (bind(sock_fd, (struct sockaddr*)&src_addr, sizeof(src_addr)) == -1) {
    syslog(LOG_ERR, "send_registration_msg: bind socket failed");
    ret = 1;
    return ret;
  };

  ret = prepare_ncsi_req_msg(&msg, REG_AEN_CH, REG_AEN_CMD, 0, NULL, 0);
  if (ret) {
    syslog(LOG_ERR, "send_registration_msg: prepare message failed");
    return ret;
  }

  syslog(LOG_INFO, "send_registration_msg: registering PID %d",
                    ((struct nlmsghdr *)msg.msg_iov->iov_base)->nlmsg_pid);

  ret = sendmsg(sock_fd, &msg, 0);
  if (ret < 0) {
    syslog(LOG_ERR, "send_registration_msg: status ret = %d, errno=%d", ret, errno);
  }

  free_ncsi_req_msg(&msg);
  return ret;
}


static void
init_gNicCapability(NCSI_Get_Capabilities_Response *buf)
{
  memcpy(&gNicCapability, buf, sizeof(NCSI_Get_Capabilities_Response));

  // fix endianness
  gNicCapability.capabilities_flags = ntohl(buf->capabilities_flags);

  gNicCapability.broadcast_packet_filter_capabilities =
      ntohl(buf->broadcast_packet_filter_capabilities);

  gNicCapability.multicast_packet_filter_capabilities =
      ntohl(buf->multicast_packet_filter_capabilities);

  gNicCapability.buffering_capabilities = ntohl(buf->buffering_capabilities);
  gNicCapability.aen_control_support = ntohl(buf->aen_control_support);
#if DEBUG
  syslog(LOG_INFO, "init_gNicCapability 0x%x, 0x%x, 0x%x, 0x%x, 0x%x",
         gNicCapability.capabilities_flags,
         gNicCapability.broadcast_packet_filter_capabilities,
         gNicCapability.multicast_packet_filter_capabilities,
         gNicCapability.buffering_capabilities,
         gNicCapability.aen_control_support);
#endif
}


static void
get_mlx_fw_version(uint8_t *buf, char *version)
{
  int ver_index_based = 0;
  int major = 0;
  int minor = 0;
  int revision = 0;

  major = buf[ver_index_based++];
  minor = buf[ver_index_based++];
  revision = buf[ver_index_based++] << 8;
  revision += buf[ver_index_based++];

  sprintf(version, "%d.%d.%d", major, minor, revision);
}

static void
get_bcm_fw_version(char *buf, char *version)
{
  int ver_start_index = 0;      //the index is defined in the NC-SI spec
  const int ver_end_index = 11;
  char ver[32]={0};
  int i = 0;

  for( ; ver_start_index <= ver_end_index; ver_start_index++, i++)
  {
    if ( 0 == buf[ver_start_index] )
    {
      break;
    }
    ver[i] = buf[ver_start_index];
  }

  strcat(version, ver);
}


static void
init_version_data(Get_Version_ID_Response *buf)
{
  char logbuf[256];
  char version[32]={0};
  int i = 0;
  int nleft = sizeof(logbuf);
  int nwrite = 0;

  vendor_IANA = ntohl(buf->IANA);

  nwrite = snprintf(logbuf, nleft, "NIC Firmware Version: ");
  i += nwrite;
  nleft -= nwrite;

  if (vendor_IANA == MLX_IANA)  {
    aen_enable_mask = AEN_ENABLE_DEFAULT;
    nwrite = snprintf(logbuf+i, nleft, "Mellanox ");
    get_mlx_fw_version(buf->fw_ver, version);
  } else if (vendor_IANA == BCM_IANA)  {
    aen_enable_mask = AEN_ENABLE_MASK_BCM;
    nwrite = snprintf(logbuf+i, nleft, "Broadcom ");
    get_bcm_fw_version(buf->fw_name, version);
  } else {
    aen_enable_mask = AEN_ENABLE_DEFAULT;
    nwrite = snprintf(logbuf+i, nleft, "Unknown Vendor(IANA 0x%x) ", vendor_IANA);
  }
  i += nwrite;
  nleft -= nwrite;
  nwrite = snprintf(logbuf+i, nleft, "%s ", version);

  syslog(LOG_INFO, "%s", logbuf);
}



static int
send_cmd_and_get_resp(nl_sfd_t *sfd, uint8_t ncsi_cmd,
                      uint16_t payload_len, unsigned char *payload,
                      NCSI_NL_RSP_T *resp_buf)
{
  struct sockaddr_nl src_addr;
  struct msghdr msg;
  int ret = 0;
  int sock_fd = ((nl_sfd_t *)sfd)->fd;
  int msg_resp_size = sizeof(NCSI_NL_RSP_T);
  /* msg response from kernel */
  NCSI_NL_RSP_T *rcv_buf = NULL;

  memset(&msg, 0, sizeof(msg));
  memset(&src_addr, 0, sizeof(src_addr));
  src_addr.nl_family = AF_NETLINK;
  src_addr.nl_pid = getpid(); /* self pid */

  if (bind(sock_fd, (struct sockaddr*)&src_addr, sizeof(src_addr)) == -1) {
    syslog(LOG_ERR, "send_cmd(0x%x): bind socket failed", ncsi_cmd);
    return -1;
  };

  ret = prepare_ncsi_req_msg(&msg, 0, ncsi_cmd, payload_len, payload,
                             sizeof(NCSI_NL_RSP_T));
  if (ret) {
    syslog(LOG_ERR, "send_cmd(0x%x): prepare message failed", ncsi_cmd);
    ret = -1;
    goto free_exit;
  }

  ret = sendmsg(sock_fd, &msg, 0);
  if (ret < 0) {
    syslog(LOG_ERR, "send_cmd(0x%x): status ret = %d, errno=%d",
           ncsi_cmd, ret, errno);
    ret = -1;
    goto free_exit;
  }

  /* Read response from kernel */
  msg.msg_iov->iov_len = NLMSG_SPACE(msg_resp_size);
  ret = recvmsg(sock_fd, &msg, 0);
  if (ret < 0) {
    syslog(LOG_ERR, "send_cmd(0x%x): failed receiving response, ret = %d, errno=%d",
           ncsi_cmd, ret, errno);
    ret = -1;
    goto free_exit;
  }
  rcv_buf = (NCSI_NL_RSP_T *)NLMSG_DATA(msg.msg_iov->iov_base);
  // check NCSI command response, exit if command failed
  ret = get_cmd_status(rcv_buf);
  if (ret != RESP_COMMAND_COMPLETED)
  {
    syslog(LOG_ERR, "send_cmd(0x%x): Command failed, resp = 0x%x",
           ncsi_cmd, ret);
    ret = -1;
    goto free_exit;
  }

  if (resp_buf)
    memcpy(resp_buf, rcv_buf, sizeof(NCSI_NL_RSP_T));

free_exit:
  free_ncsi_req_msg(&msg);
  return ret;
}


// do_pldm_discovery
//
// Queries NIC to discover its PLDM capabilities, including
//    1. PLDM types supported,
//    2. supported version for each PLDM type
static int
do_pldm_discovery(nl_sfd_t *sfd, NCSI_NL_RSP_T *resp_buf)
{
  pldm_cmd_req pldmReq = {0};
  int pldmStatus = 0;
  uint64_t pldmType = 0;
  pldm_ver_t pldm_version = {0};
  int ret = 0;
  int i = 0;

  pldmCreateGetPldmTypesCmd(&pldmReq);
  ret = send_cmd_and_get_resp(sfd, NCSI_PLDM_REQUEST,  PLDM_COMMON_REQ_LEN,
                              (unsigned char *)&(pldmReq.common), resp_buf);
  if (ret < 0) {
    syslog(LOG_ERR, "do_pldm_discovery: failed PLDM Discovery");
    return ret;
  }

  pldmStatus = ncsiDecodePldmCompCode(resp_buf);
  syslog(LOG_INFO, "PLDM discovery status= %d(%s)",
         pldmStatus, pldm_fw_cmd_cc_to_name(pldmStatus));
  if (pldmStatus == CC_SUCCESS) {
    pldmType = pldmHandleGetPldmTypesResp((PLDM_GetPldmTypes_Response_t *)&(resp_buf->msg_payload[7]));
  }
  syslog(LOG_CRIT, "PLDM type spported = 0x%" PRIx64, pldmType);

  // Query  version for each supported type
  for (i = 0; i<PLDM_RSV; ++i) {
    if (((1<<i) & pldmType) == 0) {
       // this type is not supported
       continue;
    }

    pldmCreateGetVersionCmd(i, &pldmReq);
    ret = send_cmd_and_get_resp(sfd, NCSI_PLDM_REQUEST,
            (PLDM_COMMON_REQ_LEN + sizeof(PLDM_GetPldmVersion_t)),
            (unsigned char *)&(pldmReq.common), resp_buf);
    if (ret < 0) {
      syslog(LOG_ERR, "do_pldm_discovery: failed get PLDM (%d) version", i);
      return ret;
    }
    pldmStatus = ncsiDecodePldmCompCode(resp_buf);
    if (pldmStatus == CC_SUCCESS) {
      pldmHandleGetVersionResp((PLDM_GetPldmVersion_Response_t *)&(resp_buf->msg_payload[7]),
                                         &pldm_version);
    }
    syslog(LOG_CRIT, "    PLDM type %d version = %d.%d.%d.%d", i,
             pldm_version.major, pldm_version.minor, pldm_version.update,
             pldm_version.alpha);
  }
  return 0;
}


// Configure NIC
//
// Performs initial configuration of network controller
// For now, it does the following:
//  1. issues "Get Capability" command to NIC to see what AENs this device +
//     firmware supports
//         1a. initialize global gNicCapability  with NIC capabilities
//  2. determine NIC manufacturer and FW versions
//  3. Enable AEN based on NIC capability
//  4. Discovery NIC's PLDM capabilities
static int
init_nic_config(nl_sfd_t *sfd)
{
  NCSI_NL_RSP_T *resp_buf = calloc(1, sizeof(NCSI_NL_RSP_T));
  NCSI_Response_Packet* pNcsiResp;
  int ret = 0;

  if (!resp_buf) {
    syslog(LOG_ERR, "init_nic_config: failed to allocate resp buffer (%d)",
           sizeof(NCSI_NL_RSP_T));
    return -1;
  }

  // get NIC CAPABILITY
  ret = send_cmd_and_get_resp(sfd, NCSI_GET_CAPABILITIES, 0, NULL, resp_buf);
  if (ret < 0) {
    syslog(LOG_ERR, "init_nic_config: failed to send cmd (0x%x)",
           NCSI_GET_CAPABILITIES);
    ret = -1;
    goto free_exit;
  }
  pNcsiResp = (NCSI_Response_Packet*)(resp_buf->msg_payload);
  init_gNicCapability((NCSI_Get_Capabilities_Response*)(pNcsiResp->Payload_Data));


  // get NIC Manufacturer and firmware version
  ret = send_cmd_and_get_resp(sfd, NCSI_GET_VERSION_ID,  0, NULL, resp_buf);
  if (ret < 0) {
    syslog(LOG_ERR, "init_nic_config: failed to send cmd (0x%x)",
           NCSI_GET_VERSION_ID);
    ret = -1;
    goto free_exit;
  }
  pNcsiResp = (NCSI_Response_Packet*)(resp_buf->msg_payload);
  init_version_data((Get_Version_ID_Response*)(pNcsiResp->Payload_Data));

  aen_enable_mask &= gNicCapability.aen_control_support;
  syslog(LOG_CRIT, "NIC AEN Supported: 0x%x, AEN Enable Mask=0x%x",
          gNicCapability.aen_control_support, aen_enable_mask);


  // PLDM discovery
  do_pldm_discovery(sfd, resp_buf);
  // PLDM support is optional, so ignore return value for now

free_exit:
  if (resp_buf)
    free(resp_buf);
  return ret;
}


static int
process_NCSI_resp(NCSI_NL_RSP_T *buf)
{
  unsigned char cmd = buf->hdr.cmd;
  NCSI_Response_Packet *resp = (NCSI_Response_Packet *)buf->msg_payload;
  unsigned short cmd_response_code = ntohs(resp->Response_Code);
  unsigned short cmd_reason_code   = ntohs(resp->Reason_Code);
  int ret = 0;
  struct timespec ts;

  /* chekc for command completion before processing
     response payload */
  if (cmd_response_code != RESP_COMMAND_COMPLETED) {
      syslog(LOG_WARNING, "NCSI Cmd (0x%x) failed,"
             " Cmd Response 0x%x, Reason 0x%x",
             cmd, cmd_response_code, cmd_reason_code);
     /* if command fails for a specific signature (i.e. NCSI interface failure)
        try to re-init NCSI interface */
    if ((cmd_response_code == RESP_COMMAND_UNAVAILABLE) &&
           ((cmd_reason_code == REASON_INTF_INIT_REQD)  ||
            (cmd_reason_code == REASON_CHANNEL_NOT_RDY) ||
            (cmd_reason_code == REASON_PKG_NOT_RDY))
       ) {
      clock_gettime(CLOCK_MONOTONIC, &ts);  // to avoid re-initialize closely
      if (((ts.tv_sec - last_config_ts.tv_sec) >= NIC_STATUS_SAMPLING_DELAY/2) ||
          (last_config_ts.tv_sec == 0)) {
        syslog(LOG_WARNING, "NCSI Cmd (0x%x) failed,"
               " Cmd Response 0x%x, Reason 0x%x, re-init NCSI interface",
               cmd, cmd_response_code, cmd_reason_code);

        last_config_ts.tv_sec = ts.tv_sec;
        handle_ncsi_config(0);
        return NCSI_IF_REINIT;
      }
    }

    /* for other types of command failures, ignore for now */
    return 0;
  } else {
    switch (cmd) {
      case NCSI_GET_LINK_STATUS:
        ret = handle_get_link_status(resp);
        break;
      case NCSI_GET_VERSION_ID:
        ret = handle_get_version_id(resp);
        break;
      case NCSI_AEN_ENABLE:
        // ignore the response from this command, as it contains no payload
        break;

      // TBD: handle other command response here
      default:
        syslog(LOG_WARNING, "unknown or unexpected command response, cmd 0x%x(%s)",
               cmd, ncsi_cmd_type_to_name(cmd));
        break;
    }
  }
  return ret;
}


// enable platform-specific AENs
void
enable_aens(void *sfd, uint32_t aen_enable_mask) {
#define PAYLOAD_SIZE 8
  int ret = 0;
  unsigned char payload[PAYLOAD_SIZE]  = {0};

  syslog(LOG_INFO, "enable aens: mask=0x%x", aen_enable_mask);

  // convert to network byte order
  aen_enable_mask = htonl(aen_enable_mask);

  memcpy(&(payload[4]), &aen_enable_mask, sizeof(uint32_t));

  ret = send_cmd_and_get_resp(sfd, NCSI_AEN_ENABLE, PAYLOAD_SIZE, payload, NULL);
  if (ret < 0) {
    syslog(LOG_ERR, "enable_aens: failed to enable AEN");
  }

  return;
}


// Thread to handle the incoming responses
static void*
ncsi_rx_handler(void *sfd) {
  int sock_fd = ((nl_sfd_t *)sfd)->fd;
  struct msghdr msg;
  struct iovec iov;
  int msg_size = sizeof(NCSI_NL_RSP_T);
  struct nlmsghdr *nlh = NULL;
  int ret = 0;

  /* msg response from kernel */
  NCSI_NL_RSP_T *rcv_buf;
  memset(&msg, 0, sizeof(msg));

  syslog(LOG_INFO, "rx: ncsi_rx_handler thread started");

  nlh = (struct nlmsghdr *)malloc(NLMSG_SPACE(msg_size));
  if (!nlh) {
    syslog(LOG_ERR, "rx: Error, failed to allocate message buffer");
    return NULL;
  }
  memset(nlh, 0, NLMSG_SPACE(msg_size));
  nlh->nlmsg_len = NLMSG_SPACE(msg_size);
  nlh->nlmsg_pid = getpid();
  nlh->nlmsg_flags = 0;

  iov.iov_base = (void *)nlh;
  iov.iov_len = nlh->nlmsg_len;
  msg.msg_iov = &iov;
  msg.msg_iovlen = 1;

  // the last timestamp to call handle_ncsi_config() when processing NCSI_resp
  last_config_ts.tv_sec = 0;

  while (1) {
    /* Read message from kernel */
    recvmsg(sock_fd, &msg, 0);
    rcv_buf = (NCSI_NL_RSP_T *)NLMSG_DATA(nlh);
    if (is_aen_packet((AEN_Packet *)rcv_buf->msg_payload)) {
      ret = process_NCSI_AEN((AEN_Packet *)rcv_buf->msg_payload);
    } else {
      ret = process_NCSI_resp(rcv_buf);
    }

    if (ret == NCSI_IF_REINIT) {
      send_registration_msg(sfd);
      enable_aens(sfd, aen_enable_mask);
    }
  }

  free(nlh);
  close(sock_fd);

  pthread_exit(NULL);
}


// Thread to send periodic NC-SI cmmands to check NIC status
static void*
ncsi_tx_handler(void *sfd) {
  int ret, sock_fd = ((nl_sfd_t *)sfd)->fd;
  struct msghdr lsts_msg, vid_msg;

  syslog(LOG_INFO, "ncsi_tx_handler thread started");

  memset(&lsts_msg, 0, sizeof(lsts_msg));
  memset(&vid_msg, 0, sizeof(vid_msg));
  prepare_ncsi_req_msg(&lsts_msg, 0, NCSI_GET_LINK_STATUS, 0, NULL, 0);
  prepare_ncsi_req_msg(&vid_msg, 0, NCSI_GET_VERSION_ID, 0, NULL, 0);

  while (1) {
    /* send "Get Link status" message to NIC  */
    ret = sendmsg(sock_fd, &lsts_msg, 0);
    if (ret < 0) {
      syslog(LOG_ERR, "tx: failed to send lsts_msg, status ret = %d, errno=%d\n",
             ret, errno);
    }

    /* send "Get Version ID" message to NIC  */
    ret = sendmsg(sock_fd, &vid_msg, 0);
    if (ret < 0) {
      syslog(LOG_ERR, "tx: failed to send vid_msg, status ret = %d, errno=%d\n",
             ret, errno);
    }

    ret = check_valid_mac_addr();
    if (ret == NCSI_IF_REINIT) {
      send_registration_msg(sfd);
      enable_aens(sfd, aen_enable_mask);
    }
    sleep(NIC_STATUS_SAMPLING_DELAY);
  }

  free_ncsi_req_msg(&lsts_msg);
  free_ncsi_req_msg(&vid_msg);
  close(sock_fd);

  pthread_exit(NULL);
}


// Thread to setup netlink and recieve AEN
static void*
ncsi_aen_handler(void *unused) {
  int sock_fd;
  pthread_t tid_rx;
  pthread_t tid_tx;
  nl_sfd_t *sfd;
  int ret = 0;

  syslog(LOG_INFO, "ncsi_aen_handler thread started\n");
  /* open NETLINK socket to send message to kernel */
  sock_fd = socket(PF_NETLINK, SOCK_RAW, NETLINK_USER);
  if(sock_fd < 0)  {
    syslog(LOG_ERR, "ncsi_aen_handler: Error: failed to allocate tx socket\n");
    return NULL;
  }

  sfd = (nl_sfd_t *)malloc(sizeof(nl_sfd_t));
  if (!sfd) {
    syslog(LOG_ERR, "ncsi_aen_handler: malloc fail on sfd\n");
    goto close_and_exit;
  }

  sfd->fd = sock_fd;

  ret = init_nic_config(sfd);
  if (ret < 0)  {
    syslog(LOG_ERR, "init_nic_config failed, ret= %d\n", ret);
  }


  if (pthread_create(&tid_rx, NULL, ncsi_rx_handler, (void*) sfd) != 0) {
    syslog(LOG_ERR, "ncsi_aen_handler: pthread_create failed on ncsi_rx_handler, errno %d\n",
           errno);
    goto free_and_exit;
  }

  send_registration_msg(sfd);


  if (pthread_create(&tid_tx, NULL, ncsi_tx_handler, (void*) sfd) != 0) {
    syslog(LOG_ERR, "ncsi_aen_handler: pthread_create failed on ncsi_tx_handler, errno %d\n",
           errno);
    goto free_and_exit;
  }

  // enable platform-specific AENs
  enable_aens(sfd, aen_enable_mask);

  pthread_join(tid_rx, NULL);
  pthread_join(tid_tx, NULL);

free_and_exit:
  if (sfd)
    free (sfd);

close_and_exit:
  close(sock_fd);
  pthread_exit(NULL);
}


int
main(int argc, char * const argv[]) {
  pthread_t tid_ncsi_aen_handler;

  syslog(LOG_INFO, "started\n");

  // Create thread to handle AEN registration and Responses
  if (pthread_create(&tid_ncsi_aen_handler, NULL, ncsi_aen_handler, (void*) NULL) < 0) {
    syslog(LOG_ERR, "pthread_create failed on ncsi_handler\n");
    goto cleanup;
  }


cleanup:
  if (tid_ncsi_aen_handler > 0) {
    pthread_join(tid_ncsi_aen_handler, NULL);
  }
  syslog(LOG_INFO, "exit\n");
  return 0;
}
