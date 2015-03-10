#!/bin/sh
#
# Copyright 2004-present Facebook. All Rights Reserved.
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
    echo "Usage: $0 [Fan Unit (0..3)]" >&2
}

PWM_DIR=/sys/devices/platform/ast_pwm_tacho.0
set -e

# refer to the comments in init_pwn.sh regarding
# the fan unit and tacho mapping
if [ "$#" -eq 0 ]; then
    TACHOS="0:3 1:2 2:0 3:1"
elif [ "$#" -eq 1 ]; then
    case "$1" in
    "0")
        TACHOS="0:3"
        ;;
    "1")
        TACHOS="1:2"
        ;;
    "2")
        TACHOS="2:0"
        ;;
    "3")
        TACHOS="3:1"
        ;;
    *)
        usage
        exit 1
        ;;
    esac
else
    usage
    exit 1
fi

for fan_tacho in $TACHOS; do
    fan=${fan_tacho%%:*}
    tacho=${fan_tacho##*:}
    echo "Fan $fan RPMs:    $(cat $PWM_DIR/tacho${tacho}_rpm), $(cat $PWM_DIR/tacho$((tacho+4))_rpm)"
done
