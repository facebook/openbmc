#!/bin/bash
#
# Copyright 2020-present Facebook. All Rights Reserved.
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
#shellcheck disable=SC1091
. /usr/local/bin/openbmc-utils.sh

eeprom_file=/sys/bus/i2c/devices/8-0054/eeprom
crc_cal_file=/usr/local/bin/crc_16.py

crc=$(python $crc_cal_file $eeprom_file check)
if [[ $crc -eq 1 ]];then
    echo "CRC is Valid"
else
    echo "Invalid CRC ... Fatal Error !"
fi

read_x86_mac_from_eeprom
