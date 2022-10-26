/*
 * lan.c
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

#define _GNU_SOURCE /* To get defns of NI_MAXSERV and NI_MAXHOST */
#include <arpa/inet.h>
#include <ifaddrs.h>
#include <linux/if_link.h>
#include <net/if.h>
#include <netdb.h>
#include <openbmc/ipmi.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <unistd.h>
#include "cfg-parse.h"
#include "common-handles.h"

#define IPV6_LINK_LOCAL_BYTE1 0xFE
#define IPV6_LINK_LOCAL_BYTE2 0x80

#define BYTE_MASK 0xFF
#define BYTE1_OFFSET 8
#define BYTE2_OFFSET 16
#define BYTE3_OFFSET 24

void plat_lan_init(lan_config_t* lan) {
  struct ifaddrs *ifaddr, *ifa;
  struct sockaddr_in* addr;
  struct sockaddr_in6* addr6;
  int family, n, i;
  unsigned long ip;
  unsigned char *ip6, *netmask6;
  int sd;
  struct ifreq ifr;
  uint8_t eui_64_addr[8] = {0x0};
  char if_name[MAX_NAME_SIZE];

  if (getifaddrs(&ifaddr) == -1) {
    return;
  }

  sd = socket(PF_INET, SOCK_DGRAM, 0);
  if (sd < 0) {
    goto init_done;
  }

  for (ifa = ifaddr, n = 0; ifa != NULL; ifa = ifa->ifa_next, n++) {
    if (ifa->ifa_addr == NULL) {
      continue;
    }

    get_cfg_item_str("eth_intf_name", if_name, MAX_NAME_SIZE - 1);
    if (strncmp(ifa->ifa_name, if_name, MAX_ETH_IF_SIZE)) {
      continue;
    }

    family = ifa->ifa_addr->sa_family;

    if (family == AF_INET) {
      addr = (struct sockaddr_in*)ifa->ifa_addr;
      ip = addr->sin_addr.s_addr;

      // copy the ip address from long to byte array with MSB first
      lan->ip_addr[3] = (ip >> BYTE3_OFFSET) & BYTE_MASK;
      lan->ip_addr[2] = (ip >> BYTE2_OFFSET) & BYTE_MASK;
      lan->ip_addr[1] = (ip >> BYTE1_OFFSET) & BYTE_MASK;
      lan->ip_addr[0] = ip & BYTE_MASK;
    } else if (family == AF_INET6) {
      addr6 = (struct sockaddr_in6*)ifa->ifa_addr;
      ip6 = addr6->sin6_addr.s6_addr;

      // If the address is Link Local, get the MAC address, then Ignore it
      if ((ip6[0] == IPV6_LINK_LOCAL_BYTE1) &&
          (ip6[1] == IPV6_LINK_LOCAL_BYTE2)) {
        strcpy(ifr.ifr_name, ifa->ifa_name);
        if (ioctl(sd, SIOCGIFHWADDR, &ifr) != -1)
          memcpy(lan->mac_addr, ifr.ifr_hwaddr.sa_data, SIZE_MAC_ADDR);
        continue;
      }

      // Get the MAC address
      strcpy(ifr.ifr_name, ifa->ifa_name);
      if (ioctl(sd, SIOCGIFHWADDR, &ifr) != -1) {
        uint8_t* mac_addr = (uint8_t*)ifr.ifr_hwaddr.sa_data;

        /*
         * SLAAC address has lower 8B as follows:
         * 3B == First 24b MAC address
         * 2B == FFFE
         * 3B == Last 24b MAC address
         */
        memcpy((void*)&eui_64_addr[0], (void*)&mac_addr[0], 3);
        eui_64_addr[3] = 0xFF;
        eui_64_addr[4] = 0xFE;
        memcpy((void*)&eui_64_addr[5], (void*)&mac_addr[3], 3);
        eui_64_addr[0] += 2;

        // Check if the address is SLAAC address. If yes, skip it.
        if (!memcmp((void*)&ip6[8], (void*)eui_64_addr, 8)) {
          continue;
        }
      }

      // copy the ip address from array with MSB first
      memcpy(lan->ip6_addr, ip6, SIZE_IP6_ADDR);

      // calculate the Address Prefix Length
      netmask6 = ((struct sockaddr_in6*)ifa->ifa_netmask)->sin6_addr.s6_addr;
      for (i = 0; i < SIZE_IP6_ADDR * 8; i++) {
        if (!((netmask6[i / 8] << (i % 8)) & 0x80))
          break;
      }
      lan->ip6_prefix = i;
    }
  }

  // close socket descriptor
  close(sd);

init_done:
  freeifaddrs(ifaddr);
}
