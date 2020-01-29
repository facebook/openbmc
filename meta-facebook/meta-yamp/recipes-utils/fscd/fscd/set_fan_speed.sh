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

# YAMP fan set utility for fscd.

usage() {
  echo "Usage: $0 <PWM_PERCENT> [FAN_TRAY] " 
  echo "<PWM_PERCENT> : From 0 to 100 " 
  echo "[FAN_TRAY] : 1, 2, 3, 4 or 5." 
  echo "If no FAN_TRAY is specified, all FANs will be programmed"
}

set -e

# Make sure we only have 1 or 2 arguments
if [ "$#" -ne 2 ] && [ "$#" -ne 1 ]; then
    usage
    exit 1
fi

# Also make sure that percent value is really between 0 and 100
if [ "$1" -lt 0 ] || [ "$1" -gt 100 ]; then
    echo "PWM_PERCENT $1: should be between 0 and 100"
    exit 1
fi

# Read FAN_TRAY value from user argument. 
# If no FAN_TRAY is given, configure all FANs 
if [ "$#" -eq 1 ]; then
    FAN_UNIT="1 2 3 4 5"
else
    if [ "$2" -le 0 ] || [ "$2" -ge 6 ]; then
        echo "Fan $2: should be between 1 and 5"
        exit 1
    fi
    FAN_UNIT="$2"
fi

# Convert the percentage to our 1/32th level (0-31).
# The user input is given in percent (0-100), 
# but YAMP FANCPLD takes PWM as 8bit value (0-255)
# So scale the value by multiplying 2.5
float_step=$(echo "$1 255 * 100 / p" | dc)
step=`printf '%.*f' 0 $float_step`

for unit in ${FAN_UNIT}; do
    pwm="/sys/bus/i2c/drivers/fancpld/13-0060/fan${unit}_pwm"
    # echo "$step" > "${pwm}"
    echo "$step will be set to ${pwm} !!!!"
    echo "Successfully set fan ${unit} speed to $1%"
done
