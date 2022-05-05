#!/bin/bash
#
# Copyright (c) 2022 Cisco Systems Inc.
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
#

#shellcheck disable=SC1091
. /usr/local/bin/openbmc-utils.sh

prog="$0"

usage() {
    echo "Usage: $prog <command> [command options]"
    echo
    echo "Commands:"
    echo "  status: Get the current x86 power status"
    echo
    echo "  on: Power on x86 if not powered on already"
    echo "    options:"
    echo "      -f: Re-do power on sequence no matter if x86 has "
    echo "          been powered on or not."
    echo
    echo "  off: Power off x86 ungracefully"
    echo
    echo "  reset: Power reset x86 ungracefully"
    echo
    echo
}

do_status() {
    cat /sys/bus/platform/devices/pseq/power_state
}

do_on_com_e() {
    #Commands to turn x86 on
    PDEV=/sys/bus/platform/devices
    if [ -w ${PDEV}/pseq/config ]; then
        echo on > ${PDEV}/pseq/config
    else
        i2cset -f -y 2 0x21 0x8c
        i2cset -f -y 2 0x22 0x40
        i2cset -f -y 2 0x23 0x01
        i2cset -f -y 2 0x24 0x00
        i2cset -f -y 2 0x25 0x02
        i2cset -f -y 2 0x20 0x0
    fi
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
                exit 1
                ;;
        esac
    done
    echo -n "Power on x86 ..."
    if [ $force -eq 0 ]; then
        # need to check if power is on or not
        do_status | grep "on"
        if [ $? == 0 ]; then
            echo " Skip!"
            return 1
        fi
    fi

    do_on_com_e
    echo " Done"
}

do_off_com_e() {
    PDEV=/sys/bus/platform/devices
    if [ -w ${PDEV}/pseq/config ]; then
        echo off > ${PDEV}/pseq/config
    else
        i2cset -f -y 2 0x21 0x8c
        i2cset -f -y 2 0x22 0x40
        i2cset -f -y 2 0x23 0x01
        i2cset -f -y 2 0x24 0x00
        i2cset -f -y 2 0x25 0x04
        i2cset -f -y 2 0x20 0x0
    fi
}

do_off() {
    echo -n "Power off x86 ..."
    do_off_com_e
    echo " Done"
}

do_reset_com_e() {
    #Commands to reboot x86
    PDEV=/sys/bus/platform/devices
    if [ -w ${PDEV}/pseq/config ]; then
        echo cycle > ${PDEV}/pseq/config
    else
        i2cset -f -y 2 0x21 0x8c
        i2cset -f -y 2 0x22 0x40
        i2cset -f -y 2 0x23 0x01
        i2cset -f -y 2 0x24 0x00
        i2cset -f -y 2 0x25 0x08
        i2cset -f -y 2 0x20 0x0
    fi
}

do_reset() {
    echo -n "Reset x86 ..."
    do_reset_com_e
    echo " Done"
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
