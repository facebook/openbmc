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

#shellcheck disable=SC1091
. /usr/local/bin/openbmc-utils.sh

board_type=$(type wedge_board_type >/dev/null 2>&1 && wedge_board_type || echo "no_type")

if [ "$board_type" = "no_type" ]; then
    echo "Unkonwn board type!"
    exit 1
fi

usage() {
    program=$(basename "$0")
    echo "Usage:"
    if [ "$board_type" != "CMM" ]; then
        echo "  $program <bmc|bios> reset <master|slave>"
    else
        echo "  $program <bmc> reset <master|slave>"
    fi

    echo ""
    echo "Examples:"
    echo "  $program bmc"
    echo "  $program bmc reset master"
}

board_type=$(type wedge_board_type >/dev/null 2>&1 && wedge_board_type || echo "no_type")

WDT1_STATUS_REG="0x1e785010"
WDT1_CLR_STATUS_REG="0x1e785014"

WDT2_STATUS_REG="0x1e785030"
WDT2_CLR_STATUS_REG="0x1e785034"
WDT2_CNT_RELAOD_REG="0x1e785024"
WDT2_CNT_RESTART_REG="0x1e785028"
WDT2_CTRL_REG="0x1e78502c"

check_boot_source(){
    # Please refer to reg WDT1/WDT2 Control Register definition to
    # understand this code block, WDT1 is on page 646 of ast2500v16.pdf
    # and WDT2 is on page 649 of ast2500v16.pdf
    # get watch dog1 timeout status register
    wdt1=$(devmem "$WDT1_STATUS_REG")

    # get watch dog2 timeout status register
    wdt2=$(devmem "$WDT2_STATUS_REG")

    wdt1_timeout_cnt=$(( ((wdt1 & 0xff00) >> 8) ))
    wdt2_timeout_cnt=$(( ((wdt2 & 0xff00) >> 8) ))
    wdt1_boot_code_source=$(( ((wdt1 & 0x2) >> 1) ))
    wdt2_boot_code_source=$(( ((wdt2 & 0x2) >> 1) ))
    boot_code_source=0

    # Check both WDT1 and WDT2 to indicate the boot source
    if [ $wdt1_timeout_cnt -ge 1 ] && [ $wdt1_boot_code_source -eq 1 ]; then
        boot_code_source=1
    elif [ $wdt2_timeout_cnt -ge 1 ] && [ $wdt2_boot_code_source -eq 1 ]; then
        boot_code_source=1
    fi

    echo $boot_code_source
}

bmc_boot_info() {
    # get watch dog1 timeout status register
    wdt1=$(devmem "$WDT1_STATUS_REG")

    # get watch dog2 timeout status register
    wdt2=$(devmem "$WDT2_STATUS_REG")

    wdt1_timeout_cnt=$(( ((wdt1 & 0xff00) >> 8) ))
    wdt2_timeout_cnt=$(( ((wdt2 & 0xff00) >> 8) ))

    boot_code_source=$(check_boot_source)

    boot_source="Master Flash"
    if [ $((boot_code_source)) -eq 1 ]
    then
      boot_source="Slave Flash"
    fi

    echo "WDT1 Timeout Count: " $wdt1_timeout_cnt
    echo "WDT2 Timeout Count: " $wdt2_timeout_cnt
    echo "Current Boot Code Source: $boot_source"
}

bmc_boot_from() {
    # Please refer to reg WDT1/WDT2 Control Register definition to
    # understand this code block, WDT1 is on page 646 of ast2500v16.pdf
    # and WDT2 is on page 649 of ast2500v16.pdf
    # Enable watchdog reset_system_after_timeout bit and WDT_enable_signal bit.
    # Refer to ast2500v16.pdf page 650th.
    boot_source=0x00000013
    boot_code_source=$(check_boot_source)
    if [ "$1" = "master" ]; then
        if [ $((boot_code_source)) -eq 0 ]; then
            echo "Current boot source is master, no need to switch."
            return 0
        fi
        # Set bit_7 to 0 : Use default boot code whenever WDT reset.
        boot_source=0x00000033
    elif [ "$1" = "slave" ]; then
        if [ $((boot_code_source)) -eq 1 ]; then
            echo "Current boot source is slave, no need to switch."
            return 0
        fi
        # No matter BMC boot from any one of master and slave.
        # Set bit_7 to 1 : Use second boot code whenever WDT reset.
        # And the sencond boot code stands for the other boot source.
        boot_source=0x000000b3
    fi
    echo "BMC will switch to $1 after 10 seconds..."
    # Clear WDT1 counter and boot code source status
    devmem "$WDT1_CLR_STATUS_REG" w 0x77
    # Clear WDT2 counter and boot code source status
    devmem "$WDT2_CLR_STATUS_REG" w 0x77
    # Set WDT time out 10s, 0x00989680 = 10,000,000 us
    devmem "$WDT2_CNT_RELAOD_REG" 32 0x00989680
    # WDT magic number to restart WDT counter to decrease.
    devmem "$WDT2_CNT_RESTART_REG" 32 0x4755
    devmem "$WDT2_CTRL_REG" 32 $boot_source
}

bios_boot_info() {
    # CPLD path and related COMe BIOS select bits may differ for different
    # AST2500 based platforms. Additional logic may needed for specific
    # platforms
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
        if [ "$board_type" != "CMM" ]; then
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
        fi
    ;;
    *)
        usage
        exit 1
    ;;
esac
