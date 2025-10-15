#!/bin/bash
#
# Copyright 2025-present Facebook. All Rights Reserved.
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

# shellcheck disable=SC1091
. /usr/local/bin/openbmc-utils.sh

trap cleanup INT TERM QUIT EXIT

cleanup() {
    rm -f "$TMP_BIOS_FILE"
}

if [ ! -f "$BIOS_VER_CACHE" ]; then
    # force read it
    bios_util.sh read "$TMP_BIOS_FILE" --partition bios_ver > /dev/null 2>&1
    ver=$(dd if="$TMP_BIOS_FILE" bs="$SECTION_BLOCK_SIZE" count="$BIOS_VER_BLOCKS" \
          skip="$BIOS_VER_SKIP" 2>/dev/null | grep -a "CONFIG_LOCALVERSION" \
          | awk -F'[/=/"]' '{print $3}')
    if [ -z "$ver" ]; then
        ver="UNKNOWN"
    fi
    echo "$ver" > "$BIOS_VER_CACHE"
fi

cat "$BIOS_VER_CACHE"
