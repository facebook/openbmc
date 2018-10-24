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

# There are 4 busses and 3 fans on each bus. The fans are labeled 1-12 in the
# following order: Bus 171 fan 1,2,3; Bus 179 fan 4,5,6; Bus 187 fan 7,8,9; and
# Bus 195 fan 10,11,12.
usage() {
    echo "Usage: $0 <PERCENT (0..100)>  <Fan ID (1..12)>" >&2
}

set -e

if [ "$#" -ne 2 ] && [ "$#" -ne 1 ]; then
    usage
    exit 1
fi

busses="171 179 187 195"
if [ "$#" -eq 1 ]; then
    ids="1 2 3"
else
    if [ "$2" -le 0 ] || [ "$2" -ge 13 ]; then
        echo "Fan id $2: not a valid id"
        exit 1
    fi
    FAN="$2"
    FAN=$((${FAN} - 1))
    busIndex=$((${FAN} / 3))
    temp=$((${FAN} - ( ${FAN} / 3 * 3 )))
    ids=$((${temp} + 1))
    if [ ${busIndex} -eq 0 ]; then
        busses="171"
    elif [ ${busIndex} -eq 1 ]; then
        busses="179"
    elif [ ${busIndex} -eq 2 ]; then
        busses="187"
    else
        busses="195"
    fi
fi

# Convert the percentage to our 1/32th unit (0-31).
unit=$(( ( $1 * 31 ) / 100 ))

for bus in ${busses}; do
    for id in ${ids}; do
        pwm="/sys/class/i2c-dev/i2c-${bus}/device/${bus}-0033/fantray${id}_pwm"
        echo "$unit" > "${pwm}"
        echo "Successfully set fan ${id} on bus ${bus} speed to $1%"
    done
done
