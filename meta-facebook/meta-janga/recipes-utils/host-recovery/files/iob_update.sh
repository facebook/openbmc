#!/bin/sh
#
# Copyright (c) Meta Platforms, Inc. and affiliates. (http://www.meta.com)
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
# shellcheck source=/dev/null
. /usr/local/bin/openbmc-utils.sh
. /usr/local/bin/spi-utils.sh
trap cleanup_spi INT TERM QUIT EXIT

ui(){
    op=$1
    file=$2

    export_gpio
    config_spi1_pin_and_path "IOB_FPGA"
    operate_spi1_dev "$op" "IOB_FPGA" "$file"
}

usage(){
    program=$(basename "$0")
    echo "Usage:"
    echo "  $program <op> <file>"
    echo "    <op>          : read, write, erase"
    echo ""
    echo "Examples:"
    echo "  $program write iobfpga.bit"
    echo ""
}

check_parameter(){
    program=$(basename "$0")
    op=$1
    file=$2

    # check operations.
    case ${op} in
        "read" | "write" | "erase")
            ;;
        *)
            return 1
            ;;
    esac

    return 0
}


# check_duplicate_process - to check if there is the duplicate spi util process, will return fail and stop execute command.
check_duplicate_process

# check_parameter - to check command parameters
check_parameter "$@"
is_par_ok=$?

if [ $is_par_ok -eq 0 ]; then
    ui "$@"
    exit 0
else
    usage
    exit 1
fi
