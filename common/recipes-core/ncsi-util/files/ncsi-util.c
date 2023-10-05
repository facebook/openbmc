/*
 * ncsi-util
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
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <syslog.h>
#include <stdint.h>
#include <stddef.h>
#include <pthread.h>
#include <time.h>
#include <unistd.h>
#include <getopt.h>
#include <sys/socket.h>
#include <sys/utsname.h>
#include <linux/netlink.h>
#include <openbmc/pal.h>
#include <openbmc/kv.h>
#include <openbmc/ncsi.h>
#include <openbmc/nl-wrapper.h>
#include <openbmc/pldm.h>
#include <openbmc/pldm_base.h>
#include <openbmc/pldm_fw_update.h>
#include "ncsi-util.h"
#include "brcm-ncsi-util.h"
#include "nvidia-ncsi-util.h"
#include <openbmc/nl-wrapper.h>

#ifndef max
#define max(a, b) ((a) > (b)) ? (a) : (b)
#endif

#define MAX_SEND_NL_MSG_RETRY 3

#define NCSI_MAX_CHANNEL 32

int nl_conf = -1;  // default value indicating auto-detection

enum {
  STATE_IDLE=0,
  STATE_LEARN_COMPONENTS,
  STATE_READY_XFER,
  STATE_DOWNLOAD,
  STATE_VERIFY,
  STATE_APPLY,
  STATE_ACTIVATE,
};

// ncsi-util API for communicating with kernel
//
//  Take a NCSI_NL_MSG_T buffer, send it to kernel, and returns reply from kernel
//   in NCSI_NL_RSP_T format
NCSI_NL_RSP_T * (*send_nl_msg)(NCSI_NL_MSG_T *nl_msg);


// Sending data to kernel via NETLINK_USER  message type
//    (legacy mode, only used in kernel v4.1)
static NCSI_NL_RSP_T *
send_nl_msg_netlink_user(NCSI_NL_MSG_T *nl_msg)
{
  int sock_fd, ret;
  struct sockaddr_nl src_addr, dest_addr;
  struct nlmsghdr *nlh = NULL;
  struct iovec iov;
  struct msghdr msg;
  size_t req_msg_size = offsetof(NCSI_NL_MSG_T, msg_payload) + nl_msg->payload_length;
  size_t msg_size = max(sizeof(NCSI_NL_RSP_T), req_msg_size);

  /* msg response from kernel */
  NCSI_NL_RSP_T *rcv_buf, *ret_buf = NULL;

  /* open NETLINK socket to send message to kernel */
  sock_fd = socket(PF_NETLINK, SOCK_RAW, NETLINK_USER);
  if (sock_fd < 0)  {
    printf("Error: failed to allocate socket\n");
    return NULL;
  }

  memset(&src_addr, 0, sizeof(src_addr));
  src_addr.nl_family = AF_NETLINK;
  src_addr.nl_pid = getpid(); /* self pid */

  bind(sock_fd, (struct sockaddr*)&src_addr, sizeof(src_addr));
  memset(&dest_addr, 0, sizeof(dest_addr));
  dest_addr.nl_family = AF_NETLINK;
  dest_addr.nl_pid = 0; /* For Linux Kernel */
  dest_addr.nl_groups = 0; /* unicast */

#ifdef DEBUG   /* debug printfs */
  printf("msg_size=%d, NLMSG_SPACE(msg_size)=%d\n",
         msg_size, NLMSG_SPACE(msg_size));
#endif
  nlh = (struct nlmsghdr *)malloc(NLMSG_SPACE(msg_size));
  if (!nlh) {
    printf("Error, failed to allocate message buffer (%d)\n",
           NLMSG_SPACE(msg_size));
    goto close_and_exit;
  }
  memset(nlh, 0, NLMSG_SPACE(msg_size));
  nlh->nlmsg_len = NLMSG_SPACE(req_msg_size);
  nlh->nlmsg_pid = getpid();
  nlh->nlmsg_flags = 0;

  /* the actual NC-SI command from user */
  memcpy(NLMSG_DATA(nlh), nl_msg, req_msg_size);

  iov.iov_base = (void *)nlh;
  iov.iov_len = nlh->nlmsg_len;

  memset(&msg, 0, sizeof(msg));
  msg.msg_name = (void *)&dest_addr;
  msg.msg_namelen = sizeof(dest_addr);
  msg.msg_iov = &iov;
  msg.msg_iovlen = 1;

  ret = sendmsg(sock_fd, &msg, 0);
  if (ret == -1) {
    printf("Sending NC-SI command error: errno=%d\n", errno);
    goto free_and_exit;
  }

  /* Read message from kernel */
  iov.iov_len = NLMSG_SPACE(msg_size);
  recvmsg(sock_fd, &msg, 0);
  rcv_buf = (NCSI_NL_RSP_T *)NLMSG_DATA(nlh);
  unsigned int response_size = sizeof(NCSI_NL_RSP_HDR_T) + rcv_buf->hdr.payload_length;

  ret_buf = (NCSI_NL_RSP_T *)malloc(response_size);
  if (!ret_buf) {
    printf("Error, failed to allocate response buffer (%d)\n", response_size);
    goto free_and_exit;
  }
  memcpy(ret_buf, rcv_buf, response_size);

free_and_exit:
  free(nlh);
close_and_exit:
  close(sock_fd);
  return ret_buf;
}

// Determine NC-SI netlink send method
static void determine_nl_method()
{
  struct utsname unamebuf;

  // netlink config - auto detection
  if (nl_conf == -1) {
    int ret = uname(&unamebuf);
    if (!ret) {
      if (!strcmp(unamebuf.release, "4.1.51"))
        nl_conf = 0;
      else
        nl_conf = 1;
    }
  }

  if (nl_conf == 0)
    send_nl_msg = send_nl_msg_netlink_user;
  else
    send_nl_msg = send_nl_msg_libnl;
}

// Initialize util arguments
static void ncsi_util_init_args(ncsi_util_args_t *util_args)
{
  memset(util_args, 0, sizeof(*util_args));
}

// Set NC-SI packet netdev
//   - If user has specified one, use it;
//   - Otherwise, if a default device is provided, use it;
//   - Otherwise, do nothing.
void ncsi_util_set_pkt_dev(NCSI_NL_MSG_T *nl_msg,
                           const ncsi_util_args_t *util_args,
                           const char *dflt_dev_name)
{
  const char *dev_name;

  if (util_args->dev_name)
    dev_name = util_args->dev_name;
  else if (dflt_dev_name)
    dev_name = dflt_dev_name;
  else
    return;

  strncpy(nl_msg->dev_name, dev_name, sizeof(nl_msg->dev_name));
  nl_msg->dev_name[sizeof(nl_msg->dev_name)-1] = '\0';
}

// Initialize NC-SI netlink message by util arguments
void ncsi_util_init_pkt_by_args(NCSI_NL_MSG_T *nl_msg,
                                const ncsi_util_args_t *util_args)
{
  ncsi_util_set_pkt_dev(nl_msg, util_args, NCSI_UTIL_DEFAULT_DEV_NAME);

  nl_msg->channel_id = util_args->channel_id;
}

// Print progress. It return the printed percentage
unsigned long ncsi_util_print_progress(unsigned long cur, unsigned long total,
                                       unsigned long last_percent,
                                       const char *prefix_str)
{
  unsigned long percent = ((unsigned long long)cur * 100) / total;

  /* Only print when percentage changes */
  if (cur && percent == last_percent)
    return last_percent;

  printf("\r%s %10lu/%lu (%lu%%)  ", prefix_str, cur, total, percent);
  fflush(stdout);
  if (percent == 100)
    printf("\n");

  return percent;
}


static void print_pldm_resp_raw(NCSI_NL_RSP_T *nl_resp)
{
  int i;
  printf("PLDM Payload\n");
  for (i = 8; i < nl_resp->hdr.payload_length; ++i)
    printf("0x%x ", nl_resp->msg_payload[i]);
  printf("\n");
}


static void print_pldm_cmd_status(NCSI_NL_RSP_T *nl_resp)
{
  // Debug code to print complete NC-SI payload
  // print_ncsi_resp(nl_resp);
  if (nl_resp->hdr.cmd == NCSI_PLDM_REQUEST) {
    printf("PLDM Completion Code = 0x%x (%s)\n",
        ncsiDecodePldmCompCode(nl_resp),
        pldm_fw_cmd_cc_to_name(ncsiDecodePldmCompCode(nl_resp)));
  }
}


// sends nl_msg containing PLDM command across NCSI interface and
//  and returns response
// Returns -1 if error occurs
static int sendPldmCmdAndCheckResp(NCSI_NL_MSG_T *nl_msg)
{
  NCSI_NL_RSP_T *nl_resp;
  int ret = 0;

  if (!nl_msg)
    return -1;

  nl_resp = send_nl_msg(nl_msg);

  if (!nl_resp) {
    return -1;
  }

  // debug prints
  print_pldm_cmd_status(nl_resp);
  ret = ncsiDecodePldmCompCode(nl_resp);

  free(nl_resp);

  return ret;
}

__attribute__((destructor))
static void reset_ncsi_lock(void) {
  kv_set("block_ncsi_xmit", "0", 0, 0);
}

static NCSI_NL_RSP_T * send_nl_msg_retry(NCSI_NL_MSG_T *nl_msg)
{
  int retry = 0;
  NCSI_NL_RSP_T *nl_resp = NULL;

  while (retry <= MAX_SEND_NL_MSG_RETRY) {
    nl_resp = send_nl_msg(nl_msg);
    if (nl_resp != NULL) {
      break;
    }
    retry++;
  }

  return nl_resp;
}

static int enable_multi_channel(char *dev_name, int channel_num, int channel_id) {
  int ret = 0;

  //Send set channel mask command to kernel via netlink libnl
  ret = send_nl_set_libnl(dev_name, channel_num, channel_id);
  if (ret < 0) {
    syslog(LOG_WARNING, "%s() Fail to enable multi_channel with number of channels %d", __func__, channel_num);
  }

  return ret;
}

static int pldm_update_fw(char *path, int pldm_bufsize, uint8_t ch)
{
#define SLEEP_TIME_MS               20  // wait time per loop in ms
  NCSI_NL_MSG_T *nl_msg = NULL;
  NCSI_NL_RSP_T *nl_resp = NULL;
  pldm_fw_pkg_hdr_t *pkgHdr;
  pldm_cmd_req pldmReq = {0};
  pldm_response *pldmRes = NULL;
  int pldmCmdStatus = 0;
  int i = 0, j = 0;
  int ret = 0;
  int waitcycle = 0;
  int waitTOsec = 0;
  int currnet_state = -1, previous_state = -1;
  char value[64];
  struct timespec ts;
#define MAX_WAIT_CYCLE 1000

  clock_gettime(CLOCK_MONOTONIC, &ts);
  snprintf(value, sizeof(value), "%lld", (uint64_t) ts.tv_sec + 600);
  kv_set("block_ncsi_xmit", value, 0, 0);

  nl_msg = calloc(1, sizeof(NCSI_NL_MSG_T));
  if (!nl_msg) {
    printf("%s, Error: failed nl_msg buffer allocation(%d)\n",
           __func__, sizeof(NCSI_NL_MSG_T));
    ret = -1;
    goto free_exit;
  }
  memset(nl_msg, 0, sizeof(NCSI_NL_MSG_T));

  pldmRes = calloc(1, sizeof(pldm_response));
  if (!pldmRes) {
    printf("%s, Error: failed pldmRes buffer allocation(%d)\n",
           __func__, sizeof(pldm_response));
    ret = -1;
    goto free_exit;
  }


  pkgHdr = pldm_parse_fw_pkg(path);
  if (!pkgHdr) {
    ret = -1;
    goto free_exit;
  }

  pldmCreateReqUpdateCmd(pkgHdr, &pldmReq, pldm_bufsize);
  printf("\n01 PldmRequestUpdateOp: payload_size=%d\n", pldmReq.payload_size);
  ret = create_ncsi_ctrl_pkt(nl_msg, ch, NCSI_PLDM_REQUEST, pldmReq.payload_size,
                       &(pldmReq.common[0]));
  if (ret) {
    goto free_exit;
  }

  if (sendPldmCmdAndCheckResp(nl_msg) != CC_SUCCESS) {
    ret = -1;
    goto free_exit;
  }

  for (i=0; i<pkgHdr->componentImageCnt; ++i) {
    memset(&pldmReq, 0, sizeof(pldm_cmd_req));
    pldmCreatePassComponentTblCmd(pkgHdr, i, pkgHdr->componentImageCnt, &pldmReq);
    printf("\n02 PldmPassComponentTableOp[%d]: payload_size=%d\n", i,
            pldmReq.payload_size);
    ret = create_ncsi_ctrl_pkt(nl_msg, ch, NCSI_PLDM_REQUEST, pldmReq.payload_size,
                         &(pldmReq.common[0]));
    if (ret) {
      goto free_exit;
    }
    if (sendPldmCmdAndCheckResp(nl_msg) != CC_SUCCESS) {
      ret = -1;
      goto free_exit;
    }
  }



  for (i=0; i<pkgHdr->componentImageCnt; ++i) {
    memset(&pldmReq, 0, sizeof(pldm_cmd_req));
    pldmCreateUpdateComponentCmd(pkgHdr, i, &pldmReq);
    printf("\n03 PldmUpdateComponentOp[%d]: payload_size=%d\n", i,
            pldmReq.payload_size);
    ret = create_ncsi_ctrl_pkt(nl_msg, ch, NCSI_PLDM_REQUEST, pldmReq.payload_size,
                         &(pldmReq.common[0]));
    if (ret) {
      goto free_exit;
    }
    if (sendPldmCmdAndCheckResp(nl_msg) != CC_SUCCESS) {
      ret = -1;
      goto free_exit;
    }
  }

  // FW data transfer
  int loopCount = 0;
  int idleCnt = 0;
  int pldmCmd = 0;
  setPldmTimeout(CMD_UPDATE_COMPONENT, &waitTOsec);
  while (idleCnt < (waitTOsec * 1000 /SLEEP_TIME_MS) ) {
//    printf("\n04 QueryPendingNcPldmRequestOp, loop=%d\n", loopCount);
    ret = create_ncsi_ctrl_pkt(nl_msg, ch, NCSI_QUERY_PENDING_NC_PLDM_REQ, 0, NULL);
    if (ret) {
      goto free_exit;
    }
    nl_resp = send_nl_msg_retry(nl_msg);
    if (!nl_resp) {
      ret = -1;
      goto free_exit;
    }
    print_pldm_cmd_status(nl_resp);

    pldmCmd = ncsiGetPldmCmd(nl_resp, &pldmReq);
    free(nl_resp);
    nl_resp = NULL;
    if (pldmCmd == -1) {
  //    printf("No pending command, loop %d\n", idleCnt);
      msleep(SLEEP_TIME_MS); // wait some time and try again
      idleCnt++;
      continue;
    } else {
      idleCnt = 0;
    }

    if ( (pldmCmd == CMD_REQUEST_FIRMWARE_DATA) ||
         (pldmCmd == CMD_TRANSFER_COMPLETE) ||
         (pldmCmd == CMD_VERIFY_COMPLETE) ||
         (pldmCmd == CMD_APPLY_COMPLETE)) {
      setPldmTimeout(pldmCmd, &waitTOsec);
      loopCount++;
      waitcycle = 0;
      pldmCmdStatus = pldmFwUpdateCmdHandler(pkgHdr, &pldmReq, pldmRes);
      ret = create_ncsi_ctrl_pkt(nl_msg, ch, NCSI_SEND_NC_PLDM_REPLY,
                                 pldmRes->resp_size, pldmRes->common);
      if (ret) {
        goto free_exit;
      }
      nl_resp = send_nl_msg_retry(nl_msg);
      if (!nl_resp) {
        ret = -1;
        goto free_exit;
      }
      //print_ncsi_resp(nl_resp);
      free(nl_resp);
      nl_resp = NULL;
      if ((pldmCmd == CMD_APPLY_COMPLETE) || (pldmCmdStatus == -1))
        break;
      if (nl_conf == 0) // Linux 4.1
        msleep(10); // add dealy to reduce retry sending request to NIC
    } else {
      printf("unknown PLDM cmd 0x%x\n", pldmCmd);
      waitcycle++;
      if (waitcycle >= MAX_WAIT_CYCLE) {
        printf("max wait cycle exceeded, exit\n");
        break;
      }
    }
  }

  // only activate FW if update loop exists with good status
  if (!pldmCmdStatus && (pldmCmd == CMD_APPLY_COMPLETE)) {
    // update successful,  activate FW
    memset(&pldmReq, 0, sizeof(pldm_cmd_req));
    pldmCreateActivateFirmwareCmd(&pldmReq);
    printf("\n05 PldmActivateFirmwareOp\n");
    ret = create_ncsi_ctrl_pkt(nl_msg, ch, NCSI_PLDM_REQUEST, pldmReq.payload_size,
                         &(pldmReq.common[0]));
    if (ret) {
      goto free_exit;
    }
    if (sendPldmCmdAndCheckResp(nl_msg) != CC_SUCCESS) {
      for (j=0; j<5; j++) {
        sleep(2);
        memset(&pldmReq, 0, sizeof(pldm_cmd_req));
        pldmCreateGetStatusCmd(&pldmReq);
        ret = create_ncsi_ctrl_pkt(nl_msg, ch, NCSI_PLDM_REQUEST, pldmReq.payload_size,
                            &(pldmReq.common[0]));
        if (ret) {
          printf("\nPldmGetStatus fail ret=%d\n",ret);
          goto free_exit;
        }
        nl_resp = send_nl_msg(nl_msg);
        if (!nl_resp) {
          printf("\nPldmGetStatus send_nl_msg fail\n");
          ret = -1;
          goto free_exit;
        }
        print_pldm_cmd_status(nl_resp);
        print_pldm_resp_raw(nl_resp);
        currnet_state = nl_resp->msg_payload[8];
        previous_state = nl_resp->msg_payload[9];
        free(nl_resp);
        nl_resp = NULL;
        if (currnet_state == STATE_IDLE && previous_state == STATE_ACTIVATE) {
          ret = 0;
          break;
        } else {
          ret = -1;
        }
      }
    }
  } else {
    printf("PLDM cmd (%d) failed (status %d), abort update\n",
      pldmCmd, pldmCmdStatus);

    // send abort update cmd
    memset(&pldmReq, 0, sizeof(pldm_cmd_req));
    pldmCreateCancelUpdateCmd(&pldmReq);
    ret = create_ncsi_ctrl_pkt(nl_msg, ch, NCSI_PLDM_REQUEST, pldmReq.payload_size,
                              &(pldmReq.common[0]));
    if (ret) {
      ret = -1;
      goto free_exit;
    }
    // ignore the return status and exit since this is on the error path
    sendPldmCmdAndCheckResp(nl_msg);
    ret = -1;
  }

free_exit:
  if (pkgHdr)
    free_pldm_pkg_data(&pkgHdr);
  if (nl_resp)
    free(nl_resp);

  if (pldmRes)
    free(pldmRes);

  if (nl_msg)
    free(nl_msg);
  return ret;
}

// Help for common command line arguments
void ncsi_util_common_usage(void)
{
  printf("       -n netdev      Specifies the net device to send command to [default=\"eth0\"]\n");
  printf("       -c channel     Specifies the NC-SI channel on the net device [default=0]\n");
  printf("       -l [config]    netlink protocol [default=auto-detect]\n");
  printf("                        0 - use NETLINK_USER (kernel 4.1)\n");
  printf("                        1 - use libnl (kernel 5.0 and above)\n");
}

static void default_ncsi_util_usage(void) {
  printf("Usage: ncsi-util [options] <cmd> \n\n");
  printf("       -h             This help\n");
  ncsi_util_common_usage();
  printf("       -m [vendor]    OEM vendor specific command. Use -h for vendor specific usage\n");
  printf("                        brcm - Broadcom OEM command\n");
  printf("                        nvidia - Nvidia OEM command\n");
  printf("       -S             show adapter statistics\n");
  printf("       -p [file]      Update NIC firmware via PLDM\n");
  printf("           -b [n]     (optional) buffer size for PLDM FW update [default=1024]\n");
  printf("       -z             send \"PLDM Cancel Update\" cmd \n");
  printf("       -s [n]         socket test\n\n");
  printf("       -t [channels]  Enable multi channels\n\n");
  printf("Sample debug commands: \n");
  printf("       ncsi-util -n eth0 -c 0 0x50 0 0 0x81 0x19 0 0 0x1b 0\n");
  printf("       ncsi-util 0x8   (AEN Enable)\n");
  printf("       ncsi-util 0xa   (Get Link Status) \n");
  printf("       ncsi-util 0x15  (Get Version ID) \n");
  printf("       ncsi-util 0x16  (Get Capabilities) \n");
  printf("       ncsi-util 0x17  (Get Parameters) \n");
  printf("       ncsi-util 0x18/0x19/0x1a (Get Controller Packet/NC-SI/Pass-through statistics)\n\n");
}

static int default_ncsi_util_handler(int argc, char **argv,
                                     ncsi_util_args_t *util_args)
{
  int i = 0;
  NCSI_NL_MSG_T *msg = NULL;
  NCSI_NL_RSP_T *rsp = NULL;
  int argflag;
  int fshowethstats = 0;
  int cancelUpdate = 0;
  int bufSize = 0;
  char *pfile = NULL;
  int fupgrade = 0;
  int ret = 0;
  int sockettest = 0;
  int channel_num = 0;

  /*
   * Handle ncsi DMTF command options
   */
  while ((argflag = getopt(argc, argv, "hs:Sp:zt:?" NCSI_UTIL_COMMON_OPT_STR)) != -1)
  {
    switch(argflag) {
    case 'h':
            default_ncsi_util_usage();
            return 0;
    case 's':
            bufSize = (int)strtoul(optarg, NULL, 0);
            sockettest = 1;
            break;
    case 'S':
            fshowethstats = 1;
            break;
    case 'p':
            bufSize = 1024; // buffer size for PLDM FW update [default=1024]
            pfile = optarg;
            printf ("Input file: \"%s\"\n", pfile);
            argflag = getopt(argc, (char **)argv, "b:");
            if (argflag == 'b') {
              bufSize = (int)strtoul(optarg, NULL, 0);
              if (bufSize <= 0) {
                printf("bufSize %d is out of range.\n", bufSize);
                goto free_err_exit;
              } else {
                printf("bufSize = %d\n", bufSize);
              }
            }
            // if invalid opt str or missing argument, getopt returns a '?'
            else if (argflag == '?') {
              goto free_err_exit;
            }
            // if "b" is not found, getopt returns -1,  in this case, continue,
            //   and uses default size
            else if (argflag == -1) {
              printf("use default buf size %d\n", bufSize);
            } else {
              goto free_err_exit;
            }
            fupgrade = 1;
            break;
    case 'z':
            cancelUpdate = 1;
            printf ("Cancel firmware update...\n");
            break;
    case 't':
            channel_num = atoi(optarg);
            if ((channel_num > 0) && (channel_num <= NCSI_MAX_CHANNEL)) {
              ret = enable_multi_channel(NCSI_UTIL_DEFAULT_DEV_NAME, channel_num, util_args->channel_id);
              if (ret < 0) {
                printf ("Fail to enable multi channel!\n");
                syslog(LOG_WARNING, "Fail to enable multi channel!");
              }
            } else {
              printf ("The number of Channels is invalid or out of range! (1 ~ 32)\n");
              /*
              Currently the maximum number of channels that NCSI driver can accept is 32,
              you need to modify the NCSI driver to support more than 32 channels.
              */
            }
            return ret;
    case NCSI_UTIL_GETOPT_COMMON_OPT_CHARS:
            // Already handled
            break;
    default :
            goto free_err_exit;
    }
  }

  msg = calloc(1, sizeof(NCSI_NL_MSG_T));
  if (!msg) {
    printf("Error: failed buffer allocation\n");
    return -1;
  }
  ncsi_util_init_pkt_by_args(msg, util_args);

  if (fshowethstats) {
    msg->cmd = NCSI_GET_CONTROLLER_PACKET_STATISTICS;
    msg->payload_length =0;
  } else if (cancelUpdate) {
    msg->cmd = NCSI_PLDM_REQUEST;
    // hard code "PLDM cancel update" for debug purpose
    msg->payload_length = 3;
    msg->msg_payload[0] = 0x84;  // Cmd req, iid 1
    msg->msg_payload[1] = PLDM_TYPE_FIRMWARE_UPDATE;
    msg->msg_payload[2] = CMD_CANCEL_UPDATE;
  } else if (!fupgrade) {
    int tmp_cmd = (int)strtoul(argv[optind++], NULL, 0);
    if (tmp_cmd <= 0xFF) {
      msg->cmd = tmp_cmd;
    } else {
      goto free_err_exit;
    }
    msg->payload_length = argc - optind;
    for (i=0; i<msg->payload_length; ++i) {
      msg->msg_payload[i] = (int)strtoul(argv[i + optind], NULL, 0);
    }
  }

  if(sockettest) {
    msg->cmd = 0xDE;
    msg->payload_length = bufSize;
    printf("socket test, size =%d\n",bufSize);
    for (i=0; i<msg->payload_length; ++i) {
      msg->msg_payload[i] = i;
    }
  }

 if (fupgrade) {
    // special case - invoke PLDM fw upgrade
    ret = pldm_update_fw(pfile, bufSize, msg->channel_id);
  } else {
    // send individual NCSI cmds
#ifdef DEBUG
  printf("debug prints:");
  printf("dev = %s\n", msg->dev_name);
  printf("cmd = 0x%x, channel = 0x%x, payload_length=0x%x\n",
         msg->cmd, msg->channel_id, msg->payload_length);
  for (i=0; i<msg->payload_length; i++) {
    if (i && !(i%16))
      printf("\n");

    printf("0x%x ", msg->msg_payload[i]);
  }
  printf("\n");
#endif
    rsp = send_nl_msg(msg);
    if (rsp) {
      if (msg->cmd == NCSI_PLDM_REQUEST) {

        if (get_cmd_status(rsp) != RESP_COMMAND_COMPLETED) {
          // command failed, likely due to device not supporting PLDM over NCSI
          print_ncsi_completion_codes(rsp);
        } else {
          // debug prints
          print_pldm_cmd_status(rsp);
          print_pldm_resp_raw(rsp);

          pldm_response *pldmRes = NULL;
          pldmRes = calloc(1, sizeof(pldm_response));
          if (!pldmRes) {
            printf("%s, Error: failed pldmRes buffer allocation(%d)\n",
                   __func__, sizeof(pldm_response));
          }
          pldmRes->resp_size = rsp->hdr.payload_length - 4;
          memcpy(&(pldmRes->common[0]), &(rsp->msg_payload[4]), pldmRes->resp_size);

          // handle command response
          pldmRespHandler(pldmRes);
        }
      } else {
        print_ncsi_resp(rsp);
      }
      free(rsp);
    } else {
      if(!sockettest)
        goto free_err_exit;
    }
  }

  free(msg);
  return ret;

free_err_exit:
  if (msg)
    free(msg);

  default_ncsi_util_usage();
  return -1;
}

static ncsi_util_vendor_t vendors[NCSI_UTIL_VENDOR_MAX] = {
  [NCSI_UTIL_VENDOR_DMTF] = {
    .name       = "dmtf",
    .handler    = default_ncsi_util_handler,
  },
  [NCSI_UTIL_VENDOR_BRCM] = {
    .name       = "brcm",
    .handler    = brcm_ncsi_util_handler,
  },
  [NCSI_UTIL_VENDOR_NVIDIA] = {
    .name       = "nvidia",
    .handler    = nvidia_ncsi_util_handler,
  },
};

// Find vendor by name
static ncsi_util_vendor_t *find_vendor_by_name(const char *name)
{
  int i;

  for (i = 0; i < NCSI_UTIL_VENDOR_MAX; i++) {
    if (!strcmp(vendors[i].name, name))
      return &vendors[i];
  }

  return NULL;
}

int main(int argc, char **argv)
{
  ncsi_util_vendor_t *vendor = &vendors[NCSI_UTIL_VENDOR_DMTF]; // default
  int argflag;
  int i;
  ncsi_util_args_t util_args;

  if (argc < 2)
    goto err_exit;

  // Initialize util args
  ncsi_util_init_args(&util_args);

  /*
   * Handle util common options.
   * It ignores errors for other options and prefixes the option string
   * with '-' to prevent getopt changing the order of the arguments in argv.
   */
  opterr = 0;
  while ((argflag = getopt(argc, argv, "-"NCSI_UTIL_COMMON_OPT_STR)) != -1)
  {
    switch(argflag) {
    case 'n':
            util_args.dev_name = optarg;
            break;
    case 'c':
            i = (int)strtoul(optarg, NULL, 0);
            if (i < 0) {
              printf("channel %d is out of range.\n", i);
              goto err_exit;
            }
            util_args.channel_id = i;
            break;
    case 'l':
            nl_conf = (int)strtoul(optarg, NULL, 0);
            if ((nl_conf < 0) || (nl_conf > 1)) {
              printf("invalid nl_conf\n");
              goto err_exit;
            }
            break;
    case 'm':
            vendor = find_vendor_by_name(optarg);
            if (!vendor) {
              printf("unknown oem vendor\n");
              goto err_exit;
            }
            break;
    default :
            break;
    }
  }

  // Determine netlink send function
  determine_nl_method();

  // Configure NC-SI lib to use printf for logging for this util
  ncsi_config_log(NCSI_LOG_METHOD_PRINTF);

  optind = 0;
  opterr = 1;
  return vendor->handler(argc, argv, &util_args);

err_exit:
  default_ncsi_util_usage();
  return -1;
}


