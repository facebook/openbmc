/*
 * libnl-wrapper.h
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
#ifndef _NL_WRAPPER_H_
#define _NL_WRAPPER_H_

#ifdef __cplusplus
extern "C" {
#endif
#include <linux/types.h>

#define ETHERNET_HEADER_SIZE 16


enum ncsi_nl_commands {
	NCSI_CMD_UNSPEC,
	NCSI_CMD_PKG_INFO,
	NCSI_CMD_SET_INTERFACE,
	NCSI_CMD_CLEAR_INTERFACE,
	NCSI_CMD_SEND_CMD,
	NCSI_CMD_SET_PACKAGE_MASK,
	NCSI_CMD_SET_CHANNEL_MASK,

	__NCSI_CMD_AFTER_LAST,
	NCSI_CMD_MAX = __NCSI_CMD_AFTER_LAST - 1
};

enum ncsi_nl_attrs {
	NCSI_ATTR_UNSPEC,
	NCSI_ATTR_IFINDEX,
	NCSI_ATTR_PACKAGE_LIST,
	NCSI_ATTR_PACKAGE_ID,
	NCSI_ATTR_CHANNEL_ID,
	NCSI_ATTR_DATA,
	NCSI_ATTR_MULTI_FLAG,
	NCSI_ATTR_PACKAGE_MASK,
	NCSI_ATTR_CHANNEL_MASK,

	__NCSI_ATTR_AFTER_LAST,
	NCSI_ATTR_MAX = __NCSI_ATTR_AFTER_LAST - 1
};

struct ncsi_pkt_hdr {
	unsigned char mc_id;        /* Management controller ID */
	unsigned char revision;     /* NCSI version - 0x01      */
	unsigned char reserved;     /* Reserved                 */
	unsigned char id;           /* Packet sequence number   */
	unsigned char type;         /* Packet type              */
	unsigned char channel;      /* Network controller ID    */
	__be16        length;       /* Payload length           */
	__be32        reserved1[2]; /* Reserved                 */
};

struct ncsi_msg {
	struct nl_sock  *sk;
	struct nl_msg   *msg;
	struct nlmsghdr *hdr;
	NCSI_NL_RSP_T   *rsp;
	int             ret;
};


// APIs
NCSI_NL_RSP_T * send_nl_msg_libnl(NCSI_NL_MSG_T *nl_msg);
int islibnl(void);

#ifdef __cplusplus
} // extern "C"
#endif

#endif
