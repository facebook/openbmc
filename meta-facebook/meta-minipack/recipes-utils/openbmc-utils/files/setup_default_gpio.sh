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

### BEGIN INIT INFO
# Provides:          setup_board.sh
# Required-Start:
# Required-Stop:
# Default-Start:     S
# Default-Stop:
# Short-Description: Setup the board
### END INIT INFO

. /usr/local/bin/openbmc-utils.sh

PATH=/sbin:/bin:/usr/sbin:/usr/bin:/usr/local/bin

board_rev=$(wedge_board_rev)

echo -n "Setting up Default GPIOs value... "

# EVTA is 4, EVTB is 0
if [ $board_rev -ne 4 ]; then
    # Set BMC_RST_FPGA(GPIOP2) pin to high, IOB FPGA in normal mode
    gpio_set BMC_RST_FPGA 1
fi

# Set all reset pin to high, keep in normal mode
gpio_set BMC_PIM1_9548_RST_N 1
gpio_set BMC_PIM2_9548_RST_N 1
gpio_set BMC_PIM3_9548_RST_N 1
gpio_set BMC_PIM4_9548_RST_N 1
gpio_set BMC_PIM5_9548_RST_N 1
gpio_set BMC_PIM6_9548_RST_N 1
gpio_set BMC_PIM7_9548_RST_N 1
gpio_set BMC_PIM8_9548_RST_N 1
gpio_set BMC10_9548_1_RST 1
gpio_set BMC12_9548_2_RST 1
gpio_set BMC9_9548_0_RST 1
gpio_set BMC_eMMC_RST_N 1

echo "done."
