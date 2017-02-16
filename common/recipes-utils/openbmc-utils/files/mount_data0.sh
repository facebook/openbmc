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

# Find out which device maps to 'data0' on mtd
# Note: /proc/mtd lists partitions using mtdX, where X is a number,
#       but we mount using /dev/mtdblockX. We'll do some magic here
#       to get the mtdX (char device) and mtdblockX (block device)
#       names.
MOUNT_POINT="/mnt/data"
DATA_CHAR_DEV=$(cat /proc/mtd | awk '{ if ($4 == "\"data0\"") print $1 }' |
  cut -d ':' -f 1 | awk '{ print "/dev/" $1 }')
DATARO_CHAR_DEV=$(cat /proc/mtd | awk '{ if ($4 == "\"dataro\"") print $1 }' |
  cut -d ':' -f 1 | awk '{ print "/dev/" $1 }')

if [ -z "$DATA_CHAR_DEV" ]; then
  if [ ! -z "$DATARO_CHAR_DEV" ]; then
    DATA_CHAR_DEV=$DATARO_CHAR_DEV
  fi
fi

if [ -z "$DATA_CHAR_DEV" ]
then
  echo "No data0/dataro partition found. Not mounting anything to $MOUNT_POINT."
else
  DEVICE_ID=$(echo $DATA_CHAR_DEV | tail -c 2)
  DATA_BLOCK_DEV=${DATA_CHAR_DEV/mtd/mtdblock}

  echo "data0 partition found on $DATA_BLOCK_DEV; mounting to $MOUNT_POINT."
  [ -d $MOUNT_POINT ] || mkdir -p $MOUNT_POINT
  mount -t jffs2 $DATA_BLOCK_DEV $MOUNT_POINT

  # if the mount failed, format the partition and remount
  if [ $? -ne 0 ]
  then
    echo "Mount failed; formatting $DATA_BLOCK_DEV and remounting."
    flash_eraseall $DATA_CHAR_DEV
    mount -t jffs2 $DATA_BLOCK_DEV $MOUNT_POINT
  fi
fi

: exit 0
