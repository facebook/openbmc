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

# shellcheck source=/dev/null
source /usr/local/bin/openbmc-utils.sh

echo -e "\n##### MCB FPGA i2c dump (Mapped to Register Specification) #####"
for NUM_I2C in {0..255}; do
    HEX_ADDR=$(printf "0x%x" $((NUM_I2C * 4)))
    VALUE=$(i2cget -f -y 12 0x72 "$NUM_I2C")
    echo "$HEX_ADDR: $VALUE"

    # 0xfc is the end of supported offsets
    if [[ "$HEX_ADDR" == "0xfc" ]]; then
        break
    fi
done
