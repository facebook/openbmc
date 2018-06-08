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

//#define DEBUG 0

#define NETLINK_USER 31
#define MAX_PAYLOAD 128 /* maximum payload size*/

// special  channel/cmd  used for register AEN handler with kernel
#define REG_AEN_CH  0xfa
#define REG_AEN_CMD 0xce


typedef struct ncsi_nl_msg_t {
  char dev_name[10];
  unsigned char channel_id;
  unsigned char cmd;
  unsigned char payload_length;
  unsigned char msg_payload[MAX_PAYLOAD];
} NCSI_NL_MSG_T;

#define MAX_RESPONSE_PAYLOAD 128 /* maximum payload size*/
typedef struct ncsi_nl_response {
  unsigned char payload_length;
  unsigned char msg_payload[MAX_RESPONSE_PAYLOAD];
} NCSI_NL_RSP_T;

/* AEN Type */
#define AEN_TYPE_LINK_STATUS_CHANGE            0x0
#define AEN_TYPE_CONFIGURATION_REQUIRED        0x1
#define AEN_TYPE_HOST_NC_DRIVER_STATUS_CHANGE  0x2
#define AEN_TYPE_MEDIUM_CHANGE                0x70
#define AEN_TYPE_PENDING_PLDM_REQUEST         0x71
#define AEN_TYPE_OEM                          0x80

/* BCM-specific  OEM AEN definitions */
#define BCM_IANA                                         0x0000113D
#define BCM_HOST_ERR_TYPE_UNGRACEFUL_HOST_SHUTDOWN             0x01
#define NCSI_AEN_TYPE_OEM_BCM_HOST_ERROR      0x80
#define NCSI_AEN_TYPE_OEM_BCM_RESET_REQUIRED  0x81
#define MAX_AEN_DATA_IN_SHORT                   16

typedef struct {
/* Ethernet Header */
	unsigned char  DA[6];
	unsigned char  SA[6]; /* Network Controller SA = FF:FF:FF:FF:FF:FF */
	unsigned short EtherType; /* DMTF NC-SI */
/* AEN Packet Format */
	/* AEN HEADER */
	/* Network Controller should set this field to 0x00 */
	unsigned char  MC_ID;
	/* For NC-SI 1.0 spec, this field has to set 0x01 */
	unsigned char  Header_Revision;
	unsigned char  Reserved_1; /* Reserved has to set to 0x00 */
	unsigned char  IID;        /* Instance ID = 0 in AEN */
	unsigned char  Command;    /* AEN = 0xFF */
	unsigned char  Channel_ID;
	unsigned short Payload_Length; /* Payload Length = 4 in Network Controller AEN Packet */
	unsigned long  Reserved_2;
	unsigned long  Reserved_3;
/* end of  AEN HEADER */
	unsigned char  Reserved_4[3];
	unsigned char  AEN_Type;
	unsigned short  Optional_AEN_Data[MAX_AEN_DATA_IN_SHORT];
} __attribute__((packed)) AEN_Packet;

// Structure for netlink file descriptor and socket
typedef struct _nl_sfd_t {
  int fd;
  int sock;
} nl_sfd_t;


static void
process_NCSI_AEN(AEN_Packet *buf)
{
  unsigned long manu_id=0;
  //unsigned char host_err_event_num=0;
  unsigned char host_err_type=0;
  unsigned char host_err_len=0;
  unsigned char req_rst_type=0;
  char logbuf[512];
  unsigned char log_level = LOG_NOTICE;

#ifdef DEBUG
  // print AEN packet content
  sprintf(logbuf, "AEN Packet:");
  sprintf(logbuf + strlen(logbuf), "DA[6]: %x %x %x %x %x %x   ",
          ntohs(buf->DA[0]), ntohs(buf->DA[1]), ntohs(buf->DA[2]),
          ntohs(buf->DA[3]), ntohs(buf->DA[4]), ntohs(buf->DA[5]));
  sprintf(logbuf + strlen(logbuf), "SA[6]: %x %x %x %x %x %x   ",
          ntohs(buf->SA[0]), ntohs(buf->SA[1]), ntohs(buf->SA[2]),
          ntohs(buf->SA[3]), ntohs(buf->SA[4]), ntohs(buf->SA[5]));
  sprintf(logbuf + strlen(logbuf), "EtherType: 0x%04x   ",
          ntohs(buf->EtherType));
  sprintf(logbuf + strlen(logbuf), "MC_ID: %d   ", ntohs(buf->MC_ID));
  sprintf(logbuf + strlen(logbuf), "Header_Revision: 0x%x   ",
          ntohs(buf->Header_Revision));
  sprintf(logbuf + strlen(logbuf), "IID: %d   ", ntohs(buf->IID));
  sprintf(logbuf + strlen(logbuf), "Command: 0x%x   ", ntohs(buf->Command));
  sprintf(logbuf + strlen(logbuf), "Channel_ID: %d   ",
          ntohs(buf->Channel_ID));
  sprintf(logbuf + strlen(logbuf), "Payload_Length: %d   ",
          ntohs(buf->Payload_Length));
  sprintf(logbuf + strlen(logbuf), "AEN_Type: 0x%x   ", ntohs(buf->AEN_Type));
  for (int idx=0; idx<((buf->Payload_Length-4)/2); idx++) {
    if (idx >= MAX_AEN_DATA_IN_SHORT)
      break;
    sprintf(logbuf + strlen(logbuf), " data[%d]=0x%x",
            idx, ntohs(buf->Optional_AEN_Data[idx]));
  }
  syslog(LOG_NOTICE, "%s", logbuf);
#endif


  if ((buf->MC_ID != 0x00) ||
      (buf->Header_Revision != 0x01) ||
      (buf->IID != 0x00)) {
    // Invalid AEN packet
    syslog(LOG_NOTICE, "ncsid: Invalid NCSI AEN rcvd: MC_ID=0x%x Rev=0x%x IID=0x%x\n",
           buf->MC_ID, buf->Header_Revision, buf->IID);
    return;
  }

  buf->Payload_Length = ntohs(buf->Payload_Length);

  sprintf(logbuf, "NCSI AEN rcvd: ch=%d pl_len=%d type=0x%x",
         buf->Channel_ID, buf->Payload_Length, buf->AEN_Type);

  if (buf->AEN_Type < AEN_TYPE_OEM) {
    // DMTF standard AENs
    switch (buf->AEN_Type) {
      case AEN_TYPE_LINK_STATUS_CHANGE:
        sprintf(logbuf + strlen(logbuf), ", LinkStatus=0x%04x-%04x",
        ntohs(buf->Optional_AEN_Data[0]),
        ntohs(buf->Optional_AEN_Data[1]));
        // printk(" OemLinkStatus=0x%04x-%04x",
        //              ntohs(lp->NCSI_Aen.Optional_AEN_Data[2]),
        //              ntohs(lp->NCSI_Aen.Optional_AEN_Data[3]));
        break;

      case AEN_TYPE_CONFIGURATION_REQUIRED:
        sprintf(logbuf + strlen(logbuf), ", Config Required");
        break;

      case AEN_TYPE_HOST_NC_DRIVER_STATUS_CHANGE:
        sprintf(logbuf + strlen(logbuf), ", DriverStatus=0x%04x-%04x",
        ntohs(buf->Optional_AEN_Data[0]),
        ntohs(buf->Optional_AEN_Data[1]));
        break;

      case AEN_TYPE_MEDIUM_CHANGE:
        sprintf(logbuf + strlen(logbuf), ", Medium Change");
        break;

      case AEN_TYPE_PENDING_PLDM_REQUEST:
        sprintf(logbuf + strlen(logbuf), ", Pending PLDM Request");
        break;

      default:
        sprintf(logbuf + strlen(logbuf), ", Unknown AEN Type");
    }
  } else {
    // OEM AENs
    manu_id = ntohs(buf->Optional_AEN_Data[0]) << 16 |
              ntohs(buf->Optional_AEN_Data[1]);

    if (manu_id == BCM_IANA) {
      switch (buf->AEN_Type) {
        case NCSI_AEN_TYPE_OEM_BCM_HOST_ERROR:
          log_level = LOG_CRIT;
          sprintf(logbuf + strlen(logbuf), ", BCM Host Err");
          //host_err_event_num = ntohs(buf->Optional_AEN_Data[3])&0xFF;
          host_err_type = ntohs(buf->Optional_AEN_Data[4])>>8;
          host_err_len  = ntohs(buf->Optional_AEN_Data[4])&0xFF;
          if (host_err_type == BCM_HOST_ERR_TYPE_UNGRACEFUL_HOST_SHUTDOWN) {
            sprintf(logbuf + strlen(logbuf), ", HostId=0x%x",
            ntohs(buf->Optional_AEN_Data[5]));
            sprintf(logbuf + strlen(logbuf), " DownCnt=0x%x",
            ntohs(buf->Optional_AEN_Data[6]));
          } else {
            sprintf(logbuf + strlen(logbuf), ", Unknown HostErrType=0x%x Len=0x%x",
                   host_err_type, host_err_len);
          }
          break;

        case NCSI_AEN_TYPE_OEM_BCM_RESET_REQUIRED:
          log_level = LOG_CRIT;
          sprintf(logbuf + strlen(logbuf), ", BCM Reset Required");
          req_rst_type = ntohs(buf->Optional_AEN_Data[3])&0xFF;
          sprintf(logbuf + strlen(logbuf), ", ResetType=0x%x", req_rst_type);
          break;

        default:
          sprintf(logbuf + strlen(logbuf), ", Unknown BCM AEN Type");
      }
    } else {
      sprintf(logbuf + strlen(logbuf), ", Unknown OEM AEN, IANA=0x%lx", manu_id);
    }
  }
  syslog(log_level, "%s", logbuf);
  return;
}



// Thread to handle the incoming responses
static void*
ncsi_rx_handler(void *sfd) {
  int sock_fd = ((nl_sfd_t *)sfd)->fd;
  struct msghdr msg;
  struct iovec iov;
  int msg_size = sizeof(NCSI_NL_MSG_T);
  struct nlmsghdr *nlh = NULL;

  /* msg response from kernel */
  NCSI_NL_RSP_T *rcv_buf;
  memset(&msg, 0, sizeof(msg));

  syslog(LOG_INFO, "ncsid rx: ncsi_rx_handler thread started");

  nlh = (struct nlmsghdr *)malloc(NLMSG_SPACE(msg_size));
  if (!nlh) {
    syslog(LOG_ERR, "ncsid rx: Error, failed to allocate message buffer");
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

  while (1) {
    /* Read message from kernel */
    recvmsg(sock_fd, &msg, 0);
    rcv_buf = (NCSI_NL_RSP_T *)NLMSG_DATA(nlh);
    syslog(LOG_NOTICE, "ncsid: NCSI AEN rcvd, pl_len=%d, type=0x%x\n",
            rcv_buf->payload_length,
            rcv_buf->msg_payload[offsetof(AEN_Packet, AEN_Type)]);
    process_NCSI_AEN((AEN_Packet *)rcv_buf->msg_payload);

  }
  free(nlh);
  close(sock_fd);

  pthread_exit(NULL);
}



// Thread to setup netlink and recieve AEN
static void*
ncsi_aen_handler(void *unused) {
  int sock_fd, ret;
  struct sockaddr_nl src_addr, dest_addr;
  struct nlmsghdr *nlh = NULL;
  NCSI_NL_MSG_T *nl_msg = NULL;
  struct iovec iov;
  struct msghdr msg;
  pthread_t tid_rx;
  int msg_size = sizeof(NCSI_NL_MSG_T);
  nl_sfd_t *sfd;


  syslog(LOG_INFO, "ncsid tx: ncsi_tx_handler thread started\n");
  /* open NETLINK socket to send message to kernel */
  sock_fd = socket(PF_NETLINK, SOCK_RAW, NETLINK_USER);
  if(sock_fd < 0)  {
    syslog(LOG_ERR, "ncsid tx: Error: failed to allocate tx socket\n");
    return NULL;
  }

  memset(&msg, 0, sizeof(msg));
  memset(&src_addr, 0, sizeof(src_addr));
  src_addr.nl_family = AF_NETLINK;
  src_addr.nl_pid = getpid(); /* self pid */

  if (bind(sock_fd, (struct sockaddr*)&src_addr, sizeof(src_addr)) == -1) {
    syslog(LOG_ERR, "ncsid tx: bind socket failed\n");
    goto close_and_exit;
  };

  sfd = (nl_sfd_t *)malloc(sizeof(nl_sfd_t));
  if (!sfd) {
    syslog(LOG_ERR, "ncsid tx: malloc fail on sfd\n");
    goto close_and_exit;
  }

  sfd->fd = sock_fd;
  if (pthread_create(&tid_rx, NULL, ncsi_rx_handler, (void*) sfd) != 0) {
    syslog(LOG_ERR, "ncsid tx: pthread_create failed on ncsi_rx_handler, errno %d\n",
           errno);
    goto free_and_exit;
  }

  memset(&dest_addr, 0, sizeof(dest_addr));
  dest_addr.nl_family = AF_NETLINK;
  dest_addr.nl_pid = 0;    /* For Linux Kernel */
  dest_addr.nl_groups = 0;

  nlh = (struct nlmsghdr *)malloc(NLMSG_SPACE(msg_size));
  if (!nlh) {
    syslog(LOG_ERR, "ncsid tx: Error, failed to allocate message buffer\n");
    goto free_and_exit;
  }

  memset(nlh, 0, NLMSG_SPACE(msg_size));
  nlh->nlmsg_len = NLMSG_SPACE(msg_size);
  nlh->nlmsg_pid = getpid();
  nlh->nlmsg_flags = 0;

  nl_msg = calloc(1, sizeof(NCSI_NL_MSG_T));
  if (!nl_msg) {
    syslog(LOG_ERR, "ncsid tx: Error, failed to allocate NCSI_NL_MSG\n");
    goto free_and_exit;
  }

  /* send registration message to kernel to register ncsid as AEN handler */
  /* for now only listens on eth0 */
  sprintf(nl_msg->dev_name, "eth0");
  nl_msg->channel_id = REG_AEN_CH;
  nl_msg->cmd = REG_AEN_CMD;
  nl_msg->payload_length = 0;

  memcpy(NLMSG_DATA(nlh), nl_msg, msg_size);

  iov.iov_base = (void *)nlh;
  iov.iov_len = nlh->nlmsg_len;
  msg.msg_name = (void *)&dest_addr;
  msg.msg_namelen = sizeof(dest_addr);
  msg.msg_iov = &iov;
  msg.msg_iovlen = 1;

  syslog(LOG_INFO, "ncsid tx: registering AEN Handler\n");
  ret = sendmsg(sock_fd,&msg,0);
  if (ret < 0) {
    syslog(LOG_ERR, "ncsid tx: AEN Registration status ret = %d, errno=%d\n",
             ret, errno);
  }

  pthread_join(tid_rx, NULL);

free_and_exit:
  if (nlh)
    free(nlh);
  if (nl_msg)
    free(nl_msg);
  if (sfd)
    free (sfd);

close_and_exit:
  close(sock_fd);
  pthread_exit(NULL);
}


int
main(int argc, char * const argv[]) {
  pthread_t tid_ncsi_aen_handler;

  syslog(LOG_INFO, "ncsid:started\n");

  // Create thread to handle AEN registration and Responses
  if (pthread_create(&tid_ncsi_aen_handler, NULL, ncsi_aen_handler, (void*) NULL) < 0) {
    syslog(LOG_ERR, "ncsid: pthread_create failed on ncsi_handler\n");
    goto cleanup;
  }

cleanup:
  if (tid_ncsi_aen_handler > 0) {
    pthread_join(tid_ncsi_aen_handler, NULL);
  }
  syslog(LOG_INFO, "ncsid exit\n");
  return 0;
}
