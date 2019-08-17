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

# Do not change this line to openbmc-utils.sh, or it will generate a source loop.
. /usr/local/bin/i2c-utils.sh

SCMCPLD_SYSFS_DIR=$(i2c_device_sysfs_abspath 2-003e)
SMBCPLD_SYSFS_DIR=$(i2c_device_sysfs_abspath 12-003e)
if [[ ${KERNEL_VERSION} != 4.1.* ]]; then
    PWRCPLD_SYSFS_DIR=$(i2c_device_sysfs_abspath 29-003e)
    FCMCPLD_SYSFS_DIR=$(i2c_device_sysfs_abspath 30-003e)
else
    PWRCPLD_SYSFS_DIR=$(i2c_device_sysfs_abspath 31-003e)
    FCMCPLD_SYSFS_DIR=$(i2c_device_sysfs_abspath 32-003e)
fi
PWR_USRV_SYSFS="${SCMCPLD_SYSFS_DIR}/com_exp_pwr_enable"
PWR_USRV_FORCE_OFF="${SCMCPLD_SYSFS_DIR}/com_exp_pwr_force_off"
PWR_TH_RST_SYSFS="${SMBCPLD_SYSFS_DIR}/mac_reset_n"

wedge_is_us_on() {
    local val0 val1

    val0=$(cat $PWR_USRV_SYSFS 2> /dev/null | head -n 1)
    val1=$(cat $PWR_USRV_FORCE_OFF 2> /dev/null | head -n 1)
    
    if [ "$val0" == "0x1" ] && [ "$val1" == "0x1" ] ; then
        return 0            # powered on
    else
        return 1
    fi

    return 0
}

wedge_board_type() {
    rev=$(wedge_board_rev)
    if [ $((rev&0x04)) -eq 4 ]; then
        echo 'WEDGE400'
    else
        echo 'WEDGE400-2'
    fi
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
