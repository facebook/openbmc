#!/bin/bash
#
# Copyright 2025-present Facebook. All Rights Reserved.
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

META_EEPROM_OFFSET=$((15 * 1024))

bmc_has_meta_eeprom() {
    # Check if the BMC EEPROM has been programmed with Meta EEPROM format.
    local eeprom_file="/sys/bus/i2c/drivers/at24/0-0050/eeprom"
    local meta_hdr_bytes="fbfb[0-9a-f]{2}ff"
    bmc_hdr=$(hexdump -e '16/1 "%02x" "\n"' -n 4 -s ${META_EEPROM_OFFSET} \
              ${eeprom_file} | awk '{$1=$1};1')
    if [[ "${bmc_hdr}" =~ $meta_hdr_bytes ]]; then
        return 0;
    fi
    return 1
}
