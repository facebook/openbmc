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

# EEPROM
echo 24c64 0x51 > /sys/class/i2c-dev/i2c-6/device/new_device

# Two temperature sensors on CMM
echo tmp75 0x48 > /sys/class/i2c-dev/i2c-3/device/new_device # inlet
echo tmp75 0x49 > /sys/class/i2c-dev/i2c-3/device/new_device # outlet
