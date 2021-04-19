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

SUPPORTED_FS_TYPE="btrfs"

mount_btrfs() {

    #
    # Use blkid to find the underlying filesystem type.
    #
    FS_TYPE="$(blkid "$BLK_DEVICE" || echo 'TYPE="unknown"')"
    FS_TYPE="$(echo "$FS_TYPE" | sed 's/.*TYPE="\([^"]*\).*/\1/')"

    #
    # If filesystem is not btrfs already, convert or reformat.
    #
    case "$FS_TYPE" in
        ext4)
            echo "EXT4 filesystem found on $BLK_DEVICE.  Reformat as btrfs.."
            if ! output="$(mkfs.btrfs -f "$BLK_DEVICE" 2>&1)"; then
                echo "Error: failed to format as btrfs on $BLK_DEVICE:"
                echo "$output"
                exit 1
            fi
            ;;

        btrfs)
            ;;

        *)
            echo "No filesystem found on $BLK_DEVICE. Creating btrfs fs.."
            if ! output=$(mkfs.btrfs -f "$BLK_DEVICE" 2>&1); then
                echo "Error: failed to create btrfs on $BLK_DEVICE:"
                echo "$output"
                exit 1
            fi
            ;;
    esac

    #
    # Run btrfs-check (same as btrfsck) to confirm filesystem integrity.
    #
    if ! output="$(btrfs check "$BLK_DEVICE" 2>&1)"; then
        echo "Error: btrfs volume shows errors:"
        echo "$output"
        echo "Try to repair btrfs"
        if ! output="$(btrfs check --repair "$BLK_DEVICE" 2>&1)"; then
            echo "Error: btrfs volume shows errors:"
            echo "$output"
            exit 1
        fi
    fi

    #
    # Mount btrfs.
    #   Use zstd compression because we get faster speed and less wear.
    #
    echo "Mounting $BLK_DEVICE to $MOUNT_POINT.."
    if ! output=$(mount -o compress=zstd,discard "$BLK_DEVICE" "$MOUNT_POINT" 2>&1);
    then
        echo "Error: failed to mount $BLK_DEVICE to $MOUNT_POINT:"
        echo "$output"
        exit 1
    fi

    #
    # Mount successful.  Show some usage statstics.
    #
    echo "$MOUNT_POINT mounted successfully, device usage:"
    df -h "$MOUNT_POINT"
    btrfs filesystem df "$MOUNT_POINT"
    exit 0
}

mount_ext4() {

    #
    # Workaround for EMMC data lost because sometimes e2fsck detect filesystem fails
    # if SDHCI IO fails when system boot up, add retry to avoid EMMC format by mistake.
    #
    maxtry=3
    retry=0
    while [ $retry -lt $maxtry ]; do
        if ! e2fsck -p "$BLK_DEVICE" > /dev/null 2>&1; then
            retry=$((retry + 1))
            echo "No filesystem found on $BLK_DEVICE try: $retry."
        else
            break
        fi
    done

    #
    # Create ext4 if ext filesystem is not detected or corrupted.
    #
    if [ $retry -eq $maxtry ]; then
        echo "No filesystem found on $BLK_DEVICE after $maxtry times try. Creating ext4 filesystem.."
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
}

mount_$SUPPORTED_FS_TYPE
