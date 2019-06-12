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

echo slave-mqueue 0x1010 > /sys/bus/i2c/devices/i2c-14/new_device  #USB DBG
echo slave-mqueue 0x1010 > /sys/bus/i2c/devices/i2c-1/new_device  #MB1
echo slave-mqueue 0x1010 > /sys/bus/i2c/devices/i2c-2/new_device  #MB2
echo slave-mqueue 0x1010 > /sys/bus/i2c/devices/i2c-3/new_device  #MB3
echo slave-mqueue 0x1010 > /sys/bus/i2c/devices/i2c-4/new_device  #MB4

ulimit -q 1024000
runsv /etc/sv/ipmbd_14 > /dev/null 2>&1 &
runsv /etc/sv/ipmbd_1 > /dev/null 2>&1 &
runsv /etc/sv/ipmbd_2 > /dev/null 2>&1 &
runsv /etc/sv/ipmbd_3 > /dev/null 2>&1 &
runsv /etc/sv/ipmbd_4 > /dev/null 2>&1 &

echo "done."

