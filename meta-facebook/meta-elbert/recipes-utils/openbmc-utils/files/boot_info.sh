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
source /usr/local/bin/openbmc-utils.sh

usage() {
    program=$(basename "$0")
    echo "Usage:"
    echo "  $program <bmc> reset <primary|secondary>"
    echo ""
    echo "Examples:"
    echo "  $program bmc"
    echo "  $program bmc reset primary"
}

FMC_WDT2_CONTROL_REG="0x1E620064"
FMC_WDT2_COUNTER_REG="0x1E620068"
FMC_WDT2_RELOAD_REG="0x1E62006C"

bmc_boot_info() {
    # Please refer to reg FMC WDT1/WDT2 Control Register definition to
    # understand this code block, WDT2 is on page 232 of ast2620.pdf

    # get watch dog2 timeout status register
    wdt2=$(devmem "$FMC_WDT2_CONTROL_REG")

    wdt2_timeout_cnt=$(( ((wdt2 & 0xff00) >> 8) ))
    boot_code_source=$(( ((wdt2 & 0x10) >> 4) ))

    boot_source="Primary Flash"
    if [ $boot_code_source -eq 1 ]
    then
      boot_source="Secondary Flash"
    fi

    echo "WDT2 Timeout Count: " $wdt2_timeout_cnt
    echo "Current Boot Code Source: $boot_source"
}

bmc_boot_from() {
    # Please refer to reg WDT2 Control Register definition to
    # understand this code block, WDT2 is on page 232 of ast2620.pdf
    # Enable watchdog reset_system_after_timeout bit and WDT_enable_signal bit.
    # Refer to ast2620.pdf page 232.
    boot_source=0x00000000
    wdt2=$(devmem "$FMC_WDT2_CONTROL_REG")
    boot_code_source=$(( (wdt2 & 0x10) >> 4 ))

    if [ "$1" = "primary" ]; then
        if [ $boot_code_source = 0 ]; then
            echo "Current boot source is primary, no need to switch."
            return 0
        else
            # Bit 0 - Enable Watchdog
            # Bit 4 - BOOT_SOURCE_PRIMARY_N
            boot_source=0x00000001
        fi
    elif [ "$1" = "secondary" ]; then
        if [ $boot_code_source = 1 ]; then
            echo "Current boot source is secondary, no need to switch."
            return 0
        else
            # Bit 0 - Enable Watchdog
            # Bit 4 - BOOT_SOURCE_PRIMARY_N
            boot_source=0x00000011
        fi
    fi

    echo "BMC will switch to $1 after 2 seconds    ..."
    # Set WDT time out 2s, 0x00000014 = 2s
    devmem "$FMC_WDT2_COUNTER_REG" 32 0x00000014
    # WDT magic number to restart WDT counter to decrease.
    devmem "$FMC_WDT2_RELOAD_REG" 32 0x4755
    devmem "$FMC_WDT2_CONTROL_REG" 32 $boot_source
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
                    "primary" | "secondary")
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
