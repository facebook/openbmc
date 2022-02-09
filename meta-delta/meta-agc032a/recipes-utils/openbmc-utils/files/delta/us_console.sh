#!/bin/sh
#
# Copyright 2019-present Facebook. All Rights Reserved.
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

# GPIOS5 (32) controls if uS console connects to UART5 or not.
# To enable GPIOS5, SCU8C[5] must be 0
devmem_clear_bit $(scu_addr 8C) 5

gpiocli --shadow BMC_UART_SEL5 set-direction out
gpiocli --shadow BMC_UART_SEL5 set-value $VALUE