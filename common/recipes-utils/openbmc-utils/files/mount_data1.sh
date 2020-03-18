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
#

### BEGIN INIT INFO
# Provides:          mount_data1.sh
# Required-Start:
# Required-Stop:
# Default-Start:     S
# Default-Stop:
# Short-Description: Mount persistent storage (currently eMMC) to /mnt/data1
# Description:
### END INIT INFO

MOUNT_POINT="/mnt/data1"
if [ ! -d "$MOUNT_POINT" ]; then
    if ! output=$(mkdir -p "$MOUNT_POINT"); then
        echo "Error: failed to create $MOUNT_POINT directory:"
        echo "$output"
        exit 1
    fi
fi

/usr/local/bin/emmc_auto_mount.sh "$MOUNT_POINT"
