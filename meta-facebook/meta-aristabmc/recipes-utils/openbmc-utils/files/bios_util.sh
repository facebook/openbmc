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

# shellcheck disable=SC1091
. /usr/local/bin/openbmc-utils.sh

# Check if another fw upgrade is ongoing
check_fwupgrade_running

trap cleanup INT TERM QUIT EXIT

SPI_CHANNEL="1.0"
# Used by spi_mtd_for_channel() from the sourced board-utils.sh.
# shellcheck disable=SC2034
SPI_MTD_LABEL="cpu-boot"
DRIVER=""
BIOS_UTIL_NAME="$0"

usage() {
    program=$(basename "$BIOS_UTIL_NAME")
    echo "Usage:"
    echo "$program <action> [<bios file>] [--partition <partition 0> ... <partition N>]"
    echo "      <action> : read, write, verify, erase, fullread, fullwrite"
    echo "      [<bios file>] : file to read to/write from"
    echo "          Required for all actions except erase"
    echo "      [<partition>] : partition(s) of layout file; defaults to image"
    echo "          If <partition> is given, <action> must be"
    echo "          read, write, program, verify, or erase"
}

cleanup() {
    local rc=0

    [ "$HARDWARE_CONNECTED" -eq 1 ] || return 0
    disconnect_program_paths || rc=1
    HARDWARE_CONNECTED=0
    return "$rc"
}

ORIG_POWER_STATE=$(userver_power_is_on; echo $?)
HARDWARE_CONNECTED=0

disconnect_program_paths() {
    local rc=0

    unbind_spi_nor_driver "$SPI_CHANNEL" || rc=1
    gpio_set_value ABOOT_GRAB 0 || rc=1
    gpio_set_value BMC_SPI_2_CPLD 0 || rc=1

    if [ "$ORIG_POWER_STATE" -eq 0 ]; then
        userver_power_on || rc=1
    fi
    return "$rc"
}

connect_spi() {
    if [ "$ORIG_POWER_STATE" -eq 0 ]; then
        userver_power_off || return 1
    fi

    gpio_set_value ABOOT_GRAB 1
    gpio_set_value SW_CPLD_JTAG_SEL 0
    gpio_set_value BMC_SPI_2_CPLD 0
    sleep 1
}

select_spi_driver() {
    # SPI1 uses the AST2700 NOR controller, matching the working Arista2700
    # topology.  The external mux is selected before this point; make sure the
    # CPU flash child is bound and use its MTD device for the operation.
    bind_spi_nor_driver "$SPI_CHANNEL" || return 1
    DRIVER="SPINOR"
}

narg_err() {
    echo "Invalid number of arguments"
    usage
    exit 1
}

BIOS_PARTITIONS=("image")

bios_action="$1"
case "${bios_action^^}" in
    FULLREAD|FULLWRITE)
        if [ $# -ne 2 ]; then
            narg_err
        fi
        partitions=("total")
        target_image="$2"
        ;;
    READ|VERIFY|WRITE|PROGRAM)
        if [ $# -eq 2 ]; then
            partitions=("${BIOS_PARTITIONS[@]}")
            target_image="$2"
        elif [ $# -ge 4 ] && [ "$3" == "--partition" ]; then
            target_image="$2"
            shift 3
            partitions=("$@")
        else
            narg_err
        fi
        ;;
    ERASE)
        if [ $# -eq 1 ]; then
            partitions=("${BIOS_PARTITIONS[@]}")
        elif [ $# -ge 3 ] && [ "$2" == "--partition" ]; then
            shift 2
            partitions=("$@")
        else
            narg_err
        fi
        target_image=""
        ;;
    *)
        echo "Unknown action: $1"
        usage
        exit 1
        ;;
esac

HARDWARE_CONNECTED=1
connect_spi || exit 1
select_spi_driver || exit 1
do_spi_image "$target_image" "$DRIVER" "$SPI_CHANNEL" BIOS "$bios_action" "${partitions[@]}"
