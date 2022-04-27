#!/bin/bash
#
# Copyright 2021-present Facebook. All Rights Reserved.
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
    echo "  status: Get the current microserver power status"
    echo
    echo "  reset: Power reset microserver ungracefully"
    echo "    options:"
    echo "      -s: Power reset whole fbdarwin system ungracefully"
    echo
}

do_status() {
    # FBDARWIN doesn't control CPU power, always on
    echo "Microserver power is on"
    return 0
}

do_on() {
   return 0
}

do_off() {
   echo "FBDARWIN doesn't support SCM power control"
   return 1
}

do_sync() {
   # Try to flush all data to MMC and remount the drive read-only. Sync again
   # because the remount is asynchronous.
   partition="/dev/mmcblk0"
   mountpoint="/mnt/data1"

   sync
   # rsyslogd logs to eMMC, so we must stop it prior to remouting.
   kill -TERM "$(cat /var/run/rsyslogd.pid)"
   sleep .05
   if [ -e $mountpoint ]; then
      retry_command 5 mount -o remount,ro $partition $mountpoint
      sleep 1
   fi
   sync
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
                exit 1
                ;;
        esac
    done
    if [ $system -eq 1 ]; then
        do_sync
        gpio_set_value BMC_SYS_PWR_CYC0 1
        sleep 8
        # The chassis shall be reset now... if not, we are in trouble
        echo " Failed"
        gpio_set_value BMC_SYS_PWR_CYC0 0
        return 254
    else
        echo "FBDARWIN doesn't support SCM reset"
        return 1
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
