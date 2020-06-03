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
# Provides:          mount_data0
# Required-Start:    mountvirtfs
# Required-Stop:
# Default-Start:     S
# Default-Stop:
# Short-Description: Mount data0 partition from flash chip.
# Description:
### END INIT INFO

. /etc/default/rcS

MOUNT_POINT="/mnt/data"

jffs2_mount_and_check () {
  error=0
  block_dev=$1
  mnt_point=$2
  persist_etc="${mnt_point}/etc"

  if mount -t jffs2 "$block_dev" "$mnt_point"; then
    #
    # Wait a second to make sure filesystem is fully ready.
    #
    sleep 1

    echo "Checking JFFS2 filesystem health on $block_dev.."

    #
    # If JFFS2 prints below messages at mount time, then probably the
    # filesystem is corrupted. Although mount may still return "success",
    # the file structure/content can be corrupted in this case.
    # For details, please refer to:
    # http://www.linux-mtd.infradead.org/faq/jffs2.html
    #
    if dmesg | grep "jffs2_scan_eraseblock(): Magic bitmask.*not found" \
       > /dev/null 2>&1; then
      echo "JFFS2 health check error: <Magic bitmask not found> reported!"
      error=$((error+1))
    fi

    #
    # Failing to list JFFS2 root directory is definitely not acceptable.
    #
    if ! ls "$mnt_point" > /dev/null 2>&1; then
      echo "JFFS2 health check error: unable to list files in $mnt_point!"
      error=$((error+1))
    fi

    #
    # We've seen several instances of corrupted /mnt/data/etc on Minipack
    # and CMM BMC: somehow /mnt/data/etc is interpreted as a regular file
    # instead of a directory, and people lost connection to BMC in this
    # condition because sshd cannot be started.
    #
    if [ -e "$persist_etc" ] && [ ! -d "$persist_etc" ] ; then
      echo "JFFS2 health check error: $persist_etc corrupted (not a directory)!"
      error=$((error+1))
    fi

    if [ "$error" -gt 0 ]; then
      echo "Unmounting $block_dev and starting JFFS2 recovery.."
      umount "$block_dev"
    fi
  else
    echo "Unable to mount $block_dev. Starting JFFS2 recovery.."
    error=$((error+1))
  fi

  return "$error"
}

# Find out which device maps to 'data0' on mtd
# Note: /proc/mtd lists partitions using mtdX, where X is a number,
#       but we mount using /dev/mtdblockX. We'll do some magic here
#       to get the mtdX (char device) and mtdblockX (block device)
#       names.
DATA_CHAR_DEV=$(cat /proc/mtd | awk '{ if ($4 == "\"data0\"") print $1 }' |
  cut -d ':' -f 1 | awk '{ print "/dev/" $1 }')
DATARO_CHAR_DEV=$(cat /proc/mtd | awk '{ if ($4 == "\"dataro\"") print $1 }' |
  cut -d ':' -f 1 | awk '{ print "/dev/" $1 }')

if [ -z "$DATA_CHAR_DEV" ] && [ -n "$DATARO_CHAR_DEV" ]; then
  DATA_CHAR_DEV=$DATARO_CHAR_DEV
fi

if [ -z "$DATA_CHAR_DEV" ]; then
  echo "No data0/dataro partition found. Not mounting anything to $MOUNT_POINT."
else
  DATA_BLOCK_DEV=${DATA_CHAR_DEV/mtd/mtdblock}
  echo "data0 partition found on $DATA_BLOCK_DEV; mounting to $MOUNT_POINT."
  [ -d $MOUNT_POINT ] || mkdir -p $MOUNT_POINT

  if ! jffs2_mount_and_check "$DATA_BLOCK_DEV" "$MOUNT_POINT"; then
    echo "Erasing and creating JFFS2 filesystem on $DATA_CHAR_DEV.."
    if ! flash_eraseall -j "$DATA_CHAR_DEV"; then
      echo "Error: failed to erase $DATA_CHAR_DEV!"
      exit 1
    fi

    echo "Re-mounting $DATA_BLOCK_DEV to $MOUNT_POINT.."
    if ! mount -t jffs2 "$DATA_BLOCK_DEV" "$MOUNT_POINT"; then
      echo "Error: failed to mount $DATA_BLOCK_DEV! Exiting!"
      exit 1
    fi
  fi
fi

: exit 0
