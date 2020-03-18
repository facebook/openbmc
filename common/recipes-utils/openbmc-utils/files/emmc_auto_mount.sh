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

if [ $# -ne 1 ]; then
    echo "Error: invalid command line arguments!"
    echo "Usage: $0 <mount-point>"
    exit 1
fi
MOUNT_POINT=$1
MMC_DEVICE="/dev/mmcblk0"
MOUNT_SCRIPT="/usr/local/bin/blkdev_mount.sh"

#
# We provide people a way to disable/skip the script by setting u-boot
# environment variable, and it's mainly intended for cases when something
# is wrong with emmc driver/device.
#
emmc_mount_precheck() {
    lock_dir="/run/lock"
    link_path="/var/lock"

    #
    # the script may be triggered when "/var/lock" is not populated, so
    # let's manually create the directories to avoid fw_printenv failure
    # when creating "/var/lock/fw_printenv.lock".
    #
    if [ ! -d "$lock_dir" ]; then
        if ! output=$(mkdir -p "$lock_dir"); then
            echo "Error: failed to create $lock_dir:"
            echo "$output"
            exit 1
        fi
    fi
    if [ ! -L "$link_path" ]; then
        if ! output=$(ln -s "$lock_dir" "$link_path"); then
            echo "Error: failed to create symlink $link_path:"
            echo "$output"
            exit 1
        fi
    fi

    if fw_printenv | grep "emmc_auto_mount=no" > /dev/null 2>&1; then
        echo "emmc_auto_mount is disabled. Skip emmc mount."
        exit 0
    fi
}

emmc_mount_precheck

#
# timeout "-t" option is removed since Busybox version v1.30.0, so we
# need to get rid of "-t" from all yocto-warrior platforms (Busybox
# v1.30.1), but the option is still needed for all yocto-rocko platforms
# (without Busybox v1.24.1).
# MOUNT_SCRIPT takes ~25 seconds if ext4 filesystem needs to be created,
# so 60 seconds timeout value is long enough.
#
MOUNT_TIMEOUT=60
if timeout --help 2>&1 | grep "Usage:.*\[\-t " > /dev/null 2>&1; then
    timeout -t "$MOUNT_TIMEOUT" "$MOUNT_SCRIPT" "$MMC_DEVICE" "$MOUNT_POINT"
else
    timeout "$MOUNT_TIMEOUT" "$MOUNT_SCRIPT" "$MMC_DEVICE" "$MOUNT_POINT"
fi
