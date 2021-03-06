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

. /usr/local/bin/openbmc-utils.sh

fcm_b_ver=`head -n1 ${BOTTOM_FCMCPLD_SYSFS_DIR}/cpld_ver 2> /dev/null`
fcm_t_ver=`head -n1 ${TOP_FCMCPLD_SYSFS_DIR}/cpld_ver 2> /dev/null`

usage() {
    echo "Usage: $0 <PERCENT (0..100)> <Fan Unit (1..8)> " >&2
}

set -e

if [ "$#" -ne 2 ] && [ "$#" -ne 1 ]; then
    usage
    exit 1
fi

# Top FCM    (BUS=64) PWM 1 ~ 4 control Fantray 1 3 5 7
# Bottom FCM (BUS=72) PWM 1 ~ 4 control Fantray 2 4 6 8
BUS="64 72"
if [ "$#" -eq 1 ]; then
    FAN_UNIT="1 2 3 4"
else
    if [ "$2" -le 0 ] || [ "$2" -ge 9 ]; then
        echo "Fan $2: not a valid Fan Unit"
        exit 1
    fi

    FAN="$2"

    if [ $(($FAN % 2)) -eq 0 ]; then
      BUS="72"
      FAN_UNIT=$((${FAN} / 2 ))
    else
      BUS="64"
      FAN=$((${FAN} / 2 ))
      FAN_UNIT=$((${FAN} + 1))
    fi
fi

if [ "$fcm_b_ver" == "0x0" ] || [ "$fcm_t_ver" == "0x0" ]; then
    # Convert the percentage to our 1/32th level (0-31).
    step=$((($1 * 31) / 100))
else
    # Convert the percentage to our 1/64th level (0-63).
    step=$((($1 * 63) / 100))
fi
cnt=1

for bus in ${BUS}; do
    for unit in ${FAN_UNIT}; do
        pwm="$(i2c_device_sysfs_abspath ${bus}-0033)/fantray${unit}_pwm"
        echo "$step" > "${pwm}"

    if [ "$#" -eq 1 ]; then
        echo "Successfully set fan $cnt speed to $1%"
        cnt=$(($cnt+1))
    else
        echo "Successfully set fan $2 speed to $1%"
    fi
    done
done
