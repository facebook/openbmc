#!/bin/bash
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

### BEGIN INIT INFO
# Provides:          setup-fan
# Required-Start:    board-id
# Required-Stop:
# Default-Start:     S
# Default-Stop:
# Short-Description: Set fan speed
### END INIT INFO

. /usr/local/fbpackages/utils/ast-functions
. /usr/local/bin/openbmc-utils.sh

PATH=/sbin:/bin:/usr/sbin:/usr/bin:/usr/local/bin

default_fsc_config="/etc/fsc-config.json"

echo -n "Setup fan speed... "

echo "Run FSC AGC032A Config"
cp /etc/FSC-AGC032A-config.json ${default_fsc_config}

echo "Setting fan speed to 50%..."
/usr/local/bin/set_fan_speed.sh 50 all both
echo "Done setting fan speed"

# Execute fscd
fcm_compatible=1


if [ $fcm_compatible -eq 0 ]; then
    echo "Running fan at the fixed speed."
    /usr/local/bin/wdtcli stop
else
    echo "Starting fscd..."
    runsv /etc/sv/fscd > /dev/null 2>&1 &
fi
echo "done."
