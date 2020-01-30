#!/bin/bash
#
# Copyright 2020-present Facebook. All Rights Reserved.
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

# ELBERTTODO 442074 wedge_power.sh
# . /usr/local/bin/openbmc-utils.sh

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
    echo "      -s: Power reset whole elbert system ungracefully"
    echo
    echo "  lcreset: Power-cycle one or all LC(s)"
    echo "    options:"
    echo "      -a  : Reset all LCs or "
    echo "      -1 , -2 , ... , -8 : Reset a single LC (1, 2 ... 8) "
    echo
}

do_status() {
   echo "WEDGE_POWER do_status NOT IMPLEMENTED YET"
   exit 1
}

do_on() {
    local force opt ret
    force=0
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
    echo -n "Power on microserver ..."
    if [ $force -eq 0 ]; then
        # need to check if uS is on or not
        if wedge_is_us_on; then
            echo " Already on. Skip!"
            return 1
        fi
    fi

    wedge_power_on_board
    ret=$?
    if [ $ret -eq 0 ]; then
        echo " Done"
        logger "Successfully power on micro-server"
    else
        echo " Failed"
        logger "Failed to power on micro-server"
    fi
    return $ret
}

do_off() {
   echo "WEDGE_POWER do_off NOT IMPLEMENTED YET"
   exit 1
}

do_reset() {
   echo "WEDGE_POWER NOT IMPLEMENTED YET"
   exit 1

}

toggle_pim_reset() {
   echo "PIM RESET NOT IMPLEMENTED YET"
   exit 1
}

do_pimreset() {
   echo "PIM RESET NOT IMPLEMENTED YET"
   exit 1
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
    pimreset)
        do_pimreset "$@"
        ;;
    lcreset)
        do_pimreset "$@"
        ;;
    *)
        usage
        exit 1
        ;;
esac

exit $?
