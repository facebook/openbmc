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

# shellcheck disable=SC1091,SC2039
. /usr/local/fbpackages/utils/ast-functions

echo slave-mqueue 0x1010 > /sys/bus/i2c/devices/i2c-3/new_device  #BIC
echo slave-mqueue 0x1010 > /sys/bus/i2c/devices/i2c-4/new_device  #NIC

#Set NIC1 EID to 0x22
/usr/local/bin/mctp-util -d 0x4 0x64 0x00 0x00 0x80 0x01 0x00 0x22


echo -n "Starting MCTPD Rx/Tx Daemon.."
ulimit -q 1024000
runsv /etc/sv/mctpd_3 > /dev/null 2>&1 &
runsv /etc/sv/mctpd_4 > /dev/null 2>&1 &


echo "done."

