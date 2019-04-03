#!/bin/bash
#
# Copyright 2018-present Facebook. All Rights Reserved.
#
# This program file is free software; you can redistribute it and/or modify it
# under the terms of the GNU General Public License as published by the
# Free Software Foundation; version 2 of the License.
#
# This program is distributed in the hope that it will be useful, but WITHOUT
# ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
# FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
# for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program in a file named COPYING; if not, write to the
# Free Software Foundation, Inc.,
# 51 Franklin Street, Fifth Floor,
# Boston, MA 02110-1301 USA
#

### BEGIN INIT INFO
# Provides:          eth0_mac_fixup.sh
# Required-Start:
# Required-Stop:
# Default-Start:     S
# Default-Stop:
# Short-Description:  Fixup the MAC address for eth0 based on wedge EEPROM
### END INIT INFO

PATH=/sbin:/bin:/usr/sbin:/usr/bin:/usr/local/bin

#
# FIXME restart eth0 interface to refresh ipv6 link-local address. The
# workaround is needed for yamp mainly because ipv6 link-local address
# might be derived from the "random" mac address which leads to DHCP
# request failures.
#
# Below explains why the problem happens. We are still investigating the
# official fix, which most likely goes to kernel space.
#   1) when "ftgmac100" driver is binded to BMC's MAC Controller, the
#      controller is assigned with a "random" MAC address.
#   2) later when "/etc/init.d/networking start" is executed for the first
#      time, "ftgmac100" driver starts to communicate with (Broadcom) NCSI
#      NIC, including issuing "GET_MAC_ADDR" command to NCSI NIC.
#   3) given ftgmac100->ndo_open() and NCSI command-handling are running
#      asynchronously, BMC's ipv6 link-local address is "undefined": it may
#      be derived from the "random" mac address, or the permanent one (from
#      NCSI NIC), depending on which task (ipv6 link-local address generation
#      or handling "GET_DMA_ADDR" response from NCSI NIC) happens first.
#   4) If unfortunately random MAC is used (which happens most times), DHCP
#      server will ignore BMC's DHCP request due to UUID mismatch.
#
# Note:
#   1) the workaround is not needed by kernel 4.1 because ftgmac100->ndo_open
#      won't return until "GET_MAC_ADDR" NSCI command is handled.
#
KERNEL_VERSION=`uname -r`
if [[ ${KERNEL_VERSION} != 4.1.* ]]; then
  echo "fixup eth0 interface.."
  sleep 1
  ifdown eth0
  ifup eth0
fi

# set the MAC from ifconfig back to u-boot environment so that u-boot
# can use it
mac=$(ifconfig eth0 2>/dev/null |grep HWaddr 2>/dev/null |awk '{ print $5 }')
ethaddr=$(fw_printenv ethaddr 2>/dev/null | cut -d'=' -f2 2>/dev/null)
if [ "$ethaddr" != "$mac" ]; then
    fw_setenv "ethaddr" "$mac"
fi
