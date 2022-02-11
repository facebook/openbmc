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

. /usr/local/bin/openbmc-utils.sh
echo -n "Starting IPMB Rx/Tx Daemon.."

i2c_mslave_add 0 0x17 #MB
i2c_mslave_add 8 0x10 #USB DBG
i2c_mslave_add 1 0x10  #NIC0
i2c_mslave_add 9 0x10  #NIC1
i2c_mslave_add 2 0x10  #NIC2
i2c_mslave_add 10 0x10 #NIC3
i2c_mslave_add 4 0x10  #NIC4
i2c_mslave_add 11 0x10 #NIC5
i2c_mslave_add 7 0x10  #NIC6
i2c_mslave_add 13 0x10 #NIC7

ulimit -q 1024000
runsv /etc/sv/ipmbd_8 > /dev/null 2>&1 &
runsv /etc/sv/ipmbd_0 > /dev/null 2>&1 &

echo "done."
