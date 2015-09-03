#!/bin/sh
#
# Copyright 2014-present Facebook. All Rights Reserved.
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
. /usr/local/bin/openbmc-utils.sh

PATH=/sbin:/bin:/usr/sbin:/usr/bin:/usr/local/bin

echo -n "Checking microserver power status ... "
if wedge_is_us_on 10 "."; then
    echo "on"
    on=1
else
    echo "off"
    on=0
fi

if [ $on -eq 0 ]; then
    # Reset USB hub
    /usr/local/bin/reset_usb.sh
    # Set up ROV
    /usr/local/bin/setup_rov.sh
    # Configure the management switch on LEFT side of FC.
    # Must do so after setup_rov.sh, as setup_rov.sh might reset
    # T2, which also resets the switch.
    if [ "$(wedge_board_type)" = "FC-LEFT" ]; then
        echo -n "Configure management switch ... "
        if [ $(wedge_board_rev) -gt 1 ]; then
            echo "skip. Need new switch setup script!!!"
        else
            # configure MDIO as GPIO output
            gpio_set A6 1
            gpio_set A7 1
            # set the switch to be configured by CPU
            gpio_set E2 1
            # set the switch to be configured by CPU, then reset the switch,
            # which also cause T2 reset in the current board
            gpio_set E2 1
            gpio_set C0 0
            sleep 1
            gpio_set C0 1
            # configure the switch
            setup_switch.py
            echo "done"
        fi
    fi
    # Power on now
    wedge_power.sh on -f
fi
