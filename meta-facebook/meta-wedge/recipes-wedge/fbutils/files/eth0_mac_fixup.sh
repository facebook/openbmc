#!/bin/bash
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

### BEGIN INIT INFO
# Provides:          eth0_mac_fixup.sh
# Required-Start:
# Required-Stop:
# Default-Start:     S
# Default-Stop:
# Short-Description:  Fixup the MAC address for eth0 based on wedge EEPROM
### END INIT INFO

PATH=/sbin:/bin:/usr/sbin:/usr/bin:/usr/local/bin

# get the MAC from EEPROM
mac=$(weutil 2>/dev/null | grep '^Local MAC' 2>/dev/null | cut -d' ' -f3 2>/dev/null)

# get the MAC from u-boot environment
ethaddr=$(fw_printenv ethaddr 2>/dev/null | cut -d'=' -f2 2>/dev/null)

if [ -z "$mac" ] && [ -n "$ethaddr" ]; then
    # no MAC from EEPROM, use the one from u-boot environment
    mac="$ethaddr"
fi

if [ -n "$mac" ]; then
    ifconfig eth0 hw ether $mac
else
    # no MAC from either EEPROM or u-boot environment
    mac=$(ifconfig eth0 2>/dev/null |grep HWaddr 2>/dev/null |awk '{ print $5 }')

fi

if [ "$ethaddr" != "$mac" ]; then
    # set the MAC from EEPROM or ifconfig back to u-boot environment so that u-boot
    # can use it
    fw_setenv "ethaddr" "$mac"
fi
