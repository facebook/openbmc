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

if [ $board_rev -ge 2 ]; then
    # DVT or later
    # Enable the isolation buffer between BMC and COMe i2c bus
    echo 1 > ${SYSCPLD_SYSFS_DIR}/com-e_i2c_isobuf_en
    # Enable the COM6_BUF_EN to open the isolation buffer between COMe BIOS
    # EEPROM with COMe
    gpio_set COM6_BUF_EN 0
    # Make the BIOS EEPROM connect to COMe instead of BMC
    gpio_set COM_SPI_SEL 0
fi
