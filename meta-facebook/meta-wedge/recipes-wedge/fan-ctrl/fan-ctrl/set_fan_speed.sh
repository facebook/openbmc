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
    echo "Usage: $0 <PERCENT (0..100)> <Fan Unit (0..3)> " >&2
}

PWM_DIR=/sys/devices/platform/ast_pwm_tacho.0

# The maximum unit setting.
# This should be the value in pwm_type_m_unit plus 1
PWM_UNIT_MAX=96

set -e

if [ "$#" -ne 2 ] && [ "$#" -ne 1 ]; then
    usage
    exit 1
fi

# refer to the comments in init_pwn.sh regarding
# the fan unit and PWM mapping
if [ "$#" -eq 1 ]; then
    PWMS="0:7 1:6 2:0 3:1"
else
    case "$2" in
    "0")
        PWMS="0:7"
        ;;
    "1")
        PWMS="1:6"
        ;;
    "2")
        PWMS="2:0"
        ;;
    "3")
        PWMS="3:1"
        ;;
    *)
        usage
        exit 1
        ;;
    esac
fi

# Convert the percentage to our 1/96th unit.
unit=$(( ( $1 * $PWM_UNIT_MAX ) / 100 ))

for FAN_PWM in $PWMS; do
    FAN_N=${FAN_PWM%%:*}
    PWM_N=${FAN_PWM##*:}
    if [ "$unit" -eq 0 ]; then
        # For 0%, turn off the PWM entirely
        echo 0 > $PWM_DIR/pwm${PWM_N}_en
    else
        if [ "$unit" -eq $PWM_UNIT_MAX ]; then
            # For 100%, set falling and rising to the same value
            unit=0
        fi

        # always use type M. refer to the comments in init_pwm.sh
        echo 0 > $PWM_DIR/pwm${PWM_N}_type
        echo 0 > $PWM_DIR/pwm${PWM_N}_rising
        echo "$unit" > $PWM_DIR/pwm${PWM_N}_falling
        echo 1 > $PWM_DIR/pwm${PWM_N}_en
    fi

    echo "Successfully set fan ${FAN_N} (PWM: $PWM_N) speed to $1%"
done
