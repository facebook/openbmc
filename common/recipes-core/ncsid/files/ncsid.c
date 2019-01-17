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

//#define DEBUG 0

// special  channel/cmd  used for register AEN handler with kernel
#define REG_AEN_CH  0xfa
#define REG_AEN_CMD 0xce

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


static int
prepare_ncsi_req_msg(struct msghdr *msg, uint8_t ch, uint8_t cmd,
                     uint16_t payload_len, unsigned char *payload) {
  struct sockaddr_nl *dest_addr = NULL;
  struct nlmsghdr *nlh = NULL;
  NCSI_NL_MSG_T *nl_msg = NULL;
  struct iovec *iov;
  int msg_size = offsetof(NCSI_NL_MSG_T, msg_payload) + payload_len;

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

  ret = prepare_ncsi_req_msg(&msg, REG_AEN_CH, REG_AEN_CMD, 0, NULL);
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

    /* for other types of command fallures, ignore for now */
    return 0;
  } else {
    switch (cmd) {
      case NCSI_GET_LINK_STATUS:
        ret = handle_get_link_status(resp);
        break;
      case NCSI_GET_VERSION_ID:
        ret = handle_get_version_id(resp);
        break;
      // TBD: handle other command response here

      default:
        syslog(LOG_WARNING, "unknown command response, cmd 0x%x", cmd);
        break;
    }
  }

  return ret;
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
      syslog(LOG_NOTICE, "rx: aen packet rcvd, pl_len=%d, type=0x%x",
              rcv_buf->hdr.payload_length,
              rcv_buf->msg_payload[offsetof(AEN_Packet, AEN_Type)]);
      ret = process_NCSI_AEN((AEN_Packet *)rcv_buf->msg_payload);
    } else {
      ret = process_NCSI_resp(rcv_buf);
    }

    if (ret == NCSI_IF_REINIT) {
      send_registration_msg(sfd);
      enable_aens();
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

  prepare_ncsi_req_msg(&lsts_msg, 0, NCSI_GET_LINK_STATUS, 0, NULL);
  prepare_ncsi_req_msg(&vid_msg, 0, NCSI_GET_VERSION_ID, 0, NULL);

  while (1) {
    /* send "Get Link status" message to NIC  */
    ret = sendmsg(sock_fd, &lsts_msg, 0);
    if (ret < 0) {
      syslog(LOG_ERR, "tx: failed to send lsts_msg, status ret = %d, errno=%d\n", ret, errno);
    }

    /* send "Get Version ID" message to NIC  */
    ret = sendmsg(sock_fd, &vid_msg, 0);
    if (ret < 0) {
      syslog(LOG_ERR, "tx: failed to send vid_msg, status ret = %d, errno=%d\n", ret, errno);
    }

    ret = check_ncsi_status();
    if (ret == NCSI_IF_REINIT) {
      send_registration_msg(sfd);
      enable_aens();
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
  enable_aens();

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
