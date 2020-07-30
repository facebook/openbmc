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

#shellcheck disable=SC1091
# Do not change this line to openbmc-utils.sh, or it will generate a source loop.
. /usr/local/bin/i2c-utils.sh

SCMCPLD_SYSFS_DIR=$(i2c_device_sysfs_abspath 2-003e)
SMBCPLD_SYSFS_DIR=$(i2c_device_sysfs_abspath 12-003e)
PWRCPLD_SYSFS_DIR=$(i2c_device_sysfs_abspath 47-003e)
FCMCPLD_SYSFS_DIR=$(i2c_device_sysfs_abspath 48-003e)

PWR_USRV_SYSFS="${SCMCPLD_SYSFS_DIR}/com_exp_pwr_enable"
PWR_USRV_FORCE_OFF="${SCMCPLD_SYSFS_DIR}/com_exp_pwr_force_off"
PWR_TH_RST_SYSFS="${SMBCPLD_SYSFS_DIR}/mac_reset_n"

# CPLD version
SMB_CPLD_VER=$(head -n 1 "$SMBCPLD_SYSFS_DIR/cpld_ver" 2> /dev/null)
SMB_CPLD_SUB_VER=$(head -n 1 "$SMBCPLD_SYSFS_DIR/cpld_sub_ver" 2> /dev/null)
PWR_CPLD_VER=$(head -n 1 "$PWRCPLD_SYSFS_DIR/cpld_ver" 2> /dev/null)
PWR_CPLD_SUB_VER=$(head -n 1 "$PWRCPLD_SYSFS_DIR/cpld_sub_ver" 2> /dev/null)
SCM_CPLD_VER=$(head -n 1 "$SCMCPLD_SYSFS_DIR/cpld_ver" 2> /dev/null)
SCM_CPLD_SUB_VER=$(head -n 1 "$SCMCPLD_SYSFS_DIR/cpld_sub_ver" 2> /dev/null)
FCM_CPLD_VER=$(head -n 1 "$FCMCPLD_SYSFS_DIR/cpld_ver" 2> /dev/null)
FCM_CPLD_SUB_VER=$(head -n 1 "$FCMCPLD_SYSFS_DIR/cpld_sub_ver" 2> /dev/null)

export SCMCPLD_SYSFS_DIR
export SMBCPLD_SYSFS_DIR
export PWRCPLD_SYSFS_DIR
export FCMCPLD_SYSFS_DIR
export PWR_USRV_SYSFS
export PWR_USRV_FORCE_OFF
export PWR_TH_RST_SYSFS
export SMB_CPLD_VER
export SMB_CPLD_SUB_VER
export PWR_CPLD_VER
export PWR_CPLD_SUB_VER
export SCM_CPLD_VER
export SCM_CPLD_SUB_VER
export FCM_CPLD_VER
export FCM_CPLD_SUB_VER

wedge_is_us_on() {
    local val0 val1

    val0=$(head -n 1 < "$PWR_USRV_SYSFS" 2> /dev/null)
    val1=$(head -n 1 < "$PWR_USRV_FORCE_OFF" 2> /dev/null)
    
    if [ "$val0" == "0x1" ] && [ "$val1" == "0x1" ] ; then
        return 0            # powered on
    else
        return 1
    fi

    return 0
}

wedge_board_type_rev(){
    rev=$(wedge_board_rev)
    case $rev in
        0)
            echo "Cloudripper_EVT"
            ;;
        1)
            echo "Cloudripper_EVT"
            ;;
        *)
            echo "Cloudripper (Undefine $rev)"
            ;;
    esac
}

wedge_slot_id() {
    printf "1\n"
}

wedge_board_rev() {
    local val0 val1 val2
    val0=$(gpio_get BMC_CPLD_BOARD_REV_ID0)
    val1=$(gpio_get BMC_CPLD_BOARD_REV_ID1)
    val2=$(gpio_get BMC_CPLD_BOARD_REV_ID2)
    echo $((val0 | (val1 << 1) | (val2 << 2)))
}

wedge_power_on_board() {
    # Cloudripper SCM power is controlled by SCM CPLD
    return 0
}

wedge_prepare_cpld_update() {
    echo "Stop fscd service."
    sv stop fscd

    echo "Disable watchdog."
    /usr/local/bin/wdtcli stop

    echo "Set fan speed 40%."
    set_fan_speed.sh 40
}
