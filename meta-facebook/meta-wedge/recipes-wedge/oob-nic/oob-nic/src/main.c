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
#include <stdio.h>
#include <unistd.h>
#include <sys/select.h>
#include <sys/time.h>
#include <sys/errno.h>

#include "nic.h"
#include "intf.h"

#include "openbmc/log.h"
#include "facebook/wedge_eeprom.h"

#define WAIT4PACKET_TIMEOUT 10000        /* 10ms */
#define NO_RCV_CHECK_THRESHOLD 100 /* if not receiving pkt for 100 times (1s),
                                    * check the NIC status
                                    */

static void io_loop(oob_nic *nic, oob_intf *intf, const uint8_t mac[6]) {

  fd_set rfds;
  int fd = oob_intf_get_fd(intf);
  struct timeval timeout;
  int rc;
  int n_fds;
  int n_io;
  char buf[NIC_PKT_SIZE_MAX];
  int no_rcv = 0;
  struct oob_nic_status_t sts;

  while (1) {
    memset(&timeout, 0, sizeof(timeout));
    timeout.tv_sec = 0;
    timeout.tv_usec = WAIT4PACKET_TIMEOUT;

    FD_ZERO(&rfds);
    FD_SET(fd, &rfds);

    n_fds = select(fd + 1, &rfds, NULL, NULL, &timeout);
    if (n_fds < 0) {
      rc = errno;
      LOG_ERR(rc, "Failed to select");
      continue;
    }

    /*
     * no matter what, receive packet from nic first, as the nic
     * has small amount of memory. Without read, the sending could
     * fail due to OOM.
     *
     * TODO: We might want to do something smart here to prevent attack or
     * just rx flooding. Disable the Receive Enable first, drain the buffer
     * with oob_nic_receive(), then Tx, and enable Receive Enable after.
     */
    for (n_io = 0; n_io < 16; n_io++) {
      rc = oob_nic_receive(nic, buf, sizeof(buf));
      if (rc <= 0) {
        no_rcv++;
        break;
      }
      oob_intf_send(intf, buf, rc);
      no_rcv = 0;
    }

    /*
     * if we didn't receive any packet for NO_RCV_CHECK_THRESHOLD times,
     * check the nic status
     */
    if (no_rcv >= NO_RCV_CHECK_THRESHOLD) {
      while(oob_nic_get_status(nic, &sts)) {
        usleep(1000);
      }
      LOG_INFO("Failed to receive packets for %d times. NIC status is "
               "%x.%x", NO_RCV_CHECK_THRESHOLD, sts.ons_byte1, sts.ons_byte2);
      /*
       * if the NIC went through initialization, or not set force up, need to
       * re-program the filters by calling oob_nic_start().
       */
      if ((sts.ons_byte1 & NIC_STATUS_D1_INIT)
          || !(sts.ons_byte1 & NIC_STATUS_D1_FORCE_UP)) {
        while(oob_nic_start(nic, mac)) {
          usleep(1000);
        }
      }
      no_rcv = 0;
    }

    if (n_fds > 0 && FD_ISSET(fd, &rfds)) {
      for (n_io = 0; n_io < 1; n_io++) {
        rc = oob_intf_receive(intf, buf, sizeof(buf));
        if (rc <= 0) {
          break;
        }
        oob_nic_send(nic, buf, rc);
      }
    }
  }
}

int main(int argc, const char **argv) {

  uint8_t mac[6];
  oob_nic *nic;
  oob_intf *intf;
  struct wedge_eeprom_st eeprom;
  int rc;
  int from_eeprom = 0;

  nic = oob_nic_open(0, 0x49);
  if (!nic) {
    return -1;
  }

  /* read EEPROM for the MAC */
  if (wedge_eeprom_parse(NULL, &eeprom) == 0) {
    uint16_t carry;
    int pos;
    int adj;
    /*
     * OOB MAC comes from this range. We pick the last MAC from the range to
     * use as OOB MAC.
     */
    if (eeprom.fbw_mac_size > 128) {
      LOG_ERR(EFAULT, "Extended MAC size (%d) is too large.",
              eeprom.fbw_mac_size);
      carry = 128;
    } else {
      carry = eeprom.fbw_mac_size;
    }

    /*
     * Due to various manufacture issues, some FC boards have MAC range overlap
     * between LEFT and RIGHT sides. A SW workaround is done below to use the
     * 8th (or 7th for right side FC) last MAC from the range for FC.
     */
    if (strncmp(eeprom.fbw_location, "LEFT", FBW_EEPROM_F_LOCATION) == 0) {
      adj = 8;
    } else if (strncmp(eeprom.fbw_location, "RIGHT", FBW_EEPROM_F_LOCATION)
               == 0) {
      adj = 7;
    } else {
      adj = 1;
    }

    if (carry < adj) {
      LOG_ERR(EFAULT, "Invalid extended MAC size: %d", eeprom.fbw_mac_size);
    } else {
      carry -= adj;
      memcpy(mac, eeprom.fbw_mac_base, sizeof(mac));
      for (pos = sizeof(mac) - 1; pos >= 0 && carry; pos--) {
        uint16_t tmp = mac[pos] + carry;
        mac[pos] = tmp & 0xFF;
        carry = tmp >> 8;
      }
      from_eeprom = 1;
    }
  }

  if (!from_eeprom) {
    while (oob_nic_get_mac(nic, mac)) {
      usleep(1000);
    }
    /*
     * increase the last byte of the mac by 1 and turn on the
     * local administered bit to use it as the oob nic mac
     */
    mac[0] |= 0x2;
    mac[5]++;
  }

  LOG_INFO("Retrieve MAC %x:%x:%x:%x:%x:%x from %s",
           mac[0], mac[1], mac[2], mac[3], mac[4], mac[5],
           (from_eeprom) ? "EEPROM" : "NIC");

  /* create the tap interface */
  intf = oob_intf_create("oob", mac);
  if (!intf) {
    return -1;
  }

  while (oob_nic_start(nic, mac)) {
    usleep(1000);
  }

  io_loop(nic, intf, mac);

  return 0;
}
