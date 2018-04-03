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

# Get slot type
sh /usr/local/bin/check_slot_type.sh > /dev/NULL
SLOT_TYPE=$?
echo $SLOT_TYPE > /tmp/slot.bin


# do init power
echo "[ T6A ] : Start setup PCA9555"
do_init_power=0;

ret=$(i2cget -y 12 0x20  0x02 w)
if [ "$ret" != "0xd7fd" ]; then
    do_init_power=1;
fi

ret=$(i2cget -y 12 0x20  0x06 w)
if [ "$ret" != "0xd18d" ]; then
    do_init_power=1;
fi

if [ "$do_init_power" -eq "1" ]; then
    # set 0x02 output register, 0xFD port0, 0xD7 port1
    i2cset -y 12 0x20  0x02 0xFD 0xD7 i
    # set 0x06 dir register, 0xFD port0, 0xD3 port1
    i2cset -y 12 0x20  0x06 0xFD 0xD3 i
    usleep 2000;

    # set P0V9 as high
    i2cset -y 12 0x20  0x06 0xDD 0xD3 i
    usleep 2000;

    # set P1V8 as high
    i2cset -y 12 0x20  0x06 0x9D 0xD3 i
    usleep 2000;

    # set OSC_OE as high
    i2cset -y 12 0x20  0x06 0x9D 0xD3 i
    usleep 2000;

    # set RST_PLT_MEZZ_N as high
    i2cset -y 12 0x20  0x06 0x8D 0xD1 i
    usleep 2000;

    echo "[ T6A ] : setup PCA9555 finish !!!"
else
    echo "[ T6A ] : no need to setup PCA9555 !!!"
fi
