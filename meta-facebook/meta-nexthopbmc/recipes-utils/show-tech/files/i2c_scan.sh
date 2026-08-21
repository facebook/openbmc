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
. /usr/local/bin/openbmc-utils.sh

scan_i2cbus() {
    # nexthopbmc enabled I2C buses. Bus 4 holds the BMC IDPROM and bus 8 the
    # chassis EEPROM; bus 11 is disabled in the device tree (mdio3 shares its
    # pins) so it is intentionally omitted.
    local buses=(0 1 2 3 4 5 6 7 8 9 10 12 13 14 15)

    for bus in "${buses[@]}"; do
        echo -e "##### I2C Bus${bus} INFO #####"
        # i2cdetect command
        timeout 5 i2cdetect -y "$bus"

        echo ""
    done
}
scan_i2cbus
