#!/bin/sh
#
# Copyright 2019-present Facebook. All Rights Reserved.
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

BUS=$1

. /usr/local/bin/openbmc-utils.sh

# Remove stale device if present from a previous run
if [ -e /sys/bus/i2c/devices/${BUS}-1010 ]; then
    i2c_device_delete ${BUS} 0x1010
fi

# Create the slave-mqueue device
if ! i2c_device_add ${BUS} 0x1010 slave-mqueue; then
    echo "Failed to add slave-mqueue for bus ${BUS}"
    exit 1
fi
