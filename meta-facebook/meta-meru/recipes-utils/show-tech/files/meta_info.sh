#!/bin/bash
#
# Copyright 2023-present Facebook. All Rights Reserved.
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

# This utility prints the meta information for specified boot flash
# mtd partition
START_OFFSET_KB=960
LEN_KB=64

usage() {
    echo "Usage $0 [flash0|flash1]"
}

case "$1" in
    flash0)
        mtd="$(grep flash0 /proc/mtd | awk '{print $1}' | tr -d ':')"
        ;;
    flash1)
        mtd="$(grep flash1 /proc/mtd | awk '{print $1}' | tr -d ':')"
        ;;
    *)
        usage
        exit 1
        ;;
esac

dd if=/dev/"$mtd" of=/tmp/."$1"_meta bs=1K skip="$START_OFFSET_KB" count="$LEN_KB"
strings /tmp/."$1"_meta
