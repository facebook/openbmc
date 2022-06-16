#!/bin/bash
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

#shellcheck disable=SC1091,SC2034
# Do not change this line to openbmc-utils.sh, or it will generate a source loop.
. /usr/local/bin/i2c-utils.sh

SCMCPLD_SYSFS_DIR=$(i2c_device_sysfs_abspath 2-003e)
SMBCPLD_SYSFS_DIR=$(i2c_device_sysfs_abspath 12-003e)
if [[ ${KERNEL_VERSION} != 4.1.* ]]; then
    PWRCPLD_SYSFS_DIR=$(i2c_device_sysfs_abspath 31-003e)
    FCMCPLD_SYSFS_DIR=$(i2c_device_sysfs_abspath 32-003e)
else
    PWRCPLD_SYSFS_DIR=$(i2c_device_sysfs_abspath 33-003e)
    FCMCPLD_SYSFS_DIR=$(i2c_device_sysfs_abspath 34-003e)
fi
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

# SMB Board Rev
SMB_CPLD_BOARD_REV=$(head -n 1 "$SMBCPLD_SYSFS_DIR/board_ver" 2> /dev/null)

# 48V DC PSU P/N
DELTA_48V="0x45 0x43 0x44 0x32 0x35 0x30 0x31 0x30 0x30 0x31 0x35"
LITEON_48V="0x44 0x44 0x2d 0x32 0x31 0x35 0x32 0x2d 0x32 0x4c"

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
    full_board_type=$(wedge_full_board_type)
    board_type=$(wedge_board_type)
    rev=$(wedge_board_rev)
    if [ $((board_type)) -eq 0 ]; then
        if [ $(( full_board_type&2 )) -ne 0 ]; then
            case $rev in
                6)
                    echo "WEDGE400_MP_RESPIN"
                    ;;
                *)
                    echo "WEDGE400 (Undefine $rev)"
                    ;;
            esac
        else
            case $rev in
                0)
                    echo "WEDGE400_EVT/EVT3"
                    ;;
                2)
                    echo "WEDGE400_DVT"
                    ;;
                3)
                    echo "WEDGE400_DVT2/PVT1/PV2"
                    ;;
                4)
                    echo "WEDGE400_PVT3"
                    ;;
                5)
                    echo "WEDGE400_MP"
                    ;;
                *)
                    echo "WEDGE400 (Undefine $rev)"
                    ;;
            esac
        fi
    elif [ $((board_type)) -eq 1 ]; then
        if [ $(( full_board_type&2 )) -ne 0 ]; then
            case $rev in
                4)
                    echo "WEDGE400-C_MP_RESPIN"
                    ;;
                *)
                    echo "WEDGE400-C (Undefine $rev)"
                    ;;
            esac
        else
            case $rev in
                0)
                    echo "WEDGE400-C_EVT"
                    ;;
                1)
                    echo "WEDGE400-C_EVT2"
                    ;;
                2)
                    echo "WEDGE400-C_DVT"
                    ;;
                3)
                    echo "WEDGE400-C_DVT2"
                    ;;
                *)
                    echo "WEDGE400-C_(Undefine $rev)"
                    ;;
            esac
        fi
    else
        echo "Undefine_($type,$rev)"
    fi
}



wedge_board_type() {
    rev=$(gpio_get BMC_CPLD_BOARD_TYPE_0)
    if [ $((rev)) -eq 0 ]; then
        echo 1  # Wedge400-C
    else
        echo 0  # Wedge400
    fi
}

wedge_full_board_type() {
    # Wedge400 MP respin or later is added more 2 pins to indicate new board type
    local val0 val1 val2
    val0=$(gpio_get BMC_CPLD_BOARD_TYPE_0)
    val1=$(gpio_get BMC_CPLD_BOARD_TYPE_1)
    val2=$(gpio_get BMC_CPLD_BOARD_TYPE_2)
    echo $((val0 | (val1 << 1) | (val2 << 2)))
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
    # wedge400 SCM power is controlled by SCMCPLD
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

wedge_power_supply_type() {
    # Only PEM has max6615 which I2C slave address is 0x18
    is_pem1=$(i2cget -f -y 24 0x18 0x0 &> /dev/null; echo $?)
    is_pem2=$(i2cget -f -y 25 0x18 0x0 &> /dev/null; echo $?)
    if i2cget -y -f 24 0x58 0x9a s &> /dev/null;then    # PSU1
        is_psu1_48v=$(i2cget -y -f 24 0x58 0x9a s &)
    elif i2cget -y -f 25 0x58 0x9a s &> /dev/null;then  # PSU2
        is_psu2_48v=$(i2cget -y -f 25 0x58 0x9a s &)
    fi

    power_type=""
    if [ $# -eq 1 ]; then
        if [ "$1" = "1" ]; then
            if [ "$is_pem1" -eq 0 ]; then
                power_type="PEM"
            else
                power_type="PSU"
            fi
        elif [ "$1" = "2" ]; then
            if [ "$is_pem2" -eq 0 ]; then
                power_type="PEM"
            else
                power_type="PSU"
            fi
        else
            echo "wedge_power_supply_type < 1 | 2 >"
            return
        fi
    else
        if [ "$is_pem1" -eq 0 ] || [ "$is_pem2" -eq 0 ]; then
            power_type="PEM"
        else
            power_type="PSU"
        fi
    fi
    # Detect 48V DC PSU
    if [[ "$is_psu1_48v" = "$DELTA_48V" ||
        "$is_psu1_48v" = "$LITEON_48V" ||
        "$is_psu2_48v" = "$DELTA_48V" ||
        "$is_psu2_48v" = "$LITEON_48V" ]];then
            power_type="PSU48"
    fi
    echo $power_type
}
