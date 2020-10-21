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
mac=$(weutil  | grep '^Local MAC' | cut -d' ' -f3)

# get the MAC from u-boot environment
ethaddr=$(fw_printenv | grep "^ethaddr=" | cut -d'=' -f2)

if [ -z "$mac" ]; then
    if [ -n "$ethaddr" ]; then
        # no MAC from EEPROM, use the one from u-boot environment
        echo "No MAC address from EEPROM: use $ethaddr from uboot-env"
        mac="$ethaddr"
    else
        echo "Error: unable to read MAC address from EEPROM or uboot-env!"
        exit 1
    fi
fi

if [ "$ethaddr" != "$mac" ]; then
    # set the MAC from EEPROM back to u-boot environment so that
    # u-boot can use it
    fw_setenv "ethaddr" "$mac"
fi

#ifconfig eth0 hw ether $macifconfig
echo "Update BMC eth0 MAC address to $mac"
ip link set dev eth0 address "$mac"
exit $?
