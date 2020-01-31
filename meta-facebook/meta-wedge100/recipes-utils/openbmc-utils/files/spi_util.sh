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

trap cleanup_spi INT TERM QUIT EXIT

# Global
RET=0
INFO_OUTPUT=""
PIDFILE='/var/run/spi_util.pid'

check_duplicate_process() {
    local pid
    exec 2>"$PIDFILE"
    flock -n 2 || (echo "Another process is running" && exit 1)
    RET="$?"
    if [ "$RET" -eq 1 ]; then
        exit 1
    fi
    pid="$$"
    echo "$pid" 1>&2
}

remove_pid_file() {
    if [ -f "$PIDFILE" ]; then
        rm "$PIDFILE"
    fi
}

check_flash_info() {
    local spi_no="$1"
    if ! INFO_OUTPUT=$(flashrom -p linux_spi:dev=/dev/spidev"${spi_no}".0); then
        echo "Check flash info error: [$INFO_OUTPUT]"
        exit 1
    fi
}

get_flash_first_type() {
    local type
    if [ -n "$INFO_OUTPUT" ]; then
        type=$(echo "$INFO_OUTPUT" \
            | sed ':a;N;$!ba;s/\n/ /g' \
            | cut -d'"' -f2)
        if [ -n "$type" ]; then
            echo "$type"
            return 0
        fi
    fi
    return 1
}

get_flash_size() {
    local spi_no="$1"
    local flash_sz
    if [ -n "$INFO_OUTPUT" ]; then
        flash_sz=$(echo "$INFO_OUTPUT" \
            | sed ':a;N;$!ba;s/\n/ /g' \
            | cut -d'(' -f3 \
            | cut -d' ' -f1)
        if grep -E -q '^[0-9]+$' <<< "$flash_sz"; then
            echo "$flash_sz"
            return 0
        fi
    fi
    echo "Get flash size error: [$INFO_OUTPUT]"
    return 1
}

# $1: input file size $2: flash size $3: output file path
pad_ff() {
    local out_file="$3"
    local pad_size=$(($2 - $1))
    if dd if=/dev/zero bs="$pad_size" count=1 | tr "\000" "\377" >> "$out_file"; then
        return 0
    else
        echo "Resize error ret $?"
        exit 1
    fi
}

resize_file() {
    local in_file="$1"
    local out_file="$2"
    local spi_no="$3"
    local in_file_sz
    local storage_sz
    local flash_sz

    in_file_sz=$(stat -c%s "$in_file")
    if flash_sz=$(get_flash_size "$spi_no"); then
        storage_sz=$((flash_sz * 1024))
    else
        echo "debug message: $flash_sz"
        exit 1
    fi

    if [ "$in_file_sz" -ne "$storage_sz" ]; then
        cp "$in_file" "$out_file"
        pad_ff "$in_file_sz" "$storage_sz" "$out_file"
    else
        ln -s "$(realpath "$in_file")" "$out_file"
    fi
}

config_bios_spi() {
  devmem_set_bit "$(scu_addr 70)" 12
  gpio_set COM6_BUF_EN 0
  gpio_set COM_SPI_SEL 1
}

read_flash_to_file() {
    local spi_no="$1"
    local tmp_file="$2"
    local type
    local cmd
    check_flash_info "$spi_no"
    type=$(get_flash_first_type)
    cmd="flashrom -p linux_spi:dev=/dev/spidev${spi_no}.0 -r ${tmp_file} -c ${type}"
    if ! $cmd; then
        echo "debug cmd: [${cmd}]"
        exit 1
    fi
}

write_flash_to_file() {
    local spi_no="$1"
    local in_file="$2"
    local out_file="/tmp/${3}_spi${1}_tmp"
    local type
    local cmd
    check_flash_info "$spi_no"
    resize_file "$in_file" "$out_file" "$spi_no"
    type=$(get_flash_first_type)
    cmd="flashrom -p linux_spi:dev=/dev/spidev${spi_no}.0 -w ${out_file} -c ${type}"
    if ! $cmd; then
        echo "debug cmd: [${cmd}]"
        exit 1
    fi
}

erase_flash() {
    local spi_no="$1"
    local type
    local cmd
    check_flash_info "$spi_no"
    type=$(get_flash_first_type)
    cmd="flashrom -p linux_spi:dev=/dev/spidev${spi_no}.0 -E -c ${type}"
    if ! $cmd; then
        echo "debug cmd: [${cmd}]"
        exit 1
    fi
}

detect_flash() {
    local spi_no="$1"
    check_flash_info "$spi_no"
    echo "$INFO_OUTPUT"
}

read_spi1_dev() {
    local spi_no=1
    local dev="$1"
    local file="$2"
    case "$dev" in
        "BACKUP_BIOS")
            echo "Reading flash to $file..."
            read_flash_to_file "$spi_no" "$file"
        ;;
    esac
}

write_spi1_dev() {
    local spi_no=1
    local dev="$1"
    local file="$2"
    case "$dev" in
        "BACKUP_BIOS")
            echo "Writing $file to flash..."
            write_flash_to_file "$spi_no" "$file" "$dev"
        ;;
    esac
}

erase_spi1_dev() {
    local spi_no=1
    local dev="$1"
    local file="$2"
    case "$dev" in
        "BACKUP_BIOS")
            echo "Erasing flash..."
            erase_flash "$spi_no"
        ;;
    esac
}

detect_spi1_dev() {
    local spi_no=1
    local dev="$1"
    local file="$2"
    case "$dev" in
        "BACKUP_BIOS")
            echo "Detecting flash..."
            detect_flash "$spi_no"
        ;;
    esac
}

config_spi1_pin_and_path() {
    local dev="$1"
    case "$dev" in
        "BACKUP_BIOS")
            devmem_set_bit "$(scu_addr 70)" 12
            gpio_set COM6_BUF_EN 0
            gpio_set COM_SPI_SEL 1
        ;;
        *)
            echo "Please enter {BACKUP_BIOS}"
            exit 1
        ;;
    esac
    echo "Config SPI1 Done."
}

operate_spi1_dev() {
    local op="$1"
    local dev="$2"
    local file="$3"
    ## Operate devices ##
    case "$op" in
        "read")
                read_spi1_dev "$dev" "$file"
        ;;
        "write")
                write_spi1_dev "$dev" "$file"
        ;;
        "erase")
                erase_spi1_dev "$dev"
        ;;
        "detect")
                detect_spi1_dev "$dev"
        ;;
        *)
            echo "Operation $op is not defined."
        ;;
    esac
}

cleanup_spi() {
    if [ "$RET" -eq 1 ]; then
      exit 1
    fi
    devmem_clear_bit "$(scu_addr 70)" 12
    rm -rf /tmp/*_spi*_tmp
    remove_pid_file
}

ui() {
    local op="$1"
    local spi="$2"
    local dev="$3"
    local file="$4"

	## Open the path to device ##
    case "$spi" in
        "spi1")
            config_spi1_pin_and_path "$dev"
            operate_spi1_dev "$op" "$dev" "$file"
        ;;
        *)
            echo "No such SPI bus."
            return 1
        ;;
    esac
}

usage() {
    local prog
    prog=$(basename "$0")
    echo "Usage:"
    echo "$prog <op> spi1 <spi1 device> <file>"
    echo "  <op>          : read, write, erase, detect"
    echo "  <spi1 device> : BACKUP_BIOS"
    echo "Examples:"
    echo "  $prog write spi1 BACKUP_BIOS bios.bin"
    echo ""
}

check_parameter() {
    local op="$1"
    local spi="$2"
    local dev="$3"
    local file="$4"

    case "$op" in
        "read" | "write" | "erase" | "detect")
            ;;
        *)
            return 1
            ;;
    esac
    if [ -n "$spi" ] && [ "$spi" = "spi1" ]; then
        ## Check device
        case "$dev" in
            "BACKUP_BIOS")
                ## Check operation
                case "$op" in
                    "write")
                        if [ $# -ne 4 ] || [ ! -f "$file" ]; then
                            echo "File ${file} is not exists."
                            return 1
                        fi
                        ;;
                    "read")
                        if [ $# -ne 4 ]; then
                            return 1
                        fi
                        ;;
                    "erase" | "detect")
                        if [ $# -ne 3 ]; then
                            return 1
                        fi
                        ;;
                esac
                ;;
            *)
                return 1
                ;;
        esac
    else
        return 1
    fi

    return 0
}

check_duplicate_process
if check_parameter "$@"; then
    ui "$@"
else
    usage
    exit 1
fi

