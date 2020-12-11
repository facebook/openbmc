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

# Provides:          xdpe12284 setup on EVT3
# Short-Description: Some EVT3 units xdpe12284 has uncorrect firmware, BMC need
#                    to set xdpe12284 registers to change max power supply as
#                    power required.

# shellcheck disable=SC1091
. /usr/local/bin/openbmc-utils.sh

# Only some EVT3 need to set xdpe12284 power
[ "$(wedge_board_type_rev)" != "Cloudripper_EVT3" ] && exit 0

echo -n "Setup xdpe12284 on EVT3 unit ..."

# set register value n~m bits as expected
# $1: current value
# $2: start bit
# $3: end bit
# $4: start-end bits expect value
update_reg() {
    reg=$1
    startbit=$2
    endbit=$3
    data=$4

    ((lsb_mask = ((0xffff) >> startbit) << startbit))
    ((msb_mask = ((0xffff) >> (15 - endbit)) ))
    ((mask = lsb_mask & msb_mask))

    reg=$((reg & ~mask))
    reg=$((reg | ((data << startbit) & mask)))

    printf 0x%x $reg
}

for i in $(seq 1 2)
do
    if [ "$i" -eq 1 ]; then
        XPDE_BUS="19"
        XPDE_ADDR="0x68"
    elif [ "$i" -eq 2 ]; then
        XPDE_BUS="22"
        XPDE_ADDR="0x60"
    fi

    # We need to use i2c get/set instead of sysfs nodes because the pmbus driver not support.
    # set page to 0x20
    i2cset -y -f $XPDE_BUS $XPDE_ADDR 0x00 0x20
    val=$(i2cget -f -y $XPDE_BUS $XPDE_ADDR 0x9 w)      #   page addr start end expect
    val=$(update_reg "$val" 10 15 0x14)                 #   0x20  0x9   10   15  0x14
    i2cset -y -f $XPDE_BUS $XPDE_ADDR 0x9 "$val" w
    val=$(i2cget -f -y $XPDE_BUS $XPDE_ADDR 0xA w)
    val=$(update_reg "$val" 10 15 0x24)                 #   0x20  0xA   10   15  0x24
    i2cset -y -f $XPDE_BUS $XPDE_ADDR 0xA "$val" w
    val=$(i2cget -f -y $XPDE_BUS $XPDE_ADDR 0xB w)
    val=$(update_reg "$val" 6 11 0x14)                  #   0x20  0xB   6    11  0x14
    i2cset -y -f $XPDE_BUS $XPDE_ADDR 0xB "$val" w
    val=$(i2cget -f -y $XPDE_BUS $XPDE_ADDR 0x11 w)
    val=$(update_reg "$val" 6 10 0x2)                   #   0x20  0x11  6    10  0x2
    i2cset -y -f $XPDE_BUS $XPDE_ADDR 0x11 "$val" w
    val=$(i2cget -f -y $XPDE_BUS $XPDE_ADDR 0x25 w)
    val=$(update_reg "$val" 0 5  0x1E)                  #   0x20  0x25  0    5   0x1E
    i2cset -y -f $XPDE_BUS $XPDE_ADDR 0x25 "$val" w
    val=$(i2cget -f -y $XPDE_BUS $XPDE_ADDR 0x26 w)
    val=$(update_reg "$val" 0 6  0x75)                  #   0x20  0x26  0    6   0x75
    i2cset -y -f $XPDE_BUS $XPDE_ADDR 0x26 "$val" w

    # set page to 0x21
    i2cset -y -f $XPDE_BUS $XPDE_ADDR 0x00 0x21
    val=$(i2cget -f -y $XPDE_BUS $XPDE_ADDR 0x2 w)
    val=$(update_reg "$val" 0 9  0x50)                  #   0x21  0x2   0    9   0x50
    i2cset -y -f $XPDE_BUS $XPDE_ADDR 0x2 "$val" w
    val=$(i2cget -f -y $XPDE_BUS $XPDE_ADDR 0x5 w)
    val=$(update_reg "$val" 8 10 0x6)                   #   0x21  0x5   8    10  0x6
    i2cset -y -f $XPDE_BUS $XPDE_ADDR 0x5 "$val" w
    val=$(i2cget -f -y $XPDE_BUS $XPDE_ADDR 0x9 w)
    val=$(update_reg "$val" 10 15 0x1E)                 #   0x21  0x9   10   15  0x1E
    i2cset -y -f $XPDE_BUS $XPDE_ADDR 0x9 "$val" w
    val=$(i2cget -f -y $XPDE_BUS $XPDE_ADDR 0xA w)
    val=$(update_reg "$val" 10 15 0x1E)                 #   0x21  0xA   10   15  0x1E
    i2cset -y -f $XPDE_BUS $XPDE_ADDR 0xA "$val" w
    val=$(i2cget -f -y $XPDE_BUS $XPDE_ADDR 0xB w)
    val=$(update_reg "$val" 6 11 0x14)                  #   0x21  0xB   6    11  0x14
    i2cset -y -f $XPDE_BUS $XPDE_ADDR 0xB "$val" w
    val=$(i2cget -f -y $XPDE_BUS $XPDE_ADDR 0xE w)
    val=$(update_reg "$val" 0 5  0x29)                  #   0x21  0xE   0    5   0x29
    i2cset -y -f $XPDE_BUS $XPDE_ADDR 0xE "$val" w
    val=$(i2cget -f -y $XPDE_BUS $XPDE_ADDR 0x1E w)
    val=$(update_reg "$val" 4 7  0x2)                   #   0x21  0x1E  4    7   0x2
    i2cset -y -f $XPDE_BUS $XPDE_ADDR 0x1E "$val" w
    val=$(i2cget -f -y $XPDE_BUS $XPDE_ADDR 0x23 w)
    val=$(update_reg "$val" 6 15 0x3C)                  #   0x21  0x23  6    15  0x3C
    i2cset -y -f $XPDE_BUS $XPDE_ADDR 0x23 "$val" w
    val=$(i2cget -f -y $XPDE_BUS $XPDE_ADDR 0x25 w)
    val=$(update_reg "$val" 0 5  0x14)                  #   0x21  0x25  0    5   0x14
    val=$(update_reg "$val" 6 15 0x50)                  #   0x21  0x25  6    15  0x50
    i2cset -y -f $XPDE_BUS $XPDE_ADDR 0x25 "$val" w
    val=$(i2cget -f -y $XPDE_BUS $XPDE_ADDR 0x27 w)
    val=$(update_reg "$val" 0 7  0x2A)                  #   0x21  0x27  0    7   0x2A
    i2cset -y -f $XPDE_BUS $XPDE_ADDR 0x27 "$val" w
    val=$(i2cget -f -y $XPDE_BUS $XPDE_ADDR 0x2C w)
    val=$(update_reg "$val" 0 3  0x2)                   #   0x21  0x2C  0    3   0x2
    val=$(update_reg "$val" 4 7  0x2)                   #   0x21  0x2C  4    7   0x2
    val=$(update_reg "$val" 8 11 0x2)                   #   0x21  0x2C  8    11  0x2
    val=$(update_reg "$val" 12 15 0x2)                  #   0x21  0x2C  12   15  0x2
    i2cset -y -f $XPDE_BUS $XPDE_ADDR 0x2C "$val" w
    val=$(i2cget -f -y $XPDE_BUS $XPDE_ADDR 0x37 w)
    val=$(update_reg "$val" 0 7  0x28)                  #   0x21  0x37  0    7   0x28
    val=$(update_reg "$val" 8 11 0x0)                   #   0x21  0x37  8    11  0x0
    i2cset -y -f $XPDE_BUS $XPDE_ADDR 0x37 "$val" w
    val=$(i2cget -f -y $XPDE_BUS $XPDE_ADDR 0x53 w)
    val=$(update_reg "$val" 0 6  0xC)                   #   0x21  0x53  0    6   0xC
    i2cset -y -f $XPDE_BUS $XPDE_ADDR 0x53 "$val" w

    # set page to 0x22
    i2cset -y -f $XPDE_BUS $XPDE_ADDR 0x00 0x22
    val=$(i2cget -f -y $XPDE_BUS $XPDE_ADDR 0x45 w)
    val=$(update_reg "$val" 0 3  0x0)                   #   0x22  0x45  0    3   0x0
    i2cset -y -f $XPDE_BUS $XPDE_ADDR 0x45 "$val" w

    # reset page to 0x0
    i2cset -f -y $XPDE_BUS $XPDE_ADDR 0x0 0x0
done

echo "done."
