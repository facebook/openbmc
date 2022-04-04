#!/bin/bash
#
# Copyright (c) 2022 Cisco Systems Inc.
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
#

### BEGIN INIT INFO
# Provides:          eth0_mac_fixup.sh
# Required-Start:
# Required-Stop:
# Default-Start:     S
# Default-Stop:
# Short-Description:  Fixup the MAC address for eth0 based on BMC EEPROM
### END INIT INFO

PATH=/sbin:/bin:/usr/sbin:/usr/bin:/usr/local/bin:/opt/cisco/bin

# get the MAC from EEPROM
mac=00:00:00:00:00:00
if [ -z "/opt/cisco/bin/idprom" ]; then
    temp_mac=$(idprom /sys/bus/i2c/devices/8-0054/eeprom | grep MACADDR_BASE | awk '{print $2}')
    if [ -z "$temp_mac" ]; then
        echo "mac address not found"
    else
        mac=$(sed -e "s/\(..\)\(..\)\(..\)\(..\)\(..\)\(..\)/\1:\2:\3:\4:\5:\6/" <<< "$temp_mac")
    fi
else
    # Temporary workaround until we get the idprom utility via PR for BSP
    hexdump -ve '1/1 "%.2x"' /sys/bus/i2c/devices/8-0054/eeprom > hexdump
    ((i = 0))
    while [[ $i -lt 512 ]]
    do
        var=$( dd skip=$i count=4 if=hexdump bs=1 2> /dev/null )
        if [ "$var" = "cf06" ];then
            ((i = i + 4))
            mac1=$( dd skip=$i count=2 if=hexdump bs=1 2> /dev/null)
            ((i = i + 2))
            mac2=$( dd skip=$i count=2 if=hexdump bs=1 2> /dev/null)
            ((i = i + 2))
            mac3=$( dd skip=$i count=2 if=hexdump bs=1 2> /dev/null)
            ((i = i + 2))
            mac4=$( dd skip=$i count=2 if=hexdump bs=1 2> /dev/null)
            ((i = i + 2))
            mac5=$( dd skip=$i count=2 if=hexdump bs=1 2> /dev/null)
            ((i = i + 2))
            mac6=$( dd skip=$i count=2 if=hexdump bs=1 2> /dev/null)

            mac=$mac1:$mac2:$mac3:$mac4:$mac5:$mac6
            break
        fi
        ((i = i + 1))
    done
    rm hexdump
fi

echo "mac is $mac"

# get the MAC from u-boot environment
ethaddr=$(fw_printenv | grep "^ethaddr=" | cut -d'=' -f2)

if [ "$mac" = "00:00:00:00:00:00" ] || [ "$mac" = "ff:ff:ff:ff:ff:ff" ]; then
    if [ -n "$ethaddr" ]; then
        # no MAC from EEPROM, use the one from u-boot environment
        echo "No MAC address from EEPROM: use $ethaddr from uboot-env"
        mac="$ethaddr"
    else
        echo "Error: unable to read MAC address from EEPROM or uboot-env!"
        mac=ba:d0:4a:9c:4e:ce
    fi
fi

if [ "$ethaddr" != "$mac" ]; then
    # set the MAC from EEPROM back to u-boot environment so that
    # u-boot can use it
    fw_setenv "ethaddr" "$mac"
fi

echo "Update BMC eth0 MAC address to $mac"
ip link set dev eth0 address "$mac"
exit $?
