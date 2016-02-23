#!/bin/sh
#
# Copyright 2016-present Facebook. All Rights Reserved.
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

I2C_BUS_FCB=5

# NCT7904 is the fan controller on the FCB board
I2C_ADDR_NCT7904=0x2d

# Enable the Pins to sense the Temperature VSEN23_MD and VSEN45_MD
i2cset -y $I2C_BUS_FCB $I2C_ADDR_NCT7904 0x2E 0x5 b

#Enable the ADC controller for ADC0~ADC7
devmem 0x1E6E9000 32 0x00FF000F
