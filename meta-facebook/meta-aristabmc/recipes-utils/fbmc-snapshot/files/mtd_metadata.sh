#!/bin/bash
#
# Copyright 2026-present Facebook. All Rights Reserved.
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

# Aristabmc's FBOBMC JSON metadata partition begins at 0x3e8000 and is 96 KiB.
START_OFFSET_KB=$((0x003e8000 / 1024))
LEN_KB=96

usage() {
    echo "Usage $0 [flash0|flash1]"
}

case "${1:-}" in
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

if [ -z "$mtd" ]; then
    echo "Unable to find MTD device for $1" >&2
    exit 1
fi

metadata=$(dd if="/dev/$mtd" bs=1K skip="$START_OFFSET_KB" \
    count="$LEN_KB" 2>/dev/null | strings)

if [ -z "$metadata" ]; then
    echo "Unable to find Aristabmc image metadata on $1" >&2
    exit 1
fi

printf '%s\n' "$metadata"
