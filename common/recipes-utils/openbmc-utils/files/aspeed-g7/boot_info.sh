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
#

# shellcheck disable=SC1091
source /usr/local/bin/openbmc-utils.sh

PATH=/sbin:/bin:/usr/sbin:/usr/bin:/usr/local/bin

usage() {
    program=$(basename "$0")
    echo "Usage:"
    echo "  $program bmc reset <master|slave>"
    echo ""
    echo "Examples:"
    echo "  $program bmc"
    echo "  $program bmc reset master"
}

WDTA_RELOAD_REG=0x14c37404
WDTA_RESTART_REG=0x14c37408
WDTA_CONTROL_REG=0x14c3740c
WDTA_TIMEOUT_STATUS_REG=0x14c37410
WDTA_CLEAR_TIMEOUT_REG=0x14c37414
WDTA_BOOT_SOURCE_STATUS_REG=0x14c3744c

# WDT0C Register. Check AST2750 A2 Datasheet, Chapter 62 "Watchdog Timer (WDT)"
# for details.
bmc_boot_info() {
    wdta=$(devmem "$WDTA_TIMEOUT_STATUS_REG")
    wdta_boot_source=$(devmem "$WDTA_BOOT_SOURCE_STATUS_REG")
    wdta_timeout_cnt=$(( (wdta & 0xff00) >> 8 ))
    boot_code_source=$(( (wdta_boot_source & 0x2) >> 1 ))
    boot_source="Master Flash"
    if [ $boot_code_source -eq 1 ]
    then
      boot_source="Slave Flash"
    fi

    echo "WDTA Timeout Count: $wdta_timeout_cnt"
    echo "Current Boot Code Source: $boot_source"
}

bmc_boot_from() {
    wdta_boot_source=$(devmem "$WDTA_BOOT_SOURCE_STATUS_REG")
    boot_code_source=$(((wdta_boot_source & 0x2) >> 1))
    if [ "$1" = "master" ]; then
        if [ $boot_code_source = 0 ]; then
            echo "Current boot source is master, no need to switch."
            return 0
        fi
    elif [ "$1" = "slave" ]; then
        if [ $boot_code_source = 1 ]; then
            echo "Current boot source is slave, no need to switch."
            return 0
        fi
    fi

    # ABR flips to the other flash after WDTA expires. Use a two-second
    # timeout; the WDTA counter runs at 1 MHz.

    echo "BMC will switch to $1 after 2 seconds    ..."
    devmem "$WDTA_RELOAD_REG" 32 0x1e8480 >/dev/null
    devmem "$WDTA_CLEAR_TIMEOUT_REG" 32 0x1 >/dev/null
    devmem "$WDTA_RESTART_REG" 32 0x4755 >/dev/null
    devmem "$WDTA_CONTROL_REG" 32 0x3 >/dev/null
}

if [ $# -lt 1 ]; then
    usage
    exit 1
fi

case $1 in
    "bmc")
        if [ $# -eq 1 ]; then
            bmc_boot_info
            exit 0
        else
            if [ "$2" = "reset" ] && [ $# -ge 3 ]; then
                case $3 in
                    "master" | "slave")
                        bmc_boot_from "$3"
                        exit 0
                    ;;
                    *)
                        usage
                        exit 1
                    ;;
                esac
            fi
        fi
    ;;
    *)
        usage
        exit 1
    ;;
esac
