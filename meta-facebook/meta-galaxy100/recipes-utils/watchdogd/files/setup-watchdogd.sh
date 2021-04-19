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
#

### BEGIN INIT INFO
# Provides:          setup-watchdogd
# Required-Start:
# Required-Stop:
# Default-Start:     S
# Default-Stop:
# Short-Description: Set watchdogd handler
### END INIT INFO

. /usr/local/bin/openbmc-utils.sh

# Disable the dual boot watch dog
devmem_clear_bit 0x1e78502c 0

#
# "watchdogd" is only needed in legacy kernel (v4.1), and the watchdog can
# be automatically kicked by a kernel thread in the latest kernel.
#
if uname -r | grep "4\.1\.*" > /dev/null 2>&1; then
    /usr/bin/watchdogd.sh > /dev/null 2>&1 &
fi
