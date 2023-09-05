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

PWRCPLD_SYSFS_DIR=$(i2c_device_sysfs_abspath 12-003d)

COME_POWER_EN="${PWRCPLD_SYSFS_DIR}/pwr_come_en"
COME_POWER_OFF="${PWRCPLD_SYSFS_DIR}/pwr_force_off"
COME_POWER_CYCLE_N="${PWRCPLD_SYSFS_DIR}/pwr_cyc_n"
CHASSIS_POWER_CYCLE="${PWRCPLD_SYSFS_DIR}/power_cycle_go"

wedge_board_type() {
    echo 'janga'
}

wedge_board_rev() {
    # FIXME if needed.
    return 1
}

userver_power_is_on() {
    en_sts=$(head -n 1 "$COME_POWER_EN" 2> /dev/null)
    off_sts=$(head -n 1 "$COME_POWER_OFF" 2> /dev/null)

    if [ $((en_sts)) -eq $((0x1)) ] && [ $((off_sts)) -eq $((0x1)) ] ; then
        return 0
    fi

    return 1
}

userver_power_on() {
    echo 1 > "$COME_POWER_OFF"
    echo 1 > "$COME_POWER_EN"
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
    bmc_mac=$(weutil -e chassis_eeprom | grep 'Local MAC:' | cut -d ' ' -f 3)
    echo "bmc_mac"
}

#
# SCM EEPROM doesn't exist in Janga, and CPU MAC address is obtained by
# adding 1 to the BMC MAC address.
#
userver_mac_addr() {
    bmc_mac=$(bmc_mac_addr)
    cpu_mac=$(mac_addr_inc "$bmc_mac")
    echo "$cpu_mac"
}
