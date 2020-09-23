#!/bin/sh
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
#

usage()
{
  echo "usage :"
  echo "   $0 <voltage in mv>"
  echo "   eg. $0 850"
  echo " limit : max 930 mV"
}

if [ $# -ne 1 ]; then
  usage
  exit 255
fi

if [ $(($1)) -gt 931 ]; then
  usage
  exit 255
fi

i2cset -f -y 17 0x40 0x00 0x00
i2cset -f -y 17 0x40 0x20 0x14    # linear mode  ,, N = 0x12
value=$(( $1 * 1000 / 244 ))
i2cset -f -y 17 0x40 0x21 $value w

if [ $? -eq 0 ]; then
  echo "$1"
  exit 0
else
  echo "setting error"
  exit 254
fi
