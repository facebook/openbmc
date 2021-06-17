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
# 4SEX by default
gpio_set PAX0_SKU_ID0 0
gpio_set PAX0_SKU_ID1 0
gpio_set PAX1_SKU_ID0 0
gpio_set PAX1_SKU_ID1 0
gpio_set PAX2_SKU_ID0 0
gpio_set PAX2_SKU_ID1 0
gpio_set PAX3_SKU_ID0 0
gpio_set PAX3_SKU_ID1 0

server_type=$(kv get server_type persistent)
if [ $? -ne 0 ]; then
  # Get config from MB0's BMC
  for retry in {1..200};
  do
    val=$(/usr/local/bin/ipmb-util 2 0x20 0xE8 0x0)
    val=${val:1:1}
    if [[ "$val" == "0" ]]; then
      /usr/local/bin/cfg-util server_type 8
      server_type=8
      break
    elif [[ "$val" == "1" ]]; then
      /usr/local/bin/cfg-util server_type 4
      server_type=4
      break
    elif [[ "$val" == "2" ]]; then
      /usr/local/bin/cfg-util server_type 2
      server_type=2
      break
    else
      echo -n "."
      sleep 1
    fi
  done
else
  # TODO:
  #   Sync EEPROM with cache for those old systems
  #   We can remove this after all systems got updated
  val=$(ipmitool i2c bus=6 0xA8 0x1 0x4 0x6)
  val=${val:2:1}
  if [[ "$val" != "$server_type" ]]; then
    /usr/local/bin/cfg-util server_type $server_type
  fi
fi

case $server_type in
  8)
    gpio_set PAX0_SKU_ID0 0
    gpio_set PAX0_SKU_ID1 1
    gpio_set PAX1_SKU_ID0 0
    gpio_set PAX1_SKU_ID1 1
    gpio_set PAX2_SKU_ID0 0
    gpio_set PAX2_SKU_ID1 1
    gpio_set PAX3_SKU_ID0 0
    gpio_set PAX3_SKU_ID1 1
    ;;
  4)
    gpio_set PAX0_SKU_ID0 0
    gpio_set PAX0_SKU_ID1 0
    gpio_set PAX1_SKU_ID0 0
    gpio_set PAX1_SKU_ID1 0
    gpio_set PAX2_SKU_ID0 0
    gpio_set PAX2_SKU_ID1 0
    gpio_set PAX3_SKU_ID0 0
    gpio_set PAX3_SKU_ID1 0
    ;;
  2)
    gpio_set PAX0_SKU_ID0 1
    gpio_set PAX0_SKU_ID1 0
    gpio_set PAX1_SKU_ID0 1
    gpio_set PAX1_SKU_ID1 0
    gpio_set PAX2_SKU_ID0 1
    gpio_set PAX2_SKU_ID1 0
    gpio_set PAX3_SKU_ID0 1
    gpio_set PAX3_SKU_ID1 0
    ;;
esac
echo "done"

gpio_set BMC_READY_N 0
