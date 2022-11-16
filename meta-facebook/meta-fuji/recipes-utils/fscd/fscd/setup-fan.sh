#!/bin/bash
#
# Copyright 2021-present Facebook. All Rights Reserved.
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

# shellcheck disable=SC1091
source /usr/local/bin/openbmc-utils.sh

PATH=/sbin:/bin:/usr/sbin:/usr/bin:/usr/local/bin

# shellcheck disable=SC2034
default_fsc_config="/etc/fsc-config.json"

echo "Setting fan speed to 50%..."
/usr/local/bin/set_fan_speed.sh 50
echo "Done setting fan speed"

fscd_enable=1

board_ver=$(wedge_board_rev)
if [ "$board_ver" == "BOARD_FUJI_EVT1" ] || \
[ "$board_ver" == "BOARD_FUJI_EVT2" ] || \
[ "$board_ver" == "BOARD_FUJI_EVT3" ]; then
    fscd_enable=0
    echo "For EVT board, Setting fan speed to 70%..."
    /usr/local/bin/set_fan_speed.sh 70
fi

if [ $fscd_enable -eq 0 ]; then
    echo "Running fan at the fixed speed."
else
    echo "Starting fscd..."
    runsv /etc/sv/fscd > /dev/null 2>&1 &
fi
echo "done."
