#!/bin/sh
#
# Copyright 2020-present Facebook. All Rights Reserved.
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

set -eu -o pipefail

PATH=/sbin:/bin:/usr/sbin:/usr/bin:/usr/local/bin

if [ $# -ne 2 ]; then
    echo "Error: invalid command line arguments!"
    echo "Usage: $0 <block-device> <mount-point>"
    exit 1
fi
BLK_DEVICE=$1
MOUNT_POINT=$2

if mount | grep "$BLK_DEVICE"; then
    echo "$BLK_DEVICE is already mounted. Exiting."
    exit 0
fi

#
# Create ext4 if ext filesystem is not detected or corrupted.
#
if ! e2fsck -p "$BLK_DEVICE" > /dev/null 2>&1; then
    echo "No filesystem found on $BLK_DEVICE. Creating ext4 filesystem.."
    if ! output=$(mkfs.ext4 -F "$BLK_DEVICE" 2>&1); then
        echo "Error: failed to create ext4 on $BLK_DEVICE:"
        echo "$output"
        exit 1
    fi
fi

#
# Mount block device to specified mount point.
#
echo "Mounting $BLK_DEVICE to $MOUNT_POINT.."
if ! output=$(mount "$BLK_DEVICE" "$MOUNT_POINT" 2>&1); then
    echo "Error: failed to mount $BLK_DEVICE to $MOUNT_POINT:"
    echo "$output"
    exit 1
fi

#
# Dump summary information.
#
echo "$BLK_DEVICE mounted successfully, device usage:"
df -h "$BLK_DEVICE"
exit 0
