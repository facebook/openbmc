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

# Enable the isolation buffer between BMC and COMe i2c bus
echo 1 > ${SCMCPLD_SYSFS_DIR}/i2c_bus_en
echo 0 > ${SCMCPLD_SYSFS_DIR}/iso_com_early_en

# Make the backup BIOS flash connect to COMe instead of BMC
echo 0 > ${SCMCPLD_SYSFS_DIR}/com_spi_oe_n
echo 0 > ${SCMCPLD_SYSFS_DIR}/com_spi_sel

# Setup management port LED
/usr/local/bin/setup_mgmt.sh led &

# Setup TH3 PCI-e repeater
/usr/local/bin/setup_pcie_repeater.sh th3 write

# Init server_por_cfg
if [ ! -f "/mnt/data/kv_store/server_por_cfg" ]; then
    /usr/bin/kv set server_por_cfg on persistent
fi

/usr/bin/kv set smb_board_rev $board_rev
