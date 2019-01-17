/*
 *
 * Copyright 2018-present Facebook. All Rights Reserved.
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
#ifndef _NCSI_H_
#define _NCSI_H_

#define NETLINK_USER 31
#define NCSI_IF_REINIT 0xEF



#define NCSI_MAX_PAYLOAD 1088 /* maximum payload size*/
#define NCSI_MAX_RESPONSE 1024 /* maximum payload size*/

// NCSI Reset time. NCSI Spec specfies NIC to finish reset within 2second max,
// here we add an extra 1 sec to provide extra buffer
#define NCSI_RESET_TIMEOUT  3





/* NCSI Command and Response Type */
#define NCSI_CLEAR_INITIAL_STATE                 0x00
#define NCSI_SELECT_PACKAGE                      0x01
#define NCSI_DESELECT_PACKAGE                    0x02
#define NCSI_ENABLE_CHANNEL                      0x03
#define NCSI_DISABLE_CHANNEL                     0x04
#define NCSI_RESET_CHANNEL                       0x05
#define NCSI_ENABLE_CHANNEL_NETWORK_TX           0x06
#define NCSI_DISABLE_CHANNEL_NETWORK_TX          0x07
#define NCSI_AEN_ENABLE                          0x08
#define NCSI_SET_LINK                            0x09
#define NCSI_GET_LINK_STATUS                     0x0A
#define NCSI_SET_VLAN_FILTER                     0x0B
#define NCSI_ENABLE_VLAN                         0x0C
#define NCSI_DISABLE_VLAN                        0x0D
#define NCSI_SET_MAC_ADDRESS                     0x0E
        // no 0x0F
#define NCSI_ENABLE_BROADCAST_FILTERING          0x10
#define NCSI_DISABLE_BROADCAST_FILTERING         0x11
#define NCSI_ENABLE_GLOBAL_MULTICAST_FILTERING   0x12
#define NCSI_DISABLE_GLOBAL_MULTICAST_FILTERING  0x13
#define NCSI_SET_NCSI_FLOW_CONTROL               0x14
#define NCSI_GET_VERSION_ID                      0x15
#define NCSI_GET_CAPABILITIES                    0x16
#define NCSI_GET_PARAMETERS                      0x17
#define NCSI_GET_CONTROLLER_PACKET_STATISTICS    0x18
#define NCSI_GET_NCSI_STATISTICS                 0x19
#define NCSI_GET_NCSI_PASS_THROUGH_STATISTICS    0x1a

// NCSI OEM commands
#define NCSI_OEM_CMD                             0x50

// NCSI PLDM commands
#define NCSI_PLDM_REQUEST                        0x51
#define NCSI_QUERY_PENDING_NC_PLDM_REQ           0x56
#define NCSI_SEND_NC_PLDM_REPLY                  0x57


#define NUM_NCSI_CDMS 27
extern const char *ncsi_cmd_string[NUM_NCSI_CDMS];



typedef struct ncsi_nl_msg_t {
  char dev_name[10];
  unsigned char channel_id;
  unsigned char cmd;
  uint16_t payload_length;
  unsigned char msg_payload[NCSI_MAX_PAYLOAD];
} NCSI_NL_MSG_T;


typedef struct ncsi_nl_rsp_hdr_t {
  uint8_t cmd;
  uint16_t payload_length;
} __attribute__((packed)) NCSI_NL_RSP_HDR_T;

typedef struct ncsi_nl_response {
  NCSI_NL_RSP_HDR_T hdr;
  unsigned char msg_payload[NCSI_MAX_RESPONSE];
} __attribute__((packed)) NCSI_NL_RSP_T;



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

/* Get Version ID Response */
typedef struct {
  uint8_t NCSI_ver[8];
  char fw_name[12];
  uint8_t fw_ver[4];
  uint16_t PCI_DID;
  uint16_t PCI_VID;
  uint16_t PCI_SSID;
  uint16_t PCI_SVID;
  uint32_t IANA;
} __attribute__((packed)) Get_Version_ID_Response;


#define RESP_COMMAND_COMPLETED   0x0000
#define RESP_COMMAND_FAILED      0x0001
#define RESP_COMMAND_UNAVAILABLE 0x0002
#define RESP_COMMAND_UNSUPPORTED 0x0003

#define REASON_NO_ERROR             0x0000
#define REASON_INTF_INIT_REQD       0x0001
#define REASON_PARAM_INVALID        0x0002
#define REASON_CHANNEL_NOT_RDY      0x0003
#define REASON_PKG_NOT_RDY          0x0004
#define REASON_INVALID_PAYLOAD_LEN  0x0005
#define REASON_UNKNOWN_CMD_TYPE     0x7FFF


/* NC-SI Response Packet */
typedef struct {
/* end of NC-SI header */
	unsigned short  Response_Code;
	unsigned short  Reason_Code;
	unsigned char   Payload_Data[128];
} __attribute__((packed)) NCSI_Response_Packet;


typedef struct {
  uint32_t counters_cleared_from_last_read_MSB;
  uint32_t counters_cleared_from_last_read_LSB;
  uint64_t total_bytes_rcvd;
  uint64_t total_bytes_tx;
  uint64_t total_unicast_pkts_rcvd;
  uint64_t total_multicast_pkts_rcvd;
  uint64_t total_broadcast_pkts_rcvd;
  uint64_t total_unicast_pkts_tx;
  uint64_t total_multicast_pkts_tx;
  uint64_t total_broadcast_pkts_tx;
  uint32_t fcs_receive_errs;
  uint32_t alignment_errs;
  uint32_t false_carrier_detections;
  uint32_t runt_pkts_rcvd;
  uint32_t jabber_pkts_rcvd;
  uint32_t pause_xon_frames_rcvd;
  uint32_t pause_xoff_frames_rcvd;
  uint32_t pause_xon_frames_tx;
  uint32_t pause_xoff_frames_tx;
  uint32_t single_collision_tx_frames;
  uint32_t multiple_collision_tx_frames;
  uint32_t late_collision_frames;
  uint32_t excessive_collision_frames;
  uint32_t control_frames_rcvd;
  uint32_t rx_frame_64;
  uint32_t rx_frame_65_127;
  uint32_t rx_frame_128_255;
  uint32_t rx_frame_256_511;
  uint32_t rx_frame_512_1023;
  uint32_t rx_frame_1024_1522;
  uint32_t rx_frame_1523_9022;
  uint32_t tx_frame_64;
  uint32_t tx_frame_65_127;
  uint32_t tx_frame_128_255;
  uint32_t tx_frame_256_511;
  uint32_t tx_frame_512_1023;
  uint32_t tx_frame_1024_1522;
  uint32_t tx_frame_1523_9022;
  uint64_t valid_bytes_rcvd;
  uint32_t err_runt_packets_rcvd;
  uint32_t err_jabber_packets_rcvd;
} __attribute__((packed)) NCSI_Controller_Packet_Stats_Response;


typedef struct {
  uint32_t cmds_rcvd;
  uint32_t ctrl_pkts_dropped;
  uint32_t cmd_type_errs;
  uint32_t cmd_chksum_errs;
  uint32_t rx_pkts;
  uint32_t tx_pkts;
  uint32_t aens_sent;
} __attribute__((packed)) NCSI_Stats_Response;


typedef struct {
  uint64_t tx_packets_rcvd_on_ncsi;
  uint32_t tx_packets_dropped;
  uint32_t tx_channel_state_err;
  uint32_t tx_undersize_err;
  uint32_t tx_oversize_err;
  uint32_t rx_packets_rcvd_on_lan;
  uint32_t total_rx_packets_dropped;
  uint32_t rx_channel_state_err;
  uint32_t rx_undersize_err;
  uint32_t rx_oversize_err;
} __attribute__((packed)) NCSI_Passthrough_Stats_Response;


int ncsi_init_if(int);
void handle_ncsi_config(int delay);
int getMacAddr(int *values);
int checkValidMacAddr(int *value);
int check_ncsi_status(void);
void print_ncsi_resp(NCSI_NL_RSP_T *rcv_buf);
void print_ncsi_controller_stats(NCSI_NL_RSP_T *rcv_buf);
void print_ncsi_stats(NCSI_NL_RSP_T *rcv_buf);
void print_passthrough_stats(NCSI_NL_RSP_T *rcv_buf);
int handle_get_link_status(NCSI_Response_Packet *resp);
int handle_get_version_id(NCSI_Response_Packet *resp);

int create_ncsi_ctrl_pkt(NCSI_NL_MSG_T *nl_msg, uint8_t ch, uint8_t cmd,
                     uint16_t payload_len, unsigned char *payload);


#endif
