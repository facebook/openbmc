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
#

### BEGIN INIT INFO
# Provides:          sensor-setup
# Required-Start:    power-on
# Required-Stop:
# Default-Start:     S
# Default-Stop:
# Short-Description: Power on micro-server
### END INIT INFO
. /usr/local/fbpackages/utils/ast-functions

#Set PWR_AVR to 128 samples
i2cset -y -f 7 0x11 0xd4 0x3F1C w

#calibrtion to get HSC to trigger 123A based on EE team input
# 0xDC3 = 3523 (dec)
#(3523*10-20475)/ (800*0.15)= 122.958A
i2cset -y -f 7 0x11 0x4a 0x0DC3 w

#Enable IOUT_OC_WARN_EN1
i2cset -y -f 7 0x11 0xD5 0x0400 w
