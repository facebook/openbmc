#!/bin/bash
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

usage() {
    echo "Usage: $1 <on | off>"
    exit -1
}

. /usr/local/bin/openbmc-utils.sh

PATH=/sbin:/bin:/usr/sbin:/usr/bin

set -e

if [ $# != 1 ]; then
    usage $0
fi

if [ $1 = "on" ]; then
    val=1
elif [ $1 = "off" ]; then
    val=0
else
    usage $0
fi

# To use GPIOE5 (37), SCU80[21], SCU8C[14], and SCU70[22] must be 0
devmem_clear_bit $(scu_addr 80) 21
devmem_clear_bit $(scu_addr 8C) 14
devmem_clear_bit $(scu_addr 70) 22

gpio_set 37 $val
