#!/bin/bash
#
# Copyright 2023-present Facebook. All Rights Reserved.
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

serial_num=$(weutil -e scm | grep '^Product Serial Number:' | awk '{print $4}')

if [[ "$serial_num" =~ JAS231406(4[5-9]|5[0-9]|6[0-4]) ]]; then
    echo "Using 32M SPI flash layout"
    cp /etc/meru_flash.layout /etc/meru_flash_64m.layout
    cp /etc/meru_flash_32m.layout /etc/meru_flash.layout
fi
