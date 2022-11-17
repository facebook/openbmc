#!/bin/bash
#
# Copyright 2021-present Facebook. All Rights Reserved.
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
# shellcheck disable=SC2059
. /usr/local/bin/openbmc-utils.sh


# This utility displays the TH4 QSPI version in the QSPI flash

echo "------QSPI-FLASH-CONFIG------"
QSPI_REVISION_FILE="/tmp/.qspi_spi_image"

partition="header_th4_qspi"
if [ ! -f "$QSPI_REVISION_FILE" ]; then
    # version is not cached, force read it
    /usr/local/bin/fpga_util.sh "$partition" read \
                                "$QSPI_REVISION_FILE" > /dev/null 2>&1
    if [ ! -f "$QSPI_REVISION_FILE" ]; then
        echo "Failed to read $partition"
        exit 1
    fi
fi

HEADER_OFFSET=0x2000
HEADER_LEN=32
LOADER_VERSION_OFFSET=0x4
VERSION_OFFSET=0x10
LOADER_BUILDDATE_OFFSET=0x1c
VERSION_LEN=12

hexbytes=()
while IFS='' read -r line; do hexbytes+=("$line"); done <<< "$(
   hexdump -v -e '/1 "0x%x\n"' -s "$HEADER_OFFSET" -n "$HEADER_LEN" \
      "$QSPI_REVISION_FILE"
)"

fwver=""
foundnull=0
for h in "${hexbytes[@]:$VERSION_OFFSET:$VERSION_LEN}"
do
    if (( h == 0 ))
    then
        foundnull=1
        break
    fi

    if (( h < 0x20 || h > 0x7e ))
    then
        echo "Error: non-printable characters detected"
        fwver=""
        break
    fi

    char=$(printf "\x""${h#0x}")
    fwver="$fwver$char"
done

if (( foundnull == 0 ))
then
    fwver=""
fi

if [[ -z "$fwver" ]]
then
    echo "Error: failed to parse th4_qspi version"
    exit 1
fi

# Decode the firmware loader version
LOADER_VERSION_OFFSET=0x4
mjhigh=${hexbytes[$((LOADER_VERSION_OFFSET + 3 ))]}
mjlow=${hexbytes[$((LOADER_VERSION_OFFSET + 2 ))]}
major=$(( (mjhigh << 8) | mjlow ))
mnhigh=${hexbytes[$((LOADER_VERSION_OFFSET + 1 ))]}
mnlow=${hexbytes[$LOADER_VERSION_OFFSET]}
minor=$(( (mnhigh << 8) | mnlow ))

printf "FIRMWARE_LOADER_VERSION: %d.%d\n" $major $minor
echo "FIRMWARE_VERSION: $fwver"

# Print the firmware loader build date
echo -n "FIRMWARE_LOADER_BUILD_DATE: "
for i in 3 2 1 0
do
    printf "%02d" "${hexbytes[$((LOADER_BUILDDATE_OFFSET + i))]#0x}"
done
echo
