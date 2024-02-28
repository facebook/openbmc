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

PWRCPLD_SYSFS_DIR=$(i2c_device_sysfs_abspath 12-0060)
SCMCPLD_SYSFS_DIR=$(i2c_device_sysfs_abspath 1-0035)

BOARD_ID="${PWRCPLD_SYSFS_DIR}/board_id"
VERSION_ID="${PWRCPLD_SYSFS_DIR}/version_id"

COME_POWER_OFF="${SCMCPLD_SYSFS_DIR}/pwr_force_off"
COME_POWER_CYCLE_N="${SCMCPLD_SYSFS_DIR}/pwr_cyc_n"
CHASSIS_POWER_CYCLE="${PWRCPLD_SYSFS_DIR}/power_cycle_go"

wedge_board_type() {
    echo 'janga'
}

wedge_board_rev() {
    board_id=$(head -n 1 < "$BOARD_ID" 2> /dev/null)
    version_id=$(head -n 1 < "$VERSION_ID" 2> /dev/null)

    case "$((board_id))" in
        4)
            echo "Board type: Janga Switching System"
            ;;
        *)
            echo "Board type: unknown value [$board_id]"
            ;;
    esac

    case "$((version_id))" in
        0)
            echo "Revision: EVT-1"
            ;;
        1)
            echo "Revision: EVT-2"
            ;;
        *)
            echo "Revision: unknown value [$version_id]"
            ;;
    esac

    return 0
}

userver_power_is_on() {
    off_sts=$(head -n 1 "$COME_POWER_OFF" 2> /dev/null)

    if [ $((off_sts)) -eq $((0x1)) ] ; then
        return 0
    fi

    return 1
}

userver_power_on() {
    echo 1 > "$COME_POWER_OFF"
}

userver_power_off() {
    echo 0 > "$COME_POWER_OFF"
}

userver_reset() {
    echo 0 > "$COME_POWER_CYCLE_N"

    # $COME_POWER_CYCLE_N will be auto-set to 1 when power cycle completes.
    timeout=10 # 10 seconds
    while [ $((timeout)) -ge 0 ]; do
        val=$(head -n 1 "$COME_POWER_CYCLE_N" 2> /dev/null)
        if [ $((val)) -eq 1 ]; then
            return 0
        fi

        sleep 1
        timeout=$((timeout-1))
    done

    echo "failed to reset userver!"
    return 1
}

chassis_power_cycle() {
    echo 1 > "$CHASSIS_POWER_CYCLE"
}

bmc_mac_addr() {
    # Fetch mac addr supporting v4 and v5 format.
    bmc_mac=$(weutil | sed -nE 's/((Local MAC)|(BMC MAC Base)): (.*)/\4/p')
    if [ ! "$bmc_mac" ]; then
        echo "BMC MAC Address Not Found !"
    else
        echo "$bmc_mac"
    fi
}

userver_mac_addr() {
    mac_addr_string=$(weutil | grep -E "(X86 CPU MAC Base)|(Local MAC)")
    [[ "$mac_addr_string" =~ (((X86 CPU MAC Base)|(Local MAC)):[[:space:]]*(.*)) ]];
    mac_addr_type="${BASH_REMATCH[2]}"
    mac_addr_value="${BASH_REMATCH[5]}"
    case "$mac_addr_type" in
        # Fetch cpu mac addr based on EEPROM v5.
        "X86 CPU MAC Base")
            echo "$mac_addr_value"
            ;;
        "Local MAC")
            #
            # Fetch cpu mac addr based on EEPROM v4 and older.
            # SCM EEPROM doesn't exist in Janga, and CPU MAC address is obtained by
            # adding 1 to the BMC MAC address.
            #
            cpu_mac=$(mac_addr_inc "$mac_addr_value")
            echo "$cpu_mac"
            ;;
        *)
            echo "CPU MAC Address Not Found !"
            ;;
    esac
}
