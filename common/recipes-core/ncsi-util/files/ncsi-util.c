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
#include <pthread.h>
#include <time.h>
#include <unistd.h>
#include <getopt.h>
#include <sys/socket.h>
#include <linux/netlink.h>
#include <openbmc/pal.h>

#define noDEBUG   /* debug printfs */

#define NETLINK_USER 31

#define MAX_PAYLOAD 128 /* maximum payload size*/
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



int
send_nl_msg(NCSI_NL_MSG_T *nl_msg)
{
  int sock_fd;
  struct sockaddr_nl src_addr, dest_addr;
  struct nlmsghdr *nlh = NULL;
  struct iovec iov;
  struct msghdr msg;

  int i  = 0;
  int msg_size = sizeof(NCSI_NL_MSG_T);

  /* msg response from kernel */
  NCSI_NL_RSP_T *rcv_buf;

  /* open NETLINK socket to send message to kernel */
  sock_fd = socket(PF_NETLINK, SOCK_RAW, NETLINK_USER);
  if(sock_fd < 0)  {
    printf("Error: failed to allocate socket\n");
    return -1;
  }

  memset(&src_addr, 0, sizeof(src_addr));
  src_addr.nl_family = AF_NETLINK;
  src_addr.nl_pid = getpid(); /* self pid */

  bind(sock_fd, (struct sockaddr*)&src_addr, sizeof(src_addr));
  memset(&dest_addr, 0, sizeof(dest_addr));
  dest_addr.nl_family = AF_NETLINK;
  dest_addr.nl_pid = 0; /* For Linux Kernel */
  dest_addr.nl_groups = 0; /* unicast */

  nlh = (struct nlmsghdr *)malloc(NLMSG_SPACE(msg_size));
  if (!nlh) {
    printf("Error, failed to allocate message buffer\n");
    goto close_and_exit;
  }
  memset(nlh, 0, NLMSG_SPACE(msg_size));
  nlh->nlmsg_len = NLMSG_SPACE(msg_size);
  nlh->nlmsg_pid = getpid();
  nlh->nlmsg_flags = 0;

  /* the actual NC-SI command from user */
  memcpy(NLMSG_DATA(nlh), nl_msg, msg_size);

  iov.iov_base = (void *)nlh;
  iov.iov_len = nlh->nlmsg_len;
  msg.msg_name = (void *)&dest_addr;
  msg.msg_namelen = sizeof(dest_addr);
  msg.msg_iov = &iov;
  msg.msg_iovlen = 1;

  printf("Sending NC-SI command\n");
  sendmsg(sock_fd,&msg,0);

  printf("NC-SI Command Response:\n");
  /* Read message from kernel */
  recvmsg(sock_fd, &msg, 0);
  rcv_buf = (NCSI_NL_RSP_T *)NLMSG_DATA(nlh);
  for (i = 0; i < rcv_buf->payload_length; ++i) {
		if (i && !(i%16))
			printf("\n");
    printf("0x%x ", rcv_buf->msg_payload[i]);
  }
  printf("\n");

  free(nlh);
close_and_exit:
  close(sock_fd);

  return 0;
}


static void
showUsage(void) {
  printf("Usage: ncsi-util [options] <cmd> \n\n");
  printf("       -h             This help\n");
  printf("       -n netdev      Specifies the net device to send command to [default=\"eth0\"]\n");
  printf("       -c channel     Specifies the NC-SI channel on the net device [default=0]\n");
  printf("Sample: \n");
  printf("       ncsi-util -n eth0 -c 0 0x50 0 0 0x81 0x19 0 0 0x1b 0\n");
}

int
main(int argc, char **argv) {
  int i = 0;
  NCSI_NL_MSG_T *msg = NULL;
  int argflag;
  char * netdev = NULL;
  int channel = 0;

  if (argc < 2)
    goto err_exit;

  msg = calloc(1, sizeof(NCSI_NL_MSG_T));
  if (!msg) {
    printf("Error: failed buffer allocation\n");
    return -1;
  }

  while ((argflag = getopt(argc, (char **)argv, "hn:c:?")) != -1)
  {
    switch(argflag) {
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
  msg->cmd = (int)strtoul(argv[optind++], NULL, 0);
  msg->payload_length = argc - optind;
  for (i=0; i<msg->payload_length; ++i) {
    msg->msg_payload[i] = (int)strtoul(argv[i + optind], NULL, 0);
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

  send_nl_msg(msg);

  free(msg);
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
