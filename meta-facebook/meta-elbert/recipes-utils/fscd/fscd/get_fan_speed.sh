#!/bin/sh
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


usage() {
    echo "Usage: $0 [Fan Unit (1..5)]" >&2
}

FCMCPLD="/sys/bus/i2c/drivers/fancpld/6-0060"

show_pwm()
{
    pwm="$FCMCPLD/fan$1_pwm"
    val="$(head -n 1 "$pwm")"
    # Val is [0,255], so let's scale the value down to [0,100]
    float_val=$(echo "$val 100 * 255 / p" | dc)
    rounded="$(printf '%.*f' 0 "$float_val")"
    echo "$rounded%"
}

show_rpm()
{
    rpm="$(head -n 1 "$FCMCPLD"/fan"$1"_input)"
    # Keep the output in the same format as Minipack
    echo "$rpm, $rpm"
}

set -e

# If user input is given, use it as FAN TRAY instance
# Otherwise, read all
if [ "$#" -eq 0 ]; then
    FANS="1 2 3 4 5"
elif [ "$#" -eq 1 ]; then
    if [ "$1" -le 0 ] || [ "$1" -ge 6 ]; then
        echo "Fan $1: Should be between 1 and 5"
        exit 1
    fi
    FANS="$1"
else
    usage
    exit 1
fi


for fan in $FANS; do
    echo "Fan $fan RPMs: $(show_rpm "$fan"), ($(show_pwm "$fan"))"
done
