#!/bin/sh
#
# Copyright 2014-present Facebook. All Rights Reserved.
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

mac=$(ipmitool i2c bus=6 0xa8 0x6 0x4 0x0)
if [[ "$mac" == " ff ff ff ff ff ff" ]]
then
  addr="02:00:00:00:00:01"
else
  addr="$(echo $mac | cut -d " " -f 1)"
  addr+=":$(echo $mac | cut -d " " -f 2)"
  addr+=":$(echo $mac | cut -d " " -f 3)"
  addr+=":$(echo $mac | cut -d " " -f 4)"
  addr+=":$(echo $mac | cut -d " " -f 5)"
  addr+=":$(echo $mac | cut -d " " -f 6)"
fi
modprobe g_ether host_addr=02:00:00:00:00:02 dev_addr=$addr

ifconfig usb0 up

dhclient -d -pf /var/run/dhclient.usb0.pid usb0 > /dev/null 2>&1 &
