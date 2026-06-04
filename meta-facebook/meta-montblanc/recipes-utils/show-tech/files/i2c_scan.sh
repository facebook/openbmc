#!/bin/bash
#
# Copyright (c) Meta Platforms, Inc. and affiliates. (http://www.meta.com)
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

# shellcheck disable=SC1091
# read the Hardware revision from MCB CPLD register
. /usr/local/bin/openbmc-utils.sh

scan_i2cbus() {
    # Define bus list
    local buses=(0 1 2 3 4 5 6 7 8 9 10 12 13)
    
    for bus in "${buses[@]}"; do
        echo -e "##### I2C Bus${bus} INFO #####"
        # i2cdetect command
        timeout 5 i2cdetect -y "$bus"
        echo ""
    done
}
scan_i2cbus