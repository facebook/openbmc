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

### BEGIN INIT INFO
# Provides:          power-on
# Required-Start:
# Required-Stop:
# Default-Start:     S
# Default-Stop:
# Short-Description: Power on micro-server
### END INIT INFO

# shellcheck disable=SC1091
. /usr/local/bin/openbmc-utils.sh

PATH=/sbin:/bin:/usr/sbin:/usr/bin:/usr/local/bin

# Only run board power up sequence is CPU is not up, otherwise this could
# disrupt data plane
if wedge_is_us_on; then
    echo "uServer is already up. Skipping board power-on sequence"
else
    echo "uServer is not up. Running board power-on sequence"
    wedge_power_on_board
fi

# Now, double check if power is on, and do forced power up as needed.
echo "Checking microserver power status ... "
if wedge_is_us_on; then
    echo "on"
    on=1
else
    echo "off"
    on=0
fi

if [ $on -eq 0 ]; then
    # Power on now
    echo "uServer still not up. Running forced power-up sequence"
    wedge_power.sh on -f
fi

# ELBERTTODO LC Support
# pim_enable.sh > /dev/null 2>&1 &
