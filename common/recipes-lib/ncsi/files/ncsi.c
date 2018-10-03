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
getMacAddr(int *values) {
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
checkValidMacAddr(int *value) {
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
check_ncsi_status(void)
{
  int values[6] = {0};
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


void
print_ncsi_resp(int cmd, NCSI_NL_RSP_T *rcv_buf)
{
  uint8_t *pbuf = rcv_buf->msg_payload;
  int i = 0;

  printf("NC-SI Command Response:\n");
  printf("Response Code: 0x%04x  Reason Code: 0x%04x\n", (pbuf[0]<<8)+pbuf[1], (pbuf[2]<<8)+pbuf[3]);

  switch (cmd) {
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
    for (i = 4; i < rcv_buf->payload_length; ++i) {
  		if (i && !(i%4))
  			printf("\n%d: ", 16+i);
      printf("0x%02x ", rcv_buf->msg_payload[i]);
    }
    printf("\n");

  }

  return;
}


void
print_ncsi_controller_stats(NCSI_NL_RSP_T *rcv_buf)
{
  unsigned char *pbuf = rcv_buf->msg_payload;
  NCSI_Controller_Packet_Stats_Response *pResp =
    (NCSI_Controller_Packet_Stats_Response *)((NCSI_Response_Packet *)(pbuf))->Payload_Data;

  setlocale(LC_ALL, "");

  printf("\nNIC statistics\n");
  printf("  Counters cleared last read (MSB): %'zu\n", ntohl(pResp->counters_cleared_from_last_read_MSB));
  printf("  Counters cleared last read (LSB): %'zu\n", ntohl(pResp->counters_cleared_from_last_read_LSB));
  printf("  Total Bytes Received: %'"PRIu64"\n", be64toh(pResp->total_bytes_rcvd));
  printf("  Total Bytes Transmitted: %'"PRIu64"\n", be64toh(pResp->total_bytes_tx));
  printf("  Total Unicast Packet Received: %'"PRIu64"\n", be64toh(pResp->total_unicast_pkts_rcvd));
  printf("  Total Multicast Packet Received: %'"PRIu64"\n", be64toh(pResp->total_multicast_pkts_rcvd));
  printf("  Total Broadcast Packet Received: %'"PRIu64"\n", be64toh(pResp->total_broadcast_pkts_rcvd));
  printf("  Total Unicast Packet Transmitted: %'"PRIu64"\n", be64toh(pResp->total_unicast_pkts_tx));
  printf("  Total Multicast Packet Transmitted: %'"PRIu64"\n", be64toh(pResp->total_multicast_pkts_tx));
  printf("  Total Broadcast Packet Transmitted: %'"PRIu64"\n", be64toh(pResp->total_broadcast_pkts_tx));
  printf("  FCS Receive Errors: %'zu\n", ntohl(pResp->fcs_receive_errs));
  printf("  Alignment Errors: %'zu\n", ntohl(pResp->alignment_errs));
  printf("  False Carrier Detections: %'zu\n", ntohl(pResp->false_carrier_detections));
  printf("  Runt Packets Received: %'zu\n", ntohl(pResp->runt_pkts_rcvd));
  printf("  Jabber Packets Received: %'zu\n", ntohl(pResp->jabber_pkts_rcvd));
  printf("  Pause XON Frames Received: %'zu\n", ntohl(pResp->pause_xon_frames_rcvd));
  printf("  Pause XOFF Frames Received: %'zu\n", ntohl(pResp->pause_xoff_frames_rcvd));
  printf("  Pause XON Frames Transmitted: %'zu\n", ntohl(pResp->pause_xon_frames_tx));
  printf("  Pause XOFF Frames Transmitted: %'zu\n", ntohl(pResp->pause_xoff_frames_tx));
  printf("  Single Collision Transmit Frames: %'zu\n", ntohl(pResp->single_collision_tx_frames));
  printf("  Multiple Collision Transmit Frames: %'zu\n", ntohl(pResp->multiple_collision_tx_frames));
  printf("  Late Collision Frames: %'zu\n", ntohl(pResp->late_collision_frames));
  printf("  Excessive Collision Frames: %'zu\n", ntohl(pResp->excessive_collision_frames));
  printf("  Control Frames Received: %'zu\n", ntohl(pResp->control_frames_rcvd));
  printf("  64-Byte Frames Received: %'zu\n", ntohl(pResp->rx_frame_64));
  printf("  65-127 Byte Frames Received: %'zu\n", ntohl(pResp->rx_frame_65_127));
  printf("  128-255 Byte Frames Received: %'zu\n", ntohl(pResp->rx_frame_128_255));
  printf("  256-511 Byte Frames Received: %'zu\n", ntohl(pResp->rx_frame_256_511));
  printf("  512-1023 Byte Frames Received: %'zu\n", ntohl(pResp->rx_frame_512_1023));
  printf("  1024-1522 Byte Frames Received: %'zu\n", ntohl(pResp->rx_frame_1024_1522));
  printf("  1523-9022 Byte Frames Received: %'zu\n", ntohl(pResp->rx_frame_1523_9022));
  printf("  64-Byte Frames Transmitted: %'zu\n", ntohl(pResp->tx_frame_64));
  printf("  65-127 Byte Frames Transmitted: %'zu\n", ntohl(pResp->tx_frame_65_127));
  printf("  128-255 Byte Frames Transmitted: %'zu\n", ntohl(pResp->tx_frame_128_255));
  printf("  256-511 Byte Frames Transmitted: %'zu\n", ntohl(pResp->tx_frame_256_511));
  printf("  512-1023 Byte Frames Transmitted: %'zu\n", ntohl(pResp->tx_frame_512_1023));
  printf("  1024-1522 Byte Frames Transmitted: %'zu\n", ntohl(pResp->tx_frame_1024_1522));
  printf("  1523-9022 Byte Frames Transmitted: %'zu\n", ntohl(pResp->tx_frame_1523_9022));
  printf("  Valid Bytes Received: %'"PRIu64"\n", be64toh(pResp->valid_bytes_rcvd));
  printf("  Error Runt Packets Received: %'zu\n", ntohl(pResp->err_runt_packets_rcvd));
  printf("  Error Jabber Packets Received: %'zu\n", ntohl(pResp->err_jabber_packets_rcvd));
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
  printf("  NC-SI Commands Received: %'zu\n", ntohl(pResp->cmds_rcvd));
  printf("  NC-SI Control Packets Dropped: %'zu\n", ntohl(pResp->ctrl_pkts_dropped));
  printf("  NC-SI Command Type Errors: %'zu\n", ntohl(pResp->cmd_type_errs));
  printf("  NC-SI Commands Checksum Errors: %'zu\n", ntohl(pResp->cmd_chksum_errs));
  printf("  NC-SI Receive Packets: %'zu\n", ntohl(pResp->rx_pkts));
  printf("  NC-SI Transmit Packets: %'zu\n", ntohl(pResp->tx_pkts));
  printf("  AENs Sent: %'zu\n", ntohl(pResp->aens_sent));
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
  printf("  Pass-through TX Packets Received: %'"PRIu64"\n", be64toh(pResp->tx_packets_rcvd_on_ncsi));
  printf("  Pass-through TX Packets Dropped: %'zu\n", ntohl(pResp->tx_packets_dropped));
  printf("  Pass-through TX Packet Channel State Errors: %'zu\n", ntohl(pResp->tx_channel_state_err));
  printf("  Pass-through TX Packet Undersize Errors: %'zu\n", ntohl(pResp->tx_undersize_err));
  printf("  Pass-through TX Packets Oversize Packets: %'zu\n", ntohl(pResp->tx_oversize_err));
  printf("  Pass-through RX Packets Received on LAN: %'zu\n", ntohl(pResp->rx_packets_rcvd_on_lan));
  printf("  Total Pass-through RX Packets Dropped: %'zu\n", ntohl(pResp->total_rx_packets_dropped));
  printf("  Pass-through RX Packet Channel State Errors: %'zu\n", ntohl(pResp->rx_channel_state_err));
  printf("  Pass-through RX Packet Undersize Errors: %'zu\n", ntohl(pResp->rx_undersize_err));
  printf("  Pass-through RX Packets Oversize Packets: %'zu\n", ntohl(pResp->rx_oversize_err));
  printf("\n");
}
