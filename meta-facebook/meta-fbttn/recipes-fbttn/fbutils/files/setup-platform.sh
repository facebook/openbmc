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

# Get platform SKU
sh /usr/local/bin/check_pal_sku.sh > /dev/NULL
PAL_SKU=$?
echo $PAL_SKU > /tmp/system.bin

# Record the platform SKU in syslog
IOM_TYPE=$(($PAL_SKU & 0xF))
IOM_SLOTID=$(($(($PAL_SKU >> 4)) & 0x3))

if [ $IOM_TYPE -eq 1 ]; then
    if [ $IOM_SLOTID -eq 1 ]; then
        IOM_LOCAL="side A"
    elif [ $IOM_SLOTID -eq 2 ]; then
        IOM_LOCAL="side B"
    else
        IOM_LOCAL="unknown"
    fi
    logger -s -p user.info -t setup-platform "System: type 5, IOM location: $IOM_LOCAL"
 elif [ $IOM_TYPE -eq 2 ]; then
    logger -s -p user.info -t setup-platform "System: type 7"
 else
    logger -s -p user.info -t setup-platform "System: unknown"
 fi
