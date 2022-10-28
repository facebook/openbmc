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
  #define DBG_PRINT(...) printf(__VA_ARGS__)
#else
  #define DBG_PRINT(...)
#endif

#define RECVMSG_TIMEOUT 3


// re-used from
// https://github.com/sammj/ncsi-netlink

static void free_ncsi_msg(struct ncsi_msg *msg)
{
	if (msg->msg)
		nlmsg_free(msg->msg);
	if (msg->sk)
		nl_socket_free(msg->sk);
}

static int aen_cb(struct nl_msg *msg, void *arg)
{
	struct nlmsghdr *hdr = nlmsg_hdr(msg);
	struct nlattr *tb[NCSI_ATTR_MAX + 1] = {0};
	int rc;
	size_t data_len;
	char *ncsi_rsp;
	NCSI_NL_RSP_T *dst_buf = (NCSI_NL_RSP_T *) arg;

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

	rc = genlmsg_parse(hdr, 0, tb, NCSI_ATTR_MAX, ncsi_genl_policy);
	if (rc) {
		syslog(LOG_ERR, "Failed to parse ncsi info callback\n");
		return rc;
	}

	if (!tb[NCSI_ATTR_DATA]) {
		syslog(LOG_ERR, "null data attribute\n");
		errno = EFAULT;
		return -1;
	}

	rc = nla_len(tb[NCSI_ATTR_DATA]);
	if (rc < 0) {
		syslog(LOG_ERR, "unable to get nla_len: %d, %s\n", rc, strerror(errno));
		return -1;
	}

	data_len = (size_t)rc;
	if (data_len > sizeof(dst_buf->msg_payload) - sizeof(struct eth_hdr)) {
		syslog(LOG_ERR, "data_len (%d) exceeds max buffer size (%d)\n",
		       data_len,
		       sizeof(dst_buf->msg_payload) - sizeof(struct eth_hdr));
	} else {
		ncsi_rsp = nla_data(tb[NCSI_ATTR_DATA]);
		dst_buf->hdr.payload_length = data_len;
		dst_buf->hdr.cmd = 0x00;
		// hack
		// function used to parse AEN: int is_aen_packet(AEN_Packet *buf)
		// which ncsid uses to check for AEN packet defines  AEN_Packet  as
		// ethernet frame format (in common/recipes-lib/ncsi/files/aen.h),
		// which includes 14 ethernet header
		//
		// Kernel only sends NC-SI packet (without the AEN header), so we
		// need to offset struct eth_hdr to account for ethernet frame header
		// for AEN to be properly interpreted
		memcpy(dst_buf->msg_payload + sizeof(struct eth_hdr), ncsi_rsp, data_len);
	}

	return 0;
}

int setup_ncsi_mc_socket(struct nl_sock **sk, unsigned char *dst)
{
	int rc, id = 0;
	int mcgroup = 0;

	*sk = nl_socket_alloc();
	if (!(*sk)) {
		syslog(LOG_ERR, "Could not alloc socket\n");
		return -1;
	}

	rc = genl_connect(*sk);
	if (rc) {
		syslog(LOG_ERR, "genl_connect() failed\n");
		goto err;
	}

	id = genl_ctrl_resolve(*sk, "NCSI");
	if (id < 0) {
		syslog(LOG_ERR, "Could not resolve NCSI\n");
		goto err;
	}

	// resolve AEN MC group and add to socket
	mcgroup = genl_ctrl_resolve_grp(*sk, "NCSI", NCSI_GENL_AEN_MCGROUP);
    if (mcgroup < 0) {
        syslog(LOG_ERR, "Could not resolve AEN MC group. err %d\n", mcgroup);
		goto err;
	}

    rc = nl_socket_add_memberships(*sk, mcgroup, 0);
    if (rc) {
        syslog(LOG_ERR, "Could not register to the multicast group. %d\n", rc);
		goto err;
    }

	nl_socket_disable_seq_check(*sk);
	rc = nl_socket_modify_cb(*sk, NL_CB_VALID, NL_CB_CUSTOM, aen_cb, dst);
	return 0;

err:
	if (*sk)
		nl_socket_free(*sk);
	return -1;
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
		syslog(LOG_ERR, "Could not alloc socket\n");
		return -1;
	}

	rc = genl_connect(msg->sk);
	if (rc) {
		syslog(LOG_ERR, "genl_connect() failed\n");
		goto out;
	}

	id = genl_ctrl_resolve(msg->sk, "NCSI");
	if (id < 0) {
		syslog(LOG_ERR, "Could not resolve NCSI\n");
		rc = id;
		goto out;
	}

	msg->msg = nlmsg_alloc();
	if (!msg->msg) {
		syslog(LOG_ERR, "Failed to allocate message\n");
		rc = -1;
		goto out;
	};

	msg->hdr = genlmsg_put(msg->msg, NL_AUTO_PORT, NL_AUTO_SEQ, id, 0,
			flags, cmd, 0);

	if (!msg->hdr) {
		syslog(LOG_ERR, "Failed to create header\n");
		rc = -1;
		goto out;
	}

	msg->ret = 1;
	return 0;

out:
	if (errno)
		syslog(LOG_ERR, "\t%s\n", strerror(errno));
	free_ncsi_msg(msg);
	return rc;
}


static int send_cb(struct nl_msg *msg, void *arg)
{
	struct nlmsghdr *hdr = nlmsg_hdr(msg);
	struct nlattr *tb[NCSI_ATTR_MAX + 1] = {0};
	int rc;
	size_t data_len, len;
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
		syslog(LOG_ERR, "Failed to parse ncsi info callback\n");
		return rc;
	}

	if (!tb[NCSI_ATTR_DATA]) {
		syslog(LOG_ERR, "null data attribute\n");
		errno = EFAULT;
		return -1;
	}

	rc = nla_len(tb[NCSI_ATTR_DATA]);
	if (rc < 0) {
		syslog(LOG_ERR, "unable to get nla_len: %d, %s\n", rc, strerror(errno));
		return -1;
	}

	data_len = (size_t)rc;
	if (data_len < sizeof(CTRL_MSG_HDR_t)) {
		syslog(LOG_ERR, "short ncsi data, %u\n", data_len);
		errno = ERANGE;
		return -1;
	}

	/* len includes payload + checksum + FCS */
	len = data_len - sizeof(CTRL_MSG_HDR_t);
	if (len > sizeof(ncsi_msg->rsp->msg_payload)) {
		len = sizeof(ncsi_msg->rsp->msg_payload);
	}

	ncsi_rsp = nla_data(tb[NCSI_ATTR_DATA]);
	// parse the first 16 bytes of NCSI response (the header area) to get
	//  payload length
	CTRL_MSG_HDR_t *pNcsiHdr = (CTRL_MSG_HDR_t *)(void*)(ncsi_rsp);
	ncsi_msg->rsp->hdr.payload_length = ntohs(pNcsiHdr->Payload_Length);

	// copy NC-SI response, skip NCSI header bytes
	memcpy(ncsi_msg->rsp->msg_payload, (void*)(ncsi_rsp + sizeof(CTRL_MSG_HDR_t)),
	       len);

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
	struct timeval tv = {RECVMSG_TIMEOUT, 0};

	// allocate a  contiguous buffer space to hold ncsi message
	//  (header + Control Packet payload)
	pData = calloc(1, sizeof(struct ncsi_pkt_hdr) + payload_len);
	if (!pData) {
		syslog(LOG_ERR, "Failed to allocate buffer for control packet, %s\n", strerror(errno));
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
		syslog(LOG_ERR, "Failed to add ifindex, %s\n", strerror(errno));
		goto out;
	}

	if (package >= 0) {
		rc = nla_put_u32(msg.msg, NCSI_ATTR_PACKAGE_ID, package);
		if (rc) {
			syslog(LOG_ERR, "Failed to add package id, %s\n", strerror(errno));
			goto out;
		}
	}

	rc = nla_put_u32(msg.msg, NCSI_ATTR_CHANNEL_ID, nl_msg->channel_id);
	if (rc) {
		syslog(LOG_ERR, "Failed to add channel, %s\n", strerror(errno));
	}


	hdr->type = nl_msg->cmd;
	hdr->length = htons(payload_len);  // NC-SI command payload length
	rc = nla_put(msg.msg, NCSI_ATTR_DATA,
		sizeof(struct ncsi_pkt_hdr) + payload_len, (void *)pData);
	if (rc) {
		syslog(LOG_ERR, "Failed to add opcode, %s\n", strerror(errno));
	}
	nl_socket_disable_seq_check(msg.sk);
	rc = nl_socket_modify_cb(msg.sk, NL_CB_VALID, NL_CB_CUSTOM, send_cb, &msg);
	if (rc) {
		syslog(LOG_ERR, "Failed to modify callback function, %s\n", strerror(errno));
		goto out;
	}

	rc = nl_send_auto(msg.sk, msg.msg);
	if (rc < 0) {
		syslog(LOG_ERR, "Failed to send message, %s\n", strerror(errno));
		goto out;
	}

	while (msg.ret == 1) {
		if (setsockopt(nl_socket_get_fd(msg.sk), SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv))) {
			syslog(LOG_ERR, "Failed to set SO_RCVTIMEO for receiving message");
		}
		rc = nl_recvmsgs_default(msg.sk);
		DBG_PRINT("%s, rc = %d\n", __FUNCTION__, rc);
		if (rc) {
			syslog(LOG_ERR, "Failed to receive message, rc=%d %s\n", rc, strerror(errno));
			goto out;
		}
	}

out:
	free_ncsi_msg(&msg);
	if (pData)
		free(pData);
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
    syslog(LOG_ERR, "Invalid netdev %s %s\n", nl_msg->dev_name, strerror(errno));
    return NULL;
  }

  ret_buf = calloc(1, sizeof(NCSI_NL_RSP_T));
  if (!ret_buf) {
    syslog(LOG_ERR, "Failed to allocate rspbuf %d  %s\n", sizeof(NCSI_NL_RSP_T), strerror(errno));
    return NULL;
  }

  if (run_command_send(ifindex, nl_msg, ret_buf)) {
	syslog(LOG_ERR, "run cmd send failed");
    free(ret_buf);
    return NULL;
  } else {
    return ret_buf;
  }
}

// wrapper for rcv msg
int nl_rcv_msg(struct nl_sock *sk)
{
  return nl_recvmsgs_default(sk);
}

// wrapper for freeing socket
int libnl_free_socket(struct nl_sock *sk)
{
  if (sk)
	nl_socket_free(sk);
  return 0;
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
