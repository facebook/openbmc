#!/bin/bash
#
# Copyright 2020-present Facebook. All rights reserved.
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
. /usr/local/bin/flashrom-utils.sh

PID_FILE='/var/run/spi_util.pid'

#
# Modules used to talk to backup BIOS
#
SPI_ASPEED_MODULE=spi_aspeed

if uname -r | grep "4\.1\.*" > /dev/null 2>&1; then
    LEGACY_KERNEL="true"
fi

check_duplicate_process() {
    exec 99>"$PID_FILE"
    if ! flock -n 99; then
        echo "Another instance is running. Exiting!!"
        exit 1
    fi
}

#
# Helper function to generate pathname of a temporary file.
# Please always use this function to generate temp files so they can be
# removed at cleanup.
#
# $1 - reference file name.
#
spi_tmp_file() {
    name=$(basename "$1")
    echo "/tmp/.spitmp_$name"
}

#
# Helper function to append more bytes to the given file.
#
# $1 - file to be updated.
# $2 - pad size
#
pad_file() {
    out_file="$1"
    pad_size="$2"
    if ! dd if=/dev/zero bs="$pad_size" count=1 | tr "\000" "\377" >> "$out_file"; then
        echo "Error: failed to pad $out_file (pad_size=$pad_size)!"
        exit 1
    fi
}

#
# Helper function to resize a file ($1) to target size ($3).
#
# $1 - input/reference file
# $2 - output file
# $3 - expected size of the output file.
#
resize_file() {
    in_file="$1"
    out_file="$2"
    target_size="$3"

    in_file_sz=$(stat -c%s "$in_file")
    if [ "$in_file_sz" -lt "$target_size" ]; then
        cp "$in_file" "$out_file"
        pad_size=$((target_size - in_file_sz))
        pad_file "$out_file" "$pad_size"
    elif [ "$in_file_sz" -eq "$target_size" ]; then
        ln -s "$(realpath "$in_file")" "$out_file"
    else
        echo "Input file too big: $in_file_sz > $target_size"
        exit 1
    fi
}

kmod_unload() {
    if lsmod | grep "$1" > /dev/null 2>&1; then
        modprobe -r "$1"
    fi
}

kmod_reload() {
    kmod_unload "$1"
    modprobe "$1"
}

#
############################################################
# Functions to deal with spi1.0: the backup BIOS flash.
############################################################
#

spi1_connect() {
    echo "enable spi1 (bios) connection.."

    # Enable SPI master in SCU register
    devmem_set_bit "$(scu_addr 70)" 12

    gpio_set_value COM6_BUF_EN 0

    if [ ! -L "${SHADOW_GPIO}/COM_SPI_SEL" ]; then
        gpio_export_by_name "$ASPEED_GPIO" GPIOO0 COM_SPI_SEL
    fi
    gpio_set_value COM_SPI_SEL 1

    if [ -z "$LEGACY_KERNEL" ]; then
        kmod_reload "$SPI_ASPEED_MODULE"
    fi
}

spi1_disconnect() {
    echo "disable spi1 (bios) connection.."
    if [ -L "${SHADOW_GPIO}/COM_SPI_SEL" ]; then
        gpio_set_value COM_SPI_SEL 0
        gpio_unexport COM_SPI_SEL
    fi

    kmod_unload "$SPI_ASPEED_MODULE"

    # Disable SPI interface
    devmem_clear_bit "$(scu_addr 70)" 12
}

spi1_flash_write() {
    flash_dev="$1"
    in_file="$2"
    flash_model="$3"

    out_file=$(spi_tmp_file "$in_file")
    rm -rf "$out_file"

    if ! flash_size=$(flash_get_size "$flash_dev"); then
        exit 1
    fi

    target_size=$((flash_size * 1024))
    resize_file "$in_file" "$out_file" "$target_size"

    flash_write "$flash_dev" "$out_file" "$flash_model"
}

spi1_flash_detect() {
    if ! flash_dump_summary "$1"; then
        exit 1
    fi
}

spi1_launch_io() {
    op="$1"
    flash_dev="$2"
    image_file="$3"

    if ! flash_model=$(flash_get_model "$flash_dev"); then
        exit 1
    fi

    case "$op" in
        "read")
            flash_read "$flash_dev" "$image_file" "$flash_model"
        ;;
        "write")
            spi1_flash_write "$flash_dev" "$image_file" "$flash_model"
        ;;
        "erase")
            flash_erase "$flash_dev" "$flash_model"
        ;;
        "detect")
            spi1_flash_detect "$flash_dev"
        ;;
        *)
            echo "Operation $op is not supported!"
            exit 1
        ;;
    esac
}

cleanup_spi() {
    spi1_disconnect

    rm -rf /tmp/.spitmp_*
}

ui() {
    op="$1"
    spi_bus="$2"
    file="$4"

    case "$spi_bus" in
        "spi1")
            spi1_connect

            flash_dev=$(flash_device_name 1 0)
            flash_path="/dev/$flash_dev"
            if [ ! -e "$flash_path" ]; then
                echo "Error: unable to find $flash_path!"
                exit 1
            fi

            spi1_launch_io "$op" "$flash_dev" "$file"
        ;;
        *)
            echo "Error: no such SPI bus ($spi_bus)!"
            exit 1
        ;;
    esac
}

usage() {
    local prog
    prog=$(basename "$0")
    echo "Usage:"
    echo "$prog <op> spi1 <spi device> <file>"
    echo "  <op>          : read, write, erase, detect"
    echo "  <spi1 device> : BACKUP_BIOS"
    echo "Examples:"
    echo "  $prog write spi1 BACKUP_BIOS bios.bin"
    echo ""
}

check_parameter() {
    op="$1"
    spi="$2"
    dev="$3"
    file="$4"

    case "$op" in
        "read")
            if [ $# -ne 4 ]; then
                echo "Error: <image-file> argument is required!"
                return 1
            fi
            ;;
        "write")
            if [ $# -ne 4 ] || [ ! -f "$file" ]; then
                echo "Error: unable to find image file ${file}!"
                return 1
            fi
            ;;
        "erase" | "detect")
            if [ $# -ne 3 ]; then
                echo "Error: please specific spi device!"
                return 1
            fi
            ;;
        *)
            return 1
            ;;
    esac

    case "$spi" in
        "spi1")
            if [ "$dev" != "BACKUP_BIOS" ]; then
                echo "Error: supported spi1 devices: BACKUP_BIOS!"
                return 1
            fi
            ;;
        *)
            return 1
            ;;
    esac

    return 0
}

check_duplicate_process

if ! check_parameter "$@"; then
    usage
    exit 1
fi

trap cleanup_spi INT TERM QUIT EXIT

ui "$@"
