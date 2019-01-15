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

. /usr/local/fbpackages/utils/ast-functions

gpio_set F4 1
gpio_set F5 1

if [[ $(gpio_get_val L0) == "0" && $(gpio_get_val L1) == "1" ]]; then
  val=0
  for i in {1..3}; do
    val=$(i2cget -y -f 12 0x20 0 b)
    val=$((val & 0x0f))
    [ $val -eq 15 ] && break
    usleep 100000
  done

  if [ $val -ne 15 ]; then
    i2cset -y -f 12 0x20 6 0xffff w
    i2cset -y -f 12 0x20 2 0xffff w
    usleep 200000
    i2cset -y -f 12 0x20 6 0xfe b
    sleep 1
    i2cset -y -f 12 0x20 6 0xfc b
  fi
fi
