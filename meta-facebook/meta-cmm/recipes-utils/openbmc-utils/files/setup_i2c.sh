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

##### NOTE #########################################################
# When adding new devices, always add to end of the list here
# so any other dependencies that are hard coded with GPIO
# will not break.
####################################################################

PATH=/sbin:/bin:/usr/sbin:/usr/bin:/usr/local/bin

add_device() {
    echo ${1} ${2} > /sys/class/i2c-dev/i2c-${3}/device/new_device
}

# EEPROM
add_device 24c64 0x51 6

#PCB EEPROM
add_device 24c64 0x50 28

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
# SCM-FAB2: 129 0x48, 130 0x49

# LC101: 42, 0x48-0x4b
# LC102: 45, 0x48-0x4b
# SCM-LC1: 49 0x48, 50 0x49

# LC201: 58
# LC202: 61
# SCM-LC2: 65 0x48, 66, 0x49

# LC301: 74
# LC302: 77
# SCM-LC3: 81 0x48, 82, 0x49

# LC401: 90
# LC401: 93
# SCM-LC4: 97 0x48, 98 0x49

# FAB3: 138
# SCM-FAB3: 145 0x48, 146 0x49

# FAB4: 154
# SCM-FAB4: 161 0x48, 162 0x49

# Temperature sensors on LC/FC
temp_busses="42 45 58 61 74 77 90 93 106 122 138 154"
temp_addrs="0x48 0x49 0x4a 0x4b"
for bus in ${temp_busses}; do
    for addr in ${temp_addrs}; do
        add_device tmp75 ${addr} ${bus}
    done
done

# Temperature sensors on SCM
temp_busses="113 129 49 65 81 97 145 161"
for bus in ${temp_busses}; do
    add_device tmp75 0x48 ${bus}
    add_device tmp75 0x49 $((bus + 1))
done

# FCB PCA9534
# GPIOs are allocated backwards.
# So, list the bus list in reverse order,
# so that GPIO numbers are increasing from FCB1-FCB4
fcb_busses="196 188 180 172"
for bus in ${fcb_busses}; do
    add_device pca9534 0x22 ${bus}
done

# PSU PRESENCE
# Spec says pca9506 but this kernel version supports only 9505
# which works for us
add_device pca9505 0x20 29

# PFE modules
add_device pfe3000 0x10 27 # psu1
add_device pfe3000 0x10 26 # psu2
add_device pfe3000 0x10 25 # psu3
add_device pfe3000 0x10 24 # psu4

# LEDs
add_device pca9535 0x21 30
