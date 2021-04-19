#!/bin/bash
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
#

# shellcheck disable=SC1091
. /usr/local/bin/openbmc-utils.sh


# This utility displays the FPGA versions in each PIM SPI Flash parition

echo "------PIM-FLASH-CONFIG------"
partition_list="header_pim_base header_pim16q header_pim8ddm"
for partition in ${partition_list}; do
    PIM_REVISION_FILE="/tmp/.pim_spi_header_$partition"
    if [ ! -f "$PIM_REVISION_FILE" ]; then
        # version is not cached, force read it
        /usr/local/bin/fpga_util.sh "$partition" read \
                                    "$PIM_REVISION_FILE" > /dev/null 2>&1
        if [ ! -f "$PIM_REVISION_FILE" ]; then
            echo "Failed to read header for $partition"
            exit 1
        fi
    fi
    ver="$(hexdump -n2 "$PIM_REVISION_FILE" | cut -c 9-)"
    val_major="0x${ver:2:2}"
    val_minor="0x${ver:0:2}"
    if [ "$((val_major))" -eq 0 ]; then
        echo "${partition^^}: NOT PRESENT"
     elif [ "$((val_major))" -eq 255 ] && [ "$((val_minor))" -eq 255 ]; then
        echo "${partition^^}: NOT PRESENT"
    else
        echo "${partition^^}: $((val_major)).$((val_minor))"
    fi
done
