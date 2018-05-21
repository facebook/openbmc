#!/bin/sh
#
# Copyright 2015-present Facebook. All Rights Reserved.
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
    echo "Usage: $0 [Fan Unit (1..5)]" >&2
}

FAN_DIR=/sys/class/i2c-adapter/i2c-8/8-0033

show_pwm()
{
    pwm="${FAN_DIR}/fantray${1}_pwm"
    val=$(cat $pwm | head -n 1)
    echo "$((val * 100 / 31))%"
}

show_rpm()
{
    front_rpm="${FAN_DIR}/fan$((($1 * 2 - 1)))_input"
    rear_rpm="${FAN_DIR}/fan$((($1 * 2)))_input"
    echo "$(cat $front_rpm), $(cat $rear_rpm)"
}

set -e

# refer to the comments in init_pwn.sh regarding
# the fan unit and tacho mapping
if [ "$#" -eq 0 ]; then
    FANS="1 2 3 4 5"
elif [ "$#" -eq 1 ]; then
    if [ $1 -gt 5 ]; then
        usage
        exit 1
    fi
    FANS="$1"
else
    usage
    exit 1
fi

for fan in $FANS; do
    echo "Fan $fan RPMs: $(show_rpm $fan), ($(show_pwm $fan))"
done
