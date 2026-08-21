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

# Serializes userver power operations (on/off/reset) so concurrent callers
# don't interleave the power sequence on the CPE_CTRL GPIO.
USERVER_PWR_LOCK="/run/userver_pwr.lock"

# Standard Linux error codes used as exit codes (per FBOSS wedge_power.sh spec).
EBUSY_ERR=16     # Device or resource busy (power sequencing in progress)
EINVAL_ERR=22    # Invalid argument (reset requested while userver is off)

wedge_board_type() {
    echo 'nexthopbmc'
}

wedge_board_rev() {
    product_state=$(weutil -e bmc_eeprom | grep 'Production State' | cut -d ' ' -f3)
    product_sub_state=$(weutil -e bmc_eeprom | grep 'Production Sub-State' | cut -d ' ' -f3)
    echo "$product_state$product_sub_state"
}

userver_power_is_on() {
    [ "$(gpio_get_value CPE_CTRL)" = "1" ]
}

userver_power_on() {
    (
        # Bail out if another power operation is already in progress.
        if ! flock -n 9; then
            echo "userver_power_on: power sequencing in progress, try again later" >&2
            exit $EBUSY_ERR
        fi

        # No-op if the userver is already powered on.
        if userver_power_is_on; then
            exit 0
        fi

        gpio_set_value CPE_CTRL 1
    ) 9>"$USERVER_PWR_LOCK"
}

userver_power_off() {
    (
        # Bail out if another power operation is already in progress.
        if ! flock -n 9; then
            echo "userver_power_off: power sequencing in progress, try again later" >&2
            exit $EBUSY_ERR
        fi

        # No-op if the userver is already powered off.
        if ! userver_power_is_on; then
            exit 0
        fi

        gpio_set_value CPE_CTRL 0

        sleep 6
    ) 9>"$USERVER_PWR_LOCK"
}

userver_reset() {
    (
        # Bail out if another power operation is already in progress.
        if ! flock -n 9; then
            echo "userver_reset: power sequencing in progress, try again later" >&2
            exit $EBUSY_ERR
        fi

        # Reset is invalid while the userver is powered off.
        if ! userver_power_is_on; then
            echo "userver is off, please run <wedge_power.sh on> to power on userver" >&2
            exit $EINVAL_ERR
        fi

        gpio_set_value CPE_CTRL 0

        sleep 6

        gpio_set_value CPE_CTRL 1
    ) 9>"$USERVER_PWR_LOCK"
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
