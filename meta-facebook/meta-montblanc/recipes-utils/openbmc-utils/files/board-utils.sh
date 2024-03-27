#!/bin/bash
#
# Copyright (c) Meta Platforms, Inc. and affiliates.
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

#shellcheck disable=SC1091,SC2034
# Do not change this line to openbmc-utils.sh, or it will generate a source loop.
. /usr/local/bin/i2c-utils.sh

MCBCPLD_SYSFS_DIR=$(i2c_device_sysfs_abspath 12-0060)
SCMCPLD_SYSFS_DIR=$(i2c_device_sysfs_abspath 1-0035)

PWR_FORCE_OFF="${MCBCPLD_SYSFS_DIR}/pwr_force_off"
PWR_COME_EN="${MCBCPLD_SYSFS_DIR}/pwr_come_en"
PWR_COME_CYCLE_N="${MCBCPLD_SYSFS_DIR}/pwr_come_cycle_n"

TIMER_BASE_10S="${MCBCPLD_SYSFS_DIR}/timer_base_10s"
TIMER_BASE_1S="${MCBCPLD_SYSFS_DIR}/timer_base_1s"
TIMER_BASE_100MS="${MCBCPLD_SYSFS_DIR}/timer_base_100ms"
TIMER_BASE_10MS="${MCBCPLD_SYSFS_DIR}/timer_base_10ms"
TIMER_COUNTER_SETTING="${MCBCPLD_SYSFS_DIR}/timer_counter_setting"
TIMER_COUNTER_SETTING_UPDATE="${MCBCPLD_SYSFS_DIR}/timer_counter_setting_update"
POWER_CYCLE_GO="${MCBCPLD_SYSFS_DIR}/power_cycle_go"

BOARD_ID="${MCBCPLD_SYSFS_DIR}/board_id"
VERSION_ID="${MCBCPLD_SYSFS_DIR}/version_id"

wedge_board_type() {
    echo 'MONTBLANC'
}

# read hardware revision from MCB CPLD register
wedge_board_rev() {
    board_id=$(head -n 1 < "$BOARD_ID" 2> /dev/null)
    version_id=$(head -n 1 < "$VERSION_ID" 2> /dev/null)

    case "$((board_id))" in
        1)
            echo "Board type: Minipack3 Switching System"
            ;;
        *)
            echo "Board type: unknown value [$board_id]"
            ;;
    esac

    case "$((version_id))" in
        0)
            echo "Revision: EVT-1"
            ;;
        *) 
            echo "Revision: unknown value [$version_id]"
            ;;
    esac

    return 0
}

userver_power_is_on() {
    if [ ! -e "$PWR_FORCE_OFF" ] || [ ! -e "$PWR_COME_EN" ]; then
        echo "Error: $PWR_FORCE_OFF does not exist! Is mcbcpld ready??"
        echo "Assuming uServer is off!"
        return 1
    fi

    val1=$(head -n 1 < "$PWR_FORCE_OFF" 2> /dev/null)
    val2=$(head -n 1 < "$PWR_COME_EN" 2> /dev/null)

    if [ $((val1)) -eq $((0x1)) ] && [ $((val2)) -eq $((0x1)) ] ; then
        return 0            # powered on
    else
        return 1
    fi
}

userver_power_on() {
    if ! sysfs_write "$PWR_FORCE_OFF" 1; then
        return 1
    fi
    if ! sysfs_write "$PWR_COME_EN" 1; then
        return 1
    fi

    return 0
}

userver_power_off() {
    if ! sysfs_write "$PWR_FORCE_OFF" 0; then
        return 1
    fi

    return 0
}

userver_reset() {
    # write 0 to trigger CPLD power cycling COMe
    # then this bit will auto set to 1 after Power cycle finish
    if ! sysfs_write "$PWR_COME_CYCLE_N" 0; then
        return 1
    fi
    
    timeout=10 #timeout 10 second
    while [ $((timeout)) -ge 0 ]; do
        val=$(head -n 1 < "$PWR_COME_CYCLE_N" 2> /dev/null)
        if [ $((val)) -eq 1 ]; then
            # reset successful
            return 0   
        fi
        sleep 1
        timeout=$((timeout-1))
    done
    # reset fail
    echo " userver reset failed."
    return 1
}

chassis_power_cycle() {
    # TIME_COUTER will be calculated 
    # based on register TIMER_BASE (10S,1S,100MS,10MS)
    #
    # eg. TIMER_BASE set as 100MS
    #     TIMER_COUNTER_SETTING set as 50
    #     timer is 50 x 100ms = 5 sec.
    #
    # Fall through on failures.
    #
    sysfs_write "$TIMER_BASE_10S" 0
    sysfs_write "$TIMER_BASE_1S" 0
    sysfs_write "$TIMER_BASE_100MS" 1
    sysfs_write "$TIMER_BASE_10MS" 0
    sysfs_write "$TIMER_COUNTER_SETTING" 50
    sysfs_write "$TIMER_COUNTER_SETTING_UPDATE" 1

    if ! sysfs_write "$POWER_CYCLE_GO" 1; then
        return 1
    fi

    return 0
}

bmc_mac_addr() {
    # Fetch mac addr supporting v4 and v5 format.
    weutil_output=$(weutil -e chassis_eeprom | sed -nE 's/((Local MAC)|(BMC MAC Base)): (.*)/\4/p')

    if [ -n "$weutil_output" ]; then
        # Mac address: xx:xx:xx:xx:xx:xx
        echo "$weutil_output" 
        return 0
    else
        echo "Cannot find out the BMC MAC" 1>&2
        return 1
    fi
}

userver_mac_addr() {
    # Fetch mac addr supporting v4 and v5 format.
    weutil_output=$(weutil -e scm_eeprom | sed -nE 's/((Local MAC)|(X86 CPU MAC Base)): (.*)/\4/p')

    if [ -n "$weutil_output" ]; then
        # Mac address: xx:xx:xx:xx:xx:xx
        echo "$weutil_output" 
        return 0
    else
        echo "Cannot find out the microserver MAC" 1>&2
        return 1
    fi
}
