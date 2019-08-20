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

### BEGIN INIT INFO
# Provides:          setup_emmc.sh
# Required-Start:
# Required-Stop:
# Default-Start:     S
# Default-Stop:
# Short-Description: Format and mount mmcblk0 partition from EMMC.
# Description:
### END INIT INFO

MOUNT_POINT="/mnt/data1"
DATA_BLOCK_DEV="/dev/mmcblk0"

# Check eMMC exist or not, if exist and eMMC can not mount.
# That means eMMC without filesystem.
# Then format eMMC with ext4 filesystem and mount on /mnt/data1.
if [ ! -b "$DATA_BLOCK_DEV" ]; then
    echo "No mmcblk0 partition found. Not mounting anything to $MOUNT_POINT."
else
    echo "mmcblk0 partition found on $DATA_BLOCK_DEV, mounting to $MOUNT_POINT."
    [ -d $MOUNT_POINT ] || mkdir -p $MOUNT_POINT
    mount -t ext4 $DATA_BLOCK_DEV $MOUNT_POINT 2> /dev/null

    if [ $? -ne 0 ]; then
        echo "Mount failed; formatting EXT4 filesystem on mmcblk0 partition."
        mkfs.ext4 /dev/mmcblk0
        echo "Mounting $DATA_BLOCK_DEV to $MOUNT_POINT."
        mount -t ext4 $DATA_BLOCK_DEV $MOUNT_POINT 2> /dev/null
    fi
fi
