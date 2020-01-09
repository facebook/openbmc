#!/bin/sh
#
# Copyright 2018-present Facebook. All Rights Reserved.
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

. /usr/local/bin/openbmc-utils.sh

# TODO:
#	This is a workaround before EVT
#	CPLD will handle with 12V power when BMC is ready.
power-util mb on > /dev/null

# Thermal sensors for PCIe switch
i2c_device_add 6 0x4d tmp422
i2c_device_add 6 0x4e tmp422

i2c_device_add 5 0x30 mpq8645p
i2c_device_add 5 0x31 mpq8645p
i2c_device_add 5 0x32 mpq8645p
i2c_device_add 5 0x33 mpq8645p
i2c_device_add 5 0x34 mpq8645p
i2c_device_add 5 0x35 mpq8645p
i2c_device_add 5 0x36 mpq8645p
i2c_device_add 5 0x3b mpq8645p
