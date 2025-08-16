#!/bin/bash
#
# Copyright 2025-present Facebook. All Rights Reserved.
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

# x86 (userver) power status, power on/off is no-op in Darwin as the x86 is always powered-on
do_status() {
    echo "Microserver power is on"
    return 0
}

do_on() {
    echo "Done"
    return 0
}

do_off() {
    echo "FBDARWIN doesn't support SCM power off!"
    echo "Failed"
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
        echo  "Power reset the whole system ..."

        /usr/libexec/phosphor-state-manager/chassis-powercycle

        echo "Failed to reset the system!!!"
        exit 1
    else
        echo "Power reset x86 (userver) ..."

        if ! /usr/libexec/phosphor-state-manager/host-powercycle ; then
            echo "Failed to reset x86 (userver)!!!"
            exit 1
        fi

        echo " Done"
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
