#!/bin/sh
#
# Copyright 2018-present Facebook. All Rights Reserved.
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

# YAMP get_fan : the fan configuration of YAMP is totally different 
# from Minipack (1x5 vs 2x4), therefore logic for reading fan RPM
# is also very different, but the output of this function will be
# of the same format as Minipack, so as to reuse the existing code
# as much as possible
usage() {
    echo "Usage: $0 [Fan Unit (1..5)]" >&2
}

FCMCPLD="/sys/bus/i2c/drivers/fancpld/13-0060"

show_pwm()
{
    pwm="$FCMCPLD/fan$1_pwm"
    val=$(cat $pwm | head -n 1)
    # Val is [0,255], so let's scale the value down to [0,100]
    float_val=$(echo "$val 100 * 255 / p" | dc)
    rounded=`printf '%.*f' 0 $float_val`
    echo "$rounded%"
}

show_rpm()
{
    rpm=`cat $FCMCPLD/fan$1_input|head -n 1`
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
    echo "Fan $fan RPMs: $(show_rpm $fan), ($(show_pwm $fan))"
done
