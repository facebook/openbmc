#!/bin/bash
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

. /usr/local/bin/openbmc-utils.sh

# Make sure RESET_SEQ0 and RESET_SEQ1 are 0
gpio_set RESET_SEQ0 0
gpio_set RESET_SEQ1 0

# Now toggle T2_RESET_N
gpio_set T2_RESET_N 0
sleep 1
gpio_set T2_RESET_N 1

logger "Reset T2 with both SEQ0 and SEQ1 low"
