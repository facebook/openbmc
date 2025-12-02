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
#

### BEGIN INIT INFO
# Provides:          mount_flash1_data0
# Required-Start:    mountvirtfs
# Required-Stop:
# Default-Start:     S
# Default-Stop:
# Short-Description: Mount flash1-data0 partition from flash chip.
# Description:
### END INIT INFO

# shellcheck disable=SC1091
if [ -f /etc/default/rcS ]; then
    . /etc/default/rcS
fi

#
# Use ubifs filesystem type for flash1-data0 partition
#
FLASH_FS_TYPE=ubifs

MOUNT_POINT="/mnt/flash1_data"

mtd_lookup_flash1_data_partition() {
    dev_flash1_data=$(awk '{ if ($4 == "\"flash1-data0\"") print $1 }' \
                          /proc/mtd | cut -d ':' -f 1)
    echo "$dev_flash1_data"
}

#
# Sanity check if directories and files are corrupted after mount.
#
mnt_point_health_check() {
    mnt_error=0
    mnt_point="$1"

    if ! ls "$mnt_point" > /dev/null 2>&1; then
        echo "$FLASH_FS_TYPE health check error: unable to list root directory!"
        mnt_error=$((mnt_error+1))
    fi

    persist_etc="${mnt_point}/etc"
    if [ -e "$persist_etc" ] && [ ! -d "$persist_etc" ] ; then
        echo "$FLASH_FS_TYPE health check error: $persist_etc corrupted (not a directory)!"
        mnt_error=$((mnt_error+1))
    fi

    return "$mnt_error"
}

ubifs_mount() {
    ubi_vol="$1"
    mnt_point="$2"

    echo "ubifs_mount $ubi_vol to $mnt_point.."
    mount -t ubifs "$ubi_vol" "$mnt_point" -o sync,compr=zstd
}

do_mount_ubifs() {
    mtd_chardev="$1"
    mnt_point="$2"
    ubi_dev="/dev/ubi1"
    ubi_vol="${ubi_dev}_0"
    need_recovery=0

    #
    # Ignore modprobe failures
    #
    modprobe ubi mtd=flash1-data0 > /dev/null 2>&1

    if ! [ -e "$ubi_dev" ]; then
        ubiattach -p "$mtd_chardev"
    fi

    #
    # Check if ubi volume exists and attempt to mount it
    #
    if [ -e "$ubi_vol" ]; then
        device_size=$(ubinfo "$ubi_dev" | sed -n '/^Total amount of logical eraseblocks:/s/.*(\([0-9]*\) bytes.*/\1/p')
        volume_size=$(ubinfo "$ubi_vol" | sed -n '/^Size:/s/.*(\([0-9]*\) bytes.*/\1/p')
        if [ -n "$device_size" ] && [ -n "$volume_size" ] && [ $((volume_size+4194304)) -lt "$device_size" ]; then
            # If UBI device is larger than volume (+4MB to account for overhead), report size mismatch
            echo "ERROR: Volume size mismatch - $ubi_vol has size $volume_size, but device $ubi_dev has size $device_size"
            need_recovery=1
        elif ubifs_mount "$ubi_vol" "$mnt_point"; then
            echo "Check ubifs filesystem health on $ubi_vol.."

            if ! mnt_point_health_check "$mnt_point"; then
                echo "ERROR: Filesystem health check failed on $ubi_vol"
                need_recovery=$((need_recovery+1))
            fi

            if [ "$need_recovery" -gt 0 ]; then
                echo "Unmounting $ubi_vol due to health check failure.."
                umount "$mnt_point"
                ubidetach -p "$mtd_chardev"
            else
                echo "ubifs health check passed: no error found."
            fi
        else
            echo "ERROR: Unable to mount $ubi_vol"
            need_recovery=1
        fi
    else
        echo "ERROR: UBI volume $ubi_vol does not exist"
        need_recovery=1
    fi

    if [ "$need_recovery" -gt 0 ]; then
        echo "$MOUNT_POINT not mounted."
        exit 1
    fi
}

MTD_FLASH1_DATA_DEV=$(mtd_lookup_flash1_data_partition)
if [ -z "$MTD_FLASH1_DATA_DEV" ]; then
    echo "No flash1-data0 partition found. $MOUNT_POINT not mounted."
    exit 1
fi

MTD_FLASH1_DATA_PATH="/dev/$MTD_FLASH1_DATA_DEV"
[ -d $MOUNT_POINT ] || mkdir -p $MOUNT_POINT

do_mount_$FLASH_FS_TYPE "$MTD_FLASH1_DATA_PATH" "$MOUNT_POINT"

: exit 0
