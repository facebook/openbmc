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

PATH=/sbin:/bin:/usr/sbin:/usr/bin:/usr/local/bin

usage() {
    program=$(basename "$0")
    echo "Usage:"
    echo "  $program <bmc|bios> reset <master|slave>"
    echo ""
    echo "Examples:"
    echo "  $program bmc"
    echo "  $program bmc reset master"
}

WDT1_STATUS_REG="0x1E785010"
FMC_WDT2_RELOAD_VAL_REG="0x1E620068"
FMC_WDT2_RESTART_REG="0x1E62006C"
FMC_WDT2_CTRL_STATUS_REG="0x1E620064"


bmc_boot_info() {
    # Please refer to reg WDT1/WDT2 Control Register definition to
    # understand this code block, WDT1 is on page 758 of ast2620.pdf
    # and WDT2 is on page 232 of ast2620.pdf
    # get watch dog1 timeout status register
    wdt1=$(devmem "$WDT1_STATUS_REG")

    # get watch dog2 timeout status register
    wdt2=$(devmem "$FMC_WDT2_CTRL_STATUS_REG" )

    wdt1_timeout_cnt=$(( ((wdt1 & 0xff00) >> 8) ))
    wdt2_timeout_cnt=$(( ((wdt2 & 0xff00) >> 8) ))
    boot_code_source=$(( ((wdt2 & 0x10) >> 4) ))

    boot_source="Master Flash"
    if [ $boot_code_source -eq 1 ]
    then
      boot_source="Slave Flash"
    fi

    echo "WDT1 Timeout Count: " $wdt1_timeout_cnt
    echo "WDT2 Timeout Count: " $wdt2_timeout_cnt
    echo "Current Boot Code Source: $boot_source"
}

bmc_boot_from() {
    # Please refer to reg WDT2 Control Register definition to
    # understand this code block, WDT2 is on page 232 of ast2620.pdf
    # Enable watchdog reset_system_after_timeout bit and WDT_enable_signal bit.
    # Refer to ast2620.pdf page 232.
    boot_source=0x00000000
    wdt2=$(devmem "$FMC_WDT2_CTRL_STATUS_REG")
    boot_code_source=$(( (wdt2 & 0x10) >> 4 ))
    if [ "$1" = "master" ]; then
        if [ $boot_code_source = 0 ]; then
            echo "Current boot source is master, no need to switch."
            return 0
        fi
        # No matter BMC boot from any one of master and slave.
        # Set bit_7 to 1 : Use second boot code whenever WDT reset.
        # And the sencond boot code stands for the other boot source.
        boot_source=0x00000001
    elif [ "$1" = "slave" ]; then
        if [ $boot_code_source = 1 ]; then
            echo "Current boot source is slave, no need to switch."
            return 0
        fi
        # No matter BMC boot from any one of master and slave.
        # Set bit_7 to 1 : Use second boot code whenever WDT reset.
        # And the sencond boot code stands for the other boot source.
        boot_source=0x00000001
    fi

    # Extend wdt1 timeout value to prevent wdt1 timeout during uboot stage.
    if [ "$(LC_ALL=C type -t wdt_set_timeout)" = function ]; then
        wdt_set_timeout 120
    else
        wdtcli set-timeout 120
    fi
    echo "BMC will switch to $1 after 2 seconds    ..."
    # Set WDT time out 2s, 0x00000014 = 2s
    devmem "$FMC_WDT2_RELOAD_VAL_REG" 32 0x00000014
    # WDT magic number to restart WDT counter to decrease.
    devmem "$FMC_WDT2_RESTART_REG" 32 0x4755
    devmem "$FMC_WDT2_CTRL_STATUS_REG" 32 $boot_source
}

bios_boot_info() {
    master_disable=$(( $(head -n 1 < "${SCMCPLD_SYSFS_DIR}/iso_buff_brg_com_bios_dis0_n") ))
    slave_disable=$(( $(head -n 1 < "${SCMCPLD_SYSFS_DIR}/iso_buff_brg_com_bios_dis1_n") ))
    boot_source="master"
    if [ $master_disable -eq 0 ]; then
        boot_source="master"
    elif [ $slave_disable -eq 0 ]; then
        boot_source="slave"
    fi
    echo $boot_source
}

bios_boot_from() {
    boot_source=$(bios_boot_info)
    if [ "$1" = "master" ]; then
        if [ "$boot_source" = "master" ]; then
            echo "Current boot source is master, no need to switch."
            return 0
        fi
        echo 0 > "${SCMCPLD_SYSFS_DIR}/iso_buff_brg_com_bios_dis0_n"
        echo 0 > "${SCMCPLD_SYSFS_DIR}/iso_buff_brg_com_bios_dis1_n"

        echo "uServer will boot from $1 bios at next reboot!!!!"
    elif [ "$1" = "slave" ]; then
        if [ "$boot_source" = "slave" ]; then
            echo "Current boot source is slave, no need to switch."
            return 0
        fi
        echo 1 > "${SCMCPLD_SYSFS_DIR}/iso_buff_brg_com_bios_dis0_n"
        echo 0 > "${SCMCPLD_SYSFS_DIR}/iso_buff_brg_com_bios_dis1_n"

        echo "uServer will boot from $1 bios at next reboot!!!!"
    else
        echo "Unknown uServer boot source, should be either master or slave!"
    fi
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
    "bios")
        if [ $# -eq 1 ]; then
            bios_boot_info
            exit 0
        else
            if [ "$2" = "reset" ] && [ $# -ge 3 ]; then
                case $3 in
                    "master" | "slave")
                        bios_boot_from "$3"
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
