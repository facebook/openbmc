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

usage() {
    echo "$0 <connect | disconnect>"
}

. /usr/local/bin/openbmc-utils.sh

if [ $# -ne 1 ]; then
    usage
    exit 1
fi

if [ "$1" == "connect" ]; then
    VALUE=1
elif [ "$1" == "disconnect" ]; then
    VALUE=0
else
    usage
    exit 1
fi

# GPIOE0 (32) controls if uS console connects to UART1 or not.
# To enable GPIOE0, SCU80[16], SCU8C[12], and SCU70[22] must be 0
devmem_clear_bit $(scu_addr 80) 16
devmem_clear_bit $(scu_addr 8C) 12
devmem_clear_bit $(scu_addr 70) 22

gpio_set 32 $VALUE
