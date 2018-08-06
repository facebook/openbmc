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


//#define DEBUG 0

#define NETLINK_USER 31
#define MAX_PAYLOAD 128 /* maximum payload size*/

// special  channel/cmd  used for register AEN handler with kernel
#define REG_AEN_CH  0xfa
#define REG_AEN_CMD 0xce


/*
   Default config:
      - poll NIC status once every 60 seconds
      - log 1 NIC error entries per 30 min
*/

/* POLL nic status every N seconds */
#define NIC_STATUS_SAMPLING_DELAY  60
/* to avoid log flood when NIC reports error, we only allow 1 NIC log Entry
   per this many minutes
*/
#define NIC_LOG_PERIOD 30
#define NUM_NIC_SAMPLES  (NIC_LOG_PERIOD * (60/NIC_STATUS_SAMPLING_DELAY))



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
#define NCSI_AEN_TYPE_OEM_BCM_HOST_ERROR                       0x80
#define NCSI_AEN_TYPE_OEM_BCM_RESET_REQUIRED                   0x81
#define NCSI_AEN_TYPE_OEM_BCM_HOST_DECOMMISSIONED              0x82
#define MAX_AEN_DATA_IN_SHORT                   16

typedef union {
  struct
  {
     unsigned long link_flag :    1;
     unsigned long speed_duplex : 4;
     unsigned long auto_negotiate_en : 1;
     unsigned long auto_negoation_complete : 1;
     unsigned long parallel_detection : 1;
     unsigned long rsv : 1;
     unsigned long link_partner_1000_full_duplex : 1;
     unsigned long link_partner_1000_half_duplex : 1;
     unsigned long link_partner_100_T4 : 1;
     unsigned long link_partner_100_full_duplex : 1;
     unsigned long link_partner_100_half_duplex : 1;
     unsigned long link_partner_10_full_duplex : 1;
     unsigned long link_partner_10_half_duplex : 1;
     unsigned long tx_flow_control : 1;
     unsigned long rx_flow_control : 1;
     unsigned long link_partner_flow_control: 2;
     unsigned long serdes : 1;
     unsigned long oem_link_speed_valid : 1;
     unsigned long rsv2 : 10;
  } bits;
  unsigned long all32;
} Link_Status;

typedef union {
  struct
  {
     unsigned long host_NC_driver_status :    1;
     unsigned long rsv : 31;
  } bits;
  unsigned long all32;
} Other_Indications;

/* Get Link Status Response */
typedef struct {
  Link_Status link_status;
  Other_Indications other_indications;
  unsigned long oem_link_status;
} __attribute__((packed)) Get_Link_Status_Response;

/* NC-SI Response Packet */
typedef struct {
/* end of NC-SI header */
	unsigned short  Response_Code;
	unsigned short  Reason_Code;
	unsigned char   Payload_Data[128];
} __attribute__((packed)) NCSI_Response_Packet;


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

// use to signal all threads to exit
volatile int thread_stop = 0;

static void
process_NCSI_resp(NCSI_NL_RSP_T *buf)
{
  char logbuf[512];
  int i=0;

  // flag to ensure we only log NIC error event once per NIC_LOG_PERIOD
  static int log_event = 0;

  NCSI_Response_Packet *resp = (NCSI_Response_Packet *)buf->msg_payload;
  Get_Link_Status_Response *linkresp = (Get_Link_Status_Response *)resp->Payload_Data;
  Link_Status linkstatus;
  Other_Indications linkOther;
  linkstatus.all32 = ntohl(linkresp->link_status.all32);
  linkOther.all32 = ntohl(linkresp->other_indications.all32);

  // check NIC link status, log if it reports error
  if (linkstatus.bits.link_flag) {
    // reset our log count if good status is detected, so next error status will be logged
    log_event = 0;
  } else {
    // NIC error detected

    if ((log_event > 0) && (log_event < NUM_NIC_SAMPLES)) {
      // NIC erorr status has already been logged in this period, do not log again
      log_event++;
    } else {
      // first time NIC error has been detected in a new sampling period, log it
      i += sprintf(logbuf, "NIC error:");
      i += sprintf(logbuf + i, "Rsp:0x%04x ", ntohs(resp->Response_Code));
      i += sprintf(logbuf + i, "Rsn:0x%04x ", ntohs(resp->Reason_Code));
      i += sprintf(logbuf + i, "Link:0x%lx ", linkstatus.all32);
      i += sprintf(logbuf + i, "(LF:0x%x ", linkstatus.bits.link_flag);
      i += sprintf(logbuf + i, "SP:0x%x ",  linkstatus.bits.speed_duplex);
      i += sprintf(logbuf + i, "SD:0x%x) ", linkstatus.bits.serdes);
      i += sprintf(logbuf + i, "Other:0x%lx ", linkOther.all32);
      i += sprintf(logbuf + i, "(Driver:0x%x) ", linkOther.bits.host_NC_driver_status);
      i += sprintf(logbuf + i, "OEM:0x%lx ", (unsigned long)ntohl(linkresp->oem_link_status));
      syslog(LOG_CRIT, "%s", logbuf);

      // reset sampling counter
      log_event = 1;
    }
  }
}


// helper function to check if a given buf is a valid AEN
static int
is_aen_packet(AEN_Packet *buf)
{
  return (  (buf->MC_ID == 0x00)
         && (buf->Header_Revision == 0x01)
         && (buf->IID == 0x00));
}

// reload kernel NC-SI driver and trigger NC-SI interface initialization
static int
ncsi_init_if()
{
  char cmd[64] = {0};
  int ret = 0;
  syslog(LOG_CRIT, "ncsid: re-configure NC-SI and restart eth0 interface");

  memset(cmd, 0, sizeof(cmd));
  sprintf(cmd, "ifdown eth0; ifup eth0");
  ret = system(cmd);

  syslog(LOG_CRIT, "ncsid: re-start eth0 interface done! ret=%d", ret);

  return ret;
}


// Handles AEN type 0x01 - Cnfiguration Required
// Steps
//    1. ifdown eth0;ifup eth0    # re-init NIC NC-SI interface
//    2. restart ncsid
static void
handle_ncsi_config()
{
// NCSI Reset time. NCSI Spec specfies NIC to finish reset within 2second max,
// here we add an extra 1 sec to provide extra buffer
#define NCSI_RESET_TIMEOUT  3

  // Give NIC some time to finish its reset opeartion before  BMC sends
  // NCSI commands to re-initialize the interface
  sleep(NCSI_RESET_TIMEOUT);

  ncsi_init_if();

  // set flag indicate all threads to exit
  thread_stop = 1;
}


static int
getMacAddr(int *values) {
  FILE *fp;

  fp = fopen("/sys/class/net/eth0/address", "rt");
  if (!fp) {
    int err = errno;
    return err;
  }
  if( 6 == fscanf(fp, "%x:%x:%x:%x:%x:%x%*c",
                  &values[0], &values[1], &values[2],
                  &values[3], &values[4], &values[5] ) )
  {
#ifdef DEBUG
    syslog(LOG_CRIT, "mac check %x:%x:%x:%x:%x:%x",
               values[0], values[1], values[2], values[3],
               values[4], values[5]);
#endif
  }

  fclose(fp);
  return 0;
}



#define ZERO_OCTET_THRESHOLD 5

// rudimentary check for valid MAC:
//    invalid MAC == (number of 0 octet >= 5)
//    valid MAC   == everything else
static int
checkValidMacAddr(int *value) {
  int numZeroOctet = 0;
  int valid = 1;

  for (int i=0; i<6; ++i) {
    if (value[i] == 0)
      numZeroOctet++;
  }

  if (numZeroOctet >= ZERO_OCTET_THRESHOLD) {
    valid = 0;
  }

  return valid;
}


static int
check_ncsi_status()
{
  int values[6] = {0};
  int ret;

  ret = getMacAddr(values);

  // if fail to obtain MAC addr, do not proceed further
  if (ret)
    return ret;

  // if invalid MAC was obtained, re-init NCSI interface and
  //     check the MAC addr again
  if (!checkValidMacAddr(values)) {
    /* invalid mac */
    syslog(LOG_CRIT, "invalid MAC(%x:%x:%x:%x:%x:%x) detected, reinit NCSI",
             values[0], values[1], values[2], values[3],
             values[4], values[5]);

    // re-init NCSI interface and get MAC address again
    ncsi_init_if();
    ret = getMacAddr(values);
    if (!ret && checkValidMacAddr(values)) {
      syslog(LOG_CRIT, "Valid MAC(%x:%x:%x:%x:%x:%x) obtained after NCSI \t"
             "re-init, restarting ncsid",
             values[0], values[1], values[2], values[3],
             values[4], values[5]);
      // set flag indicate all threads to exit and re-init NCSID
      thread_stop = 1;
    } else {
      syslog(LOG_CRIT, "Invalid MAC(%x:%x:%x:%x:%x:%x), after NCSI re-init",
            values[0], values[1], values[2], values[3],
            values[4], values[5]);
    }
  }

  return 0;
}



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
  int i=0;

#ifdef DEBUG
  // print AEN packet content
  i += sprintf(logbuf, "AEN Packet:");
  i += sprintf(logbuf + i, "DA[6]: %x %x %x %x %x %x   ",
          ntohs(buf->DA[0]), ntohs(buf->DA[1]), ntohs(buf->DA[2]),
          ntohs(buf->DA[3]), ntohs(buf->DA[4]), ntohs(buf->DA[5]));
  i += sprintf(logbuf + i, "SA[6]: %x %x %x %x %x %x   ",
          ntohs(buf->SA[0]), ntohs(buf->SA[1]), ntohs(buf->SA[2]),
          ntohs(buf->SA[3]), ntohs(buf->SA[4]), ntohs(buf->SA[5]));
  i += sprintf(logbuf + i, "EtherType: 0x%04x   ",
          ntohs(buf->EtherType));
  i += sprintf(logbuf + i, "MC_ID: %d   ", ntohs(buf->MC_ID));
  i += sprintf(logbuf + i, "Header_Revision: 0x%x   ",
          ntohs(buf->Header_Revision));
  i += sprintf(logbuf + i, "IID: %d   ", ntohs(buf->IID));
  i += sprintf(logbuf + i, "Command: 0x%x   ", ntohs(buf->Command));
  i += sprintf(logbuf + i, "Channel_ID: %d   ",
          ntohs(buf->Channel_ID));
  i += sprintf(logbuf + i, "Payload_Length: %d   ",
          ntohs(buf->Payload_Length));
  i += sprintf(logbuf + i, "AEN_Type: 0x%x   ", ntohs(buf->AEN_Type));
  for (int idx=0; idx<((buf->Payload_Length-4)/2); idx++) {
    if (idx >= MAX_AEN_DATA_IN_SHORT)
      break;
    i += sprintf(logbuf + i, " data[%d]=0x%x",
            idx, ntohs(buf->Optional_AEN_Data[idx]));
  }
  syslog(LOG_NOTICE, "%s", logbuf);
#endif


  if (!is_aen_packet(buf)) {
    // Invalid AEN packet
    syslog(LOG_NOTICE, "ncsid: Invalid NCSI AEN rcvd: MC_ID=0x%x Rev=0x%x IID=0x%x\n",
           buf->MC_ID, buf->Header_Revision, buf->IID);
    return;
  }

  buf->Payload_Length = ntohs(buf->Payload_Length);

  i = sprintf(logbuf, "NCSI AEN rcvd: ch=%d pLen=%d type=0x%x",
         buf->Channel_ID, buf->Payload_Length, buf->AEN_Type);

  if (buf->AEN_Type < AEN_TYPE_OEM) {
    // DMTF standard AENs
    switch (buf->AEN_Type) {
      case AEN_TYPE_LINK_STATUS_CHANGE:
        log_level = LOG_CRIT;
        i += sprintf(logbuf + i, ", LinkStatus=0x%04x-%04x",
        ntohs(buf->Optional_AEN_Data[0]),
        ntohs(buf->Optional_AEN_Data[1]));
        // printk(" OemLinkStatus=0x%04x-%04x",
        //              ntohs(lp->NCSI_Aen.Optional_AEN_Data[2]),
        //              ntohs(lp->NCSI_Aen.Optional_AEN_Data[3]));
        break;

      case AEN_TYPE_CONFIGURATION_REQUIRED:
        log_level = LOG_CRIT;
        i += sprintf(logbuf + i, ", Configuration Required");
        syslog(log_level, "%s", logbuf);

        handle_ncsi_config();  // this function will signal ncsid to exit
        break;

      case AEN_TYPE_HOST_NC_DRIVER_STATUS_CHANGE:
        log_level = LOG_CRIT;
        i += sprintf(logbuf + i, ", DriverStatus=0x%04x-%04x",
        ntohs(buf->Optional_AEN_Data[0]),
        ntohs(buf->Optional_AEN_Data[1]));
        break;

      case AEN_TYPE_MEDIUM_CHANGE:
        i += sprintf(logbuf + i, ", Medium Change");
        break;

      case AEN_TYPE_PENDING_PLDM_REQUEST:
        i += sprintf(logbuf + i, ", Pending PLDM Request");
        break;

      default:
        i += sprintf(logbuf + i, ", Unknown AEN Type");
    }
  } else {
    // OEM AENs
    manu_id = ntohs(buf->Optional_AEN_Data[0]) << 16 |
              ntohs(buf->Optional_AEN_Data[1]);

    if (manu_id == BCM_IANA) {
      unsigned char aen_iid = buf->Reserved_4[0];
      i += sprintf(logbuf + i, " iid=0x%02x", aen_iid);
      switch (buf->AEN_Type) {
        case NCSI_AEN_TYPE_OEM_BCM_HOST_ERROR:
          log_level = LOG_CRIT;
          i += sprintf(logbuf + i, ", BCM Host Err");
          //host_err_event_num = ntohs(buf->Optional_AEN_Data[3])&0xFF;
          host_err_type = ntohs(buf->Optional_AEN_Data[4])>>8;
          host_err_len  = ntohs(buf->Optional_AEN_Data[4])&0xFF;
          if (host_err_type == BCM_HOST_ERR_TYPE_UNGRACEFUL_HOST_SHUTDOWN) {
            i += sprintf(logbuf + i, ", HostId=0x%x",
            ntohs(buf->Optional_AEN_Data[5]));
            i += sprintf(logbuf + i, " DownCnt=0x%04x",
            ntohs(buf->Optional_AEN_Data[6]));
          } else {
            i += sprintf(logbuf + i, ", Unknown HostErrType=0x%x Len=0x%x",
                   host_err_type, host_err_len);
          }
          break;

        case NCSI_AEN_TYPE_OEM_BCM_RESET_REQUIRED:
          log_level = LOG_CRIT;
          i += sprintf(logbuf + i, ", BCM Reset Required");
          req_rst_type = ntohs(buf->Optional_AEN_Data[3])&0xFF;
          i += sprintf(logbuf + i, ", ResetType=0x%02x", req_rst_type);
          break;

       case NCSI_AEN_TYPE_OEM_BCM_HOST_DECOMMISSIONED:
          log_level = LOG_CRIT;
          i += sprintf(logbuf + i, ", BCM Host Decommissioned");
          i += sprintf(logbuf + i, ", HostId=0x%x",
               ntohs(buf->Optional_AEN_Data[3]));
          break;

        default:
          log_level = LOG_CRIT;
          i += sprintf(logbuf + i, ", Unknown BCM AEN Type");
      }
    } else {
      log_level = LOG_CRIT;
      i += sprintf(logbuf + i, ", Unknown OEM AEN, IANA=0x%lx", manu_id);
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

  while (! thread_stop) {
    /* Read message from kernel */
    recvmsg(sock_fd, &msg, 0);
    rcv_buf = (NCSI_NL_RSP_T *)NLMSG_DATA(nlh);
    if (is_aen_packet((AEN_Packet *)rcv_buf->msg_payload)) {
      syslog(LOG_NOTICE, "ncsid rx: aen packet rcvd, pl_len=%d, type=0x%x",
              rcv_buf->payload_length,
              rcv_buf->msg_payload[offsetof(AEN_Packet, AEN_Type)]);
      process_NCSI_AEN((AEN_Packet *)rcv_buf->msg_payload);
    } else {
      process_NCSI_resp(rcv_buf);
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
  struct sockaddr_nl dest_addr;
  struct nlmsghdr *nlh = NULL;
  NCSI_NL_MSG_T *nl_msg = NULL;
  struct iovec iov;
  struct msghdr msg;
  int msg_size = sizeof(NCSI_NL_MSG_T);


  syslog(LOG_INFO, "ncsid: ncsi_tx_handler thread started");
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
  nl_msg->channel_id = 0;
  nl_msg->cmd = 0x0a; // get link status
  nl_msg->payload_length = 0;

  memcpy(NLMSG_DATA(nlh), nl_msg, msg_size);

  iov.iov_base = (void *)nlh;
  iov.iov_len = nlh->nlmsg_len;
  msg.msg_name = (void *)&dest_addr;
  msg.msg_namelen = sizeof(dest_addr);
  msg.msg_iov = &iov;
  msg.msg_iovlen = 1;


  while (! thread_stop) {
    /* send "Get Link status" message to NIC  */
    ret = sendmsg(sock_fd,&msg,0);
    if (ret < 0) {
      syslog(LOG_ERR, "ncsid tx: failed to send cmd, status ret = %d, errno=%d\n",
               ret, errno);
    }

    ret = check_ncsi_status();
    sleep(NIC_STATUS_SAMPLING_DELAY);
  }

free_and_exit:
  free(nlh);
  close(sock_fd);

  pthread_exit(NULL);
}


// enable platform-specific AENs
static void
enable_aens(void) {
  char cmd[64] = {0};

  memset(cmd, 0, sizeof(cmd));
  sprintf(cmd, "/usr/local/bin/enable-aen.sh");
  system(cmd);

  return;
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
  pthread_t tid_tx;
  int msg_size = sizeof(NCSI_NL_MSG_T);
  nl_sfd_t *sfd;


  syslog(LOG_INFO, "ncsid ncsi_aen_handler: ncsi_tx_handler thread started\n");
  /* open NETLINK socket to send message to kernel */
  sock_fd = socket(PF_NETLINK, SOCK_RAW, NETLINK_USER);
  if(sock_fd < 0)  {
    syslog(LOG_ERR, "ncsid ncsi_aen_handler: Error: failed to allocate tx socket\n");
    return NULL;
  }

  memset(&msg, 0, sizeof(msg));
  memset(&src_addr, 0, sizeof(src_addr));
  src_addr.nl_family = AF_NETLINK;
  src_addr.nl_pid = getpid(); /* self pid */

  if (bind(sock_fd, (struct sockaddr*)&src_addr, sizeof(src_addr)) == -1) {
    syslog(LOG_ERR, "ncsid ncsi_aen_handler: bind socket failed\n");
    goto close_and_exit;
  };

  sfd = (nl_sfd_t *)malloc(sizeof(nl_sfd_t));
  if (!sfd) {
    syslog(LOG_ERR, "ncsid ncsi_aen_handler: malloc fail on sfd\n");
    goto close_and_exit;
  }

  sfd->fd = sock_fd;
  if (pthread_create(&tid_rx, NULL, ncsi_rx_handler, (void*) sfd) != 0) {
    syslog(LOG_ERR, "ncsid ncsi_aen_handler: pthread_create failed on ncsi_rx_handler, errno %d\n",
           errno);
    goto free_and_exit;
  }

  memset(&dest_addr, 0, sizeof(dest_addr));
  dest_addr.nl_family = AF_NETLINK;
  dest_addr.nl_pid = 0;    /* For Linux Kernel */
  dest_addr.nl_groups = 0;

  nlh = (struct nlmsghdr *)malloc(NLMSG_SPACE(msg_size));
  if (!nlh) {
    syslog(LOG_ERR, "ncsid ncsi_aen_handler: Error, failed to allocate message buffer\n");
    goto free_and_exit;
  }

  memset(nlh, 0, NLMSG_SPACE(msg_size));
  nlh->nlmsg_len = NLMSG_SPACE(msg_size);
  nlh->nlmsg_pid = getpid();
  nlh->nlmsg_flags = 0;

  nl_msg = calloc(1, sizeof(NCSI_NL_MSG_T));
  if (!nl_msg) {
    syslog(LOG_ERR, "ncsid ncsi_aen_handler: Error, failed to allocate NCSI_NL_MSG\n");
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

  syslog(LOG_INFO, "ncsid ncsi_aen_handler: registering AEN Handler\n");
  ret = sendmsg(sock_fd,&msg,0);
  if (ret < 0) {
    syslog(LOG_ERR, "ncsid ncsi_aen_handler: AEN Registration status ret = %d, errno=%d\n",
             ret, errno);
  }

  if (pthread_create(&tid_tx, NULL, ncsi_tx_handler, (void*) sfd) != 0) {
    syslog(LOG_ERR, "ncsid ncsi_aen_handler: pthread_create failed on ncsi_tx_handler, errno %d\n",
           errno);
    goto free_and_exit;
  }

  // enable platform-specific AENs
  enable_aens();


  pthread_join(tid_rx, NULL);
  pthread_join(tid_tx, NULL);

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
