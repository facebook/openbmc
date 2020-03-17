#! /bin/sh
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

### BEGIN INIT INFO
# Provides:          ipmbd
# Required-Start:
# Required-Stop:
# Default-Start:     S
# Default-Stop:      0 6
# Short-Description: Provides ipmb message tx/rx service
#
### END INIT INFO

. /usr/local/fbpackages/utils/ast-functions
echo -n "Starting IPMB Rx/Tx Daemon.."

addr=$((0x1010 | $(gpio_get FM_BLADE_ID_1)<<1 | $(gpio_get FM_BLADE_ID_0)))

echo slave-mqueue 0x1010 > /sys/bus/i2c/devices/i2c-0/new_device  #USB DBG
echo slave-mqueue $addr > /sys/bus/i2c/devices/i2c-2/new_device   #Slave BMC
echo slave-mqueue 0x1010 > /sys/bus/i2c/devices/i2c-5/new_device  #ME
echo slave-mqueue 0x1010 > /sys/bus/i2c/devices/i2c-6/new_device  #JG7
echo slave-mqueue 0x1010 > /sys/bus/i2c/devices/i2c-8/new_device  #CM

ulimit -q 1024000
runsv /etc/sv/ipmbd_0 > /dev/null 2>&1 &
runsv /etc/sv/ipmbd_2 > /dev/null 2>&1 &
runsv /etc/sv/ipmbd_5 > /dev/null 2>&1 &
runsv /etc/sv/ipmbd_6 > /dev/null 2>&1 &
runsv /etc/sv/ipmbd_8 > /dev/null 2>&1 &

echo "done."

