#!/bin/bash
#
# Copyright 2023-present Facebook. All Rights Reserved.
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

connect_uart2_4() {
  echo -ne "uart1" > /sys/devices/platform/ahb/ahb:apb/1e789000.lpc/1e789098.uart_routing/uart4
  echo -ne "uart4" > /sys/devices/platform/ahb/ahb:apb/1e789000.lpc/1e789098.uart_routing/uart1
  echo -ne "io1" > /sys/devices/platform/ahb/ahb:apb/1e789000.lpc/1e789098.uart_routing/uart2
  echo -ne "uart2" > /sys/devices/platform/ahb/ahb:apb/1e789000.lpc/1e789098.uart_routing/io1
}

setup_artemis_uart() {
  local i=0
  local retry=5
  # input PSOC vendor id and product id to Generic Serial driver
  printf "04b4 e17a" > /sys/bus/usb-serial/drivers/generic/new_id
  # Wait for us to have detected all 24 devices. If we do not have
  # 24 devices within 5s of probing the new vendor, then print
  # a SEL.
  while [ $i -le $retry ]; do
     num=$(lsusb | grep -c e17a)
     if [ "$num" -lt 24 ]; then
       if [ $i -eq $retry ]; then
         logger -p user.crit -t usbmon "FRU: 16 ASSERT: Detected ${num} USB devices instead of expected 24"
       fi
     else
       logger -p user.info -t usbmon "FRU: 16 Detected ${num} USB devices"
       break
     fi
     i=$((i+1))
     sleep 1
  done
}

connect_uart2_4
setup_artemis_uart

