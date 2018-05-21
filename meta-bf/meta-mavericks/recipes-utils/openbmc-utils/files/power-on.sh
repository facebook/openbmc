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

# make power button high to prepare for power on sequence
gpio_set BMC_PWR_BTN_OUT_N 1

#switch COMe tty to dbg port
mav_tty_switch_delay.sh 1

# First power on TH, and if Panther+ is used,
# provide standby power to Panther+.
wedge_power_on_board

echo -n "Checking microserver power status ... "
if wedge_is_us_on 10 "."; then
    echo "on"
    on=1
else
    echo "off"
    on=0
fi

if [ $on -eq 0 ]; then
    # Power on now
    wedge_power.sh on -f
fi

#switch COMe tty to BMC UART after 45 seconds
echo "wait for 45 seconds before connecting to COMe..."
mav_tty_switch_delay.sh 0 45 &

# set the Tofino VDD voltage here before powering-ON COMe
CODE="$(i2cget -f -y 12 0x31 0xb)"
CODE_M=$(($CODE & 0x7))
if [ $CODE_M != 0 ]; then
    tbl=(0 0.83 0.78 0.88 0.755 0.855 0.805 0.905)
    # If not able to access value it is a montara otherwise mavericks
    cat /sys/bus/i2c/drivers/fancpld/9-0033/board_rev >& /dev/null
    if [ $? == 1 ]; then
        btools.py --IR set_vdd_core montara ${tbl[$CODE_M]}
    else
        btools.py --IR set_vdd_core mavericks ${tbl[$CODE_M]}
    fi  
    logger "VDD setting: ${tbl[$CODE_M]}"
fi
