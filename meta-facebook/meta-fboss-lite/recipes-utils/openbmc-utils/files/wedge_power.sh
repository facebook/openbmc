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

# shellcheck disable=SC1091
. /usr/local/bin/openbmc-utils.sh

PATH=/sbin:/bin:/usr/sbin:/usr/bin:/usr/local/bin

prog="$0"

usage() {
    echo "Usage: $prog <command> [command options]"
    echo
    echo "Commands:"
    echo "  status: Get the current x86 (userver) power status"
    echo
    echo "  on: Power on x86 (userver) if not powered on already"
    echo "    options:"
    echo "      -f: Re-do power on sequence no matter if x86 (userver) has "
    echo "          been powered on or not."
    echo
    echo "  off: Power off x86 (userver) ungracefully"
    echo
    echo "  reset: Power reset x86 (userver) ungracefully"
    echo "    options:"
    echo "      -s: Power cycle the whole chassis ungracefully"
    echo
    echo
}

do_status() {
    echo -n "Microserver power is "

    if userver_power_is_on; then
        echo "on"
    else
        echo "off"
    fi

    return 0
}

do_on() {
    local opt
    local force=0

    while getopts "f" opt; do
        case $opt in
            f)
                force=1
                ;;
            *)
                usage
                exit 1
                ;;
        esac
    done

    if [ $force -eq 0 ]; then
        if userver_power_is_on; then
            echo " uServer is already on. Skipped."
            return 0
        fi
    fi

    logger -p user.crit "Power on x86 (userver) ..."
    echo -n "Power on x86 (userver) ..."
    if userver_power_on; then
        echo " Done"
        return 0
    fi

    echo " Failed"
    return 1
}

do_off() {
    logger -p user.crit "Power off x86 (userver) ..."
    echo -n "Power off x86 (userver) ..."

    if userver_power_off; then
        echo " Done"
        return 0
    fi

    echo " Failed"
    return 1
}

do_reset() {
    local opt
    local system=0

    while getopts "st:" opt; do
        case $opt in
            s)
                system=1
                ;;
            *)
                usage
                exit 1
                ;;
        esac
    done

    if [ $system -eq 1 ]; then
        logger -p user.crit "Power reset the whole system ..."
        echo  "Power reset the whole system ..."

        sync

        chassis_power_cycle

        sleep 3
        # Control should not reach here!!!
        logger -p user.crit "Failed to reset the system!!!"
        echo "Failed to reset the system!!!"
        exit 1
    else
        if userver_power_is_on; then
            logger -p user.crit "Power reset x86 (userver) ..."
            echo -n "Power reset x86 (userver) ..."

            userver_reset
            echo " Done"
        else
            echo "x86 (userver) is off. Power on x86 now.."
            do_on
        fi
    fi
}

if [ $# -lt 1 ]; then
    usage
    exit 1
fi

command="$1"
shift

case "$command" in
    status)
        do_status "$@"
        ;;
    on)
        do_on "$@"
        ;;
    off)
        do_off "$@"
        ;;
    reset)
        do_reset "$@"
        ;;
    *)
        usage
        exit 1
        ;;
esac

exit $?
