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

PATH=/sbin:/bin:/usr/sbin:/usr/bin:/usr/local/bin

add_device() {
    echo ${1} ${2} > /sys/class/i2c-dev/i2c-${3}/device/new_device
}

# EEPROM
add_device 24c64 0x51 6

# Two temperature sensors on CMM
add_device tmp75 0x48 3         # inlet
add_device tmp75 0x49 3         # outlet

# CMM CPLD
add_device cmmcpld 0x3e 13

# FAN CPLD
fancpld_busses="171 179 187 195"
for bus in ${fancpld_busses}; do
    add_device fancpld 0x33 ${bus}
done

# Temperature sensors
# FAB1: 106   0x48 - 0x4b
# SCM-FAB1: 113 0x48, 114 0x49

# FAB2: 122
# SCm-FAB2:

# LC101: 42, 0x48-0x4b
# LC102: 45, 0x48-0x4b
# SCM-LC1:

# LC201: 58
# LC202: 61

# LC301: 74
# LC302: 77

# LC401: 90
# LC401: 93

# FAB3: 138

# FAB4: 154
temp_busses="42 45 58 61 74 77 90 93 106 122 138 154"
temp_addrs="0x48 0x49 0x4a 0x4b"
for bus in ${temp_busses}; do
    for addr in ${temp_addrs}; do
        add_device tmp75 ${addr} ${bus}
    done
done
