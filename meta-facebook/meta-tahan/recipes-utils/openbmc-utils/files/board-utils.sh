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

#shellcheck disable=SC1091
# Do not change this line to openbmc-utils.sh, or it will generate a source loop.
. /usr/local/bin/i2c-utils.sh

PWRCPLD_SYSFS_DIR=$(i2c_device_sysfs_abspath 12-0060)

CHASSIS_POWER_CYCLE="${PWRCPLD_SYSFS_DIR}/power_cycle_go"
BOARD_ID="${PWRCPLD_SYSFS_DIR}/board_id"
VERSION_ID="${PWRCPLD_SYSFS_DIR}/version_id"
PWR_CPLD_MJR="${PWRCPLD_SYSFS_DIR}/cpld_major_ver"
PWR_CPLD_MNR="${PWRCPLD_SYSFS_DIR}/cpld_minor_ver"

wedge_board_type() {
    echo 'tahan'
}

wedge_board_rev() {
    board_id=$(head -n 1 < "$BOARD_ID" 2> /dev/null)
    version_id=$(head -n 1 < "$VERSION_ID" 2> /dev/null)

    case "$((board_id))" in
        1)
            echo "Board type: Tahan Switching System"
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
    echo "FIXME: feature not implemented!!"
    return 1
}

userver_power_on() {
    echo "FIXME: feature not implemented!!"
    return 1
}

userver_power_off() {
    echo "FIXME: feature not implemented!!"
    return 1
}

userver_reset() {
    return 1
    echo "FIXME: feature not implemented!!"
}

chassis_power_cycle() {
    echo 1 > "$CHASSIS_POWER_CYCLE"
    return 1
}

bmc_mac_addr() {
    echo "FIXME: feature not implemented!!"
    return 1
}

userver_mac_addr() {
    echo "FIXME: feature not implemented!!"
    return 1
}

# The function is used to display the cpld version. 
wedge_cpld_rev(){
    # PWR CPLD version
    wedge_pwr_cpld_rev    
}

wedge_pwr_cpld_rev(){
    # PWR CPLD version
    pwr_cpld_major=$(head -n 1 < "$PWR_CPLD_MJR" 2> /dev/null)
    pwr_cpld_minor=$(head -n 1 < "$PWR_CPLD_MNR" 2> /dev/null)

    echo "PWR_CPLD Version:$((pwr_cpld_major)).$((pwr_cpld_minor))"
}
