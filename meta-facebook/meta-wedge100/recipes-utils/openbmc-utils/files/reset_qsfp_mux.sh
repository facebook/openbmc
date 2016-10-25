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

function reset_to_mux
{
  local num=0 val=$1
  while [ $num -lt 4 ]; do
    CMD='/sys/class/i2c-adapter/i2c-12/12-0031/i2c_mux'$num'_rst_n'
    echo $val > $CMD
    (( num++ ))
  done
}

echo -n "Reset QSFP i2c-mux ... "
reset_to_mux 0
usleep 50000
reset_to_mux 1
echo "Done"
