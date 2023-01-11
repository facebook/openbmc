#!/bin/sh
#
# Copyright (c) Meta Platforms, Inc. and affiliates.
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

. /usr/local/bin/openbmc-utils.sh
. /usr/local/fbpackages/utils/ast-functions

setup_hgx_eeprom () {
  if [ ! -L "/sys/bus/i2c/drivers/at24/9-0053" ];
  then
    i2c_device_add 9 0x53 24c64
    sleep 1
    dd if=/sys/class/i2c-dev/i2c-9/device/9-0053/eeprom of=/tmp/fruid_hgx.bin bs=512 count=1
  fi
}


ifconfig usb0 192.168.31.2 netmask 255.255.0.0
kv set is_usbnet_ready 1
setup_hgx_eeprom
