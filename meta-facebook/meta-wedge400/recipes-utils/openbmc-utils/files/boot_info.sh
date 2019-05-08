#!/bin/sh
#
# Copyright 2019-present Facebook. All Rights Reserved.
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
. /usr/local/bin/openbmc-utils.sh

PATH=/sbin:/bin:/usr/sbin:/usr/bin:/usr/local/bin

usage() {
    program=`basename "$0"`
    echo "Usage:"
    echo "$program <bmc|bios> reset <master|slave>"
}

bmc_boot_info() {
    # get watch dog1 timeout status register
    wdt1=$(devmem 0x1e785010)

    # get watch dog2 timeout status register
    wdt2=$(devmem 0x1e785030)

    let "wdt1_timeout_cnt = ((wdt1 & 0xff00) >> 8)"
    let "wdt2_timeout_cnt = ((wdt2 & 0xff00) >> 8)"
    let "boot_code_source = ((wdt2 & 0x2) >> 1)"

    boot_source='Master Flash'
    if [ $boot_code_source -eq 1 ]
    then
      boot_source='Slave Flash'
    fi

    echo "WDT1 Timeout Count: " $wdt1_timeout_cnt
    echo "WDT2 Timeout Count: " $wdt2_timeout_cnt
    echo "Current Boot Code Source: " $boot_source
}

bmc_boot_from() {
    boot_source=0x00000013
    wdt2=$(devmem 0x1e785030)
    ((boot_code_source = (wdt2 & 0x2) >> 1))
    if [ "$1" = "master" ]; then
        if [ $boot_code_source = 0 ]; then
            echo "Current boot source is master, no need to switch."
            return 0
        fi
        boot_source=0x00000093
    elif [ "$1" = "slave" ]; then
        if [ $boot_code_source = 1 ]; then
            echo "Current boot source is slave, no need to switch."
            return 0
        fi
        boot_source=0x00000093
    fi
    devmem 0x1e785024 32 0x00989680
    devmem 0x1e785028 32 0x4755
    devmem 0x1e78502c 32 $boot_source
}

bios_boot_info() {
    echo "Not Support"
}

bios_boot_from() {
    echo "Not Support"
}

if [ $# -lt 1 ]; then
    usage
    exit -1
fi

case $1 in
    "bmc")
        if [ $# -eq 1 ]; then
            bmc_boot_info
        else
            if [ $2 == "reset" ] && [ $# -ge 3 ]; then
                case $3 in
                    "master" | "slave")
                        bmc_boot_from $3
                    ;;
                    *)
                        usage
                        exit -1
                    ;;
                esac
            fi
        fi
    ;;
    "bios")
        if [ $# -eq 1 ]; then
            bios_boot_info
        else
            if [ $2 == "reset" ] && [ $# -ge 3 ]; then
                case $3 in
                    "master" | "slave")
                        bios_boot_from $3
                    ;;
                    *)
                        usage
                        exit -1
                    ;;
                esac
            fi
        fi
    ;;
    *)
        usage
        exit -1
    ;;
esac
