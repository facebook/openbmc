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

# Light up sys led
i2cset -f -y 50 0x20 0x06 0x00
i2cset -f -y 50 0x20 0x07 0x00
i2cset -f -y 50 0x20 0x03 0xfd

# Enable the isolation buffer between BMC and COMe i2c bus
echo 1 > ${SCMCPLD_SYSFS_DIR}/i2c_bus_en
echo 0 > ${SCMCPLD_SYSFS_DIR}/iso_com_en

# Make the backup BIOS flash connect to COMe instead of BMC
echo 0 > ${SCMCPLD_SYSFS_DIR}/com_spi_oe_n
echo 0 > ${SCMCPLD_SYSFS_DIR}/com_spi_sel

# Disable watchdog temporarily because fscd is not ready
/usr/local/bin/watchdog_ctrl.sh off

# EVTA is 4, EVTB is 0
if [ $board_rev -ne 4 ]; then
    # Set BMC_RST_FPGA(GPIOP2) pin to high, IOB FPGA in normal mode
    gpio_set BMC_RST_FPGA 1
fi

# Setup management port LED
/usr/local/bin/setup_mgmt.sh led
