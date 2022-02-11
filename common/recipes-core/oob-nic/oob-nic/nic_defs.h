/*
 * Copyright 2014-present Facebook. All Rights Reserved.
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
#ifndef NIC_DEFS_H
#define NIC_DEFS_H

#include <stdint.h>

#define OOB_NIC_PKT_FRAGMENT_SIZE 240
#define NIC_PKT_SIZE_MAX 1536

/** Get System MAC Address */
#define NIC_READ_MAC_CMD 0xD4
#define NIC_READ_MAC_RES_OPT 0xD4
#define NIC_READ_MAC_RES_LEN 7

/** Read Status */
#define NIC_READ_STATUS_CMD 0xDE
#define NIC_READ_STATUS_RES_OPT 0xDD
#define NIC_READ_STATUS_RES_LEN 3

struct oob_nic_status_t {
  uint8_t ons_byte1;
  uint8_t ons_byte2;
};

#define NIC_STATUS_D1_POWER_DR 0x00
#define NIC_STATUS_D1_POWER_D0U 0x01
#define NIC_STATUS_D1_POWER_D0 0x10
#define NIC_STATUS_D1_POWER_D3 0x11
#define NIC_STATUS_D1_PORT_MSB (0x1 << 2)
#define NIC_STATUS_D1_INIT (0x1 << 3)
#define NIC_STATUS_D1_FORCE_UP (0x1 << 4)
#define NIC_STATUS_D1_LINK (0x1 << 5)
#define NIC_STATUS_D1_TCO_CMD_ABORT (0x1 << 6)
#define NIC_STATUS_D1_PORT_LSB (0x1 << 7)

#define NIC_STATUS_D2_ICR (0x1 << 1)
#define NIC_STATUS_D2_IPI (0x1 << 2)
#define NIC_STATUS_D2_DRV_VALID (0x1 << 3)

/** Receive TCO Packet */
#define NIC_READ_PKT_CMD 0xC0
#define NIC_READ_PKT_RES_FIRST_OPT 0x90
#define NIC_READ_PKT_RES_MIDDLE_OPT 0x10
#define NIC_READ_PKT_RES_LAST_OPT 0x50
#define NIC_READ_PKT_RES_LAST_LEN 17

/** Transmit Packet */
#define NIC_WRITE_PKT_SINGLE_CMD 0xC4
#define NIC_WRITE_PKT_FIRST_CMD 0x84
#define NIC_WRITE_PKT_MIDDLE_CMD 0x04
#define NIC_WRITE_PKT_LAST_CMD 0x44

/** Management Control */
#define NIC_WRITE_MNG_CTRL_CMD 0xC1

#define NIC_MNG_CTRL_KEEP_LINK_UP_NUM 0x00
#define NIC_MNG_CTRL_KEEP_LINK_UP_ENABLE 0x01
#define NIC_MNG_CTRL_KEEP_LINK_UP_DISABLE 0x00

/** Update MNG RCV Filter Parameters */
#define NIC_WRITE_FILTER_CMD 0xCC

#define NIC_FILTER_MNG_ONLY_NUM 0xF
#define NIC_FILTER_MNG_ONLY_FILTER0 (0x1)
#define NIC_FILTER_MNG_ONLY_FILTER1 (0x1 << 1)
#define NIC_FILTER_MNG_ONLY_FILTER2 (0x1 << 2)
#define NIC_FILTER_MNG_ONLY_FILTER3 (0x1 << 3)
#define NIC_FILTER_MNG_ONLY_FILTER4 (0x1 << 4)

#define NIC_FILTER_MAC_NUM 0x66
#define NIC_FILTER_MAC_PAIR0 0
#define NIC_FILTER_MAC_PAIR1 1
#define NIC_FILTER_MAC_PAIR2 2
#define NIC_FILTER_MAC_PAIR3 3

#define NIC_FILTER_ETHERTYPE_NUM 0x67
#define NIC_FILTER_ETHERTYPE0 0
#define NIC_FILTER_ETHERTYPE1 1
#define NIC_FILTER_ETHERTYPE2 2
#define NIC_FILTER_ETHERTYPE3 3

#define NIC_FILTER_DECISION_EXT_NUM 0x68
#define NIC_FILTER_MDEF0 0      /* index 0 */
#define NIC_FILTER_MDEF1 1      /* index 1 */
#define NIC_FILTER_MDEF2 2      /* index 2 */
#define NIC_FILTER_MDEF3 3      /* index 3 */
#define NIC_FILTER_MDEF4 4      /* index 4 */
#define NIC_FILTER_MDEF5 5      /* index 5 */
#define NIC_FILTER_MDEF6 6      /* index 6 */
#define NIC_FILTER_MDEF7 7      /* index 7 */

#define NIC_FILTER_MDEF_MAC_AND_OFFSET 0
#define NIC_FILTER_MDEF_BCAST_AND_OFFSET 4
#define NIC_FILTER_MDEF_VLAN_AND_OFFSET 5
#define NIC_FILTER_MDEF_IPV4_AND_OFFSET 13
#define NIC_FILTER_MDEF_IPV6_AND_OFFSET 17
#define NIC_FILTER_MDEF_MAC_OR_OFFSET 21
#define NIC_FILTER_MDEF_BCAST_OR_OFFSET 25
#define NIC_FILTER_MDEF_MCAST_AND_OFFSET 26
#define NIC_FILTER_MDEF_ARP_REQ_OR_OFFSET 27
#define NIC_FILTER_MDEF_ARP_RES_OR_OFFSET 28
#define NIC_FILTER_MDEF_NBG_OR_OFFSET 29
#define NIC_FILTER_MDEF_PORT298_OR_OFFSET 30
#define NIC_FILTER_MDEF_PORT26F_OR_OFFSET 31

#define NIC_FILTER_MDEF_EXT_ETHTYPE_AND_OFFSET 0
#define NIC_FILTER_MDEF_EXT_ETHTYPE_OR_OFFSET 8
#define NIC_FILTER_MDEF_EXT_FLEX_PORT_OR_OFFSET 16
#define NIC_FILTER_MDEF_EXT_FLEX_TCO_OR_OFFSET 24
#define NIC_FILTER_MDEF_EXT_NCSI_DISABLE_OFFSET 28
#define NIC_FILTER_MDEF_EXT_FLOW_CONTROL_DISCARD_OFFSET 29
#define NIC_FILTER_MDEF_EXT_NET_EN_OFFSET 30
#define NIC_FILTER_MDEF_EXT_HOST_EN_OFFSET 31

#define NIC_FILTER_MDEF_BIT(offset) ((0x1) << (offset))
#define NIC_FILTER_MDEF_BIT_VAL(offset, val) ((0x1) << ((offset) + (val)))

/** Receive Enable */
#define NIC_WRITE_RECV_ENABLE_CMD 0xCA
#define NIC_WRITE_RECV_ENABLE_LEN_MAX 14

#define NIC_WRITE_RECV_ENABLE_EN        0x1
#define NIC_WRITE_RECV_ENABLE_ALL       (0x1 << 1)
#define NIC_WRITE_RECV_ENABLE_STA       (0x1 << 2)
#define NIC_WRITE_RECV_ENABLE_ARP_RES   (0x1 << 3)
#define NIC_WRITE_RECV_ENABLE_NM_ALERT  (0x00 << 4)
#define NIC_WRITE_RECV_ENABLE_NM_ASYNC  (0x01 << 4)
#define NIC_WRITE_RECV_ENABLE_NM_DIRECT (0x02 << 4)
#define NIC_WRITE_RECV_ENABLE_NM_UNSUPP (0x03 << 4)
#define NIC_WRITE_RECV_ENABLE_RESERVED  (0x1 << 6)
#define NIC_WRITE_RECV_ENABLE_CBDM      (0x1 << 7)

#endif
