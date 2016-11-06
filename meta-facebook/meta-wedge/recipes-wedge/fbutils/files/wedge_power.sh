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

. /usr/local/bin/openbmc-utils.sh

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
    local force opt pulse_us n retries
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
    # start with resetting T2
    reset_brcm.sh
    # first make sure, GPIOD1 (25) is high
    gpio_set 25 1
    sleep 1
    # then, put GPIOP7 (127) to low
    gpio_set 127 0
    pulse_us=500000             # 500ms
    retries=3
    n=1
    while true; do
        # first make sure, GPIOD1 (25) is high
        gpio_set 25 1
        usleep $pulse_us
        # generate the power on pulse
        gpio_set 25 0
        usleep $pulse_us
        gpio_set 25 1
        sleep 3
        if wedge_is_us_on 1 '' 1; then
            break
        fi
        n=$((n+1))
        if [ $n -gt $retries ]; then
            echo " Failed"
            logger "Failed to power on micro-server"
            return 1
        fi
        echo -n "..."
    done
    # Turn on the power LED (GPIOE5)
    /usr/local/bin/power_led.sh on
    echo " Done"
    logger "Successfully power on micro-server"
    return 0
}

do_off() {
    echo -n "Power off microserver ..."
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
    logger "Successfully power off micro-server"
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
        logger "Power reset the whole system ..."
        echo -n "Power reset the whole system ..."
        sleep 1             # give some time for the message to go out
        i2cset -f -y 12 0x10 0xd9 c
    else
        if ! wedge_is_us_on; then
            echo "Power resetting microserver that is powered off has no effect."
            echo "Use '$prog on' to power the microserver on"
            return -1
        fi
        # reset T2 first
        reset_brcm.sh
        echo -n "Power reset microserver ..."
        # then, put GPIOP7 (127) to low
        gpio_set 127 0
        gpio_set 17 0
        sleep 1
        gpio_set 17 1
        sleep 1
        logger "Successfully power reset micro-server"
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
