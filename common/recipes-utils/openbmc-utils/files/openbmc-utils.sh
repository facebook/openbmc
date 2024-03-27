#!/bin/bash

# Copyright 2015-present Facebook. All Rights Reserved.
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

# shellcheck disable=SC1091

DEVMEM=/sbin/devmem

devmem_set_bit() {
    local addr
    local val
    addr=$1
    val=$($DEVMEM "$addr")
    val=$((val | (0x1 << $2)))
    $DEVMEM "$addr" 32 $val
}

devmem_clear_bit() {
    local addr
    local val
    addr=$1
    val=$($DEVMEM "$addr")
    val=$((val & ~(0x1 << $2)))
    $DEVMEM "$addr" 32 $val
}

#
# Test if the given bit is set in register <addr>
# $1 - register address
# $2 - bit position
#
devmem_test_bit() {
    local addr=$1
    local bitpos=$2
    local val

    val=$($DEVMEM "$addr")
    val=$(((val >> "$bitpos") & 0x1))

    if [ $val -eq 1 ]; then
        # 0 for true
        return 0
    else
        # 1 for false
        return 1
    fi
}

#
# Increment the given MAC address
# $1 - MAC address in "##:##:##:##:##:##" format
#
mac_addr_inc() {
    mac_hex=$(echo "$1" | tr -d ':')
    new_mac=$((0x$mac_hex + 1))
    printf "%012X" "$new_mac" | sed 's/../&:/g;s/:$//'
}

#
# Lookup mtd device based on name/label.
# $1 - mtd partition name/label.
#
mtd_lookup_by_name() {
    mtd=$(awk -v name="$1" '$4~name {print $1}' /proc/mtd |
          cut -d ':' -f 1)
    echo "$mtd"
}

#
# Write a value to the given sysfs file.
# $1 - sysfs pathname
# $2 - input value
#
sysfs_write() {
    pathname="$1"
    value="$2"

    if [ ! -e "$pathname" ]; then
        echo "Error: $pathname does not exist!"
        return 1
    fi

    if ! echo "$value" > "$pathname"; then
        echo "Error: failed to write $value to $pathname!"
        return 1
    fi
}

source "/usr/local/bin/shell-utils.sh"
source "/usr/local/bin/i2c-utils.sh"
source "/usr/local/bin/gpio-utils.sh"
source "/usr/local/bin/ast-utils.sh"
source "/usr/local/bin/reset_script_utils.sh"

if [ -f "/usr/local/bin/soc-utils.sh" ]; then
    source "/usr/local/bin/soc-utils.sh"
fi

if [ -f "/usr/local/bin/board-utils.sh" ]; then
    source "/usr/local/bin/board-utils.sh"
fi
