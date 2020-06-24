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
set -e

if uname -r | grep "4\.1\.*" > /dev/null 2>&1; then
    PWM_SYSFS_DIR="/sys/devices/platform/ast_pwm_tacho.0"

    #
    # Refer to "init_pwm.sh" for more details about fan-tacho mapping.
    #
    FAN_TACHO_MAP="0:3,7 1:2,6 2:0,4 3:1,5"
fi

usage() {
    echo "Usage: $0 [Fan Unit (0..3)]" >&2
}

get_fan_speed_41() {
    fan_id="$1"
    front_tacho="$2"
    rear_tacho="$3"

    front_rpm=$(cat "${PWM_SYSFS_DIR}/tacho${front_tacho}_rpm")
    rear_rpm=$(cat "${PWM_SYSFS_DIR}/tacho${rear_tacho}_rpm")
    echo "Fan $fan_id RPMs: $front_rpm, $rear_rpm"
}

get_fan_speed() {
    fan_id="$1"
    tacho_list="$2"

    front_tacho=${tacho_list%%,*}
    rear_tacho=${tacho_list##*,}
    if uname -r | grep "4\.1\.*" > /dev/null 2>&1; then
        get_fan_speed_41 "$fan_id" "$front_tacho" "$rear_tacho"
    fi
}

# refer to the comments in init_pwn.sh regarding
# the fan unit and tacho mapping
if [ "$#" -eq 0 ]; then
    FAN_TACHOS="$FAN_TACHO_MAP"
elif [ "$#" -eq 1 ]; then
    case "$1" in
    "0")
        FAN_TACHOS=$(echo "$FAN_TACHO_MAP" | cut -d' ' -f1)
        ;;
    "1")
        FAN_TACHOS=$(echo "$FAN_TACHO_MAP" | cut -d' ' -f2)
        ;;
    "2")
        FAN_TACHOS=$(echo "$FAN_TACHO_MAP" | cut -d' ' -f3)
        ;;
    "3")
        FAN_TACHOS=$(echo "$FAN_TACHO_MAP" | cut -d' ' -f4)
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

for fan_tacho in $FAN_TACHOS; do
    fan_id=${fan_tacho%%:*}
    tacho_list=${fan_tacho##*:}

    get_fan_speed "$fan_id" "$tacho_list"
done
