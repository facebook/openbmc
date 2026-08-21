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

prog="$0"
CPLD_TYPE="$1"
UPDATE_IMG="$2"

DLL_PATH=/usr/lib/libcpldupdate_dll_gpio.so

export_gpio()
{
    setup_gpio MUX_JTAG_SEL_1 GPIOF5 out 0 > /dev/null 2>&1

    setup_gpio BMC_TDI GPIOI1 out 0 > /dev/null 2>&1
    setup_gpio BMC_TCK GPIOI2 out 0 > /dev/null 2>&1
    setup_gpio BMC_TMS GPIOI3 out 0 > /dev/null 2>&1
    setup_gpio BMC_TDO GPIOI4 in > /dev/null 2>&1

    sleep 1
}

unexport_gpio()
{
    gpio_unexport MUX_JTAG_SEL_1 > /dev/null 2>&1

    gpio_unexport BMC_TDI > /dev/null 2>&1
    gpio_unexport BMC_TCK > /dev/null 2>&1
    gpio_unexport BMC_TMS > /dev/null 2>&1
    gpio_unexport BMC_TDO > /dev/null 2>&1
}

usage() {
    echo "Usage: $prog <CPLD_TYPE> <img_file>"
    echo
    echo "CPLD_TYPE: ( COME )"
    echo "  COME: COMe CPLD"
    echo
    echo "img_file: Image file for lattice CPLD"
    echo "  VME file for software mode"
    echo
    echo
}

if [ $# -lt 2 ]; then
    usage
    exit 1
fi

#
#  Multiplexer Diagram
#                                                 MUX_JTAG_SEL_0
#   [ HEADER ] ───────────────────────────────────── │ 0    │
#                                                    │    C │ ── [ SCM CPLD ]
#             │    0 │ ───────────────────────────── │ 1    │
#   [ BMC ] ─ │ C    │
#             │    1 │ ───────────────────────────────────────── [ COMe CPLD ]
#          MUX_JTAG_SEL_1
#
# Note: support only COMe CPLD
#

enable_come_jtag_chain() {
    gpio_set_value MUX_JTAG_SEL_1 1
}

disable_jtag_chain() {
    gpio_set_value MUX_JTAG_SEL_1 0
}

cpld_update() {
    netlake_identify > /dev/null 2>&1
    netlake_type=$?

    if [ "$CPLD_TYPE" = "COME" ]; then
        if [ "$netlake_type" -eq 0 ]; then
            echo 'This is Netlake 1.0, skip update'
            exit 1
        elif [ "$netlake_type" -eq 1 ]; then
            echo 'This is Netlake 2.0, starting to update'
            ispvm dll "$DLL_PATH" "$UPDATE_IMG" \
                  --tms BMC_TMS --tdo BMC_TDO --tdi BMC_TDI --tck BMC_TCK
        else
            echo 'failed to identify COMe type: Netlake 1.0 or 2.0'
            exit 1
        fi
    else
        echo "CPLD_TYPE is wrong"
        exit 1
    fi

    result=$?
    if [ "$result" -eq 1 ]; then
        echo "Upgrade successful."
        exit 0
    fi
}

trap 'rm -rf /tmp/cpld_update && disable_jtag_chain && unexport_gpio' INT TERM QUIT EXIT

echo 1 > /tmp/cpld_update

export_gpio

case "$CPLD_TYPE" in
    COME)
        enable_come_jtag_chain
        ;;
    *)
        echo "argument $CPLD_TYPE is wrong"
        usage
        exit 1
        ;;
esac

cpld_update
