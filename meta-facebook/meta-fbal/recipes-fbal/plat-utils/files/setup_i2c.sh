#!/bin/sh
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

PATH=/sbin:/bin:/usr/sbin:/usr/bin:/usr/local/bin

. /usr/local/bin/openbmc-utils.sh

if [ "$(gpio_get HP_LVC3_OCP_V3_1_PRSNT2_N)" == "0" ]; then
  i2c_device_add 17 0x50 24c32
fi

if [ "$(gpio_get HP_LVC3_OCP_V3_2_PRSNT2_N)" == "0" ]; then
  i2c_device_add 18 0x52 24c32
  if [ "$(cat /mnt/data/kv_store/eth1_ipv6_autoconf)" != "1" ]; then
    echo 0 > /proc/sys/net/ipv6/conf/eth1/autoconf
  fi
  ifconfig eth1 up
fi
