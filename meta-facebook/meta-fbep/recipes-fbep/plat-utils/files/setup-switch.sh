#!/bin/sh
#
# Copyright 2020-present Facebook. All Rights Reserved.
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
# Provides:          switch-setup
# Required-Start:
# Required-Stop:
# Default-Start:     5
# Default-Stop:
# Short-Description:  Set up strap pins to determine the switch configuration
### END INIT INFO

. /usr/local/fbpackages/utils/ast-functions

echo -n "Setup PCIe switch configuration "
# 8S by default
gpio_set PAX0_SKU_ID0 0
gpio_set PAX1_SKU_ID0 0
gpio_set PAX2_SKU_ID0 0
gpio_set PAX3_SKU_ID0 0

server_type=$(kv get server_type persistent)
if [ $? -ne 0 ]; then
  # Get config from MB0's BMC
  for retry in {1..30};
  do
    server_type=$(/usr/local/bin/ipmb-util 2 0x20 0xE8 0x0)
    server_type=${server_type:1:1}
    if [[ "$server_type" == "0" ]]; then
      /usr/local/bin/cfg-util server_type 8
      server_type=8
      break
    elif [[ "$server_type" == "1" ]]; then
      /usr/local/bin/cfg-util server_type 4
      server_type=4
      break
    elif [[ "$server_type" == "2" ]]; then
      /usr/local/bin/cfg-util server_type 2
      server_type=2
      break
    else
      echo -n "."
      sleep 1
    fi
  done
fi
if [[ "$server_type" == "2" ]]; then
  gpio_set PAX0_SKU_ID0 1
  gpio_set PAX1_SKU_ID0 1
  gpio_set PAX2_SKU_ID0 1
  gpio_set PAX3_SKU_ID0 1
fi
echo "done"

gpio_set BMC_READY_N 0
