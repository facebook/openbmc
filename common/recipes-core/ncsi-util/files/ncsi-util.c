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
#include <linux/netlink.h>
#include <openbmc/pal.h>
#include <openbmc/ncsi.h>
#include <openbmc/pldm.h>
#include <openbmc/pldm_base.h>
#include <openbmc/pldm_fw_update.h>

#define noDEBUG   /* debug printfs */

#ifndef max
#define max(a, b) ((a) > (b)) ? (a) : (b)
#endif


NCSI_NL_RSP_T *
send_nl_msg(NCSI_NL_MSG_T *nl_msg)
{
  int sock_fd, ret;
  struct sockaddr_nl src_addr, dest_addr;
  struct nlmsghdr *nlh = NULL;
  struct iovec iov;
  struct msghdr msg;
  int req_msg_size = offsetof(NCSI_NL_MSG_T, msg_payload) + nl_msg->payload_length;
  int msg_size = max(sizeof(NCSI_NL_RSP_T), req_msg_size);

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

void print_pldm_cmd_status(NCSI_NL_RSP_T *nl_resp)
{
  // Debug code to print complete NC-SI payload
  // print_ncsi_resp(nl_resp);
  if (nl_resp->hdr.cmd == NCSI_PLDM_REQUEST) {
    printf("PLDM Completion Code = 0x%x (%s)\n",
        ncsiDecodePldmCompCode(nl_resp),
        pldm_fw_cmd_cc_to_name(ncsiDecodePldmCompCode(nl_resp)));
  }
}


int pldm_test(char *path)
{
  NCSI_NL_MSG_T *nl_msg = NULL;
  NCSI_NL_RSP_T *nl_resp = NULL;
  pldm_fw_pkg_hdr_t *pkgHdr;
  pldm_cmd_req pldmReq = {0};
  pldm_response *pldmRes = NULL;

  int i = 0;
  int ret = 0;
  nl_msg = calloc(1, sizeof(NCSI_NL_MSG_T));
  if (!nl_msg) {
    printf("%s, Error: failed nl_msg buffer allocation(%d)\n",
           __FUNCTION__, sizeof(NCSI_NL_MSG_T));
    ret = -1;
    goto free_exit;
  }
  memset(nl_msg, 0, sizeof(NCSI_NL_MSG_T));

  pldmRes = calloc(1, sizeof(pldm_response));
  if (!pldmRes) {
    printf("%s, Error: failed pldmRes buffer allocation(%d)\n",
           __FUNCTION__, sizeof(pldm_response));
  }


  pkgHdr = pldm_parse_fw_pkg(path);


  pldmCreateReqUpdateCmd(pkgHdr, &pldmReq);
  printf("\n01 PldmRequestUpdateOp: payload_size=%d\n", pldmReq.payload_size);
  ret = create_ncsi_ctrl_pkt(nl_msg, 0, NCSI_PLDM_REQUEST, pldmReq.payload_size,
                       &(pldmReq.common[0]));
  if (ret) {
    goto free_exit;
  }

  nl_resp = send_nl_msg(nl_msg);
  if (!nl_resp) {
    goto free_exit;
  }
  print_pldm_cmd_status(nl_resp);

  for (i=0; i<pkgHdr->componentImageCnt; ++i) {
    memset(&pldmReq, 0, sizeof(pldm_cmd_req));
    pldmCreatePassComponentTblCmd(pkgHdr, i, &pldmReq);
    printf("\n02 PldmPassComponentTableOp[%d]: payload_size=%d\n", i,
            pldmReq.payload_size);
    ret = create_ncsi_ctrl_pkt(nl_msg, 0, NCSI_PLDM_REQUEST, pldmReq.payload_size,
                         &(pldmReq.common[0]));
    if (ret) {
      goto free_exit;
    }
    nl_resp = send_nl_msg(nl_msg);
    if (!nl_resp) {
      goto free_exit;
    }
    print_pldm_cmd_status(nl_resp);
  }



  for (i=0; i<pkgHdr->componentImageCnt; ++i) {
    memset(&pldmReq, 0, sizeof(pldm_cmd_req));
    pldmCreateUpdateComponentCmd(pkgHdr, i, &pldmReq);
    printf("\n03 PldmUpdateComponentOp[%d]: payload_size=%d\n", i,
            pldmReq.payload_size);
    ret = create_ncsi_ctrl_pkt(nl_msg, 0, NCSI_PLDM_REQUEST, pldmReq.payload_size,
                         &(pldmReq.common[0]));
    if (ret) {
      goto free_exit;
    }
    nl_resp = send_nl_msg(nl_msg);
    if (!nl_resp) {
      goto free_exit;
    }
    print_pldm_cmd_status(nl_resp);
  }

  // FW data transfer
  int loopCount = 0;
  int idleCnt = 0;
  int pldmCmd = 0;
  while (idleCnt < 70) {
    printf("\n04 QueryPendingNcPldmRequestOp, loop=%d\n", loopCount);
    ret = create_ncsi_ctrl_pkt(nl_msg, 0, NCSI_QUERY_PENDING_NC_PLDM_REQ, 0, NULL);
    if (ret) {
      goto free_exit;
    }
    nl_resp = send_nl_msg(nl_msg);
    if (!nl_resp) {
      goto free_exit;
    }
    print_pldm_cmd_status(nl_resp);

    pldmCmd = ncsiDecodePldmCmd(nl_resp, &pldmReq);

    if (pldmCmd == -1) {
      printf("No pending command, loop %d\n", idleCnt);
      msleep(200); // wait some time and try again
      idleCnt++;
      continue;
    } else {
      idleCnt = 0;
    }

    if ( (pldmCmd == CMD_REQUEST_FIRMWARE_DATA) ||
         (pldmCmd == CMD_TRANSFER_COMPLETE) ||
         (pldmCmd == CMD_VERIFY_COMPLETE) ||
         (pldmCmd == CMD_APPLY_COMPLETE)) {
      loopCount++;
      int cmdStatus = 0;
      cmdStatus = pldmCmdHandler(pkgHdr, &pldmReq, pldmRes);
      ret = create_ncsi_ctrl_pkt(nl_msg, 0, NCSI_SEND_NC_PLDM_REPLY,
                                 pldmRes->resp_size, pldmRes->common);
      if (ret) {
        goto free_exit;
      }
      nl_resp = send_nl_msg(nl_msg);
      if (!nl_resp) {
        goto free_exit;
      }
      //print_ncsi_resp(nl_resp);
      if ((pldmCmd == CMD_APPLY_COMPLETE) || (cmdStatus == -1))
        break;
    } else {
      printf("unknown PLDM cmd 0x%x, exit\n", pldmCmd);
      break;
    }
  }

  // activate FW
  memset(&pldmReq, 0, sizeof(pldm_cmd_req));
  pldmCreateActivateFirmwareCmd(&pldmReq);
  printf("\n05 PldmActivateFirmwareOp\n");
  ret = create_ncsi_ctrl_pkt(nl_msg, 0, NCSI_PLDM_REQUEST, pldmReq.payload_size,
                       &(pldmReq.common[0]));
  if (ret) {
    goto free_exit;
  }
  nl_resp = send_nl_msg(nl_msg);
  if (!nl_resp) {
    goto free_exit;
  }
  print_ncsi_resp(nl_resp);

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

static void
showUsage(void) {
  printf("Usage: ncsi-util [options] <cmd> \n\n");
  printf("       -h             This help\n");
  printf("       -n netdev      Specifies the net device to send command to [default=\"eth0\"]\n");
  printf("       -c channel     Specifies the NC-SI channel on the net device [default=0]\n");
  printf("       -S             show adapter statistics\n");
  printf("       -p [file]      Parse and display PLDM package information\n");
  printf("       -s [n]         socket test\n");
  printf("Sample: \n");
  printf("       ncsi-util -n eth0 -c 0 0x50 0 0 0x81 0x19 0 0 0x1b 0\n");
}

int
main(int argc, char **argv) {
  int i = 0;
  NCSI_NL_MSG_T *msg = NULL;
  NCSI_NL_RSP_T *rsp = NULL;
  int argflag;
  char * netdev = NULL;
  int channel = 0;
  int fshowethstats = 0;
  int bufSize;

  if (argc < 2)
    goto err_exit;

  msg = calloc(1, sizeof(NCSI_NL_MSG_T));
  if (!msg) {
    printf("Error: failed buffer allocation\n");
    return -1;
  }
  memset(msg, 0, sizeof(NCSI_NL_MSG_T));
  while ((argflag = getopt(argc, (char **)argv, "s:p:hSn:c:?")) != -1)
  {
    switch(argflag) {
    case 's':
            bufSize = (int)strtoul(optarg, NULL, 0);
            printf("socket test, size =%d\n",bufSize);
            char *buf = malloc(bufSize);
            if (!buf) {
              printf("error: buf malloc failed\n");
              return -1;
            }
            msg->cmd = 0xDE;
            msg->payload_length = bufSize;
            for (i=0; i<msg->payload_length; ++i) {
              msg->msg_payload[i] = i;
            }
            sprintf(msg->dev_name, "eth0");
            msg->channel_id = channel;
            rsp = send_nl_msg(msg);
            if (rsp) {
              print_ncsi_resp(rsp);
              free(rsp);
            }
            free(msg);
            return 0;
    case 'n':
            netdev = strdup(optarg);
            if (netdev == NULL) {
              printf("Error: malloc fail\n");
              goto free_exit;
            }
            break;
    case 'c':
            channel = (int)strtoul(optarg, NULL, 0);
            if (channel < 0) {
              printf("channel %d is out of range.\n", channel);
              goto free_exit;
            }
            break;
    case 'S':
            fshowethstats = 1;
            break;
    case 'p':
            printf ("Input file: \"%s\"\n", optarg);
            pldm_test(optarg);
            goto free_exit;
    case 'h':
    default :
            goto free_exit;
    }
  }

  if (netdev) {
    sprintf(msg->dev_name, "%s", netdev);
  } else {
    sprintf(msg->dev_name, "eth0");
  }
  msg->channel_id = channel;

  if (fshowethstats) {
    msg->cmd = NCSI_GET_CONTROLLER_PACKET_STATISTICS;
    msg->payload_length =0;
  } else {
    msg->cmd = (int)strtoul(argv[optind++], NULL, 0);
    msg->payload_length = argc - optind;
    for (i=0; i<msg->payload_length; ++i) {
      msg->msg_payload[i] = (int)strtoul(argv[i + optind], NULL, 0);
   }
 }

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
    if (msg->cmd == NCSI_PLDM_REQUEST)
      print_pldm_cmd_status(rsp);
    else
      print_ncsi_resp(rsp);
  } else {
    goto free_exit;
  }

  free(msg);
  if (rsp)
    free(rsp);
  return 0;

free_exit:
  if (msg)
    free(msg);

  if (netdev)
    free(netdev);

err_exit:
  showUsage();
  return -1;
}
;
