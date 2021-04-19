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

prog="$0"
#board_rev=$(wedge_board_rev)

PWR_USRV_RST_SYSFS="${SCMCPLD_SYSFS_DIR}/sys_reset_n"
PWR_L_CYCLE_SYSFS="${LEFT_PDBCPLD_SYSFS_DIR}/power_cycle_go"
PWR_R_CYCLE_SYSFS="${RIGHT_PDBCPLD_SYSFS_DIR}/power_cycle_go"
PWR_L_TIMER_BASE_1S_SYSFS="${LEFT_PDBCPLD_SYSFS_DIR}/timer_base_1s"
PWR_R_TIMER_BASE_1S_SYSFS="${RIGHT_PDBCPLD_SYSFS_DIR}/timer_base_1s"
PWR_L_TIMER_BASE_10S_SYSFS="${LEFT_PDBCPLD_SYSFS_DIR}/timer_base_10s"
PWR_R_TIMER_BASE_10S_SYSFS="${RIGHT_PDBCPLD_SYSFS_DIR}/timer_base_10s"
PWR_L_TIMER_COUNTER_SETTING_SYSFS="${LEFT_PDBCPLD_SYSFS_DIR}/timer_counter_setting"
PWR_R_TIMER_COUNTER_SETTING_SYSFS="${RIGHT_PDBCPLD_SYSFS_DIR}/timer_counter_setting"
PWR_L_TIMER_COUNTER_SETTING_UPDATE_SYSFS="${LEFT_PDBCPLD_SYSFS_DIR}/timer_counter_setting_update"
PWR_R_TIMER_COUNTER_SETTING_UPDATE_SYSFS="${RIGHT_PDBCPLD_SYSFS_DIR}/timer_counter_setting_update"

PIM_NAME="${PIM_RST_SYSFS}/pim"
CPLD_RESET="_hw_rst_l"
CPLD_PG="_smb_pg"

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
    echo "      -s: Power reset whole fuji system ungracefully"
    echo "      -s -t [1-2550]: Setting boot up time."
    echo "            From 1 to 249 the step is 1 second."
    echo "            From 250 to 2550 the step is 10 seconds"
    echo "            and don't care ones-digits."
    echo
    echo "  pimreset: Power-cycle one or all PIM(s)"
    echo "    options:"
    echo "      -a  : Reset all PIMs or "
    echo "      -2 , -3 , ... , -9 : Reset a single PIM (2, 3 ... 9) "
    echo
    echo "  pim_power_on/off: Power-on/off one or all PIM(s)"
    echo "    options:"
    echo "      -a  : Power-on/off all PIMs or "
    echo "      -2 , -3 , ... , -9 : Power-on/off a single PIM (2, 3 ... 9) "
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
    echo 1 > "$PWR_USRV_SYSFS"
    echo 1 > "$PWR_USRV_FORCE_OFF"
    return $?
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
                exit 1
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
    echo 0 > "$PWR_USRV_FORCE_OFF"
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

do_config_reset_timer() {
    # Check numeric
    wake_t=$1
    echo "$wake_t" | grep -E -q '^[0-9]+$'
    ret=$?
    if [ $ret -ne 0 ]; then
        usage
        exit 1
    else
        if [ "$wake_t" -ge 1 ] && [ "$wake_t" -lt 250 ];then
            echo 1 > "$PWR_L_TIMER_BASE_1S_SYSFS"
            echo 1 > "$PWR_R_TIMER_BASE_1S_SYSFS"
            echo 0 > "$PWR_L_TIMER_BASE_10S_SYSFS"
            echo 0 > "$PWR_R_TIMER_BASE_10S_SYSFS"
            logger "Waiting $wake_t seconds for the system boot up"
            echo "Waiting $wake_t seconds for the system boot up"
        elif [ "$wake_t" -ge 250 ] && [ "$wake_t" -le 2550 ];then
            echo 0 > "$PWR_L_TIMER_BASE_1S_SYSFS"
            echo 0 > "$PWR_R_TIMER_BASE_1S_SYSFS"
            echo 1 > "$PWR_L_TIMER_BASE_10S_SYSFS"
            echo 1 > "$PWR_R_TIMER_BASE_10S_SYSFS"
            wake_t=$((wake_t/10))
            logger "Waiting $((wake_t * 10)) seconds for the system boot up"
            echo "Waiting $((wake_t * 10)) seconds for the system boot up"
        else
            usage
            exit 1
        fi
        echo $wake_t > "$PWR_L_TIMER_COUNTER_SETTING_SYSFS"
        echo $wake_t > "$PWR_R_TIMER_COUNTER_SETTING_SYSFS"
        echo 1 > "$PWR_L_TIMER_COUNTER_SETTING_UPDATE_SYSFS"
        echo 1 > "$PWR_R_TIMER_COUNTER_SETTING_UPDATE_SYSFS"
        echo 0 > "$PWR_L_TIMER_COUNTER_SETTING_UPDATE_SYSFS"
        echo 0 > "$PWR_R_TIMER_COUNTER_SETTING_UPDATE_SYSFS"
    fi
}

do_reset() {
    local system timer wake_t opt
    system=0
    timer=0
    wake_t=0
    while getopts "st:" opt; do
        case $opt in
            s)
                system=1
                ;;
            t)
                timer=1
                wake_t=$OPTARG
                ;;
            *)
                usage
                exit 1
                ;;
        esac
    done

    if [ $system -eq 1 ]; then
            if [ $timer -eq 1 ]; then
                do_config_reset_timer "$wake_t"
            fi
            # Synchronize cached writes to persistent storage
            # We found somethimes /mnt/data data lost after wedge_power.sh reset -s,
            # add sync in case of file system will be broken by random power cycle.
            sync
            logger "Power reset the whole system ..."
            echo  "Power reset the whole system ..."
            echo 1 > "$PWR_L_CYCLE_SYSFS"
            echo 1 > "$PWR_R_CYCLE_SYSFS"
            sleep 3
            # Control should not reach here, but if it failed to reset
            # the system through PSU, then run a workaround to reset
            # most of the system instead (if not all)
            logger "Failed to reset the system. Running a workaround"
            echo "Failed to reset the system. Running a workaround"
            i2cset -f -y 1 0x3a 0x12 0
    else
        if ! wedge_is_us_on; then
            echo "Power resetting microserver that is powered off has no effect."
            echo "Use '$prog on' to power the microserver on"
            return 1
        fi
        echo -n "Power reset microserver ..."
        echo 0 > "$PWR_USRV_RST_SYSFS"
        sleep 1
        echo 1 > "$PWR_USRV_RST_SYSFS"
        logger "Successfully power reset micro-server"
    fi
    echo " Done"
    return 0
}

toggle_pim_reset() {
    retval=0
    pim=$1
    for slot in 2 3 4 5 6 7 8 9; do
      if [ "$pim" -eq 0 ] || [ $slot -eq "$pim" ]; then
        index=$((slot - 1))
         echo Power-cycling PIM in slot $slot
         #i2cset -f -y ${PIM_CPLD_BUS[$index]} $CPLD_ADDR $CPLD_RESET_CMD
         echo 0 > "$PIM_NAME$index$CPLD_RESET"
         sleep 1
         echo 1 > "$PIM_NAME$index$CPLD_RESET"
         result=$?
         if [ $result -ne 0 ]; then
           retval=$result
         fi
      fi
    done
    return $retval
}

toggle_pim_power_onoff() {
	retval=0
	action=$1
	pim=$2
	for slot in 2 3 4 5 6 7 8 9; do
		if [ "$pim" -eq 0 ] || [ $slot -eq "$pim" ]; then
			index=$((slot - 1))
			if [ "$action" = "on" ]; then
				echo Power-on PIM in slot $slot
				echo 1 > "$PIM_NAME$index$CPLD_PG"
				result=$?
			elif [ "$action" = "off" ]; then
				echo Power-off PIM in slot $slot
				echo 0 > "$PIM_NAME$index$CPLD_PG"
				result=$?
			fi
			if [ "$result" -ne 0 ]; then
				retval=$result
			fi
		fi
	done
	return $retval
}

do_pimreset() {
    local pim opt retval
    retval=0
    pim=-1
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
                exit 1
                ;;
        esac
    done
    if [ $pim -eq -1 ]; then
      usage
      exit 1
    fi

    toggle_pim_reset $pim
    retval=$?

    return $retval
}

do_pim_power_on() {
	local pim opt retval
	retval=0
	pim=-1
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
				exit 1
				;;
		esac
	done
	if [ $pim -eq -1 ]; then
		usage
		exit 1
	fi

	toggle_pim_power_onoff on $pim
	retval=$?

	return $retval
}

do_pim_power_off() {
	local pim opt retval
	retval=0
	pim=-1
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
				exit 1
				;;
		esac
	done
	if [ $pim -eq -1 ]; then
		usage
		exit 1
	fi

	toggle_pim_power_onoff off $pim
	retval=$?

	return $retval
}

if [ $# -lt 1 ]; then
    usage
    exit 1
fi

command="$1"
shift

set_bic=0
case "$command" in
    status)
        do_status "$@"
        ;;
    on)
        do_on "$@"
        set_bic=1
        ;;
    off)
        do_off "$@"
        ;;
    reset)
        do_reset "$@"
        set_bic=1
        ;;
    pimreset)
        do_pimreset "$@"
        exit $?
        ;;
	pim_power_on)
		do_pim_power_on "$@"
		exit $?
		;;
	pim_power_off)
		do_pim_power_off "$@"
		exit $?
		;;
    *)
        usage
        exit 1
        ;;
esac

if [ $set_bic -ne 0 ]; then
# sleep 10 second to wait BIC & COMe ready
    (sleep 10; setup_bic.sh) &
fi

exit $?
