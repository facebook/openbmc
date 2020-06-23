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
    # Refer to "init_pwm.sh" for more details about pwm configuration as
    # well as fan-pwm mapping.
    #
    PWM_UNIT_MAX=96
    FAN_PWM_MAP="0:7 1:6 2:0 3:1"
fi

usage() {
    echo "Usage: $0 <PERCENT (0..100)> <Fan Unit (0..3)> " >&2
}

set_fan_pwm_41() {
    pwm_id="$1"
    unit="$2"

    if [ "$unit" -eq 0 ]; then
        # For 0%, turn off the PWM entirely
        echo 0 > "$PWM_SYSFS_DIR/pwm${pwm_id}_en"
    else
        if [ "$unit" -eq $PWM_UNIT_MAX ]; then
            # For 100%, set falling and rising to the same value
            unit=0
        fi

        # always use type M. refer to the comments in init_pwm.sh
        echo 0 > "$PWM_SYSFS_DIR/pwm${pwm_id}_type"
        echo 0 > "$PWM_SYSFS_DIR/pwm${pwm_id}_rising"
        echo "$unit" > "$PWM_SYSFS_DIR/pwm${pwm_id}_falling"
        echo 1 > "$PWM_SYSFS_DIR/pwm${pwm_id}_en"
    fi
}

set_fan_speed() {
    pwm_id="$1"
    percent="$2"
    unit=$((percent * PWM_UNIT_MAX / 100))

    if uname -r | grep "4\.1\.*" > /dev/null 2>&1; then
        set_fan_pwm_41 "$pwm_id" "$unit"
    fi
}

if [ "$#" -ne 2 ] && [ "$#" -ne 1 ]; then
    usage
    exit 1
fi
PERCENT="$1"
FAN_ID="$2"

# refer to the comments in init_pwn.sh regarding
# the fan unit and PWM mapping
if [ "$#" -eq 1 ]; then
    FAN_PWMS="${FAN_PWM_MAP}"
else
    case "$FAN_ID" in
    "0")
        FAN_PWMS=$(echo "$FAN_PWM_MAP" | cut -d' ' -f1)
        ;;
    "1")
        FAN_PWMS=$(echo "$FAN_PWM_MAP" | cut -d' ' -f2)
        ;;
    "2")
        FAN_PWMS=$(echo "$FAN_PWM_MAP" | cut -d' ' -f3)
        ;;
    "3")
        FAN_PWMS=$(echo "$FAN_PWM_MAP" | cut -d' ' -f4)
        ;;
    *)
        usage
        exit 1
        ;;
    esac
fi

for fan_pwm in $FAN_PWMS; do
    fan_id=${fan_pwm%%:*}
    pwm_id=${fan_pwm##*:}

    set_fan_speed "$pwm_id" "$PERCENT"
    echo "Successfully set fan ${fan_id} (PWM: $pwm_id) speed to $percent%"
done
