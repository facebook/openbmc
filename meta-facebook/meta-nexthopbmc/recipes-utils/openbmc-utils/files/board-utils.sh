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

. /usr/local/bin/gpio-utils.sh

wedge_board_type() {
    echo 'nexthopbmc'
}

wedge_board_rev() {
    product_state=$(weutil -e bmc_eeprom | grep 'Production State' | cut -d ' ' -f3)
    product_sub_state=$(weutil -e bmc_eeprom | grep 'Production Sub-State' | cut -d ' ' -f3)
    echo "$product_state$product_sub_state"
}

userver_power_is_on() {
    local val
    val=$(gpio_get_value CPE_CTRL)
    if [ "$val" = "1" ]; then
        return 0
    else
        return 1
    fi
}

userver_power_on() {
    gpio_set_value CPE_CTRL 1
}

userver_power_off() {
    gpio_set_value CPE_CTRL 0
}

userver_reset() {
    userver_power_off

    sleep 5

    userver_power_on
    return 0
}

chassis_power_cycle() {
    gpio_set_value BMC_PWR_CYC_REQ 1
}

bmc_mac_addr() {
    # Fetch mac addr supporting v5+ format.
    bmc_mac=$(weutil -e bmc_eeprom | sed -nE 's/BMC MAC Base: (.*)/\1/p')
    if [ -z "$bmc_mac" ]; then
        echo "BMC MAC Address Not Found !" 1>&2
        logger -p user.crit "BMC MAC Address Not Found !"
        return 1
    else
        echo "$bmc_mac"
    fi
}

userver_mac_addr() {
    # Fetch mac addr supporting v5+ format.
    cpu_mac=$(weutil -e bmc_eeprom | sed -nE 's/X86 CPU MAC Base: (.*)/\1/p')
    if [ -z "$cpu_mac" ]; then
        echo "x86 CPU MAC Address Not Found !" 1>&2
        logger -p user.crit "x86 CPU MAC Address Not Found !"
        return 1
    else
        echo "$cpu_mac"
    fi
}
