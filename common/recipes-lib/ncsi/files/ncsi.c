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
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <syslog.h>
#include <pthread.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/time.h>
#include <unistd.h>
#include <stdint.h>
#include <arpa/inet.h>
#include "ncsi.h"
#include <inttypes.h>
#include <locale.h>
#include <fcntl.h>

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(_a) (sizeof(_a) / sizeof((_a)[0]))
#endif /* ARRAY_SIZE */


#define NIC_FW_VER_PATH "/tmp/cache_store/nic_fw_ver"

// NCSI response code string
const char *ncsi_resp_string[RESP_MAX] = {
  "COMMAND_COMPLETED",
  "COMMAND_FAILED",
  "COMMAND_UNAVAILABLE",
  "COMMAND_UNSUPPORTED",
};

// NCSI reason code string
const char *ncsi_reason_string[NUM_NCSI_REASON_CODE] = {
  "NO_ERROR",
  "INTF_INIT_REQD",
  "PARAM_INVALID",
  "CHANNEL_NOT_RDY",
  "PKG_NOT_RDY",
  "INVALID_PAYLOAD_LEN",
  "INFO_NOT_AVAIL",
  "UNKNOWN_CMD_TYPE",
};

// NCSI command name
const char *ncsi_cmd_string[NUM_NCSI_CDMS] = {
  "CLEAR_INITIAL_STATE",
  "SELECT_PACKAGE",
  "DESELECT_PACKAGE",
  "ENABLE_CHANNEL",
  "DISABLE_CHANNEL",
  "RESET_CHANNEL",
  "ENABLE_CHANNEL_NETWORK_TX",
  "DISABLE_CHANNEL_NETWORK_TX",
  "AEN_ENABLE",
  "SET_LINK",
  "GET_LINK_STATUS",
  "SET_VLAN_FILTER",
  "ENABLE_VLAN",
  "DISABLE_VLAN",
  "SET_MAC_ADDRESS",
  "invalid",  // no command 0x0f
  "ENABLE_BROADCAST_FILTERING",
  "DISABLE_BROADCAST_FILTERING",
  "ENABLE_GLOBAL_MULTICAST_FILTERING",
  "DISABLE_GLOBAL_MULTICAST_FILTERING",
  "SET_NCSI_FLOW_CONTROL",
  "GET_VERSION_ID",
  "GET_CAPABILITIES",
  "GET_PARAMETERS",
  "GET_CONTROLLER_PACKET_STATISTICS",
  "GET_NCSI_STATISTICS",
  "GET_NCSI_PASS_THROUGH_STATISTICS",
};


// Get Link status response
const char *link_speed_string[] = {
  "n/a",
  "10BASE-T half-duplex",
  "10BASE-T full-duplex",
  "100BASE-TX half-duplex",
  "100BASE-T4",
  "100BASE-TX full-duplex",
  "1000BASE-T half-duplex",
  "1000BASE-T full-duplex",
  "10Gbps",
  "20Gbps",
  "25Gbps",
  "40Gbps",
  "50Gbps",
  "100Gbps",
  "2.5Gbps",
  "reserved",
};

// NC-SI lib logging function.
// If the logging method is printf, it will add '\n' for the message.
void (*ncsi_log)(int priority, const char *format, ...)
          __attribute__ ((__format__ (__printf__, 2, 3))) = syslog;

// NC-SI printf
// priority - syslog priority, e.g. LOG_ERR, LOG_WARINING, ...
static void ncsi_printf(int priority __attribute__((unused)), const char *format, ...)
{
  va_list ap;

  va_start(ap, format);
  vprintf(format, ap);
  va_end(ap);
  printf("\n");
}

// Configure NC-SI lib logging
void ncsi_config_log(int log_method)
{
  switch (log_method) {
    case NCSI_LOG_METHOD_SYSLOG:
      ncsi_log = syslog;
      break;
    case NCSI_LOG_METHOD_PRINTF:
      ncsi_log = ncsi_printf;
      break;
    default:
      break;
  }
}

// reload kernel NC-SI driver and trigger NC-SI interface initialization
int
ncsi_init_if(int inv_addr)
{
  char cmd[80] = {0};
  int ret = 0;
  syslog(LOG_CRIT, "re-configure NC-SI and restart eth0 interface");

  if (inv_addr) {
    sprintf(cmd, "ifdown eth0; ifconfig eth0 hw ether 00:00:00:00:00:e0; ifup eth0");
  } else {
    sprintf(cmd, "ifdown eth0; ifup eth0");
  }
  ret = system(cmd);

  syslog(LOG_CRIT, "re-start eth0 interface done! ret=%d", ret);

  return ret;
}


// Handles AEN type 0x01 - Cnfiguration Required
// Steps
//    1. ifdown eth0;ifup eth0    # re-init NIC NC-SI interface
//    2. restart ncsid
void
handle_ncsi_config(int delay)
{
  if (delay) {
    // Give NIC some time to finish its reset opeartion before  BMC sends
    // NCSI commands to re-initialize the interface
    sleep(delay);
  }

  ncsi_init_if(0);
}


int
getMacAddr(unsigned int *values) {
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
int
checkValidMacAddr(unsigned int *value) {
  int numZeroOctet = 0;
  int valid = 1;

  if ((value[0] & 0x01))  // broadcast or multicast address
    return 0;

  for (int i=0; i<6; ++i) {
    if (value[i] == 0)
      numZeroOctet++;
  }

  if (numZeroOctet >= ZERO_OCTET_THRESHOLD) {
    valid = 0;
  }

  return valid;
}

// check if MAC address was successfully obtained from NIC via NC-SI
// will re-init/retry if needed
// return values
//     0 - success
//     1 - generic Error
//     NCSI_IF_REINIT - NCSI reinited, restart required
int
check_valid_mac_addr(void)
{
  unsigned int values[6] = {0};
  int ret = 0;

  ret = getMacAddr(values);

  // if fail to obtain MAC addr, do not proceed further
  if (ret)
    return 1;

  // if invalid MAC was obtained, re-init NCSI interface and
  //     check the MAC addr again
  if (!checkValidMacAddr(values)) {
    /* invalid mac */
    syslog(LOG_CRIT, "invalid MAC(%x:%x:%x:%x:%x:%x) detected, reinit NCSI",
             values[0], values[1], values[2], values[3],
             values[4], values[5]);

    // re-init NCSI interface and get MAC address again
    ncsi_init_if(1);
    ret = getMacAddr(values);
    if (!ret && checkValidMacAddr(values)) {
       syslog(LOG_CRIT, "Valid MAC(%x:%x:%x:%x:%x:%x) obtained after NCSI re-init",
       values[0], values[1], values[2], values[3],
             values[4], values[5]);
      return NCSI_IF_REINIT;
    } else {
      syslog(LOG_CRIT, "Invalid MAC(%x:%x:%x:%x:%x:%x), after NCSI re-init",
            values[0], values[1], values[2], values[3],
            values[4], values[5]);
    }
  }

  return 0;
}

const char *
ncsi_cmd_type_to_name(int cmd)
{
  switch (cmd) {
    case NCSI_OEM_CMD:
      return "NCSI_OEM_CMD";
    case NCSI_PLDM_REQUEST:
      return "SEND_PLDM_REQUEST";
    case NCSI_QUERY_PENDING_NC_PLDM_REQ:
      return "QUERY_PENDING_NC_PLDM_REQ";
    case NCSI_SEND_NC_PLDM_REPLY:
      return "SEND_NC_PLDM_REPLY";
    default:
      if ((cmd < 0) ||
          (cmd >= NUM_NCSI_CDMS) ||
          (ncsi_cmd_string[cmd] == NULL)) {
        return "unknown_ncsi_cmd";
      } else {
        return ncsi_cmd_string[cmd];
      }
  }
}

const char *
ncsi_cc_resp_name(int cc_resp)
{
  if ((cc_resp < 0) ||
      (cc_resp >= RESP_MAX) ||
      (ncsi_resp_string[cc_resp] == NULL)) {
    return "unknown_response";
  } else {
    return ncsi_resp_string[cc_resp];
  }
}


const char *
ncsi_cc_reson_name(int cc_reason)
{
  switch (cc_reason) {
    case REASON_UNKNOWN_CMD_TYPE:
      return "UNKNOWN_CMD_TYPE";
    default:
      if ((cc_reason < 0) ||
          (cc_reason >= NUM_NCSI_REASON_CODE) ||
          (ncsi_reason_string[cc_reason] == NULL)) {
        return "unknown_reason";
      } else {
        return ncsi_reason_string[cc_reason];
      }
  }
}

// Print up to print_num of data bytes
// If print_num is 0 or negative, print all data.
// print_offset is the starting offset printed on each line.
void print_ncsi_data(void *data, int size, int print_num, int print_offset)
{
  int n = (print_num <= 0 || size <= print_num) ? size : print_num;
  int i;

  for (i = 0; i < n; ++i) {
  		if (i % 4 == 0)
  			printf("\n%02d: ", print_offset + i);
      printf("0x%02x ", ((uint8_t *)data)[i]);
  }
  printf("\n");

  if (n < size)
    printf("...\n");
}

void print_ncsi_completion_codes(NCSI_NL_RSP_T *rcv_buf)
{
  uint8_t *pbuf = rcv_buf->msg_payload;
  int cmd = rcv_buf->hdr.cmd;
  int cc_resp = (pbuf[0]<<8)+pbuf[1];
  int cc_reason = (pbuf[2]<<8)+pbuf[3];

  printf("NC-SI Command Response:\n");
  printf("cmd: %s(0x%x)\n", ncsi_cmd_type_to_name(cmd), cmd);
  printf("Response: %s(0x%04x)  Reason: %s(0x%04x)\n",
          ncsi_cc_resp_name(cc_resp), cc_resp,
          ncsi_cc_reson_name(cc_reason), cc_reason);

  return;
}


int
get_cmd_status(NCSI_NL_RSP_T *rcv_buf)
{
  uint8_t *pbuf = rcv_buf->msg_payload;
  int cc_resp = (pbuf[0]<<8)+pbuf[1];

  return (cc_resp);
}


void
print_ncsi_resp(NCSI_NL_RSP_T *rcv_buf)
{
  int cmd = rcv_buf->hdr.cmd;

  print_ncsi_completion_codes(rcv_buf);
  if (get_cmd_status(rcv_buf) != RESP_COMMAND_COMPLETED)
  {
    return;
  }

  switch (cmd) {
  case NCSI_GET_LINK_STATUS:
    print_link_status(rcv_buf);
    break;
  case NCSI_GET_CAPABILITIES:
    print_get_capabilities(rcv_buf);
    break;
  case NCSI_GET_PARAMETERS:
    print_get_parameters(rcv_buf);
    break;
  case NCSI_GET_CONTROLLER_PACKET_STATISTICS:
    print_ncsi_controller_stats(rcv_buf);
    break;
  case NCSI_GET_NCSI_STATISTICS:
    print_ncsi_stats(rcv_buf);
    break;
  case NCSI_GET_NCSI_PASS_THROUGH_STATISTICS:
    print_passthrough_stats(rcv_buf);
    break;
  default:
    printf("Payload length = %d\n",rcv_buf->hdr.payload_length);
    print_ncsi_data(&rcv_buf->msg_payload[4], rcv_buf->hdr.payload_length - 4,
                    0 /* print all */, sizeof(CTRL_MSG_HDR_t) + 4 /* print off */);
  }

  return;
}



void
print_get_capabilities(NCSI_NL_RSP_T *rcv_buf)
{
  unsigned char *pbuf = rcv_buf->msg_payload;
  NCSI_Get_Capabilities_Response *pResp =
    (NCSI_Get_Capabilities_Response *)((NCSI_Response_Packet *)(pbuf))->Payload_Data;

  printf("\nGet Capabilities response\n");
  printf("  capabilities_flags = 0x%x\n", ntohl(pResp->capabilities_flags));
  printf("  broadcast_packet_filter_capabilities = 0x%x\n", ntohl(pResp->broadcast_packet_filter_capabilities));
  printf("  multicast_packet_filter_capabilities = 0x%x\n", ntohl(pResp->multicast_packet_filter_capabilities));
  printf("  buffering_capabilities = 0x%x\n", ntohl(pResp->buffering_capabilities));
  printf("  aen_control_support = 0x%x\n", ntohl(pResp->aen_control_support));
  printf("  unicast_filter_cnt = %d\n", pResp->unicast_filter_cnt);
  printf("  multicast_filter_cnt = %d\n", pResp->multicast_filter_cnt);
  printf("  mixed_filter_cnt = %d\n", pResp->mixed_filter_cnt);
  printf("  vlan_filter_cnt = %d\n", pResp->vlan_filter_cnt);
  printf("  channel_cnt = %d\n", pResp->channel_cnt);
  printf("  vlan_mode_support = %d\n", pResp->vlan_mode_support);
}

void
print_get_parameters(NCSI_NL_RSP_T *rcv_buf)
{
  unsigned char *pbuf = rcv_buf->msg_payload;
  NCSI_Get_Parameters_Response *pResp =
    (NCSI_Get_Parameters_Response *)((NCSI_Response_Packet *)(pbuf))->Payload_Data;

  printf("\nGet Parameters response\n");
  printf("  mac_addr_flags = 0x%x\n", pResp->mac_addr_flags);
  printf("  mac_addr_cnt = %d\n", pResp->mac_addr_cnt);
  printf("  vlan_tag_flags = 0x%x\n", ntohs(pResp->vlan_tag_flags));
  printf("  vlan_tag_cnt = %d\n", pResp->vlan_tag_cnt);
  printf("  link_settings = 0x%x\n", ntohl(pResp->link_settings));
  printf("  broadcast_packet_filter_settings = 0x%x\n", ntohl(pResp->broadcast_packet_filter_settings));
  printf("  configuration_flags = 0x%x\n", ntohl(pResp->configuration_flags));
  printf("  flow_ctrl_enable = %d\n", pResp->flow_ctrl_enable);
  printf("  vlan_mode = %d\n", pResp->vlan_mode);
  printf("  aen_control = 0x%x\n", ntohl(pResp->aen_control));
  printf("  mac_1 = 0x%x\n", ntohl(pResp->mac_1));
  printf("  mac_2 = 0x%x\n", ntohl(pResp->mac_2));
  printf("  mac_3 = 0x%x\n", ntohl(pResp->mac_3));
}


int
ncsi_response_handler(NCSI_NL_RSP_T *rcv_buf)
{
  int cmd = rcv_buf->hdr.cmd;
  int ret = 0;

  if (get_cmd_status(rcv_buf) != RESP_COMMAND_COMPLETED)
  {
    print_ncsi_completion_codes(rcv_buf);
    return -1;
  }

  switch (cmd) {
  // TBD
  case NCSI_GET_CAPABILITIES:
    break;
  case NCSI_GET_PARAMETERS:
    break;

  // for these commands, just print the response for now
  case NCSI_GET_CONTROLLER_PACKET_STATISTICS:
  case NCSI_GET_NCSI_STATISTICS:
  case NCSI_GET_NCSI_PASS_THROUGH_STATISTICS:
  default:
    print_ncsi_resp(rcv_buf);
    ret = 0;
  }

  return ret;
}


void
print_ncsi_controller_stats(NCSI_NL_RSP_T *rcv_buf)
{
  unsigned char *pbuf = rcv_buf->msg_payload;
  NCSI_Controller_Packet_Stats_Response *pResp =
    (NCSI_Controller_Packet_Stats_Response *)((NCSI_Response_Packet *)(pbuf))->Payload_Data;

  setlocale(LC_ALL, "");

  printf("\nNIC statistics\n");
  printf("  Counters cleared last read (MSB): %u\n", ntohl(pResp->counters_cleared_from_last_read_MSB));
  printf("  Counters cleared last read (LSB): %u\n", ntohl(pResp->counters_cleared_from_last_read_LSB));
  printf("  Total Bytes Received: %"PRIu64"\n", be64toh(pResp->total_bytes_rcvd));
  printf("  Total Bytes Transmitted: %"PRIu64"\n", be64toh(pResp->total_bytes_tx));
  printf("  Total Unicast Packet Received: %"PRIu64"\n", be64toh(pResp->total_unicast_pkts_rcvd));
  printf("  Total Multicast Packet Received: %"PRIu64"\n", be64toh(pResp->total_multicast_pkts_rcvd));
  printf("  Total Broadcast Packet Received: %"PRIu64"\n", be64toh(pResp->total_broadcast_pkts_rcvd));
  printf("  Total Unicast Packet Transmitted: %"PRIu64"\n", be64toh(pResp->total_unicast_pkts_tx));
  printf("  Total Multicast Packet Transmitted: %"PRIu64"\n", be64toh(pResp->total_multicast_pkts_tx));
  printf("  Total Broadcast Packet Transmitted: %"PRIu64"\n", be64toh(pResp->total_broadcast_pkts_tx));
  printf("  FCS Receive Errors: %u\n", ntohl(pResp->fcs_receive_errs));
  printf("  Alignment Errors: %u\n", ntohl(pResp->alignment_errs));
  printf("  False Carrier Detections: %u\n", ntohl(pResp->false_carrier_detections));
  printf("  Runt Packets Received: %u\n", ntohl(pResp->runt_pkts_rcvd));
  printf("  Jabber Packets Received: %u\n", ntohl(pResp->jabber_pkts_rcvd));
  printf("  Pause XON Frames Received: %u\n", ntohl(pResp->pause_xon_frames_rcvd));
  printf("  Pause XOFF Frames Received: %u\n", ntohl(pResp->pause_xoff_frames_rcvd));
  printf("  Pause XON Frames Transmitted: %u\n", ntohl(pResp->pause_xon_frames_tx));
  printf("  Pause XOFF Frames Transmitted: %u\n", ntohl(pResp->pause_xoff_frames_tx));
  printf("  Single Collision Transmit Frames: %u\n", ntohl(pResp->single_collision_tx_frames));
  printf("  Multiple Collision Transmit Frames: %u\n", ntohl(pResp->multiple_collision_tx_frames));
  printf("  Late Collision Frames: %u\n", ntohl(pResp->late_collision_frames));
  printf("  Excessive Collision Frames: %u\n", ntohl(pResp->excessive_collision_frames));
  printf("  Control Frames Received: %u\n", ntohl(pResp->control_frames_rcvd));
  printf("  64-Byte Frames Received: %u\n", ntohl(pResp->rx_frame_64));
  printf("  65-127 Byte Frames Received: %u\n", ntohl(pResp->rx_frame_65_127));
  printf("  128-255 Byte Frames Received: %u\n", ntohl(pResp->rx_frame_128_255));
  printf("  256-511 Byte Frames Received: %u\n", ntohl(pResp->rx_frame_256_511));
  printf("  512-1023 Byte Frames Received: %u\n", ntohl(pResp->rx_frame_512_1023));
  printf("  1024-1522 Byte Frames Received: %u\n", ntohl(pResp->rx_frame_1024_1522));
  printf("  1523-9022 Byte Frames Received: %u\n", ntohl(pResp->rx_frame_1523_9022));
  printf("  64-Byte Frames Transmitted: %u\n", ntohl(pResp->tx_frame_64));
  printf("  65-127 Byte Frames Transmitted: %u\n", ntohl(pResp->tx_frame_65_127));
  printf("  128-255 Byte Frames Transmitted: %u\n", ntohl(pResp->tx_frame_128_255));
  printf("  256-511 Byte Frames Transmitted: %u\n", ntohl(pResp->tx_frame_256_511));
  printf("  512-1023 Byte Frames Transmitted: %u\n", ntohl(pResp->tx_frame_512_1023));
  printf("  1024-1522 Byte Frames Transmitted: %u\n", ntohl(pResp->tx_frame_1024_1522));
  printf("  1523-9022 Byte Frames Transmitted: %u\n", ntohl(pResp->tx_frame_1523_9022));
  printf("  Valid Bytes Received: %"PRIu64"\n", be64toh(pResp->valid_bytes_rcvd));
  printf("  Error Runt Packets Received: %u\n", ntohl(pResp->err_runt_packets_rcvd));
  printf("  Error Jabber Packets Received: %u\n", ntohl(pResp->err_jabber_packets_rcvd));
  printf("\n");
}


void
print_ncsi_stats(NCSI_NL_RSP_T *rcv_buf)
{
  unsigned char *pbuf = rcv_buf->msg_payload;
  NCSI_Stats_Response *pResp =
    (NCSI_Stats_Response *)((NCSI_Response_Packet *)(pbuf))->Payload_Data;

  setlocale(LC_ALL, "");

  printf("\nNIC NC-SI statistics\n");
  printf("  NC-SI Commands Received: %u\n", ntohl(pResp->cmds_rcvd));
  printf("  NC-SI Control Packets Dropped: %u\n", ntohl(pResp->ctrl_pkts_dropped));
  printf("  NC-SI Command Type Errors: %u\n", ntohl(pResp->cmd_type_errs));
  printf("  NC-SI Commands Checksum Errors: %u\n", ntohl(pResp->cmd_chksum_errs));
  printf("  NC-SI Receive Packets: %u\n", ntohl(pResp->rx_pkts));
  printf("  NC-SI Transmit Packets: %u\n", ntohl(pResp->tx_pkts));
  printf("  AENs Sent: %u\n", ntohl(pResp->aens_sent));
  printf("\n");
}


void
print_passthrough_stats(NCSI_NL_RSP_T *rcv_buf)
{
  unsigned char *pbuf = rcv_buf->msg_payload;
  NCSI_Passthrough_Stats_Response *pResp =
    (NCSI_Passthrough_Stats_Response *)((NCSI_Response_Packet *)(pbuf))->Payload_Data;

  setlocale(LC_ALL, "");

  printf("\nNIC NC-SI Pass-through statistics\n");
  printf("  Pass-through TX Packets Received: %"PRIu64"\n", be64toh(pResp->tx_packets_rcvd_on_ncsi));
  printf("  Pass-through TX Packets Dropped: %u\n", ntohl(pResp->tx_packets_dropped));
  printf("  Pass-through TX Packet Channel State Errors: %u\n", ntohl(pResp->tx_channel_state_err));
  printf("  Pass-through TX Packet Undersize Errors: %u\n", ntohl(pResp->tx_undersize_err));
  printf("  Pass-through TX Packets Oversize Packets: %u\n", ntohl(pResp->tx_oversize_err));
  printf("  Pass-through RX Packets Received on LAN: %u\n", ntohl(pResp->rx_packets_rcvd_on_lan));
  printf("  Total Pass-through RX Packets Dropped: %u\n", ntohl(pResp->total_rx_packets_dropped));
  printf("  Pass-through RX Packet Channel State Errors: %u\n", ntohl(pResp->rx_channel_state_err));
  printf("  Pass-through RX Packet Undersize Errors: %u\n", ntohl(pResp->rx_undersize_err));
  printf("  Pass-through RX Packets Oversize Packets: %u\n", ntohl(pResp->rx_oversize_err));
  printf("\n");
}


const char *
link_speed_to_name(int val)
{
  // val is 4 bit value
  if ((val < 0) ||
      (val >= (int)ARRAY_SIZE(link_speed_string)) ||
      (link_speed_string[val] == NULL)) {
    return "unknown";
  } else {
    return link_speed_string[val];
  }
}

void
print_link_status(NCSI_NL_RSP_T *rcv_buf)
{
  unsigned char *pbuf = rcv_buf->msg_payload;
  Get_Link_Status_Response *plink =
    (Get_Link_Status_Response *)((NCSI_Response_Packet *)(pbuf))->Payload_Data;
  Link_Status linkstatus;
  linkstatus.all32 = ntohl(plink->link_status.all32);

  printf("\nLink Flag: ");
  if (linkstatus.bits.link_flag)
    printf("Up\n");
  else
    printf("Down\n");

  printf("Speed and duplex: %s\n", link_speed_to_name(linkstatus.bits.speed_duplex));
  printf("\n");
  return;
}

int
handle_get_link_status(NCSI_Response_Packet *resp)
{
  char logbuf[512];
  int currentLinkStatus =  0;
  int i = 0;
  int nleft = sizeof(logbuf);
  int nwrite = 0;
  static int prevLinkStatus = 0;

  Get_Link_Status_Response *linkresp = (Get_Link_Status_Response *)resp->Payload_Data;
  Link_Status linkstatus;
  Other_Indications linkOther;
  linkstatus.all32 = ntohl(linkresp->link_status.all32);
  linkOther.all32 = ntohl(linkresp->other_indications.all32);
  currentLinkStatus = linkstatus.bits.link_flag;

  if (currentLinkStatus != prevLinkStatus)
  {
    // log link status change
    if (currentLinkStatus) {
      nwrite = snprintf(logbuf, nleft, "NIC link up:");
    } else {
      nwrite = snprintf(logbuf, nleft, "NIC link down:");
    }
    i += nwrite;
    nleft -= nwrite;
    nwrite = snprintf(logbuf + i, nleft, "Rsp:0x%04x ", ntohs(resp->Response_Code));
    i += nwrite;
    nleft -= nwrite;
    nwrite = snprintf(logbuf + i, nleft, "Rsn:0x%04x ", ntohs(resp->Reason_Code));
    i += nwrite;
    nleft -= nwrite;
    nwrite = snprintf(logbuf + i, nleft, "Link:0x%lx ", linkstatus.all32);
    i += nwrite;
    nleft -= nwrite;
    nwrite = snprintf(logbuf + i, nleft, "(LF:0x%x ", linkstatus.bits.link_flag);
    i += nwrite;
    nleft -= nwrite;
    nwrite = snprintf(logbuf + i, nleft, "SP:0x%x ",  linkstatus.bits.speed_duplex);
    i += nwrite;
    nleft -= nwrite;
    nwrite = snprintf(logbuf + i, nleft, "SD:0x%x) ", linkstatus.bits.serdes);
    i += nwrite;
    nleft -= nwrite;
    nwrite = snprintf(logbuf + i, nleft, "Other:0x%lx ", linkOther.all32);
    i += nwrite;
    nleft -= nwrite;
    nwrite = snprintf(logbuf + i, nleft, "(Driver:0x%x) ", linkOther.bits.host_NC_driver_status);
    i += nwrite;
    nleft -= nwrite;
    nwrite = snprintf(logbuf + i, nleft, "OEM:0x%lx ", (unsigned long)ntohl(linkresp->oem_link_status));
    syslog(LOG_WARNING, "%s", logbuf);
    prevLinkStatus = currentLinkStatus;
  }
  return 0;
}

int
handle_get_version_id(NCSI_Response_Packet *resp)
{
  Get_Version_ID_Response *vidresp = (Get_Version_ID_Response *)resp->Payload_Data;
  uint8_t nic_fw_ver[4] = {0};
  int fd;

  fd = open(NIC_FW_VER_PATH, O_RDWR | O_CREAT, 0666);
  if (fd < 0) {
    return -1;
  }

  lseek(fd, (long)&((Get_Version_ID_Response *)0)->fw_ver, SEEK_SET);
  int version_length = read(fd, nic_fw_ver, sizeof(nic_fw_ver));
  if (version_length != 0 && version_length != sizeof(nic_fw_ver)) {
    close(fd);
    return -1;
  }
  if (memcmp(vidresp->fw_ver, nic_fw_ver, sizeof(nic_fw_ver))) {
    lseek(fd, 0, SEEK_SET);
    if (write(fd, vidresp, sizeof(Get_Version_ID_Response)) == sizeof(Get_Version_ID_Response)) {
      syslog(LOG_WARNING, "updated %s", NIC_FW_VER_PATH);
    }
  }
  close(fd);

  return 0;
}

int
create_ncsi_ctrl_pkt(NCSI_NL_MSG_T *nl_msg, uint8_t ch, uint8_t cmd,
                     uint16_t payload_len, unsigned char *payload) {
  nl_msg->cmd = cmd;
  nl_msg->payload_length = payload_len;
  nl_msg->channel_id = ch;
  int package = (ch & 0xE0) >> 5;

  sprintf(nl_msg->dev_name, "eth%d", package);
//  printf("%s %s\n", __func__, nl_msg->dev_name);

  if (payload_len > NCSI_MAX_PAYLOAD) {
    syslog(LOG_ERR, "%s payload length(%d) exceeds threshold(%d), cmd=0x%02x",
           __func__, payload_len, NCSI_MAX_PAYLOAD, cmd);
    return -1;
  }

  if (payload) {
    memcpy(&(nl_msg->msg_payload[0]), payload, payload_len);
  }

  return 0;
}


// Takes a NCSI_Response_Packet message, returns the NCSI payload
// (minus any NCSI header, which is
//    2 byte response code + 2 byte reason code)
unsigned char *
get_ncsi_resp_payload(NCSI_Response_Packet *ncsi_resp)
{
  return ncsi_resp->Payload_Data;
}
