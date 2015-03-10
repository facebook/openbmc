#!/bin/bash
#
# Copyright 2014-present Facebook. All Rights Reserved.
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

. /usr/local/fbpackages/utils/ast-functions

PATH=/sbin:/bin:/usr/sbin:/usr/bin:/usr/local/bin

prog="$0"

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
    echo "      -s: Power reset whole wedge system ungracefully"
    echo
}

# Because the voltage leak from uS COM pins could cause uS to struck when transitting
# from S5 to S0, we will need to explicitely pull down uS COM pins before powering off/reset
# and restoring COM pins after

pull_down_us_com() {
    # set GPIOL6 and GPIOL7 low
    devmem_clear_bit $(scu_addr 84) 22
    devmem_clear_bit $(scu_addr 84) 23
    gpio_set 94 0
    gpio_set 95 0
    # now, connect uart from BMC to the uS
    gpio_set 32 1
}

restore_us_com() {
    devmem_set_bit $(scu_addr 84) 22
    devmem_set_bit $(scu_addr 84) 23
    # if sol.sh is running, keep uart from uS connected with BMC
    if ps | grep sol.sh > /dev/null 2>&1; then
        gpio_set 32 1
    else
        gpio_set 32 0
    fi
}

do_status() {
    echo -n "Microserver power is "
    if wedge_is_us_on; then
        echo "on"
    else
        echo "off"
    fi
    return 0
}

do_on() {
    local force opt
    force=0
    while getopts "f" opt; do
        case $opt in
            f)
                force=1
                ;;
            *)
                usage
                exit -1
                ;;

        esac
    done
    echo -n "Power on microserver ..."
    if [ $force -eq 0 ]; then
        # need to check if uS is on or not
        if wedge_is_us_on 10 "."; then
            echo " Already on. Skip!"
            return 1
        fi
    fi
    # first make sure, GPIOD1 (25) is high
    gpio_set 25 1
    # then, put GPIOP7 (127) to low
    gpio_set 127 0
    # generate the power on pulse
    gpio_set 25 0
    sleep 1
    gpio_set 25 1
    sleep 1
    restore_us_com
    # Turn on the power LED (GPIOE5)
    /usr/local/bin/power_led.sh on
    echo " Done"
    return 0
}

do_off() {
    echo -n "Power off microserver ..."
    pull_down_us_com
    # first make sure, GPIOD1 (25) is high
    gpio_set 25 1
    # then, put GPIOP7 (127) to low
    gpio_set 127 0
    gpio_set 25 0
    sleep 5
    gpio_set 25 1
    # Turn off the power LED (GPIOE5)
    /usr/local/bin/power_led.sh off
    echo " Done"
    return 0
}

do_reset() {
    local system opt
    system=0
    while getopts "s" opt; do
        case $opt in
            s)
                system=1
                ;;
            *)
                usage
                exit -1
                ;;
        esac
    done
    if [ $system -eq 1 ]; then
        echo -n "Power reset whole system ..."
        rmmod adm1275
        i2cset -y 12 0x10 0xd9 c
    else
        if ! wedge_is_us_on; then
            echo "Power resetting microserver that is powered off has no effect."
            echo "Use '$prog on' to power the microserver on"
            return -1
        fi
        echo -n "Power reset microserver ..."
        pull_down_us_com
        # then, put GPIOP7 (127) to low
        gpio_set 127 0
        gpio_set 17 0
        sleep 1
        gpio_set 17 1
        sleep 1
        restore_us_com
    fi
    echo " Done"
    return 0
}

if [ $# -lt 1 ]; then
    usage
    exit -1
fi

command="$1"
shift

case "$command" in
    status)
        do_status $@
        ;;
    on)
        do_on $@
        ;;
    off)
        do_off $@
        ;;
    reset)
        do_reset $@
        ;;
    *)
        usage
        exit -1
        ;;
esac

exit $?
