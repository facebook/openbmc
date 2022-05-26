#!/bin/bash
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

#shellcheck disable=SC1091
. /usr/local/bin/openbmc-utils.sh

get_version () {
    length=$( stat -c%s "$1" )
    ((i = length - 4))
    ((major_offset_found = 0))
    while [[ $i -ne 0 ]]
    do
        var=$( dd skip=$i count=4 if="$1" bs=1 2> /dev/null )
        if [ "$var" = "fa7f" ];then
            if [ $major_offset_found -eq 0 ]; then
                ((major_offset_found = 1))
                ((major_offset = i + 4))
            else
                ((minor_offset = i + 4))
                break
            fi
        fi
        ((i = i - 1))
    done
    major=$( dd skip=$major_offset count=2 if="$1" bs=1 2> /dev/null)
    minor=$( dd skip=$minor_offset count=2 if="$1" bs=1 2> /dev/null)
    echo "version is $major.$minor"
}

# GB switch firmware upgrade command
gb_upgrade() {

    if [ ! -f /sys/bus/i2c/devices/8-0050/eeprom ]; then
        echo "/sys/bus/i2c/devices/8-0050/eeprom does not exist"
        return 1
    fi

    if [ "$1" = "upgrade" ];then
        if [ -f "$2" ]; then
            oob_switch_eeprom_select 
            cat < "$2" 1<> /sys/bus/i2c/devices/8-0050/eeprom
            oob_switch_eeprom_deselect 
        else 
            echo "$2 does not exist"
        fi
    fi

    if [ "$1" = "enable" ];then
        oob_switch_eeprom_select 
    fi

    if [ "$1" = "disable" ];then
        oob_switch_eeprom_deselect 
    fi

    if [ "$1" = "reset" ];then
        oob_switch_reset 
    fi

    if [ "$1" = "erase" ];then
        printf '\xff' > /tmp/erasefile
        oob_switch_eeprom_select 
        ((i = 0))
        while [[ $i -lt 512 ]]
        do
            dd seek=$i count=1 if=/tmp/erasefile of=/sys/bus/i2c/devices/8-0050/eeprom bs=1 2> /dev/null
            ((i = i +1))
        done
        oob_switch_eeprom_deselect 
        rm /tmp/erasefile
    fi

    if [ "$1" = "eeprom_version" ];then
        oob_switch_eeprom_select
        hexdump -ve '1/1 "%.2x"' /sys/bus/i2c/devices/8-0050/eeprom > /tmp/eeprom_data
        oob_switch_eeprom_deselect
        get_version /tmp/eeprom_data
        rm /tmp/eeprom_data
    fi

    if [ "$1" = "packaged_version" ];then
        hexdump -ve '1/1 "%.2x"' "$2" > /tmp/eeprom_data
        get_version /tmp/eeprom_data
        rm /tmp/eeprom_data
    fi

    if [ "$1" = "check" ];then
        i2cset -f -y 12 0x31 0x0c
        i2cset -f -y 12 0x32 0x00
        i2cset -f -y 12 0x33 0x00
        i2cset -f -y 12 0x34 0x00
        i2cset -f -y 12 0x30 0x1
        access=$( i2cget -f -y 12 0x35 )
        if [ $(( access & 0x4 )) -eq $(( 0x0 )) ]; then
            echo "access is disabled"
        else
            echo "access is enabled"
        fi
    fi
}

case "$1" in
    "--help" | "?" | "-h")
        program=$(basename "$0")
        echo "Usage: switch firmware upgrade command"
        echo "  <$program upgrade image> enable access to switch EEPROM, upgrade FW with provided image, then disable access"
        echo "  <$program enable> enable access to switch EEPROM for firmware upgrade"
        echo "  <$program disable> disable access to switch EEPROM"
        echo "  <$program reset> reset switch, can be done after FW upgrade to activate new image"
        echo "  <$program erase> erase the FW image from the switch EEPROM, switch will boot with HW configuration after next reset"
        echo "  <$program eeprom_version> get the FW version from the switch EEPROM"
        echo "  <$program packaged_version image> get the FW version from the switch FW image file" 
        echo "  <$program check> check access status to switch EEPROM"
    ;;

    *)
        gb_upgrade "$1" "$2"
    ;;
esac
