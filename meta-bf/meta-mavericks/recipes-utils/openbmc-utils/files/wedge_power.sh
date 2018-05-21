#!/bin/bash
#
# Copyright 2015-present Facebook. All Rights Reserved.
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

PWR_BTN_GPIO="BMC_PWR_BTN_OUT_N"
PWR_SYSTEM_SYSFS="${SYSCPLD_SYSFS_DIR}/pwr_cyc_all_n"
PWR_USRV_RST_SYSFS="${SYSCPLD_SYSFS_DIR}/usrv_rst_n"
PWR_TH_RST_SYSFS="${SYSCPLD_SYSFS_DIR}/th_sys_rst_n"
MAIN_PWR="${SYSCPLD_SYSFS_DIR}/pwr_main_n"
PWR_USRV_SYSFS="${SYSCPLD_SYSFS_DIR}/pwr_main_n"

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
    echo "      -s: Power reset whole wedge system ungracefully"
    echo
}

main_power_status() {
  status=$(cat $MAIN_PWR | head -1 )
  if [ "$status" == "0x1" ]; then
      return 0            # powered on
  else
      return 1
  fi
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
    main_power_status
    rc=$?
    if [ $rc == "0" ]; then
        echo "System main power is on"
    else
        echo "System main power is off"
        return_code=1
    fi
    return $return_code
}

do_on_com_e() {
    echo 1 > $PWR_USRV_SYSFS
    return $?
}

do_on_main_pwr() {
  main_power_status
  rc=$?
  if [ $rc == "1" ]; then
    echo 1 > $MAIN_PWR
    ret=$?
    if [ $ret -eq 0 ]; then
      echo "Turning on system main power"
      logger "Successfully power on main power"
      return 0
    else
      return 1
    fi
  fi
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
        if wedge_is_us_on 10 "."; then
            echo " Already on. Skip!"
            return 1
        fi
    fi

    # reset Tofino
    reset_brcm.sh
    # power on sequence
    do_on_main_pwr
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
        pulse_us=100000             # 100ms
        logger "Power reset the whole system ..."
        echo -n "Power reset the whole system ..."
        sleep 1
        echo 0 > $PWR_SYSTEM_SYSFS
        # Echo 0 above should work already. However, after CPLD upgrade,
        # We need to re-generate the pulse to make this work
        usleep $pulse_us
        echo 1 > $PWR_SYSTEM_SYSFS
        usleep $pulse_us
        echo 0 > $PWR_SYSTEM_SYSFS
        usleep $pulse_us
        echo 1 > $PWR_SYSTEM_SYSFS
    else
        if ! wedge_is_us_on; then
            echo "Power resetting microserver that is powered off has no effect."
            echo "Use '$prog on' to power the microserver on"
            return -1
        fi
        # reset Tofino first
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
    *)
        usage
        exit -1
        ;;
esac

exit $?
