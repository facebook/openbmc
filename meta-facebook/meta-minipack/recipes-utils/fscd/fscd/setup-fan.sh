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

### BEGIN INIT INFO
# Provides:          setup-fan
# Required-Start:    board-id
# Required-Stop:
# Default-Start:     S
# Default-Stop:
# Short-Description: Set fan speed
### END INIT INFO

. /usr/local/fbpackages/utils/ast-functions

default_fsc_config="/etc/fsc-config.json"
fcm_b_ver=`head -n1 /sys/class/i2c-adapter/i2c-72/72-0033/cpld_ver \
           2> /dev/null`
fcm_t_ver=`head -n1 /sys/class/i2c-adapter/i2c-64/64-0033/cpld_ver \
           2> /dev/null`

echo -n "Setup fan speed... "

if [ "$fcm_b_ver" == "0x0" ] || [ "$fcm_t_ver" == "0x0" ]; then
    echo "Run FSC PWM 32 Levels Config"
    cp /etc/FSC-PWM-32-config.json ${default_fsc_config}
else
    echo "Run FSC PWM 64 Levels Config"
    cp /etc/FSC-PWM-64-config.json ${default_fsc_config}
fi

/usr/local/bin/set_fan_speed.sh 50
runsv /etc/sv/fscd > /dev/null 2>&1 &
echo "done."
