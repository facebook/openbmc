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

. /usr/local/bin/openbmc-utils.sh

echo -n "Reset USB Switch ... "

# To use GPIOD7 (31), SCU90[1], SCU8C[11], and SCU70[21] must be 0
devmem_clear_bit $(scu_addr 8C) 11
devmem_clear_bit $(scu_addr 70) 21
devmem_clear_bit $(scu_addr 90) 1

gpio_set 31 1
sleep 1
gpio_set 31 0
sleep 1
gpio_set 31 1

echo "Done"
