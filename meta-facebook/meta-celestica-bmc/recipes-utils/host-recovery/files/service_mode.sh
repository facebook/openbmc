#!/bin/sh
#
# Copyright (c) Meta Platforms, Inc. and affiliates. (http://www.meta.com)
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

usage() {
    echo "Usage: $0 <command>"
    echo
    echo "Commands:"
    echo "  status: Get the current service mode status"
    echo
    echo "  on: to enable service mode"
    echo
    echo "  off: to disable service mode"
    echo
}

service_mode_is_on() {
    # Check if service mode is on
    if [ -f /tmp/service_mode ]; then
        return 0
    fi
    # Return 0 if service mode is on, 1 otherwise
    return 1
}

service_mode_on() {
    if service_mode_is_on; then
        echo "No action, Service mode is already on"
        exit 1
    fi
    
    # Enable service mode
    echo "Enable service mode "

    touch /tmp/service_mode
    # Create symlinks for host-recovery tools except service_mode.sh
    for tool in /usr/local/fbpackages/host-recovery/*; do
        if [ "$(basename "$tool")" != "service_mode.sh" ]; then
            ln -s "$tool" "/usr/local/bin/$(basename "$tool")"
        fi
    done
}

service_mode_off() {
    if ! service_mode_is_on; then
        echo "No action, Service mode is already off"
        exit 1
    fi

    # Disable service mode
    echo "Disable service mode"

    # Remove symlinks for host-recovery tools except service_mode.sh
    for tool in /usr/local/fbpackages/host-recovery/*; do
        if [ "$(basename "$tool")" != "service_mode.sh" ]; then
            rm -f "/usr/local/bin/$(basename "$tool")"
        fi
    done

    rm -rf /tmp/service_mode
}

if [ $# -lt 1 ]; then
    usage
    exit 1
fi

command="$1"
case "$command" in
    status)
        echo "Service mode is $(service_mode_is_on && echo "on" || echo "off")"
        ;;
    on)
        service_mode_on
        ;;
    off)
        service_mode_off
        ;;
    *)
        usage
        exit 1
        ;;
esac

exit $?