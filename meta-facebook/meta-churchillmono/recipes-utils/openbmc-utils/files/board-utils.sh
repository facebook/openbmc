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
# shellcheck disable=SC1091

# enable access to GE switch EEPROM
oob_switch_eeprom_select() {

    i2cset -f -y 12 0x31 0x0c
    i2cset -f -y 12 0x32 0x00
    i2cset -f -y 12 0x33 0x00
    i2cset -f -y 12 0x34 0x00
    i2cset -f -y 12 0x35 0x07
    i2cset -f -y 12 0x36 0x2a
    i2cset -f -y 12 0x30 0x0
}

# disable access to GE switch EEPROM
oob_switch_eeprom_deselect() {

    i2cset -f -y 12 0x31 0x0c
    i2cset -f -y 12 0x32 0x00
    i2cset -f -y 12 0x33 0x00
    i2cset -f -y 12 0x34 0x00
    i2cset -f -y 12 0x35 0x03
    i2cset -f -y 12 0x36 0x2a
    i2cset -f -y 12 0x30 0x0
}

# read mac address from eeprom
read_mac_from_eeprom() {

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

    echo $mac
}
