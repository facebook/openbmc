# Copyright 2015-present Facebook. All Rights Reserved.
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

DEVMEM=/sbin/devmem

devmem_set_bit() {
    local addr
    local val
    addr=$1
    val=$($DEVMEM $addr)
    val=$((val | (0x1 << $2)))
    $DEVMEM $addr 32 $val
}

devmem_clear_bit() {
    local addr
    local val
    addr=$1
    val=$($DEVMEM $addr)
    val=$((val & ~(0x1 << $2)))
    $DEVMEM $addr 32 $val
}

#
# Test if the given bit is set in register <addr>
# $1 - register address
# $2 - bit position
#
devmem_test_bit() {
    local addr=$1
    local bitpos=$2
    local val

    val=$($DEVMEM $addr)
    val=$(((val >> $bitpos) & 0x1))

    if [ $val -eq 1 ]; then
        # 0 for true
        return 0
    else
        # 1 for false
        return 1
    fi
}

source "/usr/local/bin/shell-utils.sh"
source "/usr/local/bin/i2c-utils.sh"
source "/usr/local/bin/gpio-utils.sh"

if [ -f "/usr/local/bin/soc-utils.sh" ]; then
    source "/usr/local/bin/soc-utils.sh"
fi

if [ -f "/usr/local/bin/board-utils.sh" ]; then
    source "/usr/local/bin/board-utils.sh"
fi
