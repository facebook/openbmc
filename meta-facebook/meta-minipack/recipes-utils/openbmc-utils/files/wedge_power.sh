#!/bin/bash
#
# Copyright 2018-present Facebook. All Rights Reserved.
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

prog="$0"

PWR_USRV_RST_SYSFS="${SCMCPLD_SYSFS_DIR}/iso_com_rst_n"
PWR_TH_RST_SYSFS="${SMBCPLD_SYSFS_DIR}/cpld_mac_reset_n"

SCM_CPLD_BUS=16
PIM_CPLD_BUS=(84 92 10 108 116 124 132 140)
CPLD_ADDR=0x10
CPLD_RESET_CMD=0xd9

usage() {
    echo "Usage: $prog <command> [command options]"
    echo
    echo "Commands:"
    echo "  status: Get the current microserver power status"
    echo
    echo "  on: Power on microserver if not powered on already"
    echo "    options:"
    echo "      -f: Re-do power on sequence no matter if microserver has "
    echo "          been powered on or not."
    echo
    echo "  off: Power off microserver ungracefully"
    echo
    echo "  reset: Power reset microserver ungracefully"
    echo "    options:"
    echo "      -s: Power reset whole minipack system ungracefully"
    echo
    echo "  pimreset: Power-cycle one or all PIM(s)"
    echo "    options:"
    echo "      -a  : Reset all PIMs or "
    echo "      -2 , -3 , ... , -9 : Reset a single PIM (2, 3 ... 9) "
    echo
}

main_power_status() {
    return 0
}

do_status() {
    echo -n "Microserver power is "
    return_code=0

    if wedge_is_us_on; then
        echo "on"
    else
        echo "off"
        return_code=1
    fi

    return $return_code
}

do_on_com_e() {
    echo 1 > $PWR_USRV_SYSFS
    return $?
}

do_on_main_pwr() {
    # minipack com-e main power is controlled by SCMCPLD
    return 0
}

do_on() {
    local force opt ret
    force=0
    while getopts "f" opt; do
        case $opt in
            f)
                force=1
                ;;
            *)
                usage
                exit -1
                ;;
        esac
    done
    echo -n "Power on microserver ..."
    if [ $force -eq 0 ]; then
        # need to check if uS is on or not
        if wedge_is_us_on; then
            echo " Already on. Skip!"
            return 1
        fi
    fi

    # reset TH
    reset_brcm.sh
    # power on sequence
    do_on_com_e
    ret=$?
    if [ $ret -eq 0 ]; then
        echo " Done"
	logger "Successfully power on micro-server"
    else
        echo " Failed"
        logger "Failed to power on micro-server"
    fi
    return $ret
}

do_off_com_e() {
    echo 0 > $PWR_USRV_SYSFS
    return $?
}

do_off() {
    local ret
    echo -n "Power off microserver ..."
    do_off_com_e
    ret=$?
    if [ $ret -eq 0 ]; then
        echo " Done"
    else
        echo " Failed"
    fi
    return $ret
}

do_reset() {
    local system opt pulse_us
    system=0
    while getopts "s" opt; do
        case $opt in
            s)
                system=1
                ;;
            *)
                usage
                exit -1
                ;;
        esac
    done
    if [ $system -eq 1 ]; then

        logger "Power reset the whole system ..."
        echo -n "Power reset the whole system ..."
        echo -n "Hardware is not ready, do the best effort ..."
        # 0 in toggle_pim_reset means "all PIMs to reset"
        toggle_pim_reset 0
        toggle_scm_reset
        toggle_smb_reset
    else
        if ! wedge_is_us_on; then
            echo "Power resetting microserver that is powered off has no effect."
            echo "Use '$prog on' to power the microserver on"
            return -1
        fi
        # reset TH first
        reset_brcm.sh
        echo -n "Power reset microserver ..."
        echo 0 > $PWR_USRV_RST_SYSFS
        sleep 1
        echo 1 > $PWR_USRV_RST_SYSFS
        logger "Successfully power reset micro-server"
    fi
    echo " Done"
    return 0
}


toggle_pim_reset() {
    pim=$1
    for slot in 2 3 4 5 6 7 8 9; do
      if [ $pim -eq 0 ] || [ $slot -eq $pim ]; then
        index=$(expr $slot - 2)
         # We don't have PIM CPLD driver for now, 
         # so we will use raw i2c access for the time being
         echo Power-cycling PIM in slot $slot
         i2cset -f -y ${PIM_CPLD_BUS[$index]} $CPLD_ADDR $CPLD_RESET_CMD
      fi 
    done
}

toggle_scm_reset() {
  echo Power-cycling SCM
  # We don't have SCM CPLD driver for now, 
  # so we will use raw i2c access for the time being
  i2cset -f -y ${SCM_CPLD_BUS} $CPLD_ADDR $CPLD_RESET_CMD
}

toggle_smb_reset() {
  echo Power-cycling most of SMB
  # This is a temporary way to toggle reset most of SMB,
  # as per HW team. We will use this way for the time being
  # until one-shot reset command is supported in CPLD
  i2cset -f -y 1 0x3a 0x12 0
}

do_pimreset() {
    local pim opt retval rc
    retval=0
    while getopts "23456789a" opt; do
        case $opt in
            a)
                pim=0
                ;;
            2) 
                pim=2
                ;;
            3) 
                pim=3
                ;;
            4) 
                pim=4
                ;;
            5) 
                pim=5
                ;;
            6) 
                pim=6
                ;;
            7) 
                pim=7
                ;;
            8) 
                pim=8
                ;;
            9) 
                pim=9
                ;;
            *)
                usage
                exit -1
                ;;
        esac
    done

    toggle_pim_reset $pim

    return $retval
}

if [ $# -lt 1 ]; then
    usage
    exit -1
fi

command="$1"
shift

case "$command" in
    status)
        do_status $@
        ;;
    on)
        do_on $@
        ;;
    off)
        do_off $@
        ;;
    reset)
        do_reset $@
        ;;
    pimreset)
        do_pimreset $@
        ;;
    *)
        usage
        exit -1
        ;;
esac

exit $?
