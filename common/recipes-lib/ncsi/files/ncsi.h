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


/* This field below needs to be consistent with buffer size specified
   by kernel driver, in
   meta-aspeed/recipes-kernel/linux/files/linux-aspeed-4.1/drivers/net/ethernet/faraday/ftgmac100.c
*/
#define NCSI_MAX_PAYLOAD 1480 /* maximum payload size*/
/* max ethernet frame size = 1518 */
/* ethernet headr (14) + nc-si header (16) + nc-si payload (1480) + nc-si checksum (4) + 4 (FCS) = 1518*/

/* Maximum NC-SI netlink response
 * Kernel sends all frame data after ethernet header, including FCS,
 * as netlink response data.
 *    nc-si header (16) + nc-si payload (1480) + nc-si checksum (4) + FCS (4) = 1504
 */
#define NCSI_MAX_NL_RESPONSE (sizeof(CTRL_MSG_HDR_t) + NCSI_MAX_PAYLOAD + 4 + 4)

// NCSI Reset time. NCSI Spec specfies NIC to finish reset within 2second max,
// here we add an extra 1 sec to provide extra buffer
#define NCSI_RESET_TIMEOUT  3

// NIC manufacturer IANAs
#define BCM_IANA                                         0x0000113D
#define MLX_IANA                                         0x00008119




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

// defined in DSP0222 Table 9
typedef struct ctrl_msg_hdr {
/* 16 bytes NC-SI header */
	unsigned char  MC_ID;
	/* For NC-SI 1.0 spec, this field has to set 0x01 */
	unsigned char  Header_Revision;
	unsigned char  Reserved_1; /* Reserved has to set to 0x00 */
	unsigned char  IID; /* Instance ID */
	unsigned char  Command;
	unsigned char  Channel_ID;
	/* Payload Length = 12 bits, 4 bits are reserved */
	unsigned short Payload_Length;
	unsigned short  Reserved_2;
	unsigned short  Reserved_3;
	unsigned short  Reserved_4;
	unsigned short  Reserved_5;
} __attribute__((packed)) CTRL_MSG_HDR_t;

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
  unsigned char msg_payload[NCSI_MAX_NL_RESPONSE];
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


enum {
  RESP_COMMAND_COMPLETED = 0,
  RESP_COMMAND_FAILED,
  RESP_COMMAND_UNAVAILABLE,
  RESP_COMMAND_UNSUPPORTED,
  RESP_MAX, /* max number of response code. */
};


#define NUM_NCSI_REASON_CODE             8
#define REASON_NO_ERROR             0x0000
#define REASON_INTF_INIT_REQD       0x0001
#define REASON_PARAM_INVALID        0x0002
#define REASON_CHANNEL_NOT_RDY      0x0003
#define REASON_PKG_NOT_RDY          0x0004
#define REASON_INVALID_PAYLOAD_LEN  0x0005
#define REASON_INFO_NOT_AVAIL       0x0006
#define REASON_UNKNOWN_CMD_TYPE     0x7FFF


/* NC-SI Response Packet */
typedef struct {
/* end of NC-SI header */
	unsigned short  Response_Code;
	unsigned short  Reason_Code;
	unsigned char   Payload_Data[NCSI_MAX_PAYLOAD];
} __attribute__((packed)) NCSI_Response_Packet;

typedef struct {
  uint32_t capabilities_flags;
  uint32_t broadcast_packet_filter_capabilities;
  uint32_t multicast_packet_filter_capabilities;
  uint32_t buffering_capabilities;
  uint32_t aen_control_support;
  uint8_t  vlan_filter_cnt;
  uint8_t  mixed_filter_cnt;
  uint8_t  multicast_filter_cnt;
  uint8_t  unicast_filter_cnt;
  uint16_t reserved;
  uint8_t  vlan_mode_support;
  uint8_t  channel_cnt;
} __attribute__((packed)) NCSI_Get_Capabilities_Response;


typedef struct {
  uint8_t  mac_addr_cnt;
  uint16_t reserved0;
  uint8_t  mac_addr_flags;
  uint8_t  vlan_tag_cnt;
  uint8_t  reserved1;
  uint16_t vlan_tag_flags;
  uint32_t link_settings;
  uint32_t broadcast_packet_filter_settings;
  uint32_t configuration_flags;
  uint8_t  vlan_mode;
  uint8_t  flow_ctrl_enable;
  uint16_t reserved2;
  uint32_t aen_control;
  uint32_t mac_1;
  uint32_t mac_2;
  uint32_t mac_3;
} __attribute__((packed)) NCSI_Get_Parameters_Response;


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


enum {
  NCSI_LOG_METHOD_SYSLOG = 0,
  NCSI_LOG_METHOD_DEFAULT = NCSI_LOG_METHOD_SYSLOG,
  NCSI_LOG_METHOD_PRINTF,
  NCSI_LOG_METHOD_MAX
};

void ncsi_config_log(int log_method);
extern void (*ncsi_log)(int priority, const char *format, ...)
              __attribute__ ((__format__ (__printf__, 2, 3)));

int ncsi_init_if(int);
void handle_ncsi_config(int delay);
int getMacAddr(unsigned int *values);
int checkValidMacAddr(unsigned int *value);
int check_valid_mac_addr(void);
const char * ncsi_cmd_type_to_name(int cmd);
void print_ncsi_data(void *data, int size, int print_num, int print_offset);
void print_ncsi_completion_codes(NCSI_NL_RSP_T *rcv_buf);
int get_cmd_status(NCSI_NL_RSP_T *rcv_buf);
void print_ncsi_resp(NCSI_NL_RSP_T *rcv_buf);
int ncsi_response_handler(NCSI_NL_RSP_T *rcv_buf);
void print_get_capabilities(NCSI_NL_RSP_T *rcv_buf);
void print_get_parameters(NCSI_NL_RSP_T *rcv_buf);
void print_ncsi_controller_stats(NCSI_NL_RSP_T *rcv_buf);
void print_ncsi_stats(NCSI_NL_RSP_T *rcv_buf);
void print_passthrough_stats(NCSI_NL_RSP_T *rcv_buf);
void print_link_status(NCSI_NL_RSP_T *rcv_buf);
int handle_get_link_status(NCSI_Response_Packet *resp);
int handle_get_version_id(NCSI_Response_Packet *resp);

int create_ncsi_ctrl_pkt(NCSI_NL_MSG_T *nl_msg, uint8_t ch, uint8_t cmd,
                     uint16_t payload_len, unsigned char *payload);
unsigned char * get_ncsi_resp_payload(NCSI_Response_Packet *ncsi_resp);


#endif
