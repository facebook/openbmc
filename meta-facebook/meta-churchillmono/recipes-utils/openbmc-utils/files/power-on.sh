#!/bin/bash
#
# Copyright (c) 2022 Cisco Systems Inc.
# Copyright (c) Meta Platforms, Inc. and affiliates.
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
# Required-Start:    85
# Required-Stop:
# Default-Start:     S
# Default-Stop:
# Short-Description: Power on micro-server
### END INIT INFO

#shellcheck disable=SC1091
. /usr/local/bin/openbmc-utils.sh

PATH=/sbin:/bin:/usr/sbin:/usr/bin:/usr/local/bin

# Power on if not already powered on
wedge_power.sh on
