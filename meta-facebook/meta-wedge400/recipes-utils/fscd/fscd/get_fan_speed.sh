#!/bin/sh
#
# Copyright 2019-present Facebook. All Rights Reserved.
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

PATH=/sbin:/bin:/usr/sbin:/usr/bin:/usr/local/bin

fcb_ver=`head -n1 $FCMCPLD_SYSFS_DIR/cpld_ver \
         2> /dev/null`

usage() {
    echo "Usage: $0 [Fan Unit (1..4)]" >&2
}

# FCB    PWM 1 ~ 4 control Fantray 1 2 3 4
show_pwm()
{
    pwm="$FCMCPLD_SYSFS_DIR/fan$1_pwm"
    val=$(cat $pwm | head -n 1)

    if [ "$fcb_ver" == "0x0" ]; then
        echo "$((val * 100 / 31))%"
    else
        # Convert the percentage to our 1/64th level (0-63) and round.
        echo "$(( (val * 1000 / 63 + 5) /10 ))%"
    fi
}

show_rpm()
{
    front_rpm="$FCMCPLD_SYSFS_DIR/fan$((($1 * 2 - 1)))_input"
    rear_rpm="$FCMCPLD_SYSFS_DIR/fan$((($1 * 2)))_input"
    echo "$(cat $front_rpm), $(cat $rear_rpm)"
}

set -e

# refer to the comments in init_pwn.sh regarding
# the fan unit and tacho mapping
if [ "$#" -eq 0 ]; then
    FANS="1 2 3 4"
elif [ "$#" -eq 1 ]; then
    if [ "$1" -le 0 ] || [ "$1" -ge 9 ]; then
        echo "Fan $1: not a valid Fan Unit"
        exit 1
    fi
    FANS="$1"
else
    usage
    exit 1
fi


for fan in $FANS; do
    FAN="${fan}"
    UNIT=${FAN}

    echo "Fan $fan RPMs: $(show_rpm $UNIT), ($(show_pwm $UNIT))"
done
