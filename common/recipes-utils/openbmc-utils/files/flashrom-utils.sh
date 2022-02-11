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
# The file contains helper functions to parse flashrom output.
#
# Sample flashrom command output:
#
#   root@bmc:~# flashrom -p linux_spi:dev=/dev/spidev1.0
#   flashrom v1.0 on Linux 5.10.5-minipack (armv6l)
#   flashrom is free software, get the source code at https://flashrom.org
#
#   Using clock_gettime for delay loops (clk_id: 1, resolution: 1ns).
#   Found Winbond flash chip "W25Q128.V" (16384 kB, SPI) on linux_spi.
#   No operations were specified.
#

#
# Generate spidev device name in <spidev#.#> format.
#
# $1 - spi bus number
# $2 - spi device address
#
flash_device_name() {
    echo "spidev${1}.${2}"
}

#
# Wrapper function to launch flashrom command.
#
# $1 - spi device name in "spidev#.#" format. For example, "spidev1.0".
# $@ - extra options passed to flashrom command
#
flashrom_cmd() {
    spi_dev="$1"
    shift

    flashrom -p linux_spi:dev=/dev/"$spi_dev" $@
}

#
# Dump flash summary by running flashrom.
#
# $1 - spi device name in "spidev#.#" format. For example, "spidev1.0".
#
flash_dump_summary() {
    flashrom_cmd "$1"
}

#
# Extract flash information from flashrom output.
#
# $1 - spi device name in "spidev#.#" format. For example, "spidev1.0".
#
flash_extract_info() {
    summary=$(flash_dump_summary "$1")
    info=$(echo "$summary" | grep -E "Found.*flash.*linux_spi")
    if [ -z "$info" ]; then
        echo "Unable to extract flash info! flashrom output:"
        echo "$summary"
        return 1
    fi

    echo "$info"
}

#
# Read flash vendor from flashrom output.
#
# $1 - spi device name in "spidev#.#" format. For example, "spidev1.0".
#
flash_get_vendor() {
    if ! info=$(flash_extract_info "$1"); then
        return 1
    fi

    vendor=$(echo "$info" | cut -d ' ' -f 2)
    if [ -z "$vendor" ]; then
        echo "Unable to determine flash vendor! flashrom output:"
        flash_dump_summary "$1"
        return 1
    fi

    echo "$vendor"
}

#
# Read flash model from flashrom output.
#
# $1 - spi device name in "spidev#.#" format. For example, "spidev1.0".
#
flash_get_model() {
    if ! info=$(flash_extract_info "$1"); then
        return 1
    fi

    model=$(echo "$info" | cut -d '"' -f 2)
    if [ -z "$model" ]; then
        echo "Unable to determine flash model! flashrom output:"
        flash_dump_summary "$1"
        return 1
    fi

    echo "$model"
}

#
# Helper function to check if given input is a decimal number.
#
# $1 - decimal number.
#
is_decimal() {
    echo "$size" | grep -E -q '^[0-9]+$' > /dev/null 2>&1
}

#
# Read flash size from flashrom output (unit: kb).
#
# $1 - spi device name in "spidev#.#" format. For example, "spidev1.0".
#
flash_get_size() {
    if ! info=$(flash_extract_info "$1"); then
        return 1
    fi

    size=$(echo "$info" | cut -d '(' -f 2 | cut -d ' ' -f 1)
    if ! is_decimal "$size"; then
        echo "Unable to determine flash size! flashrom output:"
        flash_dump_summary "$1"
        return 1
    fi

    echo "$size"
}

#
# Read the flash ($1) and store content in the given file ($2).
#
# $1 - spi device name in "spidev#.#" format. For example, "spidev1.0".
# $2 - file name/path to store the flash content.
# $3 - flash model, optional.
#
flash_read() {
    spi_dev="$1"
    image_file="$2"

    if [ -n "$3" ]; then
        model="$3"
    else
        if ! model=$(flash_get_model "$spi_dev"); then
            return 1
        fi
    fi

    flashrom_cmd "$spi_dev" -r "$image_file" -c "$model"
}

#
# Write the file ($2) to the given flash ($1).
#
# $1 - spi device name in "spidev#.#" format. For example, "spidev1.0".
# $2 - file name/path containing the flash image.
# $3 - flash model, optional.
#
flash_write() {
    spi_dev="$1"
    image_file="$2"

    if [ ! -e "$image_file" ]; then
        echo "Error: image file $image_file does not exist!"
        return 1
    fi

    if [ -n "$3" ]; then
        model="$3"
    else
        if ! model=$(flash_get_model "$spi_dev"); then
            return 1
        fi
    fi

    flashrom_cmd "$spi_dev" -w "$image_file" -c "$model"
}

#
# Erase the given flash ($1).
#
# $1 - spi device name in "spidev#.#" format. For example, "spidev1.0".
# $3 - flash model, optional.
#
flash_erase() {
    spi_dev="$1"

    if [ -n "$2" ]; then
        model="$2"
    else
        if ! model=$(flash_get_model "$spi_dev"); then
            return 1
        fi
    fi

    flashrom_cmd "$spi_dev" -E -c "$model"
}
