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
    echo "Usage: $0 <PERCENT (0..100)> <Fan Unit (1..5)(6...10)> <board type (Montara Mavericks)>" >&2
}

FAN_DIR=/sys/class/i2c-adapter/i2c-8/8-0033
FAN_DIR_UPPER=/sys/class/i2c-adapter/i2c-9/9-0033

set -e

if [ "$#" -gt 3 ] || [ "$#" -lt 1 ]; then
    usage
    exit 1
fi

# Adding fix for Mavericks and Montara while keeping backward compatibility with FB command line.
if [ "$#" -eq 1 ]; then
    FANS="1 2 3 4 5"
elif [ "$#" -eq 2 ]; then
    if [ $2 = "Mavericks" ]; then
    	FANS="1 2 3 4 5 6 7 8 9 10"
    elif [ $2 = "Montara" ]; then
    	FANS="1 2 3 4 5"
    else 
    	if [ $2 -gt 10 ]; then
       	    usage
            exit 1
    	fi
    	FANS="$2"
    fi
else
    if [ $3 = "Mavericks" ]; then
    	if [ $2 -gt 10 ]; then
       	    usage
            exit 1
    	fi
    elif [ $3 = "Montara" ]; then
    	if [ $2 -gt 5 ]; then
       	    usage
            exit 1
    	fi
    else 
        usage
        exit 1
    fi
    FANS="$2"
fi

# Convert the percentage to our 1/32th unit (0-31).
unit=$(( ( $1 * 31 ) / 100 ))

for fan in $FANS; do
    if [ $fan -gt 5 ]; then
        fan_idx=$(( $fan - 5))
        pwm="${FAN_DIR_UPPER}/fantray${fan_idx}_pwm"
    else
        pwm="${FAN_DIR}/fantray${fan}_pwm"
    fi
    echo "$unit" > $pwm
    echo "Successfully set fan ${fan} speed to $1%"
done
