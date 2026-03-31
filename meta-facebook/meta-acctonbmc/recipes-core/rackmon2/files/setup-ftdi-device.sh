#!/bin/bash
#
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

# Get RS485 enable pin status
rs485_cfg=0
rs485_cfg=$((rs485_cfg + $(/usr/local/bin/ftdi_control -o | grep CHANNEL_B_RS485 | grep -c 1 ) ))
rs485_cfg=$((rs485_cfg + $(/usr/local/bin/ftdi_control -o | grep CHANNEL_C_RS485 | grep -c 1 ) ))
rs485_cfg=$((rs485_cfg + $(/usr/local/bin/ftdi_control -o | grep CHANNEL_D_RS485 | grep -c 1 ) ))
if [ $((rs485_cfg)) -lt 3 ]; then
   /usr/local/bin/ftdi_control -N -B1 -C1 -D1
   touch /tmp/need_to_pwrcycle
   echo "setup-ftdi-device.sh : Need to power cycle to reload the FTDI configuration"
   exit 1
fi

if [ -e /tmp/need_to_pwrcycle ]; then
    echo "setup-ftdi-device.sh : Need to power cycle to reload the FTDI configuration"
    exit 1
fi
