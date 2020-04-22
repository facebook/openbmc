/*
 * nl-wrapper.c
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
#include <errno.h>
#include <syslog.h>
#include <stdint.h>
#include <net/if.h>
#include <openbmc/ncsi.h>
#include <netlink/genl/genl.h>
#include <netlink/genl/ctrl.h>
#include <sys/utsname.h>
#include "nl-wrapper.h"

#define noDEBUG_LIBNL

#ifdef DEBUG_LIBNL
  #define DBG_PRINT(fmt, args...) printf(fmt, ##args)
#else
  #define DBG_PRINT(fmt, args...)
#endif


// re-used from
// https://github.com/sammj/ncsi-netlink

static void free_ncsi_msg(struct ncsi_msg *msg)
{
	if (msg->msg)
		nlmsg_free(msg->msg);
	if (msg->sk)
		nl_socket_free(msg->sk);
}


int setup_ncsi_message(struct ncsi_msg *msg, int cmd, int flags)
{
	int rc, id;

	if (!msg)
		return -1;

	memset(msg, 0, sizeof(*msg));
	errno = 0;

	msg->sk = nl_socket_alloc();
	if (!msg->sk) {
		fprintf(stderr, "Could not alloc socket\n");
		return -1;
	}

	rc = genl_connect(msg->sk);
	if (rc) {
		fprintf(stderr, "genl_connect() failed\n");
		goto out;
	}

	id = genl_ctrl_resolve(msg->sk, "NCSI");
	if (id < 0) {
		fprintf(stderr, "Could not resolve NCSI\n");
		rc = id;
		goto out;
	}

	msg->msg = nlmsg_alloc();
	if (!msg->msg) {
		fprintf(stderr, "Failed to allocate message\n");
		rc = -1;
		goto out;
	};

	msg->hdr = genlmsg_put(msg->msg, NL_AUTO_PORT, NL_AUTO_SEQ, id, 0,
			flags, cmd, 0);

	if (!msg->hdr) {
		fprintf(stderr, "Failed to create header\n");
		rc = -1;
		goto out;
	}

	msg->ret = 1;
	return 0;

out:
	if (errno)
		fprintf(stderr, "\t%m\n");
	free_ncsi_msg(msg);
	return rc;
}


static int send_cb(struct nl_msg *msg, void *arg)
{
	struct nlmsghdr *hdr = nlmsg_hdr(msg);
	struct nlattr *tb[NCSI_ATTR_MAX + 1] = {0};
	int rc, data_len;
	char *ncsi_rsp;
	struct ncsi_msg *ncsi_msg = (struct ncsi_msg *)arg;

	static struct nla_policy ncsi_genl_policy[NCSI_ATTR_MAX + 1] = {
		[NCSI_ATTR_IFINDEX] =      { .type = NLA_U32 },
		[NCSI_ATTR_PACKAGE_LIST] = { .type = NLA_NESTED },
		[NCSI_ATTR_PACKAGE_ID] =   { .type = NLA_U32 },
		[NCSI_ATTR_CHANNEL_ID] =   { .type = NLA_U32 },
		[NCSI_ATTR_DATA] =         { .type = NLA_BINARY },
		[NCSI_ATTR_MULTI_FLAG] =   { .type = NLA_FLAG },
		[NCSI_ATTR_PACKAGE_MASK] = { .type = NLA_U32 },
		[NCSI_ATTR_CHANNEL_MASK] = { .type = NLA_U32 },
	};

	DBG_PRINT("%s called\n", __FUNCTION__);
	rc = genlmsg_parse(hdr, 0, tb, NCSI_ATTR_MAX, ncsi_genl_policy);
	DBG_PRINT("%s rc = %d\n", __FUNCTION__, rc);
	if (rc) {
		fprintf(stderr, "Failed to parse ncsi info callback\n");
		return rc;
	}

	if (!tb[NCSI_ATTR_DATA]) {
		fprintf(stderr, "null data attribute\n");
		errno = EFAULT;
		return -1;
	}

	data_len = nla_len(tb[NCSI_ATTR_DATA]);

	if (data_len >= NCSI_MAX_RESPONSE) {
		fprintf(stderr, "data_len (%d) exceeds max buffer size (%d)\n",
            data_len, NCSI_MAX_RESPONSE);
	} else {
		ncsi_rsp = nla_data(tb[NCSI_ATTR_DATA]);
    // prase the first 16 bytes of NCSI response (the header area) to get
    //  payload length
    CTRL_MSG_HDR_t *pNcsiHdr = (CTRL_MSG_HDR_t *)(void*)(ncsi_rsp);
    ncsi_msg->rsp->hdr.payload_length = ntohs(pNcsiHdr->Payload_Length);

    // copy NC-SI response, skip NCSI header bytes
    memcpy(ncsi_msg->rsp->msg_payload, (void*)(ncsi_rsp + sizeof(CTRL_MSG_HDR_t)),
           data_len - sizeof(CTRL_MSG_HDR_t));
	}

#ifdef DEBUG_LIBNL
	int i = 0;
	DBG_PRINT("%s, data len %d\n", __FUNCTION__, data_len);
  DBG_PRINT("%s, NCSI Response len %d\n", __FUNCTION__, ncsi_msg->rsp->hdr.payload_length);
	DBG_PRINT("payload:\n");
	ncsi_rsp = nla_data(tb[NCSI_ATTR_DATA]);

	for (i = 0; i < data_len; ++i) {
		DBG_PRINT("0x%x ", *(ncsi_rsp+i));
	}
	DBG_PRINT("\n");
#endif

	// indicating callback has been completed
	ncsi_msg->ret = 0;
	return 0;
}


int run_command_send(int ifindex, NCSI_NL_MSG_T *nl_msg, NCSI_NL_RSP_T *rsp)
{
	struct ncsi_msg msg;
	struct ncsi_pkt_hdr *hdr = {0};
	int rc = 0;
	int payload_len = nl_msg->payload_length;
	int package = (nl_msg->channel_id & 0xE0) >> 5;

	uint8_t *pData, *pCtrlPktPayload;

	// allocate a  contiguous buffer space to hold ncsi message
	//  (header + Control Packet payload)
	pData = calloc(1, sizeof(struct ncsi_pkt_hdr) + payload_len);
	if (!pData) {
		fprintf(stderr, "Failed to allocate buffer for control packet, %m\n");
		goto out;
	}
	// prepare buffer to be copied to netlink msg
	hdr = (struct ncsi_pkt_hdr *)pData;
	pCtrlPktPayload = pData + sizeof(struct ncsi_pkt_hdr);
	memcpy(pCtrlPktPayload, nl_msg->msg_payload, payload_len);

	rc = setup_ncsi_message(&msg, NCSI_CMD_SEND_CMD, 0);
	if (rc)
		return -1;

	nl_msg->channel_id &= 0x1F;
	DBG_PRINT("send cmd, ifindex %d, package %d, channel %d, cmd 0x%x\n",
			ifindex, package, nl_msg->channel_id, nl_msg->cmd);

	msg.rsp = rsp;
	msg.rsp->hdr.cmd = nl_msg->cmd;

	rc = nla_put_u32(msg.msg, NCSI_ATTR_IFINDEX, ifindex);
	if (rc) {
		fprintf(stderr, "Failed to add ifindex, %m\n");
		goto out;
	}

	if (package >= 0) {
		rc = nla_put_u32(msg.msg, NCSI_ATTR_PACKAGE_ID, package);
		if (rc) {
			fprintf(stderr, "Failed to add package id, %m\n");
			goto out;
		}
	}

	rc = nla_put_u32(msg.msg, NCSI_ATTR_CHANNEL_ID, nl_msg->channel_id);
	if (rc) {
		fprintf(stderr, "Failed to add channel, %m\n");
	}


	hdr->type = nl_msg->cmd;
	hdr->length = htons(payload_len);  // NC-SI command payload length
	rc = nla_put(msg.msg, NCSI_ATTR_DATA,
		sizeof(struct ncsi_pkt_hdr) + payload_len, (void *)pData);
	if (rc) {
		fprintf(stderr, "Failed to add opcode, %m\n");
	}
	nl_socket_disable_seq_check(msg.sk);
	rc = nl_socket_modify_cb(msg.sk, NL_CB_VALID, NL_CB_CUSTOM, send_cb, &msg);
	if (rc) {
		fprintf(stderr, "Failed to modify callback function, %m\n");
		goto out;
	}

	rc = nl_send_auto(msg.sk, msg.msg);
	if (rc < 0) {
		fprintf(stderr, "Failed to send message, %m\n");
		goto out;
	}

	while (msg.ret == 1) {
		rc = nl_recvmsgs_default(msg.sk);
		DBG_PRINT("%s, rc = %d\n", __FUNCTION__, rc);
		if (rc) {
			fprintf(stderr, "Failed to receive message, rc=%d %m\n", rc);
			goto out;
		}
	}

out:
	free_ncsi_msg(&msg);
	return rc;
}

// Sending data to kernel via netlink libnl
NCSI_NL_RSP_T * send_nl_msg_libnl(NCSI_NL_MSG_T *nl_msg)
{
  NCSI_NL_RSP_T *ret_buf = NULL;
  unsigned int ifindex = 0;  // network interface (e.g. eth0)'s ifindex

  ifindex = if_nametoindex(nl_msg->dev_name);
  // if_nametoindex returns 0 on error
  if (ifindex == 0) {
    fprintf(stderr, "Invalid netdev %s %m\n", nl_msg->dev_name);
    return NULL;
  }

  ret_buf = calloc(1, sizeof(NCSI_NL_RSP_T));
  if (!ret_buf) {
    fprintf(stderr, "Failed to allocate rspbuf %d  %m\n", sizeof(NCSI_NL_RSP_T));
    return NULL;
  }

  if (run_command_send(ifindex, nl_msg, ret_buf)) {
    free(ret_buf);
    return NULL;
  } else {
    return ret_buf;
  }
}


// returns 1 if libnl is available in ncsi driver
//         0 otherwise
int islibnl(void) {
  struct utsname unamebuf;
  int ret = 0;

  ret = uname(&unamebuf);
  if ((!ret) && (strcmp(unamebuf.release, "4.1.51")))
    return 1;
  else
    return 0;
}
