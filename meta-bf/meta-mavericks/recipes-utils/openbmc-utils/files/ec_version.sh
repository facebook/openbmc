#!/bin/bash
#
# Copyright 2015-present Facebook. All Rights Reserved.
#
# This program file is free software; you can redistribute it and/or modify it
# under the terms of the GNU General Public License as published by the
# Free Software Foundation; version 2 of the License.
#
# This program is distributed in the hope that it will be useful, but WITHOUT
# ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
# FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
# for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program in a file named COPYING; if not, write to the
# Free Software Foundation, Inc.,
# 51 Franklin Street, Fifth Floor,
# Boston, MA 02110-1301 USA
#

version=$(cat /sys/class/i2c-adapter/i2c-4/4-0033/version)
released_date=$(cat /sys/class/i2c-adapter/i2c-4/4-0033/date)

if [ "${version:(-4)}" == "0x80" ]; then
  echo "EC Test Version"
  echo "EC Released Date: $released_date"
else
  r=$((16#${version:15:2}))
  e=$((16#${version:22:2}))
  echo "EC Released Version: R$(printf '%02d' $r).E$(printf '%02d' $e)"
  echo "EC Released Date: $released_date"
fi
