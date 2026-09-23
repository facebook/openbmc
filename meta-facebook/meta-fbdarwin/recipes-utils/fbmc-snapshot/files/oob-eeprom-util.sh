#!/bin/bash
#
# Copyright 2022-present Facebook. All Rights Reserved.
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

trap cleanup EXIT
trap handle_signal INT TERM QUIT

OOB_EEPROM_SPI_PATH="/sys/bus/spi/devices/spi1.1/spi1.10/nvmem"

usage() {
    echo "Read or write to the BCM53134 OOB EEPROM"
    echo
    echo "read <file>"
    echo "write <file>"
    echo "version"
}

grab_uWire() {
    gpio_set_value OOB_EEPROM_SPI_SEL 1
}

ungrab_uWire() {
    gpio_set_value OOB_EEPROM_SPI_SEL 0
}

cleanup() {
    ungrab_uWire
}

handle_signal() {
    echo "Exiting because of signal" >&2
    cleanup
    exit 1
}

eeprom_read() {
    file="$1"
    file_temp="$file"_temp
    echo "Reading spi1.1 BCM53134P EEPROM to $file..."
    # Add retry to BCM53134_EE read in case it fails
    for n in 1 2 3 4 5
    do
        echo "Reading $file $n times"
        dd if="$OOB_EEPROM_SPI_PATH" of="$file"
        dd if="$OOB_EEPROM_SPI_PATH" of="$file_temp"
        if ! diff "$file" "$file_temp" >/dev/null; then
            echo "Readback file differ on attempt $n"
            # read-back file differ
            rm -rf "$file"*
        else
            # read-back file same
            rm -rf "$file_temp";
            return;
        fi
    done
    logger -p user.err "Unable to read the correct $file after 5 retries"
    exit 1
}

eeprom_write() {
    file=$1
    echo "Verifying target binary to program..."
    # We expect 0x01FF to be read in a certain offset of the binary
    # If we get 0xFF01, the endianness of the binary is likely incorrect
    # If we get neither, the binary is not recognized
    magic_bytes="$(dd if="$file" bs=1 skip=2 count=2 2>/dev/null | hexdump -n2)"
    if echo "$magic_bytes" | grep -q "01ff"; then
        echo "Binary endianness is big endian"
    elif echo "$magic_bytes" | grep -q "ff01"; then
        echo "Binary endianness is little endian"
        echo "Are you sure the binary is in the correct format?"
        exit 1
    else
        echo "Unable to match magic byte in binary"
        echo "Are you sure the binary is correct?"
        exit 1
    fi
    echo "Writing $file to spi1.1 BCM53134P EEPROM..."
    dd if="$file" of="$OOB_EEPROM_SPI_PATH"
}

eeprom_version() {
    local version
    version_file="/tmp/version.bin"
    eeprom_read $version_file >/dev/null 2>&1
    # Failure of eemprom_read terminates the process;
    # don't need to check return value
    version="0x$(hexdump $version_file | awk '{$1=""}1' | tr -d "[:space:]" | tail -c 4)"
    printf "Version: %d.%d\n" $(( version & 0xff )) $(( version >> 8 ))
    rm -rf $version_file
}

if [ $# -lt 1 ]
then
    echo "Error: missing command" >&2
    usage
    exit 1
fi

# Only allow one instance of script to run at a time.
script=$(realpath "$0")
exec 100< "$script"
flock -n 100 || { echo "ERROR: $0 already running" && exit 1; }

command="$1"
shift

grab_uWire
case "$command" in
    read)
        eeprom_read "$@"
        ;;
    write)
        eeprom_write "$@"
        ;;
    version)
        eeprom_version
        ;;
    *)
        echo "Error: invalid command: $command" >&2
        usage
        exit 1
        ;;
esac
