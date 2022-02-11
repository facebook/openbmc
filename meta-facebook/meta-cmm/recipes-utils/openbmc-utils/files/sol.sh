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

PATH=/sbin:/bin:/usr/sbin:/usr/bin:/usr/local/bin

usage() {
    echo "Usage: $0 <card name>"
    echo "    card name: lc101, lc102, ..., fc3, fc4, cmm"
}

if [ $# -ne 1 ]; then
    usage "$0"
    exit -1
fi

declare -A card_uart_map
card_uart_map[fc1]=0x1
card_uart_map[fc2]=0x2
card_uart_map[fc3]=0x3
card_uart_map[fc4]=0x4
card_uart_map[lc101]=0x5
card_uart_map[lc102]=0x9
card_uart_map[lc201]=0x6
card_uart_map[lc202]=0xa
card_uart_map[lc301]=0x7
card_uart_map[lc302]=0xb
card_uart_map[lc401]=0x8
card_uart_map[lc402]=0xc
card_uart_map[cmm]=0xd

value=${card_uart_map[${1,,}]}
if [ -z "$value" ]; then
    usage "$0"
    exit -1
fi

sysfs_path="/sys/class/i2c-dev/i2c-13/device/13-003e/uart3_mux_ctrl"

echo "You are in SOL session."
echo "Use ctrl-x to quit."
echo "-----------------------"
echo

trap 'echo 0 > "$sysfs_path"' INT TERM QUIT EXIT

echo $value > "$sysfs_path"

/usr/bin/microcom -s 9600 /dev/ttyS2

echo
echo
echo "-----------------------"
echo "Exit from SOL session."
